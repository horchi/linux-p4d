<?php
session_start();

include("header.php");

  echo "
    <div class=\"schema_text\">
  ";

// if (!isset($_SESSION["num"]))
// {
//    session_start();
   
//    $_SESSION["num"] = 0; 
//    $_SESSION["cur"] = -1;
//    echo "<div style=\"position:absolute; z-index:2; left:900px; top:20px;\">Reset</div>\n";
//    syslog(LOG_DEBUG, "p4: session started");
// }

$action = htmlspecialchars($_POST["action"]);

// -------------------------
// establish db connection

mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
mysql_select_db($mysqldb);
mysql_query("set names 'utf8'");
mysql_query("SET lc_time_names = 'de_DE'");

if ($action == "click") 
{
   $mouseX = $_POST["mouse_x"];
   $mouseY = $_POST["mouse_y"];
   
   if ($mouseY > 100 && $mouseX < 100) 
   {
      $result = mysql_query("select * from schemaconf c, valuefacts f where f.address = c.address and f.type = c.type");
      $_SESSION["num"] = mysql_numrows($result);
      $_SESSION["cur"] = 0;
      $_SESSION["addr"] = -1;
 
      echo "<div style=\"position:absolute; left:800px; top:20px;\">Started ". $_SESSION["num"] . "</div>\n";
      syslog(LOG_DEBUG, "p4: init");

      $title = mysql_result($result, $_SESSION["cur"], "f.title");
      $_SESSION["addr"] = mysql_result($result, $_SESSION["cur"], "f.address");
      $_SESSION["type"] = mysql_result($result, $_SESSION["cur"], "f.type");
      
      echo "<div style=\"position:absolute; left:30px; top:40px; z-index:2;\">$title</div>\n";
      $_SESSION["cur"]++;   
   }
   elseif ($_SESSION["cur"] >= 0)
   {
      $result = mysql_query("select * from schemaconf c, valuefacts f where f.address = c.address and f.type = c.type");
      $_SESSION["num"] = mysql_numrows($result);  // update, who nows :o

      if ($_SESSION["cur"] < $_SESSION["num"])
      {
         if ($_SESSION["addr"] >= 0)
         {
            syslog(LOG_DEBUG, "p4: store position: " . $mouseX . "/" . $mouseY);
            
            mysql_query("update schemaconf set xpos = '" . $mouseX . 
                        "', ypos = '" . $mouseY . "' where address = '" . $_SESSION["addr"] . "' and type = '" .
                        $_SESSION["type"] . "'");
         }

         syslog(LOG_DEBUG, "p4: select next " .  $_SESSION["cur"]);

         $title = mysql_result($result, $_SESSION["cur"], "f.title");
         $_SESSION["addr"] = mysql_result($result, $_SESSION["cur"], "f.address");
         $_SESSION["type"] = mysql_result($result, $_SESSION["cur"], "f.type");
            
         echo "<div style=\"position:absolute; left:30px; top:40px; z-index:2;\">$title</div>\n";
         $_SESSION["cur"]++;
      }
   }
}
else
{
   $mouseX = -1;
   $mouseY = -1;
}

  echo "<form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>\n"; 
  echo "<div style=\"z-index:1;\">\n";
  echo "<input type=\"hidden\" name=\"action\" value=\"click\">\n";
  echo "<input type=\"image\" src=\"schema.jpg\" name=\"mouse\">\n";
  echo "</div>\n";

  // -------------------------
  // get last time stamp

  $result = mysql_query("select max(time), DATE_FORMAT(max(time),'%d. %M %Y   %H:%i') as maxPretty from samples;");
  $row = mysql_fetch_array($result, MYSQL_ASSOC);
  $max = $row['max(time)'];
  $maxPretty = $row['maxPretty'];

  // -------------------------
  // show values

  // echo "<div style=\"position:absolute; left:30px; top:10px; z-index:2;\">$maxPretty</div>\n";
  echo "<div style=\"position:absolute; left:700px; top:20px; z-index:2;\">($mouseX / $mouseY)</div>\n";
  
  // -------------------------
  // 

  $resultConf = mysql_query("select address, type, kind, color, xpos, ypos from schemaconf where state = 'A'");

  while ($rowConf = mysql_fetch_array($resultConf, MYSQL_ASSOC))
  {
     $addr = $rowConf['address'];
     $type = $rowConf['type'];
     $left = $rowConf['xpos'];
     $top = $rowConf['ypos'];
     $color = $rowConf['color'];

     $strQuery = sprintf("select s.value as s_value, f.unit as f_unit from samples s, valuefacts f where f.address = s.address and f.type = s.type and s.time = '%s' and f.address = %s and f.type = '%s';", $max, $addr, $type);
     $result = mysql_query($strQuery);

     if ($row = mysql_fetch_array($result, MYSQL_ASSOC))
     {
        $value = $row['s_value'];
        $unit = $row['f_unit'];

        echo "<div style=\"position:absolute; top:" . $top . "px; left:" . $left . "px" .
           "; color:" . $color . "; font-size:28px; z-index:2" . "\">" . $value . " " . $unit . "</div>\n";
     }
  }

  echo "  </form>\n";
  echo "</div>";

include("footer.php");
?>

