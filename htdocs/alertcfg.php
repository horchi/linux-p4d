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

  for ($i=1; intval($_POST["id"]); $i++)
  { 
    $ID    = ($i < 10) ? "0".$i : $i;
    $adr   = "Adr" . $ID;
    $type  = "Type" . $ID;
    $min   = "min" . $ID;
    $max   = "max" . $ID;
    $int   = "Int" . $ID;
    $delta = "Delta" . $ID;
    $range = "Range" . $ID;
    $madr  = "MAdr" . $ID;
    $msub  = "MSub" . $ID;
    $mbod  = "MBod" . $ID;
    $act   = ($_POST["Act$ID"]) ? "A" : "D";

    $data = "update sensoralert set address=\"$_POST[$adr]\", type=\"$_POST[$type]\", min=\"$_POST[$min]\", max=\"$_POST[$max]\", maxrepeat=\"$_POST[$int]\", delta=\"$_POST[$delta]\", rangem=\"$_POST[$range]\", maddress=\"$_POST[$madr]\", msubject=\"$_POST[$msub]\", mbody=\"$_POST[$mbod]\", state=\"$act\" where id=$i";
    $update = mysql_query($data) or die("<br/>Error: " . mysql_error());
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


seperator("Benachrichtigung bei bestimmten Sensor-Werten", 0, 1);
echo "        <div class=\"input\">\n";
echo "          Adresse deiner Visualisierung:<input class=\"inputEdit2\" style=\"width:400px\" type=\"text\" name=\"Link\" value=\"" . $_SESSION['link'] . "\"></input>&nbsp;\n";
echo "          <span class=\"inputComment\">&nbsp;(taucht dann in der Mail auf)</span>\n";
echo "        </div><br/>\n";
seperator("Bedingungen", 0, 2);
echo "        <div class=\"input\">\n";
echo "          <span class=\"inputComment\">
                Hier formulierst du die Bedingungen (Alarmwerte) für die einzelnen Sensoren, dabei gilt wieder: Sensor-ID und Typ aus der Tabelle <br />
                'Aufzeichnung' entnehmen und hier eintragen.<br /><br />
                Beispiel: Benachrichtigung wenn die Kesselstellgröße unter 50% sinkt, oder sich mehr als 10% in 1min ändert, aber nicht öfter als alle 5min.<br />
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; ID:18 Typ:VA min=50 max=100 Intervall=5 Änderung=10 Zeitraum=1<br /><br />
                für Betreff und Text können folgende Platzhalter verwendet werden: %sensorid% %title% %value% %unit% %min% %max% %delta% %time% %repeat% %range%<br />
                mit 'aktiv' aktiviert oder deaktiviert ihr die Benachrichtigung, auf die Steuerung hat dies keinen Einfluss
                </span>";
echo "          <span class=\"inputComment\">&nbsp;(taucht dann in der Mail auf)</span>\n";
echo "        </div><br/>\n";

$result = mysql_query("select * from sensoralert") or die("<br/>Error: " . mysql_error());
     
  while ($row = mysql_fetch_array($result, MYSQL_ASSOC))
  {
    $ID = ($row['id'] < 10) ? "0" . $row['id'] : $row['id'];
		echo "        <div class=\"input\">\n";
		echo "          <input type=hidden name=id value=$ID>";
		echo "          <input type=checkbox name=Act" . $ID . (($row['state'] == "A") ? " checked" : "") . "></input> aktiv?\n";
		echo "          ID:<input class=\"inputEdit2\" style=\"width:33px\" type=\"text\" name=\"Adr" . $ID . "\" value=\"" . $row['address'] . "\"></input>&nbsp;\n";
		echo "          Type:<input class=\"inputEdit2\" style=\"width:33px\" type=\"text\" name=\"Type" . $ID . "\" value=\"" . $row['type'] . "\"></input>&nbsp;\n";
		echo "          min:<input class=\"inputEdit2\" style=\"width:42px\" type=\"text\" name=\"min" . $ID . "\" value=\"" . $row['min'] . "\"></input>&nbsp;\n";
		echo "          max:<input class=\"inputEdit2\" style=\"width:42px\" type=\"text\" name=\"max" . $ID . "\" value=\"" . $row['max'] . "\"></input>&nbsp;\n";
		echo "          Intervall:<input class=\"inputEdit2\" style=\"width:42px\" type=\"text\" name=\"Int" . $ID . "\" value=\"" . $row['maxrepeat'] . "\"></input>&nbsp;\n";
		echo "          Änderung:<input class=\"inputEdit2\" style=\"width:33px\" type=\"text\" name=\"Delta" . $ID . "\" value=\"" . $row['delta'] . "\"></input>&nbsp;\n";
		echo "          Zeitraum:<input class=\"inputEdit2\" style=\"width:42px\" type=\"text\" name=\"Range" . $ID . "\" value=\"" . $row['rangem'] . "\"></input>&nbsp;\n";
		echo "          <br /><br />\n";
		echo "          Mail-Empfänger:<input class=\"inputEdit2\" style=\"width:260px\" type=\"text\" name=\"MAdr" . $ID . "\" value=\"" . $row['maddress'] . "\"></input>&nbsp;\n";
		echo "          Betreff:<input class=\"inputEdit2\" style=\"width:450px\" type=\"text\" name=\"MSub" . $ID . "\" value=\"" . $row['msubject'] . "\"></input>&nbsp;\n";
		echo "          <br /><br />\n";
		echo "          <span style=\"vertical-align:top\">Mail-Inhalt:</span><textarea class=\"inputEdit2\"  cols=\"400\" rows=\"7\" style=\"width:805px; position:relative; left:45px;\" name=\"MBod" . $ID . "\">" . $row['mbody'] . "</textarea>&nbsp;\n";
		echo "        </div><br />\n";
	}
	
mysql_close();
echo "      </form>\n";

include("footer.php");



?>
