//***************************************************************************
// Automation Control
// File specific.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 16.04.2021 - Jörg Wendel
//***************************************************************************

#ifndef _NO_RASPBERRY_PI_
#  include <wiringPi.h>
#else
#  include "gpio.h"
#endif

#include "lib/json.h"
#include "specific.h"

volatile int showerSwitch {0};

//***************************************************************************
// Configuration Items
//***************************************************************************

std::list<Daemon::ConfigItemDef> P4d::configuration
{
   // web

   // { "addrsDashboard",            ctMultiSelect, "", false, "2 WEB Interface", "Sensoren 'Dashboard'", "Komma getrennte Liste aus Typ:ID siehe 'Aufzeichnung'" },
   // { "addrsList",                 ctMultiSelect, "", false, "2 WEB Interface", "Sensoren 'Liste'", "Komma getrennte Liste aus Typ:ID siehe 'Aufzeichnung'" },

   { "webUrl",                    ctString,  "", false, "2 WEB Interface", "URL der Visualisierung", "kann mit %weburl% in die Mails eingefügt werden" },
   { "webSSL",                    ctBool,    "", false, "2 WEB Interface", "Use SSL for WebInterface", "" },
   { "haUrl",                     ctString,  "", false, "2 WEB Interface", "URL der Hausautomatisierung", "Zur Anzeige des Menüs als Link" },

   { "heatingType",               ctChoice,  "", false, "2 WEB Interface", "Typ der Heizung", "" },
   { "style",                     ctChoice,  "", false, "2 WEB Interface", "Farbschema", "" },
   { "iconSet",                   ctChoice,  "", false, "2 WEB Interface", "Status Icon Set", "" },
   { "schema",                    ctChoice,  "", false, "2 WEB Interface", "Schematische Darstellung", "" },

   { "dashboards",                ctString,  "{\"dashboard\":{\"UD:0x03\":true,\"UD:0x01\":true,\"VA:0x76\":true}}",     true, "2 WEB Interface", "Dashboard", "" },
   { "chartRange",                ctNum,     "1.5",    true, "2 WEB Interface", "Chart Range", "" },
   { "chartSensors",              ctNum,     "VA:0x0", true, "2 WEB Interface", "Chart Sensors", "" },

   // p4d

   { "interval",                  ctInteger, "60",   false, "1 P4 Daemon", "Intervall der Aufzeichung", "Datenbank Aufzeichung [s]" },
   { "webPort",                   ctInteger, "1111", false, "1 P4 Daemon", "Port des Web Interfaces", "" },
   { "stateCheckInterval",        ctInteger, "10",   false, "1 P4 Daemon", "Intervall der Status Prüfung", "Intervall der Status Prüfung [s]" },
   { "arduinoInterval",           ctInteger, "10",   false, "1 P4 Daemon", "Intervall der Arduino Messungen", "[s]" },
   { "ttyDevice",                 ctString,  "/dev/ttyUSB0", false, "1 P4 Daemon", "TTY Device", "Beispiel: '/dev/ttyUsb0'" },
   { "loglevel",                  ctInteger, "1",    false, "1 P4 Daemon", "Log level", "" },
   { "mqttUrl",                   ctString,  "tcp://localhost:1883", false, "4 MQTT Interface", "MQTT Broker Url", "Optional. Beispiel: 'tcp://127.0.0.1:1883'" },

   { "tsync",                     ctBool,    "0",    false, "1 P4 Daemon", "Zeitsynchronisation", "täglich 3:00" },
   { "maxTimeLeak",               ctInteger, "5",    false, "1 P4 Daemon", " bei Abweichung über [s]", "Mindestabweichung für Synchronisation in Sekunden" },

   { "aggregateHistory",          ctInteger, "1",    false, "1 P4 Daemon", "Historie [Tage]", "history for aggregation in days (default 0 days -&gt; aggegation turned OFF)" },
   { "aggregateInterval",         ctInteger, "15",   false, "1 P4 Daemon", " danach aggregieren über", "aggregation interval in minutes - 'one sample per interval will be build'" },
   { "peakResetAt",               ctString,  "",     true,  "1 P4 Daemon", "", "" },

   { "consumptionPerHour",        ctNum,     "4",    false, "1 P4 Daemon", "Pellet Verbrauch / Stoker Stunde", "" },

   // MQTT interface

   { "mqttHassUrl",               ctString,  "",  false, "4 MQTT Interface", "MQTT HA Broker Url", "Optional. Beispiel: 'tcp://127.0.0.1:1883'" },
   { "mqttHassUser",              ctString,  "",  false, "4 MQTT Interface", "MQTT HA User", "" },
   { "mqttHassPassword",          ctString,  "",  false, "4 MQTT Interface", "MQTT HA Password", "" },
   { "mqttDataTopic",             ctString,  "",  false, "4 MQTT Interface", "MQTT HA Data Topic Name", "&lt;NAME&gt; wird gegen den Messwertnamen und &lt;GROUP&gt; gegen den Namen der Gruppe ersetzt. Beispiel: p4d2mqtt/sensor/&lt;NAME&gt;/state" },
   { "mqttHaveConfigTopic",       ctBool,    "1", false, "4 MQTT Interface", "MQTT HA Config Topic", "Speziell für HomeAssistant" },

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
   free(stateMailAtStates);
}

//***************************************************************************
// Init/Exit
//***************************************************************************

int P4d::init()
{
   int status = Daemon::init();

   getConfigItem("knownStates", knownStates, "");

   if (!isEmpty(knownStates))
   {
      std::vector<std::string> sStates = split(knownStates, ':');
      for (const auto& s : sStates)
         stateDurations[atoi(s.c_str())] = 0;

      tell(eloAlways, "Loaded (%zu) states [%s]", stateDurations.size(), knownStates);
   }

   return status;
}

//***************************************************************************
// Init/Exit Database
//***************************************************************************

cDbFieldDef minValueDef("MIN_VALUE", "minvalue", cDBS::ffInt, 0, cDBS::ftData);
cDbFieldDef rangeEndDef("time", "time", cDBS::ffDateTime, 0, cDBS::ftData);
cDbFieldDef endTimeDef("END_TIME", "endtime", cDBS::ffDateTime, 0, cDBS::ftData);

int P4d::initDb()
{
   int status = Daemon::initDb();

   tableErrors = new cDbTable(connection, "errors");
   if (tableErrors->open() != success) return fail;

   tableMenu = new cDbTable(connection, "menu");
   if (tableMenu->open() != success) return fail;

   tableSensorAlert = new cDbTable(connection, "sensoralert");
   if (tableSensorAlert->open() != success) return fail;

   tableSchemaConf = new cDbTable(connection, "schemaconf");
   if (tableSchemaConf->open() != success) return fail;

   tableTimeRanges = new cDbTable(connection, "timeranges");  // #TODO - still needed
   if (tableTimeRanges->open() != success) return fail;

   tablePellets = new cDbTable(connection, "pellets");
   if (tablePellets->open() != success) return fail;

   // ----------------

   selectAllMenuItems = new cDbStatement(tableMenu);

   selectAllMenuItems->build("select ");
   selectAllMenuItems->bindAllOut();
   selectAllMenuItems->build(" from %s", tableMenu->TableName());

   status += selectAllMenuItems->prepare();

   // ----------------

   selectMenuItemsByParent = new cDbStatement(tableMenu);

   selectMenuItemsByParent->build("select ");
   selectMenuItemsByParent->bindAllOut();
   selectMenuItemsByParent->build(" from %s where ", tableMenu->TableName());
   selectMenuItemsByParent->bind("PARENT", cDBS::bndIn | cDBS::bndSet);

   status += selectMenuItemsByParent->prepare();

   // ----------------

   selectMenuItemsByChild = new cDbStatement(tableMenu);

   selectMenuItemsByChild->build("select ");
   selectMenuItemsByChild->bindAllOut();
   selectMenuItemsByChild->build(" from %s where ", tableMenu->TableName());
   selectMenuItemsByChild->bind("CHILD", cDBS::bndIn | cDBS::bndSet);

   status += selectMenuItemsByChild->prepare();

   // ------------------

   selectSchemaConfByState = new cDbStatement(tableSchemaConf);

   selectSchemaConfByState->build("select ");
   selectSchemaConfByState->bindAllOut();
   selectSchemaConfByState->build(" from %s where ", tableSchemaConf->TableName());
   selectSchemaConfByState->bind("STATE", cDBS::bndIn | cDBS::bndSet);

   status += selectSchemaConfByState->prepare();

   // ------------------

   selectAllSchemaConf = new cDbStatement(tableSchemaConf);

   selectAllSchemaConf->build("select ");
   selectAllSchemaConf->bindAllOut();
   selectAllSchemaConf->build(" from %s", tableSchemaConf->TableName());

   status += selectAllSchemaConf->prepare();

   // ------------------

   selectSensorAlerts = new cDbStatement(tableSensorAlert);

   selectSensorAlerts->build("select ");
   selectSensorAlerts->bindAllOut();
   selectSensorAlerts->build(" from %s where state = 'A'", tableSensorAlert->TableName());
   selectSensorAlerts->bind("KIND", cDBS::bndIn | cDBS::bndSet, " and ");

   status += selectSensorAlerts->prepare();

   // ------------------

   selectAllSensorAlerts = new cDbStatement(tableSensorAlert);

   selectAllSensorAlerts->build("select ");
   selectAllSensorAlerts->bindAllOut();
   selectAllSensorAlerts->build(" from %s", tableSensorAlert->TableName());

   status += selectAllSensorAlerts->prepare();

   // ------------------
   // select min(value), time from samples
   //    where address = ? type = ?
   //     and time > ?

   minValue.setField(&minValueDef);
   selectStokerHours = new cDbStatement(tableSamples);

   selectStokerHours->build("select ");
   selectStokerHours->bindTextFree("min(value)", &minValue, "", cDBS::bndOut);
   selectStokerHours->bind("TIME", cDBS::bndOut, ", ");
   selectStokerHours->build(" from %s where ", tableSamples->TableName());
   selectStokerHours->bind("ADDRESS", cDBS::bndIn | cDBS::bndSet);
   selectStokerHours->bind("TYPE", cDBS::bndIn | cDBS::bndSet, " and ");
   selectStokerHours->bindCmp(0, "TIME", 0, ">", " and ");

   status += selectStokerHours->prepare();

   // ------------------
   // state duration
   // select value, text, min(time)
   //  from samples
   //  where
   //    address = 1
   //    and type = 'UD'
   //    and text is not null
   //    and date(time) = curdate()
   //    and time > ?
   //    and vaue != ?

   endTime.setField(&endTimeDef);
   selectStateDuration = new cDbStatement(tableSamples);

   selectStateDuration->build("select ");
   selectStateDuration->bind("VALUE", cDBS::bndOut);
   selectStateDuration->bind("TEXT", cDBS::bndOut, ", ");
   selectStateDuration->bindTextFree("min(time)", &endTime, ", ", cDBS::bndOut);
   selectStateDuration->build(" from %s",  tableSamples->TableName());
   selectStateDuration->build(" where %s = 1", tableSamples->getField("ADDRESS")->getDbName());
   selectStateDuration->build(" and %s = 'UD'", tableSamples->getField("TYPE")->getDbName());
   selectStateDuration->build(" and %s is not null", tableSamples->getField("TEXT")->getDbName());
   selectStateDuration->build(" and date(%s) = curdate()", tableSamples->getField("TIME")->getDbName());
   selectStateDuration->bindCmp(0, "TIME", 0, ">", " and ");
   selectStateDuration->bindCmp(0, "VALUE", 0, "!=", " and ");

   status += selectStateDuration->prepare();

   // ------------------
   // all errors

   selectAllErrors = new cDbStatement(tableErrors);

   selectAllErrors->build("select ");
   selectAllErrors->bindAllOut();
   selectAllErrors->build(" from %s",  tableErrors->TableName());
   selectAllErrors->build(" order by time1 desc");

   status += selectAllErrors->prepare();

   // ------------------
   // pending errors

   selectPendingErrors = new cDbStatement(tableErrors);

   selectPendingErrors->build("select ");
   selectPendingErrors->bindAllOut();
   selectPendingErrors->build(" from %s where %s <> 'quittiert' and (%s <= 0 or %s is null)",
                              tableErrors->TableName(),
                              tableErrors->getField("STATE")->getDbName(),
                              tableErrors->getField("MAILCNT")->getDbName(),
                              tableErrors->getField("MAILCNT")->getDbName());

   status += selectPendingErrors->prepare();

   // ------------------
   // select all pellets

   selectAllPellets = new cDbStatement(tablePellets);
   selectAllPellets->build("select ");
   selectAllPellets->bindAllOut();
   selectAllPellets->build(" from %s", tablePellets->TableName());
   selectAllPellets->build(" order by time");
   status += selectAllPellets->prepare();

   // ------------------
   // select * from samples      (for alertCheck)
   //    where type = ? and address = ?
   //     and time <= ?
   //     and time > ?

   rangeEnd.setField(&rangeEndDef);

   selectSampleInRange = new cDbStatement(tableSamples);

   selectSampleInRange->build("select ");
   selectSampleInRange->bind("ADDRESS", cDBS::bndOut);
   selectSampleInRange->bind("TYPE", cDBS::bndOut, ", ");
   selectSampleInRange->bind("TIME", cDBS::bndOut, ", ");
   selectSampleInRange->bind("VALUE", cDBS::bndOut, ", ");
   selectSampleInRange->build(" from %s where ", tableSamples->TableName());
   selectSampleInRange->bind("ADDRESS", cDBS::bndIn | cDBS::bndSet);
   selectSampleInRange->bind("TYPE", cDBS::bndIn | cDBS::bndSet, " and ");
   selectSampleInRange->bindCmp(0, &rangeEnd, "<=", " and ");
   selectSampleInRange->bindCmp(0, "TIME", 0, ">", " and ");
   selectSampleInRange->build(" order by time");

   status += selectSampleInRange->prepare();

   return status;

}

int P4d::exitDb()
{
   delete tablePellets;               tablePellets= nullptr;
   delete tableMenu;                  tableMenu = nullptr;
   delete tableSensorAlert;           tableSensorAlert = nullptr;
   delete tableSchemaConf;            tableSchemaConf = nullptr;
   delete tableErrors;                tableErrors = nullptr;
   delete tableTimeRanges;            tableTimeRanges = nullptr;

   delete selectAllMenuItems;         selectAllMenuItems = nullptr;
   delete selectMenuItemsByParent;    selectMenuItemsByParent = nullptr;
   delete selectMenuItemsByChild;     selectMenuItemsByChild = nullptr;
   delete selectSensorAlerts;         selectSensorAlerts = nullptr;
   delete selectAllSensorAlerts;      selectAllSensorAlerts = nullptr;
   delete selectSampleInRange;        selectSampleInRange = nullptr;
   delete selectAllErrors;            selectAllErrors = nullptr;
   delete selectPendingErrors;        selectPendingErrors = nullptr;

   delete selectAllPellets;           selectAllPellets = nullptr;
   delete selectStokerHours;          selectStokerHours = nullptr;
   delete selectStateDuration;        selectStateDuration = nullptr;
   delete selectSchemaConfByState;    selectSchemaConfByState = nullptr;
   delete selectAllSchemaConf;        selectAllSchemaConf = nullptr;

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

   getConfigItem("tsync", tSync, no);
   getConfigItem("maxTimeLeak", maxTimeLeak, 10);
   getConfigItem("stateMailStates", stateMailAtStates, "0,1,3,19");
   getConfigItem("consumptionPerHour", consumptionPerHour, 0);
   getConfigItem("iconSet", iconSet, "light");
   getConfigItem("heatingType", heatingType, "P4");
   tell(eloDetail, "The heating type is set to '%s'", heatingType);
   getConfigItem("knownStates", knownStates, "");

   if (!isEmpty(knownStates))
   {
      std::vector<std::string> sStates = split(knownStates, ':');
      for (const auto& s : sStates)
         stateDurations[atoi(s.c_str())] = 0;

      tell(eloAlways, "Loaded (%zu) states [%s]", stateDurations.size(), knownStates);
   }


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
// On Update
//***************************************************************************

int P4d::onUpdate(bool webOnly, cDbTable* table, time_t lastSampleTime, json_t* ojData)
{
   char num[100] {'\0'};
   const char* type = tableValueFacts->getStrValue("TYPE");
   uint addr = table->getIntValue("ADDRESS");
   const char* unit = tableValueFacts->getStrValue("UNIT");
   const char* name = tableValueFacts->getStrValue("NAME");
   const char* title = tableValueFacts->getStrValue("TITLE");
   const char* usrtitle = tableValueFacts->getStrValue("USRTITLE");
   double factor = tableValueFacts->getIntValue("FACTOR");
   uint groupid = tableValueFacts->getIntValue("GROUPID");
   // const char* orgTitle = title;

   if (!isEmpty(usrtitle))
      title = usrtitle;

   if (table->hasValue("TYPE", "SD"))   // state duration
   {
      const auto it = stateDurations.find(addr);

      if (it == stateDurations.end())
         return done;

      double value = stateDurations[addr] / 60;

      json_object_set_new(ojData, "value", json_real(value));

      if (!webOnly)
      {
         store(lastSampleTime, name, title, unit, type, addr, value, factor, groupid);
         sprintf(num, "%.2f%s", value / factor, unit);
         addParameter2Mail(title, num);
      }
   }
   else if (tableValueFacts->hasValue("TYPE", "UD"))
   {
      switch (tableValueFacts->getIntValue("ADDRESS"))
      {
         case udState:
         {
            json_object_set_new(ojData, "text", json_string(currentState.stateinfo));
            json_object_set_new(ojData, "image", json_string(getStateImage(currentState.state)));

            if (!webOnly)
            {
               store(lastSampleTime, name, unit, title, type, udState, currentState.state, factor, groupid, currentState.stateinfo);
               addParameter2Mail(title, currentState.stateinfo);
            }

            break;
         }
         case udMode:
         {
            json_object_set_new(ojData, "text", json_string(currentState.modeinfo));

            if (!webOnly)
            {
               store(lastSampleTime, name, title, unit, type, udMode, currentState.mode, factor, groupid, currentState.modeinfo);
               addParameter2Mail(title, currentState.modeinfo);
            }

            break;
         }
         case udTime:
         {
            std::string date = l2pTime(currentState.time, "%A, %d. %b. %Y %H:%M:%S");
            json_object_set_new(ojData, "text", json_string(date.c_str()));

            if (!webOnly)
            {
               store(lastSampleTime, name, title, unit, type, udTime, currentState.time, factor, groupid, date.c_str());
               addParameter2Mail(title, date.c_str());
            }

            break;
         }
      }
   }
   else if (tableValueFacts->hasValue("TYPE", "VA"))
   {
      int status {success};
      Value v(addr);

      if ((status = request->getValue(&v)) != success)
      {
         tell(eloAlways, "Getting value 0x%04x failed, error %d", addr, status);
         return done;
      }

      int dataType = tableValueFacts->getIntValue("SUBTYPE");
      int value = dataType == 1 ? (word)v.value : (sword)v.value;
      double theValue = value / factor;

      json_object_set_new(ojData, "value", json_real(theValue));
      // needed?? json_object_set_new(ojData, "image", json_string(getImageFor(orgTitle, theValue)));

      if (!webOnly)
      {
         vaSensors[v.address].kind = "value";
         vaSensors[v.address].value = value / (double)factor;
         vaSensors[v.address].valid = true;
         vaSensors[v.address].last = lastSampleTime;

         store(lastSampleTime, name, title, unit, type, v.address, value, factor, groupid);
         sprintf(num, "%.2f%s", theValue, unit);
         addParameter2Mail(title, num);
      }
   }

   return done;
}

//***************************************************************************
// Update State
//***************************************************************************

int P4d::updateState()
{
   static time_t nextReportAt = 0;

   int status;
   time_t now;

   // get state

   sem->p();
   tell(eloDetail, "Checking state ...");
   status = request->getStatus(&currentState);
   now = time(0);
   sem->v();

   if (status != success)
      return status;

   tell(eloDetail, "... got (%d) '%s'%s", currentState.state, toTitle(currentState.state),
        isError(currentState.state) ? " -> Störung" : "");

   // ----------------------
   // check time sync

   if (!nextTimeSyncAt)
      scheduleTimeSyncIn();

   if (tSync && maxTimeLeak && labs(currentState.time - now) > maxTimeLeak)
   {
      if (now > nextReportAt)
      {
         tell(eloAlways, "Time drift is %ld seconds", currentState.time - now);
         nextReportAt = now + 2 * tmeSecondsPerMinute;
      }

      if (now > nextTimeSyncAt)
      {
         scheduleTimeSyncIn(tmeSecondsPerDay);

         tell(eloAlways, "Time drift is %ld seconds, syncing now", currentState.time - now);

         sem->p();

         if (request->syncTime() == success)
            tell(eloAlways, "Time sync succeeded");
         else
            tell(eloAlways, "Time sync failed");

         sleep(2);   // S-3200 need some seconds to store time :o

         status = request->getStatus(&currentState);
         now = time(0);

         sem->v();

         tell(eloAlways, "Time drift now %ld seconds", currentState.time - now);
      }
   }

   return status;
}

//***************************************************************************
// Schedule Time Sync In
//***************************************************************************

void P4d::scheduleTimeSyncIn(int offset)
{
   struct tm tm = {0};
   time_t now;

   now = time(0);
   localtime_r(&now, &tm);

   tm.tm_sec = 0;
   tm.tm_min = 0;
   tm.tm_hour = 3;
   tm.tm_isdst = -1;               // force DST auto detect

   nextTimeSyncAt = mktime(&tm);
   nextTimeSyncAt += offset;
}

//***************************************************************************
// Send State Mail
//***************************************************************************

int P4d::sendStateMail()
{
   std::string subject = "Heizung - Status: " + std::string(currentState.stateinfo);

   // check

   if (!isMailState() || isEmpty(mailScript) || !mailBodyHtml.length() || isEmpty(stateMailTo))
      return done;

   // HTML mail

   char* html {nullptr};

   loadHtmlHeader();

   asprintf(&html,
            "<html>\n"
            " %s\n"
            "  <body>\n"
            "   <font face=\"Arial\"><br/>WEB Interface: <a href=\"%s\">S 3200</a><br/></font>\n"
            "   <br/>\n"
            "   <table>\n"
            "     <thead>\n"
            "       <tr class=\"head\">\n"
            "         <th><font>Parameter</font></th>\n"
            "         <th><font>Wert</font></th>\n"
            "       </tr>\n"
            "     </thead>\n"
            "     <tbody>\n"
            "%s"
            "     </tbody>\n"
            "   </table>\n"
            "   <br/>\n"
            "  </body>\n"
            "</html>\n",
            htmlHeader.memory, webUrl, mailBodyHtml.c_str());

   int result = sendMail(stateMailTo, subject.c_str(), html, "text/html");
   free(html);
   mailBodyHtml = "";

   return result;
}

//***************************************************************************
// Is Mail State
//***************************************************************************

int P4d::isMailState()
{
   int result = no;
   char* mailStates = 0;

   if (isEmpty(stateMailAtStates))
      return yes;

   mailStates = strdup(stateMailAtStates);

   for (const char* p = strtok(mailStates, ","); p; p = strtok(0, ","))
   {
      if (atoi(p) == currentState.state)
      {
         result = yes;
         break;
      }
   }

   free(mailStates);

   return result;
}

//***************************************************************************
// Perform Jobs
//***************************************************************************

int P4d::performJobs()
{
   return done;
}

//***************************************************************************
// After Update
//***************************************************************************

void P4d::afterUpdate()
{
   Daemon::afterUpdate();

   updateErrors();
   calcStateDuration();
   sensorAlertCheck(lastSampleTime);

   if (mail && errorsPending)
      sendErrorMail();

   if (mail && stateChanged)
      sendStateMail();

   oJson = json_object();
   s3200State2Json(oJson);
   pushOutMessage(oJson, "s3200-state");
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

//***************************************************************************
// Initialize
//***************************************************************************

int P4d::initialize(bool truncate)
{
   sem->p();

   tell(0, "opening %s", ttyDevice);

   if (serial->open(ttyDevice) != success)
   {
      sem->v();
      return fail;
   }

   if (request->check() != success)
   {
      tell(0, "request->check failed");
      serial->close();
      return fail;
   }

   updateTimeRangeData();

   if (!connection)
      return fail;

   // truncate config tables ?

   if (truncate)
   {
      tell(eloAlways, "Truncate configuration tables!");

      tableValueFacts->truncate();
      tableSchemaConf->truncate();
      tableMenu->truncate();
   }

   tell(eloAlways, "Requesting value facts from s 3200");
   initValueFacts();
   tell(eloAlways, "Update html schema configuration");
   updateSchemaConfTable();
   tell(eloAlways, "Requesting menu structure from s 3200");
   initMenu();

   serial->close();
   sem->v();

   return done;
}

//***************************************************************************
// Setup
//***************************************************************************

int P4d::setup()
{
   if (!connection)
      return fail;

   for (int f = selectAllValueFacts->find(); f; f = selectAllValueFacts->fetch())
   {
      char* res {nullptr};
      char buf[100+TB] {""};
      char oldState = tableValueFacts->getStrValue("STATE")[0];
      char state {oldState};

      printf("%s 0x%04lx '%s' (%s)",
             tableValueFacts->getStrValue("TYPE"),
             tableValueFacts->getIntValue("ADDRESS"),
             tableValueFacts->getStrValue("UNIT"),
             tableValueFacts->getStrValue("TITLE"));

      printf(" - aufzeichnen? (%s): ", oldState == 'A' ? "Y/n" : "y/N");

      if ((res = fgets(buf, 100, stdin)) && strlen(res) > 1)
         state = toupper(res[0]) == 'Y' ? 'A' : 'D';

      if (state != oldState && tableValueFacts->find())
      {
         tableValueFacts->setCharValue("STATE", state);
         tableValueFacts->store();
      }
   }

   selectAllValueFacts->freeResult();

   tell(eloAlways, "Update html schema configuration");
   updateSchemaConfTable();

   return done;
}

//***************************************************************************
// Initialize Menu Structure
//***************************************************************************

int P4d::initMenu(bool updateParameters)
{
   int status;
   Fs::MenuItem m;
   int count = 0;

   // check serial communication

   if (request->check() != success)
   {
      serial->close();
      return fail;
   }

   tableMenu->truncate();
   tableMenu->clear();

   // ...

   for (status = request->getFirstMenuItem(&m); status != Fs::wrnLast && !doShutDown();
        status = request->getNextMenuItem(&m))
   {
      if (status == wrnSkip)
         continue;

      if (status != success)
         break;

      tell(eloDebug, "%3d) Address: 0x%4x, parent: 0x%4x, child: 0x%4x; '%s'",
           count++, m.parent, m.address, m.child, m.description);

      // update table

      tableMenu->clear();

      tableMenu->setValue("STATE", "D");
      tableMenu->setValue("UNIT", m.type == mstAnlOut && isEmpty(m.unit) ? "%" : m.unit);

      tableMenu->setValue("PARENT", m.parent);
      tableMenu->setValue("CHILD", m.child);
      tableMenu->setValue("ADDRESS", m.address);
      tableMenu->setValue("TITLE", m.description);

      tableMenu->setValue("TYPE", m.type);
      tableMenu->setValue("UNKNOWN1", m.unknown1);
      tableMenu->setValue("UNKNOWN2", m.unknown2);

      tableMenu->insert();

      count++;
   }

   tell(eloAlways, "Read %d menu items", count);

   if (updateParameters)
   {
      count = 0;
      tell(eloAlways, "Update menu parameters");
      tableMenu->clear();

      for (int f = selectAllMenuItems->find(); f; f = selectAllMenuItems->fetch())
      {
         updateParameter(tableMenu);
         count++;
      }

      tell(eloAlways, "Updated %d menu parameters", count);
   }

   return success;
}

//***************************************************************************
// Update Parameter
//***************************************************************************

int P4d::updateParameter(cDbTable* tableMenu)
{
   int type = tableMenu->getIntValue("TYPE");
   int paddr = tableMenu->getIntValue("ADDRESS");
   int child = tableMenu->getIntValue("CHILD");

   tell(3, "Update parameter %d/%d ...", type, paddr);

   sem->p();

   if (type == mstFirmware)
   {
      Fs::Status s;

      if (request->getStatus(&s) == success)
      {
         if (tableMenu->find())
         {
            tableMenu->setValue("VALUE", s.version);
            tableMenu->setValue("UNIT", "");
            tableMenu->update();
         }
      }
   }

   else if (type == mstDigOut || type == mstDigIn || type == mstAnlOut)
   {
      int status;
      Fs::IoValue v(paddr);

      if (type == mstDigOut)
         status = request->getDigitalOut(&v);
      else if (type == mstDigIn)
         status = request->getDigitalIn(&v);
      else
         status = request->getAnalogOut(&v);

      if (status == success)
      {
         char* buf = 0;

         if (type == mstAnlOut)
         {
            if (v.mode == 0xff)
               asprintf(&buf, "%d (A)", v.state);
            else
               asprintf(&buf, "%d (%d)", v.state, v.mode);
         }
         else
            asprintf(&buf, "%s (%c)", v.state ? "on" : "off", v.mode);

         if (tableMenu->find())
         {
            tableMenu->setValue("VALUE", buf);
            tableMenu->setValue("UNIT", "");
            tableMenu->update();
         }

         free(buf);
      }
   }

   else if (type == mstMesswert || type == mstMesswert1)
   {
      int status;
      Fs::Value v(paddr);

      tableValueFacts->clear();
      tableValueFacts->setValue("TYPE", "VA");
      tableValueFacts->setValue("ADDRESS", paddr);

      if (tableValueFacts->find())
      {
         double factor = tableValueFacts->getIntValue("FACTOR");
         const char* unit = tableValueFacts->getStrValue("UNIT");
         int dataType = tableValueFacts->getIntValue("SUBTYPE");

         status = request->getValue(&v);

         if (status == success)
         {
            char* buf = 0;
            int value = dataType == 1 ? (word)v.value : (sword)v.value;

            asprintf(&buf, "%.2f", value / factor);

            if (tableMenu->find())
            {
               tableMenu->setValue("VALUE", buf);
               tableMenu->setValue("UNIT", strcmp(unit, "°") == 0 ? "°C" : unit);
               tableMenu->update();
            }

            free(buf);
         }
      }
   }
   else if (isGroup(type) || type == mstBusValues || type == mstReset || type == mstEmpty)
   {
      // nothing to do
   }
   else if (child)
   {
      // I have childs -> I have no value -> nothing to do
   }
   else if (paddr == 0 && type != mstPar)
   {
      // address 0 only for type mstPar
   }
   else if (paddr == 9997 || paddr == 9998 || paddr == 9999)
   {
      // this 3 'special' addresses takes a long while and don't deliver any usefull data
   }
   else
   {
      Fs::ConfigParameter p(paddr);

      if (request->getParameter(&p) == success)
      {
         cRetBuf value = p.toNice(type);

         if (tableMenu->find())
         {
            tableMenu->setValue("VALUE", value);
            tableMenu->setValue("UNIT", strcmp(p.unit, "°") == 0 ? "°C" : p.unit);
            tableMenu->setValue("DIGITS", p.digits);
            tableMenu->setValue("MIN", p.rMin);
            tableMenu->setValue("MAX", p.rMax);
            tableMenu->setValue("DEF", p.rDefault);
            tableMenu->setValue("FACTOR", p.getFactor());
            tableMenu->setValue("PUB1", p.ub1);
            tableMenu->setValue("PUB2", p.ub2);
            tableMenu->setValue("PUB3", p.ub3);
            tableMenu->setValue("PUW1", p.uw1);
            tableMenu->update();
         }
      }
   }

   sem->v();

   return done;
}

//***************************************************************************
// Update Time Range Data
//***************************************************************************

int P4d::updateTimeRangeData()
{
   Fs::TimeRanges t;
   int status;
   char fName[10+TB];
   char tName[10+TB];

   tell(0, "Updating time ranges data ...");

   if (request->check() != success)
   {
      tell(0, "request->check failed");
      serial->close();
      return fail;
   }

   // update / insert time ranges

   for (status = request->getFirstTimeRanges(&t); status == success; status = request->getNextTimeRanges(&t))
   {
      tableTimeRanges->clear();
      tableTimeRanges->setValue("ADDRESS", t.address);

      for (int n = 0; n < 4; n++)
      {
         sprintf(fName, "FROM%d", n+1);
         sprintf(tName, "TO%d", n+1);
         tableTimeRanges->setValue(fName, t.getTimeRangeFrom(n));
         tableTimeRanges->setValue(tName, t.getTimeRangeTo(n));
      }

      tableTimeRanges->store();
      tableTimeRanges->reset();

   }

   tell(0, "Updating time ranges data done");

   return done;
}

//***************************************************************************
// Update Errors
//***************************************************************************

int P4d::updateErrors()
{
   int status;
   Fs::ErrorInfo e;
   char timeField[5+TB] = "";
   time_t timeOne = 0;
   cTimeMs timeMs;

   cDbStatement* select = new cDbStatement(tableErrors);
   select->build("select ");
   select->bindAllOut();
   select->build(" from %s where ", tableErrors->TableName());
   select->bind("NUMBER", cDBS::bndIn | cDBS::bndSet);
   select->bind("TIME1", cDBS::bndIn | cDBS::bndSet, " and ");

   if (select->prepare() != success)
   {
      tell(eloAlways, "prepare failed!");
      return fail;
   }

   tell(eloAlways, "Updating error list");

   sem->p();

   for (status = request->getFirstError(&e); status == success; status = request->getNextError(&e))
   {
      int insert = yes;

      sprintf(timeField, "TIME%d", e.state);

      tell(eloDebug2, "Debug: S-3200 error-message %d / %d '%s' '%s' %d [%s]; (for %s)",
           e.number, e.state, l2pTime(e.time).c_str(),  Fs::errState2Text(e.state), e.info, e.text,
           timeField);

      if (e.state == 1)
         timeOne = e.time;

      if (!timeOne)
         continue;

      if (timeOne)
      {
         tableErrors->clear();
         tableErrors->setValue("NUMBER", e.number);
         tableErrors->setValue("TIME1", timeOne);

         insert = !select->find();
      }

      tableErrors->clearChanged();

      if (insert
          || (e.state == 2 && !tableErrors->hasValue("STATE", Fs::errState2Text(2)))
          || (e.state == 4 && tableErrors->hasValue("STATE", Fs::errState2Text(1))))
      {
         tableErrors->setValue(timeField, e.time);
         tableErrors->setValue("STATE", Fs::errState2Text(e.state));
         tableErrors->setValue("NUMBER", e.number);
         tableErrors->setValue("INFO", e.info);
         tableErrors->setValue("TEXT", e.text);
      }

      if (insert)
         tableErrors->insert();
      else if (tableErrors->getChanges())
         tableErrors->update();

      if (e.state == 2)
         timeOne = 0;
   }

   sem->v();
   delete select;

   tell(eloAlways, "Updating error list done in %" PRIu64 "ms", timeMs.Elapsed());

   // count pending (not 'quittiert' AND not mailed) errors

   tableErrors->clear();
   selectPendingErrors->find();
   errorsPending = selectPendingErrors->getResultCount();
   selectPendingErrors->freeResult();

   tell(eloDetail, "Info: Found (%d) pending errors", errorsPending);

   return success;
}

//***************************************************************************
// Send Error Mail
//***************************************************************************

int P4d::sendErrorMail()
{
   std::string body = "";
   const char* subject = "Heizung: STÖRUNG";
   char* html = 0;

   // check

   if (isEmpty(mailScript) || isEmpty(errorMailTo))
      return done;

   // build mail ..

   for (int f = selectPendingErrors->find(); f; f = selectPendingErrors->fetch())
   {
      char* line = 0;
      time_t t = std::max(std::max(tableErrors->getTimeValue("TIME1"), tableErrors->getTimeValue("TIME4")), tableErrors->getTimeValue("TIME2"));

      asprintf(&line, "        <tr><td>%s</td><td>%s</td><td>%s</td></tr>\n",
               l2pTime(t).c_str(), tableErrors->getStrValue("TEXT"), tableErrors->getStrValue("STATE"));

      body += line;

      tell(eloDebug, "Debug: MAILCNT is (%ld), setting to (%ld)",
           tableErrors->getIntValue("MAILCNT"), tableErrors->getIntValue("MAILCNT")+1);
      tableErrors->find();
      tableErrors->setValue("MAILCNT", tableErrors->getIntValue("MAILCNT")+1);
      tableErrors->update();

      free(line);
   }

   selectPendingErrors->freeResult();

   // HTML mail

   loadHtmlHeader();

   asprintf(&html,
            "<html>\n"
            " %s\n"
            "  <body>\n"
            "   <font face=\"Arial\"><br/>WEB Interface: <a href=\"%s\">S 3200</a><br/></font>\n"
            "   <br/>\n"
            "Aktueller Status: %s"
            "   <br/>\n"
            "   <table>\n"
            "     <thead>\n"
            "       <tr class=\"head\">\n"
            "         <th><font>Zeit</font></th>\n"
            "         <th><font>Fehler</font></th>\n"
            "         <th><font>Status</font></th>\n"
            "       </tr>\n"
            "     </thead>\n"
            "     <tbody>\n"
            "%s"
            "     </tbody>\n"
            "   </table>\n"
            "   <br/>\n"
            "  </body>\n"
            "</html>\n",
            htmlHeader.memory, webUrl, currentState.stateinfo, body.c_str());

   int result = sendMail(errorMailTo, subject, html, "text/html");

   free(html);

   return result;
}

//***************************************************************************
// Calc State Duration
//***************************************************************************

int P4d::calcStateDuration()
{
   time_t beginTime {0};
   int thisState = {-1};
   std::string text {""};
   std::string kStates {""};

   for (auto& s : stateDurations)
   {
      s.second = 0;
      kStates += ":" + std::to_string(s.first);
   }

   if (knownStates != kStates)
   {
      setConfigItem("knownStates", kStates.c_str());
      getConfigItem("knownStates", knownStates, "");
   }

   tableSamples->clear();
   tableSamples->setValue("TIME", beginTime);
   tableSamples->setValue("VALUE", (double)thisState);

   while (selectStateDuration->find())
   {
      time_t eTime {time(0)};

      if (!endTime.isNull())
         eTime = endTime.getTimeValue();

      if (beginTime)
      {
         stateDurations[thisState] += eTime-beginTime;
         tell(3, "%s:0x%02x (%s) '%d/%s' %.2f minutes", "SD", thisState,
              l2pTime(beginTime).c_str(), thisState, text.c_str(), (eTime-beginTime) / 60.0);
      }

      if (endTime.isNull())
         break;

      thisState = tableSamples->getFloatValue("VALUE");
      text = tableSamples->getStrValue("TEXT");
      beginTime = eTime;

      addValueFact(thisState, "SD", 1, ("State_Duration_"+std::to_string(thisState)).c_str(),
                   "min", wtChart, (std::string(text)+" (Laufzeit/Tag)").c_str(), false, 2000);

      selectStateDuration->freeResult();
      tableSamples->clear();
      tableSamples->setValue("TIME", beginTime);
      tableSamples->setValue("VALUE", (double)thisState);
   }

   selectStateDuration->freeResult();

   if (loglevel >= 2)
   {
      int total {0};

      for (const auto& d : stateDurations)
      {
         tell(2, "%d: %ld minutes", d.first, d.second / 60);
         total += d.second;
      }

      tell(2, "total: %d minutes", total / 60);
   }

   return success;
}

//***************************************************************************
// Sensor Alert Check
//***************************************************************************

void P4d::sensorAlertCheck(time_t now)
{
   tableSensorAlert->clear();
   tableSensorAlert->setValue("KIND", "M");

   // iterate over all alert roules ..

   for (int f = selectSensorAlerts->find(); f; f = selectSensorAlerts->fetch())
   {
      alertMailBody = "";
      alertMailSubject = "";

      performAlertCheck(tableSensorAlert->getRow(), now, 0);
   }

   selectSensorAlerts->freeResult();
}

//***************************************************************************
// Perform Login
//***************************************************************************

int P4d::performLogin(json_t* oObject)
{
   Daemon::performLogin(oObject);

   long client = getLongFromJson(oObject, "client");

   json_t* oJson = json_object();
   s3200State2Json(oJson);
   pushOutMessage(oJson, "s3200-state", client);

   return done;
}

//***************************************************************************
// Perform Alert Check
//***************************************************************************

int P4d::performAlertCheck(cDbRow* alertRow, time_t now, int recurse, int force)
{
   int alert = 0;

   // data from alert row

   int addr = alertRow->getIntValue("ADDRESS");
   const char* type = alertRow->getStrValue("TYPE");

   int id = alertRow->getIntValue("ID");
   int lgop = alertRow->getIntValue("LGOP");
   time_t lastAlert = alertRow->getIntValue("LASTALERT");
   int maxRepeat = alertRow->getIntValue("MAXREPEAT");

   int minIsNull = alertRow->getValue("MIN")->isNull();
   int maxIsNull = alertRow->getValue("MAX")->isNull();
   int min = alertRow->getIntValue("MIN");
   int max = alertRow->getIntValue("MAX");

   int range = alertRow->getIntValue("RANGEM");
   int delta = alertRow->getIntValue("DELTA");

   // lookup value facts

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", addr);
   tableValueFacts->setValue("TYPE", type);

   // lookup samples

   tableSamples->clear();
   tableSamples->setValue("ADDRESS", addr);
   tableSamples->setValue("TYPE", type);
   tableSamples->setValue("AGGREGATE", "S");
   tableSamples->setValue("TIME", now);

   if (!tableSamples->find() || !tableValueFacts->find())
   {
      tell(eloAlways, "Info: Can't perform sensor check for %s/%d '%s'", type, addr, l2pTime(now).c_str());
      return 0;
   }

   // data from samples and value facts

   double value = tableSamples->getFloatValue("VALUE");

   const char* title = tableValueFacts->getStrValue("TITLE");
   const char* unit = tableValueFacts->getStrValue("UNIT");

   // -------------------------------
   // check min / max threshold

   if (!minIsNull || !maxIsNull)
   {
      if (force || (!minIsNull && value < min) || (!maxIsNull && value > max))
      {
         tell(eloAlways, "%d) Alert for sensor %s/0x%x, value %.2f not in range (%d - %d)",
              id, type, addr, value, min, max);

         // max one alert mail per maxRepeat [minutes]

         if (force || !lastAlert || lastAlert < time(0)- maxRepeat * tmeSecondsPerMinute)
         {
            alert = 1;
            add2AlertMail(alertRow, title, value, unit);
         }
      }
   }

   // -------------------------------
   // check range delta

   if (range && delta)
   {
      // select value of this sensor around 'time = (now - range)'

      time_t rangeStartAt = time(0) - range*tmeSecondsPerMinute;
      time_t rangeEndAt = rangeStartAt + interval;

      tableSamples->clear();
      tableSamples->setValue("ADDRESS", addr);
      tableSamples->setValue("TYPE", type);
      tableSamples->setValue("AGGREGATE", "S");
      tableSamples->setValue("TIME", rangeStartAt);
      rangeEnd.setValue(rangeEndAt);

      if (selectSampleInRange->find())
      {
         double oldValue = tableSamples->getFloatValue("VALUE");

         if (force || labs(value - oldValue) > delta)
         {
            tell(eloAlways, "%d) Alert for sensor %s/0x%x , value %.2f changed more than %d in %d minutes",
                 id, type, addr, value, delta, range);

            // max one alert mail per maxRepeat [minutes]

            if (force || !lastAlert || lastAlert < time(0)- maxRepeat * tmeSecondsPerMinute)
            {
               alert = 1;
               add2AlertMail(alertRow, title, value, unit);
            }
         }
      }

      selectSampleInRange->freeResult();
      tableSamples->reset();
   }

   // ---------------------------
   // Check sub rules recursive

   if (alertRow->getIntValue("SUBID") > 0)
   {
      if (recurse > 50)
      {
         tell(eloAlways, "Info: Aborting recursion after 50 steps, seems to be a config error!");
      }
      else
      {
         tableSensorAlert->clear();
         tableSensorAlert->setValue("ID", alertRow->getIntValue("SUBID"));

         if (tableSensorAlert->find())
         {
            int sAlert = performAlertCheck(tableSensorAlert->getRow(), now, recurse+1);

            switch (lgop)
            {
               case loAnd:    alert = alert &&  sAlert; break;
               case loOr:     alert = alert ||  sAlert; break;
               case loAndNot: alert = alert && !sAlert; break;
               case loOrNot:  alert = alert || !sAlert; break;
            }
         }
      }
   }

   // ---------------------------------
   // update master row and send mail

   if (alert && !recurse)
   {
      tableSensorAlert->clear();
      tableSensorAlert->setValue("ID", id);

      if (tableSensorAlert->find())
      {
         if (!force)
         {
            tableSensorAlert->setValue("LASTALERT", time(0));
            tableSensorAlert->update();
         }

         sendAlertMail(tableSensorAlert->getStrValue("MADDRESS"));
      }
   }

   return alert;
}

//***************************************************************************
// Send Mail
//***************************************************************************

int P4d::sendAlertMail(const char* to)
{
   // check

   if (isEmpty(to) || isEmpty(mailScript))
      return done;

   if (alertMailBody.empty())
      alertMailBody = "- undefined -";

   char* html = 0;

   alertMailBody = strReplace("\n", "<br/>\n", alertMailBody);

   const char* htmlHead =
      "<head>\n"
      "  <style type=\"text/css\">\n"
      "    caption { background: #095BA6; font-family: Arial Narrow; color: #fff; font-size: 18px; }\n"
      "  </style>\n"
      "</head>\n";

   asprintf(&html,
            "<html>\n"
            " %s"
            " <body>\n"
            "  %s\n"
            " </body>\n"
            "</html>\n",
            htmlHead, alertMailBody.c_str());

   alertMailBody = html;
   free(html);

   // send mail

   return sendMail(to, alertMailSubject.c_str(), alertMailBody.c_str(), "text/html");
}

//***************************************************************************
// Send Mail
//***************************************************************************

int P4d::add2AlertMail(cDbRow* alertRow, const char* title, double value, const char* unit)
{
   char* sensor = 0;

   std::string subject = alertRow->getStrValue("MSUBJECT");
   std::string body = alertRow->getStrValue("MBODY");
   int addr = alertRow->getIntValue("ADDRESS");
   const char* type = alertRow->getStrValue("TYPE");

   int min = alertRow->getIntValue("MIN");
   int max = alertRow->getIntValue("MAX");
   int range = alertRow->getIntValue("RANGEM");
   int delta = alertRow->getIntValue("DELTA");
   int maxRepeat = alertRow->getIntValue("MAXREPEAT");
   double minv {0};
   double maxv {0};

   tablePeaks->clear();
   tablePeaks->setValue("ADDRESS", alertRow->getIntValue("ADDRESS"));
   tablePeaks->setValue("TYPE", alertRow->getStrValue("TYPE"));

   if (tablePeaks->find())
   {
      minv = tablePeaks->getFloatValue("MIN");
      maxv = tablePeaks->getFloatValue("MAX");
   }

   tablePeaks->reset();

   if (!body.length())
      body = "- undefined -";

   // prepare

   asprintf(&sensor, "%s/0x%x", type, addr);

   // templating

   subject = strReplace("%sensorid%", sensor, subject);
   subject = strReplace("%value%", value, subject);
   subject = strReplace("%unit%", unit, subject);
   subject = strReplace("%title%", title, subject);
   subject = strReplace("%min%", (long)min, subject);
   subject = strReplace("%max%", (long)max, subject);
   subject = strReplace("%range%", (long)range, subject);
   subject = strReplace("%delta%", (long)delta, subject);
   subject = strReplace("%time%", l2pTime(time(0)).c_str(), subject);
   subject = strReplace("%repeat%", (long)maxRepeat, subject);
   subject = strReplace("%weburl%", webUrl, subject);
   subject = strReplace("%minv%", minv, subject);
   subject = strReplace("%maxv%", maxv, subject);

   body = strReplace("%sensorid%", sensor, body);
   body = strReplace("%value%", value, body);
   body = strReplace("%unit%", unit, body);
   body = strReplace("%title%", title, body);
   body = strReplace("%min%", (long)min, body);
   body = strReplace("%max%", (long)max, body);
   body = strReplace("%range%", (long)range, body);
   body = strReplace("%delta%", (long)delta, body);
   body = strReplace("%time%", l2pTime(time(0)).c_str(), body);
   body = strReplace("%repeat%", (long)maxRepeat, body);
   body = strReplace("%weburl%", webUrl, body);
   body = strReplace("%minv%", minv, body);
   body = strReplace("%maxv%", maxv, body);

   alertMailSubject += std::string(" ") + subject;
   alertMailBody += std::string("\n") + body;

   free(sensor);

   return success;
}

//***************************************************************************
// Dispatch Special Request
//***************************************************************************

int P4d::dispatchSpecialRequest(Event event, json_t* oObject, long client)
{
   int status {fail};

   switch (event)
   {
      case evInitTables:        status = performInitTables(oObject, client);       break;
      case evUpdateTimeRanges:  status = performUpdateTimeRanges(oObject, client); break;
      case evPellets:           status = performPellets(oObject, client);          break;
      case evPelletsAdd:        status = performPelletsAdd(oObject, client);       break;
      case evStoreAlerts:       status = storeAlerts(oObject, client);             break;
      case evSchema:            status = performSchema(oObject, client);           break;
      case evStoreSchema:       status = storeSchema(oObject, client);             break;
      case evParEditRequest:    status = performParEditRequest(oObject, client);   break;
      case evParStore:          status = performParStore(oObject, client);         break;
      case evMenu:              status = performMenu(oObject, client);             break;
      case evAlerts:            status = performAlerts(oObject, client);           break;
      case evErrors:            status = performErrors(client);                    break;
      default:                  return ignore;
   }

   return status;
}

//***************************************************************************
// Perform Init Tables
//***************************************************************************

int P4d::performInitTables(json_t* oObject, long client)
{
   const char* action = getStringFromJson(oObject, "action");

   if (isEmpty(action))
      return replyResult(fail, "missing action", client);

   if (strcmp(action, "valuefacts") == 0)
   {
      initValueFacts();
      updateTimeRangeData();
   }
   else if (strcmp(action, "menu") == 0)
   {
      initMenu();
   }
   else if (strcmp(action, "menu-force") == 0)
   {
      initMenu(true);
   }

   return replyResult(success, "... init abgeschlossen!", client);
}

//***************************************************************************
// Perform Pellets Request
//***************************************************************************

int P4d::performPellets(json_t* oObject, long client)
{
   uint stokerHhLast {0};
   time_t timeLast {0};
   double tAmount {0.0};
   double tPrice {0.0};
   json_t* oJson = json_array();
   double consumptionHLast {0.0};

   tablePellets->clear();

   for (int f = selectAllPellets->find(); f; f = selectAllPellets->fetch())
   {
      json_t* oData = json_object();
      json_array_append_new(oJson, oData);

      json_object_set_new(oData, "id", json_integer(tablePellets->getIntValue("ID")));
      json_object_set_new(oData, "time", json_integer(tablePellets->getTimeValue("TIME")));
      json_object_set_new(oData, "amount", json_integer(tablePellets->getIntValue("AMOUNT")));
      json_object_set_new(oData, "price", json_real(tablePellets->getFloatValue("PRICE")));
      json_object_set_new(oData, "comment", json_string(tablePellets->getStrValue("COMMENT")));

      double amount = tablePellets->getIntValue("AMOUNT");
      tAmount += tablePellets->getIntValue("AMOUNT");
      tPrice += tablePellets->getFloatValue("PRICE");

      // ...

      minValue.clear();
      tableSamples->clear();
      tableSamples->setValue("TYPE", "VA");
      tableSamples->setValue("ADDRESS", 0xad);
      tableSamples->setValue("TIME", tablePellets->getTimeValue("TIME"));

      if (!selectStokerHours->find())
      {
         tell(eloAlways, "Info: Sample for stoker hours not found!");
         continue;
      }

      if (timeLast)
      {
         uint durationDays = (tablePellets->getTimeValue("TIME") - timeLast) / tmeSecondsPerDay;
         json_object_set_new(oData, "duration", json_integer(durationDays));
      }

      uint stokerHh = minValue.getIntValue();   // is bound as int !!

      tell(eloAlways, "Getankt: '%s' stokerH %d at '%s'",
           l2pTime(tablePellets->getTimeValue("TIME")).c_str(),
           stokerHh, l2pTime(tableSamples->getTimeValue("TIME")).c_str());

      if (stokerHhLast && stokerHh-stokerHhLast > 0)
      {
         tell(eloAlways, "stokerHh delta => %d", stokerHh - stokerHhLast);
         json_object_set_new(oData, "stokerHours", json_integer(stokerHh - stokerHhLast));
         consumptionHLast = amount/(stokerHh-stokerHhLast);
         json_object_set_new(oData, "consumptionH", json_real(consumptionHLast));
      }

      timeLast = tablePellets->getTimeValue("TIME");
      stokerHhLast = stokerHh;
   }

   selectAllPellets->freeResult();

   double consumptionH = consumptionPerHour ? consumptionPerHour : consumptionHLast;
   char* hint;
   asprintf(&hint, "consumption sice tankering by %.2f kg / stoker hour", consumptionH);
   tell(eloAlways, "Calculating %s", hint);

   json_t* oData = json_object();
   json_array_append_new(oJson, oData);

   json_object_set_new(oData, "id", json_integer(-1));
   json_object_set_new(oData, "sum", json_boolean(true));
   json_object_set_new(oData, "time", json_integer(time(0)));
   json_object_set_new(oData, "amount", json_integer(tAmount));
   json_object_set_new(oData, "price", json_real(tPrice));
   json_object_set_new(oData, "comment", json_string("Total"));
   json_object_set_new(oData, "consumptionHint", json_string(hint));
   json_object_set_new(oData, "stokerHours", json_integer(vaSensors[0xad].value - stokerHhLast));
   json_object_set_new(oData, "consumptionDelta", json_integer(consumptionH * (vaSensors[0xad].value-stokerHhLast)));

   free(hint);

   return pushOutMessage(oJson, "pellets", client);
}

//***************************************************************************
// Perform Pellets Add Entry
//***************************************************************************

int P4d::performPelletsAdd(json_t* oObject, long client)
{
   int id = getIntFromJson(oObject, "id", -1);
   bool del = getBoolFromJson(oObject, "delete", false);

   tablePellets->clear();
   tablePellets->setValue("ID", id);

   if (del)
   {
      if (!tablePellets->find())
         return replyResult(success, "failed", client);

      tablePellets->deleteWhere("id = %d", id);
      performPellets(0, client);
      return replyResult(success, "Eintrag gelöscht", client);
   }

   if (id >= 0 && !tablePellets->find())
      return replyResult(success, "failed", client);

   tablePellets->setValue("TIME", getLongFromJson(oObject, "time"));
   tablePellets->setValue("AMOUNT", getIntFromJson(oObject, "amount"));
   tablePellets->setValue("PRICE", getDoubleFromJson(oObject, "price"));
   tablePellets->setValue("COMMENT", getStringFromJson(oObject, "comment"));
   tablePellets->store();

   performPellets(0, client);

   return replyResult(success, "Eintrag gespeichert", client);
}

//***************************************************************************
// Perform WS Error Data Request
//***************************************************************************

int P4d::performErrors(long client)
{
   if (client == 0)
      return done;

   json_t* oJson = json_array();

   tableErrors->clear();

   for (int f = selectAllErrors->find(); f; f = selectAllErrors->fetch())
   {
      json_t* oData = json_object();
      json_array_append_new(oJson, oData);

      time_t t = std::max(std::max(tableErrors->getTimeValue("TIME1"), tableErrors->getTimeValue("TIME4")), tableErrors->getTimeValue("TIME2"));
      std::string strTime = l2pTime(t);
      uint duration {0};

      if (tableErrors->getValue("TIME2")->isNull())
         duration = time(0) - tableErrors->getTimeValue("TIME1");
      else
         duration = tableErrors->getTimeValue("TIME2") - tableErrors->getTimeValue("TIME1");

      json_object_set_new(oData, "state", json_string(tableErrors->getStrValue("STATE")));
      json_object_set_new(oData, "text", json_string(tableErrors->getStrValue("TEXT")));
      json_object_set_new(oData, "duration", json_integer(duration));
      json_object_set_new(oData, "time", json_string(strTime.c_str()));
   }

   selectAllErrors->freeResult();

   pushOutMessage(oJson, "errors", client);

   return done;
}
int P4d::performAlertTestMail(int id, long client)
{
   tell(eloDetail, "Test mail for alert (%d) requested", id);

   if (isEmpty(mailScript))
      return replyResult(fail, "missing mail script", client);

   if (!fileExists(mailScript))
      return replyResult(fail, "mail script not found", client);

   if (!selectMaxTime->find())
      tell(eloAlways, "Warning: Got no result by 'select max(time) from samples'");

   time_t  last = tableSamples->getTimeValue("TIME");
   selectMaxTime->freeResult();

   tableSensorAlert->clear();
   tableSensorAlert->setValue("ID", id);

   if (!tableSensorAlert->find())
      return replyResult(fail, "requested alert ID not found", client);

   alertMailBody = "";
   alertMailSubject = "";

   if (!performAlertCheck(tableSensorAlert->getRow(), last, 0, yes/*force*/))
      return replyResult(fail, "send failed", client);

   tableSensorAlert->reset();

   return replyResult(success, "mail sended", client);
}

//***************************************************************************
// Perform WS Parameter Edit Request
//***************************************************************************

int P4d::performParEditRequest(json_t* oObject, long client)
{
   if (client == 0)
      return done;

   int id = getIntFromJson(oObject, "id", na);
   int parent = getIntFromJson(oObject, "parent", 1);

   if (id == 0)
      return performTimeParEditRequest(oObject, client);

   tableMenu->clear();
   tableMenu->setValue("ID", id);

   if (!tableMenu->find())
   {
      tell(0, "Info: Id %d for 'pareditrequest' not found!", id);
      return fail;
   }

   int type = tableMenu->getIntValue("TYPE");
   unsigned int address = tableMenu->getIntValue("ADDRESS");
   const char* title = tableMenu->getStrValue("TITLE");

   tableMenu->reset();
   sem->p();

   ConfigParameter p(address);

   if (request->getParameter(&p) == success)
   {
      cRetBuf value = p.toNice(type);

      json_t* oJson = json_object();
      json_object_set_new(oJson, "id", json_integer(id));
      json_object_set_new(oJson, "type", json_integer(type));
      json_object_set_new(oJson, "address", json_integer(address));
      json_object_set_new(oJson, "title", json_string(title));
      json_object_set_new(oJson, "unit", json_string(type == mstParZeit ? "Uhr" : p.unit));
      json_object_set_new(oJson, "value", json_string(value));
      json_object_set_new(oJson, "def", json_integer(p.rDefault));
      json_object_set_new(oJson, "min", json_integer(p.rMin));
      json_object_set_new(oJson, "max", json_integer(p.rMax));
      json_object_set_new(oJson, "digits", json_integer(p.digits));
      json_object_set_new(oJson, "parent", json_integer(parent));

      pushOutMessage(oJson, "pareditrequest", client);
   }

   sem->v();

   return done;
}

int P4d::performTimeParEditRequest(json_t* oObject, long client)
{
   if (client == 0)
      return done;

   int trAddr = getIntFromJson(oObject, "address", na);
   int range = getIntFromJson(oObject, "range", na);
   int parent = getIntFromJson(oObject, "parent", 1);

   tableTimeRanges->clear();
   tableTimeRanges->setValue("ADDRESS", trAddr);

   if (!tableTimeRanges->find())
   {
      tell(0, "Info: Address %d or 'pareditrequest' in timetanges not found!", trAddr);
      return fail;
   }

   char* rTitle {nullptr};
   char* value {nullptr};
   char* from {nullptr};
   char* to {nullptr};

   asprintf(&rTitle, "Range %d", range);
   asprintf(&from, "from%d", range);
   asprintf(&to, "to%d", range);

   if (!tableTimeRanges->hasValue(from, "nn:nn") && !tableTimeRanges->hasValue(to, "nn:nn"))
      asprintf(&value, "%s - %s", tableTimeRanges->getStrValue(from), tableTimeRanges->getStrValue(to));
   else
      asprintf(&value, "%s", "-");

   json_t* oJson = json_object();
   json_object_set_new(oJson, "id", json_integer(0));
   json_object_set_new(oJson, "type", json_integer(mstParZeit));
   json_object_set_new(oJson, "address", json_integer(trAddr));
   json_object_set_new(oJson, "range", json_integer(range));
   json_object_set_new(oJson, "title", json_string(rTitle));
   json_object_set_new(oJson, "unit", json_string("Uhr"));
   json_object_set_new(oJson, "value", json_string(value));
   json_object_set_new(oJson, "parent", json_integer(parent));

   pushOutMessage(oJson, "pareditrequest", client);

   return done;
}

//***************************************************************************
// Perform WS Parameter Store
//***************************************************************************

int P4d::performParStore(json_t* oObject, long client)
{
   if (client == 0)
      return done;

   int id = getIntFromJson(oObject, "id", na);

   if (id == 0)
      return performTimeParStore(oObject, client);

   json_t* oJson = json_object();
   const char* value = getStringFromJson(oObject, "value");

   tableMenu->clear();
   tableMenu->setValue("ID", id);

   if (!tableMenu->find())
   {
      replyResult(fail, "Parameter not found", client);
      tell(0, "Info: Id %d for 'parstore' not found!", id);
      return done;
   }

   int parent = tableMenu->getIntValue("PARENT");
   int type = tableMenu->getIntValue("TYPE");
   unsigned int address = tableMenu->getIntValue("ADDRESS");
   ConfigParameter p(address);

   sem->p();
   request->getParameter(&p);
   sem->v();

   if (p.setValue(type, value) != success)
   {
      tableMenu->reset();
      tell(eloAlways, "Set of parameter failed, wrong format");
      return replyResult(fail, "Value format error", client);
   }

   {
      int status {fail};
      tell(eloAlways, "Storing value '%s/%s' for parameter at address 0x%x", value, p.toNice(type).string(), address);
      sem->p();

      if ((status = request->setParameter(&p)) == success)
      {
         tableMenu->setValue("VALUE", p.toNice(type));
         tableMenu->setValue("UNIT", p.unit);
         tableMenu->update();
         json_object_set_new(oJson, "parent", json_integer(parent));
         sem->v();

         replyResult(status, "Parameter gespeichert", client);
         return performMenu(oJson, client);
      }

      sem->v();
      tell(eloAlways, "Set of parameter failed, error %d", status);

      if (status == P4Request::wrnNonUpdate)
         replyResult(status, "Value identical, ignoring request", client);
      else if (status == P4Request::wrnOutOfRange)
         replyResult(status, "Value out of range", client);
      else
         replyResult(status, "Serial communication error", client);
   }

   tableMenu->reset();

   return done;
}

int P4d::performTimeParStore(json_t* oObject, long client)
{
   int status {success};
   int trAddr = getIntFromJson(oObject, "address", na);
   int range = getIntFromJson(oObject, "range", na);
   int parent = getIntFromJson(oObject, "parent", 1);
   const char* value = getStringFromJson(oObject, "value");

   tableTimeRanges->clear();
   tableTimeRanges->setValue("ADDRESS", trAddr);

   if (!tableTimeRanges->find() || range < 1)
   {
      replyResult(fail, "Parameter not found", client);
      tell(0, "Info: Address %d for 'parstore' not found!", trAddr);
      return done;
   }

   Fs::TimeRanges t(trAddr);
   char fName[10+TB];
   char tName[10+TB];
   char valueFrom[100+TB];
   char valueTo[100+TB];

   // parse range and value from data

   if (isEmpty(value))
      value = "nn:nn - nn:nn";

   if (sscanf(value, "%[^-]-%[^-]", valueFrom, valueTo) != 2)
   {
      replyResult(fail, "Parsing of value failed, wrong format!", client);
      tell(eloAlways, "Parsing of value '%s' failed, wrong format!", value);
      return done;
   }

   allTrim(valueFrom);
   allTrim(valueTo);

   // update struct with time ranges from table

   for (int n = 0; n < 4; n++)
   {
      sprintf(fName, "FROM%d", n+1);
      sprintf(tName, "TO%d", n+1);

      status += t.setTimeRange(n, tableTimeRanges->getStrValue(fName), tableTimeRanges->getStrValue(tName));
   }

   // update the changed range with new value

   status += t.setTimeRange(range-1, valueFrom, valueTo);

   if (status != success)
   {
      replyResult(fail, "Set of time range parameter failed, wrong format", client);
      tell(eloAlways, "Set of time range parameter failed, wrong format");
   }

   tell(4, "Value was: '%s'-'%s'", valueFrom, valueTo);
   tell(eloAlways, "Storing '%s' for time range '%d' of parameter 0x%x", t.getTimeRange(range-1), range, t.address);

   if (request->setTimeRanges(&t) != success)
   {
      replyResult(fail, "Set of time range parameter failed", client);
      tell(eloAlways, "Set of time range parameter failed, error %d", status);
   }

   // update time range table

   sprintf(fName, "FROM%d", range);
   sprintf(tName, "TO%d", range);
   tableTimeRanges->setValue(fName, t.getTimeRangeFrom(range-1));
   tableTimeRanges->setValue(tName, t.getTimeRangeTo(range-1));
   tableTimeRanges->update();

   replyResult(status, "Parameter gespeichert", client);

   json_t* oJson = json_object();
   json_object_set_new(oJson, "parent", json_integer(parent));

   return performMenu(oJson, client);
}

//***************************************************************************
// Perform WS Menu Request
//***************************************************************************

int P4d::performMenu(json_t* oObject, long client)
{
   if (client == 0)
      return done;

   int parent {1};
   int last {0};
   char* title {nullptr};

   if (oObject)
      parent = getIntFromJson(oObject, "parent", 1);

   json_t* oJson = json_object();
   json_t* oArray = json_array();

   tableMenu->clear();
   tableMenu->setValue("CHILD", parent);

   if (selectMenuItemsByChild->find())
   {
      last = tableMenu->getIntValue("PARENT");
      title = strdup(tableMenu->getStrValue("title"));
   }

   selectMenuItemsByChild->freeResult();

   tableMenu->clear();
   tableMenu->setValue("PARENT", parent);

   for (int f = selectMenuItemsByParent->find(); f; f = selectMenuItemsByParent->fetch())
   {
      int type = tableMenu->getIntValue("TYPE");
      int address = tableMenu->getIntValue("ADDRESS");
      int child = tableMenu->getIntValue("CHILD");
      int digits = tableMenu->getIntValue("DIGITS");
      char* title = strdup(tableMenu->getStrValue("TITLE"));

      if (isEmpty(rTrim(title)))
      {
         free(title);
         continue;
      }

      if (title[strlen(title)-1] == ':')
         title[strlen(title)-1] = '\0';

      bool timeGroup = (type == mstMenuChoice || type == mstMenu2) && strcmp(title, "Zeiten") == 0 && (child == 230 || child == 350 || child == 430 || child == 573);

      if (type == mstBusValues || type == mstReset)
         continue;

      // this 3 'special' addresses takes a long while and don't deliver any usefull data

      if (address == 9997 || address == 9998 || address == 9999)
         continue;

      updateParameter(tableMenu);

      if (!child && tableMenu->getValue("VALUE")->isNull())
         continue;

      if (!timeGroup)
      {
         json_t* oData = json_object();
         json_array_append_new(oArray, oData);

         json_object_set_new(oData, "id", json_integer(tableMenu->getIntValue("ID")));
         json_object_set_new(oData, "parent", json_integer(tableMenu->getIntValue("PARENT")));
         json_object_set_new(oData, "child", json_integer(child));
         json_object_set_new(oData, "type", json_integer(type));
         json_object_set_new(oData, "address", json_integer(address));
         json_object_set_new(oData, "title", json_string(title));
         json_object_set_new(oData, "unit", json_string(tableMenu->getStrValue("UNIT")));
         json_object_set_new(oData, "range", json_integer(na));
         json_object_set_new(oData, "parent", json_integer(parent));
         json_object_set_new(oData, "digits", json_integer(digits));

         if (type == mstMesswert || type == mstMesswert1)
            json_object_set_new(oData, "value", json_real(vaSensors[address].value));
         else
            json_object_set_new(oData, "value", json_string(tableMenu->getStrValue("VALUE")));

         if (type == mstPar || type == mstParSet || type == mstParSet1 || type == mstParSet2 ||
             type == mstParDig || type == mstParZeit)
            json_object_set_new(oData, "editable", json_boolean(true));
      }

      else
      {
         int baseAddr {0};

         switch (child)
         {
            case 230: baseAddr = 0x00 + (address * 7); break;   // Boiler 'n'
            case 350: baseAddr = 0x38 + (address * 7); break;   // Heizkreis 'n'
            case 430: baseAddr = 0xb6 + (address * 7); break;   // Puffer 'n'
         // case ???: baseAddr = 0xd2 + (address * 7); break;   // Kessel
            case 573: baseAddr = 0xd9 + (address * 7); break;   // Zirkulation
         }

         // updateTimeRangeData();

         for (int wday = 0; wday < 7; wday++)
         {
            int trAddr = baseAddr + wday;
            char* dayTitle {nullptr};
            asprintf(&dayTitle, "Zeiten '%s'", toWeekdayName(wday));

            tableTimeRanges->clear();
            tableTimeRanges->setValue("ADDRESS", trAddr);

            json_t* oData = json_object();
            json_array_append_new(oArray, oData);

            json_object_set_new(oData, "id", json_integer(0));
            json_object_set_new(oData, "parent", json_integer(0));
            json_object_set_new(oData, "child", json_integer(0));
            json_object_set_new(oData, "type", json_integer(0));
            json_object_set_new(oData, "address", json_integer(0));
            json_object_set_new(oData, "title", json_string(dayTitle));
            json_object_set_new(oData, "unit", json_string(""));
            json_object_set_new(oData, "parent", json_integer(parent));

            free(dayTitle);

            if (tableTimeRanges->find())
            {
               for (int n = 1; n < 5; n++)
               {
                  char* rTitle {nullptr};
                  char* value {nullptr};
                  char* from {nullptr};
                  char* to {nullptr};

                  asprintf(&rTitle, "Range %d", n);
                  asprintf(&from, "from%d", n);
                  asprintf(&to, "to%d", n);
                  asprintf(&value, "%s - %s", tableTimeRanges->getStrValue(from), tableTimeRanges->getStrValue(to));

                  json_t* oData = json_object();
                  json_array_append_new(oArray, oData);

                  json_object_set_new(oData, "id", json_integer(0));
                  json_object_set_new(oData, "range", json_integer(n));
                  json_object_set_new(oData, "parent", json_integer(0));
                  json_object_set_new(oData, "child", json_integer(0));
                  json_object_set_new(oData, "type", json_integer(0));
                  json_object_set_new(oData, "address", json_integer(trAddr));
                  json_object_set_new(oData, "title", json_string(rTitle));
                  json_object_set_new(oData, "unit", json_string(""));
                  json_object_set_new(oData, "value", json_string(value));
                  json_object_set_new(oData, "editable", json_boolean(true));
                  json_object_set_new(oData, "parent", json_integer(parent));

                  free(rTitle);
                  free(value);
                  free(from);
                  free(to);
               }

               tableTimeRanges->reset();
            }
         }
      }

      free(title);
   }

   selectMenuItemsByParent->freeResult();

   json_object_set_new(oJson, "items", oArray);
   json_object_set_new(oJson, "parent", json_integer(parent));
   json_object_set_new(oJson, "last", json_integer(last));
   json_object_set_new(oJson, "title", json_string(title));

   pushOutMessage(oJson, "menu", client);

   return done;
}

//***************************************************************************
// Perform WS Scehma Data
//***************************************************************************

int P4d::performSchema(json_t* oObject, long client)
{
   if (client == 0)
      return done;

   json_t* oArray = json_array();

   tableSchemaConf->clear();

   for (int f = selectAllSchemaConf->find(); f; f = selectAllSchemaConf->fetch())
   {
      tableValueFacts->clear();
      tableValueFacts->setValue("ADDRESS", tableSchemaConf->getIntValue("ADDRESS"));
      tableValueFacts->setValue("TYPE", tableSchemaConf->getStrValue("TYPE"));

      if (!tableSchemaConf->hasValue("TYPE", "UC") && (!tableValueFacts->find() || !tableValueFacts->hasValue("STATE", "A")))
         continue;

      json_t* oData = json_object();
      json_array_append_new(oArray, oData);

      addFieldToJson(oData, tableSchemaConf, "ADDRESS");
      addFieldToJson(oData, tableSchemaConf, "TYPE");
      addFieldToJson(oData, tableSchemaConf, "STATE");
      addFieldToJson(oData, tableSchemaConf, "KIND");
      addFieldToJson(oData, tableSchemaConf, "WIDTH");
      addFieldToJson(oData, tableSchemaConf, "HEIGHT");
      addFieldToJson(oData, tableSchemaConf, "SHOWUNIT");
      addFieldToJson(oData, tableSchemaConf, "SHOWTITLE");
      addFieldToJson(oData, tableSchemaConf, "USRTEXT");
      addFieldToJson(oData, tableSchemaConf, "FUNCTION", true, "fct");

      const char* properties = tableSchemaConf->getStrValue("PROPERTIES");
      if (isEmpty(properties))
         properties = "{}";
      json_error_t error;
      json_t* o = json_loads(properties, 0, &error);
      json_object_set_new(oData, "properties", o);
   }

   selectAllSchemaConf->freeResult();

   pushOutMessage(oArray, "schema", client);
   update(true, client);     // push the data ('init')

   return done;
}

//***************************************************************************
// Store Schema
//***************************************************************************

int P4d::storeSchema(json_t* oObject, long client)
{
   if (!client)
      return done;

   size_t index {0};
   json_t* jObj {nullptr};

   json_array_foreach(oObject, index, jObj)
   {
      int address = getIntFromJson(jObj, "address");
      const char* type = getStringFromJson(jObj, "type");
      int deleted = getBoolFromJson(jObj, "deleted");

      tableSchemaConf->clear();
      tableSchemaConf->setValue("ADDRESS", address);
      tableSchemaConf->setValue("TYPE", type);

      if (tableSchemaConf->find() && deleted)
      {
         tableSchemaConf->deleteWhere("%s = '%s' and %s = %d",
                                      tableSchemaConf->getField("TYPE")->getName(), type,
                                      tableSchemaConf->getField("ADDRESS")->getName(), address);
         continue;
      }

      tableSchemaConf->setValue("FUNCTION", getStringFromJson(jObj, "fct"));
      tableSchemaConf->setValue("USRTEXT", getStringFromJson(jObj, "usrtext"));
      tableSchemaConf->setValue("KIND", getIntFromJson(jObj, "kind"));
      tableSchemaConf->setValue("WIDTH", getIntFromJson(jObj, "width"));
      tableSchemaConf->setValue("HEIGHT", getIntFromJson(jObj, "height"));
      tableSchemaConf->setValue("SHOWTITLE", getIntFromJson(jObj, "showtitle"));
      tableSchemaConf->setValue("SHOWUNIT", getIntFromJson(jObj, "showunit"));
      tableSchemaConf->setValue("STATE", getStringFromJson(jObj, "state"));

      json_t* jProp = json_object_get(jObj, "properties");
      char* p = json_dumps(jProp, JSON_REAL_PRECISION(4));

      if (tableSchemaConf->getField("PROPERTIES")->getSize() < (int)strlen(p))
         tell(0, "Warning, Ignoring properties of %s:0x%x due to field limit of %d bytes",
              type, address, tableSchemaConf->getField("PROPERTIES")->getSize());
      else
         tableSchemaConf->setValue("PROPERTIES", p);

      tableSchemaConf->store();
      free(p);
   }

   tableSchemaConf->reset();
   replyResult(success, "Konfiguration gespeichert", client);

   return done;
}

//***************************************************************************
// Perform WS Sensor Alert Request
//***************************************************************************

int P4d::performAlerts(json_t* oObject, long client)
{
   json_t* oArray = json_array();

   tableSensorAlert->clear();

   for (int f = selectAllSensorAlerts->find(); f; f = selectAllSensorAlerts->fetch())
   {
      json_t* oData = json_object();
      json_array_append_new(oArray, oData);

      json_object_set_new(oData, "id", json_integer(tableSensorAlert->getIntValue("ID")));
      json_object_set_new(oData, "kind", json_string(tableSensorAlert->getStrValue("ID")));
      json_object_set_new(oData, "subid", json_integer(tableSensorAlert->getIntValue("SUBID")));
      json_object_set_new(oData, "lgop", json_integer(tableSensorAlert->getIntValue("LGOP")));
      json_object_set_new(oData, "type", json_string(tableSensorAlert->getStrValue("TYPE")));
      json_object_set_new(oData, "address", json_integer(tableSensorAlert->getIntValue("ADDRESS")));
      json_object_set_new(oData, "state", json_string(tableSensorAlert->getStrValue("STATE")));
      json_object_set_new(oData, "min", json_integer(tableSensorAlert->getIntValue("MIN")));
      json_object_set_new(oData, "max", json_integer(tableSensorAlert->getIntValue("MAX")));
      json_object_set_new(oData, "rangem", json_integer(tableSensorAlert->getIntValue("RANGEM")));
      json_object_set_new(oData, "delta", json_integer(tableSensorAlert->getIntValue("DELTA")));
      json_object_set_new(oData, "maddress", json_string(tableSensorAlert->getStrValue("MADDRESS")));
      json_object_set_new(oData, "msubject", json_string(tableSensorAlert->getStrValue("MSUBJECT")));
      json_object_set_new(oData, "mbody", json_string(tableSensorAlert->getStrValue("MBODY")));
      json_object_set_new(oData, "maxrepeat", json_integer(tableSensorAlert->getIntValue("MAXREPEAT")));

      //json_object_set_new(oData, "lastalert", json_integer(0));
   }

   selectAllSensorAlerts->freeResult();
   pushOutMessage(oArray, "alerts", client);

   return done;
}

//***************************************************************************
// Store Sensor Alerts
//***************************************************************************

int P4d::storeAlerts(json_t* oObject, long client)
{
   const char* action = getStringFromJson(oObject, "action", "");

   if (strcmp(action, "delete") == 0)
   {
      int alertid = getIntFromJson(oObject, "alertid", na);

      tableSensorAlert->deleteWhere("id = %d", alertid);

      performAlerts(0, client);
      replyResult(success, "Sensor Alert gelöscht", client);
   }

   else if (strcmp(action, "store") == 0)
   {
      json_t* array = json_object_get(oObject, "alerts");
      size_t index {0};
      json_t* jObj {nullptr};

      json_array_foreach(array, index, jObj)
      {
         int id = getIntFromJson(jObj, "id", na);

         tableSensorAlert->clear();

         if (id != na)
         {
            tableSensorAlert->setValue("ID", id);

            if (!tableSensorAlert->find())
               continue;
         }

         tableSensorAlert->clearChanged();
         tableSensorAlert->setValue("STATE", getStringFromJson(jObj, "state"));
         tableSensorAlert->setValue("MAXREPEAT", getIntFromJson(jObj, "maxrepeat"));

         tableSensorAlert->setValue("ADDRESS", getIntFromJson(jObj, "address"));
         tableSensorAlert->setValue("TYPE", getStringFromJson(jObj, "type"));
         tableSensorAlert->setValue("MIN", getIntFromJson(jObj, "min"));
         tableSensorAlert->setValue("MAX", getIntFromJson(jObj, "max"));
         tableSensorAlert->setValue("DELTA", getIntFromJson(jObj, "delta"));
         tableSensorAlert->setValue("RANGEM", getIntFromJson(jObj, "rangem"));
         tableSensorAlert->setValue("MADDRESS", getStringFromJson(jObj, "maddress"));
         tableSensorAlert->setValue("MSUBJECT", getStringFromJson(jObj, "msubject"));
         tableSensorAlert->setValue("MBODY", getStringFromJson(jObj, "mbody"));

         if (id == na)
            tableSensorAlert->insert();
         else if (tableSensorAlert->getChanges())
            tableSensorAlert->update();
      }

      performAlerts(0, client);
      replyResult(success, "Konfiguration gespeichert", client);
   }

   return success;
}

//***************************************************************************
// Perform Update TimeRanges
//***************************************************************************

int P4d::performUpdateTimeRanges(json_t* oObject, long client)
{
   int parent = getIntFromJson(oObject, "parent");
   updateTimeRangeData();
   replyResult(success, "... done", client);

   json_t* oJson = json_object();
   json_object_set_new(oJson, "parent", json_integer(parent));

   return performMenu(oJson, client);
}

//***************************************************************************
// Update Conf Tables
//***************************************************************************

int P4d::updateSchemaConfTable()
{
   const int step = 20;
   int y = 50;
   int added = 0;

   tableValueFacts->clear();
   tableValueFacts->setValue("STATE", "A");

   for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
   {
      int addr = tableValueFacts->getIntValue("ADDRESS");
      const char* type = tableValueFacts->getStrValue("TYPE");
      y += step;

      tableSchemaConf->clear();
      tableSchemaConf->setValue("ADDRESS", addr);
      tableSchemaConf->setValue("TYPE", type);

      if (!tableSchemaConf->find())
      {
         tableSchemaConf->setValue("KIND", "value");
         tableSchemaConf->setValue("STATE", "A");
         tableSchemaConf->setValue("COLOR", "black");
         tableSchemaConf->setValue("XPOS", 12);
         tableSchemaConf->setValue("YPOS", y);

         tableSchemaConf->store();
         added++;
      }
   }

   selectActiveValueFacts->freeResult();
   tell(eloAlways, "Added %d html schema configurations", added);

   return success;
}

//***************************************************************************
// Update Value Facts
//***************************************************************************

int P4d::initValueFacts(bool truncate)
{
   int status {success};
   Fs::ValueSpec v;
   int count {0};
   int added {0};
   int modified {0};

   // check serial communication

   if (request->check() != success)
   {
      serial->close();
      return fail;
   }

   if (truncate)
      tableValueFacts->truncate();

   // ---------------------------------
   // Add the sensor definitions delivered by the S 3200

   for (status = request->getFirstValueSpec(&v); status != Fs::wrnLast; status = request->getNextValueSpec(&v))
   {
      if (status != success)
         continue;

      tell(eloDebug, "%3d) 0x%04x '%s' %d '%s' (%04d) '%s'",
           count, v.address, v.name, v.factor, v.unit, v.type, v.description);

      int res = addValueFact(v.address, "VA", v.factor, v.name, strcmp(v.unit, "°") == 0 ? "°C" : v.unit,
                             wtMeter, v.description, 0, v.unit[0] == '%' ? 100 : 300);

      // set special value SUBTYPE for valuefact

      tableValueFacts->clear();
      tableValueFacts->setValue("ADDRESS", v.address);
      tableValueFacts->setValue("TYPE", "VA");

      tableValueFacts->find();
      tableValueFacts->setValue("SUBTYPE", v.type);
      tableValueFacts->store();

      count++;

      if (res == 1)
         added++;
      else if (res == 2)
         modified++;
   }

   tell(eloAlways, "Read %d value facts, modified %d and added %d", count, modified, added);

   // count = 0;
   // tell(0, "Update example value of table valuefacts");

   // for (int f = selectAllValueFacts->find(); f; f = selectAllValueFacts->fetch())
   // {
   //    if (!tableValueFacts->hasValue("TYPE", "VA"))
   //       continue;

   //    Value v(tableValueFacts->getIntValue("ADDRESS"));

   //    if ((status = request->getValue(&v)) != success)
   //    {
   //       tell(eloAlways, "Getting value 0x%04x failed, error %d", v.address, status);
   //       continue;
   //    }

   //    double factor = tableValueFacts->getIntValue("FACTOR");
   //    int dataType = tableValueFacts->getIntValue("SUBTYPE");
   //    int value = dataType == 1 ? (word)v.value : (sword)v.value;
   //    double theValue = value / factor;
   //    tableValueFacts->setValue("VALUE", theValue);
   //    tableValueFacts->update();
   //    count++;
   // }

   // selectAllValueFacts->freeResult();
   // tell(0, "Updated %d example values", count);

   // ---------------------------------
   // add default for digital outputs

   added = 0;
   count = 0;
   modified = 0;

   for (int f = selectAllMenuItems->find(); f; f = selectAllMenuItems->fetch())
   {
      char* name {nullptr};
      const char* type {nullptr};
      int structType = tableMenu->getIntValue("TYPE");
      std::string sname = tableMenu->getStrValue("TITLE");
      WidgetType widgetType = wtMeter;

      switch (structType)
      {
         case mstDigOut: type = "DO"; widgetType = wtSymbol; break;
         case mstDigIn:  type = "DI"; widgetType = wtSymbol; break;
         case mstAnlOut: type = "AO"; widgetType = wtMeter;  break;
      }

      if (!type)
         continue;

      removeCharsExcept(sname, nameChars);
      asprintf(&name, "%s_0x%x", sname.c_str(), (int)tableMenu->getIntValue("ADDRESS"));

      const char* unit = tableMenu->getStrValue("UNIT");

      if (isEmpty(unit) && strcmp(type, "AO") == 0)
         unit = "%";

      int res = addValueFact(tableMenu->getIntValue("ADDRESS"), type, 1, name, unit,
                             widgetType, tableMenu->getStrValue("TITLE"), 0, unit[0] == '%' ? 100 : 300);

      if (res == 1)
         added++;
      else if (res == 2)
         modified++;

      free(name);
      count++;
   }

   selectAllMenuItems->freeResult();
   tell(eloAlways, "Checked %d digital lines, added %d, modified %d", count, added, modified);

   // ---------------------------------
   // add value definitions for special data

   addValueFact(udState, "UD", 1, "Status", "zst", wtSymbol, "Heizungsstatus");

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", udState);      // 1  -> Kessel Status
   tableValueFacts->setValue("TYPE", "UD");            // UD -> User Defined

   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue("STATE", "A");
      tableValueFacts->store();
   }

   addValueFact(udMode, "UD", 1, "Betriebsmodus", "zst", wtText, "Betriebsmodus");

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", udMode);       // 2  -> Kessel Mode
   tableValueFacts->setValue("TYPE", "UD");            // UD -> User Defined

   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue("STATE", "A");
      tableValueFacts->store();
   }

   addValueFact(udTime, "UD", 1, "Uhrzeit", "T", wtText, "Datum Uhrzeit der Heizung");
   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", udTime);       // 3  -> Kessel Zeit
   tableValueFacts->setValue("TYPE", "UD");            // UD -> User Defined

   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue("STATE", "A");
      tableValueFacts->store();
   }

   return success;
}

//***************************************************************************
// Dispatch Mqtt Command Request
//   Format:  '{ "command" : "parstore", "address" : 0, "value" : "9" }'
//***************************************************************************

int P4d::dispatchMqttCommandRequest(json_t* jData, const char* topic)
{
   const char* command = getStringFromJson(jData, "command", "");

   if (isEmpty(command))
      return Daemon::dispatchMqttCommandRequest(jData, topic);

   if (strcmp(command, "parget") == 0)
   {
      int status {fail};
      json_t* jAddress = getObjectFromJson(jData, "address");
      int address = getIntFromJson(jData, "address", -1);

      if (!json_is_integer(jAddress) || address == -1)
      {
         tell(0, "Error: Missing address or invalid object type for MQTT command 'parstore', ignoring");
         return fail;
      }

      tell(0, "Perform MQTT command '%s' for address %d", command, address);

      ConfigParameter p(address);

      sem->p();
      status = request->getParameter(&p);
      sem->v();

      if (status != success)
      {
         tell(eloAlways, "Parameter request failed!");
         return fail;
      }

      tell(eloAlways, "Address: 0x%4.4x; Unit: %s; Value: %.*f", p.address, p.unit, p.digits, p.rValue);
   }
   else if (strcmp(command, "parstore") == 0)
   {
      int status {fail};
      json_t* jAddress = getObjectFromJson(jData, "address");
      int address = getIntFromJson(jData, "address", -1);
      const char* value = getStringFromJson(jData, "value");

      if (!json_is_integer(jAddress) || address == -1)
      {
         tell(0, "Error: Missing address or invalid object type for MQTT command 'parstore', ignoring");
         return fail;
      }

      if (isEmpty(value))
      {
         tell(0, "Error: Missing value for MQTT command 'parstore', ignoring");
         return fail;
      }

      tell(0, "Perform MQTT command '%s' for address %d with value '%s'", command, address, value);

      ConfigParameter p(address);

      sem->p();
      status = request->getParameter(&p);
      sem->v();

      if (status != success)
      {
         tell(eloAlways, "Set of parameter failed, query of current setting failed!");
         return fail;
      }

      if (p.setValueDirect(value, p.digits, p.getFactor()) != success)
      {
         tell(eloAlways, "Set of parameter failed, wrong format");
         return fail;
      }

      tell(eloAlways, "Storing value '%s' for parameter at address 0x%x", value, address);
      sem->p();
      status = request->setParameter(&p);
      sem->v();

      if (status == success)
      {
         tell(eloAlways, "Stored parameter");
      }
      else
      {
         tell(eloAlways, "Set of parameter failed, error was (%d)", status);

         if (status == P4Request::wrnNonUpdate)
            tell(eloAlways, "Value identical, ignoring request");
         else if (status == P4Request::wrnOutOfRange)
            tell(eloAlways, "Value out of range");
         else
            tell(eloAlways, "Serial communication error");
      }
   }
   else
   {
      tell(0, "Error: Got unexpected command '%s' in MQTT request, ignoring", command);
   }

   return success;
}

//***************************************************************************
//
//***************************************************************************

int P4d::s3200State2Json(json_t* obj)
{
   json_object_set_new(obj, "time", json_integer(currentState.time));
   json_object_set_new(obj, "state", json_integer(currentState.state));
   json_object_set_new(obj, "stateinfo", json_string(currentState.stateinfo));
   json_object_set_new(obj, "mode", json_integer(currentState.mode));
   json_object_set_new(obj, "modeinfo", json_string(currentState.modeinfo));
   json_object_set_new(obj, "version", json_string(currentState.version));
   json_object_set_new(obj, "image", json_string(getStateImage(currentState.state)));

   return done;
}

//***************************************************************************
//
//***************************************************************************

const char* P4d::getStateImage(int state)
{
   static char result[100] = "";
   const char* image {nullptr};

   if (state <= 0)
      image = "state-error.gif";
   else if (state == 1)
      image = "state-fireoff.gif";
   else if (state == 2)
      image = "state-heatup.gif";
   else if (state == 3)
      image = "state-fire.gif";
   else if (state == 4)
      image = "/state/state-firehold.gif";
   else if (state == 5)
      image = "state-fireoff.gif";
   else if (state == 6)
      image = "state-dooropen.gif";
   else if (state == 7)
      image = "state-preparation.gif";
   else if (state == 8)
      image = "state-warmup.gif";
   else if (state == 9)
      image = "state-heatup.gif";
   else if (state == 15 || state == 70 || state == 69)
      image = "state-clean.gif";
   else if ((state >= 10 && state <= 14) || state == 35 || state == 16)
      image = "state-wait.gif";
   else if (state == 60 || state == 61 || state == 72)
      image = "state-shfire.png";

   if (image)
      sprintf(result, "img/state/%s/%s", iconSet, image);
   else
      sprintf(result, "img/type/heating-%s.png", heatingType);

   return result;
}
