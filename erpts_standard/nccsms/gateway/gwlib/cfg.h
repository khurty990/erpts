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
 * cfg.h - configuration file handling
 *
 * All returned octet strings are copies which the caller should destroy.
 *
 * Lars Wirzenius
 */


#ifndef CFG_H
#define CFG_H

typedef struct Cfg Cfg;
typedef struct CfgLoc CfgLoc; 
typedef struct CfgGroup CfgGroup;

Cfg *cfg_create(Octstr *filename);
void cfg_destroy(Cfg *cfg);
int cfg_read(Cfg *cfg);

CfgGroup *cfg_get_single_group(Cfg *cfg, Octstr *name);
List *cfg_get_multi_group(Cfg *cfg, Octstr *name);
Octstr *cfg_get_group_name(CfgGroup *grp);
Octstr *cfg_get_configfile(CfgGroup *grp);

Octstr *cfg_get_real(CfgGroup *grp, Octstr *varname, const char *file,
    	    	     long line, const char *func);
#define cfg_get(grp, varname) \
    cfg_get_real(grp, varname, __FILE__, __LINE__, __func__)
int cfg_get_integer(long *n, CfgGroup *grp, Octstr *varname);

/* Return -1 and set n to 0 if no varname found
 * otherwise return 0 and set n to 0 if value is no, false, off or 0,
 * and 1 if it is true, yes, on or 1. Set n to 1 for other values, too
 */
int cfg_get_bool(int *n, CfgGroup *grp, Octstr *varname);
List *cfg_get_list(CfgGroup *grp, Octstr *varname);
void cfg_set(CfgGroup *grp, Octstr *varname, Octstr *value);

void grp_dump(CfgGroup *grp);
void cfg_dump(Cfg *cfg);

/*
 * Dump all known config groups and values to stdout.
 */
void cfg_dump_all(void);

#endif
