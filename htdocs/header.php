<?php

session_start();

$mysqlport = 3306;

$p4WebVersion = "<VERSION>";

include("config.php");
include("functions.php");

   // -------------------------
   // establish db connection

   $mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb, $mysqlport);

   if (mysqli_connect_error())
   {
      die('Connect Error (' . mysqli_connect_errno() . ') '
            . mysqli_connect_error() . ". Can't connect to " . $mysqlhost . ' at ' . $mysqlport);
   }

   $mysqli->query("set names 'utf8'");
   $mysqli->query("SET lc_time_names = 'de_DE'");

  // ----------------
  // Configuration

  // if (!isset($_SESSION['initialized']) || !$_SESSION['initialized'])
  {
     $_SESSION['initialized'] = true;

     // ------------------
     // get configuration

     readConfigItem("addrsMain", $_SESSION['addrsMain'], "");
     readConfigItem("addrsMainMobile", $_SESSION['addrsMainMobile'], "0,1,4,118,119,120");

     readConfigItem("chartStart", $_SESSION['chartStart'], "7");
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

     readConfigItem("hmHost", $_SESSION['hmHost']);

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

     // $mysqli->close();
  }

function printHeader($refresh = 0)
{
   // ----------------
   // HTML Head

   echo "<!DOCTYPE html>\n";
   echo "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"de\" lang=\"de\">\n";
   echo "  <head>\n";
   echo "    <meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"/>\n";

   if ($refresh)
      echo "    <meta http-equiv=\"refresh\" content=\"$refresh\"/>\n";

   echo "    <meta name=\"author\" content=\"Jörg Wendel\"/>\n";
   echo "    <meta name=\"copyright\" content=\"Jörg Wendel\"/>\n";
   echo "    <meta name=\"viewport\" content=\"initial-scale=1.0, width=device-width, user-scalable=no, maximum-scale=1, minimum-scale=1\"/>\n";
   echo "    <link rel=\"shortcut icon\" href=\"" . $_SESSION['heatingType'] . ".ico\" type=\"image/x-icon\"/>\n";
   echo "    <link rel=\"stylesheet\" type=\"text/css\" href=\"stylesheet.css\"/>\n";
   echo "    <script type=\"text/JavaScript\" src=\"jfunctions.js\"></script>\n";
   echo "    <title>Fröling " . $_SESSION['heatingType'] . "</title>\n";
   echo "  </head>\n";
   echo "  <body>\n";

   // menu button bar ...

   echo "    <a href=\"main.php\"><button class=\"rounded-border button1\">Aktuell</button></a>\n";
   echo "    <a href=\"chart.php\"><button class=\"rounded-border button1\">Charts</button></a>\n";

   if ($_SESSION['chart34'] == "1")
      echo "    <a href=\"chart2.php\"><button class=\"rounded-border button1\">Charts...</button></a>\n";

   echo "    <a href=\"schemadsp.php\"><button class=\"rounded-border button1\">Schema</button></a>\n";
   echo "    <a href=\"menu.php\"><button class=\"rounded-border button1\">Menü</button></a>\n";
   echo "    <a href=\"error.php\"><button class=\"rounded-border button1\">Fehler</button></a>\n";

   if (haveLogin())
   {
      echo "    <a href=\"basecfg.php\"><button class=\"rounded-border button1\">Setup</button></a>\n";
      echo "    <a href=\"logout.php\"><button class=\"rounded-border button1\">Logout</button></a>\n";
   }
   else
   {
      echo "    <a href=\"login.php\"><button class=\"rounded-border button1\">Login</button></a>\n";
   }

   echo "    <div class=\"content\">\n";
}

?>
