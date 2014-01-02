<?php

include("header.php");

printHeader();

session_destroy();

$hostname = $_SERVER['HTTP_HOST'];
$path = dirname($_SERVER['PHP_SELF']);

header('Location: http://'.$hostname.($path == '/' ? '' : $path).'/main.php');

?>
