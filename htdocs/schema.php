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
  // #TODO: Werte (und seofern möglich die Koordinaten) in Schleife über eine config-tabelle holen

  // -------------------------
  // get values

  $result = mysql_query("select value from samples where time = '" . $max . "' and address = 0x0 and type = 'VA';");
  $row = mysql_fetch_array($result, MYSQL_ASSOC);
  $kessel = $row['value'];

  $result = mysql_query("select value from samples where time = '" . $max . "' and address = 0x76 and type = 'VA';");
  $row = mysql_fetch_array($result, MYSQL_ASSOC);
  $pufferOben = $row['value'];

  // -------------------------
  // show values

  echo "<div id=\"kessel\">$kessel °C</div>\n";
  echo "<div id=\"pufferOben\">$pufferOben °C</div>\n";

  // -------------------------
  // 

  $value = $maxPretty;
  $top = 40;
  $left = 20;

  echo "<div style=\"top:" . $top . "px; left:" . $left . "px;\">" . $value . "</div>\n";


?>

  </body>
</html>
