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
#else
#  include "gpio.h"
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
   "setup",
   "storeiosetup",
   "chartdata",
   "logmessage",

   "userdetails",
   "storeuserconfig",
   "changepasswd",

   "reset",
   "groups",
   "groupconfig",
   "chartbookmarks",
   "storechartbookmarks",
   "sendmail",
   "syslog",
   "forcerefresh",
   "storedashboards",
   "alerts",
   "storealerts",

   "errors",
   "menu",
   "pareditrequest",
   "parstore",
   "inittables",

   "schema",
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
   free(errorMailTo);

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

   return success;
}

//***************************************************************************
// Push Out Message (from daemon to WS)
//***************************************************************************

int Daemon::pushOutMessage(json_t* oContents, const char* event, long client)
{
   json_t* obj = json_object();

   addToJson(obj, "event", event);
   json_object_set_new(obj, "object", oContents);

   char* p = json_dumps(obj, JSON_REAL_PRECISION(4));
   json_decref(obj);

   if (!p)
   {
      tell(0, "Error: Dumping json message for event '%s' failed", event);
      return fail;
   }

   webSock->pushOutMessage(p, (lws*)client);
   free(p);

   webSock->performData(cWebSock::mtData);

   return done;
}

int Daemon::pushDataUpdate(const char* event, long client)
{
   // push all in the jsonSensorList to the 'interested' clients

   json_t* oJson = json_object();
   daemonState2Json(oJson);
   pushOutMessage(oJson, "daemonstate", client);

   if (client)
   {
      auto cl = wsClients[(void*)client];

      json_t* oJson = json_object();

      for (auto& sj : jsonSensorList)
         json_object_set(oJson, sj.first.c_str(), sj.second);

      pushOutMessage(oJson, event, client);
   }
   else
   {
      for (const auto& cl : wsClients)
      {
         json_t* oJson = json_object();

         for (auto& sj : jsonSensorList)
            json_object_set(oJson, sj.first.c_str(), sj.second);

         pushOutMessage(oJson, event, (long)cl.first);
      }
   }

   // cleanup

   // since we use the references more than once we have to do it
   //  by calling json_object_set instead of json_object_set_new
   //  therefore we have to free it by json_decref()

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

   initArduino();
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
   for (auto it = digitalOutputStates.begin(); it != digitalOutputStates.end(); ++it)
      gpioWrite(it->first, false, false);

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
   addValueFact(pin, "DO", 1, name, "", wtSymbol, 0, 0, urControl);

   return done;
}

//***************************************************************************
// Init digital Input
//***************************************************************************

int Daemon::initInput(uint pin, const char* name)
{
   pinMode(pin, INPUT);

   if (!isEmpty(name))
      addValueFact(pin, "DI", 1, name, "", wtSymbol, name, 0, 0);

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

   tableScripts->clear();

   for (int f = selectScripts->find(); f; f = selectScripts->fetch())
   {
      if (!fileExists(tableScripts->getStrValue("PATH")))
      {
         char* stmt {nullptr};
         asprintf(&stmt, "%s = '%s'", tableScripts->getField("PATH")->getDbName(), tableScripts->getStrValue("PATH"));
         tableScripts->deleteWhere(stmt);
         free(stmt);
         tell(eloAlways, "Removed script '%s'", tableScripts->getStrValue("PATH"));
      }
   }

   tableValueFacts->clear();
   tableValueFacts->setValue("TYPE", "SC");

   for (int f = selectValueFactsByType->find(); f; f = selectValueFactsByType->fetch())
   {
      tableScripts->setValue("ID", tableValueFacts->getIntValue("ADDRESS"));

      if (!tableScripts->find())
      {
         char* stmt {nullptr};
         asprintf(&stmt, "%s = %ld and %s = 'SC'",
                  tableValueFacts->getField("ADDRESS")->getDbName(), tableValueFacts->getIntValue("ADDRESS"),
                  tableValueFacts->getField("TYPE")->getDbName());
         tableValueFacts->deleteWhere(stmt);
         free(stmt);
         tell(eloAlways, "Removed valuefact 'SC/%ld'", tableValueFacts->getIntValue("ADDRESS"));
      }
   }

   tableValueFacts->reset();

   // check for new scripts

   FileList scripts;

   asprintf(&path, "%s/scripts.d", confDir);
   int status = getFileList(path, DT_REG, "sh py", false, &scripts, count);

   if (status != success)
   {
      free(path);
      return status;
   }

   for (const auto& script : scripts)
   {
      char* scriptPath {nullptr};
      uint addr {0};
      char* cmd {nullptr};
      std::string result;
      PySensor* pySensor {nullptr};

      // tell(eloAlways, "Found script '%s'", script.name.c_str());

      asprintf(&scriptPath, "%s/%s", path, script.name.c_str());

      if (strstr(script.name.c_str(), ".py"))
      {
         int status {success};
         pySensor = new PySensor(this, path, script.name.c_str());

         if ((status = pySensor->init()) != success || (result = executePython(pySensor, "status")) == "")
         {
            tell(0, "Initialized python script '%s' failed, status was %d, result was '%s'",
                 script.name.c_str(), status, result.c_str());
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

      auto tuple = split(script.name, '.');
      addValueFact(addr, "SC", 1, !isEmpty(title) ? title : script.name.c_str(), unit,
                   kind == "status" ? wtSymbol : kind == "text" ? wtText : wtValue,
                   tuple[0].c_str(),
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

   free(path);

   return success;
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
      // json_t* oJson = json_array();
      json_t* ojData = json_object();
      // json_array_append_new(oJson, ojData);

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
   else if (strcmp(type, "VA") == 0)
   {
      if (vaSensors[addr].kind == "text")
         json_object_set_new(ojData, "text", json_string(vaSensors[addr].text.c_str()));
      else if (vaSensors[addr].kind == "value")
         json_object_set_new(ojData, "value", json_real(vaSensors[addr].value));

      json_object_set_new(ojData, "last", json_integer(vaSensors[addr].last));
      json_object_set_new(ojData, "valid", json_integer(vaSensors[addr].valid));

      if (vaSensors[addr].disabled)
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

int Daemon::initDb()
{
   static int initial {yes};
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
            status += table->validateStructure();
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

   tableSamples = new cDbTable(connection, "samples");
   if (tableSamples->open() != success) return fail;

   tablePeaks = new cDbTable(connection, "peaks");
   if (tablePeaks->open() != success) return fail;

   tableConfig = new cDbTable(connection, "config");
   if (tableConfig->open() != success) return fail;

   tableScripts = new cDbTable(connection, "scripts");
   if (tableScripts->open() != success) return fail;

   tableUsers = new cDbTable(connection, "users");
   if (tableUsers->open() != success) return fail;

   tableGroups = new cDbTable(connection, "groups");
   if (tableGroups->open() != success) return fail;

   tableDashboards = new cDbTable(connection, "dashboards");
   if (tableDashboards->open() != success) return fail;

   tableDashboardWidgets = new cDbTable(connection, "dashboardwidgets");
   if (tableDashboardWidgets->open() != success) return fail;

   // prepare statements

   selectActiveValueFacts = new cDbStatement(tableValueFacts);

   selectActiveValueFacts->build("select ");
   selectActiveValueFacts->bindAllOut();
   selectActiveValueFacts->build(" from %s where ", tableValueFacts->TableName());
   selectActiveValueFacts->bind("STATE", cDBS::bndIn | cDBS::bndSet);

   status += selectActiveValueFacts->prepare();

   // ------------------

   selectValueFactsByType = new cDbStatement(tableValueFacts);

   selectValueFactsByType->build("select ");
   selectValueFactsByType->bindAllOut();
   selectValueFactsByType->build(" from %s where ", tableValueFacts->TableName());
   selectValueFactsByType->bind("TYPE", cDBS::bndIn | cDBS::bndSet);

   status += selectValueFactsByType->prepare();

   // ------------------

   selectAllValueFacts = new cDbStatement(tableValueFacts);

   selectAllValueFacts->build("select ");
   selectAllValueFacts->bindAllOut();
   selectAllValueFacts->build(" from %s", tableValueFacts->TableName());

   status += selectAllValueFacts->prepare();

   // ------------------
   // select all config

   selectAllConfig = new cDbStatement(tableConfig);

   selectAllConfig->build("select ");
   selectAllConfig->bindAllOut();
   selectAllConfig->build(" from %s", tableConfig->TableName());
   // selectAllConfig->build(" order by ord");

   status += selectAllConfig->prepare();

   // ------------------
   // select all users

   selectAllUser = new cDbStatement(tableUsers);

   selectAllUser->build("select ");
   selectAllUser->bindAllOut();
   selectAllUser->build(" from %s", tableUsers->TableName());

   status += selectAllUser->prepare();

   // ------------------
   // Groups

   selectAllGroups = new cDbStatement(tableGroups);

   selectAllGroups->build("select ");
   selectAllGroups->bindAllOut();
   selectAllGroups->build(" from %s", tableGroups->TableName());

   status += selectAllGroups->prepare();

   // ------------------
   // select max(time) from samples

   selectMaxTime = new cDbStatement(tableSamples);

   selectMaxTime->build("select ");
   selectMaxTime->bind("TIME", cDBS::bndOut, "max(");
   selectMaxTime->build(") from %s", tableSamples->TableName());

   status += selectMaxTime->prepare();

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
   // select all scripts

   selectScripts = new cDbStatement(tableScripts);

   selectScripts->build("select ");
   selectScripts->bindAllOut();
   selectScripts->build(" from %s", tableScripts->TableName());

   status += selectScripts->prepare();

   // ------------------
   // select script by path

   selectScriptByPath = new cDbStatement(tableScripts);

   selectScriptByPath->build("select ");
   selectScriptByPath->bindAllOut();
   selectScriptByPath->build(" from %s where ", tableScripts->TableName());
   selectScriptByPath->bind("PATH", cDBS::bndIn | cDBS::bndSet);

   status += selectScriptByPath->prepare();


   // ------------------
   // select dashboards
   //  join dashboardWidgets
   /*
   selectDashboards = new cDbStatement(tableDashboards);

   selectDashboards->build("select ");
   selectDashboards->setBindPrefix("d.");
   selectDashboards->bind("TITLE", cDBS::bndOut);
   selectDashboards->setBindPrefix("w.");
   selectDashboards->bind(tableDashboardWidgets, "ADDRESS", cDBS::bndOut, ", ");
   selectDashboards->bind(tableDashboardWidgets, "TYPE", cDBS::bndOut, ", ");
   selectDashboards->bind(tableDashboardWidgets, "WIDGETOPTS", cDBS::bndOut, ", ");
   selectDashboards->clrBindPrefix();
   selectDashboards->build(" from %s d, %s w where",
                              tableDashboards->TableName(),
                              tableDashboardWidgets->TableName());
   selectDashboards->build(" w.%s = d.%s",
                              tableDashboardWidgets->getField("DASHBOARDID")->getDbName(),
                              tableDashboards->getField("ID")->getDbName());

   status += selectDashboards->prepare();
   */

   // ------------------

   selectDashboards = new cDbStatement(tableDashboards);

   selectDashboards->build("select ");
   selectDashboards->bind("ID", cDBS::bndOut);
   selectDashboards->bind("TITLE", cDBS::bndOut, ", ");
   selectDashboards->build(" from %s", tableDashboards->TableName());

   status += selectDashboards->prepare();

   // ------------------

   selectDashboardById = new cDbStatement(tableDashboards);

   selectDashboardById->build("select ");
   selectDashboardById->bind("ID", cDBS::bndOut);
   selectDashboardById->build(" from %s where ", tableDashboards->TableName());
   selectDashboardById->bind("TITLE", cDBS::bndIn | cDBS::bndSet);

   status += selectDashboardById->prepare();

   // ------------------

   selectDashboardWidgetsFor = new cDbStatement(tableDashboardWidgets);

   selectDashboardWidgetsFor->build("select ");
   selectDashboardWidgetsFor->bind("TYPE", cDBS::bndOut);
   selectDashboardWidgetsFor->bind("ADDRESS", cDBS::bndOut, ", ");
   selectDashboardWidgetsFor->bind("WIDGETOPTS", cDBS::bndOut, ", ");
   selectDashboardWidgetsFor->build(" from %s where ", tableDashboardWidgets->TableName());
   selectDashboardWidgetsFor->bind("DASHBOARDID", cDBS::bndIn | cDBS::bndSet);
   selectDashboardWidgetsFor->build(" order by ord");

   status += selectDashboardWidgetsFor->prepare();

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

   // check about old and new dashboards and port if possible

   tableDashboards->clear();
   tableDashboardWidgets->clear();

   char* dashboards {nullptr};
   getConfigItem("dashboards", dashboards, "");
   json_t* oDashboards = json_loads(dashboards, 0, nullptr);
   free(dashboards);

   int count {0};
   tableDashboards->countWhere("1=1", count);

   if (!count && oDashboards)
   {
      json_t* oDashboard = getObjectFromJson(oDashboards, "dashboard");

      if (oDashboard)
      {
         const char* key {nullptr};
         json_t* jValue {nullptr};

         tableDashboards->setValue("TITLE", "dashboard");
         tableDashboards->store();
         int dashboardId = tableDashboards->getLastInsertId();
         int ord {0};
         json_object_foreach(oDashboard, key, jValue)
         {
            auto tuple = split(key, ':');

            tableValueFacts->clear();
            tableValueFacts->setValue("TYPE", tuple[0].c_str());
            tableValueFacts->setValue("ADDRESS", strtol(tuple[1].c_str(), nullptr, 0));

            if (tableValueFacts->find())
            {
               tableDashboardWidgets->clear();
               tableDashboardWidgets->setValue("DASHBOARDID", dashboardId);
               tableDashboardWidgets->setValue("ORDER", ord++);
               tableDashboardWidgets->setValue("TYPE", tuple[0].c_str());
               tableDashboardWidgets->setValue("ADDRESS", strtol(tuple[1].c_str(), nullptr, 0));
               tableDashboardWidgets->setValue("WIDGETOPTS", tableValueFacts->getStrValue("WIDGETOPT"));
               tableDashboardWidgets->store();
               tell(0, "Ported dashboard widget '%s' [%s]", key, tableValueFacts->getStrValue("WIDGETOPT"));
            }
         }
      }
   }

   tableDashboards->countWhere("1=1", count);

   if (!count)
   {
      tell(0, "Starting with empty dashboard");
      tableDashboards->setValue("TITLE", "dashboard");
      tableDashboards->store();
   }

   return status;
}

int Daemon::exitDb()
{
   delete tableSamples;               tableSamples = nullptr;
   delete tablePeaks;                 tablePeaks = nullptr;
   delete tableValueFacts;            tableValueFacts = nullptr;
   delete tableConfig;                tableConfig = nullptr;
   delete tableUsers;                 tableUsers = nullptr;
   delete tableGroups;                tableGroups = nullptr;
   delete tableDashboards;            tableDashboards = nullptr;
   delete tableDashboardWidgets;      tableDashboardWidgets = nullptr;

   delete selectAllGroups;            selectAllGroups = nullptr;
   delete selectActiveValueFacts;     selectActiveValueFacts = nullptr;
   delete selectValueFactsByType;     selectValueFactsByType = nullptr;
   delete selectAllValueFacts;        selectAllValueFacts = nullptr;
   delete selectAllConfig;            selectAllConfig = nullptr;
   delete selectAllUser;              selectAllUser = nullptr;
   delete selectMaxTime;              selectMaxTime = nullptr;
   delete selectSamplesRange;         selectSamplesRange = nullptr;
   delete selectSamplesRange60;       selectSamplesRange60 = nullptr;
   delete selectScriptByPath;         selectScriptByPath = nullptr;
   delete selectScripts;              selectScripts = nullptr;
   delete selectDashboards;           selectDashboards = nullptr;
   delete selectDashboardById;        selectDashboardById = nullptr;

   delete selectDashboardWidgetsFor;  selectDashboardWidgetsFor = nullptr;

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
      int res = addValueFact((int)toW1Id(it.first.c_str()), "W1", 1, it.first.c_str(), "°C", wtMeter);

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
   // init configuration

   if (argLoglevel != na)
      loglevel = argLoglevel;
   else
      getConfigItem("loglevel", loglevel, 1);

   tell(0, "Info: LogLevel set to %d", loglevel);

   getConfigItem("interval", interval, interval);
   getConfigItem("arduinoInterval", arduinoInterval, arduinoInterval);
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

   getConfigItem("invertDO", invertDO, yes);

   getConfigItem("stateCheckInterval", stateCheckInterval, 10);
   getConfigItem("ttyDevice", ttyDevice, "/dev/ttyUSB0");

   char* addrs {nullptr};
   getConfigItem("addrsDashboard", addrs, "");
   addrsDashboard = split(addrs, ',');
   // getConfigItem("addrsList", addrs, "");
   // addrsList = split(addrs, ',');
   free(addrs);

   getConfigItem("mail", mail, no);
   getConfigItem("mailScript", mailScript);
   getConfigItem("stateMailTo", stateMailTo);
   getConfigItem("errorMailTo", errorMailTo);

   getConfigItem("aggregateInterval", aggregateInterval, aggregateInterval);
   getConfigItem("aggregateHistory", aggregateHistory, aggregateHistory);

   // MQTT

   std::string url = mqttUrl ? mqttUrl : "";
   getConfigItem("mqttUrl", mqttUrl);

   if (url != mqttUrl)
      mqttDisconnect();

   url = mqttHassUrl ? mqttHassUrl : "";
   getConfigItem("mqttHassUrl", mqttHassUrl);

   if (url != mqttHassUrl)
      mqttDisconnect();

   getConfigItem("mqttHassUser", mqttHassUser, mqttHassUser);
   getConfigItem("mqttHassPassword", mqttHassPassword, mqttHassPassword);

   getConfigItem("mqttDataTopic", mqttDataTopic, TARGET "2mqtt/<TYPE>/<NAME>/state");
   getConfigItem("mqttHaveConfigTopic", mqttHaveConfigTopic, yes);

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
// Store
//***************************************************************************

int Daemon::store(time_t now, const char* name, const char* title, const char* unit,
                  const char* type, int address, double value,
                  uint factor, uint groupid, const char* text)
{
   double theValue = value / (double)factor;

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

   IoType iot = strcmp(type, "DO") == 0 ? iotLight : iotSensor;

   if (mqttInterfaceStyle == misSingleTopic)
      jsonAddValue(oJson, name, title, unit, theValue, groupid, text, initialRun /*forceConfig*/);
   else if (mqttInterfaceStyle == misGroupedTopic)
      jsonAddValue(groups[groupid].oJson, name, title, unit, theValue, 0, text, initialRun /*forceConfig*/);
   else if (mqttInterfaceStyle == misMultiTopic)
      mqttPublishSensor(iot, name, title, unit, theValue, text, initialRun /*forceConfig*/);

   return success;
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
      usleep(5000);
   }

   return done;
}

int Daemon::standbyUntil(time_t until)
{
   while (time(0) < until && !doShutDown())
   {
      meanwhile();
      usleep(5000);
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

   tell(6, "loop ...");

   atMeanwhile();

   dispatchClientRequest();
   performMqttRequests();
   performJobs();

   return done;
}

//***************************************************************************
// Loop
//***************************************************************************

int Daemon::loop()
{
   time_t nextStateAt {0};
   int lastState {na};

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

         exitDb();
         tell(eloAlways, "Retrying in 10 seconds");
         standby(10);
      }

      if (doShutDown())
         break;

      standbyUntil(min(nextStateAt, nextRefreshAt));

      // aggregate

      if (aggregateHistory && nextAggregateAt <= time(0))
         aggregate();

      // update/check state

      int status = updateState();

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

      // refresh expected?

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

      {
         sem->p();
         update();
         sem->v();
      }

      afterUpdate();

      // after update until update fetches data (at least for 'VA') -> #TODO

      updateScriptSensors();
      process();

      initialRun = false;
   }

   serial->close();

   return success;
}

//***************************************************************************
// Update
//***************************************************************************

int Daemon::update(bool webOnly, long client)
{
   int count {0};
   int status {success};
   char num[100] {'\0'};

   if (!webOnly)
   {
      static size_t w1Count {0};

      if (w1Count < w1Sensors.size())
      {
         initW1();
         w1Count = w1Sensors.size();
      }
   }

   lastSampleTime = time(0);
   tell(eloDetail, "Update ...");

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
      uint addr = tableValueFacts->getIntValue("ADDRESS");
      const char* type = tableValueFacts->getStrValue("TYPE");
      double factor = tableValueFacts->getIntValue("FACTOR");
      const char* title = tableValueFacts->getStrValue("TITLE");
      const char* usrtitle = tableValueFacts->getStrValue("USRTITLE");
      const char* unit = tableValueFacts->getStrValue("UNIT");
      const char* name = tableValueFacts->getStrValue("NAME");
      uint groupid = tableValueFacts->getIntValue("GROUPID");

      if (!isEmpty(usrtitle))
         title = usrtitle;

      if (mqttInterfaceStyle == misGroupedTopic)
      {
         if (!groups[groupid].oJson)
            groups[groupid].oJson = json_object();
      }

      json_t* ojData = json_object();
      char* tuple {nullptr};
      asprintf(&tuple, "%s:0x%02x", type, addr);
      jsonSensorList[tuple] = ojData;
      free(tuple);

      sensor2Json(ojData, tableValueFacts);

      if (tableValueFacts->hasValue("TYPE", "W1"))
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
               store(lastSampleTime, name, title, unit, type, addr, w1Value, factor, groupid);
         }
         else
         {
            if (!w1Exist)
               tell(eloAlways, "Warning: W1 sensor '%s' missing", name);
            else
               tell(eloAlways, "Warning: Data of W1 sensor '%s' seems to be to old (%s)", name, l2pTime(w1Last).c_str());

            json_object_set_new(ojData, "text", json_string("missing sensor"));
         }
      }
      else if (tableValueFacts->hasValue("TYPE", "SC"))
      {
         if (scSensors[addr].kind == "status")
            json_object_set_new(ojData, "value", json_integer(scSensors[addr].state));
         else if (scSensors[addr].kind == "text")
            json_object_set_new(ojData, "text", json_string(scSensors[addr].text.c_str()));
         else if (scSensors[addr].kind == "value")
            json_object_set_new(ojData, "value", json_real(scSensors[addr].value));

         if (scSensors[addr].disabled)
            json_object_set_new(ojData, "disabled", json_boolean(true));

         if (!webOnly && !scSensors[addr].disabled)
            store(lastSampleTime, name, title, unit, type, addr, scSensors[addr].kind == "value" ? scSensors[addr].value : scSensors[addr].state, factor, groupid);
      }
      else if (tableValueFacts->hasValue("TYPE", "SP"))            // Special Values
      {
         if (spSensors[addr].kind == "text")
         {
            json_object_set_new(ojData, "text", json_string(spSensors[addr].text.c_str()));

            if (!webOnly && !spSensors[addr].disabled)
               store(lastSampleTime, name, title, unit, type, addr, spSensors[addr].value, factor, groupid, spSensors[addr].text.c_str());
         }
         else if (spSensors[addr].kind == "value")
         {
            json_object_set_new(ojData, "value", json_integer(spSensors[addr].value));

            if (!webOnly && !spSensors[addr].disabled)
               store(lastSampleTime, name, title, unit, type, addr, spSensors[addr].value, factor, groupid);
         }

         if (spSensors[addr].disabled)
            json_object_set_new(ojData, "disabled", json_boolean(true));
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

         if (!webOnly)
         {
            store(lastSampleTime, name, title, unit, type, v.address, v.state, factor, groupid);
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

         if (!webOnly)
         {
            store(lastSampleTime, name, title, unit, type, v.address, v.state, factor, groupid);
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
            store(lastSampleTime, name, title, unit, type, v.address, v.state, factor, groupid);
            sprintf(num, "%d", v.state);
            addParameter2Mail(title, num);
         }
      }
      else
         onUpdate(webOnly, tableValueFacts, lastSampleTime, ojData);

      count++;
   }

   selectActiveValueFacts->freeResult();

   if (!webOnly)
      connection->commit();

   // send result to all connected WEBIF clients

   pushDataUpdate(webOnly ? "init" : "all", client);

   if (!webOnly)
      tell(eloAlways, "Updated %d samples", count);

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

   return success;
}

//***************************************************************************
// Update Script Sensors
//***************************************************************************

void Daemon::updateScriptSensors()
{
   tableValueFacts->clear();
   tableValueFacts->setValue("STATE", "A");

   tell(1, "Update script sensors");

   for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
   {
      if (!tableValueFacts->hasValue("TYPE", "SC"))
         continue;

      uint addr = tableValueFacts->getIntValue("ADDRESS");
      const char* name = tableValueFacts->getStrValue("NAME");
      const char* title = tableValueFacts->getStrValue("USRTITLE");

      if (isEmpty(title))
         title = tableValueFacts->getStrValue("TITLE");

      callScript(addr, "status", name, title);
   }

   selectActiveValueFacts->freeResult();
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
// Add Parameter To Mail
//***************************************************************************

void Daemon::addParameter2Mail(const char* name, const char* value)
{
   if (stateChanged)
      mailBodyHtml += "        <tr><td>" + std::string(name) + "</td><td>" + std::string(value) + "</td></tr>\n";
}

//***************************************************************************
// Is In Time Range
//***************************************************************************

bool Daemon::isInTimeRange(const std::vector<Range>* ranges, time_t t)
{
   for (auto it = ranges->begin(); it != ranges->end(); ++it)
   {
      if (it->inRange(l2hhmm(t)))
      {
         tell(eloDebug, "Debug: %d in range '%d-%d'", l2hhmm(t), it->from, it->to);
         return true;
      }
   }

   return false;
}

//***************************************************************************
// Schedule Aggregate
//***************************************************************************

int Daemon::scheduleAggregate()
{
   struct tm tm = { 0 };
   time_t now {0};

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
   char* stmt {nullptr};
   time_t history = time(0) - (aggregateHistory * tmeSecondsPerDay);
   int aggCount {0};

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
      tell(eloDebug, "Aggregation: [%s]", stmt);
      free(stmt);

      // Einzelmesspunkte löschen ...

      asprintf(&stmt, "aggregate != 'A' and time <= from_unixtime(%ld)", history);

      if (tableSamples->deleteWhere(stmt) == success)
      {
         tell(eloAlways, "Aggregation with interval of %d minutes done; "
              "Created %d aggregation rows", aggregateInterval, aggCount);
      }
   }

   free(stmt);

   // schedule even in case of error!

   scheduleAggregate();

   return success;
}

int Daemon::sendMail(const char* receiver, const char* subject, const char* body, const char* mimeType)
{
   char* command {nullptr};
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
// Load Html Header
//***************************************************************************

int Daemon::loadHtmlHeader()
{
   char* file {nullptr};

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
                     "  </head>\n\0");

   return success;
}

//***************************************************************************
// Add Value Fact
//***************************************************************************

int Daemon::addValueFact(int addr, const char* type, int factor, const char* name, const char* unit,
                         WidgetType widgetType, const char* title,
                         int minScale, int maxScale, int rights, const char* choices)
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
      tableValueFacts->setValue("TITLE", title ? title : name);
      tableValueFacts->setValue("FACTOR", factor);
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
   tableValueFacts->setValue("TITLE", title ? title : name);
   tableValueFacts->setValue("FACTOR", factor);

   if (!isEmpty(choices))
      tableValueFacts->setValue("CHOICES", choices);

   if (tableValueFacts->getValue("RIGHTS")->isNull())
      tableValueFacts->setValue("RIGHTS", rights);

   if (tableValueFacts->getValue("WIDGETOPT")->isNull())
   {
      char* opt {nullptr};
      asprintf(&opt, "{\"unit\": \"%s\", \"scalemax\": %d, \"scalemin\": %d, \"scalestep\": %d, \"imgon\": \"%s\", \"imgoff\": \"%s\", \"widgettype\": %d}",
               unit, maxScale, minScale, 0, getImageFor(name, true), getImageFor(name, false), widgetType);
      tableValueFacts->setValue("WIDGETOPT", opt);
      free(opt);
   }

   if (tableValueFacts->getChanges())
   {
      tableValueFacts->store();
      return 2;                                // 2 for 'modified'
   }

   return done;
}

//***************************************************************************
// Dispatch Mqtt Command Request
//   Format: {"state": "OFF", "brightness": 255}
//***************************************************************************

int Daemon::dispatchMqttCommandRequest(json_t* jData, const char* topic)
{
   auto it = hassCmdTopicMap.find(topic);

   if (it != hassCmdTopicMap.end())
   {
      const char* state = getStringFromJson(jData, "state", "");

      if (isEmpty(state))
         return fail;

      for (auto itOutput = digitalOutputStates.begin(); itOutput != digitalOutputStates.end(); ++itOutput)
      {
         bool bState = strcmp(state, "ON") == 0;

         if (strcmp(it->second.c_str(), itOutput->second.name) == 0)
         {
            gpioWrite(itOutput->first, bState);
            break;
         }
      }
   }

   return success;
}

//***************************************************************************
// Config Data
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
      value = strtod(txt, nullptr);
   else if (isEmpty(txt) && def != na)
   {
      value = def;
      setConfigItem(name, value);
   }
   else
      value = 0.0;

   free(txt);

   // tell(0, "getConfigItem '%s' now %.2f\n", name, value);

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
// needed?   json_object_set_new(ojData, "image", json_string(getImageFor(digitalOutputStates[pin].title.c_str(), digitalOutputStates[pin].state)));
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

      // json_t* oJson = json_array();
      json_t* ojData = json_object();
      // json_array_append_new(oJson, ojData);
      pin2Json(ojData, pin);

      char* tuple {nullptr};
      asprintf(&tuple, "%s:0x%02x", "DO", pin);
      jsonSensorList[tuple] = ojData;
      free(tuple);

      pushDataUpdate("update", 0L);
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
      // json_t* oJson = json_array();
      json_t* ojData = json_object();
      // json_array_append_new(oJson, ojData);
      pin2Json(ojData, pin);

      char* tuple {nullptr};
      asprintf(&tuple, "%s:0x%02x", "DO", pin);
      jsonSensorList[tuple] = ojData;
      free(tuple);

      pushDataUpdate("update", 0L);
      // pushOutMessage(oJson, "update");
   }

   mqttPublishSensor(iotLight, digitalOutputStates[pin].name, "", "", digitalOutputStates[pin].state, "", false /*forceConfig*/);
}

bool Daemon::gpioRead(uint pin)
{
   digitalInputStates[pin] = digitalRead(pin);
   return digitalInputStates[pin];
}

//***************************************************************************
// Publish Special Value
//***************************************************************************

void Daemon::publishSpecialValue(int addr)
{
   cDbRow* fact = valueFactOf("SP", addr);

   if (fact)
   {
      json_t* ojData = json_object();

      sensor2Json(ojData, tableValueFacts);

      if (spSensors[addr].kind == "text")
         json_object_set_new(ojData, "text", json_string(spSensors[addr].text.c_str()));
      else if (spSensors[addr].kind == "value")
         json_object_set_new(ojData, "value", json_real(spSensors[addr].value));

      if (spSensors[addr].disabled)
         json_object_set_new(ojData, "disabled", json_boolean(true));

      char* tuple {nullptr};
      asprintf(&tuple, "SP:0x%02x", addr);
      jsonSensorList[tuple] = ojData;
      free(tuple);

      pushDataUpdate("update", 0L);
   }
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
// Arduino Stuff ...
//***************************************************************************

int Daemon::dispatchArduinoMsg(const char* message)
{
   json_error_t error;
   json_t* jObject = json_loads(message, 0, &error);

   if (!jObject)
   {
      tell(0, "Error: Can't parse json in '%s'", message);
      return fail;
   }

   const char* event = getStringFromJson(jObject, "event", "<null>");

   if (strcmp(event, "update") == 0)
   {
      json_t* jArray = getObjectFromJson(jObject, "analog");
      size_t index {0};
      json_t* jValue {nullptr};

      json_array_foreach(jArray, index, jValue)
      {
         const char* name = getStringFromJson(jValue, "name");
         double value = getDoubleFromJson(jValue, "value");
         time_t stamp = getIntFromJson(jValue, "time", 0);

         if (!stamp)
            stamp = time(0);

         if (stamp < time(0)-300)
         {
            tell(eloAlways, "Skipping old (%ld seconds) arduino value", time(0)-stamp);
            continue;
         }

         updateAnalogInput(name, value, stamp);
      }

      process();
   }
   else if (strcmp(event, "init") == 0)
   {
      initArduino();
   }
   else
   {
      tell(eloAlways, "Got unexpected event '%s' from arduino", event);
   }

   return success;
}

int Daemon::initArduino()
{
   mqttCheckConnection();

   if (isEmpty(mqttUrl) || !mqttReader->isConnected())
      return fail;

   json_t* oJson = json_object();

   json_object_set_new(oJson, "event", json_string("setUpdateInterval"));
   json_object_set_new(oJson, "parameter", json_integer(arduinoInterval));

   char* p = json_dumps(oJson, JSON_REAL_PRECISION(4));
   json_decref(oJson);

   if (!p)
   {
      tell(0, "Error: Dumping json message failed");
      return fail;
   }

   mqttReader->write(TARGET "2mqtt/arduino/in", p);
   tell(1, "DEBUG: PushMessage to arduino [%s]", p);
   free(p);

   return success;
}

void Daemon::updateAnalogInput(const char* id, double value, time_t stamp)
{
   uint input = atoi(id+1);

   // the Ardoino read the analog inputs with a resolution of 12 bits (3.3V => 4095)

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", (int)input);
   tableValueFacts->setValue("TYPE", "AI");

   if (!tableValueFacts->find() || !tableValueFacts->hasValue("STATE", "A"))
   {
      tableValueFacts->reset();
      return ;
   }

   double m = (aiSensors[input].calPointB - aiSensors[input].calPointA) / (aiSensors[input].calPointValueB - aiSensors[input].calPointValueA);
   double b = aiSensors[input].calPointB - m * aiSensors[input].calPointValueB;
   double dValue = m * value + b;

   if (aiSensors[input].round)
   {
      dValue = std::llround(dValue*aiSensors[input].round) / aiSensors[input].round;
      tell(2, "Rouned %.2f to %.2f", m * value + b, dValue);
   }

   aiSensors[input].value = dValue;
   aiSensors[input].last = stamp;
   aiSensors[input].valid = true;

   tell(1, "Debug: Input A%d: %.3f%s [%.2f]", input, aiSensors[input].value, tableValueFacts->getStrValue("UNIT"), value);

   // ----------------------------------

   json_t* ojData = json_object();

   sensor2Json(ojData, tableValueFacts);
   json_object_set_new(ojData, "value", json_real(aiSensors[input].value));

   char* tuple {nullptr};
   asprintf(&tuple, "AI:0x%02x", input);
   jsonSensorList[tuple] = ojData;
   free(tuple);

   pushDataUpdate("update", 0L);

   tableValueFacts->reset();
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
