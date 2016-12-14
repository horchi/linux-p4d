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

   readConfigItem("addrsMain", $_SESSION['addrsMain'], "");
   readConfigItem("addrsMainMobile", $_SESSION['addrsMainMobile'], "0,1,4,118,119,120");

   readConfigItem("chartStart", $_SESSION['chartStart'], "5");
   readConfigItem("chartDiv", $_SESSION['chartDiv'], "25");
   readConfigItem("chartXLines", $_SESSION['chartXLines'], "0");
   readConfigItem("chart1", $_SESSION['chart1'], "0,1,113,18");
   readConfigItem("chart2", $_SESSION['chart2'], "118,225,21,25,4,8");

   // Chart 3+4

   readConfigItem("chart34", $_SESSION['chart34'], "0");
   readConfigItem("chart3", $_SESSION['chart3']);
   readConfigItem("chart4", $_SESSION['chart4']);

   readConfigItem("user", $_SESSION['user']);
   readConfigItem("passwd", $_SESSION['passwd']);

   readConfigItem("mail", $_SESSION['mail']);
   readConfigItem("htmlMail", $_SESSION['htmlMail']);
   readConfigItem("stateMailTo", $_SESSION['stateMailTo']);
   readConfigItem("stateMailStates", $_SESSION['stateMailStates']);
   readConfigItem("errorMailTo", $_SESSION['errorMailTo']);
   readConfigItem("mailScript", $_SESSION['mailScript']);

   readConfigItem("tsync", $_SESSION['tsync']);
   readConfigItem("maxTimeLeak", $_SESSION['maxTimeLeak']);
   readConfigItem("heatingType", $_SESSION['heatingType'], "p4");
   readConfigItem("stateAni", $_SESSION['stateAni']);
   readConfigItem("schemaRange", $_SESSION['schemaRange']);
   readConfigItem("schema", $_SESSION['schema'], "p4-2hk-puffer");
   readConfigItem("schemaBez", $_SESSION['schemaBez']);
   readConfigItem("valuesBG", $_SESSION['valuesBG']);
   readConfigItem("pumpON",  $_SESSION['pumpON'], "img/icon/pump-on.gif");
   readConfigItem("pumpOFF", $_SESSION['pumpOFF'], "img/icon/pump-off.gif");
   readConfigItem("ventON",  $_SESSION['ventON'], "img/icon/vent-on.gif");
   readConfigItem("ventOFF", $_SESSION['ventOFF'], "aus");
   readConfigItem("pumpsVA", $_SESSION['pumpsVA'], "(15),140,141,142,143,144,145,146,147,148,149,150,151,152,200,201,240,241,242");
   readConfigItem("pumpsDO", $_SESSION['pumpsDO'], "0,1,25,26,31,32,37,38,43,44,49,50,55,56,61,62,67,68");
   readConfigItem("pumpsAO", $_SESSION['pumpsAO'], "3,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22");
   readConfigItem("webUrl",  $_SESSION['webUrl'], "http://127.0.0.1/p4");

   // ------------------
   // check for defaults

   if ($_SESSION['chartXLines'] == "")
      $_SESSION['chartXLines'] = "0";
   else
      $_SESSION['chartXLines'] = "1";

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
   $stylesheet = isMobile() ? "stylesheet.css" : "stylesheet.css";

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
