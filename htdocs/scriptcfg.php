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

if ($action == "store")
{
   foreach ($_POST['name'] as $key => $value)
   {
      $value = $mysqli->real_escape_string(htmlspecialchars($value));
      $id = $mysqli->real_escape_string(htmlspecialchars($_POST["id"][$key]));

      $mysqli->query("update scripts set name = '$value' where id = '$id'")
         or die("<br/>Error" . $mysqli->error);
   }

   $mysqli->query("update scripts set visible = 'N'")
      or die("<br/>Error" . $mysqli->error);

   if (isset($_POST["visible"]))
   {
      foreach ($_POST['visible'] as $key => $value)
      {
         $id = $mysqli->real_escape_string(htmlspecialchars($_POST["id"][$key]));

         $mysqli->query("update scripts set visible = 'Y' where id = '$id'")
            or die("<br/>Error" . $mysqli->error);
      }
   }

   echo "<div class=\"info\"><b><center>Einstellungen gespeichert</center></b></div>";
}

echo "      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n";
showButtons();
showTable("Skript Interface");
echo "      </form>\n";

$mysqli->close();

include("footer.php");

//***************************************************************************
// Show Buttons
//***************************************************************************

function showButtons()
{
   echo "        <div class=\"menu\">\n";
   echo "          <button class=\"rounded-border button3\" type=\"submit\" name=\"action\" value=\"store\">Speichern</button>\n";
   echo "        </div>\n";
}

//***************************************************************************
// Show Table
//***************************************************************************

function showTable($tableTitle)
{
   global $mysqli;

   seperator($tableTitle, 0);

   echo "        <div class=\"rounded-border tableMultiCol\">\n";
   echo "          <div>\n";
   echo "            <span style=\"width:5%;\"><strong>Id</strong></span>\n";
   echo "            <span><strong>Name</strong></span>\n";
   echo "            <span><strong>Skript</strong></span>\n";
   echo "            <span style=\"width:8%;\"><strong>Sichtbar</strong></span>\n";
   echo "          </div>\n";

   $result = $mysqli->query("select * from scripts")
      or die("<br/>Error" . $mysqli->error);

   for ($i = 0; $i < $result->num_rows; $i++)
   {
      $id = mysqli_result($result, $i, "id");
      $name = mysqli_result($result, $i, "name");
      $script = mysqli_result($result, $i, "path");
      $visible = mysqli_result($result, $i, "visible");
      $checked = ($visible == "Y") ? "checked" : "";

      echo "          <div>\n";
      echo "            <span style=\"width:5%;\"><input type=\"hidden\" name=\"id[]\" value=\"$id\"/>$id</span>\n";
      echo "            <span><input type=\"text\" class=\"rounded-border input\" name=\"name[]\" value=\"$name\"/></span>\n";
      echo "            <span>$script</span>\n";
      echo "            <span style=\"width:8%;\"><input type=\"checkbox\" class=\"rounded-border input\" name=\"visible[]\" $checked/></span>\n";
      echo "          </div>\n";
   }

   echo "        </div>\n";
}

?>
