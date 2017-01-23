<?php

include("header.php");

include("pChart/class/pData.class.php");
include("pChart/class/pDraw.class.php");
include("pChart/class/pImage.class.php");

printHeader();

  // -------------------------
  // establish db connection

  $mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb, $mysqlport);
  $mysqli->query("set names 'utf8'");
  $mysqli->query("SET lc_time_names = 'de_DE'");

  // ----------------
  // init

  $day   = isset($_GET['c2sday'])   ? $_GET['c2sday']   : (int)date("d",time()-86400*$_SESSION['chartStart']);
  $month = isset($_GET['c2smonth']) ? $_GET['c2smonth'] : (int)date("m",time()-86400*$_SESSION['chartStart']);
  $year  = isset($_GET['c2syear'])  ? $_GET['c2syear']  : (int)date("Y",time()-86400*$_SESSION['chartStart']);
  $range = isset($_GET['c2range'])  ? $_GET['c2range']  : $_SESSION['chartStart'];

  $range = ($range > 7) ? 31 : (($range > 1) ? 7 : 1);
  $from = date_create_from_format('!Y-m-d', $year.'-'.$month.'-'.$day)->getTimestamp();

  echo "  <div class=\"rounded-border\" id=\"aSelect\">\n";
  echo "    <form name='navigation' method='get'>\n";
  echo "      Zeitraum der Charts<br/>\n";
  echo datePicker("", "c2s", $year, $day, $month);

  echo "      <select name=\"c2range\">\n";
  echo "        <option value='1' "  . ($range == 1  ? "SELECTED" : "") . ">Tag</option>\n";
  echo "        <option value='7' "  . ($range == 7  ? "SELECTED" : "") . ">Woche</option>\n";
  echo "        <option value='31' " . ($range == 31 ? "SELECTED" : "") . ">Monat</option>\n";
  echo "      </select>\n";
  echo "      <input type=submit value=\"Go\">\n";
  echo "    </form>\n";
  echo "  </div>\n";

  echo "  <div class=\"rounded-border chart\">\n";
  echo "    <img src='detail.php?from=" . $from . "&range=" . $range
     . "&condition=" . $_SESSION['chart3'] . "&chartXLines=" . $_SESSION['chartXLines']
     . "&chartDiv=" . $_SESSION['chartDiv'] . "'></img>\n";
  echo "  </div>\n";

  echo "  <div class=\"rounded-border chart\">\n";
  echo "    <img src='detail.php?from=" . $from . "&range=" . $range
     . "&condition=" . $_SESSION['chart4'] . "&chartXLines=" . $_SESSION['chartXLines']
     . "&chartDiv=" . $_SESSION['chartDiv'] . "'></img>\n";
  echo "  </div>\n";

include("footer.php");

?>
