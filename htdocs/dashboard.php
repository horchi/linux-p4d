<?php

include("header.php");

printHeader($_SESSION['refreshWeb']);

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

$result = $mysqli->query("select max(time) from samples;")
   or die("Error" . $mysqli->error);

$row = $result->fetch_assoc();
$max = $row['max(time)'];

// ----------------
// State

$state = requestAction("s3200-state", 3, 0, "", $response);

if ($state == 0)
   list($time, $state, $status, $mode) = explode("#", $response, 4);

$stateImg = getStateImage($state);

// printState();

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

   $map = new \Ds\Map();
   $i = 0;

   while ($row = $result->fetch_assoc())
   {
      $id = "0x".dechex($row['s_address']).":".$row['s_type'];
      $map->put($id, $row);

      if ($addrWhere == "")
         $ids[$i++] = $id;
   }

   echo "      <div class=\"widgetContainer\">\n";

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

      // ------------------
      // draw widgets

      if (stripos($text, "ERROR:") !== FALSE)
      {
         list($tmp, $text) = explode(":", $text, 2);
         echo "        <div class=\"widget rounded-border\">\n";
         echo "          <div class=\"widget-title\">$title</div>";
         echo "          <div class=\"widget-value widget-error\">$text</div>";
         echo "        </div>\n";
      }

      else if ($type == "UD" && $address == 1)                                      // 'Heating State' als Symbol anzeigen
      {
         echo "        <div class=\"widget rounded-border\">\n";
         echo "          <div class=\"widget-title\">$title</div>";
         echo "          <button class=\"widget-main\">\n";
         echo "            <img src=\"$stateImg\"/>\n";
         echo "          </button>\n";

         // echo "          <img class=\"widget-main\" src=\"$stateImg\">\n";
         echo "        </div>\n";
      }
      else if ($u == '°' || $unit == '%'|| $unit == 'V' || $unit == 'A')       // 'Volt/Ampere/Prozent/°C' als  Gauge
      {
         $scaleMax = $unit == '%' ? 100 : $scaleMax;
         $scaleMin = $value >= 0 ? "0" : ceil($value / 5) * 5 - 5;

         if ($scaleMax < $value)
            $scaleMax = $value;
         if ($scaleMax < $peak)
            $scaleMax = $peak;

         $ratio = ($value - $scaleMin) / ($scaleMax - $scaleMin);
         $peak = ($peak - $scaleMin) / ($scaleMax - $scaleMin);
         $range = 1;
         $from = strtotime("today", time());

         $url = "href=\"#\" onclick=\"window.open('detail.php?width=1200&height=600&address=$address&type=$type&from="
            . $from . "&range=" . $range . "&chartXLines=" . $_SESSION['chartXLines'] . "&chartDiv="
            . $_SESSION['chartDiv'] . " ','_blank',"
            . "'scrollbars=yes,width=1200,height=600,resizable=yes,left=120,top=120')\"";

         echo "        <div $url class=\"widgetGauge rounded-border participation\" data-y=\"500\" data-unit=\"$unit\" data-value=\"$value\" data-peak=\"$peak\" data-ratio=\"$ratio\">\n";
         echo "          <div class=\"widget-title\">$title</div>";
         echo "          <svg class=\"widget-svg\" viewBox=\"0 0 1000 600\" preserveAspectRatio=\"xMidYMin slice\">\n";
         echo "            <path d=\"M 950 500 A 450 450 0 0 0 50 500\"></path>\n";
         echo "            <text class='_content' text-anchor=\"middle\" alignment-baseline=\"middle\" x=\"500\" y=\"450\" font-size=\"140\" font-weight=\"bold\">$unit</text>\n";
         echo "            <text class='scale-text' text-anchor=\"middle\" alignment-baseline=\"middle\" x=\"50\" y=\"550\">$scaleMin</text>\n";
         echo "            <text class='scale-text' text-anchor=\"middle\" alignment-baseline=\"middle\" x=\"950\" y=\"550\">$scaleMax</text>\n";
         echo "          </svg>\n";
         echo "        </div>\n";
      }

      else if ($text != '')                                                    // 'text' als Text anzeigen
      {
         $value = str_replace($wd_value, $wd_disp, $text);

         echo "        <div class=\"widget rounded-border\">\n";
         echo "          <div class=\"widget-title\">$title</div>";
         echo "          <div class=\"widget-value\">$value</div>";
         echo "        </div>\n";
      }

      else if ($type == 'DI' || $type == 'DO' || $u == '')                     // 'boolean' als symbol anzeigen
      {
         if (strpos($name, "umpe") != FALSE)
            $imagePath = $value == "1.00" ? "img/icon/pump-on-1.gif" : $imagePath = "img/icon/pump-off-1.png";
         else
            $imagePath = $value == "1.00" ? "img/icon/boolean-on.png" : $imagePath = "img/icon/boolean-off.png";

         echo "        <div class=\"widget rounded-border\">\n";
         echo "          <div class=\"widget-title\">$title</div>";
         echo "          <button class=\"widget-main\" type=submit name=action value=\"_switch:$address\">\n";
         echo "             <img src=\"$imagePath\">\n";
         echo "          </button>\n";
         echo "        </div>\n";
      }
      else // if ($u == 'h' || $u == 'U' || $u == 'R' || $u == 'm' || $u == 'u' || $u == 'l')  // 'value' als Text anzeigen
      {
         $value = round($value, 0);

         echo "        <div class=\"widget rounded-border\">\n";
         echo "          <div class=\"widget-title\">$title</div>";
         echo "          <div class=\"widget-value\">$value $unit</div>";
         echo "        </div>\n";
      }
   }

   echo "      </div>\n";  // widgetContainer
}

$mysqli->close();

include("footer.php");
?>
