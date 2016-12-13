<?php

// -----------------------------
// user settings

$mysqlhost       = "localhost";
$mysqluser       = "p4";
$mysqlpass       = "p4";
$mysqldb         = "p4";

$cache_dir       = "pChart/cache";
$chart_fontpath  = "pChart/fonts";
$debug           = 0;

$schema_path     = "img/schema/";
$schema_pattern  = "schema-*.png";
$schemaRange     = "48";
$wd_value        = array("Monday", "Tuesday",  "Wednesday", "Thursday",   "Friday",  "Saturday", "Sunday");
$wd_disp         = array("Montag", "Dienstag", "Mittwoch",  "Donnerstag", "Freitag", "Samstag",  "Sonntag");

// ------------------------------
// visual configs

$aligned         = "left";                     // Werte rechtsbündig/linksbündig ausrichten (not good working yet)
$pumpDir         = "-right|-left|-up|-down";

// -----------------------------
// don't touch below ;-)

if (function_exists('opcache_reset'))
   opcache_reset();

?>
