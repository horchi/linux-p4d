<?php

include("header.php");
include("setup.php");

// -------------------------
// establish db connection

mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
mysql_select_db($mysqldb) or die("<br/>DB error");
mysql_query("set names 'utf8'");
mysql_query("SET lc_time_names = 'de_DE'");

// ----------------
// variables

$action = "";

if (isset($_POST["action"]))
   $action = htmlspecialchars($_POST["action"]);

if (isset($_POST["style"]))
   $style = htmlspecialchars($_POST["style"]);

// ----------------
// store settings

if ($action == "store")
{
   storeColorScheme($style);
   storeConfigItem("Chart1", "");
   storeConfigItem("Chart2", "");
}

// ----------------
// setup form

echo "      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n"; 
echo "        <br/>\n";
echo "        <button class=\"button3\" type=submit name=action value=store>Speichern</button>\n";
echo "        <br/></br>\n";

// ------------------------
// to be implemented

$mail = true;
$stateMailTo = "hausmeister@krause.de";
$stateMailStates = "0,1,3,19";
$errorMailTo = "hausmeister@krause.de,dackel@krause.de";
$mailScript = "/usr/local/bin/p4d-mail.sh";

// ------------------------
// setup items ...

seperator("!! Allg. Konfiguration !! Seite In Vorbereitung nocht NICHT aktiv !!");

seperator("Web Interface");
colorScheme();

seperator("Charting");
configStrItem("Chart 1", "addr1", $addrs_chart1, "Komma separierte Werte-Adressen, siehe 'Aufzeichnung'", 400);
configStrItem("Chart 2", "addr2", $addrs_chart2, "Komma separierte Werte-Adressen, siehe 'Aufzeichnung'", 400);

seperator("Mail Benachrichtigungen");
configBoolItem("Mail Benachrichtigung", "mail", $mail, "Mail Benachrichtigungen aktivieren/deaktivieren");
configStrItem("Status Mail Empfänger", "stateMailTo", $stateMailTo, "Komma separierte Empängerliste", 500);
configStrItem("Fehler Mail Empfänger", "errorMailTo", $errorMailTo, "Komma separierte Empängerliste", 500);
configStrItem("Status Mail für folgende Status", "stateMailStates", $stateMailStates, "Komma separierte Liste der Status", 400);
configStrItem("p4d sendet Mails über das Skript", "mailScript", $mailScript, "", 400);

echo "      </form>\n";

include("footer.php");

// ---------------------------------------------------------------------------
// Seperator
// ---------------------------------------------------------------------------

function seperator($title)
{
   echo "        <div class=\"seperatorTitle\">\n";
   echo "          <center>$title</center>\n";
   echo "        </div><br/>\n";
}

// ---------------------------------------------------------------------------
// Color Scheme
// ---------------------------------------------------------------------------

function colorScheme()
{
   $actual = readlink("stylesheet.css");
   
   echo "        <div class=\"input\">\n";
   echo "          Farbschema:\n";
   echo "          <select class=checkbox name=\"style\">\n";
   
   foreach (glob("stylesheet-*.css") as $filename) 
   {
      $sel = $actual == $filename ? "SELECTED" : "";
      $tp = substr(strstr($filename, ".", true), 11);
      echo "            <option value='$tp' " . $sel . ">$tp</option>\n";
   }
   
   echo "          </select>\n";
   echo "        </div><br/>\n";
}

function storeColorScheme($style)
{
   $target = "stylesheet-$style.css";

   if ($target != readlink("stylesheet.css"))
   {
      if (!unlink("stylesheet.css") || !symlink($target, "stylesheet.css"))
      {
         $err = error_get_last();
         echo "      <br/><br/>Fehler beim Löschen/Anlegen des Links 'stylesheet.css'<br\>\n";
         
         if (strstr($err['message'], "Permission denied"))
            echo "      <br/>Rechte der Datei prüfen!<br/><br\>\n";
         else
            echo "      <br/>Fehler: '" . $err['message'] . "'<br/>\n";
      }
      else
         echo '<script>parent.window.location.reload(true);</script>';
   }
}

// ---------------------------------------------------------------------------
// Text Config items
// ---------------------------------------------------------------------------

function configStrItem($title, $name, $value, $comment = "", $width = 200)
{
   $actual = readlink("stylesheet.css");
   
   echo "        <div class=\"input\">\n";
   echo "          $title:\n";

   echo "          <input class=\"inputEdit\" style=\"width:" . $width . "px\" type=text name=$name value=$value></input>\n";

   if ($comment != "")
      echo "          <span class=\"inputComment\"> &nbsp;($comment)</span>\n";

   echo "          <br/>\n";
   echo "        </div><br/>\n";

}

function configBoolItem($title, $name, $value, $comment = "")
{
   $actual = readlink("stylesheet.css");
   
   echo "        <div class=\"input\">\n";
   echo "          $title:\n";

   echo "          <input type=checkbox name=$name value=$value></input>\n";

   if ($comment != "")
      echo "          <span class=\"inputComment\"> &nbsp;($comment)</span>\n";

   echo "          <br/>\n";
   echo "        </div><br/>\n";

}

// ---------------------------------------------------------------------------
// Write Config Item
// ---------------------------------------------------------------------------

function writeConfigItem($name, $value)
{
   if (requestAction("write-config", 3, 0, "$name:$value", $res) != 0)
   {
      echo " <br/>failed to write config item $name\n";
      return -1;
   }

   return 0;
}

// ---------------------------------------------------------------------------
// Read Config Item
// ---------------------------------------------------------------------------

function readConfigItem($name, &$value)
{
   if (requestAction("read-config", 3, 0, "$name", $res) != 0)
   {
      echo " <br/>failed to read config item $name\n";
      return -1;
   }

   return 0;
}

?>
