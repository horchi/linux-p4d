<?php

include("header.php");

printHeader();

include("setup.php");

// -------------------------
// chaeck login

if (!haveLogin())
{
   echo "<br/><div class=\"infoError\"><b><center>Login erforderlich!</center></b></div><br/>\n";
   die("<br/>");
}

// -------------------------
// establish db connection

$mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb, $mysqlport);
$mysqli->query("set names 'utf8'");
$mysqli->query("SET lc_time_names = 'de_DE'");

// -------------------------
// syslog

requestAction("syslog", 3, 0, "", $res);

echo "<div class=\"log\">\n";
echo str_replace("\n", "<br/>\n", htmlspecialchars($res)) . "\n";
echo "</div>\n";

// -------------------------

include("footer.php");

?>
