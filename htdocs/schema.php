<?php

  // -------------------------
  // get last time stamp

$result = mysql_query("select max(time), DATE_FORMAT(max(time),'%d. %M %Y   %H:%i') as maxPretty from samples;")
   or die("Error" . mysql_error());

$row = mysql_fetch_array($result, MYSQL_ASSOC);
$max = $row['max(time)'];
$maxPretty = $row['maxPretty'];

// -------------------------
// show values

echo "      <div class=\"schema\">\n";

echo "        <div style=\"position:absolute; font-size:28px; left:" . ($jpegLeft +40) . "px; top:" .($jpegTop + 5) . "px; z-index:2;\">$maxPretty</div>\n";

// -------------------------
// show values

$resultConf = mysql_query("select address, type, kind, color, showunit, showtext, xpos, ypos from schemaconf where state = 'A'")
   or die("Error" . mysql_error());

while ($rowConf = mysql_fetch_array($resultConf, MYSQL_ASSOC))
{
   $addr = $rowConf['address'];
   $type = $rowConf['type'];
   $left = $rowConf['xpos'];
   $top = $rowConf['ypos'];
   $color = $rowConf['color'];
   $showUnit = $rowConf['showunit'];
   $showText = $rowConf['showtext'];
   
   $strQuery = sprintf("select s.value as s_value, s.text as s_text, f.title as f_title, f.unit as f_unit from samples s, valuefacts f where f.address = s.address and f.type = s.type and s.time = '%s' and f.address = %s and f.type = '%s';", $max, $addr, $type);
   $result = mysql_query($strQuery)
      or die("Error" . mysql_error());

   if ($row = mysql_fetch_array($result, MYSQL_ASSOC))
   {
      $value = $row['s_value'];
      $text = $row['s_text'];
      $unit = $row['f_unit']; 
      $title = $row['f_title'];
      $bez = ""; 
      setTxt();
      
      echo "        <div class=\"values\" title=\"" . $title . "\" style=\"position:absolute; top:" . ($top + $jpegTop) . "px; left:" . ($left + $jpegLeft) . "px" .
         "; color:" . $color . "; z-index:11" . "\">";
      
      $value = (preg_match("/[a-zA-Z ]/", $value)) ? $value : number_format(round($value, 1),1);
      if ($showText)
         echo $text;
      else if ($showUnit)
         echo $bez . $value . ($unit == "°" ? "°C" : $unit);
      else
         echo $bez . $value;
      
      echo "</div>\n";
   }
}

echo "      </div>\n";

function setTxt ()
{  
	global $addr,$type,$unit,$value,$bez,$title;
   //echo "<div style=\"z-index:12\">";
   //echo "</div>";
   
  if ($type == "VA")
  {
      ($addr == "266") ? $bez = "Holzmenge: " : $addr;
      ($addr == "269") ? $bez = "?: " : $addr;
      if ($addr == "23" || $addr == "27" || $addr == "31" || $addr == "35" || $addr == "39") {
      	switch ($value) {
      		case 2:	$value = "Nacht"; break;
      		case 1:	$value = "Party"; break;
      		case 0: $value = "Auto"; break;
      	}
      }
   
  }
  
  if ($type == "DI")
  {
      ($addr == "0") ? ($value == "1") ? $value = "Tür auf" : $value = "Tür zu" : $addr;
  
  }
  
  if ($type == "DO")
  {
      ($addr == "0" || $addr == "1")	? ($value == "1")		? $value = "an"		: $value = "aus"		: $addr;
      ($addr == "8") ? ($value == "1")	? $value = "Holzbetrieb"		: $value = "Gasbetrieb"		: $value;
      ($title == "Heizkreispumpe 0") ? $title = "Gas- / Holz-Umschaltung (HKP0)"		: $title;
  
  }
  
  if ($type == "AO")
  {
      ($addr == "266") ? $bez = "Holzmenge: "			: $bez;
  
  }
  
  if ($type == "W1")
  {
      ($addr == "266") ? $bez = "Holzmenge: "			: $bez;
  
  }
      
  ($unit == "U") ? $unit = "U/min"	: $unit;
  ($unit == "k") ? $unit = "kg"	: $unit;
   //echo "<".$addr.":".$type.":".$value.">";

}
?>
