<?php

include("header.php");

printHeader();

// ----------------
// variables

$lastMenu = "";
$menu = "";
$edit = "";
$store = "";

if (isset($_POST["menu"]))
   $menu = htmlspecialchars($_POST["menu"]);

if (isset($_POST["edit"]))
   $edit = htmlspecialchars($_POST["edit"]);

if (isset($_POST["store"]))
   $store = htmlspecialchars($_POST["store"]);

if (isset($_SESSION["menu"]))
   $lastMenu = $_SESSION["menu"];

// -------------------------
// establish db connection

$mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb, $mysqlport);
$mysqli->query("set names 'utf8'");
$mysqli->query("SET lc_time_names = 'de_DE'");

// -----------------------
// Menü

showMenu($menu != "" ? $menu : $lastMenu);

// -----------------------
// Store Parameter

if ($store == "store")
{
   if (isset($_POST["new_value"]) && isset($_POST["store_id"]))
   {
      $state = -1;
      $newValue = htmlspecialchars($_POST["new_value"]);
      $newValueTo = isset($_POST["new_value_to"]) ? htmlspecialchars($_POST["new_value_to"]) : "";
      $storeId = htmlspecialchars($_POST["store_id"]);
      $isTimeRange = (strncmp($storeId, "tr#", 3) == 0);

      if ($isTimeRange)
         $state = storeRangeParameter($storeId, $newValue, $newValueTo, $unit, $res);
      else
         $state = storeParameter($storeId, $newValue, $unit, $res);

      if ($state == 0)
         echo "      <br/><div class=\"info\"><b><center>Gespeichert!</center></b></div><br/>\n";
      else if ($state == -99)
         echo "      <br/><div class=\"infoWarn\"><b><center>Warnung, speichern von '$newValue' "
            . "für Parameter '$storeId' ignoriert<br/>>> $res <<</center></b></div><br/>\n";
      else
         echo "      <br/><div class=\"infoError\"><b><center>Fehler beim speichern von '$newValue' "
            . "für Parameter '$storeId'<br/>>> $res <<</center></b></div><br/>\n";
   }
}

// -----------------------
// Edit Parameter

if ($edit != "")
{
   if (haveLogin())
   {
      $default = $min = $max = $digits = $valueTo = 0;
      $isTimeRange = (strncmp($edit, "tr#", 3) == 0);

      if ($isTimeRange)
         $res = requestRangeParameter($edit, $title, $value, $valueTo, $unit);
      else
         $res = requestParameter($edit, $title, $value, $unit, $default, $min, $max, $digits);

      error_log("DEBUG: $edit, $title, $value, $valueTo, $unit, $default, $min, $max, $digits");

      if ($res == 0)
      {
         echo "      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n";
         echo "        <div class=\"rounded-border inputTable\">\n";

         if ($isTimeRange)
            echo "          " . $title . ":  <span style=\"color:blue\">$value - $valueTo</span><br/>\n";
         else
            echo "          " . $title . ":  <span style=\"color:blue\">" . $value . $unit . "</span><br/>\n";

         echo "          <input type=\"hidden\" name=\"store_id\" value=$edit></input>\n";
         echo "          <input class=\"rounded-border input\" type=int name=new_value value=$value></input>\n";

         if ($isTimeRange)
         {
            echo "          <span style=\"color:blue\"> - </span>\n";
            echo "          <input class=\"rounded-border input\" type=int name=new_value_to value=$valueTo></input>\n";
            echo "          ('nn:nn - nn:nn' zum deaktivieren des Zeitbereichs)\n";
         }
         else
         {
            echo "          <span style=\"color:blue\">" . $unit . "</span>\n";
            echo "          (Bereich: " . $min . "-" . $max . ")   (Default: " . $default . ")\n";
         }

         echo "          <button class=\"rounded-border button3\" type=\"submit\" name=\"store\" value=\"store\">Speichern</button>\n";
         echo "        </div>\n";
         echo "      </form>\n";
      }
      else if ($res == -2)
         echo "      <br/><div class=\"infoWarn\"><b><center>Parametertyp noch nicht unterstützt</center></b></div><br/>";
      else
         echo "      <br/><div class=\"infoError\"><b><center>Kommunikationsfehler, Details im syslog</center></b></div><br/>";
   }
   else
   {
      echo "      <br/><div class=\"infoWarn\"><b><center>Zum ändern der Parameter Login erforderlich :o</center></b></div><br/>";
   }
}

if ($menu == "update")
{
   requestAction("updatemenu", 30, 0, "", $resonse);
   echo "      <br/><div class=\"info\"><b><center>Aktualisierung abgeschlossen</center></b></div><br/>";
   $menu = $lastMenu;
}

elseif ($menu == "init")
{
   requestAction("initmenu", 60, 0, "", $resonse);
   echo "      <br/><div class=\"info\"><b><center>Initialisierung abgeschlossen</center></b></div><br/>";
   $menu = $lastMenu;
}

if ($menu != "" || $lastMenu != "")
{
   $lastMenu = $menu != "" ? $menu : $lastMenu;

   echo "      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n";
   showChilds($lastMenu, 0);
   echo "      </form>\n";

   $_SESSION["menu"] = $lastMenu;
}

include("footer.php");

//***************************************************************************
// Show Menu
//***************************************************************************

function showMenu($current)
{
   global $mysqli;
   $i = 0;

   $result = $mysqli->query("select * from menu where parent = " . 1)
         or die("Error" . $mysqli->error);

   $count = $result->num_rows;

   echo "    <form action=\"" . htmlspecialchars($_SERVER["PHP_SELF"]) . "\" method=\"post\">\n";
//   echo "      <div class=\"menu\" style=\"position: fixed; top=44px;\">\n";
   echo "      <nav class=\"fixed-menu2\">\n";

   while ($i < $count)
   {
      $child = mysqli_result($result, $i, "child");
      $title = mysqli_result($result, $i, "title");

      if ($child == $current)
         echo "          <button class=\"rounded-border button2sel\" type=\"submit\" name=\"menu\" value=\"" . $child . "\">" . $title . "</button>\n";
      else
         echo "          <button class=\"rounded-border button2\" type=\"submit\" name=\"menu\" value=\"" . $child . "\">" . $title . "</button>\n";

      $i++;
   }

   echo "    </nav>\n";
   /* echo "    <div class=\"menu\" style=\"top=44px;\">\n"; */
   /* echo "    </div>\n"; */

   if (haveLogin())
   {
      echo "    <div class=\"menu\">\n";
      echo "      <button class=\"rounded-border button3\" type=submit name=\"menu\" value=\"init\" onclick=\"return confirmSubmit('Menüstruktur-Tabelle löschen und neu initialisieren?')\">Init</button>\n";
      echo "      <button class=\"rounded-border button3\" type=submit name=\"menu\" value=\"update\" onclick=\"return confirmSubmit('Werte der Parameter einlesen?')\">Aktualisieren</button>\n";
      echo "    </div>\n";
   }

   echo "        </form>\n";
}

//***************************************************************************
// Table
//***************************************************************************

function beginTable($title)
{
   echo "        <table class=\"tableTitle\" cellspacing=0 rules=rows>\n";
   echo "          <tr>\n";
   echo "            <td><center><b>$title</b></center></td>\n";
   echo "          </tr>\n";
}

function endTable()
{
   echo "        </table>\n";
}

//***************************************************************************
// Show Childs
//***************************************************************************

function showChilds($parnt, $level)
{
   global $debug, $mysqli, $wd_disp;

   $parnt = $mysqli->real_escape_string($parnt);
   $i = 0;

   $result = $mysqli->query("select * from menu where parent = " . $parnt)
         or die("Error" . $mysqli->error);

   $count = $result->num_rows;

   // syslog(LOG_DEBUG, "p4: menu width " . $count . " childs of parent " . $parnt . " on level " . $level);

   while ($i < $count)
   {
      $id      = mysqli_result($result, $i, "id");
      $child   = mysqli_result($result, $i, "child");
      $address = mysqli_result($result, $i, "address");
      $title   = mysqli_result($result, $i, "title");
      $type    = mysqli_result($result, $i, "type");
      $u1      = mysqli_result($result, $i, "unknown1");
      $u2      = mysqli_result($result, $i, "unknown2");
      $value   = "";

      $timeRangeGroup = ($u1 == 31 && $u2 == 16 && ($child == 573 || $child == 230 || $child == 350 || $child == 430));  // try to detect a time range

      if (rtrim($title) == "" && !$debug)
      {
         $i++;
         continue;
      }

      if ($type == 0x31)
      {
         $i++;
         continue;
      }

      if ($type == 0x03 || $type == 0x46)
         $value = getValue($address);

      if ($value == "")
         $value = getParameter($id);

      if ($debug)
      {
         switch ($type)
         {
            case 0x03:
            case 0x46: $txttp = "Messwert";  break;
            case 0x07: $txttp = "Par.";      break;
            case 0x08: $txttp = "Par Dig";   break;
            case 0x32: $txttp = "Par Set";   break;
            case 0x39: $txttp = "Par Set1";  break;
            case 0x40: $txttp = "Par Set2";  break;
            case 0x0a: $txttp = "Par Zeit";  break;
            case 0x11: $txttp = "Dig Out";   break;
            case 0x12: $txttp = "Anl Out";   break;
            case 0x13: $txttp = "Dig In";    break;
            case 0x22: $txttp = "Empty?";    break;
            case 0x23: $txttp = "Reset";     break;
            case 0x26: $txttp = "Zeiten";    break;
            case 0x2a: $txttp = "Par Guppe"; break;
            case 0x3a: $txttp = "Anzeigen";  break;
            case 0x16: $txttp = "Firmware";  break;

            default:
               $txttp = sprintf("0x%02x", $type);
         }

         $txtu1 = sprintf("0x%02x", $u1);
         $txtu2 = sprintf("0x%04x", $u2);
         $txtchild = $child ? sprintf("0x%04x", $child) : "-";
         $txtaddr  = $address ? sprintf("0x%04x", $address) : "";
      }

      if (!$level && $child)
      {
         if ($i > 0)
            endTable();

         beginTable($title);
      }
      elseif (!$level && !$child && !$address)  // ignore items without address and child in level 0
      {
         $i++;
         continue;
      }
      else
      {
         if (!$child || $level == 4)
            $style = "cellL4";
         elseif ($level == 1)
            $style = "cellL1";
         elseif ($level == 2)
            $style = "cellL2";
         elseif ($level == 3)
            $style = "cellL3";
         else
         {
            syslog(LOG_DEBUG, "p4: unexpected menu level " . $level);
            return;
         }

         echo "          <tr class=\"$style\">\n";

         if ($debug)
         {
            echo "            <td style=\"color:red\">($id)</td>\n";
            echo "            <td>$txtaddr</td>\n";
            echo "            <td style=\"color:blue\">$level</td>\n";
            echo "            <td>$txtchild</td>\n";
            echo "            <td style=\"color:red\">$txttp</td>\n";
            echo "            <td>$txtu1</td>\n";
            echo "            <td>$txtu2</td>\n";
         }

         if ($child)
            echo "            <td><center><b>$title</b></center></td>\n";
         elseif ($type == 0x07 || $type == 0x08 || $type == 0x40 || $type == 0x39 || $type == 0x32 || $type == 0x0a)
            echo "            <td><button class=tableButton type=submit name=edit value=$id>$title</button></td>\n";
         else
            echo "            <td>$title</td>\n";

         if ($value != "on (A)")
            echo "            <td style=\"color:blue\">$value</td>\n";
         else
            echo "            <td style=\"color:green\">$value</td>\n";

         echo "          </tr>\n";
      }

      if ($debug)
      {
         echo "          <tr class=\"cellL4\">\n";
         echo "            <td style=\"color:red\">($id)</td>\n";
         echo "            <td>$txtaddr</td>\n";
         echo "            <td style=\"color:blue\">$level</td>\n";
         echo "            <td>$txtchild</td>\n";
         echo "            <td style=\"color:red\">$txttp</td>\n";
         echo "            <td>$txtu1</td>\n";
         echo "            <td>$txtu2</td>\n";
         echo "            <td>TR: " . ($timeRangeGroup ? "ja" : "nein") . "</td>\n";
         echo "            <td>$title</td>\n";
         echo "          </tr>\n";
      }

      if ($timeRangeGroup)
      {
         // bei timeRangeGroup 'Zeiten' Menüpunkten enthält adress immer der wievielte (Heizkreis, Puffer, Bouiler, ... ) es ist
         // immer 7 (Tage) weiter kommt dann der nächste

         switch ($child)
         {
            case 230: $baseAddr = 0x00 + ($address * 7); break;   // Boiler 'n'
            case 350: $baseAddr = 0x38 + ($address * 7); break;   // Heizkreis 'n'
            case 430: $baseAddr = 0xb6 + ($address * 7); break;   // Puffer 'n'
            // case ???: $baseAddr = 0xd2 + ($address * 7); break    // Kessel
            case 573: $baseAddr = 0xd9 + ($address * 7); break;   // Zirkulation
         }

         for ($wday = 0; $wday < 7; $wday++)
         {
            $trAddr = $baseAddr + $wday;
            $stmt = "select * from timeranges where address = " . $trAddr;

            echo "          <tr class=\"cellL2\">\n";

            if ($debug)
            {
               echo "            <td style=\"color:red\">($id)</td>\n";
               echo "            <td>$txtaddr</td>\n";
               echo "            <td style=\"color:blue\">$level</td>\n";
               echo "            <td>$txtchild</td>\n";
               echo "            <td style=\"color:red\">$txttp</td>\n";
               echo "            <td>$txtu1</td>\n";
               echo "            <td>$txtu2</td>\n";
            }

            echo "            <td><center><b>$wd_disp[$wday]</b></center></td>\n";
            echo "            <td></td>\n";
            echo "          </tr>\n";

            $res = $mysqli->query("select * from timeranges where address = " . $trAddr)
               or die("Error" . $mysqli->error);

            $row = $res->fetch_array(MYSQLI_ASSOC);

            if ($row == NULL)
               continue;

            for ($n = 1; $n < 5; $n++)
            {
               $from = $row['from' . $n];
               $to = $row['to' . $n];

               echo "           <tr class=\"cellL4\">\n";

               if ($debug)
               {
                  echo "            <td style=\"color:red\">($id)</td>\n";
                  echo "            <td>$txtaddr</td>\n";
                  echo "            <td style=\"color:blue\">$level</td>\n";
                  echo "            <td>$txtchild</td>\n";
                  echo "            <td style=\"color:red\">$txttp</td>\n";
                  echo "            <td>$txtu1</td>\n";
                  echo "            <td>$txtu2</td>\n";
               }

               echo "            <td><button class=tableButton type=submit name=edit value=tr#$trAddr#$n#$wday>Bereich $n</button></td>\n";
               echo "             <td style=\"color:blue\">$from - $to</td>\n";
               echo "           </tr>\n";
            }
         }
      }
      else if ($child)
      {
         showChilds($child, $level+1);
      }

      $i++;
   }

   if ($level == 0)
      endTable();
}

//***************************************************************************
// Get Value
//***************************************************************************

function getValue($address)
{
   global $mysqli;

   $address = $mysqli->real_escape_string($address);

   $result = $mysqli->query("select max(time) as max from samples")
      or die("Error" . $mysqli->error);

   $row = $result->fetch_array(MYSQLI_ASSOC);
   $max = $row['max'];

   $strQuery = sprintf("select s.value as s_value, f.unit as f_unit from samples s, valuefacts f
              where f.address = s.address and f.type = s.type and s.time = '%s' and f.address = '%s' and f.type = 'VA'", $max, $address);

   // syslog(LOG_DEBUG, "p4: " . $strQuery);

   $result = $mysqli->query($strQuery)
      or die("Error" . $mysqli->error);

   if ($row = $result->fetch_array(MYSQLI_ASSOC))
   {
      $value = $row['s_value'];
      $unit = $row['f_unit'];
      return $value . $unit;
   }

   return ""; // requestValue($address);
}

//***************************************************************************
// Request Value
//***************************************************************************

function requestValue($address)
{
   global $mysqli;

   $address = $mysqli->real_escape_string($address);
   $timeout = time() + 10;

   syslog(LOG_DEBUG, "p4: requesting value " . $address);

   $mysqli->query("insert into jobs set requestat = now(), state = 'P', command = 'getv', address = '$address'")
      or die("Error" . $mysqli->error);

   $id = $mysqli->insert_id;

   while (time() < $timeout)
   {
      usleep(10000);

      $result = $mysqli->query("select * from jobs where id = $id and state = 'D'")
         or die("Error" . $mysqli->error);

      $count = $result->num_rows;

      if ($count == 1)
      {
         $response = mysqli_result($result, 0, "result");
         list($state, $value) = explode(":", $response);

         if ($state == "fail")
         {
            return "";
         }
         else
         {
            syslog(LOG_DEBUG, "p4: got response for value at addr " . $address . "-> " . $value);
            return $value;
         }
      }
   }

   syslog(LOG_DEBUG, "p4: timeout on value request ");

   return "-";
}

//***************************************************************************
// Get Parameter
//***************************************************************************

function getParameter($id)
{
   global $mysqli;

   $id = $mysqli->real_escape_string($id);
   $strQuery = sprintf("select value, unit from menu where id = '$id'");

   $result = $mysqli->query($strQuery)
      or die("Error" . $mysqli->error);

   // syslog(LOG_DEBUG, "p4: " . $strQuery);

   if ($row = $result->fetch_array(MYSQLI_ASSOC))
   {
      $value = $row['value'];
      $unit = $row['unit'];
      return $value . $unit;
   }

   return "";
}

//***************************************************************************
// Request Parameter
//***************************************************************************

function requestParameter($id, &$title, &$value, &$unit, &$default, &$min, &$max, &$digits)
{
   global $mysqli;

   $id = $mysqli->real_escape_string($id);
   $timeout = time() + 5;
   $address = 0;
   $type = "";

   $result = $mysqli->query("select * from menu where id = " . $id)
         or die("Error" . $mysqli->error);

   if ($result->num_rows != 1)
      return -1;

   $address = mysqli_result($result, 0, "address");
   $title = mysqli_result($result, 0, "title");
   $type = mysqli_result($result, 0, "type");

   syslog(LOG_DEBUG, "p4: requesting parameter" .$id . " at address " . $address . " type " . $type);

   $mysqli->query("insert into jobs set requestat = now(), state = 'P', command = 'getp', address = '$id'")
      or die("Error" . $mysqli->error);

   $jobid = $mysqli->insert_id;

   while (time() < $timeout)
   {
      usleep(10000);

      $result = $mysqli->query("select * from jobs where id = $jobid and state = 'D'")
         or die("Error" . $mysqli->error);

      if ($result->num_rows > 0)
      {
         $response = mysqli_result($result, 0, "result");
         list($state, $value, $unit, $default, $min, $max, $digits) = explode("#", $response);

         if ($state == "fail")
         {
            return -1;
         }
         else
         {
            syslog(LOG_DEBUG, "p4: got response for addr " . $address . "-> " . $value);

            return 0;
         }
      }
   }

   syslog(LOG_DEBUG, "p4: timeout on parameter request!");

   return -1;
}

//***************************************************************************
// Store Parameter
//***************************************************************************

function storeParameter($id, &$value, &$unit, &$res)
{
   global $mysqli;

   $id = $mysqli->real_escape_string($id);
   $value = $mysqli->real_escape_string($value);
   $timeout = time() + 5;
   $state = "";

   syslog(LOG_DEBUG, "p4: Storing parameter (" . $id . "), new value is " . $value);

   $mysqli->query("insert into jobs set requestat = now(), state = 'P', command = 'setp', "
               . "address = '$id', data = '$value'")
      or die("Error" . $mysqli->error);

   $jobid = $mysqli->insert_id;

   while (time() < $timeout)
   {
      usleep(10000);

      $result = $mysqli->query("select * from jobs where id = $jobid and state = 'D'")
         or die("Error" . $mysqli->error);

      if ($result->num_rows > 0)
      {
         $response = mysqli_result($result, 0, "result");

         if (!strstr($response, "success"))
         {
            list($state, $res) = explode("#", $response);

            if ($res == "no update")
               return -99;

            return -1;
         }

         list($state, $value, $unit, $default, $min, $max, $digits) = explode("#", $response);

         return 0;
      }
   }

   syslog(LOG_DEBUG, "p4: timeout on parameter store request!");
   $res = "p4d communication timeout";

   return -1;
}

//***************************************************************************
// Request Time Range Parameter
//***************************************************************************

function requestRangeParameter($id, &$title, &$valueFrom, &$valueTo, &$unit)
{
   global $mysqli, $wd_disp;

   list($tmp, $addr, $range, $wday) = explode("#", $id);
   $timeout = time() + 5;
   $title = "Zeitbereich $range für $wd_disp[$wday]";

   syslog(LOG_DEBUG, "p4: requesting time range parameter at address $addr for range $range ");

   $mysqli->query("insert into jobs set requestat = now(), state = 'P', command = 'gettrp', "
                  . "address = '$addr', data = '$range'")
      or die("Error" . $mysqli->error);

   $jobid = $mysqli->insert_id;

   while (time() < $timeout)
   {
      usleep(10000);

      $result = $mysqli->query("select * from jobs where id = $jobid and state = 'D'")
         or die("Error" . $mysqli->error);

      if ($result->num_rows > 0)
      {
         $response = mysqli_result($result, 0, "result");
         list($state, $valueFrom, $valueTo, $unit) = explode("#", $response);
         $unit = ""; // don't show 'Zeitbereich'

         if ($state == "fail")
         {
            return -1;
         }
         else
         {
            syslog(LOG_DEBUG, "p4: got response for time range parameter $addr/$range -> $value");
            return 0;
         }
      }
   }

   syslog(LOG_DEBUG, "p4: timeout on time range parameter request!");

   return -1;
}

//***************************************************************************
// Store Time Range Parameter
//***************************************************************************

function storeRangeParameter($id, &$valueFrom, &$valueTo, &$unit, &$res)
{
   global $mysqli;

   $id = $mysqli->real_escape_string($id);
   $valueFrom = $mysqli->real_escape_string($valueFrom);
   $valueTo = $mysqli->real_escape_string($valueTo);
   $timeout = time() + 5;
   $state = "";
   list($tmp, $addr, $range, $wday) = explode("#", $id);

   syslog(LOG_DEBUG, "p4: Storing time range parameter ($id), new value is $valueFrom - valueTo");

   $mysqli->query("insert into jobs set requestat = now(), state = 'P', command = 'settrp', "
               . "address = '$addr', data = '$range#$valueFrom#$valueTo'")
      or die("Error" . $mysqli->error);

   $jobid = $mysqli->insert_id;

   while (time() < $timeout)
   {
      usleep(10000);

      $result = $mysqli->query("select * from jobs where id = $jobid and state = 'D'")
         or die("Error" . $mysqli->error);

      if ($result->num_rows > 0)
      {
         $response = mysqli_result($result, 0, "result");

         if (!strstr($response, "success"))
         {
            list($state, $res) = explode("#", $response);

            return -1;
         }

         list($state, $valueFrom, $valueTo, $unit) = explode("#", $response);

         return 0;
      }
   }

   syslog(LOG_DEBUG, "p4: timeout on time range parameter store request!");
   $res = "p4d communication timeout";

   return -1;
}

?>
