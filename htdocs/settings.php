<?php
include("header.php");

$action = htmlspecialchars($_POST["action"]);

mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
mysql_select_db($mysqldb) or die("DB error");
mysql_query("set names 'utf8'");
mysql_query("SET lc_time_names = 'de_DE'");

if ($action == "") {
   echo "
      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>
      <table  width=\"50%\" border=1 cellspacing=0 rules=rows>
      <tr style=\"color:white\" bgcolor=\"#000099\">
         <td> Adresse </td>
         <td> Typ </td>
         <td> Name </td>
         <td> Unit </td>
         <td> Aufzeichnen </td>
      </tr>\n";
   $result=mysql_query("select * from $mysqltable_values");
   $num=mysql_numrows($result);
   $i = 0;

   while ($i < $num) {
      $address = mysql_result($result,$i,"address");
      $type    = mysql_result($result,$i,"type");
      $title   = mysql_result($result,$i,"title");
      $unit    = mysql_result($result,$i,"unit");
      $state   = mysql_result($result,$i,"state");
      $txtaddr = sprintf("0x%x", $address);
      if ($state == "A") {
         $checked = " checked";
      } else { 
         $checked = "";
      }  

     if ($i % 2)
        echo "         <tr style=\"color:black\" bgcolor=\"#83AFFF\">";
     else
        echo "         <tr>";
     echo "
            <td> $txtaddr </td>
            <td> $type </td>
            <td> $title </td>
            <td> $unit </td>
            <td> <input type=\"checkbox\" name=\"selected[]\" value=\"$address:$type\"$checked> </td>
         </tr>\n";
     $i++;
   }

   echo "
      </table>
         <input type=hidden name=action value=update>
         <input type=hidden name=num_of_addresses value=$i>
         <input type=submit value=Update>
      </form>";
} 
elseif ($action == "update") {
   $sql="UPDATE $mysqltable_values SET `state`='D'; ";
   $result = mysql_query($sql) or die("Error" . mysql_error());

   $selected = $_POST['selected'];
   foreach ($_POST['selected'] as $key => $value) {
      $value = htmlspecialchars($value);
      list ($addr, $type) = split(":", $value);
     
      $sql="UPDATE $mysqltable_values SET `state`='A' where `address`='$addr' and `type`='$type'";
      $result = mysql_query($sql) or die("Error" . mysql_error());
   }

   echo "values updated<br>
      <a href=\"settings.php\">back to settings<a>";
}

mysql_close();

include("footer.php");
?>
