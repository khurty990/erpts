<?php

class ImprovementsBuildingsDepreciation
{
	//attributes
	var $improvementsBuildingsDepreciationID;
	var $code;
	var $reportCode;
	var $description;
	var $rangeLowerBound;
	var $rangeUpperBound;
	var $value;
	var $status;
	var $domDocument;
	var $db;
	
	//constructor
	function ImprovementsBuildingsDepreciation() {
	
	}
	//methods
	//set
	function setDB(){
		$this->db = new DB_RPTS;
	}
	
	function setImprovementsBuildingsDepreciationID($tempVar) {
		$this->improvementsBuildingsDepreciationID = $tempVar;
	}
	function setCode($tempVar) {
		$this->code = $tempVar;
	}
	function setReportCode($tempVar){
		$this->reportCode = $tempVar;
	}
	function setDescription($tempVar) {
		$this->description = $tempVar;
	}
	function setRangeLowerBound($tempVar){
		$this->rangeLowerBound = $tempVar;
	}
	function setRangeUpperBound($tempVar){
		$this->rangeUpperBound = $tempVar;
	}
	function setValue($tempVar) {
		$this->value = $tempVar;
	}	
	function setStatus($tempVar) {
		$this->status = $tempVar;
	}
	
	//DOM
	function setDocNode($elementName,$elementValue,$domDoc,$indexNode){
		$nodeName = "";
		$nodeText = "";
		$nodeName = $domDoc->create_element($elementName);
		$nodeName = $indexNode->append_child($nodeName);
		$nodeText = $domDoc->create_text_node(htmlentities($elementValue));
		$nodeText = $nodeName->append_child($nodeText);
	}
	function setArrayDocNode($elementName,$arrayList,$indexNode){
		$list = $this->domDocument->create_element($elementName);
		$list = $indexNode->append_child($list);
		if (is_array($arrayList)){
			foreach ($arrayList as $key => $value){
                $domTmp = $value->getDomDocument();
				//$list->append_child($domTmp->document_element());

				// test clone_node()
				$nodeTmp = $domTmp->document_element();
				$nodeClone = $nodeTmp->clone_node(true);
				$list->append_child($nodeClone);
			}
		}
	}
	function setObjectDocNode($elementName,$elementObject,$domDoc,$indexNode){
		$nodeName = "";
		$nodeDomDoc = $elementObject->getDomDocument();
		$nodeObject = $nodeDomDoc->document_element();

		$nodeClone = $nodeObject->clone_node(true);
			
		$nodeName = $domDoc->create_element($elementName);
		$nodeName = $indexNode->append_child($nodeName);
		$nodeObject = $nodeName->append_child($nodeClone);
	}
	function setDomDocument() {
		$this->domDocument = domxml_new_doc("1.0");
		$rec = $this->domDocument->create_element("ImprovementsBuildingsDepreciation");
		$rec = $this->domDocument->append_child($rec);
		//$rec->set_attribute("improvementsBuildingsDepreciationID",$this->improvementsBuildingsDepreciationID);
		$this->setDocNode("improvementsBuildingsDepreciationID",$this->improvementsBuildingsDepreciationID,$this->domDocument,$rec);
		$this->setDocNode("code",$this->code,$this->domDocument,$rec);
		$this->setDocNode("reportCode",$this->reportCode,$this->domDocument,$rec);
		$this->setDocNode("description",$this->description,$this->domDocument,$rec);
		$this->setDocNode("rangeLowerBound",$this->rangeLowerBound,$this->domDocument,$rec);
		$this->setDocNode("rangeUpperBound",$this->rangeUpperBound,$this->domDocument,$rec);
		$this->setDocNode("value",$this->value,$this->domDocument,$rec);
		$this->setDocNode("status",$this->status,$this->domDocument,$rec);
	}
	function parseDomDocument($domDoc){
		$ret = true;
		$baseNode = $domDoc->document_element();
		if ($baseNode->has_child_nodes()){
			$child = $baseNode->first_child();
			while ($child){
				//eval("\$this->".$child->tagname." = \"".$child->get_content()."\";");
				//eval("\$this->set".ucfirst($child->tagname)."(\"".$child->get_content()."\");");

				// test varvars
				$varvar = $child->tagname;
				$this->$varvar = html_entity_decode($child->get_content());

				$child = $child->next_sibling();
			}
		}
		$this->setDomDocument();
		return $ret;
	}
	function getDomDocument() {
		return $this->domDocument;
	}
	
	//get
	function getImprovementsBuildingsDepreciationID() {
		return $this->improvementsBuildingsDepreciationID;
	}
	function getCode() {
		return $this->code;
	}
	function getReportCode() {
		return $this->reportCode;
	}
	function getDescription() {
		return $this->description;
	}
	function getRangeLowerBound(){
		return $this->rangeLowerBound;
	}
	function getRangeUpperBound(){
		return $this->rangeUpperBound;
	}
	function getValue() {
		return $this->value;
	}	
	function getStatus() {
		return $this->status;
	}

	//DB
	function selectRecord($improvementsBuildingsDepreciationID){
		if ($improvementsBuildingsDepreciationID=="") return;

		$this->setDB();
		$sql = sprintf("SELECT * FROM %s WHERE improvementsBuildingsDepreciationID=%s;",
			IMPROVEMENTSBUILDINGS_DEPRECIATION_TABLE, $improvementsBuildingsDepreciationID);
			$this->db->query($sql);
		$improvementsBuildingsDepreciation = new ImprovementsBuildingsDepreciation;
		if ($this->db->next_record()) {
			//*
			$this->improvementsBuildingsDepreciationID = $this->db->f("improvementsBuildingsDepreciationID");
			$this->code = $this->db->f("code");
			$this->reportCode = $this->db->f("reportCode");
			$this->description = $this->db->f("description");
			$this->rangeLowerBound = $this->db->f("rangeLowerBound");
			$this->rangeUpperBound = $this->db->f("rangeUpperBound");
			$this->value = $this->db->f("value");	
			$this->status = $this->db->f("status");
			//*/
			foreach ($this->db->Record as $key => $value){
				$this->$key = $value;
			}
			$this->setDomDocument();
			$ret = true;
		}
		else $ret = false;
		return $ret;
	}
	
	function insertRecord(){
		$sql = sprintf("insert into %s (".
			"code".
			", reportCode".
			", description".
			", rangeLowerBound".
			", rangeUpperBound".
			", value".
			", status".
			") ".
			"values ('%s', '%s', '%s', '%s', '%s', '%s', '%s');"
			, IMPROVEMENTSBUILDINGS_DEPRECIATION_TABLE
			, fixQuotes($this->code)
			, fixQuotes($this->reportCode)
			, fixQuotes($this->description)
			, fixQuotes($this->rangeLowerBound)
			, fixQuotes($this->rangeUpperBound)
		    , fixQuotes($this->value)	
			, fixQuotes($this->status)
		);
	
		$this->setDB();
		$this->db->beginTransaction();
		$this->db->query($sql);
		$improvementsBuildingsDepreciationID = $this->db->insert_id();
		if ($this->db->Errno!=0) {
			$this->db->rollbackTransaction();
			$this->db->resetErrors();
			$ret = false;
		}
		else {
			$this->db->endTransaction();
			$ret = $improvementsBuildingsDepreciationID;
		}
		
		//echo $sql;
		return $ret;
	}
	
	function deleteRecord($improvementsBuildingsDepreciationID){
		$this->setDB();
		$this->db->beginTransaction();
		$this->selectRecord($improvementsBuildingsDepreciationID);
		$sql = sprintf("delete from %s where improvementsBuildingsDepreciationID=%s;",
			IMPROVEMENTSBUILDINGS_DEPRECIATION_TABLE, $improvementsBuildingsDepreciationID);
		$this->db->query($sql);
		$improvementsBuildingsDepreciationRows = $this->db->affected_rows();
		
		if ($this->db->Errno != 0) {
			$errno = $this->db->Errno;
			$this->db->rollbackTransaction();
			$this->db->resetErrors();
			$ret = false;
		}
		else {
			$this->db->endTransaction();
			$ret = $improvementsBuildingsDepreciationRows;
		}
		return $ret;
	}
	
	function updateRecord(){
		
		$sql = sprintf("update %s set".
			" code = '%s'".
			", reportCode = '%s'".
			", description = '%s'".
			", rangeLowerBound = '%s'".
			", rangeUpperBound = '%s'".
			", value = '%s'".
			", status = '%s'".
			" where improvementsBuildingsDepreciationID = '%s';",
			IMPROVEMENTSBUILDINGS_DEPRECIATION_TABLE
			, fixQuotes($this->code)
			, fixQuotes($this->reportCode)
			, fixQuotes($this->description)
			, fixQuotes($this->rangeLowerBound)
			, fixQuotes($this->rangeUpperBound)
			, fixQuotes($this->value)
			, fixQuotes($this->status)
			, $this->improvementsBuildingsDepreciationID
		);
		//echo $sql;
		$this->setDB();
		$this->db->beginTransaction();
		$this->db->query($sql);
		if ($this->db->Errno!=0) {
			$this->db->rollbackTransaction();
			$this->db->resetErrors();
			$ret = false;
		}
		else {
			$this->db->endTransaction();
			$ret = $this->improvementsBuildingsDepreciationID;
		}
		return $ret;
	}
	
}
?>