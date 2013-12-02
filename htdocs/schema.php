<?php

  // -------------------------
  // get last time stamp

  $result = mysql_query("select max(time), DATE_FORMAT(max(time),'%d. %M %Y   %H:%i') as maxPretty from samples;");
  $row = mysql_fetch_array($result, MYSQL_ASSOC);
  $max = $row['max(time)'];
  $maxPretty = $row['maxPretty'];

  // -------------------------
  // show values

echo "      <div style=\"position:absolute; font-size:30px; left:" . ($jpegLeft +40) . "px; top:" .($jpegTop + 5) . "px; z-index:2;\">$maxPretty</div>\n";
  
  // -------------------------
  // show values

  $resultConf = mysql_query("select address, type, kind, color, xpos, ypos from schemaconf where state = 'A'");

  while ($rowConf = mysql_fetch_array($resultConf, MYSQL_ASSOC))
  {
     $addr = $rowConf['address'];
     $type = $rowConf['type'];
     $left = $rowConf['xpos'];
     $top = $rowConf['ypos'];
     $color = $rowConf['color'];

     $strQuery = sprintf("select s.value as s_value, f.unit as f_unit from samples s, valuefacts f where f.address = s.address and f.type = s.type and s.time = '%s' and f.address = %s and f.type = '%s';", $max, $addr, $type);
     $result = mysql_query($strQuery);

     if ($row = mysql_fetch_array($result, MYSQL_ASSOC))
     {
        $value = $row['s_value'];
        $unit = $row['f_unit'];

        echo "      <div style=\"position:absolute; top:" . ($top + $jpegTop) . "px; left:" . ($left + $jpegLeft) . "px" .
           "; color:" . $color . "; font-size:28px; z-index:2" . "\">" . $value . " " . $unit . "</div>\n";
     }
  }

?>
