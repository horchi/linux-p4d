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

  $mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb, $mysqlport);

  if (mysqli_connect_error())
  {
      die('Connect Error (' . mysqli_connect_errno() . ') '
            . mysqli_connect_error() . ". Can't connect to " . $mysqlhost . ' at ' . $mysqlport);
  }

  $mysqli->query("set names 'utf8'");
  $mysqli->query("SET lc_time_names = 'de_DE'");

  // -------------------------
  // get last time stamp

  $result = $mysqli->query("select max(time), DATE_FORMAT(max(time),'%d. %M %Y   %H:%i') as maxPretty, " .
                        "DATE_FORMAT(max(time),'%H:%i:%S') as maxPrettyShort from samples;")
     or die("Error" . $mysqli->error);
  $row = $result->fetch_assoc();
  $max = $row['max(time)'];
  $maxPretty = $row['maxPretty'];
  $maxPrettyShort = $row['maxPrettyShort'];

  // ----------------
  // init

  $sday   = isset($_GET['sday'])   ? $_GET['sday']    : (int)date("d",time()-86400*$_SESSION['chartStart']);
  $smonth = isset($_GET['smonth']) ? $_GET['smonth']  : (int)date("m",time()-86400*$_SESSION['chartStart']);
  $syear  = isset($_GET['syear'])  ? $_GET['syear']   : (int)date("Y",time()-86400*$_SESSION['chartStart']);
  $srange = isset($_GET['srange'])  ? $_GET['srange'] : $_SESSION['chartStart'];

  $range = ($srange > 7) ? 31 : (($srange > 1) ? 7 : 1);
  $from = date_create_from_format('!Y-m-d', $syear.'-'.$smonth.'-'.$sday)->getTimestamp();

  // ------------------
  // Script Buttons

  $action = "";

  if (isset($_POST["action"]))
     $action = htmlspecialchars($_POST["action"]);

  if ($action == "clearpeaks")
  {
      $mysqli->query("truncate table peaks")
          or die("<br/>Error" . $mysqli->error);

      echo "      <br/><div class=\"info\"><b><center>Peaks cleared</center></b></div><br/>\n";
  }
  else if (substr($action, 0, 6) == "script")
  {
     $script = substr($action, 7);

     if (requestAction("call-script", 5, 0, "$script", $resonse) != 0)
        echo "      <div class=\"infoError\"><b><center>Calling Skript failed '$resonse' - p4d/syslog log for further details</center></div></br>\n";
  }

  if (haveLogin())
  {
    $result = $mysqli->query("select * from scripts where visible = 'Y'")
       or die("Error" . $mysqli->error);

    $count = $result->num_rows;

    if ($count > 0)
    {
       $i = 0;

       echo "      <form action=\"" . htmlspecialchars($_SERVER["PHP_SELF"]) . "\" method=\"post\">\n";
       echo "        <div class=\"menu\">\n";

       while ($i < $count)
       {
          $name = mysqli_result($result, $i, "name");
          echo "          <button class=\"rounded-border button2\" type=\"submit\" name=\"action\" value=\"script-$name\"" . $name . "\">$name</button>\n";
          $i++;
       }

       echo "        </div>\n";
       echo "      </form>\n";
    }
  }

  // ------------------
  // State of P4 Daemon

  $p4dstate = requestAction("p4d-state", 3, 0, "", $response);
  $load = "";

  if ($p4dstate == 0)
    list($p4dNext, $p4dVersion, $p4dSince, $load) = explode("#", $response, 4);

  $result = $mysqli->query("select * from samples where time >= CURDATE()")
     or die("Error" . $mysqli->error);
  $p4dCountDay = $result->num_rows;

  // ------------------
  // State of S 3200

  $status = "";
  $mode = "";
  $time = "";

  $state = requestAction("s3200-state", 3, 0, "", $response);

  if ($state == 0)
     list($time, $state, $status, $mode) = explode("#", $response, 4);

  $time = str_replace($wd_value, $wd_disp, $time);
  list($day, $time) = explode(",", $time, 2);
  $heatingType = $_SESSION['heatingType'];
  $stateImg = getStateImage($state, $p4dstate);

   if ($state == 19)
      $stateStyle = "aStateOk";
   elseif ($state == 0)
      $stateStyle = "aStateFail";
   elseif ($state == 3)
      $stateStyle = "aStateHeating";
   else
      $stateStyle = "aStateOther";

  // -----------------
  // State 'flex' Box

  echo "      <div class=\"stateInfo\">\n";

  // -----------------
  // Heating State
  {
      echo "        <div class=\"rounded-border heatingState\">\n";
      echo "          <div><span id=\"" . $stateStyle . "\">$status</span></div>\n";
      echo "          <div><span>" . $day . "</span><span>" . $time . "</span></div>\n";
      echo "          <div><span>Betriebsmodus:</span><span>" . $mode ."</span></div>\n";
      echo "        </div>\n";
  }

  // -----------------
  // State Image
  {
      echo "        <div class=\"imageState\">\n";
      echo "          <a href=\"\" onclick=\"javascript:showHide('divP4dState'); return false\">\n";
      echo "            <img class=\"centerImage\" src=\"$stateImg\">\n";
      echo "          </a>\n";
      echo "        </div>\n";
  }

  // -----------------
  // p4d State
  {
     echo "        <div class=\"rounded-border P4dInfo\" id=\"divP4dState\">\n";

     if ($p4dstate == 0)
     {
        echo  "              <div id=\"aStateOk\"><span>$heatingType Daemon ONLINE</span>   </div>\n";
        echo  "              <div><span>Läuft seit:</span>            <span>$p4dSince</span>       </div>\n";
        echo  "              <div><span>Messungen heute:</span>       <span>$p4dCountDay</span>    </div>\n";
        echo  "              <div><span>Letzte Messung:</span>        <span>$maxPrettyShort</span> </div>\n";
        echo  "              <div><span>Nächste Messung:</span>       <span>$p4dNext</span>        </div>\n";
        echo  "              <div><span>Version (p4d / webif):</span> <span>$p4dVersion / $p4WebVersion</span></div>\n";
        echo  "              <div><span>CPU-Last:</span>              <span>$load</span>           </div>\n";
     }
     else
     {
        echo  "          <div id=\"aStateFail\">ACHTUNG:<br/>$heatingType Daemon OFFLINE</div>\n";
     }

     echo "        </div>\n"; // P4dInfo
  }

  echo "      </div>\n";   // stateInfo

  // ------------------
  // Sensor List
  {
     buildIdList(!isMobile() ? $_SESSION['addrsMain'] : $_SESSION['addrsMainMobile'], $sensors, $addrWhere, $ids);

     // select s.address as s_address, s.type as s_type, s.time as s_time, f.title from samples s left join peaks p on p.address = s.address and p.type = s.type join valuefacts f on f.address = s.address and f.type = s.type where f.state = 'A' and s.time = '2020-03-27 12:19:00'

     $strQueryBase = sprintf("select p.minv as p_min, p.maxv as p_max, s.address as s_address, s.type as s_type,
                             s.time as s_time, s.value as s_value, s.text as s_text,
                             f.usrtitle as f_usrtitle, f.title as f_title, f.unit as f_unit
                             from samples s left join peaks p on p.address = s.address and p.type = s.type
                             join valuefacts f on f.address = s.address and f.type = s.type");

     if ($addrWhere == "")
         $strQuery = sprintf("%s where s.time = '%s'", $strQueryBase, $max);
     else
         $strQuery = sprintf("%s where (%s) and s.time = '%s'", $strQueryBase, $addrWhere, $max);

     // syslog(LOG_DEBUG, "p4: selecting " . " '" . $strQuery . "'");

     $result = $mysqli->query($strQuery)
         or die("Error" . $mysqli->error);

     echo "      <div class=\"rounded-border table2Col\">\n";
     echo "        <center>Messwerte vom $maxPretty</center>\n";

     $map = new \Ds\Map();
     $i = 0;

     while ($row = $result->fetch_assoc())
     {
         $id = "0x".dechex($row['s_address']).":".$row['s_type'];
         $map->put($id, $row);

         if ($addrWhere == "")
             $ids[$i++] = $id;
     }

     for ($i = 0; $i < count($ids); $i++)
     {
         try
         {
             $row = $map[$ids[$i]];
         }
         catch (Exception $e)
         {
             echo "ID '".$ids[$i]."' not found, ignoring!";
         }

         $value = $row['s_value'];
         $text = $row['s_text'];
         $title = (preg_replace("/($pumpDir)/i","",$row['f_usrtitle']) != "") ? preg_replace("/($pumpDir)/i","",$row['f_usrtitle']) : $row['f_title'];
         $unit = prettyUnit($row['f_unit']);
         $address = $row['s_address'];
         $type = $row['s_type'];

         $peak = "";
         $min = $row['p_min'];
         $max = $row['p_max'];

         $txtaddr = sprintf("0x%x", $address);

         if ($type != 'DI' && $type != 'DO' && $unit != '' && $unit != 'h' && $unit != 'Stunden')
             $peak = sprintf("(%s/%s)", $min, $max);

         if ($type == 'DI' || $type == 'DO')
             $value = $value == "1.00" ? "an" : "aus";
         else if ($row['f_unit'] == 'h' || $row['f_unit'] == 'U')
             $value = round($value, 0);

         if ($row['f_unit'] == 'T')
             $value = str_replace($wd_value, $wd_disp, $text);
         else if ($row['f_unit'] == 'zst')
             $value = $text;

         $url = "<a class=\"tableButton\" href=\"#\" onclick=\"window.open('detail.php?width=1200&height=600&address=$address&type=$type&from="
             . $from . "&range=" . $srange . "&chartXLines=" . $_SESSION['chartXLines'] . "&chartDiv="
             . $_SESSION['chartDiv'] . " ','_blank',"
             . "'scrollbars=yes,width=1200,height=600,resizable=yes,left=120,top=120')\">";

         echo "         <div class=\"mainrow\">\n";
         echo "           <span>$url $title</a></span>\n";
         echo "           <span>$value&nbsp;$unit &nbsp; <p style = \"display:inline;font-size:12px;font-style:italic;\">$peak</p></span>\n";
         echo "         </div>\n";
     }

     echo "      </div>\n";  // table2Col
  }

  {
     echo "      <div class=\"rounded-border\" id=\"aSelect\">\n";
     echo "        <form name='navigation' method='get'>\n";

     // ----------------
     // Date Picker

     echo "          Zeitraum der Charts<br/>\n";
     echo datePicker("", "s", $syear, $sday, $smonth);

     echo "          <select name=\"srange\">\n";
     echo "            <option value='1' "  . ($srange == 1  ? "SELECTED" : "") . ">Tag</option>\n";
     echo "            <option value='7' "  . ($srange == 7  ? "SELECTED" : "") . ">Woche</option>\n";
     echo "            <option value='31' " . ($srange == 31 ? "SELECTED" : "") . ">Monat</option>\n";
     echo "          </select>\n";
     echo "          <input type=submit value=\"Go\">";
     echo "        </form>\n";

     // -------

     echo "        <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n";
     echo "          <br/>\n";
     echo "          <button class=\"rounded-border button3\" type=submit name=action value=clearpeaks>Peaks löschen</button>\n";
     echo "        </form>\n";
     echo "      </div>\n";
  }

  $mysqli->close();

include("footer.php");
?>
