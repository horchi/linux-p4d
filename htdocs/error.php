<?php

include("header.php");

// -------------------------
// establish db connection

mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
mysql_select_db($mysqldb);
mysql_query("set names 'utf8'");
mysql_query("SET lc_time_names = 'de_DE'");


showTable("Fehlerspeicher");

//***************************************************************************
// Show Table
//***************************************************************************

function showTable($tableTitle)
{
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

   $result = mysql_query("select * from errors order by time desc")
      or die("<br/>Error" . mysql_error());

   $num = mysql_numrows($result);
   
   while ($i < $num) 
   {
      $time    = mysql_result($result, $i, "time");
      $nr      = mysql_result($result, $i, "number");
      $state   = mysql_result($result, $i, "state");
      $text    = mysql_result($result, $i, "text");

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
