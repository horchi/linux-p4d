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

$mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb, $mysqlport);
$mysqli->query("set names 'utf8'");
$mysqli->query("SET lc_time_names = 'de_DE'");

// ------------------
// variables

$action = "";

// ------------------
// get post

if (isset($_POST["action"]))
   $action = htmlspecialchars($_POST["action"]);

if ($action == "mailtest")
{
   $resonse = "";

   if (sendTestMail("Test Mail", "test", $resonse))
      echo "      <br/><div class=\"info\"><b><center>Mail Test succeeded</center></div><br/>\n";
   else
      echo "      <br/><div class=\"infoError\"><b><center>Sending Mail failed '$resonse' - p4d log for further details</center></div><br/>\n";
}

else if ($action == "store")
{
   // ------------------
   // store settings

   if (isset($_POST["heatingType"]))
      $_SESSION['heatingType'] = htmlspecialchars($_POST["heatingType"]);

   $_SESSION['stateAni'] = isset($_POST['stateAni']);

   if (isset($_POST["style"]))
      $style = htmlspecialchars($_POST["style"]);

   if (isset($_POST["addrsMain"]))
      $_SESSION['addrsMain'] = htmlspecialchars($_POST["addrsMain"]);

   if (isset($_POST["addrsMainMobile"]))
      $_SESSION['addrsMainMobile'] = htmlspecialchars($_POST["addrsMainMobile"]);

   if (isset($_POST["chartStart"]))
      $_SESSION['chartStart'] = htmlspecialchars($_POST["chartStart"]);

   $_SESSION['chartXLines'] = isset($_POST["chartXLines"]);

   if (isset($_POST["chartDiv"]))
      $_SESSION['chartDiv'] = htmlspecialchars($_POST["chartDiv"]);

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

   $_SESSION['mail'] = isset($_POST["mail"]);
   $_SESSION['htmlMail'] = isset($_POST["htmlMail"]);

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

   if (isset($_POST["hmHost"]))
      $_SESSION['hmHost'] = htmlspecialchars($_POST["hmHost"]);

   // ------------------
   // store settings

   applyColorScheme($style);

   writeConfigItem("addrsMain", $_SESSION['addrsMain']);
   writeConfigItem("addrsMainMobile", $_SESSION['addrsMainMobile']);

   writeConfigItem("chartStart", $_SESSION['chartStart']);
   writeConfigItem("chartDiv", $_SESSION['chartDiv']);
   writeConfigItem("chartXLines", $_SESSION['chartXLines']);
   writeConfigItem("chart1", $_SESSION['chart1']);
   writeConfigItem("chart2", $_SESSION['chart2']);
   writeConfigItem("chart34", $_SESSION['chart34']);              // HK2
   writeConfigItem("chart3", $_SESSION['chart3']);
   writeConfigItem("chart4", $_SESSION['chart4']);

   writeConfigItem("mail", $_SESSION['mail']);
   writeConfigItem("htmlMail", $_SESSION['htmlMail']);
   writeConfigItem("stateMailTo", $_SESSION['stateMailTo']);
   writeConfigItem("stateMailStates", $_SESSION['stateMailStates']);
   writeConfigItem("errorMailTo", $_SESSION['errorMailTo']);
   writeConfigItem("mailScript", $_SESSION['mailScript']);
   writeConfigItem("tsync", $_SESSION['tsync']);
   writeConfigItem("maxTimeLeak", $_SESSION['maxTimeLeak']);
   writeConfigItem("heatingType", $_SESSION['heatingType']);
   writeConfigItem("stateAni", $_SESSION['stateAni']);
   writeConfigItem("webUrl", $_SESSION['webUrl']);
   writeConfigItem("hmHost", $_SESSION['hmHost']);

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
echo "        <button class=\"rounded-border button3\" type=submit name=action value=store>Speichern</button>\n";

// ------------------------
// setup items ...

seperator("Web Interface", 0);
colorSchemeItem(1, "Farbschema");
heatingTypeItem(5, "Heizung", $_SESSION['heatingType']);
configBoolItem(4, "Status Gif animiert?", "stateAni", $_SESSION['stateAni'], "");

seperator("Ansicht 'Aktuell'", 0, "seperatorTitle2");
configStrItem(1, "Sensoren", "addrsMain", $_SESSION['addrsMain'], "", 250);
configStrItem(4, "Sensoren (mobil)", "addrsMainMobile", $_SESSION['addrsMainMobile'], "", 250);

seperator("Charting", 0, "seperatorTitle2");
configStrItem(1, "Chart Zeitraum (Tage)", "chartStart", $_SESSION['chartStart'], "Standardzeitraum der Chartanzeige (seit x Tagen bis heute)", 50);
configBoolItem(2, "senkrechte Hilfslinien", "chartXLines", $_SESSION['chartXLines'], "");
configOptionItem(5, "Linien-Abstand der Y-Achse", "chartDiv", $_SESSION['chartDiv'], "klein,15 mittel,25 groß,45", "");
configBoolItem(5, "Chart 3+4", "chart34", $_SESSION['chart34'], "aktivieren?");
configStrItem(2, "Chart 1", "chart1", $_SESSION['chart1'], "", 250);
configStrItem(4, "Chart 2", "chart2", $_SESSION['chart2'], "Werte-ID, siehe 'Aufzeichnung'", 250);

if ($_SESSION['chart34'] == "1")
{
   configStrItem(1, "Chart 3", "chart3", $_SESSION['chart3'], "", 250);
   configStrItem(4, "Chart 4", "chart4", $_SESSION['chart4'], "Werte-ID siehe 'Aufzeichnung'", 250);
}

seperator("Login", 0, "seperatorTitle2");
configStrItem(1, "User", "user", $_SESSION['user'], "", 400);
configStrItem(6, "Passwort", "passwd1", "", "", 350, "", true);

seperator("HomeMatic Interface", 0);
configStrItem(3, "HomeMatic Host/IP", "hmHost", $_SESSION['hmHost'], "", 150);

seperator("p4d Konfiguration", 0);

seperator("Mail Benachrichtigungen", 0, "seperatorTitle2");
$a = ($_SESSION['mail']) ? "" : "disabled=true";
$ro = ($_SESSION['mail']) ? "'" : "; background-color:#ddd;' readOnly=\"true\"";
configBoolItem(1, "Mail Benachrichtigung", "mail\" onClick=\"disableContent('htM',this); readonlyContent('Mail',this)", $_SESSION['mail'], "Mail Benachrichtigungen aktivieren/deaktivieren");
configBoolItem(5, "HTML-Mail?", "htmlMail\" id=\"htM", $_SESSION['htmlMail'], "gilt für alle Mails", $a);
configStrItem(2, "Status Mail Empfänger", "stateMailTo\" id=\"Mail1", $_SESSION['stateMailTo'], "Komma separierte Empängerliste", 500, $ro);
configStrItem(2, "Fehler Mail Empfänger", "errorMailTo\" id=\"Mail2", $_SESSION['errorMailTo'], "Komma separierte Empängerliste", 500, $ro);
configStrItem(2, "Status Mail für folgende Status", "stateMailStates\" id=\"Mail3", $_SESSION['stateMailStates'], "Komma separierte Liste der Stati", 400, $ro);
configStrItem(2, "p4d sendet Mails über das Skript", "mailScript\" id=\"Mail4", $_SESSION['mailScript'], "", 400, $ro);
echo "        <button class=\"rounded-border button3\" type=submit name=action value=mailtest>Test Mail</button>\n";
configStrItem(6, "URL deiner Visualisierung", "webUrl\" id=\"Mail5", $_SESSION['webUrl'], "kann mit %weburl% in die Mails eingefügt werden", 350, $ro);

seperator("Sonstiges", 0, "seperatorTitle2");
$ro = ($_SESSION['tsync']) ? "'" : "; background-color:#ddd;' readOnly=\"true\"";
configBoolItem(1, "Zeitsynchronisation", "tsync\" onClick=\"readonlyContent('timeLeak',this)", $_SESSION['tsync'], "tägl. 23:00Uhr");
configStrItem(4, "Mind. Abweichung [s]", "maxTimeLeak\" id=\"timeLeak", $_SESSION['maxTimeLeak'], "Mindestabweichung für Synchronisation", 45, $ro);

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
   echo "          <select class=\"rounded-border input\" name=\"style\">\n";

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
   	  if (!readlink("stylesheet.css"))
   	    symlink($target, "stylesheet.css");

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
         echo '<script>parent.window.location.reload();</script>';
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
   echo "          <select class=\"rounded-border input\" name=\"heatingType\">\n";

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

?>
