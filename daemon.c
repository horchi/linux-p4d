//***************************************************************************
// Automation Control
// File daemon.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 25.04.2020  Jörg Wendel
//***************************************************************************

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <libxml/parser.h>
#include <algorithm>
#include <cmath>
#include <libgen.h>

#ifndef _NO_RASPBERRY_PI_
#  include <wiringPi.h>
#endif

#include "lib/json.h"
#include "daemon.h"

bool Daemon::shutdown {false};

//***************************************************************************
// Web Service
//***************************************************************************

const char* cWebService::events[] =
{
   "unknown",
   "login",
   "logout",
   "pagechange",
   "data",
   "init",
   "toggleio",
   "toggleionext",
   "togglemode",
   "storeconfig",
   "gettoken",
   "iosetup",
   "storeiosetup",
   "chartdata",
   "logmessage",
   "userconfig",
   "storeuserconfig",
   "changepasswd",
   "reset",
   "groupconfig",
   "chartbookmarks",
   "storechartbookmarks",
   "sendmail",
   "syslog",
   "forcerefresh",

   "errors",
   "menu",
   "pareditrequest",
   "parstore",
   "alerts",
   "storealerts",
   "inittables",
   "storeschema",
   "updatetimeranges",
   "pellets",
   "pelletsadd",

   0
};

const char* cWebService::toName(Event event)
{
   if (event >= evUnknown && event < evCount)
      return events[event];

   return events[evUnknown];
}

cWebService::Event cWebService::toEvent(const char* name)
{
   if (!name)
      return evUnknown;

   for (int e = evUnknown; e < evCount; e++)
      if (strcasecmp(name, events[e]) == 0)
         return (Event)e;

   return evUnknown;
}

//***************************************************************************
// Service
//***************************************************************************

const char* Daemon::widgetTypes[] =
{
   "Symbol",
   "Chart",
   "Text",
   "Value",
   "Gauge",
   "Meter",
   "MeterLevel",
   "PlainText",
   "Choice",
   0
};

const char* Daemon::toName(WidgetType type)
{
   if (type > wtUnknown && type < wtCount)
      return widgetTypes[type];

   return widgetTypes[wtText];
}

Daemon::WidgetType Daemon::toType(const char* name)
{
   if (!name)
      return wtUnknown;

   for (int t = wtUnknown+1; t < wtCount; t++)
      if (strcasecmp(name, widgetTypes[t]) == 0)
         return (WidgetType)t;

   return wtText;
}

//***************************************************************************
// Object
//***************************************************************************

Daemon::Daemon()
{
   nextRefreshAt = time(0) + 5;
   startedAt = time(0);

   cDbConnection::init();
   cDbConnection::setEncoding("utf8");
   cDbConnection::setHost(dbHost);
   cDbConnection::setPort(dbPort);
   cDbConnection::setName(dbName);
   cDbConnection::setUser(dbUser);
   cDbConnection::setPass(dbPass);

   sem = new Sem(0x3da00001);
   serial = new Serial;
   request = new P4Request(serial);
   webSock = new cWebSock(this, httpPath);
}

Daemon::~Daemon()
{
   exit();

   delete mqttReader;
   delete mqttHassWriter;
   delete mqttHassReader;
   delete mqttHassCommandReader;
   delete webSock;

   free(mailScript);
   free(stateMailTo);

   free(stateMailAtStates);
   free(errorMailTo);
   free(sensorScript);

   delete serial;
   delete request;
   delete sem;

   cDbConnection::exit();
}

//***************************************************************************
// Push In Message (from WS to daemon)
//***************************************************************************

int Daemon::pushInMessage(const char* data)
{
   cMyMutexLock lock(&messagesInMutex);

   messagesIn.push(data);
   loopCondition.Broadcast();

   return success;
}

//***************************************************************************
// Push Out Message (from daemon to WS)
//***************************************************************************

int Daemon::pushOutMessage(json_t* oContents, const char* title, long client)
{
   json_t* obj = json_object();

   addToJson(obj, "event", title);
   json_object_set_new(obj, "object", oContents);

   char* p = json_dumps(obj, JSON_REAL_PRECISION(4));
   json_decref(obj);

   if (!p)
   {
      tell(0, "Error: Dumping json message failed");
      return fail;
   }

   webSock->pushOutMessage(p, (lws*)client);
   tell(2, "-> event '%s' (0x%lx) [%.150s..]", title, client, p);
   tell(3, "DEBUG: PushMessage [%s]", p);
   free(p);

   webSock->performData(cWebSock::mtData);

   return done;
}

int Daemon::pushDataUpdate(const char* title, long client)
{
   // push all in the jsonSensorList to the 'interested' clients

   if (client)
   {
      auto cl = wsClients[(void*)client];
      json_t* oWsJson = json_array();

      json_t* oJson = json_object();
      daemonState2Json(oJson);
      pushOutMessage(oJson, "daemonstate", client);

      oJson = json_object();
      s3200State2Json(oJson);
      pushOutMessage(oJson, "s3200-state", client);

      if (cl.page == "index")
      {
         if (addrsDashboard.size())
            for (const auto& sensor : addrsDashboard)
               json_array_append(oWsJson, jsonSensorList[sensor]);
         else
            for (auto sj : jsonSensorList)
               json_array_append(oWsJson, sj.second);
      }
      else if (cl.page == "list")
      {
         if (addrsList.size())
            for (const auto& sensor : addrsList)
               json_array_append(oWsJson, jsonSensorList[sensor]);
         else
            for (auto sj : jsonSensorList)
               json_array_append(oWsJson, sj.second);
      }
      else if (cl.page == "schema")
      {
         // #TODO - send visible instead of all??

         for (auto sj : jsonSensorList)
            json_array_append(oWsJson, sj.second);
      }

      pushOutMessage(oWsJson, title, client);
   }
   else
   {
      for (const auto& cl : wsClients)
      {
         json_t* oWsJson = json_array();

         json_t* oJson = json_object();
         daemonState2Json(oJson);
         pushOutMessage(oJson, "daemonstate", client);

         oJson = json_object();
         s3200State2Json(oJson);
         pushOutMessage(oJson, "s3200-state", client);

         if (cl.second.page == "index")
         {
            if (addrsDashboard.size())
               for (const auto& sensor : addrsDashboard)
                  json_array_append(oWsJson, jsonSensorList[sensor]);
            else
               for (auto sj : jsonSensorList)
                  json_array_append(oWsJson, sj.second);
         }
         else if (cl.second.page == "list")
         {
            if (addrsList.size())
               for (const auto& sensor : addrsList)
                  json_array_append(oWsJson, jsonSensorList[sensor]);
            else
               for (auto sj : jsonSensorList)
                  json_array_append(oWsJson, sj.second);
         }
         else if (cl.second.page == "schema")
         {
            // #TODO - send visible instead of all??

            for (auto sj : jsonSensorList)
               json_array_append(oWsJson, sj.second);
         }

         pushOutMessage(oWsJson, title, (long)cl.first);
      }
   }

   // cleanup

   for (auto sj : jsonSensorList)
      json_decref(sj.second);

   jsonSensorList.clear();

   return success;
}

//***************************************************************************
// Init / Exit
//***************************************************************************

int Daemon::init()
{
   int status {success};
   char* dictPath {nullptr};

   initLocale();

   if (fileExists(httpPath))
   {
      char* link {nullptr};
      asprintf(&link, "%s/stylesheet.css", httpPath);

      if (!fileExists(link))
      {
         char* stylesheet {nullptr};
         asprintf(&stylesheet, "%s/stylesheet-dark.css", httpPath);
         tell(eloAlways, "Creating link '%s'", link);
         createLink(link, stylesheet, true);
         free(stylesheet);
      }

      free(link);
   }
   else
   {
      tell(eloAlways, "Missing http path '%s'", httpPath);
   }

   // initialize the dictionary

   asprintf(&dictPath, "%s/database.dat", confDir);

   if (dbDict.in(dictPath) != success)
   {
      tell(0, "Fatal: Dictionary not loaded, aborting!");
      return 1;
   }

   tell(0, "Dictionary '%s' loaded", dictPath);
   free(dictPath);

   if ((status = initDb()) != success)
   {
      exitDb();
      return status;
   }

   // ---------------------------------
   // check users - add default user if empty

   int userCount {0};
   tableUsers->countWhere("", userCount);

   if (userCount <= 0)
   {
      tell(0, "Initially adding default user (" TARGET "/" TARGET ")");

      md5Buf defaultPwd;
      createMd5(TARGET, defaultPwd);
      tableUsers->clear();
      tableUsers->setValue("USER", TARGET);
      tableUsers->setValue("PASSWD", defaultPwd);
      tableUsers->setValue("TOKEN", "dein&&secret12login34token");
      tableUsers->setValue("RIGHTS", 0xff);  // all rights
      tableUsers->store();
   }

   // ---------------------------------
   // Update/Read configuration from config table

   for (const auto& it : *getConfiguration())
   {
      tableConfig->clear();
      tableConfig->setValue("OWNER", myName());
      tableConfig->setValue("NAME", it.name.c_str());

      if (!tableConfig->find())
      {
         tableConfig->setValue("VALUE", it.def);
         tableConfig->store();
      }
   }

   readConfiguration(true);

   // ---------------------------------
   // setup GPIO

   wiringPiSetupPhys();     // we use the 'physical' PIN numbers
   // wiringPiSetup();      // to use the 'special' wiringPi PIN numbers
   // wiringPiSetupGpio();  // to use the 'GPIO' PIN numbers

   // ---------------------------------
   // apply configuration specials

   applyConfigurationSpecials();

   // init web socket ...

   while (webSock->init(webPort, webSocketPingTime, confDir, webSsl) != success)
   {
      tell(0, "Retrying in 2 seconds");
      sleep(2);
   }

//   initArduino();
   performMqttRequests();
   initScripts();
   loadStates();

   initialized = true;

   return success;
}

int Daemon::initLocale()
{
   // set a locale to "" means 'reset it to the environment'
   // as defined by the ISO-C standard the locales after start are C

   const char* locale {nullptr};

   setlocale(LC_ALL, "");
   locale = setlocale(LC_ALL, 0);  // 0 for query the setting

   if (!locale)
   {
      tell(0, "Info: Detecting locale setting for LC_ALL failed");
      return fail;
   }

   tell(1, "current locale is %s", locale);

   if ((strcasestr(locale, "UTF-8") != 0) || (strcasestr(locale, "UTF8") != 0))
      tell(1, "Detected UTF-8");

   return done;
}

int Daemon::exit()
{
   exitDb();
   serial->close();

   return success;
}

//***************************************************************************
// Init digital Output
//***************************************************************************

int Daemon::initOutput(uint pin, int opt, OutputMode mode, const char* name, uint rights)
{
   digitalOutputStates[pin].opt = opt;
   digitalOutputStates[pin].mode = mode;
   digitalOutputStates[pin].name = name;

   cDbRow* fact = valueFactOf("DO", pin);

   if (fact)
   {
      if (!fact->getValue("USRTITLE")->isEmpty())
         digitalOutputStates[pin].title = fact->getStrValue("USRTITLE");
      else
         digitalOutputStates[pin].title = fact->getStrValue("TITLE");
   }
   else
   {
      digitalOutputStates[pin].title = name;
      tableValueFacts->setValue("RIGHTS", (int)rights);
   }

   pinMode(pin, OUTPUT);
   gpioWrite(pin, false, false);
   addValueFact(pin, "DO", name, "", wtSymbol, 0, 0, urControl);

   return done;
}

//***************************************************************************
// Init digital Input
//***************************************************************************

int Daemon::initInput(uint pin, const char* name)
{
   pinMode(pin, INPUT);

   if (!isEmpty(name))
      addValueFact(pin, "DI", name, "", wtSymbol, 0, 0);

   digitalInputStates[pin] = gpioRead(pin);

   return done;
}

//***************************************************************************
// Init Scripts
//***************************************************************************

int Daemon::initScripts()
{
   char* path {nullptr};
   int count {0};
   FileList scripts;

   asprintf(&path, "%s/scripts.d", confDir);
   int status = getFileList(path, DT_REG, "sh py", false, &scripts, count);

   if (status == success)
   {
      for (const auto& script : scripts)
      {
         char* scriptPath {nullptr};
         uint addr {0};
         char* cmd {nullptr};
         std::string result;
         PySensor* pySensor {nullptr};

         asprintf(&scriptPath, "%s/%s", path, script.name.c_str());

         if (strstr(script.name.c_str(), ".py"))
         {
            pySensor = new PySensor(this, path, script.name.c_str());

            if (pySensor->init() != success || (result = executePython(pySensor, "status")) == "")
            {
               tell(0, "Initialized python script '%s' failed", script.name.c_str());
               delete pySensor;
               continue;
            }
         }
         else
         {
            asprintf(&cmd, "%s status", scriptPath);
            result = executeCommand(cmd);
            tell(5, "Calling '%s'", cmd);
            free(cmd);
         }

         json_error_t error;
         json_t* oData = json_loads(result.c_str(), 0, &error);

         if (!oData)
         {
            tell(0, "Error: Ignoring invalid script result [%s]", result.c_str());
            tell(0, "Error decoding json: %s (%s, line %d column %d, position %d)",
                 error.text, error.source, error.line, error.column, error.position);
            delete pySensor;
            continue;
         }

         std::string kind = getStringFromJson(oData, "kind", "status");
         const char* title = getStringFromJson(oData, "title");
         const char* unit = getStringFromJson(oData, "unit");
         const char* choices = getStringFromJson(oData, "choices");
         double value = getDoubleFromJson(oData, "value");
         bool valid = getBoolFromJson(oData, "valid", false);
         const char* text = getStringFromJson(oData, "text");

         tableScripts->clear();
         tableScripts->setValue("PATH", scriptPath);

         if (!selectScriptByPath->find())
         {
            tableScripts->store();
            addr = tableScripts->getLastInsertId();
         }
         else
            addr = tableScripts->getIntValue("ID");

         selectScriptByPath->freeResult();

         addValueFact(addr, "SC", !isEmpty(title) ? title : script.name.c_str(), unit,
                      kind == "status" ? wtSymbol : kind == "text" ? wtText : wtValue,
                      0, 0, urControl, choices);

         tell(0, "Init script value of 'SC:%d' to %.2f", addr, value);

         scSensors[addr].kind = kind;
         scSensors[addr].pySensor = pySensor;
         scSensors[addr].last = time(0);
         scSensors[addr].valid = valid;

         if (kind == "status")
            scSensors[addr].state = (bool)value;
         else if (kind == "text")
            scSensors[addr].text = text;
         else if (kind == "value")
            scSensors[addr].value = value;

         tell(0, "Found script '%s' addr (%d), unit '%s'; result was [%s]", scriptPath, addr, unit, result.c_str());
         free(scriptPath);
      }
   }

   free(path);

   return status;
}

//***************************************************************************
// Call Script
//***************************************************************************

int Daemon::callScript(int addr, const char* command, const char* name, const char* title)
{
   char* cmd {nullptr};

   tableScripts->clear();
   tableScripts->setValue("ID", addr);

   if (!tableScripts->find())
   {
      tell(0, "Fatal: Script with id (%d) not found", addr);
      return fail;
   }

   asprintf(&cmd, "%s %s", tableScripts->getStrValue("PATH"), command);

   std::string result;
   tell(2, "Info: Calling '%s'", cmd);

   if (scSensors[addr].pySensor != nullptr)
      result = executePython(scSensors[addr].pySensor, command);
   else
      result = executeCommand(cmd);

   tableScripts->reset();
   tell(2, "Debug: Result of script '%s' was [%s]", cmd, result.c_str());
   free(cmd);

   json_error_t error;
   json_t* oData = json_loads(result.c_str(), 0, &error);

   if (!oData)
   {
      tell(0, "Error: Ignoring invalid script result [%s]", result.c_str());
      tell(0, "Error decoding json: %s (%s, line %d column %d, position %d)",
           error.text, error.source, error.line, error.column, error.position);
      return fail;
   }

   std::string kind = getStringFromJson(oData, "kind", "status");
   const char* unit = getStringFromJson(oData, "unit");
   double value = getDoubleFromJson(oData, "value");
   const char* text = getStringFromJson(oData, "text");
   bool valid = getBoolFromJson(oData, "valid", false);

   tell(3, "DEBUG: Got '%s' from script (kind:%s unit:%s value:%0.2f) [SC:%d]", result.c_str(), kind.c_str(), unit, value, addr);

   scSensors[addr].kind = kind;
   scSensors[addr].last = time(0);
   scSensors[addr].valid = valid;

   if (kind == "status")
      scSensors[addr].state = (bool)value;
   else if (kind == "text")
      scSensors[addr].text = text;
   else if (kind == "value")
      scSensors[addr].value = value;
   else
      tell(0, "Got unexpected script kind '%s' in '%s'", kind.c_str(), result.c_str());

   // update WS
   {
      json_t* oJson = json_array();
      json_t* ojData = json_object();
      json_array_append_new(oJson, ojData);

      json_object_set_new(ojData, "address", json_integer((ulong)addr));
      json_object_set_new(ojData, "type", json_string("SC"));
      json_object_set_new(ojData, "name", json_string(name));
      json_object_set_new(ojData, "title", json_string(title));

      if (kind == "status")
         json_object_set_new(ojData, "value", json_integer((bool)value));
      else if (kind == "text")
         json_object_set_new(ojData, "text", json_string(text));
      else if (kind == "value")
         json_object_set_new(ojData, "value", json_real(value));

      char* tuple {nullptr};
      asprintf(&tuple, "%s:0x%02x", "SC", (int)addr);
      jsonSensorList[tuple] = ojData;
      free(tuple);

      pushDataUpdate("update", 0L);
   }

   mqttPublishSensor(iotLight, name, "", "", value, "", false /*forceConfig*/);

   return success;
}

//***************************************************************************
// Execute Python Sensor Script
//***************************************************************************

std::string Daemon::executePython(PySensor* pySensor, const char* command)
{
   pySensor->execute(command);

   return pySensor->getResult();
}

//***************************************************************************
// Value Fact Of
//***************************************************************************

cDbRow* Daemon::valueFactOf(const char* type, int addr)
{
   tableValueFacts->clear();

   tableValueFacts->setValue("ADDRESS", addr);
   tableValueFacts->setValue("TYPE", type);

   if (!tableValueFacts->find())
      return nullptr;

   return tableValueFacts->getRow();
}

//***************************************************************************
// Set Special Value
//***************************************************************************

void Daemon::setSpecialValue(uint addr, double value, const std::string& text)
{
   spSensors[addr].last = time(0);
   spSensors[addr].value = value;
   spSensors[addr].text = text;
   spSensors[addr].kind = text == "" ? "value" : "text";
   spSensors[addr].valid = spSensors[addr].kind == "text" ? true : !isNan(value);
}


//***************************************************************************
// Sensor Json String Of
//***************************************************************************

std::string Daemon::sensorJsonStringOf(const char* type, uint addr)
{
   cDbRow* fact = valueFactOf(type, addr);

   if (!fact)
      return "";

   json_t* ojData = json_object();

   sensor2Json(ojData, tableValueFacts);

   if (strcmp(type, "W1") == 0)
   {
      bool w1Exist = existW1(fact->getStrValue("NAME"));
      time_t w1Last {0};
      double w1Value {0};

      if (w1Exist)
         w1Value = valueOfW1(fact->getStrValue("NAME"), w1Last);

      json_object_set_new(ojData, "value", json_real(w1Value));
      json_object_set_new(ojData, "last", json_integer(w1Last));
      json_object_set_new(ojData, "valid", json_boolean(w1Exist && w1Last > time(0)-300));
   }
   else if (strcmp(type, "AI") == 0)
   {
      json_object_set_new(ojData, "value", json_real(aiSensors[addr].value));
      json_object_set_new(ojData, "last", json_integer(aiSensors[addr].last));
      json_object_set_new(ojData, "valid", json_boolean(aiSensors[addr].valid));
      if (aiSensors[addr].disabled)
         json_object_set_new(ojData, "disabled", json_boolean(true));
   }
   else if (strcmp(type, "DO") == 0)
   {
      json_object_set_new(ojData, "value", json_integer(digitalOutputStates[addr].state));
      json_object_set_new(ojData, "last", json_integer(digitalOutputStates[addr].last));
      json_object_set_new(ojData, "valid", json_boolean(digitalOutputStates[addr].valid));
   }
   else if (strcmp(type, "SC") == 0)
   {
      if (scSensors[addr].kind == "status")
         json_object_set_new(ojData, "value", json_integer(scSensors[addr].state));
      else if (scSensors[addr].kind == "text")
         json_object_set_new(ojData, "text", json_string(scSensors[addr].text.c_str()));
      else if (scSensors[addr].kind == "value")
         json_object_set_new(ojData, "value", json_real(scSensors[addr].value));

      json_object_set_new(ojData, "last", json_integer(scSensors[addr].last));
      json_object_set_new(ojData, "valid", json_boolean(scSensors[addr].valid));

      if (scSensors[addr].disabled)
         json_object_set_new(ojData, "disabled", json_boolean(true));
   }
   else if (strcmp(type, "SP") == 0)
   {
      if (spSensors[addr].kind == "text")
         json_object_set_new(ojData, "text", json_string(spSensors[addr].text.c_str()));
      else if (spSensors[addr].kind == "value")
         json_object_set_new(ojData, "value", json_real(spSensors[addr].value));

      json_object_set_new(ojData, "last", json_integer(spSensors[addr].last));
      json_object_set_new(ojData, "valid", json_integer(spSensors[addr].valid));

      if (spSensors[addr].disabled)
         json_object_set_new(ojData, "disabled", json_boolean(true));
   }

   char* p = json_dumps(ojData, JSON_REAL_PRECISION(4));
   json_decref(ojData);

   if (!p)
   {
      tell(0, "Error: Dumping json message in 'sensorJsonStringOf' failed");
      return "";
   }

   std::string str = p;
   free(p);

   return str;
}

//***************************************************************************
// Init/Exit Database
//***************************************************************************

cDbFieldDef xmlTimeDef("XML_TIME", "xmltime", cDBS::ffAscii, 20, cDBS::ftData);
cDbFieldDef rangeFromDef("RANGE_FROM", "rfrom", cDBS::ffDateTime, 0, cDBS::ftData);
cDbFieldDef rangeToDef("RANGE_TO", "rto", cDBS::ffDateTime, 0, cDBS::ftData);
cDbFieldDef avgValueDef("AVG_VALUE", "avalue", cDBS::ffFloat, 122, cDBS::ftData);
cDbFieldDef maxValueDef("MAX_VALUE", "mvalue", cDBS::ffInt, 0, cDBS::ftData);
cDbFieldDef minValueDef("MIN_VALUE", "minvalue", cDBS::ffInt, 0, cDBS::ftData);
cDbFieldDef rangeEndDef("time", "time", cDBS::ffDateTime, 0, cDBS::ftData);
cDbFieldDef endTimeDef("END_TIME", "endtime", cDBS::ffDateTime, 0, cDBS::ftData);

int Daemon::initDb()
{
   static int initial = yes;
   int status {success};

   if (connection)
      exitDb();

   tell(eloAlways, "Try conneting to database");

   connection = new cDbConnection();

   if (initial)
   {
      // ------------------------------------------
      // initially create/alter tables and indices
      // ------------------------------------------

      tell(0, "Checking database connection ...");

      if (connection->attachConnection() != success)
      {
         tell(0, "Error: Initial database connect failed");
         return fail;
      }

      tell(0, "Checking table structure and indices ...");

      for (auto t = dbDict.getFirstTableIterator(); t != dbDict.getTableEndIterator(); t++)
      {
         cDbTable* table = new cDbTable(connection, t->first.c_str());

         tell(1, "Checking table '%s'", t->first.c_str());

         if (!table->exist())
         {
            if ((status += table->createTable()) != success)
               continue;
         }
         else
         {
            status += table->validateStructure(2);
         }

         status += table->createIndices();

         delete table;
      }

      connection->detachConnection();

      if (status != success)
         return abrt;

      tell(0, "Checking table structure and indices succeeded");
   }

   // ------------------------
   // create/open tables
   // ------------------------

   tableValueFacts = new cDbTable(connection, "valuefacts");
   if (tableValueFacts->open() != success) return fail;

   tableGroups = new cDbTable(connection, "groups");
   if (tableGroups->open() != success) return fail;

   tableErrors = new cDbTable(connection, "errors");
   if (tableErrors->open() != success) return fail;

   tableMenu = new cDbTable(connection, "menu");
   if (tableMenu->open() != success) return fail;

   tableSamples = new cDbTable(connection, "samples");
   if (tableSamples->open() != success) return fail;

   tablePeaks = new cDbTable(connection, "peaks");
   if (tablePeaks->open() != success) return fail;

   tableSensorAlert = new cDbTable(connection, "sensoralert");
   if (tableSensorAlert->open() != success) return fail;

   tableSchemaConf = new cDbTable(connection, "schemaconf");
   if (tableSchemaConf->open() != success) return fail;

   tableConfig = new cDbTable(connection, "config");
   if (tableConfig->open() != success) return fail;

   tableTimeRanges = new cDbTable(connection, "timeranges");
   if (tableTimeRanges->open() != success) return fail;

   tableScripts = new cDbTable(connection, "scripts");
   if (tableScripts->open() != success) return fail;

   tableUsers = new cDbTable(connection, "users");
   if (tableUsers->open() != success) return fail;

   tablePellets = new cDbTable(connection, "pellets");
   if (tablePellets->open() != success) return fail;

   // prepare statements

   selectActiveValueFacts = new cDbStatement(tableValueFacts);

   selectActiveValueFacts->build("select ");
   selectActiveValueFacts->bindAllOut();
   selectActiveValueFacts->build(" from %s where ", tableValueFacts->TableName());
   selectActiveValueFacts->bind("STATE", cDBS::bndIn | cDBS::bndSet);

   status += selectActiveValueFacts->prepare();

   // ------------------

   selectAllValueFacts = new cDbStatement(tableValueFacts);

   selectAllValueFacts->build("select ");
   selectAllValueFacts->bindAllOut();
   selectAllValueFacts->build(" from %s", tableValueFacts->TableName());

   status += selectAllValueFacts->prepare();

   // ------------------

   selectAllGroups = new cDbStatement(tableGroups);

   selectAllGroups->build("select ");
   selectAllGroups->bindAllOut();
   selectAllGroups->build(" from %s", tableGroups->TableName());

   status += selectAllGroups->prepare();

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

   // ------------------
   // select samples for chart data
   // ein sample avg / 5 Minuten

   rangeFrom.setField(&rangeFromDef);
   rangeTo.setField(&rangeToDef);
   xmlTime.setField(&xmlTimeDef);
   avgValue.setField(&avgValueDef);
   maxValue.setField(&maxValueDef);
   selectSamplesRange = new cDbStatement(tableSamples);

   selectSamplesRange->build("select ");
   selectSamplesRange->bind("ADDRESS", cDBS::bndOut);
   selectSamplesRange->bind("TYPE", cDBS::bndOut, ", ");
   selectSamplesRange->bindTextFree("date_format(time, '%Y-%m-%dT%H:%i')", &xmlTime, ", ", cDBS::bndOut);
   selectSamplesRange->bindTextFree("avg(value)", &avgValue, ", ", cDBS::bndOut);
   selectSamplesRange->bindTextFree("max(value)", &maxValue, ", ", cDBS::bndOut);
   selectSamplesRange->build(" from %s where ", tableSamples->TableName());
   selectSamplesRange->bind("ADDRESS", cDBS::bndIn | cDBS::bndSet);
   selectSamplesRange->bind("TYPE", cDBS::bndIn | cDBS::bndSet, " and ");
   selectSamplesRange->bindCmp(0, "TIME", &rangeFrom, ">=", " and ");
   selectSamplesRange->bindCmp(0, "TIME", &rangeTo, "<=", " and ");
   selectSamplesRange->build(" group by date(time), ((60/5) * hour(time) + floor(minute(time)/5))");
   selectSamplesRange->build(" order by time");

   status += selectSamplesRange->prepare();

   // ------------------
   // select samples for chart data (for dashboard widget and rage > 15)
   // ein sample avg / 60 Minuten

   selectSamplesRange60 = new cDbStatement(tableSamples);

   selectSamplesRange60->build("select ");
   selectSamplesRange60->bind("ADDRESS", cDBS::bndOut);
   selectSamplesRange60->bind("TYPE", cDBS::bndOut, ", ");
   selectSamplesRange60->bindTextFree("date_format(time, '%Y-%m-%dT%H:%i')", &xmlTime, ", ", cDBS::bndOut);
   selectSamplesRange60->bindTextFree("avg(value)", &avgValue, ", ", cDBS::bndOut);
   selectSamplesRange60->bindTextFree("max(value)", &maxValue, ", ", cDBS::bndOut);
   selectSamplesRange60->build(" from %s where ", tableSamples->TableName());
   selectSamplesRange60->bind("ADDRESS", cDBS::bndIn | cDBS::bndSet);
   selectSamplesRange60->bind("TYPE", cDBS::bndIn | cDBS::bndSet, " and ");
   selectSamplesRange60->bindCmp(0, "TIME", &rangeFrom, ">=", " and ");
   selectSamplesRange60->bindCmp(0, "TIME", &rangeTo, "<=", " and ");
   selectSamplesRange60->build(" group by date(time), ((60/60) * hour(time) + floor(minute(time)/60))");
   selectSamplesRange60->build(" order by time");

   status += selectSamplesRange60->prepare();

   // ------------------
   // select samples for chart data
   // ein sample avg / Tag (for ranges > 300 Tage)

   selectSamplesRange720 = new cDbStatement(tableSamples);

   selectSamplesRange720->build("select ");
   selectSamplesRange720->bind("ADDRESS", cDBS::bndOut);
   selectSamplesRange720->bind("TYPE", cDBS::bndOut, ", ");
   selectSamplesRange720->bindTextFree("date_format(time, '%Y-%m-%dT%H:%i')", &xmlTime, ", ", cDBS::bndOut);
   selectSamplesRange720->bindTextFree("avg(value)", &avgValue, ", ", cDBS::bndOut);
   selectSamplesRange720->bindTextFree("max(value)", &maxValue, ", ", cDBS::bndOut);
   selectSamplesRange720->build(" from %s where ", tableSamples->TableName());
   selectSamplesRange720->bind("ADDRESS", cDBS::bndIn | cDBS::bndSet);
   selectSamplesRange720->bind("TYPE", cDBS::bndIn | cDBS::bndSet, " and ");
   selectSamplesRange720->bindCmp(0, "TIME", &rangeFrom, ">=", " and ");
   selectSamplesRange720->bindCmp(0, "TIME", &rangeTo, "<=", " and ");
   selectSamplesRange720->build(" group by date(time)");
   selectSamplesRange720->build(" order by time");

   status += selectSamplesRange720->prepare();

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


   // --------------------
   // select max(time) from samples

   selectMaxTime = new cDbStatement(tableSamples);

   selectMaxTime->build("select ");
   selectMaxTime->bind("TIME", cDBS::bndOut, "max(");
   selectMaxTime->build(") from %s", tableSamples->TableName());

   status += selectMaxTime->prepare();

   // ------------------

   selectScriptByName = new cDbStatement(tableScripts);

   selectScriptByName->build("select ");
   selectScriptByName->bindAllOut();
   selectScriptByName->build(" from %s where ", tableScripts->TableName());
   selectScriptByName->bind("NAME", cDBS::bndIn | cDBS::bndSet);

   status += selectScriptByName->prepare();

   // ------------------
   //

   selectScript = new cDbStatement(tableScripts);

   selectScript->build("select ");
   selectScript->bindAllOut();
   selectScript->build(" from %s where ", tableScripts->TableName());
   selectScript->bind("PATH", cDBS::bndIn | cDBS::bndSet);

   status += selectScript->prepare();

   // ------------------
   // select all config

   selectAllConfig = new cDbStatement(tableConfig);

   selectAllConfig->build("select ");
   selectAllConfig->bindAllOut();
   selectAllConfig->build(" from %s", tableConfig->TableName());

   status += selectAllConfig->prepare();

   // ------------------
   // select all users

   selectAllUser = new cDbStatement(tableUsers);

   selectAllUser->build("select ");
   selectAllUser->bindAllOut();
   selectAllUser->build(" from %s", tableUsers->TableName());

   status += selectAllUser->prepare();

   // ------------------
   // select all pellets

   selectAllPellets = new cDbStatement(tablePellets);
   selectAllPellets->build("select ");
   selectAllPellets->bindAllOut();
   selectAllPellets->build(" from %s", tablePellets->TableName());
   selectAllPellets->build(" order by time");
   status += selectAllPellets->prepare();

   // ------------------
   // select script by path

   selectScriptByPath = new cDbStatement(tableScripts);

   selectScriptByPath->build("select ");
   selectScriptByPath->bindAllOut();
   selectScriptByPath->build(" from %s where ", tableScripts->TableName());
   selectScriptByPath->bind("PATH", cDBS::bndIn | cDBS::bndSet);

   status += selectScriptByPath->prepare();

   // ------------------

   if (status == success)
      tell(eloAlways, "Connection to database established");

   int gCount {0};

   if (connection->query(gCount, "select * from groups") == success)
   {
      if (!gCount)
      {
         connection->query("insert into groups set name='Heizung'");
         connection->query("update valuefacts set groupid = 1 where groupid is null or groupid = 0");
      }
   }

   updateScripts();

   return status;
}

int Daemon::exitDb()
{
   delete tableSamples;               tableSamples = nullptr;
   delete tablePeaks;                 tablePeaks = nullptr;
   delete tableValueFacts;            tableValueFacts = nullptr;
   delete tableGroups;                tableGroups = nullptr;
   delete tableUsers;                 tableUsers = nullptr;
   delete tablePellets;               tablePellets= nullptr;
   delete tableMenu;                  tableMenu = nullptr;
   delete tableSensorAlert;           tableSensorAlert = nullptr;
   delete tableSchemaConf;            tableSchemaConf = nullptr;
   delete tableErrors;                tableErrors = nullptr;
   delete tableConfig;                tableConfig = nullptr;
   delete tableTimeRanges;            tableTimeRanges = nullptr;
   delete tableScripts;               tableScripts = nullptr;

   delete selectActiveValueFacts;     selectActiveValueFacts = nullptr;
   delete selectAllValueFacts;        selectAllValueFacts = nullptr;
   delete selectAllGroups;            selectAllGroups = nullptr;
   delete selectAllMenuItems;         selectAllMenuItems = nullptr;
   delete selectMenuItemsByParent;    selectMenuItemsByParent = nullptr;
   delete selectMenuItemsByChild;     selectMenuItemsByChild = nullptr;
   delete selectSensorAlerts;         selectSensorAlerts = nullptr;
   delete selectAllSensorAlerts;      selectAllSensorAlerts = nullptr;
   delete selectSampleInRange;        selectSampleInRange = nullptr;
   delete selectAllErrors;            selectAllErrors = nullptr;
   delete selectPendingErrors;        selectPendingErrors = nullptr;
   delete selectMaxTime;              selectMaxTime = nullptr;
   delete selectScriptByName;         selectScriptByName = nullptr;
   delete selectScript;               selectScript = nullptr;
   delete selectAllConfig;            selectAllConfig = nullptr;
   delete selectAllUser;              selectAllUser = nullptr;
   delete selectAllPellets;           selectAllPellets = nullptr;
   delete selectStokerHours;          selectStokerHours = nullptr;
   delete selectSamplesRange;         selectSamplesRange = nullptr;
   delete selectSamplesRange60;       selectSamplesRange60 = nullptr;
   delete selectSamplesRange720;      selectSamplesRange720 = nullptr;
   delete selectStateDuration;        selectStateDuration = nullptr;
   delete selectSchemaConfByState;    selectSchemaConfByState = nullptr;
   delete selectAllSchemaConf;        selectAllSchemaConf = nullptr;
   delete selectScriptByPath;         selectScriptByPath = nullptr;

   delete connection; connection = nullptr;

   return done;
}

//***************************************************************************
// Init one wire sensors
//***************************************************************************

int Daemon::initW1()
{
   int count {0};
   int added {0};
   int modified {0};

   for (const auto& it : w1Sensors)
   {
      int res = addValueFact((int)toW1Id(it.first.c_str()), "W1", 1, it.first.c_str(), "°C", "", true);

      if (res == 1)
         added++;
      else if (res == 2)
         modified++;

      count++;
   }

   tell(eloAlways, "Found %d one wire sensors, added %d, modified %d", count, added, modified);

   return success;
}

//***************************************************************************
// Read Configuration
//***************************************************************************

int Daemon::readConfiguration(bool initial)
{
   char* webUser {nullptr};
   char* webPass {nullptr};
   md5Buf defaultPwd;

   // init configuration

   if (argLoglevel != na)
      loglevel = argLoglevel;
   else
      getConfigItem("loglevel", loglevel, 1);

   // init default web user and password

   createMd5("p4-3200", defaultPwd);
   getConfigItem("user", webUser, "p4");
   getConfigItem("passwd", webPass, defaultPwd);

   free(webUser);
   free(webPass);

   // init configuration

   getConfigItem("interval", interval, 60);
   getConfigItem("consumptionPerHour", consumptionPerHour, 0);

   getConfigItem("webPort", webPort, webPort);
   getConfigItem("webUrl", webUrl);
   getConfigItem("webSsl", webSsl);

   char* port {nullptr};
   asprintf(&port, "%d", webPort);
   if (isEmpty(webUrl) || !strstr(webUrl, port))
   {
      asprintf(&webUrl, "http%s://%s:%d", webSsl ? "s" : "", getFirstIp(), webPort);
      setConfigItem("webUrl", webUrl);
   }
   free(port);

   getConfigItem("knownStates", knownStates, "");

   if (!isEmpty(knownStates))
   {
      std::vector<std::string> sStates = split(knownStates, ':');
      for (const auto& s : sStates)
         stateDurations[atoi(s.c_str())] = 0;

      tell(eloAlways, "Loaded (%zu) states [%s]", stateDurations.size(), knownStates);
   }

   getConfigItem("stateCheckInterval", stateCheckInterval, 10);
   getConfigItem("ttyDevice", ttyDevice, "/dev/ttyUSB0");
   getConfigItem("heatingType", heatingType, "P4");
   tell(eloDetail, "The heating type is set to '%s'", heatingType);
   getConfigItem("iconSet", iconSet, "light");

   char* addrs {nullptr};
   getConfigItem("addrsDashboard", addrs, "");
   addrsDashboard = split(addrs, ',');
   getConfigItem("addrsList", addrs, "");
   addrsList = split(addrs, ',');
   free(addrs);

   getConfigItem("mail", mail, no);
   getConfigItem("mailScript", mailScript, BIN_PATH "/p4d-mail.sh");
   getConfigItem("stateMailStates", stateMailAtStates, "0,1,3,19");
   getConfigItem("stateMailTo", stateMailTo);
   getConfigItem("errorMailTo", errorMailTo);

   getConfigItem("aggregateInterval", aggregateInterval, 15);
   getConfigItem("aggregateHistory", aggregateHistory, 0);

   getConfigItem("tsync", tSync, no);
   getConfigItem("maxTimeLeak", maxTimeLeak, 10);

   getConfigItem("chart", chartSensors, "");

   std::string url = mqttUrl ? mqttUrl : "";
   getConfigItem("mqttUrl", mqttUrl);

   if (url != mqttUrl)
      mqttDisconnect();

   url = mqttHassUrl ? mqttHassUrl : "";
   getConfigItem("mqttHassUrl", mqttHassUrl);

   if (url != mqttHassUrl)
      mqttDisconnect();

   getConfigItem("mqttDataTopic", mqttDataTopic, "p4d2mqtt/sensor/<NAME>/state");
   getConfigItem("mqttUser", mqttUser, nullptr);
   getConfigItem("mqttPassword", mqttPassword, nullptr);
   getConfigItem("mqttHaveConfigTopic", mqttHaveConfigTopic, yes);
   getConfigItem("mqttDataTopic", mqttDataTopic, "p4d2mqtt/sensor/<NAME>/state");

   if (mqttDataTopic[strlen(mqttDataTopic)-1] == '/')
      mqttDataTopic[strlen(mqttDataTopic)-1] = '\0';

   if (isEmpty(mqttDataTopic) || isEmpty(mqttHassUrl))
      mqttInterfaceStyle = misNone;
   else if (strstr(mqttDataTopic, "<NAME>"))
      mqttInterfaceStyle = misMultiTopic;
   else if (strstr(mqttDataTopic, "<GROUP>"))
      mqttInterfaceStyle = misGroupedTopic;
   else
      mqttInterfaceStyle = misSingleTopic;

   for (int f = selectAllGroups->find(); f; f = selectAllGroups->fetch())
      groups[tableGroups->getIntValue("ID")].name = tableGroups->getStrValue("NAME");

   selectAllGroups->freeResult();

   return done;
}

//***************************************************************************
// Initialize
//***************************************************************************

int Daemon::initialize(bool truncate)
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

int Daemon::setup()
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
// Update Conf Tables
//***************************************************************************

int Daemon::updateSchemaConfTable()
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

int Daemon::initValueFacts()
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

   // ---------------------------------
   // Add the sensor definitions delivered by the S 3200

   for (status = request->getFirstValueSpec(&v); status != Fs::wrnLast; status = request->getNextValueSpec(&v))
   {
      if (status != success)
         continue;

      tell(eloDebug, "%3d) 0x%04x '%s' %d '%s' (%04d) '%s'",
           count, v.address, v.name, v.factor, v.unit, v.type, v.description);

      // update table

      tableValueFacts->clear();
      tableValueFacts->setValue("ADDRESS", v.address);
      tableValueFacts->setValue("TYPE", "VA");

      if (!tableValueFacts->find())
      {
         tableValueFacts->setValue("NAME", v.name);
         tableValueFacts->setValue("STATE", "D");
         tableValueFacts->setValue("UNIT", strcmp(v.unit, "°") == 0 ? "°C" : v.unit);
         tableValueFacts->setValue("FACTOR", v.factor);
         tableValueFacts->setValue("TITLE", v.description);
         tableValueFacts->setValue("RES1", v.type);
         tableValueFacts->setValue("MAXSCALE", v.unit[0] == '%' ? 100 : 300);

         tableValueFacts->store();
         added++;
      }
      else
      {
         tableValueFacts->clearChanged();
         tableValueFacts->setValue("NAME", v.name);
         tableValueFacts->setValue("UNIT", strcmp(v.unit, "°") == 0 ? "°C" : v.unit);
         tableValueFacts->setValue("FACTOR", v.factor);
         tableValueFacts->setValue("TITLE", v.description);
         tableValueFacts->setValue("RES1", v.type);

         if (tableValueFacts->getValue("MAXSCALE")->isNull())
            tableValueFacts->setValue("MAXSCALE", v.unit[0] == '%' ? 100 : 300);

         if (tableValueFacts->getChanges())
         {
            tableValueFacts->store();
            modified++;
         }
      }

      count++;
   }

   tell(eloAlways, "Read %d value facts, modified %d and added %d", count, modified, added);
   count = 0;

   tell(0, "Update example value of table valuefacts");

   for (int f = selectAllValueFacts->find(); f; f = selectAllValueFacts->fetch())
   {
      if (!tableValueFacts->hasValue("TYPE", "VA"))
         continue;

      Value v(tableValueFacts->getIntValue("ADDRESS"));

      if ((status = request->getValue(&v)) != success)
      {
         tell(eloAlways, "Getting value 0x%04x failed, error %d", v.address, status);
         continue;
      }

      double factor = tableValueFacts->getIntValue("FACTOR");
      int dataType = tableValueFacts->getIntValue("RES1");
      int value = dataType == 1 ? (word)v.value : (sword)v.value;
      double theValue = value / factor;
      tableValueFacts->setValue("VALUE", theValue);
      tableValueFacts->update();
      count++;
   }

   selectAllValueFacts->freeResult();
   tell(0, "Updated %d example values", count);

   // ---------------------------------
   // add default for digital outputs

   added = 0;
   count = 0;
   modified = 0;

   for (int f = selectAllMenuItems->find(); f; f = selectAllMenuItems->fetch())
   {
      char* name = 0;
      const char* type = 0;
      int structType = tableMenu->getIntValue("TYPE");
      std::string sname = tableMenu->getStrValue("TITLE");

      switch (structType)
      {
         case mstDigOut: type = "DO"; break;
         case mstDigIn:  type = "DI"; break;
         case mstAnlOut: type = "AO"; break;
      }

      if (!type)
         continue;

      // update table

      tableValueFacts->clear();
      tableValueFacts->setValue("ADDRESS", tableMenu->getIntValue("ADDRESS"));
      tableValueFacts->setValue("TYPE", type);
      tableValueFacts->clearChanged();

      if (!tableValueFacts->find())
      {
         tableValueFacts->setValue("STATE", "D");
         added++;
         modified--;
      }

      const char* unit = tableMenu->getStrValue("UNIT");

      if (isEmpty(unit) && strcmp(type, "AO") == 0)
         unit = "%";

      removeCharsExcept(sname, nameChars);
      asprintf(&name, "%s_0x%x", sname.c_str(), (int)tableMenu->getIntValue("ADDRESS"));

      tableValueFacts->setValue("NAME", name);
      tableValueFacts->setValue("UNIT", unit);
      tableValueFacts->setValue("FACTOR", 1);
      tableValueFacts->setValue("TITLE", tableMenu->getStrValue("TITLE"));

      if (tableValueFacts->getValue("MAXSCALE")->isNull())
         tableValueFacts->setValue("MAXSCALE", v.unit[0] == '%' ? 100 : 300);

      if (tableValueFacts->getChanges())
      {
         tableValueFacts->store();
         modified++;
      }

      free(name);
      count++;
   }

   selectAllMenuItems->freeResult();
   tell(eloAlways, "Checked %d digital lines, added %d, modified %d", count, added, modified);

   // ---------------------------------
   // add value definitions for special data

   count = 0;
   added = 0;
   modified = 0;

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", udState);      // 1  -> Kessel Status
   tableValueFacts->setValue("TYPE", "UD");            // UD -> User Defined

   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue("NAME", "Status");
      tableValueFacts->setValue("STATE", "A");
      tableValueFacts->setValue("UNIT", "zst");
      tableValueFacts->setValue("FACTOR", 1);
      tableValueFacts->setValue("TITLE", "Heizungsstatus");

      tableValueFacts->store();
      added++;
   }

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", udMode);       // 2  -> Kessel Mode
   tableValueFacts->setValue("TYPE", "UD");            // UD -> User Defined

   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue("NAME", "Betriebsmodus");
      tableValueFacts->setValue("STATE", "A");
      tableValueFacts->setValue("UNIT", "zst");
      tableValueFacts->setValue("FACTOR", 1);
      tableValueFacts->setValue("TITLE", "Betriebsmodus");

      tableValueFacts->store();
      added++;
   }

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", udTime);       // 3  -> Kessel Zeit
   tableValueFacts->setValue("TYPE", "UD");            // UD -> User Defined

   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue("NAME", "Uhrzeit");
      tableValueFacts->setValue("STATE", "A");
      tableValueFacts->setValue("UNIT", "T");
      tableValueFacts->setValue("FACTOR", 1);
      tableValueFacts->setValue("TITLE", "Datum Uhrzeit der Heizung");

      tableValueFacts->store();
      added++;
   }

   tell(eloAlways, "Added %d user defined values", added);

   return success;
}

//***************************************************************************
// Update Script Table
//***************************************************************************

int Daemon::updateScripts()
{
   char* scriptPath = 0;
   DIR* dir;
   dirent* dp;

   asprintf(&scriptPath, "%s/scripts.d", confDir);

   if (!(dir = opendir(scriptPath)))
   {
      tell(0, "Info: Script path '%s' not exists - '%s'", scriptPath, strerror(errno));
      free(scriptPath);
      return fail;
   }

   while ((dp = readdir(dir)))
   {
      char* script = 0;
      asprintf(&script, "%s/%s", scriptPath, dp->d_name);

      tableScripts->clear();
      tableScripts->setValue("PATH", script);

      free(script);

      if (dp->d_type != DT_LNK && dp->d_type != DT_REG)
         continue;

      if (!selectScript->find())
      {
         tableScripts->setValue("NAME", dp->d_name);
         tableScripts->insert();
      }
   }

   closedir(dir);
   free(scriptPath);

   return done;
}

//***************************************************************************
// Initialize Menu Structure
//***************************************************************************

int Daemon::initMenu(bool updateParameters)
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
// Update Time Range Data
//***************************************************************************

int Daemon::updateTimeRangeData()
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
// Store
//***************************************************************************

int Daemon::store(time_t now, const char* name, const char* title, const char* unit,
               const char* type, int address, double value,
               uint factor, uint groupid, const char* text)
{
   // static time_t lastHmFailAt = 0;

   double theValue = value / (double)factor;

   if (strcmp(type, "VA") == 0)
      vaValues[address] = theValue;

   tableSamples->clear();

   tableSamples->setValue("TIME", now);
   tableSamples->setValue("ADDRESS", address);
   tableSamples->setValue("TYPE", type);
   tableSamples->setValue("AGGREGATE", "S");

   tableSamples->setValue("VALUE", theValue);
   tableSamples->setValue("TEXT", text);
   tableSamples->setValue("SAMPLES", 1);

   tableSamples->store();

   // peaks

   tablePeaks->clear();

   tablePeaks->setValue("ADDRESS", address);
   tablePeaks->setValue("TYPE", type);

   if (!tablePeaks->find())
   {
      tablePeaks->setValue("MIN", theValue);
      tablePeaks->setValue("MAX", theValue);
      tablePeaks->store();
   }
   else
   {
      if (theValue > tablePeaks->getFloatValue("MAX"))
         tablePeaks->setValue("MAX", theValue);

      if (theValue < tablePeaks->getFloatValue("MIN"))
         tablePeaks->setValue("MIN", theValue);

      tablePeaks->store();
   }

   // Home Assistant

   if (mqttInterfaceStyle == misSingleTopic)
      jsonAddValue(oJson, name, title, unit, theValue, groupid, text, initialRun /*forceConfig*/);
   else if (mqttInterfaceStyle == misGroupedTopic)
      jsonAddValue(groups[groupid].oJson, name, title, unit, theValue, 0, text, initialRun /*forceConfig*/);
   else if (mqttInterfaceStyle == misMultiTopic)
      mqttPublishSensor(iotSensor, name, title, unit, theValue, text, initialRun /*forceConfig*/);

   return success;
}

//***************************************************************************
// Schedule Time Sync In
//***************************************************************************

void Daemon::scheduleTimeSyncIn(int offset)
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
// standby
//***************************************************************************

int Daemon::standby(int t)
{
   time_t end = time(0) + t;

   while (time(0) < end && !doShutDown())
   {
      meanwhile();
      usleep(50000);
   }

   return done;
}

int Daemon::standbyUntil(time_t until)
{
   while (time(0) < until && !doShutDown())
   {
      meanwhile();
      loopCondition.TimedWait(loopMutex, 1000);
   }

   return done;
}

//***************************************************************************
// Meanwhile
//***************************************************************************

int Daemon::meanwhile()
{
   if (!initialized)
      return done;

   if (!connection || !connection->isConnected())
      return fail;

   tell(3, "loop ...");

   dispatchClientRequest();

   if (!isEmpty(mqttHassUrl))
      performMqttRequests();

   return done;
}

//***************************************************************************
// Loop
//***************************************************************************

int Daemon::loop()
{
   int status;
   time_t nextStateAt {0};
   int lastState {na};

   loopMutex.Lock();

   // info

   if (mail && !isEmpty(stateMailTo))
      tell(eloAlways, "Mail at states '%s' to '%s'", stateMailAtStates, stateMailTo);

   if (mail && !isEmpty(errorMailTo))
      tell(eloAlways, "Mail at errors to '%s'", errorMailTo);

   // init

   scheduleAggregate();

   sem->p();
   serial->open(ttyDevice);
   sem->v();

   while (!doShutDown())
   {
      stateChanged = false;

      // check db connection

      while (!doShutDown() && (!connection || !connection->isConnected()))
      {
         if (initDb() == success)
            break;
         else
            exitDb();

         tell(eloAlways, "Retrying in 10 seconds");
         standby(10);
      }

      if (doShutDown())
         break;

      meanwhile();
      standbyUntil(min(nextStateAt, nextRefreshAt));

      // aggregate

      if (aggregateHistory && nextAggregateAt <= time(0))
         aggregate();

      // update/check state

      status = updateState(&currentState);

      if (status != success)
      {
         sem->p();
         serial->close();
         tell(eloAlways, "Error reading serial interface, reopen now!");
         status = serial->open(ttyDevice);
         sem->v();

         if (status != success)
         {
            tell(eloAlways, "Retrying in 10 seconds");
            standby(10);
         }

         continue;
      }

      stateChanged = lastState != currentState.state;

      if (stateChanged)
      {
         mailBodyHtml = "";
         lastState = currentState.state;
         nextRefreshAt = time(0);              // force on state change
         tell(eloAlways, "State changed to '%s'", currentState.stateinfo);
      }

      nextStateAt = stateCheckInterval ? time(0) + stateCheckInterval : nextRefreshAt;

      // work expected?

      if (time(0) < nextRefreshAt)
         continue;

      // check serial connection

      sem->p();
      status = request->check();
      sem->v();

      if (status != success)
      {
         sem->p();
         serial->close();
         tell(eloAlways, "Error reading serial interface, reopen now");
         serial->open(ttyDevice);
         sem->v();

         continue;
      }

      // perform update

      nextRefreshAt = time(0) + interval;
      nextStateAt = stateCheckInterval ? time(0) + stateCheckInterval : nextRefreshAt;
      calcStateDuration();

      {
         sem->p();
         update();
         updateErrors();
         sem->v();
      }

      afterUpdate();

      if (stateChanged && mail)
      {
         sendStateMail();

         if (errorsPending)
            sendErrorMail();
      }

      initialRun = false;
   }

   serial->close();
   loopMutex.Unlock();

   return success;
}

//***************************************************************************
// Update State
//***************************************************************************

int Daemon::updateState(Status* state)
{
   static time_t nextReportAt = 0;

   int status;
   time_t now;

   // get state

   sem->p();
   tell(eloDetail, "Checking state ...");
   status = request->getStatus(state);
   now = time(0);
   sem->v();

   if (status != success)
      return status;

   tell(eloDetail, "... got (%d) '%s'%s", state->state, toTitle(state->state),
        isError(state->state) ? " -> Störung" : "");

   // ----------------------
   // check time sync

   if (!nextTimeSyncAt)
      scheduleTimeSyncIn();

   if (tSync && maxTimeLeak && labs(state->time - now) > maxTimeLeak)
   {
      if (now > nextReportAt)
      {
         tell(eloAlways, "Time drift is %ld seconds", state->time - now);
         nextReportAt = now + 2 * tmeSecondsPerMinute;
      }

      if (now > nextTimeSyncAt)
      {
         scheduleTimeSyncIn(tmeSecondsPerDay);

         tell(eloAlways, "Time drift is %ld seconds, syncing now", state->time - now);

         sem->p();

         if (request->syncTime() == success)
            tell(eloAlways, "Time sync succeeded");
         else
            tell(eloAlways, "Time sync failed");

         sleep(2);   // S-3200 need some seconds to store time :o

         status = request->getStatus(state);
         now = time(0);

         sem->v();

         tell(eloAlways, "Time drift now %ld seconds", state->time - now);
      }
   }

   return status;
}

//***************************************************************************
// Update
//***************************************************************************

int Daemon::update(bool webOnly, long client)
{
   static size_t w1Count = 0;
   time_t now = time(0);
   int count = 0;
   int status;
   char num[100];

   if (!webOnly)
   {
      if (w1Count < w1Sensors.size())
      {
         initW1();
         w1Count = w1Sensors.size();
      }
   }

   tell(eloDetail, "Reading values ...");

   tableValueFacts->clear();
   tableValueFacts->setValue("STATE", "A");

   if (!webOnly)
      connection->startTransaction();

   for (auto sj : jsonSensorList)
      json_decref(sj.second);

   jsonSensorList.clear();

   if (mqttInterfaceStyle == misSingleTopic)
       oJson = json_object();

   for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
   {
      int addr = tableValueFacts->getIntValue("ADDRESS");
      double factor = tableValueFacts->getIntValue("FACTOR");
      const char* title = tableValueFacts->getStrValue("TITLE");
      const char* usrtitle = tableValueFacts->getStrValue("USRTITLE");
      const char* type = tableValueFacts->getStrValue("TYPE"); // VA, DI, ...
      int dataType = tableValueFacts->getIntValue("RES1");
      const char* unit = tableValueFacts->getStrValue("UNIT");
      const char* name = tableValueFacts->getStrValue("NAME");
      uint groupid = tableValueFacts->getIntValue("GROUPID");
      const char* orgTitle = title;

      if (!isEmpty(usrtitle))
         title = usrtitle;

      if (mqttInterfaceStyle == misGroupedTopic)
      {
         if (!groups[groupid].oJson)
            groups[groupid].oJson = json_object();
      }

      json_t* ojData = json_object();
      char* tupel {nullptr};
      asprintf(&tupel, "%s:0x%02x", type, addr);
      jsonSensorList[tupel] = ojData;
      free(tupel);

      sensor2Json(ojData, tableValueFacts);

      if (tableValueFacts->hasValue("TYPE", "VA"))
      {
         Value v(addr);

         if ((status = request->getValue(&v)) != success)
         {
            tell(eloAlways, "Getting value 0x%04x failed, error %d", addr, status);
            continue;
         }

         int value = dataType == 1 ? (word)v.value : (sword)v.value;
         double theValue = value / factor;

         json_object_set_new(ojData, "value", json_real(theValue));
         json_object_set_new(ojData, "image", json_string(getImageFor(orgTitle, theValue)));

         if (!webOnly)
         {
            store(now, name, title, unit, type, v.address, value, factor, groupid);
            sprintf(num, "%.2f%s", theValue, unit);
            addParameter2Mail(title, num);
         }
      }

      else if (sensorScript && tableValueFacts->hasValue("TYPE", "SC"))
      {
         std::string txt = getScriptSensor(addr).c_str();

         if (!txt.empty())
         {
            double value = strtod(txt.c_str(), 0);

            tell(eloDebug, "Debug: Got '%s' (%.2f) for (%d) from script", txt.c_str(), value, addr);
            json_object_set_new(ojData, "value", json_real(value / factor));

            if (!webOnly)
            {
               store(now, name, title, unit, type, addr, value, factor, groupid);
               sprintf(num, "%.2f%s", value / factor, unit);
               addParameter2Mail(title, num);
            }
         }
      }

      else if (tableValueFacts->hasValue("TYPE", "DO"))
      {
         Fs::IoValue v(addr);

         if (request->getDigitalOut(&v) != success)
         {
            tell(eloAlways, "Getting digital out 0x%04x failed, error %d", addr, status);
            continue;
         }

         json_object_set_new(ojData, "value", json_integer(v.state));
         json_object_set_new(ojData, "image", json_string(getImageFor(orgTitle, v.state)));

         if (!webOnly)
         {
            store(now, name, title, unit, type, v.address, v.state, factor, groupid);
            sprintf(num, "%d", v.state);
            addParameter2Mail(title, num);
         }
      }

      else if (tableValueFacts->hasValue("TYPE", "DI"))
      {
         Fs::IoValue v(addr);

         if (request->getDigitalIn(&v) != success)
         {
            tell(eloAlways, "Getting digital in 0x%04x failed, error %d", addr, status);
            continue;
         }

         json_object_set_new(ojData, "value", json_integer(v.state));
         json_object_set_new(ojData, "image", json_string(getImageFor(orgTitle, v.state)));

         if (!webOnly)
         {
            store(now, name, title, unit, type, v.address, v.state, factor, groupid);
            sprintf(num, "%d", v.state);
            addParameter2Mail(title, num);
         }
      }

      else if (tableValueFacts->hasValue("TYPE", "AO"))
      {
         Fs::IoValue v(addr);

         if (request->getAnalogOut(&v) != success)
         {
            tell(eloAlways, "Getting analog out 0x%04x failed, error %d", addr, status);
            continue;
         }

         json_object_set_new(ojData, "value", json_integer(v.state));

         if (!webOnly)
         {
            store(now, name, title, unit, type, v.address, v.state, factor, groupid);
            sprintf(num, "%d", v.state);
            addParameter2Mail(title, num);
         }
      }

      else if (tableValueFacts->hasValue("TYPE", "W1"))
      {
         bool w1Exist = existW1(name);
         time_t w1Last {0};
         double w1Value {0};

         if (w1Exist)
            w1Value = valueOfW1(name, w1Last);

         // exist and updated at least once in last 5 minutes?

         if (w1Exist && w1Last > time(0)-300)
         {
            json_object_set_new(ojData, "value", json_real(w1Value));

            if (!webOnly)
               store(now, name, title, unit, type, addr, w1Value, factor, groupid);
            // store(now, name, title, unit, type, addr, w1Value);
         }
         else
         {
            if (!w1Exist)
               tell(eloAlways, "Warning: W1 sensor '%s' missing", name);
            else
               tell(eloAlways, "Warning: Data of W1 sensor '%s' seems to be to old (%s)", name, l2pTime(w1Last).c_str());

            json_object_set_new(ojData, "text", json_string("missing sensor"));
         }

         // double value = w1.valueOf(name);

         // json_object_set_new(ojData, "value", json_real(value));

         // if (!webOnly)
         // {
         //    store(now, name, title, unit, type, addr, value, factor, groupid);
         //    sprintf(num, "%.2f%s", value / factor, unit);
         //    addParameter2Mail(title, num);
         // }
      }

      else if (tableValueFacts->hasValue("TYPE", "SD"))   // state duration
      {
         const auto it = stateDurations.find(addr);

         if (it == stateDurations.end())
            continue;

         double value = stateDurations[addr] / 60;

         json_object_set_new(ojData, "value", json_real(value));

         if (!webOnly)
         {
            store(now, name, title, unit, type, addr, value, factor, groupid);
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
                  store(now, name, unit, title, type, udState, currentState.state, factor, groupid, currentState.stateinfo);
                  addParameter2Mail(title, currentState.stateinfo);
               }

               break;
            }
            case udMode:
            {
               json_object_set_new(ojData, "text", json_string(currentState.modeinfo));

               if (!webOnly)
               {
                  store(now, name, title, unit, type, udMode, currentState.mode, factor, groupid, currentState.modeinfo);
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
                  store(now, name, title, unit, type, udTime, currentState.time, factor, groupid, date.c_str());
                  addParameter2Mail(title, date.c_str());
               }

               break;
            }
         }
      }

      count++;
   }

   selectActiveValueFacts->freeResult();

   if (!webOnly)
   {
      connection->commit();
      tell(eloAlways, "Processed %d samples, state is '%s'", count, currentState.stateinfo);
   }

   // send result to all connected WEBIF clients

   pushDataUpdate(webOnly ? "init" : "all", client);

   tell(1, "MQTT InterfaceStyle is %d", mqttInterfaceStyle);

   // MQTT

   if (mqttInterfaceStyle == misSingleTopic)
   {
      mqttWrite(oJson, 0);
      json_decref(oJson);
      oJson = nullptr;
   }

   else if (mqttInterfaceStyle == misGroupedTopic)
   {
      tell(1, "Writing MQTT for %zu groups", groups.size());

      for (auto it : groups)
      {
         if (it.second.oJson)
         {
            mqttWrite(it.second.oJson, it.first);
            json_decref(groups[it.first].oJson);
            groups[it.first].oJson = nullptr;
         }
      }
   }

   if (!webOnly)
      sensorAlertCheck(now);

   return success;
}

//***************************************************************************
// Calc State Duration
//***************************************************************************

int Daemon::calcStateDuration()
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
                   "min", (std::string(text)+" (Laufzeit/Tag)").c_str(), false, 2000);

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
// Get Script Sensor
//***************************************************************************

std::string Daemon::getScriptSensor(int address)
{
   char* cmd {nullptr};

   if (!sensorScript)
      return "";

   asprintf(&cmd, "%s %d", sensorScript, address);
   tell(0, "Calling '%s'", cmd);
   std::string s = executeCommand(cmd);

   free(cmd);

   return s;
}

//***************************************************************************
// After Update
//***************************************************************************

void Daemon::afterUpdate()
{
   char* path = 0;

   asprintf(&path, "%s/after-update.sh", confDir);

   if (fileExists(path))
   {
      tell(0, "Calling '%s'", path);
      system(path);
   }

   free(path);
}

//***************************************************************************
// Dispatch Mqtt Command Request
//   Format:  '{ "command" : "parstore", "address" : 0, "value" : "9" }'
//***************************************************************************

int Daemon::dispatchMqttCommandRequest(const char* jString)
{
   json_error_t error;
   json_t* jData = json_loads(jString, 0, &error);

   if (!jData)
   {
      tell(0, "Error: Ignoring invalid json in [%s]", jString);
      tell(0, "Error decoding json: %s (%s, line %d column %d, position %d)",
           error.text, error.source, error.line, error.column, error.position);
      return fail;
   }

   const char* command = getStringFromJson(jData, "command", "");

   if (isEmpty(command))
   {
      tell(0, "Error: Missing 'command' in MQTT request [%s], ignoring", jString);
   }
   else if (strcmp(command, "parget") == 0)
   {
      int status {fail};
      json_t* jAddress = getObjectFromJson(jData, "address");
      int address = getIntFromJson(jData, "address", -1);

      if (!json_is_integer(jAddress) || address == -1)
      {
         tell(0, "Error: Missing address or invalid object type for MQTT command 'parstore' in [%s], ignoring", jString);
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
         tell(0, "Error: Missing address or invalid object type for MQTT command 'parstore' in [%s], ignoring", jString);
         return fail;
      }

      if (isEmpty(value))
      {
         tell(0, "Error: Missing value for MQTT command 'parstore' in [%s], ignoring", jString);
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
      tell(0, "Error: Got unexpected command '%s' in MQTT request [%s], ignoring", command, jString);
   }

   return success;
}

//***************************************************************************
// Sensor Alert Check
//***************************************************************************

void Daemon::sensorAlertCheck(time_t now)
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
// Perform Alert Check
//***************************************************************************

int Daemon::performAlertCheck(cDbRow* alertRow, time_t now, int recurse, int force)
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
// Add Parameter To Mail
//***************************************************************************

void Daemon::addParameter2Mail(const char* name, const char* value)
{
   if (stateChanged)
      mailBodyHtml += "        <tr><td>" + std::string(name) + "</td><td>" + std::string(value) + "</td></tr>\n";
}

//***************************************************************************
// Schedule Aggregate
//***************************************************************************

int Daemon::scheduleAggregate()
{
   struct tm tm = { 0 };
   time_t now;

   if (!aggregateHistory)
   {
      tell(0, "NO aggregateHistory configured!");
      return done;
   }

   // calc today at 01:00:00

   now = time(0);
   localtime_r(&now, &tm);

   tm.tm_sec = 0;
   tm.tm_min = 0;
   tm.tm_hour = 1;
   tm.tm_isdst = -1;               // force DST auto detect

   nextAggregateAt = mktime(&tm);

   // if in the past ... skip to next day ...

   if (nextAggregateAt <= time(0))
      nextAggregateAt += tmeSecondsPerDay;

   tell(eloAlways, "Scheduled aggregation for '%s' with interval of %d minutes",
        l2pTime(nextAggregateAt).c_str(), aggregateInterval);

   return success;
}

//***************************************************************************
// Aggregate
//***************************************************************************

int Daemon::aggregate()
{
   char* stmt = 0;
   time_t history = time(0) - (aggregateHistory * tmeSecondsPerDay);
   int aggCount = 0;

   asprintf(&stmt,
            "replace into samples "
            "  select address, type, 'A' as aggregate, "
            "    CONCAT(DATE(time), ' ', SEC_TO_TIME((TIME_TO_SEC(time) DIV %d) * %d)) + INTERVAL %d MINUTE time, "
            "    unix_timestamp(sysdate()) as inssp, unix_timestamp(sysdate()) as updsp, "
            "    round(sum(value)/count(*), 2) as value, text, count(*) samples "
            "  from "
            "    samples "
            "  where "
            "    aggregate != 'A' and "
            "    time <= from_unixtime(%ld) "
            "  group by "
            "    CONCAT(DATE(time), ' ', SEC_TO_TIME((TIME_TO_SEC(time) DIV %d) * %d)) + INTERVAL %d MINUTE, address, type;",
            aggregateInterval * tmeSecondsPerMinute, aggregateInterval * tmeSecondsPerMinute, aggregateInterval,
            history,
            aggregateInterval * tmeSecondsPerMinute, aggregateInterval * tmeSecondsPerMinute, aggregateInterval);

   tell(eloAlways, "Starting aggregation ...");

   if (connection->query(aggCount, "%s", stmt) == success)
   {
      int delCount = 0;

      tell(eloDebug, "Aggregation: [%s]", stmt);
      free(stmt);

      // Einzelmesspunkte löschen ...

      asprintf(&stmt, "aggregate != 'A' and time <= from_unixtime(%ld)", history);

      if (tableSamples->deleteWhere(stmt, delCount) == success)
      {
         tell(eloAlways, "Aggregation with interval of %d minutes done; "
              "Created %d aggregation rows, deleted %d sample rows",
              aggregateInterval, aggCount, delCount);
      }
   }

   free(stmt);

   // schedule even in case of error!

   scheduleAggregate();

   return success;
}

//***************************************************************************
// Update Errors
//***************************************************************************

int Daemon::updateErrors()
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

int Daemon::updateParameter(cDbTable* tableMenu)
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
         int dataType = tableValueFacts->getIntValue("RES1");

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
// Send Mail
//***************************************************************************

int Daemon::sendAlertMail(const char* to)
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

int Daemon::add2AlertMail(cDbRow* alertRow, const char* title, double value, const char* unit)
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
// Send Error Mail
//***************************************************************************

int Daemon::sendErrorMail()
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
// Send State Mail
//***************************************************************************

int Daemon::sendStateMail()
{
   std::string subject = "Heizung - Status: " + std::string(currentState.stateinfo);

   // check

   if (!isMailState() || isEmpty(mailScript) || !mailBodyHtml.length() || isEmpty(stateMailTo))
      return done;

   // HTML mail

   char* html = 0;

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

int Daemon::sendMail(const char* receiver, const char* subject, const char* body, const char* mimeType)
{
   char* command = {nullptr};
   int result {0};

   asprintf(&command, "%s '%s' '%s' '%s' '%s'", mailScript, subject, body, mimeType, receiver);
   result = system(command);
   free(command);

   if (loglevel >= eloDebug)
      tell(eloAlways, "Send mail '%s' with [%s] to '%s'", subject, body, receiver);
   else
      tell(eloAlways, "Send mail '%s' to '%s'", subject, receiver);

   return result;
}

//***************************************************************************
// Is Mail State
//***************************************************************************

int Daemon::isMailState()
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
// Load Html Header
//***************************************************************************

int Daemon::loadHtmlHeader()
{
   char* file;

   // load only once at first call

   if (!htmlHeader.isEmpty())
      return done;

   asprintf(&file, "%s/mail-head.html", confDir);

   if (fileExists(file))
      if (loadFromFile(file, &htmlHeader) == success)
         htmlHeader.append("\0");

   free(file);

   if (!htmlHeader.isEmpty())
      return success;

   htmlHeader.clear();

   htmlHeader.append("  <head>\n"
                     "    <style type=\"text/css\">\n"
                     "      table {"
                     "        border: 1px solid #d2d2d2;\n"
                     "        border-collapse: collapse;\n"
                     "      }\n"
                     "      table tr.head {\n"
                     "        background-color: #004d8f;\n"
                     "        color: #fff;\n"
                     "        font-weight: bold;\n"
                     "        font-family: Helvetica, Arial, sans-serif;\n"
                     "        font-size: 12px;\n"
                     "      }\n"
                     "      table tr th,\n"
                     "      table tr td {\n"
                     "        padding: 4px;\n"
                     "        text-align: left;\n"
                     "      }\n"
                     "      table tr:nth-child(1n) td {\n"
                     "        background-color: #fff;\n"
                     "      }\n"
                     "      table tr:nth-child(2n+2) td {\n"
                     "        background-color: #eee;\n"
                     "      }\n"
                     "      td {\n"
                     "        color: #333;\n"
                     "        font-family: Helvetica, Arial, sans-serif;\n"
                     "        font-size: 12px;\n"
                     "        border: 1px solid #D2D2D2;\n"
                     "      }\n"
                     "      </style>\n"
                     "  </head>\n");
   htmlHeader.append('\0');

   return success;
}

//***************************************************************************
// Add Value Fact
//***************************************************************************

int Daemon::addValueFact(int addr, const char* type, const char* name, const char* unit,
                        WidgetType widgetType, int minScale, int maxScale, int rights, const char* choices)
{
   if (maxScale == na)
      maxScale = unit[0] == '%' ? 100 : 45;

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", addr);
   tableValueFacts->setValue("TYPE", type);

   if (!tableValueFacts->find())
   {
      tell(0, "Add ValueFact '%ld' '%s'", tableValueFacts->getIntValue("ADDRESS"), tableValueFacts->getStrValue("TYPE"));

      tableValueFacts->setValue("NAME", name);
      tableValueFacts->setValue("TITLE", name);
      tableValueFacts->setValue("RIGHTS", rights);
      tableValueFacts->setValue("STATE", "D");
      tableValueFacts->setValue("UNIT", unit);

      if (!isEmpty(choices))
         tableValueFacts->setValue("CHOICES", choices);

      char* opt {nullptr};
      asprintf(&opt, "{\"unit\": \"%s\", \"scalemax\": %d, \"scalemin\": %d, \"scalestep\": %d, \"imgon\": \"%s\", \"imgoff\": \"%s\", \"widgettype\": %d}",
               unit, maxScale, minScale, 0, getImageFor(name, true), getImageFor(name, false), widgetType);
      tableValueFacts->setValue("WIDGETOPT", opt);
      free(opt);

      tableValueFacts->store();
      return 1;                               // 1 for 'added'
   }

   tableValueFacts->clearChanged();

   tableValueFacts->setValue("NAME", name);
   tableValueFacts->setValue("TITLE", name);

   if (!isEmpty(choices))
      tableValueFacts->setValue("CHOICES", choices);

   if (tableValueFacts->getValue("RIGHTS")->isNull())
      tableValueFacts->setValue("RIGHTS", rights);

   if (tableValueFacts->getChanges())
   {
      tableValueFacts->store();
      return 2;                                // 2 for 'modified'
   }

   return done;
}

//***************************************************************************
// Add Value Fact
//***************************************************************************

int Daemon::addValueFact(int addr, const char* type, int factor, const char* name,
                      const char* unit, const char* title, bool active, int maxScale)
{
   if (maxScale == na)
      maxScale = unit[0] == '%' ? 100 : 45;

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", addr);
   tableValueFacts->setValue("TYPE", type);

   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue("NAME", name);
      tableValueFacts->setValue("STATE", active ? "A" : "D");
      tableValueFacts->setValue("UNIT", unit);
      tableValueFacts->setValue("TITLE", title);
      tableValueFacts->setValue("FACTOR", factor);
      tableValueFacts->setValue("MAXSCALE", maxScale);

      tableValueFacts->store();
      tell(2, "Inserted valuefact %s:%d", type, addr);
      return 1;    // 1 for 'added'
   }
   else
   {
      tableValueFacts->clearChanged();

      tableValueFacts->setValue("NAME", name);
      tableValueFacts->setValue("UNIT", unit);
      tableValueFacts->setValue("TITLE", title);
      tableValueFacts->setValue("FACTOR", factor);

      if (tableValueFacts->getValue("MAXSCALE")->isNull())
         tableValueFacts->setValue("MAXSCALE", maxScale);

      if (tableValueFacts->getChanges())
      {
         tableValueFacts->store();
         tell(2, "Updated valuefact %s:%d", type, addr);
         return 2;  // 2 for 'modified'
      }
   }

   return fail;
}

//***************************************************************************
// Stored Parameters
//***************************************************************************

int Daemon::getConfigItem(const char* name, char*& value, const char* def)
{
   free(value);
   value = nullptr;

   tableConfig->clear();
   tableConfig->setValue("OWNER", myName());
   tableConfig->setValue("NAME", name);

   if (tableConfig->find())
   {
      value = strdup(tableConfig->getStrValue("VALUE"));
   }
   else if (def)  // only if not a nullptr
   {
      value = strdup(def);
      setConfigItem(name, value);  // store the default
   }

   tableConfig->reset();

   return success;
}

int Daemon::setConfigItem(const char* name, const char* value)
{
   tell(2, "Debug: Storing '%s' with value '%s'", name, value);
   tableConfig->clear();
   tableConfig->setValue("OWNER", myName());
   tableConfig->setValue("NAME", name);
   tableConfig->setValue("VALUE", value);

   return tableConfig->store();
}

int Daemon::getConfigItem(const char* name, int& value, int def)
{
   return getConfigItem(name, (long&)value, (long)def);
}

int Daemon::getConfigItem(const char* name, long& value, long def)
{
   char* txt {nullptr};

   getConfigItem(name, txt);

   if (!isEmpty(txt))
      value = atoi(txt);
   else if (isEmpty(txt) && def != na)
   {
      value = def;
      setConfigItem(name, value);
   }
   else
      value = 0;

   free(txt);

   return success;
}

int Daemon::setConfigItem(const char* name, long value)
{
   char txt[16];

   snprintf(txt, sizeof(txt), "%ld", value);

   return setConfigItem(name, txt);
}

int Daemon::getConfigItem(const char* name, double& value, double def)
{
   char* txt {nullptr};

   getConfigItem(name, txt);

   if (!isEmpty(txt))
   {
      std::string s = strReplace(".", ",", txt);  // #TODO needed ??
      value = strtod(s.c_str(), nullptr);
   }
   else if (isEmpty(txt) && def != na)
   {
      value = def;
      setConfigItem(name, value);
   }
   else
      value = 0.0;

   free(txt);

   return success;
}

int Daemon::setConfigItem(const char* name, double value)
{
   char txt[16+TB];

   snprintf(txt, sizeof(txt), "%.2f", value);

   return setConfigItem(name, txt);
}

int Daemon::getConfigItem(const char* name, bool& value, bool def)
{
   char* txt {nullptr};

   getConfigItem(name, txt);

   if (!isEmpty(txt))
      value = atoi(txt);
   else if (isEmpty(txt))
   {
      value = def;
      setConfigItem(name, value);
   }

   free(txt);

   return success;
}

int Daemon::setConfigItem(const char* name, bool value)
{
   char txt[16];

   snprintf(txt, sizeof(txt), "%d", value);

   return setConfigItem(name, txt);
}

//***************************************************************************
// Get Config Time Range Item
//***************************************************************************

int Daemon::getConfigTimeRangeItem(const char* name, std::vector<Range>& ranges)
{
   char* tmp {nullptr};

   getConfigItem(name, tmp, "");
   ranges.clear();

   for (const char* r = strtok(tmp, ","); r; r = strtok(0, ","))
   {
      uint fromHH, fromMM, toHH, toMM;

      if (sscanf(r, "%u:%u-%u:%u", &fromHH, &fromMM, &toHH, &toMM) == 4)
      {
         uint from = fromHH*100 + fromMM;
         uint to = toHH*100 + toMM;

         ranges.push_back({from, to});
         tell(3, "range: %d - %d", from, to);
      }
      else
      {
         tell(0, "Error: Unexpected range '%s' for '%s'", r, name);
      }
   }

   free(tmp);

   return success;
}

//***************************************************************************
// Get Image For
//***************************************************************************

const char* Daemon::getImageFor(const char* title, int value)
{
   const char* imagePath = "img/icon/unknown.png";

   if (strcasestr(title, "Pump"))
      imagePath = value ? "img/icon/pump-on.gif" : "img/icon/pump-off.png";
   else if (strcasestr(title, "Steckdose"))
      imagePath = value ? "img/icon/plug-on.png" : "img/icon/plug-off.png";
   else if (strcasestr(title, "UV-C"))
      imagePath = value ? "img/icon/uvc-on.png" : "img/icon/uvc-off.png";
   else if (strcasestr(title, "Licht") || strcasestr(title, "Light"))
      imagePath = value ? "img/icon/light-on.png" : "img/icon/light-off.png";
   else if (strcasestr(title, "Shower") || strcasestr(title, "Dusche"))
      imagePath = value ? "img/icon/shower-on.png" : "img/icon/shower-off.png";
   else
      imagePath = value ? "img/icon/boolean-on.png" : "img/icon/boolean-off.png";

   return imagePath;
}

const char* Daemon::getStateImage(int state)
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

//***************************************************************************
// Digital IO Stuff
//***************************************************************************

int Daemon::toggleIo(uint addr, const char* type)
{
   cDbRow* fact = valueFactOf(type, addr);

   if (!fact)
      return fail;

   const char* name = fact->getStrValue("NAME");
   const char* title = fact->getStrValue("USRTITLE");

   if ((isEmpty(title)))
      title = fact->getStrValue("TITLE");

   if (strcmp(type, "DO") == 0)
   {
      gpioWrite(addr, !digitalOutputStates[addr].state);
   }
   else if (strcmp(type, "SC") == 0)
   {
      callScript(addr, "toggle", name, title);
   }

   return success;
}

int Daemon::toggleIoNext(uint pin)
{
   if (digitalOutputStates[pin].state)
   {
      toggleIo(pin, "DO");
      usleep(300000);
      toggleIo(pin, "DO");
      return success;
   }

   gpioWrite(pin, true);

   return success;
}

void Daemon::pin2Json(json_t* ojData, int pin)
{
   json_object_set_new(ojData, "address", json_integer(pin));
   json_object_set_new(ojData, "type", json_string("DO"));
   json_object_set_new(ojData, "name", json_string(digitalOutputStates[pin].name));
   json_object_set_new(ojData, "title", json_string(digitalOutputStates[pin].title.c_str()));
   json_object_set_new(ojData, "mode", json_string(digitalOutputStates[pin].mode == omManual ? "manual" : "auto"));
   json_object_set_new(ojData, "options", json_integer(digitalOutputStates[pin].opt));
   json_object_set_new(ojData, "value", json_integer(digitalOutputStates[pin].state));
   json_object_set_new(ojData, "last", json_integer(digitalOutputStates[pin].last));
   json_object_set_new(ojData, "next", json_integer(digitalOutputStates[pin].next));
   json_object_set_new(ojData, "image", json_string(getImageFor(digitalOutputStates[pin].title.c_str(), digitalOutputStates[pin].state)));
   json_object_set_new(ojData, "widgettype", json_integer(wtSymbol));  // #TODO get type from valuefacts
}

int Daemon::toggleOutputMode(uint pin)
{
   // allow mode toggle only if more than one option is given

   if (digitalOutputStates[pin].opt & ooAuto && digitalOutputStates[pin].opt & ooUser)
   {
      OutputMode mode = digitalOutputStates[pin].mode == omAuto ? omManual : omAuto;
      digitalOutputStates[pin].mode = mode;

      storeStates();

      json_t* oJson = json_array();
      json_t* ojData = json_object();
      json_array_append_new(oJson, ojData);
      pin2Json(ojData, pin);

      pushOutMessage(oJson, "update");
   }

   return success;
}

void Daemon::gpioWrite(uint pin, bool state, bool store)
{
   digitalOutputStates[pin].state = state;
   digitalOutputStates[pin].last = time(0);
   digitalOutputStates[pin].valid = true;

   if (!state)
      digitalOutputStates[pin].next = 0;

   // invert the state on 'invertDO' - some relay board are active at 'false'

   digitalWrite(pin, invertDO ? !state : state);

   if (store)
      storeStates();

   performJobs();

   // send update to WS
   {
      json_t* oJson = json_array();
      json_t* ojData = json_object();
      json_array_append_new(oJson, ojData);
      pin2Json(ojData, pin);

      pushOutMessage(oJson, "update");
   }

   mqttPublishSensor(iotLight, digitalOutputStates[pin].name, "", "", digitalOutputStates[pin].state, "", false /*forceConfig*/);
}

bool Daemon::gpioRead(uint pin)
{
   digitalInputStates[pin] = digitalRead(pin);
   return digitalInputStates[pin];
}

//***************************************************************************
// Store/Load States to DB
//  used to recover at restart
//***************************************************************************

int Daemon::storeStates()
{
   long value {0};
   long mode {0};

   for (const auto& output : digitalOutputStates)
   {
      if (output.second.state)
         value += pow(2, output.first);

      if (output.second.mode == omManual)
         mode += pow(2, output.first);

      setConfigItem("ioStates", value);
      setConfigItem("ioModes", mode);

      // tell(0, "state bit (%d): %s: %d [%ld]", output.first, output.second.name, output.second.state, value);
   }

   return done;
}

int Daemon::loadStates()
{
   long value {0};
   long mode {0};

   getConfigItem("ioStates", value, 0);
   getConfigItem("ioModes", mode, 0);

   tell(2, "Debug: Loaded iostates: %ld", value);

   for (const auto& output : digitalOutputStates)
   {
      if (digitalOutputStates[output.first].opt & ooUser)
      {
         gpioWrite(output.first, value & (long)pow(2, output.first), false);
         tell(0, "Info: IO %s/%d recovered to %d", output.second.name, output.first, output.second.state);
      }
   }

   if (mode)
   {
      for (const auto& output : digitalOutputStates)
      {
         if (digitalOutputStates[output.first].opt & ooAuto && digitalOutputStates[output.first].opt & ooUser)
         {
            OutputMode m = mode & (long)pow(2, output.first) ? omManual : omAuto;
            digitalOutputStates[output.first].mode = m;
         }
      }
   }

   return done;
}

//***************************************************************************
// W1 Stuff ...
//***************************************************************************

int Daemon::dispatchW1Msg(const char* message)
{
   json_error_t error;
   json_t* jArray = json_loads(message, 0, &error);

   if (!jArray)
   {
      tell(0, "Error: Can't parse json in '%s'", message);
      return fail;
   }

   size_t index {0};
   json_t* jValue {nullptr};

   json_array_foreach(jArray, index, jValue)
   {
      const char* name = getStringFromJson(jValue, "name");
      double value = getDoubleFromJson(jValue, "value");
      time_t stamp = getIntFromJson(jValue, "time");

      if (stamp < time(0)-300)
      {
         tell(eloAlways, "Skipping old (%ld seconds) w1 value", time(0)-stamp);
         continue;
      }

      updateW1(name, value, stamp);
   }

   cleanupW1();
   process();

   return success;
}

void Daemon::updateW1(const char* id, double value, time_t stamp)
{
   tell(2, "w1: %s : %0.2f", id, value);

   w1Sensors[id].value = value;
   w1Sensors[id].last = stamp;

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", (int)toW1Id(id));
   tableValueFacts->setValue("TYPE", "W1");

   if (tableValueFacts->find())
   {
      json_t* ojData = json_object();

      sensor2Json(ojData, tableValueFacts);
      json_object_set_new(ojData, "value", json_real(value));

      char* tuple {nullptr};
      asprintf(&tuple, "W1:0x%02x", toW1Id(id));
      jsonSensorList[tuple] = ojData;
      free(tuple);

      pushDataUpdate("update", 0L);
   }

   tableValueFacts->reset();
}

void Daemon::cleanupW1()
{
   uint detached {0};

   for (auto it = w1Sensors.begin(); it != w1Sensors.end();)
   {
      if (it->second.last < time(0) - 5*tmeSecondsPerMinute)
      {
         tell(0, "Info: Missing sensor '%s', removing it from list", it->first.c_str());
         detached++;
         it = w1Sensors.erase(it--);
      }
      else
         it++;
   }

   if (detached)
   {
      tell(0, "Info: %d w1 sensors detached, reseting power line to force a re-initialization", detached);
      gpioWrite(pinW1Power, false);
      sleep(2);
      gpioWrite(pinW1Power, true);
   }
}

bool Daemon::existW1(const char* id)
{
   if (isEmpty(id))
      return false;

   auto it = w1Sensors.find(id);

   return it != w1Sensors.end();
}

double Daemon::valueOfW1(const char* id, time_t& last)
{
   last = 0;

   if (isEmpty(id))
      return 0;

   auto it = w1Sensors.find(id);

   if (it == w1Sensors.end())
      return 0;

   last = w1Sensors[id].last;

   return w1Sensors[id].value;
}

uint Daemon::toW1Id(const char* name)
{
   const char* p;
   int len = strlen(name);

   // use 4 minor bytes as id

   if (len <= 2)
      return na;

   if (len <= 8)
      p = name;
   else
      p = name + (len - 8);

   return strtoull(p, 0, 16);
}
