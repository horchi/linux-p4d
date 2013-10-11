<?php

include("pChart/class/pData.class.php");
include("pChart/class/pDraw.class.php");
include("pChart/class/pImage.class.php");
include("functions.php");

?>

<html>
    <head>
      <meta http-equiv="content-type" content="text/html; charset=UTF-8">
      <meta name="author" content="Jörg Wendel">
      <meta name="copyright" content="Jörg Wendel">
      <title>Fröhling P4</title>
    </head>

    <br>

<?php

  mysql_connect("hgate", "p4", "p4");
  mysql_select_db("p4");
  mysql_query("set names 'utf8'");
  mysql_query("SET lc_time_names = 'de_DE'");

  // get last time stamp

  $strQuery = "select max(time), DATE_FORMAT(max(time),'%d. %M %Y   %H:%m') as maxPretty from samples;";
  $result = mysql_query($strQuery);
  $row = mysql_fetch_array($result, MYSQL_ASSOC);
  $max = $row['max(time)'];
  $maxPretty = $row['maxPretty'];

  $day   = isset($_GET['sday'])   ? $_GET['sday']   : (int)date("d");
  $month = isset($_GET['smonth']) ? $_GET['smonth'] : (int)date("m");
  $year  = isset($_GET['syear'])  ? $_GET['syear']  : (int)date("Y");
  $range = isset($_GET['range'])  ? $_GET['range']  : 1;

  // syslog(LOG_DEBUG, "p4: m $month");

  echo "<form name='navigation' method='get'>\n";
  echo datePicker("Start", "s", $year, $day, $month);

  echo "Bereich: ";
  echo "   <select name=\"range\">\n";
  echo "      <option value='1' " . ($range == 1 ? "SELECTED" : "") . ">Tag</option>\n";
  echo "      <option value='7' " . ($range == 7 ? "SELECTED" : "") . ">Woche</option>\n";
  echo "      <option value='31' " . ($range == 31 ? "SELECTED" : "") . ">Monat</option>\n";
  echo "   </select>\n";

  echo "   <input type=submit value=\"Go\">";

  echo "</form>\n";
  echo "<br>\n";

  $from = date_create_from_format('!Y-m-d', $year.'-'.$month.'-'.$day)->getTimestamp();

  echo "<img src='detail.php?width=1000&height=500&from=" . $from . "&range=" . $range . "'>\n";
  echo "<br><br>\n";

  echo "  <table width=\"70%\" border=0 cellspacing=0 rules=none>\n";
  echo "    <tr style=\"color:white\" bgcolor=\"#000099\"><td/><td><center>" . $maxPretty . "</center><td/><td/><td/></tr>\n";

?>

</tr>
   <tr style="color:white" bgcolor="#000099">
     <td>Id</td>
     <td>Sensor</td>
     <td>Wert</td>
     <td>Unit</td>
   </tr>

<?php

  $strQuery = sprintf("select s.sensorid as s_sensorid, s.time as s_time, s.value as s_value, f.title as f_title, f.unit as f_unit 
              from samples s, sensorfacts f where f.sensorid = s.sensorid and s.time = '%s';", $max);

  $result = mysql_query($strQuery);

  $i = 0;

  while ($row = mysql_fetch_array($result, MYSQL_ASSOC))
  {
     $value = $row['s_value'];
     $title = $row['f_title'];
     $unit = $row['f_unit'];
     $sensorid = $row['s_sensorid'];
     
     $url = "<a href=\"#\" onclick=\"window.open('detail.php?width=1200&height=600&sensorid=$sensorid&from=" . $from . "&range=" . $range . " ','_blank'," 
        . "'scrollbars=yes,width=1200,height=600,resizable=yes,left=120,top=120')\">";
     
     if ($i++ % 2)
        echo "   <tr bgcolor=\"#83AFFF\">\n";
     
     echo "      <td>" . $sensorid . "</td>\n";   
     echo "      <td>" . $url . $title . "</a></td>\n";
     echo "      <td>$value</td>\n";
     echo "      <td>$unit</td>\n";
     echo "   </tr>\n";
  }

mysql_close();

echo "  </table><br>\n";
echo "</html>\n";
?>
