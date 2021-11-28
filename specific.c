//***************************************************************************
// Automation Control
// File specific.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 16.04.2021 - Jörg Wendel
//***************************************************************************

#ifndef _NO_RASPBERRY_PI_
#  include <wiringPi.h>
#endif

#include "specific.h"

volatile int showerSwitch {0};

//***************************************************************************
// Configuration Items
//***************************************************************************

std::list<Daemon::ConfigItemDef> P4d::configuration
{
   // web

   { "addrsDashboard",            ctMultiSelect, "", false, "2 WEB Interface", "Sensoren 'Dashboard'", "Komma getrennte Liste aus Typ:ID siehe 'Aufzeichnung'" },
   { "addrsList",                 ctMultiSelect, "", false, "2 WEB Interface", "Sensoren 'Liste'", "Komma getrennte Liste aus Typ:ID siehe 'Aufzeichnung'" },

   { "webUrl",                    ctString,  "", false, "2 WEB Interface", "URL der Visualisierung", "kann mit %weburl% in die Mails eingefügt werden" },
   { "webSSL",                    ctBool,    "", false, "2 WEB Interface", "Use SSL for WebInterface", "" },
   { "haUrl",                     ctString,  "", false, "2 WEB Interface", "URL der Hausautomatisierung", "Zur Anzeige des Menüs als Link" },

   { "heatingType",               ctChoice,  "", false, "2 WEB Interface", "Typ der Heizung", "" },
   { "style",                     ctChoice,  "", false, "2 WEB Interface", "Farbschema", "" },
   { "iconSet",                   ctChoice,  "", false, "2 WEB Interface", "Status Icon Set", "" },
   { "schema",                    ctChoice,  "", false, "2 WEB Interface", "Schematische Darstellung", "" },

   // p4d

   { "interval",                  ctInteger, "60",   false, "1 P4 Daemon", "Intervall der Aufzeichung", "Datenbank Aufzeichung [s]" },
   { "webPort",                   ctInteger, "1111", false, "1 P4 Daemon", "Port des Web Interfaces", "" },
   { "stateCheckInterval",        ctInteger, "10",   false, "1 P4 Daemon", "Intervall der Status Prüfung", "Intervall der Status Prüfung [s]" },
   { "ttyDevice",                 ctString,  "/dev/ttyUSB0", false, "1 P4 Daemon", "TTY Device", "Beispiel: '/dev/ttyUsb0'" },
   { "loglevel",                  ctInteger, "1",    false, "1 P4 Daemon", "Log level", "" },

   { "tsync",                     ctBool,    "0",    false, "1 P4 Daemon", "Zeitsynchronisation", "täglich 3:00" },
   { "maxTimeLeak",               ctInteger, "5",    false, "1 P4 Daemon", " bei Abweichung über [s]", "Mindestabweichung für Synchronisation in Sekunden" },

   { "aggregateHistory",          ctInteger, "1",    false, "1 P4 Daemon", "Historie [Tage]", "history for aggregation in days (default 0 days -&gt; aggegation turned OFF)" },
   { "aggregateInterval",         ctInteger, "15",   false, "1 P4 Daemon", " danach aggregieren über", "aggregation interval in minutes - 'one sample per interval will be build'" },
   { "peakResetAt",               ctString,  "",     true,  "1 P4 Daemon", "", "" },

   { "consumptionPerHour",        ctNum,     "4",    false, "1 P4 Daemon", "Pellet Verbrauch / Stoker Stunde", "" },

   // MQTT interface

   { "mqttUrl",                   ctString,  "tcp://localhost:1883", false, "4 MQTT Interface", "MQTT Broker Url", "Optional. Beispiel: 'tcp://127.0.0.1:1883'" },
   { "mqttHassUrl",               ctString,  "",  false, "4 MQTT Interface", "MQTT HA Broker Url", "Optional. Beispiel: 'tcp://127.0.0.1:1883'" },
   { "mqttUser",                  ctString,  "",  false, "4 MQTT Interface", "User", "" },
   { "mqttPassword",              ctString,  "",  false, "4 MQTT Interface", "Password", "" },
   { "mqttDataTopic",             ctString,  "",  false, "4 MQTT Interface", "MQTT Data Topic Name", "&lt;NAME&gt; wird gegen den Messwertnamen und &lt;GROUP&gt; gegen den Namen der Gruppe ersetzt. Beispiel: p4d2mqtt/sensor/&lt;NAME&gt;/state" },
   { "mqttHaveConfigTopic",       ctBool,    "0", false, "4 MQTT Interface", "Config Topic", "Speziell für HomeAssistant" },

   // mail

   { "mail",                      ctBool,    "0", false, "3 Mail", "Mail Benachrichtigung", "Mail Benachrichtigungen aktivieren/deaktivieren" },
   { "mailScript",                ctString,  "/usr/bin/p4d-mail.sh", false, "3 Mail", "p4d sendet Mails über das Skript", "" },
   { "stateMailTo",               ctString,  "", false, "3 Mail", "Status Mail Empfänger", "Komma getrennte Empfängerliste" },
   { "stateMailStates",           ctMultiSelect, "", false, "3 Mail", "  für folgende Status", "" },

   { "errorMailTo",               ctString,  "", false, "3 Mail", "Fehler Mail Empfänger", "Komma getrennte Empfängerliste" },
};

//***************************************************************************
// P4d Daemon
//***************************************************************************

P4d::P4d()
   : Daemon()
{
   webPort = 1111;
}

P4d::~P4d()
{
}

//***************************************************************************
// Init/Exit
//***************************************************************************

int P4d::init()
{
   int status = Daemon::init();

   return status;
}

//***************************************************************************
// Init/Exit Database
//***************************************************************************

int P4d::initDb()
{
   int status = Daemon::initDb();

   return status;

}

int P4d::exitDb()
{
   return Daemon::exitDb();
}

int P4d::loadStates()
{
   Daemon::loadStates();

   return done;
}

//***************************************************************************
// Read Configuration
//***************************************************************************

int P4d::readConfiguration(bool initial)
{
   Daemon::readConfiguration(initial);

   return done;
}

int P4d::atMeanwhile()
{
   return done;
}

//***************************************************************************
// IO Interrupt Handler
//***************************************************************************

void ioInterrupt()
{
}

//***************************************************************************
// Apply Configuration Specials
//***************************************************************************

int P4d::applyConfigurationSpecials()
{
   return done;
}

//***************************************************************************
// Perform Jobs
//***************************************************************************

int P4d::performJobs()
{
   return done;
}

//***************************************************************************
// Process
//***************************************************************************

int P4d::process()
{
   return success;
}

//***************************************************************************
// Report Actual State
//***************************************************************************

void P4d::logReport()
{
}
