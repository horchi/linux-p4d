<?php

include("header.php");

printHeader();

// -------------------------
// establish db connection

$mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb, $mysqlport);
$mysqli->query("set names 'utf8'");
$mysqli->query("SET lc_time_names = 'de_DE'");


//***************************************************************************
// Show Table
//***************************************************************************

   $i = 0;

   seperator("Fehlerspeicher", 0);

   $result = $mysqli->query("select * from errors order by time desc")
      or die("<br/>Error" . $mysqli->error);

   $num = $result->num_rows;

   echo "      <div class=\"rounded-border tableMainMM\">\n";
   echo "        <center>Messwerte vom $maxPretty</center>\n";

   while ($i < $num)
   {
      $time    = mysqli_result($result, $i, "time");
      $nr      = mysqli_result($result, $i, "number");
      $state   = mysqli_result($result, $i, "state");
      $text    = mysqli_result($result, $i, "text");

      echo "         <div>\n";
      echo "           <span>$time</span>\n";
      echo "           <span>$nr</span>\n";
      echo "           <span>$state</span>\n";
      echo "           <span>$text</span>\n";
      echo "         </div>\n";

      $i++;
   }

   echo "      </div>\n";  // tableMainMM


include("footer.php");

?>
