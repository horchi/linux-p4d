<?php
$mysqlhost          = "localhost";
$mysqluser          = "p4";
$mysqlpass          = "p4";
$mysqldb            = "p4";
$mysqltable_values  = "valuefacts";
$mysqltable_samples = "samples";
$addrs_char1        = "0,1,113";
$addrs_char2        = "118,120,21,25,4";
$cache_dir          = "/var/cache/p4";

if( function_exists('opcache_reset') ) opcache_reset();
?>
