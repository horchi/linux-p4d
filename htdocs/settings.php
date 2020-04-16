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

if ($action == "init")
{
   requestAction("initvaluefacts", 20, 0, "", $resonse);
   echo "<br/><div class=\"info\"><b><center>Initialisierung abgeschlossen</center></b></div><br/><br/>";
}

else if ($action == "store")
{
    $sAddr = count($_POST["addr"]);
    $sMax = count($_POST["maxscale"]);
    // echo "<br/><br/><div>$sAddr / $sMax</div>";

    foreach ($_POST['addr'] as $key => $value)
    {
        $value = htmlspecialchars($value);
        list($addr, $type) = explode(":", $value);
        $addr = $mysqli->real_escape_string($addr);
        $type = $mysqli->real_escape_string($type);

        $usrtitle = htmlspecialchars($_POST["usrtitle"][$key]);
        $state = in_array($value, $_POST["selected"]) || $type == 'UD' ? 'A' : 'D';
        $maxscale = intval(htmlspecialchars($_POST["maxscale"][$key]));

        // echo "<div>$key / $value / $addr / $type / $usrtitle / $maxscale / $state</div>";

        $sql = "UPDATE valuefacts set usrtitle = '$usrtitle', maxscale = '$maxscale', state = '$state' where address = '$addr' and type = '$type'";

        $mysqli->query($sql)
            or die("<br/>Error" . $mysqli->error);
    }

    // requestAction("update-schemacfg", 2, 0, "", $resonse);  // ist das noch nötig?
    echo "<div class=\"info\"><b><center>Einstellungen gespeichert</center></b></div>";
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
   echo "        <div class=\"menu\">\n";
   echo "          <button class=\"rounded-border button3\" type=submit name=action value=init onclick=\"return confirmSubmit('Stammdaten der Messwerte initialisieren/aktualisieren')\">Init</button>\n";
   echo "          <button class=\"rounded-border button3\" type=submit name=action value=store>Speichern</button>\n";
   echo "        </div>\n";
}

//***************************************************************************
// Show Table
//***************************************************************************

function showTable($type, $tableTitle)
{
   global $mysqli;

   seperator($tableTitle, 0);

   echo "        <table class=\"tableMultiCol\">\n";
   echo "          <tbody>\n";
   echo "            <tr>\n";
   echo "              <td>Name</td>\n";
   echo "              <td style=\"width:32%;\">Bezeichnung</td>\n";
   if ($type == "VA" || $type == "W1")
       echo "              <td style=\"width:6%;\">Skala max</td>\n";
   else
       echo "              <td style=\"width:6%;\"></td>\n";
   echo "              <td style=\"width:3%;\">Einheit</td>\n";
   echo "              <td style=\"width:3%;\">Aufzeichnen</td>\n";
   echo "              <td style=\"width:6%;\">ID:Typ</td>\n";
   echo "            </tr>\n";

   $result = $mysqli->query("select * from valuefacts where type = '$type'")
      or die("<br/>Error" . $mysqli->error);

   for ($i = 0; $i < $result->num_rows; $i++)
   {
      $address  = mysqli_result($result, $i, "address");
      $title    = mysqli_result($result, $i, "title");
      $unit     = mysqli_result($result, $i, "unit");
      $state    = mysqli_result($result, $i, "state");
      $usrtitle = mysqli_result($result, $i, "usrtitle");
      $maxscale = mysqli_result($result, $i, "maxscale");
      $txtaddr  = sprintf("0x%04x", $address);
      $checked  = ($state == "A") ? " checked" : "";

      echo "            <tr>\n";
      echo "              <td>$title</td>\n";
      echo "              <td class=\"tableMultiColCell\"><input class=\"rounded-border inputSetting\" name=\"usrtitle[]\" type=\"text\" value=\"$usrtitle\"/></td>\n";

      if (($type == "VA" || $type == "W1") && ($unit == '°' || $unit == '%'))
      {
          echo "              <td class=\"tableMultiColCell\"><input class=\"rounded-border inputSetting\" name=\"maxscale[]\" type=\"number\" value=\"$maxscale\"/></td>\n";
      }
      else
      {
          echo "              <td class=\"tableMultiColCell\" style=\"display:none;\"><input class=\"rounded-border inputSetting\" name=\"maxscale[]\" type=\"number\" value=\"$maxscale\"/></td>\n";
          echo "              <td></td>\n";
      }

      // das geht nicht (mit 'disabled' fehlt es im array!):
      // echo "              <td><input disabled=\"disabled\" style=\"visibility:hidden;\" class=\"rounded-border inputSetting\" name=\"maxscale[]\" type=\"number\" value=\"$maxscale\"/></td>\n";

      echo "              <td style=\"text-align:center;\">$unit</td>\n";

      if ($type != "UD")
          echo "              <td><input class=\"rounded-border inputSetting\" type=\"checkbox\" name=\"selected[]\" value=\"$address:$type\"$checked /></td>\n";
      else
          echo "              <td><input class=\"rounded-border inputSetting\" type=\"checkbox\" name=\"selected[]\" value=\"$address:$type\"$checked disabled=\"disabled\"/></td>\n";

      $hexaddr = dechex($address);
      echo "              <td><input type=\"hidden\" name=\"addr[]\" value=\"$address:$type\"/> 0x$hexaddr:$type </td>\n";
      echo "            </tr>\n";
   }

   echo "          </tbody>\n";
   echo "        </table>\n";
}

?>
