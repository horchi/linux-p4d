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

   $result = $mysqli->query("select e.*, timediff(ifnull(time2,unix_timestamp()), time1) as duration from errors e order by time1 desc")
      or die("<br/>Error" . $mysqli->error);

   $num = $result->num_rows;

   echo "      <div class=\"rounded-border tableMultiCol\">\n";

   echo "         <div>\n";
   echo "           <span style=\"width:10px;\"></span>\n";
   echo "           <span>Zeit <small>(dauer)</small></span>\n";
   echo "           <span>Fehler</span>\n";
   echo "           <span>Status</span>\n";
   echo "         </div>\n";

   while ($i < $num)
   {
      $time1   = mysqli_result($result, $i, "time1");
      $time4   = mysqli_result($result, $i, "time4");
      $time2   = mysqli_result($result, $i, "time2");
      $errornr = mysqli_result($result, $i, "number");
      $state   = mysqli_result($result, $i, "state");
      $errText = mysqli_result($result, $i, "text");
      $duration = mysqli_result($result, $i, "duration");
      $time = max(max($time1, $time4), $time2);

      $style = $state == "quittiert" ? "greenCircle" : "redCircle";

      echo "         <div>\n";
      echo "           <span style=\"width:10px;\"><div class=\"$style\"></div></span>\n";
      echo "           <span>$time <small>($duration)</small></span>\n";
      echo "           <span>$errText</span>\n";
      echo "           <span>$state</span>\n";
      echo "         </div>\n";

      $i++;
   }

   echo "      </div>\n";  // tableMultiCol


include("footer.php");

?>
