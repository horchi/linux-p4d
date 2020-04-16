<?php

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

  // ----------------
  // widgets

  {
      // select s.address as s_address, s.type as s_type, s.time as s_time, f.title from samples s left join peaks p on p.address = s.address and p.type = s.type join valuefacts f on f.address = s.address and f.type = s.type where f.state = 'A' and s.time = '2020-03-27 12:19:00'

      buildIdList($_SESSION['addrsDashboard'], $sensors, $addrWhere, $ids);

      $strQueryBase = sprintf("select p.minv as p_min, p.maxv as p_max,
                              s.address as s_address, s.type as s_type, s.time as s_time, s.value as s_value, s.text as s_text,
                              f.usrtitle as f_usrtitle, f.name as f_name, f.title as f_title, f.unit as f_unit, f.maxscale as f_maxscale
                              from samples s left join peaks p on p.address = s.address and p.type = s.type
                              join valuefacts f on f.address = s.address and f.type = s.type");

      if ($addrWhere == "")
          $strQuery = sprintf("%s where s.time = '%s'", $strQueryBase, $max);
      else
          $strQuery = sprintf("%s where (%s) and s.time = '%s'", $strQueryBase, $addrWhere, $max);

      // syslog(LOG_DEBUG, "p4: selecting " . " '" . $strQuery . "'");

      $result = $mysqli->query($strQuery)
          or die("Error" . $mysqli->error);

      echo "      <div class=\"container\">\n";

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

         $name = $row['f_name'];
         $value = $row['s_value'];
         $text = $row['s_text'];
         $title = (preg_replace("/($pumpDir)/i","",$row['f_usrtitle']) != "") ? preg_replace("/($pumpDir)/i","",$row['f_usrtitle']) : $row['f_title'];
         $unit = prettyUnit($row['f_unit']);
         $u = $row['f_unit'];
         $scaleMax = $row['f_maxscale'];
         $address = $row['s_address'];
         $type = $row['s_type'];
         $peak = $row['p_max'];

         if ($type == "UD" && $address == 1)                                      // 'Heating State' als Symbol anzeigen
         {
             echo "        <div class=\"widget-row rounded-border\">\n";
             echo "          <div class=\"widget-title\">$title</div>";
             echo "          <img class=\"widget-image\" src=\"$stateImg\">\n";
             echo "        </div>\n";
         }
         else if ($u == '°' || $unit == '%'|| $unit == 'V' || $unit == 'A')       // 'Volt/Ampere/Prozent/°C' als  Gauge
         {
             $scaleMax = $unit == '%' ? 100 : $scaleMax;
             $ratio = $value / $scaleMax;
             $peak = $peak / $scaleMax;

             echo "        <div class=\"widget-row rounded-border participation\" data-y=\"500\" data-unit=\"$unit\" data-value=\"$value\" data-peak=\"$peak\" data-ratio=\"$ratio\">\n";
             echo "          <div class=\"widget-title\">$title</div>";
             echo "          <svg class=\"svg\" viewBox=\"0 0 1000 600\" preserveAspectRatio=\"xMidYMin slice\">\n";
             echo "            <path d=\"M 950 500 A 450 450 0 0 0 50 500\"></path>\n";
             echo "            <text class='_content' text-anchor=\"middle\" alignment-baseline=\"middle\" x=\"500\" y=\"450\" font-size=\"140\" font-weight=\"bold\">$unit</text>\n";
             echo "            <text class='widget-scale' text-anchor=\"middle\" alignment-baseline=\"middle\" x=\"50\" y=\"550\">0</text>\n";
             echo "            <text class='widget-scale' text-anchor=\"middle\" alignment-baseline=\"middle\" x=\"950\" y=\"550\">$scaleMax</text>\n";
             echo "          </svg>\n";
              echo "        </div>\n";
         }
         else if ($text != '')                                                    // 'text' als Text anzeigen
         {
             $value = str_replace($wd_value, $wd_disp, $text);

             echo "        <div class=\"widget-row rounded-border\">\n";
             echo "          <div class=\"widget-title\">$title</div>";
             echo "          <div class=\"widget-value\">$value</div>";
             echo "        </div>\n";
         }
         else if ($u == 'h' || $u == 'U' || $u == 'R' || $u == 'm' || $u == 'u')  // 'value' als Text anzeigen
         {
             $value = round($value, 0);

             echo "        <div class=\"widget-row rounded-border\">\n";
             echo "          <div class=\"widget-title\">$title</div>";
             echo "          <div class=\"widget-value\">$value $unit</div>";
             echo "        </div>\n";
         }
         else if ($type == 'DI' || $type == 'DO' || $u == '')                     // 'boolean' als symbol anzeigen
         {
             if (strpos($name, "umpe") != FALSE)
                 $imagePath = $value == "1.00" ? "img/icon/pump-on-1.gif" : $imagePath = "img/icon/pump-off-1.png";
             else
                 $imagePath = $value == "1.00" ? "img/icon/boolean-on.png" : $imagePath = "img/icon/boolean-off.png";

             echo "        <div class=\"widget-row rounded-border\">\n";
             echo "          <div class=\"widget-title\">$title</div>";
             echo "          <img class=\"widget-image\" src=\"$imagePath\">\n";
             echo "        </div>\n";
         }
      }

      echo "      </div>\n";
  }

  $mysqli->close();

include("footer.php");
?>
