<?php

session_start();

$mysqlport = 3306;

$p4WebVersion = "<VERSION>";

include("config.php");
include("functions.php");

   // -------------------------
   // establish db connection

   $mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb, $mysqlport);

   if (mysqli_connect_error())
   {
      die('Connect Error (' . mysqli_connect_errno() . ') '
            . mysqli_connect_error() . ". Can't connect to " . $mysqlhost . ' at ' . $mysqlport);
   }

   $mysqli->query("set names 'utf8'");
   $mysqli->query("SET lc_time_names = 'de_DE'");

  // ----------------
  // Configuration

  // if (!isset($_SESSION['initialized']) || !$_SESSION['initialized'])
  {
     $_SESSION['initialized'] = true;

     // ------------------
     // get configuration

     readConfigItem("addrsMain", $_SESSION['addrsMain'], "");
     readConfigItem("addrsMainMobile", $_SESSION['addrsMainMobile'], "0,1,4,118,119,120");
     readConfigItem("addrsDashboard", $_SESSION['addrsDashboard'], "");

     readConfigItem("chartStart", $_SESSION['chartStart'], "7");
     readConfigItem("chartDiv", $_SESSION['chartDiv'], "25");
     readConfigItem("chartXLines", $_SESSION['chartXLines'], "0");
     readConfigItem("chart1", $_SESSION['chart1'], "0,1,113,18");
     readConfigItem("chart2", $_SESSION['chart2'], "118,225,21,25,4,8");

     // Chart 3+4

     readConfigItem("chart34", $_SESSION['chart34'], "0");
     readConfigItem("chart3", $_SESSION['chart3']);
     readConfigItem("chart4", $_SESSION['chart4']);

     readConfigItem("user", $_SESSION['user']);
     readConfigItem("passwd", $_SESSION['passwd']);

     readConfigItem("hmHost", $_SESSION['hmHost']);

     readConfigItem("mail", $_SESSION['mail']);
     readConfigItem("htmlMail", $_SESSION['htmlMail']);
     readConfigItem("stateMailTo", $_SESSION['stateMailTo']);
     readConfigItem("stateMailStates", $_SESSION['stateMailStates']);
     readConfigItem("errorMailTo", $_SESSION['errorMailTo']);
     readConfigItem("mailScript", $_SESSION['mailScript']);

     readConfigItem("tsync", $_SESSION['tsync']);
     readConfigItem("maxTimeLeak", $_SESSION['maxTimeLeak']);
     readConfigItem("heatingType", $_SESSION['heatingType'], "p4");
     readConfigItem("stateAni", $_SESSION['stateAni']);
     readConfigItem("schemaRange", $_SESSION['schemaRange']);
     readConfigItem("schema", $_SESSION['schema'], "p4-2hk-puffer");
     readConfigItem("schemaBez", $_SESSION['schemaBez']);
     readConfigItem("pumpON",  $_SESSION['pumpON'], "img/icon/pump-on.gif");
     readConfigItem("pumpOFF", $_SESSION['pumpOFF'], "img/icon/pump-off.gif");
     readConfigItem("ventON",  $_SESSION['ventON'], "img/icon/vent-on.gif");
     readConfigItem("ventOFF", $_SESSION['ventOFF'], "aus");
     readConfigItem("pumpsVA", $_SESSION['pumpsVA'], "(15),140,141,142,143,144,145,146,147,148,149,150,151,152,200,201,240,241,242");
     readConfigItem("pumpsDO", $_SESSION['pumpsDO'], "0,1,25,26,31,32,37,38,43,44,49,50,55,56,61,62,67,68");
     readConfigItem("pumpsAO", $_SESSION['pumpsAO'], "3,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22");
     readConfigItem("webUrl",  $_SESSION['webUrl'], "http://127.0.0.1/p4");
     readConfigItem("haUrl", $_SESSION['haUrl']);

     // ------------------
     // check for defaults

     if ($_SESSION['chartXLines'] == "")
        $_SESSION['chartXLines'] = "0";
     else
        $_SESSION['chartXLines'] = "1";

     // $mysqli->close();
  }

function printHeader($refresh = 0)
{
   $img = "img/type/heating-" . $_SESSION['heatingType'] . ".png";

   // ----------------
   // HTML Head

   echo "<!DOCTYPE html>\n";
   echo "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"de\" lang=\"de\">\n";
   echo "  <head>\n";
   echo "    <meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"/>\n";

   if ($refresh)
      echo "    <meta http-equiv=\"refresh\" content=\"$refresh\"/>\n";

   echo "    <meta name=\"author\" content=\"Jörg Wendel\"/>\n";
   echo "    <meta name=\"copyright\" content=\"Jörg Wendel\"/>\n";
   echo "    <meta name=\"viewport\" content=\"initial-scale=1.0, width=device-width, user-scalable=no, maximum-scale=1, minimum-scale=1\"/>\n";
   echo "    <link rel=\"shortcut icon\" href=\"img/type/heating-" . $_SESSION['heatingType'] . ".png\" type=\"image/png\"/>\n";
   echo "    <link rel=\"icon\" href=\"img/type/heating-" . $_SESSION['heatingType'] . ".png\" type=\"image/png\"/>\n";
   echo "    <link rel=\"apple-touch-icon-precomposed\" sizes=\"144x144\" href=\"img/type/heating-" . $_SESSION['heatingType'] . ".png\" type=\"image/png\"/>\n";
   echo "    <link rel=\"stylesheet\" type=\"text/css\" href=\"stylesheet.css\"/>\n";
   echo "    <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js\"></script>\n";
   echo "    <script src=\"https://cdnjs.cloudflare.com/ajax/libs/snap.svg/0.4.1/snap.svg-min.js\"></script>\n";
   echo "    <script type=\"text/JavaScript\" src=\"jfunctions.js\"></script>\n";
   echo "    <title>Fröling " . $_SESSION['heatingType'] . "</title>\n";
   echo "  </head>\n";
   echo "  <body>\n";

   // menu button bar ...

//   echo "    <nav class=\"menu\" style=\"position: fixed; top=0px;\">\n";
   echo "    <nav class=\"fixed-menu1\">\n";
   echo "      <a href=\"main.php\"><button class=\"rounded-border button1\">Aktuell</button></a>\n";
   echo "      <a href=\"dashboard.php\"><button class=\"rounded-border button1\">Dashboard</button></a>\n";
   echo "      <a href=\"chart.php\"><button class=\"rounded-border button1\">Charts</button></a>\n";

   if ($_SESSION['chart34'] == "1")
      echo "      <a href=\"chart2.php\"><button class=\"rounded-border button1\">Charts...</button></a>\n";

   echo "      <a href=\"schemadsp.php\"><button class=\"rounded-border button1\">Schema</button></a>\n";
   echo "      <a href=\"menu.php\"><button class=\"rounded-border button1\">Menü</button></a>\n";
   echo "      <a href=\"error.php\"><button class=\"rounded-border button1\">Fehler</button></a>\n";

   if (haveLogin())
   {
      echo "      <a href=\"basecfg.php\"><button class=\"rounded-border button1\">Setup</button></a>\n";
      echo "      <a href=\"logout.php\"><button class=\"rounded-border button1\">Logout</button></a>\n";
   }
   else
   {
      echo "      <a href=\"login.php\"><button class=\"rounded-border button1\">Login</button></a>\n";
   }

   if (isset($_SESSION['haUrl']) && $_SESSION['haUrl'] != "")
      echo "      <a href=\"" . $_SESSION['haUrl'] . "\"><button class=\"rounded-border button1\">HADashboard</button></a>\n";

   echo "    </nav>\n";
   /* echo "    <div class=\"menu\">\n"; */
   /* echo "    </div>\n"; */
   echo "    <div class=\"content\">\n";
?>
<script>

function pageShow()
{
   'use strict';

   var
      content = 0,
      nav1 = 0,
      nav2 = 0,
      nav3 = 0;

   content = document.querySelector('.content');
   nav1 = document.querySelector('.fixed-menu1');
   nav2 = document.querySelector('.fixed-menu2');
   nav3 = document.querySelector('.menu');

   if (!content)
      return true;

   if (nav3)
   {
      content.style.top = nav1.clientHeight + nav2.clientHeight + nav3.clientHeight + 'px';
      nav1.style.top = 0 + 'px';
      nav2.style.top = nav1.clientHeight + 'px';
      nav3.style.top = nav1.clientHeight + nav1.clientHeight + 'px';
   }
   else if (nav2)
   {
      content.style.top = nav1.clientHeight + nav2.clientHeight + 'px';
      nav1.style.top = 0 + 'px';
      nav2.style.top = nav1.clientHeight + 'px';
   }
   else if (nav1)
   {
      content.style.top = nav1.clientHeight + 'px';
      nav1.style.top = 0 + 'px';
   }
}

(function()
{
   'use strict';

   var
      navHeight = 0,
      navTop = 0,
      scrollCurr = 0,
      scrollPrev = 0,
      scrollDiff = 0,
      content = 0,
      nav1 = 0,
      nav2 = 0,
      nav3 = 0;

   window.addEventListener('scroll', function()
   {
      nav1 = document.querySelector('.fixed-menu1');
      content = document.querySelector('.content');

      if (!nav1 || !content)
         return true;

      pageShow();

      navHeight = nav1.clientHeight;
      scrollCurr = window.pageYOffset;
      scrollDiff = scrollPrev - scrollCurr;
      navTop = parseInt(window.getComputedStyle(content).getPropertyValue('top')) - scrollCurr;

      if (scrollCurr <= 0)              // Scroll to top: fix navbar to top
         nav1.style.top = '0px';
      else
         nav1.style.top = navTop - nav1.clientHeight + 'px';

      /* else if (scrollDiff > 0)          // Scroll up: show navbar */
      /*    nav1.style.top = (navTop > 0 ? 0 : navTop) + 'px'; */
      /* else if (scrollDiff < 0)          // Scroll down: hide navbar */
      /*    nav1.style.top = (Math.abs(navTop) > navHeight ? -navHeight : navTop) + 'px'; */

      nav2 = document.querySelector('.fixed-menu2');

      if (nav2)
      {
         navHeight = nav2.clientHeight;
         navTop = parseInt(window.getComputedStyle(nav2).getPropertyValue('top')) + scrollDiff;

         if (scrollCurr <= 0)           // Scroll to top: fix navbar to top
            nav2.style.top = nav1.clientHeight + 'px';
         else if (scrollDiff > 0)       // Scroll up: show navbar
            nav2.style.top = (navTop > 0 ? nav1.clientHeight : navTop) + 'px';
         else if (scrollDiff < 0)       // Scroll down: hide navbar
         {
            if (scrollCurr <= nav1.clientHeight)
               nav2.style.top = '0px';
            else
               nav2.style.top = (Math.abs(navTop) > navHeight ? -navHeight : navTop) + 'px';
         }
      }

      nav3 = document.querySelector('.menu');

      if (nav3)
      {
         navHeight = nav3.clientHeight;
         navTop = parseInt(window.getComputedStyle(nav3).getPropertyValue('top')) + scrollDiff;

         if (scrollCurr <= 0)           // Scroll to top: fix navbar to top
            nav3.style.top = '76px';
         else if (scrollDiff > 0)       // Scroll up: show navbar
            nav3.style.top = (navTop > 0 ? 76 : navTop) + 'px';
         else if (scrollDiff < 0)       // Scroll down: hide navbar
         {
            if (scrollCurr <= nav1.clientHeight)
               nav3.style.top = '0px';
            else
               nav3.style.top = (Math.abs(navTop) > navHeight ? -navHeight : navTop) + 'px';
         }
      }

      scrollPrev = scrollCurr;          // remember last scroll position
   });

}());

(function()
{
   'use strict';

   if (window.addEventListener)
      window.addEventListener("load", pageShow, false);
   else if (window.attachEvent)
   {
      window.attachEvent("onload", pageShow);
      window.attachEvent('pageshow', pageShow);
   }
   else
      window.onload = pageShow;

}());

</script>
<?php
}

?>
