<?php

include("config.php");
include("functions.php");

echo "
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">
<html>
  <head>
    <meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">
    <meta name=\"author\" content=\"Jörg Wendel\">
    <meta name=\"copyright\" content=\"Jörg Wendel\">
    <link rel=\"stylesheet\" type=\"text/css\" href=\"stylesheet.css\">
    <title>Fröhling P4</title>
  </head>
  <body>
    <a class=\"button1\" href=\"main.php\">Aktuell</a>
    <a class=\"button1\" href=\"chart.php\">Charts</a>
    <a class=\"button1\" href=\"schemadsp.php\">Schema</a>
    <a class=\"button1\" href=\"menu.php\">Menü</a>
    <a class=\"button1\" href=\"settings.php\">Setup</a>
    <a class=\"button1\" href=\"schemacfg.php\">Schema Setup</a>";

if (isset($_SERVER["PHP_AUTH_USER"]) && $_SERVER["PHP_AUTH_USER"] != "logout")
   echo "
    <a class=\"button1\" href=\"http://logout:password@" . $_SERVER["SERVER_ADDR"] . "/p4/logout/index.php\">logout</a>";

echo "
    <div class=\"content\">
";

?>
