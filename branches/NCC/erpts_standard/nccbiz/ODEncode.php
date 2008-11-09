<?php
include_once("web/prepend.php");
include_once("assessor/Address.php");
include_once("assessor/Company.php");
include_once("assessor/CompanyRecords.php");
include_once("assessor/Person.php");
include_once("assessor/PersonRecords.php");
include_once("assessor/Owner.php");
include_once("assessor/OwnerRecords.php");
include_once("assessor/OD.php");
//*
$server = new SoapServer("urn:Object");
$server->setClass('ODEncode');
$server->handle();
//*/

class ODEncode
{
	var $od;
	
    function ODEncode()
    {
		$this->od = new OD;
    }
	
	function getOwnerID($odID){
		$od = new OD;
		$od->selectRecord($odID);
		if (!$ownerID = $od->owner->getOwnerID()) return false;
		else return $ownerID;
	}
    
    function getData()
    {
        return $this->data;
    }
}

/*
$ODEncode = new ODEncode;
echo $ODEncode->getOwnerID(1);
//*/
?>