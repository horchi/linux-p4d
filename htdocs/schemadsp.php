<?php

  include("header.php");

  printHeader();

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

  echo "      <div class=\"schemaImage\" style=\"position:absolute;\">\n";
  echo "        <p><img src=\"schema.png\"></p>\n";
  echo "      </div>\n";

  include("schema.php");

  include("footer.php");

?>
