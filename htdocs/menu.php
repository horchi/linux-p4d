<?php

include("header.php");

// ----------------
// variables

$debug = 0;
$lastMenu = "";
$menu = "";
$edit = "";
$table = "";
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

mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
mysql_select_db($mysqldb);
mysql_query("set names 'utf8'");
mysql_query("SET lc_time_names = 'de_DE'");

// -----------------------
// Menü

showMenu();

// -----------------------
// Store Parameter

if ($store == "store_par")
{
   if (isset($_POST["new_value"]) && isset($_POST["store_id"]))
   {
      $newValue = $_POST["new_value"];
      $storeId = $_POST["store_id"];
      $min = $_POST["min_value"];
      $max = $_POST["max_value"];
      $value = intval($newValue);

      if (!is_numeric($newValue))
         echo "      <br/><div class=\"infoWarn\"><b><center>Fehlerhaftes Zahlenformat '$newValue' - speichern abgebrochen</center></b></div><br/>\n";
      elseif ($value < $min || $value > $max)
         echo "      <br/><div class=\"infoWarn\"><b><center>Spezifizierter Wert '$value' außerhalb des erlaubten Bereichs ($min-$max)<br/>speichern abgebrochen</center></b></div><br/>\n";
      elseif (storeParameter($storeId, $value, $unit, $state) == 0)
         echo "      <br/><div class=\"info\"><b><center>Gespeichert!</center></b></div><br/>\n";
      else
         echo "      <br/><div class=\"infoError\"><b><center>Fehler beim speichern von $newValue für Parameter id $storeId<br/>'$state'</center></b></div><br/>\n";
   }
}

// -----------------------
// Edit Parameter

if ($edit != "")
{
   if (haveLogin()) 
   {
      $res = requestParameter($edit, $title, $value, $unit, $default, $min, $max, $digits);
      
      if ($res == 0)
      {
         echo "      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n";
         echo "        <br/><br/>\n";
         echo "        <div class=\"input\">\n";
         echo "          " . $title . ":  <span style=\"color:blue\">" . $value . $unit . "</span><br/><br/>\n";
         echo "          <input type=\"hidden\" name=\"store_id\" value=$edit></input>\n";
         echo "          <input type=\"hidden\" name=\"min_value\" value=$min></input>\n";
         echo "          <input type=\"hidden\" name=\"max_value\" value=$max></input>\n";
         echo "          <input class=\"inputEdit\" type=int name=new_value value=$value></input>\n";
         echo "          <span style=\"color:blue\">" . $unit . "</span>\n";
         echo "          (Bereich: " . $min . "-" . $max . ")   (Default: " . $default . ")   digits: " . $digits;
         echo "          <button class=\"button3\" type=submit name=store value=store_par>Speichern</button>\n";
         echo "          <br/><br/>\n";
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
   echo "      <br/><div class=\"info\"><b><center>Initialisierung abgeschlossen</center></b></div><br/><br/>";
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

function showMenu()
{
   $i = 0;

   $result = mysql_query("select * from menu where parent = " . 1)
         or die("Error" . mysql_error());

   $count = mysql_numrows($result);

   echo "      <div>\n";
   echo "        <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n";

   while ($i < $count)
   {
      $child = mysql_result($result, $i, "child");
      $title   = mysql_result($result, $i, "title");

      echo "          <button class=\"button2\" type=submit name=menu value=$child>$title</button>\n";

      $i++;
   }

   if (haveLogin()) 
   {
      echo "          <br/><br/>\n";
      echo "          <button class=\"button3\" type=submit name=menu value=init onclick=\"return confirmSubmit('Menüstruktur-Tabelle löschen und neu initialisieren?')\">Init</button>\n";
      echo "          <button class=\"button3\" type=submit name=menu value=update onclick=\"return confirmSubmit('Werte der Parameter einlesen?')\">Aktualisieren</button>\n";
   }

   echo "        </form>\n";
   echo "      </div>\n";
}

//***************************************************************************
// Table
//***************************************************************************

function beginTable($title)
{
   echo "        <br/>\n";

   echo "        <table class=\"tableHead\" cellspacing=0 rules=rows>\n";
   echo "          <tr style=\"color:white\" bgcolor=\"#CC0033\">\n";
   echo "            <td><center><b>$title</b></center></td>\n";
   echo "          </tr>\n";
   echo "        </table>\n";

   echo "        <br/>\n";

   echo "        <table class=\"tableLight\" cellspacing=0 rules=rows>\n";
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
   global $debug;

   $i = 0;

   $result = mysql_query("select * from menu where parent = " . $parnt)
         or die("Error" . mysql_error());

   $count = mysql_numrows($result);
   
   // syslog(LOG_DEBUG, "p4: menu with " . $count . " childs of parent " . $parnt . " on level " . $level);

   while ($i < $count)
   {
      $id      = mysql_result($result, $i, "id");
      $child   = mysql_result($result, $i, "child");
      $address = mysql_result($result, $i, "address");
      $title   = mysql_result($result, $i, "title");
      $type    = mysql_result($result, $i, "type");
      $u1      = mysql_result($result, $i, "unknown1");
      $u2      = mysql_result($result, $i, "unknown2");
      $value   = "";

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
            case 0x40:
            case 0x39:
            case 0x32: $txttp = "Par Set";   break;
            case 0x0a: $txttp = "Par Zeit";  break;
            case 0x11: $txttp = "Dig Out";   break;
            case 0x12: $txttp = "Anl Out";   break; 
            case 0x13: $txttp = "Dig In";    break; 
            case 0x22: $txttp = "Empty?";    break;
            case 0x23: $txttp = "Reset";     break;
            case 0x26: $txttp = "Zeiten";    break;
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
         if (!$child)
            $bgc = "#FFFFCC";
         elseif ($level == 1)
            $bgc = "#FF6600";
         elseif ($level == 2)
            $bgc = "#FFF66";
         elseif ($level == 3)
            $bgc = "#FFAA66";
         elseif ($level == 4)
            $bgc = "white";
         else
         {
            syslog(LOG_DEBUG, "p4: unexpected menu level " . $level);
            return;
         }

         echo "          <tr style=\"color:black\" bgcolor=\"$bgc\">\n";

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
            echo "            <td><button class=buttont type=submit name=edit value=$id>$title</button></td>\n";
         else
            echo "            <td>&nbsp;&nbsp;$title</td>\n";
         
         if ($value != "on (A)")
            echo "            <td style=\"color:blue\">$value</td>\n";
         else
            echo "            <td style=\"color:green\">$value</td>\n";
         
         echo "          </tr>\n";
      }

      if ($child)
         showChilds($child, $level+1);

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
   $result = mysql_query("select max(time) as max from samples")
      or die("Error" . mysql_error());

   $row = mysql_fetch_array($result, MYSQL_ASSOC);
   $max = $row['max'];
   
   $strQuery = sprintf("select s.value as s_value, f.unit as f_unit from samples s, valuefacts f
              where f.address = s.address and f.type = s.type and s.time = '%s' and f.address = '%s' and f.type = 'VA'", $max, $address);

   // syslog(LOG_DEBUG, "p4: " . $strQuery);
   
   $result = mysql_query($strQuery)
      or die("Error" . mysql_error());
   
   if ($row = mysql_fetch_array($result, MYSQL_ASSOC))
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
   $timeout = time() + 10;

   syslog(LOG_DEBUG, "p4: requesting value " . $address);

   mysql_query("insert into jobs set requestat = now(), state = 'P', command = 'getv', address = '$address'")
      or die("Error" . mysql_error());

   $id = mysql_insert_id();

   while (time() < $timeout)
   {
      usleep(1000);

      $result = mysql_query("select * from jobs where id = $id and state = 'D'")
         or die("Error" . mysql_error());

      $count = mysql_numrows($result);

      if ($count == 1)
      {
         $response = mysql_result($result, 0, "result");
         list($state, $value) = split(":", $response);

         if ($state == "fail")
         {
            // #TODO show error
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
   $strQuery = sprintf("select value, unit from menu 
              where id = '$id'");

   $result = mysql_query($strQuery)
      or die("Error" . mysql_error());

   // syslog(LOG_DEBUG, "p4: " . $strQuery);

   if ($row = mysql_fetch_array($result, MYSQL_ASSOC))
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
   $timeout = time() + 5;

   $address = 0;
   $type = "";

   $result = mysql_query("select * from menu where id = " . $id)
         or die("Error" . mysql_error());

   if (mysql_numrows($result) != 1)
      return -1;

   $address = mysql_result($result, 0, "address");
   $title = mysql_result($result, 0, "title");
   $type = mysql_result($result, 0, "type");

   if ($type != 0x07)
      return -2;

   syslog(LOG_DEBUG, "p4: requesting parameter at address " . $address . " type " . $type);

   mysql_query("insert into jobs set requestat = now(), state = 'P', command = 'getp', address = '$address'")
      or die("Error" . mysql_error());

   $jobid = mysql_insert_id();

   while (time() < $timeout)
   {
      usleep(1000);

      $result = mysql_query("select * from jobs where id = $jobid and state = 'D'")
         or die("Error" . mysql_error());

      if (mysql_numrows($result) > 0)
      {
         $response = mysql_result($result, 0, "result");
         list($state, $value, $unit, $default, $min, $max, $digits) = split(":", $response);

         if ($state == "fail")
         {
            // #TODO show error
            return -1;
         }
         else
         {
            syslog(LOG_DEBUG, "p4: got response for addr " . $address . "-> " . $value);

            return 0;
         }
      }
   }
   
   syslog(LOG_DEBUG, "p4: timeout on parameter request ");

   // #TODO show timeout

   return -1;
}

//***************************************************************************
// Store Parameter
//***************************************************************************

function storeParameter($id, &$value, &$unit, &$res)
{
   $timeout = time() + 5;
   $state = "";

   syslog(LOG_DEBUG, "p4: Storing parameter (" . $id . "), new value id " . $value);

   mysql_query("insert into jobs set requestat = now(), state = 'P', command = 'setp', address = '$id', result = '$value'")
      or die("Error" . mysql_error());

   $jobid = mysql_insert_id();

   while (time() < $timeout)
   {
      usleep(1000);

      $result = mysql_query("select * from jobs where id = $jobid and state = 'D'")
         or die("Error" . mysql_error());

      if (mysql_numrows($result) > 0)
      {
         $response = mysql_result($result, 0, "result");

         if (!strstr($response, "success"))
         {
            list($state, $res) = split(":", $response);

            return -1;
         }
         
         list($state, $value, $unit, $default, $min, $max, $digits) = split(":", $response);
         
         return 0;
      }
   }

   syslog(LOG_DEBUG, "p4: timeout on parameter request ");
   $res = "p4d communication timeout";

   return -1;
}

include("jfunctions.php");