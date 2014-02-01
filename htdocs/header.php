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
   
   mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
   mysql_select_db($mysqldb) or die("<br/>DB error");
   mysql_query("set names 'utf8'");
   mysql_query("SET lc_time_names = 'de_DE'");
   
   // ------------------
   // get configuration
   
   readConfigItem("chart1", $_SESSION['chart1']);
   readConfigItem("chart2", $_SESSION['chart2']);
   
   readConfigItem("user", $_SESSION['user']);
   readConfigItem("passwd", $_SESSION['passwd']);
   
   readConfigItem("mail", $_SESSION['mail']);
   readConfigItem("stateMailTo", $_SESSION['stateMailTo']);
   readConfigItem("stateMailStates", $_SESSION['stateMailStates']);
   readConfigItem("errorMailTo", $_SESSION['errorMailTo']);
   readConfigItem("mailScript", $_SESSION['mailScript']);
   
   readConfigItem("tsync", $_SESSION['tsync']);
   readConfigItem("maxTimeLeak", $_SESSION['maxTimeLeak']);
   readConfigItem("heatingType", $_SESSION['heatingType']);
   
   // ------------------
   // check for defaults
   
   if ($_SESSION['chart1'] == "")
      $_SESSION['chart1'] = "0,1,113";
   
   if ($_SESSION['chart2'] == "")
      $_SESSION['chart2'] = "118,120,21,25,4";
   
   if ($_SESSION['heatingType'] == "")
      $_SESSION['heatingType'] = "p4";

   mysql_close();
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
   
   if ($refresh)
      echo "    <meta http-equiv=\"refresh\" content=\"$refresh\">\n";
   
   echo "    <meta name=\"author\" content=\"Jörg Wendel\">
    <meta name=\"copyright\" content=\"Jörg Wendel\">
    <link rel=\"stylesheet\" type=\"text/css\" href=\"stylesheet.css\">
    <title>Fröhling P4</title>
  </head>
  <body>
    <a class=\"button1\" href=\"main.php\">Aktuell</a>
    <a class=\"button1\" href=\"chart.php\">Charts</a>
    <a class=\"button1\" href=\"schemadsp.php\">Schema</a>
    <a class=\"button1\" href=\"menu.php\">Menü</a>
    <a class=\"button1\" href=\"error.php\">Fehler</a>\n";

   if (haveLogin())
   {
      echo "    <a class=\"button1\" href=\"basecfg.php\">Setup</a>\n";
      echo "    <a class=\"button1\" href=\"logout.php\">Logout</a>\n";
   }
   else
      echo "    <a class=\"button1\" href=\"login.php\">Login</a>\n";
   
   echo "    <div class=\"content\">\n";
}

?>
