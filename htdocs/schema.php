<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN">
<html>
  <head>
    <title>main</title>
    <meta http-equiv="content-type" content="text/html; charset=UTF-8">
    <meta name="author" content="Jörg Wendel">
    <meta name="copyright" content="Jörg Wendel">
    <img src="schema.jpg">
    <style type="text/css">
      body { margin:0; }
      div { position:absolute; }
       #time        { top:20px;  left:40px; }
       #kessel      { top:440px; left:110px; }
       #pufferOben  { top:215px; left:1100px; }
       #pufferUnten { top:379px; left:640px; }
    </style>
  </head>

  <body>
<?php

include("config.php");
include("functions.php");

  // -------------------------
  // establish db connection

  mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
  mysql_select_db($mysqldb);
  mysql_query("set names 'utf8'");
  mysql_query("SET lc_time_names = 'de_DE'");

  // -------------------------
  // get last time stamp

  $result = mysql_query("select max(time), DATE_FORMAT(max(time),'%d. %M %Y   %H:%i') as maxPretty from samples;");
  $row = mysql_fetch_array($result, MYSQL_ASSOC);
  $max = $row['max(time)'];
  $maxPretty = $row['maxPretty'];

  // -------------------------
  // show values

  echo "<div id=\"time\">$maxPretty</div>\n";

  // -------------------------
  // 

  $resultConf = mysql_query("select address, type, kind, xpos, ypos from schemaconf");

  while ($rowConf = mysql_fetch_array($resultConf, MYSQL_ASSOC))
  {
     $addr = $rowConf['address'];
     $type = $rowConf['type'];
     $left = $rowConf['xpos'];
     $top = $rowConf['ypos'];

     $strQuery = sprintf("select value from samples s, valuefacts f where f.address = s.address and f.type = s.type and s.time = '%s' and f.address = %s and f.type = '%s';", $max, $addr, $type);

     $result = mysql_query($strQuery);

     if ($row = mysql_fetch_array($result, MYSQL_ASSOC))
     {
        $value = $row['value'];
        syslog(LOG_DEBUG, "p4: add value to schema at pos " . $left . "/" . $top);
        echo "<div style=\"top:" . $top . "px; left:" . $left . "px;\">" . $value . "</div>\n";
     }
  }

?>

  </body>
</html>
