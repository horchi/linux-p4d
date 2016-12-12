<?php

session_start();

include("config.php");
include("functions.php");

// ----------------
// Configuration

 if (!isset($_SESSION['initialized']) || !$_SESSION['initialized'])
 {
  $_SESSION['initialized'] = true;

   // -------------------------
   // establish db connection

   $mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb);
   $mysqli->query("set names 'utf8'");
   $mysqli->query("SET lc_time_names = 'de_DE'");

   // ------------------
   // get configuration

   readConfigItem($mysqli, "chartStart", $_SESSION['chartStart']);
   readConfigItem($mysqli, "chartDiv", $_SESSION['chartDiv']);
   readConfigItem($mysqli, "chartXLines", $_SESSION['chartXLines']);
   readConfigItem($mysqli, "chart1", $_SESSION['chart1']);
   readConfigItem($mysqli, "chart2", $_SESSION['chart2']);

   // Chart 3+4

   readConfigItem($mysqli, "chart34", $_SESSION['chart34']);
   readConfigItem($mysqli, "chart3", $_SESSION['chart3']);
   readConfigItem($mysqli, "chart4", $_SESSION['chart4']);

   readConfigItem($mysqli, "user", $_SESSION['user']);
   readConfigItem($mysqli, "passwd", $_SESSION['passwd']);

   readConfigItem($mysqli, "mail", $_SESSION['mail']);
   readConfigItem($mysqli, "htmlMail", $_SESSION['htmlMail']);
   readConfigItem($mysqli, "stateMailTo", $_SESSION['stateMailTo']);
   readConfigItem($mysqli, "stateMailStates", $_SESSION['stateMailStates']);
   readConfigItem($mysqli, "errorMailTo", $_SESSION['errorMailTo']);
   readConfigItem($mysqli, "mailScript", $_SESSION['mailScript']);

   readConfigItem($mysqli, "tsync", $_SESSION['tsync']);
   readConfigItem($mysqli, "maxTimeLeak", $_SESSION['maxTimeLeak']);
   readConfigItem($mysqli, "heatingType", $_SESSION['heatingType']);
   readConfigItem($mysqli, "stateAni", $_SESSION['stateAni']);
   readConfigItem($mysqli, "schemaRange", $_SESSION['schemaRange']);
   readConfigItem($mysqli, "schema", $_SESSION['schema']);
   readConfigItem($mysqli, "schemaBez", $_SESSION['schemaBez']);
   readConfigItem($mysqli, "valuesBG", $_SESSION['valuesBG']);
   readConfigItem($mysqli, "pumpON",  $_SESSION['pumpON']);
   readConfigItem($mysqli, "pumpOFF", $_SESSION['pumpOFF']);
   readConfigItem($mysqli, "ventON",  $_SESSION['ventON']);
   readConfigItem($mysqli, "ventOFF", $_SESSION['ventOFF']);
   readConfigItem($mysqli, "pumpsVA", $_SESSION['pumpsVA']);
   readConfigItem($mysqli, "pumpsDO", $_SESSION['pumpsDO']);
   readConfigItem($mysqli, "pumpsAO", $_SESSION['pumpsAO']);
   readConfigItem($mysqli, "webUrl",  $_SESSION['webUrl']);

   // ------------------
   // check for defaults

   if ($_SESSION['chartStart'] == "")
      $_SESSION['chartStart'] = "5";

   if ($_SESSION['chartDiv'] == "")
      $_SESSION['chartDiv'] = "25";

   if ($_SESSION['chartXLines'] == "")
      $_SESSION['chartXLines'] = "0";
   else
      $_SESSION['chartXLines'] = "1";

   if ($_SESSION['chart1'] == "")
      $_SESSION['chart1'] = "0,1,113,18";

   if ($_SESSION['chart2'] == "")
      $_SESSION['chart2'] = "118,225,21,25,4,8";

   if ($_SESSION['chart34'] == "")
      $_SESSION['chart34'] = "0";

   if ($_SESSION['heatingType'] == "")
      $_SESSION['heatingType'] = "p4";

   if ($_SESSION['schema'] == "")
      $_SESSION['schema'] = "p4-2hk-puffer";

   if ($_SESSION['pumpON'] == "")
      $_SESSION['pumpON'] = "img/icon/pump-on.gif";

   if ($_SESSION['pumpOFF'] == "")
      $_SESSION['pumpOFF'] = "img/icon/pump-off.gif";

   if ($_SESSION['ventON'] == "")
      $_SESSION['ventON'] = "img/icon/vent-on.gif";

   if ($_SESSION['ventOFF'] == "")
      $_SESSION['ventOFF'] = "aus";

   if ($_SESSION['pumpsVA'] == "")
      $_SESSION['pumpsVA'] = "(15),140,141,142,143,144,145,146,147,148,149,150,151,152,200,201,240,241,242";

   if ($_SESSION['pumpsDO'] == "")
      $_SESSION['pumpsDO'] = "0,1,25,26,31,32,37,38,43,44,49,50,55,56,61,62,67,68";

   if ($_SESSION['pumpsAO'] == "")
      $_SESSION['pumpsAO'] = "3,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22";

   $mysqli->close();
}

function printHeader($refresh = 0)
{
   // ----------------
   // HTML Head

   //<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">

   echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">
<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"de\" lang=\"de\">
  <head>
    <meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n";
   $stylesheet = (checkMobile() == 1)? "stylesheet.css" : "stylesheet.css";

   if ($refresh)
      echo "    <meta http-equiv=\"refresh\" content=\"$refresh\">\n";

   echo "    <meta name=\"author\" content=\"Jörg Wendel\">
    <meta name=\"copyright\" content=\"Jörg Wendel\">
    <LINK REL=\"SHORTCUT ICON\" HREF=\"" . $_SESSION['heatingType'] . ".ico\">
    <link rel=\"stylesheet\" type=\"text/css\" href=\"$stylesheet\">
    <script type=\"text/JavaScript\" src=\"jfunctions.js\"></script>
    <title>Fröling  " . $_SESSION['heatingType'] . "</title>
  </head>
  <body>
    <a class=\"button1\" href=\"main.php\">Aktuell</a>
    <a class=\"button1\" href=\"chart.php\">Charts 1+2</a>";

   if ($_SESSION['chart34'] == "1")
      echo "<a class=\"button1\" href=\"chart2.php\">Charts 3+4</a>";

   echo "<a class=\"button1\" href=\"schemadsp.php\">Schema</a>
    <a class=\"button1\" href=\"menu.php\">Menü</a>
    <a class=\"button1\" href=\"error.php\">Fehler</a>\n";

   if (haveLogin())
   {
      echo "    <a class=\"button1\" href=\"basecfg.php\">Setup</a>\n";
      echo "    <a class=\"button1\" href=\"logout.php\">Logout</a>\n";
   }
   else
      echo "    <a class=\"button1\" href=\"login.php\">Login</a>\n";

//   echo $stylesheet;
   echo "    <div class=\"content\">\n";
}

?>
