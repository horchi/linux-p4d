<?php

include("header.php");
include("setup.php");

// -------------------------
// establish db connection

mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
mysql_select_db($mysqldb) or die("<br/>DB error");
mysql_query("set names 'utf8'");
mysql_query("SET lc_time_names = 'de_DE'");

// ------------------
// variables

$action = "";

// ------------------
// get post

if (isset($_POST["action"]))
   $action = htmlspecialchars($_POST["action"]);

if ($action == "store")
{
   // ------------------
   // store settings

   if (isset($_POST["style"]))
      $style = htmlspecialchars($_POST["style"]);
   
   if (isset($_POST["chart1"]))
      $_SESSION['chart1'] = htmlspecialchars($_POST["chart1"]);
   
   if (isset($_POST["chart2"]))
      $_SESSION['chart2'] = htmlspecialchars($_POST["chart2"]);
   
   if (isset($_POST["mail"]))
      $_SESSION['mail'] = true;
   else
      $_SESSION['mail'] = false;
   
   if (isset($_POST["stateMailTo"]))
      $_SESSION['stateMailTo'] = htmlspecialchars($_POST["stateMailTo"]);
   
   if (isset($_POST["stateMailStates"]))
      $_SESSION['stateMailStates'] = htmlspecialchars($_POST["stateMailStates"]);
   
   if (isset($_POST["errorMailTo"]))
      $_SESSION['errorMailTo'] = htmlspecialchars($_POST["errorMailTo"]);

   if (isset($_POST["mailScript"]))
      $_SESSION['mailScript'] = htmlspecialchars($_POST["mailScript"]);

   if (isset($_POST["user"]))
      $_SESSION['user'] = htmlspecialchars($_POST["user"]);

   if (isset($_POST["passwd1"]) && $_POST["passwd1"] != "")
      $_SESSION['passwd1'] = md5(htmlspecialchars($_POST["passwd1"]));
   
   if (isset($_POST["passwd2"]) && $_POST["passwd2"] != "")
      $_SESSION['passwd2'] = md5(htmlspecialchars($_POST["passwd2"]));

   if (isset($_POST["tsync"]))
      $_SESSION['tsync'] = htmlspecialchars($_POST["tsync"]);

   if (isset($_POST["maxTimeLeak"]))
      $_SESSION['maxTimeLeak'] = htmlspecialchars($_POST["maxTimeLeak"]);

   // ------------------
   // store settings

   applyColorScheme($style);

   writeConfigItem("chart1", $_SESSION['chart1']);
   writeConfigItem("chart2", $_SESSION['chart2']);
   writeConfigItem("mail", $_SESSION['mail']);
   writeConfigItem("stateMailTo", $_SESSION['stateMailTo']);
   writeConfigItem("stateMailStates", $_SESSION['stateMailStates']);
   writeConfigItem("errorMailTo", $_SESSION['errorMailTo']);
   writeConfigItem("mailScript", $_SESSION['mailScript']);
   writeConfigItem("tsync", $_SESSION['tsync']);
   writeConfigItem("maxTimeLeak", $_SESSION['maxTimeLeak']);

   if ($_POST["passwd2"] != "")
   {
      if ($_POST["passwd1"] == $_POST["passwd2"])
      {
         writeConfigItem("user", $_SESSION['user']);
         writeConfigItem("passwd", $_SESSION['passwd1']);
         echo "      <br/><div class=\"info\"><b><center>Passwort gespeichert</center></div><br/>\n";
      }
      else
      {
         echo "      <br/><div class=\"infoError\"><b><center>Passwort stimmt nicht überein</center></div><br/>\n";
      }
   }
}

// ------------------
// setup form

echo "      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n"; 
echo "        <br/>\n";
echo "        <button class=\"button3\" type=submit name=action value=store>Speichern</button>\n";
echo "        <br/></br>\n";

// ------------------------
// setup items ...

seperator("Web Interface", 0, 1);
colorSchemeItem("Farbschema");

seperator("Charting", 0, 2);
configStrItem("Chart 1", "chart1", $_SESSION['chart1'], "Komma separierte Werte-Adressen, siehe 'Aufzeichnung'", 400);
configStrItem("Chart 2", "chart2", $_SESSION['chart2'], "Komma separierte Werte-Adressen, siehe 'Aufzeichnung'", 400);

seperator("Login", 0, 2);
configStrItem("User", "user", $_SESSION['user'], "", 400);
configStrItem("Passwort", "passwd1", "", "", 350, true);

seperator("Daemon Konfiguration", 0, 1);

seperator("Mail Benachrichtigungen", 0, 2);
configBoolItem("Mail Benachrichtigung", "mail", $_SESSION['mail'], "Mail Benachrichtigungen aktivieren/deaktivieren");
configStrItem("Status Mail Empfänger", "stateMailTo", $_SESSION['stateMailTo'], "Komma separierte Empängerliste", 500);
configStrItem("Fehler Mail Empfänger", "errorMailTo", $_SESSION['errorMailTo'], "Komma separierte Empängerliste", 500);
configStrItem("Status Mail für folgende Status", "stateMailStates", $_SESSION['stateMailStates'], "Komma separierte Liste der Status", 400);
configStrItem("p4d sendet Mails über das Skript", "mailScript", $_SESSION['mailScript'], "", 400);

seperator("Sonstiges", 0, 2);
configBoolItem("Tägliche Zeitsynchronisation", "tsync", $_SESSION['tsync'], "Zeit der Heizung täglich um 23:00 synchronisieren?");
configStrItem("Maximale Abweichung [s]", "maxTimeLeak", $_SESSION['maxTimeLeak'], "Abweichung ab der die tägliche Synchronisation ausgeführt wird");

echo "      </form>\n";

include("footer.php");

// ---------------------------------------------------------------------------
// Color Scheme
// ---------------------------------------------------------------------------

function colorSchemeItem($title)
{
   $actual = readlink("stylesheet.css");
   
   echo "        <div class=\"input\">\n";
   echo "          $title: \n";
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

function applyColorScheme($style)
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

function configStrItem($title, $name, $value, $comment = "", $width = 200, $ispwd = false)
{
   echo "        <div class=\"input\">\n";
   echo "          $title: \n";

   if ($ispwd)
   {
      echo "          <input class=\"inputEdit\" style=\"width:" . $width . "px\" type=\"password\" name=\"passwd1\" value=\"$value\"></input>\n";
      echo "          &nbsp;&nbsp;&nbsp;wiederholen:&nbsp;\n";
      echo "          <input class=\"inputEdit\" style=\"width:" . $width . "px\" type=\"password\" name=\"passwd2\" value=\"$value\"></input>\n";
   }
   else
      echo "          <input class=\"inputEdit\" style=\"width:" . $width . "px\" type=\"text\" name=\"$name\" value=\"$value\"></input>\n";

   if ($comment != "")
      echo "          <span class=\"inputComment\"> &nbsp;($comment)</span>\n";

   echo "          <br/>\n";
   echo "        </div><br/>\n";

}

function configBoolItem($title, $name, $value, $comment = "")
{
   echo "        <div class=\"input\">\n";
   echo "          $title:\n";

   echo "          <input type=checkbox name=$name" . ($value ? " checked" : "") . "></input>\n";

   if ($comment != "")
      echo "          <span class=\"inputComment\"> &nbsp;($comment)</span>\n";

   echo "          <br/>\n";
   echo "        </div><br/>\n";

}

?>
