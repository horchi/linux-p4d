<?php
#opcache_reset();

include("config.php");

$action = htmlspecialchars($_POST["action"]);

mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
mysql_select_db($mysqldb) or die("DB error");

if ($action == "") {
   echo "<html>
      <head>
      <title>Settings</title>
      </head>
      <body>
      Settings:<br>
      <form action=" . htmlspecialchars($_SERVER["PHP_SELF"]) . " method=post>
      <table border=1>
      <tr>
         <td> Adresse </td>
         <td> Typ </td>
         <td> Name </td>
         <td> Status </td>
      </tr> ";
   $result=mysql_query("select * from $mysqltable_values");
   $num=mysql_numrows($result);
   $i=0;
   while ($i < $num) {
      $address = mysql_result($result,$i,"address");
      $type    = mysql_result($result,$i,"type");
      $title   = mysql_result($result,$i,"title");
      $state   = mysql_result($result,$i,"state");
      $txtaddr = sprintf("0x%x", $address);
      if ($state == "A") {
         $checked = " checked";
      } else { 
         $checked = "";
      }  
      echo "
         <tr>
            <td> $txtaddr </td>
            <td> $type </td>
            <td> $title </td>
            <td> <input type=\"checkbox\" name=\"selected[]\" value=\"$address:$type\"$checked> </td>
         </tr>";
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
?>
