<?php

include("header.php");

$menu = "";

if (isset($_POST["menu"]))
   $menu = htmlspecialchars($_POST["menu"]);

// -------------------------
// establish db connection

mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
mysql_select_db($mysqldb);
mysql_query("set names 'utf8'");
mysql_query("SET lc_time_names = 'de_DE'");

// -----------------------
//

showMenu();

if ($menu == "update")
{
   requestAction("updatemenu", 30);
   echo "<br/><br/><div class=\"info\"><b><center>Aktualisierung abgeschlossen</center></b></div>";
}
elseif ($menu == "init")
{
   requestAction("initmenu", 60);
   echo "<br/><br/><div class=\"info\"><b><center>Initialisierung abgeschlossen</center></b></div>";
}
elseif ($menu != "")
   showChilds($menu, 0);

include("footer.php");

//***************************************************************************
// Show Menu
//***************************************************************************

function showMenu()
{
   $i = 0;

   $result = mysql_query("select * from menu where parent = " . 1);
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

   echo "          <br>\n";
   echo "          <button class=\"button3\" type=submit name=menu value=init>Init</button>\n";
   echo "          <button class=\"button3\" type=submit name=menu value=update onclick=\"return confirmSubmit('Werte der Parameter aktualisieren?')\">Aktualisieren</button>\n";

   echo "        </form>\n";
   echo "      </div>\n";
}

//***************************************************************************
// Table
//***************************************************************************

function beginTable($title)
{
   echo "      <br>\n";

   echo "      <table class=\"tableHead\" cellspacing=0 rules=rows>\n";
   echo "        <tr style=\"color:white\" bgcolor=\"#CC0033\">\n";
   echo "          <td><center><b>$title</b></center></td>\n";
   echo "        </tr>\n";
   echo "      </table>\n";

   echo "      <br>\n";

   echo "      <table class=\"table\" cellspacing=0 rules=rows>\n";
}

function endTable()
{
   echo "      </table>\n";

}

//***************************************************************************
// Show Childs
//***************************************************************************

function showChilds($parnt, $level)
{
   $i = 0;

   $result = mysql_query("select * from menu where parent = " . $parnt);
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
      $value   = -1;

      if ($type == 0x31)
      {
         $i++;
         continue;
      }

      if ($type == 0x03)
      {
         $txttp = "Messwert";
         $value = getValue($address);
      }
      elseif ($type == 0x07)
         $txttp = "Par.";
      elseif ($type == 0x08)
         $txttp = "Par. dig";
      elseif ($type == 0x40 || $type == 0x39 || $type == 0x32)
         $txttp = "Par. set";
      elseif ($type == 0x0a)
         $txttp = "Par. Zeit";
      elseif ($type == 0x11)
         $txttp = "Dig Out";
      elseif ($type == 0x12)
         $txttp = "Anl Out";
      elseif ($type == 0x13)
         $txttp = "Dig In";
      else
         $txttp = sprintf("0x%02x", $type);
      
      if ($type != 0x03)
         $value = getParameter($id);

      $txtu1 = sprintf("0x%02x", $u1);
      $txtu2 = sprintf("0x%04x", $u2);
      $txtchild = $child ? sprintf("0x%04x", $child) : "-";
      $txtaddr  = $address ? sprintf("0x%04x", $address) : "";

      if ($level == 0 && $child)
      {
         if ($i > 0)
            endTable();
         
         beginTable($title);
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

         echo "        <tr style=\"color:black\" bgcolor=\"$bgc\">\n";
         
         echo "          <td style=\"color:red\">($id)</td>\n";
         echo "          <td>$txtaddr</td>\n";
         echo "          <td style=\"color:blue\">$level</td>\n";
         echo "          <td>$txtchild</td>\n";
         echo "          <td style=\"color:red\">$txttp</td>\n";
         echo "          <td>$txtu1</td>\n";
         echo "          <td>$txtu2</td>\n";

         if ($child)
            echo "          <td><center><b>$title</b></center></td>\n";
         elseif ($type == 0x07 || $type == 0x08 || $type == 0x40 || $type == 0x39 || $type == 0x32)
            echo "          <td><button class=buttont type=submit name=table value=$id>$title</button></td>\n";
         else
            echo "          <td>$title</td>\n";

         if ($value != -1)
            echo "          <td style=\"color:blue\">$value</td>\n";
         else
            echo "          <td></td>\n";

         echo "        </tr>\n";
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
   $result = mysql_query("select max(time) as max from samples");
   $row = mysql_fetch_array($result, MYSQL_ASSOC);
   $max = $row['max'];
   
   $strQuery = sprintf("select s.value as s_value, f.unit as f_unit from samples s, valuefacts f
              where f.address = s.address and f.type = s.type and s.time = '%s' and f.address = '%s' and f.type = 'VA'", $max, $address);

   // syslog(LOG_DEBUG, "p4: " . $strQuery);
   
   $result = mysql_query($strQuery);
   
   if ($row = mysql_fetch_array($result, MYSQL_ASSOC))
   {
      $value = $row['s_value'];
      $unit = $row['f_unit'];
      return $value . $unit;
   }
 
   return 0;
}

//***************************************************************************
// Get Parameter
//***************************************************************************

function getParameter($id)
{   
   $strQuery = sprintf("select value, unit from menu 
              where id = '$id'");

   $result = mysql_query($strQuery);

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

function requestParameter($address, $type)
{
   $timeout = time() + 10;

   syslog(LOG_DEBUG, "p4: requesting " . $address);

   mysql_query("insert into jobs set requestat = now(), state = 'P', command = 'getp', address = '$address'");
   $id = mysql_insert_id();

   while (time() < $timeout)
   {
      usleep(1000);

      $result = mysql_query("select * from jobs where id = $id and state = 'D'");
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
            syslog(LOG_DEBUG, "p4: got response for addr " . $address . "-> " . $value);

            return $value;
         }
      }
   }
   
   syslog(LOG_DEBUG, "p4: timeout on parameter request ");

   // #TODO show timeout
}

//***************************************************************************
// Request Action
//***************************************************************************

function requestAction($action, $timeout)
{
   $timeout = time() + $timeout;

   syslog(LOG_DEBUG, "p4: requesting ". $action);

   mysql_query("insert into jobs set requestat = now(), state = 'P', command = '$action', address = '0'");
   $id = mysql_insert_id();

   while (time() < $timeout)
   {
      usleep(1000);

      $result = mysql_query("select * from jobs where id = $id and state = 'D'");
      $count = mysql_numrows($result);

      if ($count)
         return;
   }

   syslog(LOG_DEBUG, "p4: timeout on " . $action);

   // #TODO show timeout
}

?>

<script language="JavaScript" type="text/JavaScript"> 
   <!-- 
function confirmSubmit(msg)
{
   if (confirm(msg)) 
      return true;
   else 
      return false;
} 
</script> 
