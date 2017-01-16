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

   $result = $mysqli->query("select * from errors order by time1 desc")
      or die("<br/>Error" . $mysqli->error);

   $num = $result->num_rows;

   echo "      <div class=\"rounded-border tableMultiCol\">\n";

   while ($i < $num)
   {
      $time1   = mysqli_result($result, $i, "time1");
      $time4   = mysqli_result($result, $i, "time4");
      $time2   = mysqli_result($result, $i, "time2");
      $errornr = mysqli_result($result, $i, "number");
      $state   = mysqli_result($result, $i, "state");
      $errText = mysqli_result($result, $i, "text");

      $time = max(max($time1, $time4), $time2);

      echo "         <div>\n";
      echo "           <span>$time</span>\n";
      echo "           <span>$errText</span>\n";
      echo "           <span>$state</span>\n";
      echo "         </div>\n";

      $i++;
   }

   echo "      </div>\n";  // tableMultiCol


include("footer.php");

?>
