<?php

session_start();

include("config.php");
include("functions.php");

// ----------------

//<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">

echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">
<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"de\" lang=\"de\">
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
    <a class=\"button1\" href=\"menu.php\">Menü</a>\n";

if (haveLogin())
{
   echo "    <a class=\"button1\" href=\"settings.php\">Setup</a>\n";
   echo "    <a class=\"button1\" href=\"schemacfg.php\">Schema Setup</a>\n";

   echo "    <a class=\"button1\" href=\"logout.php\">Logout</a>\n";
}
else
   echo "    <a class=\"button1\" href=\"login.php\">Login</a>\n";

echo "    <div class=\"content\">\n";

?>
