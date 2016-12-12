<?php

  include("header.php");

  printHeader();

  $jpegTop = 70;
  $jpegLeft = 30;

  // -------------------------
  // establish db connection

  $mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb);
  $mysqli->query("set names 'utf8'");
  $mysqli->query("SET lc_time_names = 'de_DE'");

  // -------------------------
  // show image

  $schemaImg = "img/schema/schema-" . $_SESSION["schema"] . ".png";

  echo "      <div class=\"schemaImage\" style=\"position:absolute; left:" . $jpegLeft . "px; top:" . ($jpegTop-20) . "px; z-index:2;\">\n";
  echo "        <p><img src=\"$schemaImg\"></p>\n";
  echo "      </div>\n";

  include("schema.php");

  include("footer.php");

?>
