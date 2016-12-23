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

if ($action == "store")
{
   // ------------------
   // store settings

   $ID    = explode("|", $_POST["cnt"]);

   for ($i = 1; $i <= intval($_POST["id"]); $i++)
   {
      $adr   = "Adr(" . $ID[$i] . ")";
      $type  = "Type(" . $ID[$i] . ")";
      $min   = "min(" . $ID[$i] . ")";
      $max   = "max(" . $ID[$i] . ")";
      $int   = "Int(" . $ID[$i] . ")";
      $delta = "Delta(" . $ID[$i] . ")";
      $range = "Range(" . $ID[$i] . ")";
      $madr  = "MAdr(" . $ID[$i] . ")";
      $msub  = "MSub(" . $ID[$i] . ")";
      $mbod  = "MBod(" . $ID[$i] . ")";
      $act   = ($_POST["Act($ID[$i])"]) ? "A" : "D";

      $time = time();
      $data = " address=\"$_POST[$adr]\", type=\"" . mb_strtoupper($_POST[$type]) . "\", min=\"$_POST[$min]\", max=\"$_POST[$max]\", maxrepeat=\"$_POST[$int]\", delta=\"$_POST[$delta]\", rangem=\"$_POST[$range]\", maddress=\"$_POST[$madr]\", msubject=\"$_POST[$msub]\", mbody=\"$_POST[$mbod]\", state=\"$act\" ";

      if ($i == count($ID)-1 && $_POST[$adr] != "")
      {
         $update = "insert into sensoralert set inssp=$time, updsp=$time,"; $where = "";
         $insert = $mysqli->query($update . $data . $where)
            or die("<br/>Error: " . $mysqli->error);
      }

      if ($i < count($ID)-1)
      {
         $update = "update sensoralert set updsp=$time,"; $where = "where id=" . $ID[$i];
         $update = $mysqli->query($update . $data . $where)
            or die("<br/>Error: " . $mysqli->error);
      }
   }
}

// ------------------
// delete entry

if (substr($action,0,6) == "delete")
   $update = $mysqli->query("delete from sensoralert where id=" . substr($action,6))
     or die("<br/>Error: " . $mysqli->error);

// ------------------
// setup form

$i = 0; $cnt = "0";

echo "      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n";
echo "        <button class=\"rounded-border button3\" type=\"submit\" name=\"action\" value=\"store\">Speichern</button>\n";

// ------------------------
// setup items ...

seperator("Benachrichtigungen bei bestimmten Sensor-Werten", 0);
seperator("Bedingungen<span class=\"help\" onClick=\"showContent('hlp')\">[Hilfe]</span>", 0, "seperatorTitle2");

echo "        <div class=\"input\" id=\"hlp\" style=\"display:none;\">\n";
echo "          <span class=\"inputComment\">
                Hier formulierst du die Bedingungen (Alarmwerte) für die einzelnen Sensoren, dabei gilt wieder: Sensor-ID und Typ aus der Tabelle <br />
                'Aufzeichnung' entnehmen und hier eintragen.<br /><br />
                <b>Beispiel:</b> Nachricht wenn die Kesselstellgröße unter 50% sinkt, oder sich mehr als 10% in 1min ändert, aber nicht öfter als alle 5min.<br />
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; <b>ID:18&nbsp;&nbsp; Typ:VA&nbsp;&nbsp; min:50&nbsp;&nbsp;
                max:100&nbsp;&nbsp; Intervall:5&nbsp;&nbsp; Änderung:10&nbsp;&nbsp; Zeitraum:1</b><br />
                <b>Zulässige Werte:</b><br /><b>ID:</b> Zahl (auch Hex) | <b>Typ:</b> UD, VA, DI, DO, W1 | <b>min, max, Änderung:</b> Zahl | <b>
                Intervall, Zeitraum:</b> Zahl (Minuten)<br /><br />
                für Betreff und Text können folgende Platzhalter verwendet werden:<br />
                %sensorid% %title% %value% %unit% %min% %max% %repeat% %delta% %range% %time% %weburl%<br />
                mit 'aktiv' aktivierst oder deaktivierst du nur die Benachrichtigung, auf die Steuerung hat dies keinen Einfluss
          </span>\n";
echo "        </div>\n";

$result = $mysqli->query("select * from sensoralert")
   or die("<br/>Error: " . $mysqli->error);

while ($row = $result->fetch_array(MYSQLI_ASSOC))
{
   $ID =  $row['id']; $i++; $a = chr($ID+64);
   $cnt = $cnt . "|" . $row['id']; $s = ($row['state'] == "D") ? "; background-color:#ddd\" readOnly=\"true" : "";
   echo "        <div class=\"input\">\n";
   echo "          <button class=\"rounded-border button2\" type=\"submit\" name=\"action\" value=\"delete$ID\" onclick=\"return confirmSubmit('diesen Eintrag wirklich löschen?')\">Löschen</button>\n";
   echo "          <input type=checkbox name=Act(" . $ID . ")" .(($row['state'] == "A") ? " checked" : "") . " onClick=\"readonlyContent('$a',this)\" onLoad=\"disableContent('$a',this)\"></input> aktiv?\n";
   echo "          ID:<input        class=\"inputEdit\" id=\"a$a\" style=\"width:77px$s\" type=\"text\" name=\"Adr(" . $ID . ")\"   value=\"" . $row['address'] . "\"></input>\n";
   echo "          Typ:<input       class=\"inputEdit\" id=\"b$a\" style=\"width:28px$s\" type=\"text\" name=\"Type(" . $ID . ")\"  value=\"" . $row['type'] . "\"></input>\n";
   echo "          min:<input       class=\"inputEdit\" id=\"c$a\" style=\"width:42px$s\" type=\"text\" name=\"min(" . $ID . ")\"   value=\"" . $row['min'] . "\"></input>\n";
   echo "          max:<input       class=\"inputEdit\" id=\"d$a\" style=\"width:42px$s\" type=\"text\" name=\"max(" . $ID . ")\"   value=\"" . $row['max'] . "\"></input>\n";
   echo "          Intervall:<input class=\"inputEdit\" id=\"e$a\" style=\"width:42px$s\" type=\"text\" name=\"Int(" . $ID . ")\"   value=\"" . $row['maxrepeat'] . "\"></input>\n";
   echo "          Änderung:<input  class=\"inputEdit\" id=\"f$a\" style=\"width:33px$s\" type=\"text\" name=\"Delta(" . $ID . ")\" value=\"" . $row['delta'] . "\"></input>\n";
   echo "          Zeitraum:<input  class=\"inputEdit\" id=\"g$a\" style=\"width:42px$s\" type=\"text\" name=\"Range(" . $ID . ")\" value=\"" . $row['rangem'] . "\"></input>\n";
   echo "          <br/><br/>\n";
   echo "          Empfänger:<input class=\"inputEdit\" id=\"h$a\" style=\"width:260px$s\" type=\"text\" name=\"MAdr(" . $ID . ")\"  value=\"" . $row['maddress'] . "\"></input>\n";
   echo "          Betreff:<input   class=\"inputEdit\" id=\"i$a\" style=\"width:450px$s\" type=\"text\" name=\"MSub(" . $ID . ")\"  value=\"" . $row['msubject'] . "\"></input>\n";
   echo "          <br/><br/>\n";
   echo "          <span style=\"vertical-align:top\">Inhalt:</span>\n";
   echo "          <textarea        class=\"inputEdit\" cols=\"400\" rows=\"7\" id=\"j$a\" style=\"width:805px $s\" name=\"MBod(" . $ID . ")\">" . $row['mbody'] . "</textarea>\n";
   echo "        </div>\n";
}

$mysqli->close();
$ID++;
$cnt = $cnt . "|" . $ID;

echo "        <div class=\"input\">\n";
echo "          <input type=checkbox name=Act(" . $ID . ")" . (($row['state'] == "A") ? " checked" : "") . "></input> aktiv?\n";
echo "          ID:<input        class=\"inputEdit\" style=\"width:33px\" type=\"text\" name=\"Adr(" . $ID . ")\"   value=\"" . $row['address'] . "\"></input>\n";
echo "          Typ:<input       class=\"inputEdit\" style=\"width:33px\" type=\"text\" name=\"Type(" . $ID . ")\"  value=\"" . $row['type'] . "\"></input>\n";
echo "          min:<input       class=\"inputEdit\" style=\"width:42px\" type=\"text\" name=\"min(" . $ID . ")\"   value=\"" . $row['min'] . "\"></input>\n";
echo "          max:<input       class=\"inputEdit\" style=\"width:42px\" type=\"text\" name=\"max(" . $ID . ")\"   value=\"" . $row['max'] . "\"></input>\n";
echo "          Intervall:<input class=\"inputEdit\" style=\"width:42px\" type=\"text\" name=\"Int(" . $ID . ")\"   value=\"" . $row['maxrepeat'] . "\"></input>\n";
echo "          Änderung:<input  class=\"inputEdit\" style=\"width:33px\" type=\"text\" name=\"Delta(" . $ID . ")\" value=\"" . $row['delta'] . "\"></input>\n";
echo "          Zeitraum:<input  class=\"inputEdit\" style=\"width:42px\" type=\"text\" name=\"Range(" . $ID . ")\" value=\"" . $row['rangem'] . "\"></input>\n";
echo "          <br/><br/>\n";
echo "          Empfänger:<input class=\"inputEdit\" style=\"width:260px\" type=\"text\" name=\"MAdr(" . $ID . ")\"  value=\"" . $row['maddress'] . "\"></input>\n";
echo "          Betreff:<input   class=\"inputEdit\" style=\"width:450px\" type=\"text\" name=\"MSub(" . $ID . ")\"  value=\"" . $row['msubject'] . "\"></input>\n";
echo "          <br/><br/>\n";
echo "          <span style=\"vertical-align:top\">Inhalt:</span>\n";
echo "          <textarea        class=\"inputEdit\" cols=\"400\" rows=\"7\" style=\"width:805px\" name=\"MBod(" . $ID . ")\">" . $row['mbody'] . "</textarea>\n";
echo "        </div>\n";


echo "        <input type=hidden name=id value=" . ($i+1) . ">\n";
echo "        <input type=hidden name=cnt value=" . $cnt . ">\n";
echo "      </form>\n";

include("footer.php");

?>
