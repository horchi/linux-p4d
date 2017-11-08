<?php

include("header.php");

printHeader();

global $mysqli;

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

// -----------------------
//

if ($action == "sync")
{
   requestAction("updatehm", 20, 0, "", $resonse);
   echo "<br/><div class=\"info\"><b><center>Sync abgeschlossen</center></b></div><br/><br/>";
}

else if ($action == "store")
{
   foreach ($_POST['address'] as $key => $value)
   {
      $value = htmlspecialchars($value);
      $id = htmlspecialchars($_POST["id"][$key]);
      $id = $mysqli->real_escape_string($id);

      if ($value != 0)
         $sql = "update hmsysvars set address = '$value' where id = '$id'";
      else
         $sql = "update hmsysvars set address = null where id = '$id'";

      $mysqli->query($sql)
         or die("<br/>Error" . $mysqli->error);
   }

   foreach ($_POST['atype'] as $key => $value)
   {
      $value = htmlspecialchars($value);
      $id = htmlspecialchars($_POST["id"][$key]);
      $id = $mysqli->real_escape_string($id);

      $sql = "update hmsysvars set atype = '$value' where id = '$id'";
      $mysqli->query($sql)
         or die("<br/>Error" . $mysqli->error);
   }

   echo "<div class=\"info\"><b><center>Einstellungen gespeichert</center></b></div>";
}

echo "      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n";
showButtons();
showTable("Systemvariablen");
echo "      </form>\n";

$mysqli->close();

include("footer.php");

//***************************************************************************
// Show Buttons
//***************************************************************************

function showButtons()
{
   echo "        <div class=\"menu\">\n";
   echo "          <button class=\"rounded-border button3\" type=\"submit\" name=\"action\" value=\"sync\">Import/Sync</button>\n";
   echo "          <button class=\"rounded-border button3\" type=\"submit\" name=\"action\" value=\"store\" onclick=\"return confirmSubmit('Einstellungen speichern?')\">Speichern</button>\n";
   echo "        </div>\n";
}

//***************************************************************************
// Show Table
//***************************************************************************

function showTable($tableTitle)
{
   global $mysqli;

   seperator($tableTitle, 0);

   echo "        <div class=\"rounded tableMultiCol\">\n";
   echo "          <div>\n";
   echo "            <span style=\"width:5%;\"><strong>HM</strong></span>\n";
   echo "            <span style=\"width:30%;\"><strong>Name</strong></span>\n";
   echo "            <span style=\"width:10%;\"><strong>Value</strong></span>\n";
   echo "            <span style=\"width:15%;\"><strong>Aktualisiert</strong></span>\n";
   echo "            <span style=\"width:10%;\"><strong>ID</strong></span>\n";
   echo "            <span style=\"width:10%;\"><strong>Typ</strong></span>\n";
   echo "          </div>\n";

   $result = $mysqli->query("select * from hmsysvars where visible = 1")
      or die("<br/>Error" . $mysqli->error);

   for ($i = 0; $i < $result->num_rows; $i++)
   {
      $id = mysqli_result($result, $i, "id");
      $name = mysqli_result($result, $i, "name");
      $unit = mysqli_result($result, $i, "unit");
      $type = mysqli_result($result, $i, "type");
      $visible = mysqli_result($result, $i, "visible");
      $time = mysqli_result($result, $i, "time");
      $value = mysqli_result($result, $i, "value");
      $address = mysqli_result($result, $i, "address");
      $atype = mysqli_result($result, $i, "atype");

      if ($type == 4 && $value != 0)
         $value = number_format(round($value, 1), 2);

      echo "          <div>\n";
      echo "            <span style=\"width:5%;\"><input type=\"hidden\" name=\"id[]\" value=\"$id\"/> $id </span>\n";
      echo "            <span style=\"width:30%;\">$name</span>\n";
      echo "            <span style=\"width:10%;\">$value$unit</span>\n";
      echo "            <span style=\"width:15%;\">$time</span>\n";
      echo "            <span style=\"width:10%;\"><input class=\"rounded-border input\" name=\"address[]\" type=\"text\" value=\"$address\"/></span>\n";
      echo "            <span style=\"width:10%;\"><input class=\"rounded-border input\" name=\"atype[]\" type=\"text\" value=\"$atype\"/></span>\n";
      echo "          </div>\n";
   }

   echo "        </div>\n";
}

?>
