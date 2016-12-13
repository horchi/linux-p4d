<?php

include("header.php");

printHeader();

// -------------------------
// establish db connection

$mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb);
$mysqli->query("set names 'utf8'");
$mysqli->query("SET lc_time_names = 'de_DE'");


showTable("Fehlerspeicher");

//***************************************************************************
// Show Table
//***************************************************************************

function showTable($tableTitle)
{
   global $mysqli;

   $i = 0;

   echo "      <br/>\n";
   echo "      <table class=\"tableTitle\" cellspacing=0 rules=rows>\n";
   echo "        <tr>\n";
   echo "          <td><center><b>$tableTitle</b></center></td>\n";
   echo "        </tr>\n";
   echo "      </table>\n";
   echo "      <br/>\n";

   echo "      <table  class=\"tableLight\" border=1 cellspacing=0 rules=rows>
        <tr class=\"tableHead1\">
          <td> Zeit </td>
          <td> Nr </td>
          <td> Status </td>
          <td> Fehler </td>
        </tr>\n";

   $result = $mysqli->query("select * from errors order by time desc")
      or die("<br/>Error" . $mysqli->error);

   $num = $result->num_rows;

   while ($i < $num)
   {
      $time    = mysqli_result($result, $i, "time");
      $nr      = mysqli_result($result, $i, "number");
      $state   = mysqli_result($result, $i, "state");
      $text    = mysqli_result($result, $i, "text");

      if ($i % 2)
         echo "         <tr class=\"tableLight\">";
      else
         echo "         <tr class=\"tableDark\">";

      echo "
           <td> $time </td>
           <td> $nr </td>
           <td> $state </td>
           <td> $text </td>\n";
      echo "        </tr>\n";

      $i++;
   }

   echo "      </table>\n";
}

include("footer.php");

?>
