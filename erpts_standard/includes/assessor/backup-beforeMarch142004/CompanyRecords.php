<?php
class CompanyRecords
{
	var $arrayList;
	var $domDocument;
	var $db;
	
	function CompanyRecords(){
	
	}
	
	function setDB(){
		$this->db = new DB_RPTS;
	}
	
	function setArrayList($tempVar){
		$this->arrayList[] = $tempVar;
	}
	
	function getArrayList(){
		return $this->arrayList;
	}
	
	function getDomDocument(){
		return $this->domDocument;
	}
	
	function appendToDomList($rootNode,$childNode){
		//$rootNode->append_child($childNode->document_element());

		// test clone_node()
		$nodeTmp = $childNode->document_element();
		$nodeClone = $nodeTmp->clone_node(true);
		$rootNode->append_child($nodeClone);
	}
		
	function setDomDocument(){
		$this->domDocument = domxml_new_doc("1.0");
		$domList = $this->domDocument->create_element("ArrayList");
		$domList = $this->domDocument->append_child($domList);
		foreach($this->arrayList as $key => $value){
			$domDoc = $value->getDomDocument();
			$this->appendToDomList($domList,$domDoc);
		}
		return true;
	}
	
	
	function parseDomDocument($domDoc){
		$baseNode = $domDoc->document_element();
		if ($baseNode->has_child_nodes()){
			$child = $baseNode->first_child();
			while ($child){
				//if ($child->tagname=="Company") {
				if ($child->tagname) {
					$tempXmlStr = $domDoc->dump_node($child);
					$tempDomDoc = domxml_open_mem($tempXmlStr);
					$company = new Company;
					$company->parseDomDocument($tempDomDoc);
					$this->setArrayList($company);
				}
				$child = $child->next_sibling();
			}
		}
		$this->setDomDocument();
		return true;
	}
	
	function selectRecords($condition=""){
		$sql = sprintf("select * from %s %s;",
				COMPANY_TABLE, $condition);
		$this->setDB();
		$this->db->query($sql);
		while ($this->db->next_record()) {
			$company = new Company;
			$company->selectRecord($this->db->f("companyID"));
			$this->arrayList[] = $company;
		}
		if(count($this->arrayList) > 0){
			$this->setDomDocument();
			return true;
		}
		else {
			return false;
		}
	}

	function searchRecords($searchKey,$fields,$limit){
		$condition = "where (";
		foreach($fields as $key => $value){
			if($key == 0) $condition = $condition.$value." like '%".$searchKey."%'";
			else $condition = $condition."or ".$value." like '%".$searchKey."%' ";
		}
		
		$sql = sprintf("select * from %s %s;",
				COMPANY_TABLE, $condition.") ".$limit);
		$this->setDB();
		$this->db->query($sql);
		while ($this->db->next_record()) {
			$company = new Company;
			$company->selectRecord($this->db->f("companyID"));
			$this->arrayList[] = $company;
		}
		if(count($this->arrayList) > 0){
			$this->setDomDocument();
			return true;
		}
		else {
			return false;
		}
	}
	
	function countRecords($condition=""){
		$sql = sprintf("select count(*) as count from %s %s;",
				COMPANY_TABLE, $condition);
		$this->setDB();
		$this->db->query($sql);
		if($this->db->next_record()) $ret = $this->db->f("count");
		else $ret = false;
		return $ret;
	}
	
	function countSearchRecords($searchKey,$fields){
		$condition = "where (";
		foreach($fields as $key => $value){
			if($key == 0) $condition = $condition.$value." like '%".$searchKey."%'";
			else $condition = $condition."or ".$value." like '%".$searchKey."%' ";
		}
		
		$sql = sprintf("select count(*) as count from %s %s;",
				COMPANY_TABLE, $condition.") ".$limit);
		$this->setDB();
		$this->db->query($sql);
		if($this->db->next_record()) $ret = $this->db->f("count");
		else $ret = false;
		return $ret;
	}
		
	function deleteRecords($companyIDArray=""){
		$company = new Company;
		$rows = 0;
		foreach ($companyIDArray as $key => $value){
			if ($company->deleteRecord($value)) $rows++;
		}
		return $rows;
	}
}
?>