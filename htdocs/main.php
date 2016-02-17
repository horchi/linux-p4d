<?php

//***************************************************************************
// WEB Interface of p4d / Linux - Heizungs Manager
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 07.01.2014  Jörg Wendel
//***************************************************************************

include("header.php");

printHeader(60);

  // -------------------------
  // establish db connection

  mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
  mysql_select_db($mysqldb);
  mysql_query("set names 'utf8'");
  mysql_query("SET lc_time_names = 'de_DE'");

  // -------------------------
  // get last time stamp

  $result = mysql_query("select max(time), DATE_FORMAT(max(time),'%d. %M %Y   %H:%i') as maxPretty, " .
                        "DATE_FORMAT(max(time),'%H:%i:%S') as maxPrettyShort from samples;")
     or die("Error" . mysql_error());
  $row = mysql_fetch_assoc($result);
  $max = $row['max(time)'];
  $maxPretty = $row['maxPretty'];
  $maxPrettyShort = $row['maxPrettyShort'];

  // ----------------
  // init

  $day   = isset($_GET['sday'])   ? $_GET['sday']   : (int)date("d",time()-86400*$_SESSION['chartStart']);
  $month = isset($_GET['smonth']) ? $_GET['smonth'] : (int)date("m",time()-86400*$_SESSION['chartStart']);
  $year  = isset($_GET['syear'])  ? $_GET['syear']  : (int)date("Y",time()-86400*$_SESSION['chartStart']);
  $range = isset($_GET['range'])  ? $_GET['range']  : $_SESSION['chartStart']+1;

  // ------------------
  // State of P4 Daemon

  $p4dstate = requestAction("p4d-state", 3, 0, "", $response);
  $load = "";

  if ($p4dstate == 0)
     list($p4dNext, $p4dVersion, $p4dSince, $load) = split("#", $response, 4);

  $result = mysql_query("select * from samples where time >= CURDATE()")
     or die("Error" . mysql_error());
  $p4dCountDay = mysql_numrows($result);

  // ------------------
  // State of S 3200

  $status = "";
  $mode = "";
  $time = "";

  $state = requestAction("s3200-state", 3, 0, "", $response);

  if ($state == 0)
     list($time, $state, $status, $mode) = split("#", $response, 4); 

  $time = str_replace($wd_value, $wd_disp, $time);   

  echo "      <div class=\"stateInfo\">\n";

  if ($state == 19)
     echo  "        <div id=\"aStateOk\"><center>$status</center></div>\n";
  elseif ($state == 0)
     echo  "        <div id=\"aStateFail\"><center>$status</center></div>\n";
  elseif ($state == 3)
     echo  "        <div id=\"aStateHeating\"><center>$status</center></div>\n";
  else
     echo  "        <div id=\"aStateOther\"><center>$status</center></div>\n";

  echo "        <br/>" . $time . "<br/>";
  echo "Betriebsmodus:  " . $mode ."<br/>\n";
  echo "      </div>\n";

  echo "      <div class=\"stateImgContainer\">\n";

  $heatingType = $_SESSION['heatingType'];

  if ($state == 0 || $p4dstate != 0)
     $stateImg = "img/state/state-error.gif";
  elseif ($state == 1)
     $stateImg = (isset($_SESSION['stateAni']) && file_exists("img/state/ani/state-fireoff.gif")) ? "img/state/ani/state-fireoff.gif" : "img/state/state-fireoff.gif"; 
  elseif ($state == 2)
     $stateImg = (isset($_SESSION['stateAni']) && file_exists("img/state/ani/state-heatup.gif")) ? "img/state/ani/state-heatup.gif" : "img/state/state-heatup.gif"; 
  elseif ($state == 3)
     $stateImg = (isset($_SESSION['stateAni']) && file_exists("img/state/ani/state-fire.gif")) ? "img/state/ani/state-fire.gif" : "img/state/state-fire.gif"; 
  elseif ($state == 4)
     $stateImg = (isset($_SESSION['stateAni']) && file_exists("img/state/ani/state-firehold.gif")) ? "img/state/ani/state-firehold.gif" : "img/state/state-firehold.gif"; 
  elseif ($state == 5)
     $stateImg = (isset($_SESSION['stateAni']) && file_exists("img/state/ani/state-fireoff.gif")) ? "img/state/ani/state-fireoff.gif" : "img/state/state-fireoff.gif"; 
  elseif ($state == 6)
     $stateImg = (isset($_SESSION['stateAni']) && file_exists("img/state/ani/state-dooropen.gif")) ? "img/state/ani/state-dooropen.gif" : "img/state/state-dooropen.gif"; 
  elseif ($state == 7)
     $stateImg = (isset($_SESSION['stateAni']) && file_exists("img/state/ani/state-preparation.gif")) ? "img/state/ani/state-preparation.gif" : "img/state/state-preparation.gif"; 
  elseif ($state == 8)
     $stateImg = (isset($_SESSION['stateAni']) && file_exists("img/state/ani/state-warmup.gif")) ? "img/state/ani/state-warmup.gif" : "img/state/state-warmup.gif"; 
  elseif ($state == 9)
     $stateImg = (isset($_SESSION['stateAni']) && file_exists("img/state/ani/state-heatup.gif")) ? "img/state/ani/state-heatup.gif" : "img/state/state-heatup.gif"; 
  elseif ($state == 15 || $state == 70 || $state == 69)
     $stateImg = (isset($_SESSION['stateAni']) && file_exists("img/state/ani/state-clean.gif")) ? "img/state/ani/state-clean.gif" : "img/state/state-clean.gif"; 
  elseif (($state >= 10 && $state <= 14) || $state == 35 || $state == 16)
     $stateImg = (isset($_SESSION['stateAni']) && file_exists("img/state/ani/state-wait.gif")) ? "img/state/ani/state-wait.gif" : "img/state/state-wait.gif"; 
  elseif ($state == 60 || $state == 61  || $state == 72)
     $stateImg = (isset($_SESSION['stateAni']) && file_exists("img/state/ani/state-shfire.gif")) ? "img/state/ani/state-shfire.gif" : "img/state/state-shfire.gif"; 
  else
     $stateImg = "img/type/heating-$heatingType.png";

  echo "        <img class=\"centerImage\" src=\"$stateImg\">\n";
  echo "      </div>\n";

  echo "      <div class=\"P4dInfo\">\n";

  if ($p4dstate == 0)
  {
    echo  "        <div id=\"aStateOk\"><center>Fröling $heatingType ONLINE</center></div>\n";
    echo  "          <table>\n";
    echo  "            <tr><td>Läuft seit:</td><td>$p4dSince</td></tr>\n";
    echo  "            <tr><td>Messungen heute:</td><td>$p4dCountDay</td></tr>\n";
    echo  "            <tr><td>Letzte Messung:</td><td>$maxPrettyShort</td></tr>\n";
    echo  "            <tr><td>Nächste Messung:</td><td>$p4dNext</td></tr>\n";
    echo  "            <tr><td>Version:</td><td>$p4dVersion</td></tr>\n";
    echo  "            <tr><td>CPU-Last:</td><td>$load</td></tr>\n";
    echo  "          </table>\n";
  }
  else
    echo  "        <div id=\"aStateFail\"><center>ACHTUNG:<br/>$heatingType Daemon OFFLINE</center></div>\n";

  echo "      </div>\n";
  echo "      <br/>\n";

  // ----------------
  // 

  echo "      <div id=\"aSelect\">\n";
  echo "        <form name='navigation' method='get'>\n";
  echo "          <center>Zeitraum der Charts<br/></center>\n";
  echo datePicker("Start", "s", $year, $day, $month);

  echo "          <select name=\"range\">\n";
  echo "            <option value='1' "  . ($range == 1  ? "SELECTED" : "") . ">Tag</option>\n";
  echo "            <option value='7' "  . ($range == 7  ? "SELECTED" : "") . ">Woche</option>\n";
  echo "            <option value='31' " . ($range == 31 ? "SELECTED" : "") . ">Monat</option>\n";
  echo "          </select>\n";
  echo "          <input type=submit value=\"Go\">";
  echo "        </form>\n";
  echo "      </div>\n";

  $from = date_create_from_format('!Y-m-d', $year.'-'.$month.'-'.$day)->getTimestamp();

  seperator("Messwerte vom " . $maxPretty, 290);

  // ------------------
  // table

  echo "  <div>\n";
  echo "  <table class=\"table\" cellspacing=0 rules=rows style=\"position:absolute; top:330px;\">\n";
  echo "      <tr class=\"tableHead1\">\n";


  echo "        <td>Sensor</td>\n";
  echo "        <td>Wert</td>\n";  
  echo "      </tr>\n";

  $strQuery = sprintf("select s.address as s_address, s.type as s_type, s.time as s_time, s.value as s_value, s.text as s_text, f.usrtitle as f_usrtitle, f.title as f_title, f.unit as f_unit 
              from samples s, valuefacts f where f.state = 'A' and f.address = s.address and f.type = s.type and s.time = '%s';", $max);

  $result = mysql_query($strQuery)
     or die("Error" . mysql_error());

  $i = 0;

  while ($row = mysql_fetch_assoc($result))
  {
     $value = $row['s_value'];
     $text = $row['s_text'];
     $title = (preg_replace("/($pumpDir)/i","",$row['f_usrtitle']) != "") ? preg_replace("/($pumpDir)/i","",$row['f_usrtitle']) : $row['f_title'];   
     $unit = $row['f_unit'];
     $address = $row['s_address'];
     $type = $row['s_type'];
     $txtaddr = sprintf("0x%x", $address);

     if ($unit == "zst" || $unit == "dig")
        $unit = "";
     else if ($unit == "U")
        $unit = " U/min";
     else if ($unit == "°")
        $unit = "°C";
     else if ($unit == "T")
     {
        $unit = "";
        $value = str_replace($wd_value, $wd_disp, $text);
     }

     $url = "<a href=\"#\" onclick=\"window.open('detail.php?width=1200&height=600&address=$address&type=$type&from=" . $from . "&range=" . $range . "&chartXLines=" . $_SESSION['chartXLines'] . "&chartDiv=" . $_SESSION['chartDiv'] . " ','_blank'," 
        . "'scrollbars=yes,width=1200,height=600,resizable=yes,left=120,top=120')\">";
     
     if ($i++ % 2)
        echo "     <tr class=\"tableDark\">\n";
     else
        echo "     <tr class=\"tableLight\">\n";

     echo "        <td>" . $url . $title . "</a></td>\n";
     echo "        <td>$value$unit</td>\n";
     echo "     </tr>\n";
  }

  echo "  </table>\n";
  echo "  </div>\n";

  mysql_close();

include("footer.php");
?>
