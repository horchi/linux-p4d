<?php

include("header.php");

printHeader();

if ($_SERVER['REQUEST_METHOD'] == 'POST')
{
   // -------------------------
   // establish db connection

   $mysqli = new mysqli($mysqlhost, $mysqluser, $mysqlpass, $mysqldb, $mysqlport);
   $mysqli->query("set names 'utf8'");
   $mysqli->query("SET lc_time_names = 'de_DE'");

   // --------------------------

   $user = htmlspecialchars($_POST['username']);
   $passwd = htmlspecialchars($_POST['passwort']);

   $hostname = $_SERVER['HTTP_HOST'];
   $path = dirname($_SERVER['PHP_SELF']);

   // Benutzername und Passwort werden überprüft

   if (checkLogin($user, $passwd))
   {
      $_SESSION['angemeldet'] = true;

      // Weiterleitung zur geschützten Startseite

      if ($_SERVER['SERVER_PROTOCOL'] == 'HTTP/1.1')
      {
         if (php_sapi_name() == 'cgi')
            header('Status: 303 See Other');
         else
            header('HTTP/1.1 303 See Other');
      }

      header('Location: http://'.$hostname.($path == '/' ? '' : $path).'/index.php');
      exit;
   }
}

echo "      <form action=\"login.php\" method=\"post\">\n";
echo "        <br/>\n";
echo "        <div class=\"rounded-border inputTable\">\n";
echo "          <table>\n";
echo "            <tr><td>User:</td><td><input class=\"rounded-border input\" style=\"width:200px\" type=\"text\" name=\"username\"></input></td></tr>\n";
echo "            <tr><td></td><td></td></tr>\n";
echo "            <tr><td>Passwort:</td><td><input class=\"rounded-border input\" style=\"width:200px\" type=\"password\" name=\"passwort\"></input></td></tr>\n";
echo "          </table>\n";
echo "          <br/>\n";
echo "          <button class=\"rounded-border button3\" type=submit name=store value=login>Anmelden</button>\n";
echo "        </div>\n";
echo "      </form>\n";

include("footer.php");

?>
