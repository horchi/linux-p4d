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
   
   $strQuery = sprintf("select s.value as s_value, s.text as s_text, f.unit as f_unit from samples s, valuefacts f where f.address = s.address and f.type = s.type and s.time = '%s' and f.address = %s and f.type = '%s';", $max, $addr, $type);
   $result = mysql_query($strQuery)
      or die("Error" . mysql_error());
   
   if ($row = mysql_fetch_array($result, MYSQL_ASSOC))
   {
      $value = $row['s_value'];
      $text = $row['s_text'];
      $unit = $row['f_unit'];
      
      if ($unit == "U")
         $unit = " u/min";
      
      echo "        <div style=\"position:absolute; top:" . ($top + $jpegTop) . "px; left:" . ($left + $jpegLeft) . "px" .
         "; color:" . $color . "; z-index:11" . "\">";
      
      if ($showText)
         echo $text;
      else if ($showUnit)
         echo round($value, 2) . ($unit == "°" ? "°C" : $unit);
      else
         echo round($value, 2);
      
      echo "</div>\n";
   }
}

echo "      </div>\n";

?>
