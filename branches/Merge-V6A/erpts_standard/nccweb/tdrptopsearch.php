<?

/*
	Class : RPUFactory
	Author: Juanito M. Calingacion, Jr.
	Date  : 2003
			Modified Dec 07, 2005 for Lubao implementation of eRPTS
			
	Description: Implements or rather extends DBFactory. Reads data from an eRPTS database
			     Using the Recordset Library
			     Used by custom.rpu.phc that provides the core classes to retrieve real property information
			     from the database

	Mod'ed: alxjvr, 2006.02.27
*/


require_once("tdrptops.phc");
require_once("web/prepend.php");
require_once("HTML/Template/ITX.php");

page_open(array("sess" => "rpts_Session"
	,"auth" => "rpts_Challenge_Auth"
	//"perm" => "rpts_Perm"
	));


global $auth;
global $sess;

$sessionvar= $sess->url("");
$user=$auth->auth;
$inModule = trim($_REQUEST['module']);

$tpl=new HTML_Template_ITX(".");
if($inModule == "assm") {
	$tpl->loadTemplatefile("tdrptopsearch.htm");
	$target = "RPTOPDetails.php";
}
else {
	$tpl->loadTemplatefile("tdduesearch.htm");
	$target = "CalculateRPTOPDetails.php";
}
$tpl->setVariable("module",$inModule);

$inTDNumber = trim($_REQUEST['tdNumber']);
$tpl->setVariable("tdNumber",$inTDNumber);

$limit=15;
//echo '<br>isset($offpage)='.isset($offpage).'<br>';
if(!isset($offpage)) {
	$offpage = 1;
}
else {
	$offpage = $_REQUEST['offpage'];
}
//echo '<br>$offpage='.$offpage.'<br>';
$offset=(($offpage-1) * $limit) + 1;
//$tpl->setVariable("offset",$offset);
//$tpl->setVariable("limit",$limit);


//$tpl->setVariable("Session",$sessionvar);
$tpl->setVariable("uname",$user["uname"]);
$tpl->setVariable("today", date("F j, Y"));
$tpl->setVariable("session",$sessionvar);
$tpl->setVariable("ownerName", $_POST['ownerName']);

if(strlen($inTDNumber) > 0) {
	$treassettings = new TreasSettings();
	$treassetting = $treassettings->fetchTreasSetting();
	$tsrecord = $treassetting->toArray();

	$rptops=new TDRPTOPS($inTDNumber, $offset, $limit);
	$numrecs=$rptops->getNumRecs();
	if($numrecs > 0) {
		
		$pages=ceil($numrecs / $limit);
		//echo '<br>$pages='.$pages.'<br>';
		$offseted=$offset + $limit - 1;
		$offseted=($offseted > $numrecs) ? $numrecs : $offseted;
		$href="tdrptopsearch.php".$sessionvar."&module=".$inModule."&tdNumber=".$inTDNumber;

	//	echo '<br>$href='.$href.'<br>';

		if($pages == 1) {
			$offseted = $numrecs;
			$navfirst = "First&nbsp;&nbsp;|";
			$navprev = "Previous&nbsp;&nbsp;|";
			$navnext = "Next&nbsp;&nbsp;|";
			$navlast = "Last";

		}
		else {
			if($offpage == 1) {
				$navfirst = "First&nbsp;&nbsp;|";
				$navprev  = "Prev&nbsp;&nbsp;|";
				$navnext  = "<a href=".$href."&offpage=".($offpage+1).">Next</a>&nbsp;&nbsp;|";
				$navlast  = "<a href=".$href."&offpage=".$pages.">Last</a>";
			}
			elseif($offpage == $pages) {
				$navfirst = "<a href=".$href."&offpage=1>First</a>&nbsp;&nbsp;|";
				$navprev  = "<a href=".$href."&offpage=".($offpage-1).">Prev</a>&nbsp;&nbsp;|";
				$navnext  = "Next&nbsp;&nbsp;|";
				$navlast  = "Last";
			}
			else {
				$navfirst = "<a href=".$href."&offpage=1>First</a>&nbsp;&nbsp;|";
				$navprev  = "<a href=".$href."&offpage=".($offpage-1).">Prev</a>&nbsp;&nbsp;|";
				$navnext  = "<a href=".$href."&offpage=".($offpage+1).">Next</a>&nbsp;&nbsp;|";
				$navlast  = "<a href=".$href."&offpage=".$pages.">Last</a>";
			}
		}

		//$tpl->setVariable("offseted",$offseted);
		//$tpl->setVariable("numrecs",$numrecs);
		$tpl->setVariable("shownofind","none");
		$tpl->setVariable("showresults","action");
		$tpl->setVariable("shown","Showing $offset to $offseted of $numrecs");
		$tpl->setVariable("totalpages",$pages);
		$tpl->setVariable("offpage",$offpage);
		//$tpl->setVariable("navpages","Page ".$offpage." of ".$pages);
		$tpl->setVariable("navfirst",$navfirst);
		$tpl->setVariable("navprev",$navprev);
		$tpl->setVariable("navnext",$navnext);
		$tpl->setVariable("navlast",$navlast);
		
		//$prevoffset=

		//echo '<br>['.$numrecs.']<br>';
		$tpl->setCurrentBlock("RPTOPLIST");
		while(!$rptops->EOL()){
			$rptop=$rptops->fetchRPTOP();
			$record=$rptop->toArray();
			foreach($record as $k=>$field){
				$tpl->setVariable($k,$field);
			}
			$taxDue =  $record["assdValue"] * $tsrecord["pctRPTax"];
			$taxDue += $record["assdValue"] * $tsrecord["pctSEF"];
			$taxDue += $record["assdValue"] * $tsrecord["pctIdle "];
			
//			print "<hr> ".$record["assdValue"]."<br>".$tsrecord["pctRPTax"]."<br>".$taxDue." <hr>";
			
			$tpl->setVariable("totalTaxDue",number_format($taxDue,2));
			$tpl->setVariable("Session",$sessionvar);
			$tpl->setVariable("rptopDetailsLinkLabel",
				"<a href='".$target.$sessionvar."&rptopID=".$record["rptopID"]."&formAction=view'>Edit >></a>");
			$rptops->fetchNext();	
			$tpl->parseCurrentBlock();
		}	
	}
	else {
		$tpl->setVariable("shownofind","action");
		$tpl->setVariable("showresults","none");
		/*
		echo "none!";
		$record=array(
			"rptopID"=>"", 
			"ownerName"=>"", 
			"totalMarketValue"=>"", 
			"totalAssessedValue"=>"",
			"Session"=>"",
			"rptopDetailsLinkLabel"=>"");
		$tpl->setCurrentBlock("RPTOPLIST");
		foreach($record as $k=>$field){
			$tpl->setVariable($k,$field);
		}
		$tpl->parseCurrentBlock();
		*/
	}
}
else{
	$tpl->setVariable("shownofind","none");
	$tpl->setVariable("showresults","none");
}




$tpl->show();

page_close();
?>
