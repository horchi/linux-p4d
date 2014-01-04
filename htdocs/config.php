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

// -----------------------------
// don't touch below ;-)

if (function_exists('opcache_reset')) 
   opcache_reset();

?>
