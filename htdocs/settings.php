<?php

include("header.php");

printHeader();

include("setup.php");

// -------------------------
// check login

if (!haveLogin())
{
   echo "<br/><div class=\"infoError\"><b><center>Login erforderlich!</center></b></div><br/>\n";

   die("<br/>");
}

$action = "";

if (isset($_POST["action"]))
   $action = htmlspecialchars($_POST["action"]);

// -------------------------
// establish db connection

$mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb);
$mysqli->query("set names 'utf8'");
$mysqli->query("SET lc_time_names = 'de_DE'");

// -----------------------
//

if ($action == "init")
{
   requestAction("initvaluefacts", 20, 0, "", $resonse);
   echo "<br/><div class=\"info\"><b><center>Initialisierung abgeschlossen</center></b></div><br/><br/>";
}

else if ($action == "store")
{
    foreach ($_POST['usrtitle'] as $key => $value)
    {
       $value = htmlspecialchars($value);
       $addr = htmlspecialchars($_POST["addr"][$key]);
       list ($addr, $type) = explode(":", $addr);

       $addr = $mysqli->real_escape_string($addr);
       $type = $mysqli->real_escape_string($type);

       $sql = "UPDATE valuefacts set usrtitle = '$value' where address = '$addr' and type = '$type'";
       $mysqli->query($sql)
          or die("<br/>Error" . $mysqli->error());
    }

   if (isset($_POST["selected"]))
   {
      $sql = "UPDATE valuefacts SET state = 'D'";
      $result = $mysqli->query($sql)
         or die("<br/>Error" . $mysqli->error());

      foreach ($_POST['selected'] as $key => $value)
      {
         $value = htmlspecialchars($value);
         list ($addr, $type) = explode(":", $value);

         $addr = $mysqli->real_escape_string($addr);
         $type = $mysqli->real_escape_string($type);

         $sql = "UPDATE valuefacts set state = 'A' where address = '$addr' and type = '$type'";

         $mysqli->query($sql)
            or die("<br/>Error" . $mysqli->error());

         $mysqli->query("update valuefacts set state = 'A' where type = 'UD'")
            or die("<br/>Error" . $mysqli->error());
      }

      requestAction("update-schemacfg", 2, 0, "", $resonse);

      echo "<br/><div class=\"info\"><b><center>Einstellungen gespeichert</center></b></div><br/><br/>";
   }
}

echo "      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n";
showButtons();
showTable("UD", "Allgemein");
showTable("VA", "Messwerte");
showTable("DI", "Digitale Eingänge");
showTable("DO", "Digitale Ausgänge");
showTable("AO", "Analoge Ausgänge");
showTable("W1", "One Wire Sensoren");
echo "      </form>\n";

$mysqli->close();

include("footer.php");

//***************************************************************************
// Show Buttons
//***************************************************************************

function showButtons()
{
   echo "        <div>\n";
   echo "          <br/>\n";
   echo "          <button class=\"button3\" type=submit name=action value=init onclick=\"return confirmSubmit('Stammdaten der Messwerte initialisieren')\">Init</button>\n";
   echo "          <button class=\"button3\" type=submit name=action value=store onclick=\"return confirmSubmit('Einstellungen speichern?')\">Speichern</button>\n";
   echo "        </div>\n";
}

//***************************************************************************
// Show Table
//***************************************************************************

function showTable($type, $tableTitle)
{
   global $debug, $mysqli;

   $i = 0;

   echo "        <br/>\n";
   echo "        <table class=\"tableTitle\" cellspacing=0 rules=rows>\n";
   echo "          <tr>\n";
   echo "            <td><center><b>$tableTitle</b></center></td>\n";
   echo "          </tr>\n";
   echo "        </table>\n";
   echo "        <br/>\n";

   echo "        <table  class=\"tableLight\" border=1 cellspacing=0 rules=rows>
          <tr class=\"tableHead1\">\n";


   echo "            <td width=\"43%\"> Name </td>\n";
   echo "            <td> Bezeichnung </td>\n";
   echo "            <td> Einheit </td>\n";
   echo "            <td> Aufzeichnen </td>\n";
   echo "            <td> ID / Typ </td>\n";
   echo "          </tr>\n";

   $result = $mysqli->query("select * from valuefacts where type = '$type'")
      or die("<br/>Error" . $mysqli->error());

   $num = $result->num_rows;

   while ($i < $num)
   {
      $address = mysqli_result($result, $i, "address");
      $type    = mysqli_result($result, $i, "type");
      $title   = mysqli_result($result, $i, "title");
      $unit    = mysqli_result($result, $i, "unit");
      $state   = mysqli_result($result, $i, "state");
      $usrtitle= mysqli_result($result, $i, "usrtitle");
      $txtaddr = sprintf("0x%04x", $address);

      if ($state == "A")
         $checked = " checked";
      else
         $checked = "";

      if ($i % 2)
         echo "         <tr class=\"tableLight\">";
      else
         echo "         <tr class=\"tableDark\">";


      echo "            <td> $title </td>\n";
      echo "            <td><input class=\"inputSettings\" name=\"usrtitle[]\" type=\"text\" value=\"$usrtitle\"></input></td>\n";
      echo "            <td> $unit </td>\n";

      if ($type != "UD")
         echo "            <td> <input type=\"checkbox\" name=\"selected[]\" value=\"$address:$type\"$checked></input></td>\n";
      else
         echo "            <td>[x]</td>\n";
      echo "            <td><input type=\"hidden\" name=\"addr[]\" value=\"$address:$type\"></input> $address / $type </td>\n";
      echo "          </tr>\n";

      $i++;
   }

   echo "        </table>\n";
}

?>
