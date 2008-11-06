<?php
# Setup PHPLIB in this Area
include_once("web/prepend.php");
include_once("assessor/Machineries.php");
include_once("assessor/MachineriesRecords.php");

//*
$server = new SoapServer("urn:Object");
$server->setClass('MachineriesList');
$server->handle();
//*/
class MachineriesList
{
    function MachineriesList(){
		
    }
    
    function getMachineriesList($afsID) {
		$machineriesRecords = new MachineriesRecords;
		$machineriesRecords->selectRecords($afsID);
		if(!$domDoc = $machineriesRecords->getDomDocument()){
			return false;
		}
		else {
			$xmlStr = $domDoc->dump_mem(true);
			return $xmlStr;
		}
	}
	
	function deleteMachineries($machineriesIDArray){
		$machineriesRecords = new MachineriesRecords;
		$rows = $machineriesRecords->deleteRecords($machineriesIDArray);
		return $rows;
	}
	
	function removeMachineries($machineriesIDArray){
		$machineriesRecords = new MachineriesRecords;
		$rows = $machineriesRecords->removeRecords($machineriesIDArray);
		return $rows;
	}
}
/*
$obj = new MachineriesList;
//$id[] = 1295;
//echo "xml".$obj->deleteMachineries($id);
echo $obj->getMachineriesList(1);
//*/
?>