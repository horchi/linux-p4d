<?php

include("header.php");

printHeader();

include("setup.php");

// -------------------------
// chaeck login

if (!haveLogin())
{
   echo "<br/><div class=\"infoError\"><b><center>Login erforderlich!</center></b></div><br/>\n";
   die("<br/>");
}

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

   if (isset($_POST["heatingType"]))
      $_SESSION['heatingType'] = htmlspecialchars($_POST["heatingType"]);

   // Schema

   if (isset($_POST["schema"]))
      $_SESSION['schema'] = htmlspecialchars($_POST["schema"]);

   if (isset($_POST["style"]))
      $style = htmlspecialchars($_POST["style"]);

   if (isset($_POST["chartStart"]))
      $_SESSION['chartStart'] = htmlspecialchars($_POST["chartStart"]);
   
   if (isset($_POST["chartDiv"]))
      $_SESSION['chartDiv'] = htmlspecialchars($_POST["chartDiv"]);
   
   if (isset($_POST["chartXLines"]))
      $_SESSION['chartXLines'] = 1;
   else
      $_SESSION['chartXLines'] = 0;
   
   if (isset($_POST["chart1"]))
      $_SESSION['chart1'] = htmlspecialchars($_POST["chart1"]);
   
   if (isset($_POST["chart2"]))
      $_SESSION['chart2'] = htmlspecialchars($_POST["chart2"]);
   
   // optional charts for HK2

   $_SESSION['chart34'] = isset($_POST["chart34"]);
	  
   if (isset($_POST["chart3"]))
      $_SESSION['chart3'] = htmlspecialchars($_POST["chart3"]);
   
   if (isset($_POST["chart4"]))
      $_SESSION['chart4'] = htmlspecialchars($_POST["chart4"]);

   // ---

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
      $_SESSION['tsync'] = true;
   else
      $_SESSION['tsync'] = false;

   if (isset($_POST["maxTimeLeak"]))
      $_SESSION['maxTimeLeak'] = htmlspecialchars($_POST["maxTimeLeak"]);

   if (isset($_POST["webUrl"])) 
    $_SESSION['webUrl'] = (substr($_SESSION['webUrl'],0,7) == "http://") ?  htmlspecialchars($_POST["webUrl"]) : htmlspecialchars("http://" . $_POST["webUrl"]); 

   // ------------------
   // store settings

   applyColorScheme($style);

   writeConfigItem("chartStart", $_SESSION['chartStart']);
   writeConfigItem("chartDiv", $_SESSION['chartDiv']);
   writeConfigItem("chartXLines", $_SESSION['chartXLines']);
   writeConfigItem("chart1", $_SESSION['chart1']);
   writeConfigItem("chart2", $_SESSION['chart2']);

   writeConfigItem("chart34", $_SESSION['chart34']);              // HK2
   writeConfigItem("chart3", $_SESSION['chart3']);
   writeConfigItem("chart4", $_SESSION['chart4']);

   writeConfigItem("mail", $_SESSION['mail']);
   writeConfigItem("stateMailTo", $_SESSION['stateMailTo']);
   writeConfigItem("stateMailStates", $_SESSION['stateMailStates']);
   writeConfigItem("errorMailTo", $_SESSION['errorMailTo']);
   writeConfigItem("mailScript", $_SESSION['mailScript']);
   writeConfigItem("tsync", $_SESSION['tsync']);
   writeConfigItem("maxTimeLeak", $_SESSION['maxTimeLeak']);
   writeConfigItem("heatingType", $_SESSION['heatingType']);
   writeConfigItem("schema", $_SESSION['schema']);
   writeConfigItem("webUrl", $_SESSION['webUrl']);

   if ($_POST["passwd2"] != "")
   {
      if (htmlspecialchars($_POST["passwd1"]) ==  htmlspecialchars(($_POST["passwd2"])))
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
colorSchemeItem(3, "Farbschema");

seperator("Charting", 0, 4);
configStrItem(1, "Chart Zeitraum (Tage)", "chartStart", $_SESSION['chartStart'], "Standardzeitraum der Chartanzeige (seit x Tagen bis heute)", 50);
configBoolItem(2, "senkrechte Hilfslinien", "chartXLines", $_SESSION['chartXLines'], "");
configOptionItem(5, "Linien-Abstand der Y-Achse", "chartDiv", $_SESSION['chartDiv'], "klein,15 mittel,25 groß,45", "");
configBoolItem(5, "Chart 3+4", "chart34", $_SESSION['chart34'], "aktivieren?");
/*
configOptionItem(2, "Farbe 1", "col1", $_SESSION['col1'], "grün,gelb,weiss,blau", "");
configOptionItem(5, "Farbe 2", "col2", $_SESSION['col2'], "grün,gelb,weiss,blau", "");
configOptionItem(5, "Farbe 3", "col3", $_SESSION['col3'], "grün,gelb,weiss,blau", "");
configOptionItem(5, "Farbe 4", "col4", $_SESSION['col4'], "grün,gelb,weiss,blau", "");
configOptionItem(5, "Dicke", "thk1", $_SESSION['thk1'], "1,2,3", "");
*/
configStrItem(2, "Chart 1", "chart1", $_SESSION['chart1'], "", 250);
configStrItem(4, "Chart 2", "chart2", $_SESSION['chart2'], "Werte-ID, siehe 'Aufzeichnung'", 250);

if ($_SESSION['chart34'] == "1")
{
   configStrItem(1, "Chart 3", "chart3", $_SESSION['chart3'], "", 250);
   configStrItem(4, "Chart 4", "chart4", $_SESSION['chart4'], "Werte-ID siehe 'Aufzeichnung'", 250);
}

seperator("Login", 0, 2);
configStrItem(1, "User", "user", $_SESSION['user'], "", 400);
configStrItem(6, "Passwort", "passwd1", "", "", 350, true);

seperator("Daemon Konfiguration", 0, 1);

seperator("Mail Benachrichtigungen", 0, 2);
configBoolItem(1, "Mail Benachrichtigung", "mail", $_SESSION['mail'], "Mail Benachrichtigungen aktivieren/deaktivieren");
configStrItem(2, "Status Mail Empfänger", "stateMailTo", $_SESSION['stateMailTo'], "Komma separierte Empängerliste", 500);
configStrItem(2, "Fehler Mail Empfänger", "errorMailTo", $_SESSION['errorMailTo'], "Komma separierte Empängerliste", 500);
configStrItem(2, "Status Mail für folgende Stati", "stateMailStates", $_SESSION['stateMailStates'], "Komma separierte Liste der Stati", 400);
configStrItem(2, "URL deiner Visualisierung", "webUrl", $_SESSION['webUrl'], "kann mit %weburl% in die Mails eingefügt werden", 350);
configStrItem(6, "p4d sendet Mails über das Skript", "mailScript", $_SESSION['mailScript'], "", 400);

seperator("Sonstiges", 0, 2);
configBoolItem(1, "Tägliche Zeitsynchronisation", "tsync", $_SESSION['tsync'], "Zeit der Heizung täglich um 23:00 synchronisieren?");
configStrItem(2, "Maximale Abweichung [s]", "maxTimeLeak", $_SESSION['maxTimeLeak'], "Abweichung ab der die tägliche Synchronisation ausgeführt wird");
heatingTypeItem(2, "Heizung", $_SESSION['heatingType']);
schemaItem(4, "Schema", $_SESSION['schema']);

echo "      </form>\n";

include("footer.php");

// ---------------------------------------------------------------------------
// Color Scheme
// ---------------------------------------------------------------------------

function colorSchemeItem($new, $title)
{
   $actual = readlink("stylesheet.css");
   
   $end = htmTags($new);
   echo "          $title:\n";
   echo "          <select class=checkbox name=\"style\">\n";
   
   foreach (glob("stylesheet-*.css") as $filename) 
   {
      $sel = $actual == $filename ? "SELECTED" : "";
      $tp = substr(strstr($filename, ".", true), 11);
      echo "            <option value='$tp' " . $sel . ">$tp</option>\n";
   }
   
   echo "          </select>\n";
   echo $end;     
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
// Heating Type - p4/s4/...
// ---------------------------------------------------------------------------

function heatingTypeItem($new, $title, $type)
{
   $actual = "heating-$type.png";
   
   $end = htmTags($new);
   echo "          $title:\n";
   echo "          <select class=checkbox name=\"heatingType\">\n";

   $path = "img/type/";

   foreach (glob($path . "heating-*.png") as $filename) 
   { 
      $filename = basename($filename);

      $sel = $actual == $filename ? "SELECTED" : "";
      $tp = substr(strstr($filename, ".", true), 8);
      echo "            <option value='$tp' " . $sel . ">$tp</option>\n";
   }
   
   echo "          </select>\n";
   echo $end;     
}

// ---------------------------------------------------------------------------
// Schema Selection
// ---------------------------------------------------------------------------

function schemaItem($new, $title, $schema)
{
   $actual = "schema-$schema.png";
   
   $end = htmTags($new);
   echo "          $title:\n";
   echo "          <select class=checkbox name=\"schema\">\n";

   $path  = "img/schema/";
   $path .= "schema-*.png";
   
   foreach (glob($path) as $filename) 
   {
      $filename = basename($filename);

      $sel = ($actual == $filename) ? "SELECTED" : "";
      $tp  = substr(strstr($filename, ".", true), 7);
      echo "            <option value='$tp' " . $sel . ">$tp</option>\n";
   }
   
   echo "          </select>\n";
   echo $end;     
}

// ---------------------------------------------------------------------------
// Text Config items
// ---------------------------------------------------------------------------

function configStrItem($new, $title, $name, $value, $comment = "", $width = 200, $ispwd = false)
{
   $end = htmTags($new);
   echo "          $title:\n";

   if ($ispwd)
   {
      echo "          <input class=\"inputEdit\" style=\"width:" . $width . "px\" type=\"password\" name=\"passwd1\" value=\"$value\"></input>\n";
      echo "          &nbsp;&nbsp;&nbsp;wiederholen:&nbsp;\n";
      echo "          <input class=\"inputEdit\" style=\"width:" . $width . "px\" type=\"password\" name=\"passwd2\" value=\"$value\"></input>\n";
   }
   else
      echo "          <input class=\"inputEdit\" style=\"width:" . $width . "px\" type=\"text\" name=\"$name\" value=\"$value\"></input>\n";

   if ($comment != "")
      echo "          <span class=\"inputComment\">&nbsp;($comment)</span>\n";

   echo $end;     
}

// ---------------------------------------------------------------------------
// Checkbox Config items
// ---------------------------------------------------------------------------

function configBoolItem($new, $title, $name, $value, $comment = "")
{
   $end = htmTags($new);
   echo "          $title:\n";
   echo "          <input type=checkbox name=$name" . ($value ? " checked" : "") . "></input>\n";

   if ($comment != "")
      echo "          <span class=\"inputComment\"> &nbsp;($comment)</span>\n";

   echo $end;     

}

// ---------------------------------------------------------------------------
// Option Config Item
// ---------------------------------------------------------------------------

function configOptionItem($new, $title, $name, $value, $options, $comment)
{
   $end = htmTags($new);
   echo "          $title:\n";
   echo "          <select class=checkbox name=$name>\n";

   foreach (explode(" ", $options) as $option)
   {
      $opt = explode(",", $option);
      $sel = ($value == $opt[1]) ? "SELECTED" : "";
      echo "            <option value='$opt[1]' " . $sel . ">$opt[0]</option>\n";
   }
   
   echo "          </select>\n";
   echo $end;     
}

// ---------------------------------------------------------------------------
// Set Start and End Div-Tags
// ---------------------------------------------------------------------------

function htmTags($new)
{
  switch ($new) { 
   	case 1: echo "        <div class=\"input\">\n"; $end = ""; break;
   	case 2: echo "        </div><br/>\n        <div class=\"input\">\n"; $end = ""; break;
   	case 3: echo "        <div class=\"input\">\n" ; $end = "        </div><br/>\n"; break;
   	case 4: echo "          &nbsp;|&nbsp;\n" ; $end = "        </div><br/>\n"; break;
   	case 5: echo "          &nbsp;|&nbsp;\n"; $end = ""; break;
   	case 6: echo "        </div><br/>\n        <div class=\"input\">\n" ; $end = "        </div><br/>\n"; break;
  }
  return $end;
}	

?>
