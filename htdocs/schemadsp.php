<?php

  include("header.php");

  $jpegTop = 70;
  $jpegLeft = 30;

  // -------------------------
  // establish db connection

  mysql_connect($mysqlhost, $mysqluser, $mysqlpass);
  mysql_select_db($mysqldb);
  mysql_query("set names 'utf8'");
  mysql_query("SET lc_time_names = 'de_DE'");

  // -------------------------
  // show image

  echo "<div class=\"schemaImage\">";
  echo "  <p><img src=\"schema.png\"></p>";
  echo "</div>";

  include("schema.php");
  include("footer.php");

?>
