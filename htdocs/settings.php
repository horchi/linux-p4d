<?php

include("header.php");

$action = "";

if (isset($_POST["action"]))
   $action = htmlspecialchars($_POST["action"]);

// -------------------------
// establish db connection

mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
mysql_select_db($mysqldb) or die("DB error");
mysql_query("set names 'utf8'");
mysql_query("SET lc_time_names = 'de_DE'");

// -----------------------
//

if ($action == "init")
{
   requestAction("initvaluefacts", 20, $result);
   echo "<br/><div class=\"info\"><b><center>Initialisierung abgeschlossen</center></b></div><br/><br/>";
}

else if ($action == "store")
{
   if (isset($_POST["selected"]))
   {
      $sql = "UPDATE valuefacts SET state = 'D'";
      $result = mysql_query($sql) 
         or die("Error" . mysql_error());
      
      foreach ($_POST['selected'] as $key => $value) 
      {
         $value = htmlspecialchars($value);
         list ($addr, $type) = split(":", $value);
         
         $sql = "UPDATE valuefacts set state = 'A' where address = '$addr' and type = '$type'";

         $result = mysql_query($sql) 
            or die("Error" . mysql_error());
      }

      echo "<br/><div class=\"info\"><b><center>Einstellungen gespeichert</center></b></div><br/><br/>";
   }
}

echo "      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>";
showButtons();
showTable();
echo "      </form>\n";

mysql_close();

include("footer.php");

//***************************************************************************
// Show Buttons
//***************************************************************************

function showButtons()
{
   echo "      <div>\n";
   echo "          <button class=\"button3\" type=submit name=action value=init onclick=\"return confirmSubmit('Stammdaten der Messwerte initialisieren')\">Init</button>\n";
   echo "          <button class=\"button3\" type=submit name=action value=store onclick=\"return confirmSubmit('Einstellungen speichern?')\">Speichern</button>\n";
   echo "          <br/><br/>\n";
   echo "      </div>\n";
}

//***************************************************************************
// Show Table
//***************************************************************************

function showTable()
{
   $i = 0;

   echo "
        <table  class=\"tableLight\" border=1 cellspacing=0 rules=rows>
          <tr class=\"tableHead1\">
            <td> Adresse </td>
            <td> Typ </td>
            <td> Name </td>
            <td> Einheit </td>
            <td> Aufzeichnen </td>
          </tr>\n";

   $result = mysql_query("select * from valuefacts")
      or die("Error" . mysql_error());

   $num = mysql_numrows($result);
   
   while ($i < $num) 
   {
      $address = mysql_result($result,$i,"address");
      $type    = mysql_result($result,$i,"type");
      $title   = mysql_result($result,$i,"title");
      $unit    = mysql_result($result,$i,"unit");
      $state   = mysql_result($result,$i,"state");
      $txtaddr = sprintf("0x%04x", $address);

      if ($state == "A") 
         $checked = " checked";
      else 
         $checked = "";
      
      if ($i % 2)
         echo "         <tr class=\"tableLight\">";
      else
         echo "         <tr class=\"tableDark\">";

      echo "
            <td> $txtaddr </td>
            <td> $type </td>
            <td> $title </td>
            <td> $unit </td>
            <td> <input type=\"checkbox\" name=\"selected[]\" value=\"$address:$type\"$checked></input></td>
          </tr>\n";

      $i++;
   }

   echo "        </table>\n";
}

include("jfunctions.php");
?>
