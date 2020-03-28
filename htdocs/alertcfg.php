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
   // store settings

   $ID = explode("|", $_POST["cnt"]);

   for ($i = 1; $i <= intval($_POST["id"]); $i++)
   {
      $type    = $_POST["Type(" . $ID[$i] . ")"];
      $adr     = $_POST["Adr(" . $ID[$i] . ")"];
      $int     = $_POST["Int(" . $ID[$i] . ")"];

      $min     = $_POST["min(" . $ID[$i] . ")"];
      $max     = $_POST["max(" . $ID[$i] . ")"];
      $delta   = $_POST["Delta(" . $ID[$i] . ")"];
      $range   = $_POST["Range(" . $ID[$i] . ")"];

      $madr    = $_POST["MAdr(" . $ID[$i] . ")"];
      $msub    = $_POST["MSub(" . $ID[$i] . ")"];
      $mbod    = $_POST["MBod(" . $ID[$i] . ")"];
      $act     = ($_POST["Act(" . $ID[$i] . ")"]) ? "A" : "D";
      $time    = time();
      $body    = $mysqli->real_escape_string($mbod);
      $subject = $mysqli->real_escape_string($msub);

      if (!is_numeric($min))   $min = 0;
      if (!is_numeric($max))   $max = 0;
      if (!is_numeric($delta)) $delta = 0;
      if (!is_numeric($range)) $range = 0;

      $data = " address='$adr', type='" . mb_strtoupper($type) . "', min='$min', max='$max', "
         . "maxrepeat='$int', delta='$delta', rangem='$range', "
         . "maddress='$madr', msubject='$subject', mbody='$body', state='$act', kind='M' ";

      if ($i == count($ID)-1 && $adr != "")
      {
         $stmt = "insert into sensoralert set inssp=$time, updsp=$time, ";
         $mysqli->query($stmt . $data)
            or die("<br/>Error: " . $mysqli->error);
      }

      if ($i < count($ID)-1)
      {
         $stmt = "update sensoralert set updsp=$time, ";
         $where = "where id=" . $ID[$i];
         $mysqli->query($stmt . $data . $where)
            or die("<br/>Error: " . $mysqli->error);
      }
   }
}

else if (substr($action, 0, 6) == "delete")
{
   // delete entry

   $mysqli->query("delete from sensoralert where id=" . substr($action, 6))
      or die("<br/>Error: " . $mysqli->error);
}

else if (substr($action, 0, 8) == "mailtest")
{
   $resonse = "";

   if (sendTestMail("", "", $resonse, substr($action, 8)))
      echo "      <br/><div class=\"info\"><b><center>Mail Test succeeded</center></div><br/>\n";
   else
      echo "      <br/><div class=\"infoError\"><b><center>Sending Mail failed '$resonse' - p4d/syslog log for further details</center></div><br/>\n";
}

// ------------------
// setup form

$i = 0; $cnt = "0";

echo "      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n";
echo "        <div class=\"menu\">\n";
echo "          <button class=\"rounded-border button3\" type=\"submit\" name=\"action\" value=\"store\">Speichern</button>\n";
echo "        </div>\n";

// ------------------------
// setup items ...

seperator("Benachrichtigungen bei bestimmten Sensor-Werten", 0);
seperator("Bedingungen<span class=\"help\" onClick=\"showContent('hlp')\">[Hilfe]</span>", 0, "seperatorTitle2");

echo "        <div class=\"rounded-border inputTable\" id=\"hlp\" style=\"display:none;\">\n";
echo "          <span class=\"inputComment\">
                Hier werden die die Bedingungen für die Alarmwerte der einzelnen Sensoren definiert. Dabei gilt wieder: Sensor-ID und -Typ aus der Tabelle <br/>
                'Aufzeichnung' entnehmen und hier eintragen. Mit 'aktiv' wird die Benachrichtig aktiviert bzw. deaktiviert.<br/><br/>\n
                <b>Beispiel:</b> Nachricht wenn die Kesselstellgröße unter 50% sinkt, oder sich mehr als 10% in 1min ändert, aber nicht öfter als alle 5min.<br/>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; <b>ID:18&nbsp;&nbsp; Typ:VA&nbsp;&nbsp; min:50&nbsp;&nbsp;
                max:100&nbsp;&nbsp; Intervall:5&nbsp;&nbsp; Änderung:10&nbsp;&nbsp; Zeitraum:1</b><br/><br/>
                <b>Templates:</b><br/>\n
                Für Betreff und Inhalt können folgende Templates verwendet werden:<br/>\n
                %sensorid%<br/> %title%<br/> %value%<br/> %minv%<br/> %maxv%<br/> %unit%<br/> %min%<br/> %max%<br/> %repeat%<br/> %delta%<br/> %range%<br/> %time%<br/> %weburl%<br/>\n
               </span>\n";
echo "        </div>\n";

$result = $mysqli->query("select * from sensoralert")
   or die("<br/>Error: " . $mysqli->error);

while ($row = $result->fetch_array(MYSQLI_ASSOC))
{
   $ID =  $row['id'];
   $i++;
   $cnt = $cnt . "|" . $row['id'];
   $style = ($row['state'] == "D") ? "; background-color:#ddd\" readOnly=\"true" : "";

   displayAlertConfig($ID, $row, $style);
}

$mysqli->close();
$ID++;
$cnt = $cnt . "|" . $ID;

displayAlertConfig($ID, $row, "");

echo "        <input type=hidden name=id value=" . ($i+1) . ">\n";
echo "        <input type=hidden name=cnt value=" . $cnt . ">\n";
echo "      </form>\n";

include("footer.php");

//***************************************************************************
// Display Alert Config Block
//***************************************************************************

function displayAlertConfig($ID, $row, $style)
{
   $a = chr($ID+64);

   echo "        <div class=\"rounded-border inputTable\">\n";
   echo "         <div>\n";
   echo "           <span>Aktiv</span>\n";
   echo "           <span><input type=checkbox name=\"Act(" . $ID . ")\"" .(($row['state'] == "A") ? " checked" : "") . " onClick=\"readonlyContent('$a',this)\" onLoad=\"disableContent('$a',this)\"></input></span>\n";
   echo "           <span><button class=\"rounded-border button2\" type=\"submit\" id=\"$a\" name=\"action\" value=\"mailtest$ID\">Test Mail</button></span>\n";
   echo "           <span><button class=\"rounded-border button2\" type=\"submit\" name=\"action\" value=\"delete$ID\" onclick=\"return confirmSubmit('diesen Eintrag wirklich löschen?')\">Löschen</button></span>\n";
   echo "         </div>\n";
   echo "         <div>\n";
   echo "           <span>Intervall:</span>\n";
   echo "           <span><input class=\"rounded-border input\" style=\"width:60px$style\" type=\"text\" id=\"$a\" name=\"Int(" . $ID . ")\"   value=\"" . $row['maxrepeat'] . "\"></input> Minuten</span>\n";
   echo "         </div>\n";
   echo "         <div>\n";
   echo "           <span>ID:</span>\n";
   echo "           <span><input class=\"rounded-border input\" style=\"width:60px$style\" type=\"text\" id=\"$a\" name=\"Adr(" . $ID . ")\"   value=\"" . $row['address'] . "\"></input></span>\n";
   configOptionItem(5, "Typ", "Type(" . $ID . ")", $row['type'], "VA:VA UD:US DI:DI DO:DO W1:W1", "", "id=\"$a\" style=\"width:60px$style\"");
   echo "         </div>\n";

   echo "         <div>\n";
   echo "           <span>Minimum:</span>\n";
   echo "           <span><input class=\"rounded-border input\" style=\"width:60px$style\" type=\"text\" id=\"$a\" name=\"min(" . $ID . ")\"   value=\"" . $row['min'] . "\"></input></span>\n";
   echo "           <span>Maximum:</span>\n";
   echo "           <span><input class=\"rounded-border input\" style=\"width:60px$style\" type=\"text\" id=\"$a\" name=\"max(" . $ID . ")\"   value=\"" . $row['max'] . "\"></input></span>\n";
   echo "         </div>\n";

   echo "         <div>\n";
   echo "           <span>Änderung:</span>\n";
   echo "           <span><input class=\"rounded-border input\" style=\"width:60px$style\" type=\"text\" id=\"$a\" name=\"Delta(" . $ID . ")\" value=\"" . $row['delta'] . "\"></input> %</span>\n";
   echo "           <span>im Zeitraum:</span>\n";
   echo "           <span><input class=\"rounded-border input\" style=\"width:60px$style\" type=\"text\" id=\"$a\" name=\"Range(" . $ID . ")\" value=\"" . $row['rangem'] . "\"></input> Minuten</span>\n";
   echo "         </div>\n";

   echo "         <div>\n";
   echo "           <span>Empfänger:</span>\n";
   echo "           <span><input class=\"rounded-border input\" style=\"width:805px$style\" type=\"text\" id=\"$a\" name=\"MAdr(" . $ID . ")\"  value=\"" . $row['maddress'] . "\"></input></span>\n";
   echo "         </div>\n";
   echo "         <div>\n";
   echo "           <span>Betreff:</span>\n";
   echo "           <span><input class=\"rounded-border input\" style=\"width:805px$style\" type=\"text\" id=\"$a\" name=\"MSub(" . $ID . ")\"  value=\"" . $row['msubject'] . "\"></input></span>\n";
   echo "         </div>\n";
   echo "         <div>\n";
   echo "           <span>Inhalt:</span>\n";
   echo "           <span><textarea class=\"rounded-border input\" rows=\"5\" style=\"width:805px$style\" id=\"$a\" name=\"MBod(" . $ID . ")\">" . $row['mbody'] . "</textarea></span>\n";
   echo "         </div>\n";
   echo "        </div>\n";
}

?>
