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
   foreach ($_POST['addr'] as $key => $value)
   {
      $sql = "";
      $id = $mysqli->real_escape_string(htmlspecialchars($value));
      $name = htmlspecialchars($_POST["name"][$key]);

      // echo "<div>$key / $id / $name</div>";

      if ($id == "add" && $name != "")
         $sql = "INSERT into groups set name = '$name'";
      else if ($id != "add" && $name != "")
         $sql = "UPDATE groups set name = '$name' where id = '$id'";
      else if ($id != "add" && $name == "")
         $sql = "DELETE from groups where id = '$id'";

      if ($sql != "")
         $mysqli->query($sql) or die("<br/>Error" . $mysqli->error);
   }

   echo "<div class=\"info\"><b><center>Einstellungen gespeichert</center></b></div>";
}

echo "      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n";
showButtons();
showGroups("Baugruppen");
echo "      </form>\n";

$mysqli->close();

include("footer.php");

//***************************************************************************
// Show Buttons
//***************************************************************************

function showButtons()
{
   echo "        <div class=\"menu\">\n";
   echo "          <button class=\"rounded-border button3\" type=submit name=action value=store>Speichern</button>\n";
   echo "        </div>\n";
}

//***************************************************************************
// Show Table
//***************************************************************************

function showGroups($tableTitle)
{
   global $mysqli;

   echo "        <br/>\n";
   seperator($tableTitle, 0);

   echo "        <table class=\"tableMultiCol\">\n";
   echo "          <tbody>\n";
   echo "            <tr>\n";
   echo "              <td>ID</td>\n";
   echo "              <td>Name</td>\n";
   echo "            </tr>\n";

   $result = $mysqli->query("select * from groups")
      or die("<br/>Error" . $mysqli->error);

   for ($i = 0; $i < $result->num_rows; $i++)
   {
      $name    = mysqli_result($result, $i, "name");
      $id    = mysqli_result($result, $i, "id");

      echo "            <tr>\n";
      echo "              <td><input type=\"hidden\" name=\"addr[]\" value=\"$id\"/>$id</td>\n";
      echo "              <td class=\"tableMultiColCell\"><input class=\"rounded-border inputSetting\" name=\"name[]\" type=\"text\" value=\"$name\"/></td>\n";
      echo "            </tr>\n";
   }

   echo "            <tr>\n";
   echo "              <td><input type=\"hidden\" name=\"addr[]\" value=\"add\"/></td>\n";
   echo "              <td class=\"tableMultiColCell\"><input class=\"rounded-border inputSetting\" name=\"name[]\" type=\"text\" value=\"\"/></td>\n";
   echo "            </tr>\n";

   echo "          </tbody>\n";
   echo "        </table>\n";
}

?>
