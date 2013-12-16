<?php

// -----------------------------
// user settings

$mysqlhost          = "localhost";
$mysqluser          = "p4";
$mysqlpass          = "p4";
$mysqldb            = "p4";

$addrs_chart1       = "0,1,113";
$addrs_chart2       = "118,120,21,25,4";
$cache_dir          = "/var/cache/p4";
$chart_fontpath     = "pChart/fonts";

// -----------------------------
// don't touch below ;-)

if (function_exists('opcache_reset')) 
   opcache_reset();

?>
