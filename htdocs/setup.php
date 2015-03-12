<?php

// -------------------------
// chaeck login

if (haveLogin())
{
   echo "      <div>\n";
   echo "        <a class=\"button2\" href=\"basecfg.php\">Allg. Konfiguration</a>\n";
   echo "        <a class=\"button2\" href=\"alertcfg.php\">Sensor Alerts</a>\n";
   echo "        <a class=\"button2\" href=\"settings.php\">Aufzeichnung</a>\n";
   echo "        <a class=\"button2\" href=\"schemacfg.php\">Schema Konfiguration</a>\n";
   echo "      </div>\n";
}

?>
