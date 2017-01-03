<?php

  include("header.php");

  printHeader();

  // -------------------------
  // establish db connection

  $mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb, $mysqlport);
  $mysqli->query("set names 'utf8'");
  $mysqli->query("SET lc_time_names = 'de_DE'");

  // -------------------------
  // get last time stamp

  $result = $mysqli->query("select max(time), DATE_FORMAT(max(time),'%d. %M %Y   %H:%i') as maxPretty from samples;")
     or die("Error" . $mysqli->error);

  $row = $result->fetch_assoc();
  $max = $row['max(time)'];
  $maxPretty = $row['maxPretty'];
  $schemaRange = $_SESSION['schemaRange'] or $schemaRange = 60;    // Bereich und Anfang
  $from = time() - ($schemaRange * 60 *60);                        // der Charts beim Klick auf Werte im Schema

  // -------------------------
  // show Date

  echo "      <div class=\"rounded-border box\">\n";
  echo "        <div>$maxPretty</div>\n";
  echo "        <div>Chart-Zeitraum: $schemaRange Stunden</div>\n";
  echo "      </div>\n";

  // -------------------------
  // show image

  $schemaImg = "img/schema/schema-" . $_SESSION["schema"] . ".png";

  echo "      <div class=\"rounded-border imageBox\">\n";
  echo "        <img src=\"$schemaImg\">\n";

  include("schema.php");

  echo "      </div>\n";  // imageBox

  include("footer.php");
?>
