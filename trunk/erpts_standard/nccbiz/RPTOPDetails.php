<?php
include_once("web/prepend.php");
include_once("assessor/Assessor.php");
include_once("assessor/AssessorRecords.php");
include_once("assessor/TD.php");
include_once("assessor/TDRecords.php");
include_once("assessor/RPTOP.php");

//*
$server = new SoapServer("urn:Object");
$server->setClass('RPTOPDetails');
$server->handle();
//*/

class RPTOPDetails
{
	var $rptop;
	
    function RPTOPDetails(){
	}
		
	function getRPTOP($rptopID){
		$rptop = new RPTOP;
		$rptop->selectRecord($rptopID);
		if(!$domDoc = $rptop->getDomDocument()){
			return false;
		}
		else {
			$xmlStr = $domDoc->dump_mem(true);
			return $xmlStr;
		}
	}

}

/*
$obj = new RPTOPDetails;
echo $obj->getRPTOP(8);
//*/
?>
