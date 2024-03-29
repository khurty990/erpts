/* ==================================================================== 
 * The Kannel Software License, Version 1.0 
 * 
 * Copyright (c) 2001-2004 Kannel Group  
 * Copyright (c) 1998-2001 WapIT Ltd.   
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *    the documentation and/or other materials provided with the 
 *    distribution. 
 * 
 * 3. The end-user documentation included with the redistribution, 
 *    if any, must include the following acknowledgment: 
 *       "This product includes software developed by the 
 *        Kannel Group (http://www.kannel.org/)." 
 *    Alternately, this acknowledgment may appear in the software itself, 
 *    if and wherever such third-party acknowledgments normally appear. 
 * 
 * 4. The names "Kannel" and "Kannel Group" must not be used to 
 *    endorse or promote products derived from this software without 
 *    prior written permission. For written permission, please  
 *    contact org@kannel.org. 
 * 
 * 5. Products derived from this software may not be called "Kannel", 
 *    nor may "Kannel" appear in their name, without prior written 
 *    permission of the Kannel Group. 
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED.  IN NO EVENT SHALL THE KANNEL GROUP OR ITS CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,  
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR  
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,  
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE  
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 * ==================================================================== 
 * 
 * This software consists of voluntary contributions made by many 
 * individuals on behalf of the Kannel Group.  For more information on  
 * the Kannel Group, please see <http://www.kannel.org/>. 
 * 
 * Portions of this software are based upon software originally written at  
 * WapIT Ltd., Helsinki, Finland for the Kannel project.  
 */ 

/*
 * bearerbox.c
 * 
 * this is the core module of the bearerbox. It starts everything and
 * listens to HTTP requests and traps signals.
 * All started modules are responsible for the rest.
 *
 * Kalle Marjola <rpr@wapit.com> 2000 for project Kannel
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "gwlib/gwlib.h"
#include "msg.h"
#include "bearerbox.h"
#include "shared.h"
#include "dlr.h"

/* global variables; included to other modules as needed */

List *incoming_sms;
List *outgoing_sms;

List *incoming_wdp;
List *outgoing_wdp;

Counter *incoming_sms_counter;
Counter *outgoing_sms_counter;
Counter *incoming_wdp_counter;
Counter *outgoing_wdp_counter;

/* incoming/outgoing sms queue control */
long max_incoming_sms_qlength;



/* this is not a list of items; instead it is used as
 * indicator to note how many threads we have.
 * ALL flow threads must exit before we may safely change
 * bb_status from BB_SHUTDOWN to BB_DEAD
 *
 * XXX: prehaps we could also have items in this list, as
 *     descriptors of each thread?
 */
List *flow_threads;

/* and still more abuse; we use this list to put us into
 * 'suspend' state - if there are any producers (only core adds/removes them)
 * receiver/sender systems just sit, blocked in list_consume
 */
List *suspended;

/* this one is like 'suspended', but only for receiving UDP/SMSC
 * (suspended state puts producers for both lists)
 */
List *isolated;

volatile sig_atomic_t bb_status;

/* 
 * Flags for main thread to check what is to do.
 */
enum {
    BB_LOGREOPEN = 1,
    BB_CHECKLEAKS = 2
};
/* Here we will set above flags */
static volatile sig_atomic_t bb_todo = 0;

/* own global variables */

static Mutex *status_mutex;
static time_t start_time;
volatile sig_atomic_t restart = 0;


/* to avoid copied code */

static void set_shutdown_status(void)
{
    sig_atomic_t old = bb_status;
    bb_status = BB_SHUTDOWN;
    
    if (old == BB_SUSPENDED)
	list_remove_producer(suspended);
    if (old == BB_SUSPENDED || old == BB_ISOLATED)
	list_remove_producer(isolated);
}


/*-------------------------------------------------------
 * signals
 */

static void signal_handler(int signum)
{
    /* On some implementations (i.e. linuxthreads), signals are delivered
     * to all threads.  We only want to handle each signal once for the
     * entire box, and we let the gwthread wrapper take care of choosing
     * one.
     */
    if (!gwthread_shouldhandlesignal(signum))
	return;

    switch (signum) {
        case SIGINT:
        case SIGTERM:
            if (bb_status != BB_SHUTDOWN && bb_status != BB_DEAD) {
                bb_status = BB_SHUTDOWN;
            }
            else if (bb_status == BB_SHUTDOWN) {
                bb_status = BB_DEAD;
            }
            else if (bb_status == BB_DEAD) {
                panic(0, "Cannot die by its own will");
            }
            break;

        case SIGHUP:
            bb_todo |= BB_LOGREOPEN;
            break;
        
        /* 
         * It would be more proper to use SIGUSR1 for this, but on some
         * platforms that's reserved by the pthread support. 
         */
        case SIGQUIT:
           bb_todo |= BB_CHECKLEAKS;  
           break;
    }
}

static void setup_signal_handlers(void)
{
    struct sigaction act;

    act.sa_handler = signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGPIPE, &act, NULL);
}




/*--------------------------------------------------------
 * functions to start/init sub-parts of the bearerbox
 *
 * these functions are NOT thread safe but they have no need to be,
 * as there is only one core bearerbox thread
 */



static int start_smsc(Cfg *cfg)
{
    static int started = 0;
    if (started) return 0;

    smsbox_start(cfg);

    smsc2_start(cfg);

    started = 1;
    return 0;
}


static void wdp_router(void *arg)
{
    Msg *msg;

    list_add_producer(flow_threads);
    
    while(bb_status != BB_DEAD) {

	if ((msg = list_consume(outgoing_wdp)) == NULL)
	    break;

	gw_assert(msg_type(msg) == wdp_datagram);
	
	/*
	if (msg->list == sms)
	    smsc_addwdp(msg);
	else
	*/

	udp_addwdp(msg);
    }
    udp_die();
    /* smsc_endwdp(); */

    list_remove_producer(flow_threads);
}

static int start_wap(Cfg *cfg)
{
    static int started = 0;
    if (started) return 0;

    
    wapbox_start(cfg);

    debug("bb", 0, "starting WDP router");
    if (gwthread_create(wdp_router, NULL) == -1)
	panic(0, "Failed to start a new thread for WDP routing");

    started = 1;
    return 0;
}


static int start_udp(Cfg *cfg)
{
    static int started = 0;
    if (started) return 0;

    udp_start(cfg);

    start_wap(cfg);
    started = 1;
    return 0;
}


/*
 * check that there is basic thingies in configuration
 */
static int check_config(Cfg *cfg)
{
    CfgGroup *grp;
    long smsp, wapp;

    grp = cfg_get_single_group(cfg, octstr_imm("core"));
    if (grp == NULL)
    	return -1;

    if (cfg_get_integer(&smsp, grp, octstr_imm("smsbox-port")) == -1)
    	smsp = -1;
    if (cfg_get_integer(&wapp, grp, octstr_imm("wapbox-port")) == -1)
    	wapp = -1;
    
#ifndef NO_SMS    
    grp = cfg_get_single_group(cfg, octstr_imm("smsbox"));
    if (smsp != -1 && grp == NULL) {
	error(0, "No 'smsbox' group in configuration, but smsbox-port set");
	return -1;
    }
#endif
    
#ifndef NO_WAP	
    grp = cfg_get_single_group(cfg, octstr_imm("wapbox"));
    if (wapp != -1 && grp == NULL) {
	error(0, "No 'wapbox' group in configuration, but wapbox-port set");
	return -1;
    }
#endif
    
    return 0;
}

/*
 * check our own variables
 */

static int check_args(int i, int argc, char **argv) {
    if (strcmp(argv[i], "-S")==0 || strcmp(argv[i], "--suspended")==0)
	bb_status = BB_SUSPENDED;
    else if (strcmp(argv[i], "-I")==0 || strcmp(argv[i], "--isolated")==0)
	bb_status = BB_ISOLATED;
    else
        return -1;

    return 0;
}


static Cfg *init_bearerbox(Cfg *cfg)
{
    CfgGroup *grp;
    Octstr *log, *val;
    long loglevel;
    int lf, m;
#ifdef HAVE_LIBSSL
    Octstr *ssl_server_cert_file;
    Octstr *ssl_server_key_file;
    int ssl_enabled = 0;
#endif /* HAVE_LIBSSL */

    /* defaults: use localtime and markers for access-log */
    lf = m = 1;
	
    grp = cfg_get_single_group(cfg, octstr_imm("core"));

    log = cfg_get(grp, octstr_imm("log-file"));
    if (log != NULL) {
	if (cfg_get_integer(&loglevel, grp, octstr_imm("log-level")) == -1)
	    loglevel = 0;
	log_open(octstr_get_cstr(log), loglevel, GW_NON_EXCL);
	octstr_destroy(log);
    }

    if (check_config(cfg) == -1)
        panic(0, "Cannot start with corrupted configuration");

    /* determine which timezone we use for access logging */
    if ((log = cfg_get(grp, octstr_imm("access-log-time"))) != NULL) {
        lf = (octstr_case_compare(log, octstr_imm("gmt")) == 0) ? 0 : 1;
        octstr_destroy(log);
    }

    /* should predefined markers be used, ie. prefixing timestamp */
    cfg_get_bool(&m, grp, octstr_imm("access-log-clean"));

    /* custom access-log format  */
    if ((log = cfg_get(grp, octstr_imm("access-log-format"))) != NULL) {
        bb_alog_init(log);
        octstr_destroy(log);
    }

    /* open access-log file */
    if ((log = cfg_get(grp, octstr_imm("access-log"))) != NULL) {
        alog_open(octstr_get_cstr(log), lf, m ? 0 : 1);
        octstr_destroy(log);
    }

    log = cfg_get(grp, octstr_imm("store-file"));
    /* initialize the store file */
    if (log != NULL) {
        store_init(log);
        octstr_destroy(log);
    }

    conn_config_ssl (grp);

    /* 
     * Make sure we have "ssl-server-cert-file" and "ssl-server-key-file" specified
     * in the core group since we need it to run SSL-enabled internal box 
     * connections configured via "smsbox-port-ssl = yes" and "wapbox-port-ssl = yes".
     * Check only these, because for "admin-port-ssl" and "sendsms-port-ssl" for the 
     * SSL-enabled HTTP servers are probed within gw/bb_http.c:httpadmin_start()
     */
#ifdef HAVE_LIBSSL
    ssl_server_cert_file = cfg_get(grp, octstr_imm("ssl-server-cert-file"));
    ssl_server_key_file = cfg_get(grp, octstr_imm("ssl-server-key-file"));
    if (ssl_server_cert_file != NULL && ssl_server_key_file != NULL) {
       /* we are fine, at least files are specified in the configuration */
    } else {
        cfg_get_bool(&ssl_enabled, grp, octstr_imm("smsbox-port-ssl"));
        cfg_get_bool(&ssl_enabled, grp, octstr_imm("wapbox-port-ssl"));
        if (ssl_enabled) {
	       panic(0, "You MUST specify cert and key files within core group for SSL-enabled inter-box connections!");
        }
    }
    octstr_destroy(ssl_server_cert_file);
    octstr_destroy(ssl_server_key_file);
#endif /* HAVE_LIBSSL */
	
    /* if all seems to be OK by the first glimpse, real start-up */
    
    outgoing_sms = list_create();
    incoming_sms = list_create();
    outgoing_wdp = list_create();
    incoming_wdp = list_create();

    outgoing_sms_counter = counter_create();
    incoming_sms_counter = counter_create();
    outgoing_wdp_counter = counter_create();
    incoming_wdp_counter = counter_create();
    
    status_mutex = mutex_create();

    setup_signal_handlers();

    
    /* http-admin is REQUIRED */
    httpadmin_start(cfg);

    if (cfg_get_integer(&max_incoming_sms_qlength, grp,
                           octstr_imm("maximum-queue-length")) == -1)
        max_incoming_sms_qlength = -1;
    else {
        warning(0, "Option 'maximum-queue-length' is deprecated! Please use"
                          " 'sms-incoming-queue-limit' instead!");
    }

    if (max_incoming_sms_qlength == -1 &&
        cfg_get_integer(&max_incoming_sms_qlength, grp,
                                  octstr_imm("sms-incoming-queue-limit")) == -1)
        max_incoming_sms_qlength = -1;

#ifndef NO_SMS    
    {
	List *list;
	
	list = cfg_get_multi_group(cfg, octstr_imm("smsc"));
	if (list != NULL) {
	    start_smsc(cfg);
	    list_destroy(list, NULL);
	}
    }
#endif
    
#ifndef NO_WAP
    grp = cfg_get_single_group(cfg, octstr_imm("core"));
    val = cfg_get(grp, octstr_imm("wdp-interface-name"));
    if (val != NULL && octstr_len(val) > 0)
	start_udp(cfg);
    octstr_destroy(val);

    if (cfg_get_single_group(cfg, octstr_imm("wapbox")) != NULL)
	start_wap(cfg);
#endif
    
    return cfg;
}


static void empty_msg_lists(void)
{
    Msg *msg;

#ifndef NO_WAP

    if (list_len(incoming_wdp) > 0 || list_len(outgoing_wdp) > 0)
	warning(0, "Remaining WDP: %ld incoming, %ld outgoing",
	      list_len(incoming_wdp), list_len(outgoing_wdp));

    info(0, "Total WDP messages: received %ld, sent %ld",
	 counter_value(incoming_wdp_counter),
	 counter_value(outgoing_wdp_counter));
#endif
    
    while((msg = list_extract_first(incoming_wdp))!=NULL)
	msg_destroy(msg);
    while((msg = list_extract_first(outgoing_wdp))!=NULL)
	msg_destroy(msg);

    list_destroy(incoming_wdp, NULL);
    list_destroy(outgoing_wdp, NULL);

    counter_destroy(incoming_wdp_counter);
    counter_destroy(outgoing_wdp_counter);
    
    
#ifndef NO_SMS

    /* XXX we should record these so that they are not forever lost...
     */
    if (list_len(incoming_sms) > 0 || list_len(outgoing_sms) > 0)
	debug("bb", 0, "Remaining SMS: %ld incoming, %ld outgoing",
	      list_len(incoming_sms), list_len(outgoing_sms));

    info(0, "Total SMS messages: received %ld, sent %ld",
	 counter_value(incoming_sms_counter),
	 counter_value(outgoing_sms_counter));

#endif

    list_destroy(incoming_sms, msg_destroy_item);
    list_destroy(outgoing_sms, msg_destroy_item);
    
    counter_destroy(incoming_sms_counter);
    counter_destroy(outgoing_sms_counter);
}


int main(int argc, char **argv)
{
    int cf_index;
    Cfg *cfg;
    Octstr *filename;

    bb_status = BB_RUNNING;
    
    gwlib_init();
    start_time = time(NULL);

    suspended = list_create();
    isolated = list_create();
    list_add_producer(suspended);
    list_add_producer(isolated);

    cf_index = get_and_set_debugs(argc, argv, check_args);

    if (argv[cf_index] == NULL)
        filename = octstr_create("kannel.conf");
    else
        filename = octstr_create(argv[cf_index]);
    cfg = cfg_create(filename); 
    
    if (cfg_read(cfg) == -1)
        panic(0, "Couldn't read configuration from `%s'.", octstr_get_cstr(filename));
    
    octstr_destroy(filename);

    dlr_init(cfg);
    
    report_versions("bearerbox");

    flow_threads = list_create();
    
    init_bearerbox(cfg);

    info(0, "----------------------------------------");
    info(0, GW_NAME " bearerbox II version %s starting", GW_VERSION);


    gwthread_sleep(5.0); /* give time to threads to register themselves */

    if (store_load()== -1)
	panic(0, "Cannot start with store-file failing");
    
    info(0, "MAIN: Start-up done, entering mainloop");
    if (bb_status == BB_SUSPENDED) {
	info(0, "Gateway is now SUSPENDED by startup arguments");
    } else if (bb_status == BB_ISOLATED) {
	info(0, "Gateway is now ISOLATED by startup arguments");
	list_remove_producer(suspended);
    } else {
	smsc2_resume();
	list_remove_producer(suspended);	
	list_remove_producer(isolated);
    }

    while(bb_status != BB_SHUTDOWN && bb_status != BB_DEAD && list_producer_count(flow_threads) > 0) {
        /* debug("bb", 0, "Main Thread: going to sleep."); */
        /*
         * Not infinite sleep here, because we should notice
         * when all "flow threads" are dead and shutting bearerbox
         * down.
         * XXX if all "flow threads" call gwthread_wakeup(MAIN_THREAD_ID),
         * we can enter infinite sleep then.
         */
        gwthread_sleep(10.0);
        /* debug("bb", 0, "Main Thread: woken up."); */

        if (bb_todo == 0) {
            continue;
        }

        if (bb_todo & BB_LOGREOPEN) {
            warning(0, "SIGHUP received, catching and re-opening logs");
            log_reopen();
            alog_reopen();
            bb_todo = bb_todo & ~BB_LOGREOPEN;
        }

        if (bb_todo & BB_CHECKLEAKS) {
            warning(0, "SIGQUIT received, reporting memory usage.");
	    gw_check_leaks();
            bb_todo = bb_todo & ~BB_CHECKLEAKS;
        }
    }

    if (bb_status == BB_SHUTDOWN || bb_status == BB_DEAD)
        warning(0, "Killing signal or HTTP admin command received, shutting down...");

    /* call shutdown */
    bb_shutdown();

    /* wait until flow threads exit */
    while(list_consume(flow_threads)!=NULL)
	;

    info(0, "All flow threads have died, killing core");
    bb_status = BB_DEAD;
    httpadmin_stop();

    boxc_cleanup();
    smsc2_cleanup();
    empty_msg_lists();
    list_destroy(flow_threads, NULL);
    list_destroy(suspended, NULL);
    list_destroy(isolated, NULL);
    mutex_destroy(status_mutex);

    alog_close();		/* if we have any */
    bb_alog_shutdown();
    cfg_destroy(cfg);
    dlr_shutdown();
    gwlib_shutdown();

    if (restart == 1)
        execvp(argv[0],argv);

    return 0;
}



/*----------------------------------------------------------------
 * public functions used via HTTP adminstration interface/module
 */

int bb_shutdown(void)
{
    static int called = 0;
    
    mutex_lock(status_mutex);
    
    if (called) {
	mutex_unlock(status_mutex);
	return -1;
    }
    debug("bb", 0, "Shutting down " GW_NAME "...");

    called = 1;
    set_shutdown_status();
    mutex_unlock(status_mutex);

#ifndef NO_SMS
    debug("bb", 0, "shutting down smsc");
    smsc2_shutdown();
#endif
#ifndef NO_WAP
    debug("bb", 0, "shutting down udp");
    udp_shutdown();
#endif
    store_shutdown();
    
    return 0;
}

int bb_isolate(void)
{
    mutex_lock(status_mutex);
    if (bb_status != BB_RUNNING && bb_status != BB_SUSPENDED) {
	mutex_unlock(status_mutex);
	return -1;
    }
    if (bb_status == BB_RUNNING) {
	smsc2_suspend();
	list_add_producer(isolated);
    } else
	list_remove_producer(suspended);

    bb_status = BB_ISOLATED;
    mutex_unlock(status_mutex);
    return 0;
}

int bb_suspend(void)
{
    mutex_lock(status_mutex);
    if (bb_status != BB_RUNNING && bb_status != BB_ISOLATED) {
	mutex_unlock(status_mutex);
	return -1;
    }
    if (bb_status != BB_ISOLATED) {
	smsc2_suspend();
	list_add_producer(isolated);
    }
    bb_status = BB_SUSPENDED;
    list_add_producer(suspended);
    mutex_unlock(status_mutex);
    return 0;
}

int bb_resume(void)
{
    mutex_lock(status_mutex);
    if (bb_status != BB_SUSPENDED && bb_status != BB_ISOLATED) {
	mutex_unlock(status_mutex);
	return -1;
    }
    if (bb_status == BB_SUSPENDED)
	list_remove_producer(suspended);

    smsc2_resume();
    bb_status = BB_RUNNING;
    list_remove_producer(isolated);
    mutex_unlock(status_mutex);
    return 0;
}

int bb_flush_dlr(void)
{
    /* beware that mutex locking is done in dlr_foobar() routines */
    if (bb_status != BB_SUSPENDED) {
	return -1;
    }
    dlr_flush();
    return 0;
}

int bb_stop_smsc(Octstr *id)
{
    return smsc2_stop_smsc(id);
}

int bb_restart_smsc(Octstr *id)
{
    return smsc2_restart_smsc(id);
}

int bb_restart(void)
{
    restart = 1;
    return bb_shutdown();
}



#define append_status(r, s, f, x) { s = f(x); octstr_append(r, s); \
                                 octstr_destroy(s); }


Octstr *bb_print_status(int status_type)
{
    char *s, *lb;
    char *frmt, *footer;
    char buf[1024];
    Octstr *ret, *str, *version;
    time_t t;

    if ((lb = bb_status_linebreak(status_type))==NULL)
	return octstr_create("Un-supported format");

    t = time(NULL) - start_time;
    
    if (bb_status == BB_RUNNING)
	s = "running";
    else if (bb_status == BB_ISOLATED)
	s = "isolated";
    else if (bb_status == BB_SUSPENDED)
	s = "suspended";
    else if (bb_status == BB_FULL)
        s = "filled";
    else
	s = "going down";

    version = version_report_string("bearerbox");

    if (status_type == BBSTATUS_HTML) {
	frmt = "%s</p>\n\n"
	    " <p>Status: %s, uptime %ldd %ldh %ldm %lds</p>\n\n"
	    " <p>WDP: received %ld (%ld queued), sent %ld "
	    "(%ld queued)</p>\n\n"
	    " <p>SMS: received %ld (%ld queued), sent %ld "
	    "(%ld queued), store size %ld</p>\n"
        " <p>SMS: inbound %.2f msg/sec, outbound %.2f msg/sec</p>\n\n"
        " <p>DLR: %ld queued, using %s storage</p>\n\n";
	footer = "<p>";
    } else if (status_type == BBSTATUS_WML) {
	frmt = "%s</p>\n\n"
	    "   <p>Status: %s, uptime %ldd %ldh %ldm %lds</p>\n\n"
	    "   <p>WDP: received %ld (%ld queued)<br/>\n"
	    "      WDP: sent %ld (%ld queued)</p>\n\n"
	    "   <p>SMS: received %ld (%ld queued)<br/>\n"
	    "      SMS: sent %ld (%ld queued)<br/>\n"
        "      SMS: store size %ld<br/>\n"
        "      SMS: inbound %.2f msg/sec<br/>\n"
        "      SMS: outbound %.2f msg/sec</p>\n\n"
        "   <p>DLR: %ld queued<br/>\n"
        "      DLR: using %s storage</p>\n\n";
	footer = "<p>";
    } else if (status_type == BBSTATUS_XML) {
	frmt = "<version>%s</version>\n"
	    "<status>%s, uptime %ldd %ldh %ldm %lds</status>\n"
	    "\t<wdp>\n\t\t<received><total>%ld</total><queued>%ld</queued></received>\n\t\t<sent><total>%ld"
	    "</total><queued>%ld</queued></sent>\n\t</wdp>\n"
	    "\t<sms>\n\t\t<received><total>%ld</total><queued>%ld</queued></received>\n\t\t<sent><total>%ld"
	    "</total><queued>%ld</queued></sent>\n\t\t<storesize>%ld</storesize>\n\t\t"
        "<inbound>%.2f</inbound>\n\t\t<outbound>%.2f</outbound>\n\t</sms>\n"
	    "\t<dlr>\n\t\t<queued>%ld</queued>\n\t\t<storage>%s</storage>\n\t</dlr>\n";
	footer = "";
    } else {
	frmt = "%s\n\nStatus: %s, uptime %ldd %ldh %ldm %lds\n\n"
	    "WDP: received %ld (%ld queued), sent %ld (%ld queued)\n\n"
	    "SMS: received %ld (%ld queued), sent %ld (%ld queued), store size %ld\n"
        "SMS: inbound %.2f msg/sec, outbound %.2f msg/sec\n\n"
        "DLR: %ld queued, using %s storage\n\n";
	footer = "";
    }
    
    sprintf(buf, frmt,
	    octstr_get_cstr(version),
	    s, t/3600/24, t/3600%24, t/60%60, t%60,
	    counter_value(incoming_wdp_counter),
	    list_len(incoming_wdp) + boxc_incoming_wdp_queue(),
	    counter_value(outgoing_wdp_counter),
	    list_len(outgoing_wdp) + udp_outgoing_queue(),
	    counter_value(incoming_sms_counter),
	    list_len(incoming_sms),
	    counter_value(outgoing_sms_counter),
	    list_len(outgoing_sms),
	    store_messages(),
        (float)counter_value(incoming_sms_counter)/t,
        (float)counter_value(outgoing_sms_counter)/t,
        dlr_messages(),
        dlr_type());

    octstr_destroy(version);
    ret = octstr_create(buf);
    
    append_status(ret, str, boxc_status, status_type);
    append_status(ret, str, smsc2_status, status_type);
    octstr_append_cstr(ret, footer);
    
    return ret;
}


char *bb_status_linebreak(int status_type)
{
    switch(status_type) {
    case BBSTATUS_HTML:
	return "<br>\n";
    case BBSTATUS_WML:
	return "<br/>\n";
    case BBSTATUS_TEXT:
	return "\n";
    case BBSTATUS_XML:
	return "\n";
    default:
	return NULL;
    }
}
