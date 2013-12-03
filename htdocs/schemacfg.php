<?php
session_start();

include("header.php");

$jpegTop = 90;
$jpegLeft = 30;

$action = "";
$cfg = "";
$started = 0;

if (isset($_POST["mouse"]))
   $action = htmlspecialchars($_POST["mouse"]);

if (isset($_POST["cfg"]))
   $cfg = htmlspecialchars($_POST["cfg"]);

if (isset($_SESSION["started"]))
   $started = $_SESSION["started"];

// -------------------------
// establish db connection

mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
mysql_select_db($mysqldb);
mysql_query("set names 'utf8'");
mysql_query("SET lc_time_names = 'de_DE'");

  // -------------------------
  // show image / form

  echo "\n";
  echo "    <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n"; 
  echo "      <div style=\"position:absolute; left:" . $jpegLeft . "px; top:" . $jpegTop . "px; z-index:2;\">\n";
  echo "        <input type=\"image\" src=\"schema.jpg\" value=\"click\" name=\"mouse\">\n";
  echo "      </div>\n";

if ($started == 1)
  echo "
      <input type=submit name=cfg value=Stop>";
else
  echo "
      <input type=submit name=cfg value=Start>";

  echo "
      <input type=submit name=cfg value=Skip>
      <input type=submit name=cfg value=Hide>
      <input type=submit name=cfg value=Back>
      <input type=checkbox name=unit value=unit>Einheit
  ";

if ($cfg == "Start") 
{
   $_SESSION["started"] = 1;
   syslog(LOG_DEBUG, "p4: starting config");

   $result = mysql_query("select * from schemaconf c, valuefacts f where f.address = c.address and f.type = c.type");
   $_SESSION["cur"] = 0;
   $_SESSION["addr"] = -1;

   nextConf(1);
}

if ($started == 1)
{
   if ($cfg == "Stop") 
   {
      $_SESSION["started"] = 0;
      syslog(LOG_DEBUG, "p4: stop");
   }
   elseif ($cfg == "Skip")
   {
      nextConf(1);
   }
   elseif ($cfg == "Back")
   {
      syslog(LOG_DEBUG, "p4: back");

      nextConf(-1);      
   }
   elseif ($cfg == "Hide")
   {
      syslog(LOG_DEBUG, "p4: hide");

      store("D", 0, 0, "black");
      nextConf(1);
   }
   
   if ($action == "click") 
   {
      $mouseX = $_POST["mouse_x"];
      $mouseY = $_POST["mouse_y"];
      
      if ($_SESSION["cur"] > 0)
      {
         $state = 'A';

         // check numrows

         $result = mysql_query("select * from schemaconf c, valuefacts f where f.address = c.address and f.type = c.type");
         $_SESSION["num"] = mysql_numrows($result);  // update ... who nows :o
         
         store("A", $mouseX, $mouseY, "black");
         
         nextConf(1);
      }
   }
}

  // -------------------------
  // show values

  include("schema.php");

  echo "    </form>\n";

include("footer.php");

function nextConf($dir)
{
   if ($dir == -1)
      $_SESSION["cur"] -= 2;

   if ($_SESSION["cur"] < 0)
      $_SESSION["cur"] = 0;

   syslog(LOG_DEBUG, "p4: select " .  $_SESSION["cur"]);

   // get last time stamp
   
   $result = mysql_query("select max(time), DATE_FORMAT(max(time),'%d. %M %Y   %H:%i') as maxPretty from samples;");
   $row = mysql_fetch_array($result, MYSQL_ASSOC);
   $max = $row['max(time)'];

   // select conf item

   $result = mysql_query("select * from schemaconf c, valuefacts f where f.address = c.address and f.type = c.type");
   $_SESSION["num"] = mysql_numrows($result);
   $_SESSION["addr"] = mysql_result($result, $_SESSION["cur"], "f.address");
   $_SESSION["type"] = mysql_result($result, $_SESSION["cur"], "f.type");

   $title = mysql_result($result, $_SESSION["cur"], "f.title");

   // get coresponding value/text and unit

   $strQuery = sprintf("select s.value as s_value, s.text as s_text, f.unit as f_unit from samples s, valuefacts f where f.address = s.address and f.type = s.type and s.time = '%s' and f.address = %s and f.type = '%s';", 
                       $max, $_SESSION["addr"], $_SESSION["type"]);
   $result = mysql_query($strQuery);
   
   if ($row = mysql_fetch_array($result, MYSQL_ASSOC))
   {
      $value = $row['s_value'];
      $unit = $row['f_unit'];
      $text = $row['s_text'];
   }
   
   // show

   echo "<div style=\"position:absolute; left:300px; top:51px; font-size:28px; color:blue; z-index:2;\">" . $title . " (" . $value . $unit . ")  " . $text . "</div>\n";

   $_SESSION["cur"]++;
   
   if ($_SESSION["cur"] >= $_SESSION["num"])
   {
      syslog(LOG_DEBUG, "p4: config done");
      $_SESSION["started"] = 0;
   }
}

function store($state, $xpos, $ypos, $color)
{
   if (isset($_POST["unit"]))
      $showUnit = 1;

   if ($_SESSION["cur"] < $_SESSION["num"] && $_SESSION["addr"] >= 0)
   {
      syslog(LOG_DEBUG, "p4: store position: " . $xpos . "/" . $ypos . " with unit: " . $showUnit);
      
      if ($state == "D")
         mysql_query("update schemaconf set state = '" . $state . "'" .
                     " where address = '" . $_SESSION["addr"] . "' and type = '" .
                     $_SESSION["type"] . "'");
      else
         mysql_query("update schemaconf set xpos = '" . $xpos . 
                     "', ypos = '" . $ypos . "', state = '" . $state . 
                     "' where address = '" . $_SESSION["addr"] . "' and type = '" .
                     $_SESSION["type"] . "'");
   }
}

?>
