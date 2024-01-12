//***************************************************************************
// Automation Control
// File daemon.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 2010 - 2023  Jörg Wendel
//***************************************************************************

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <algorithm>
#include <cmath>
#include <libgen.h>

#ifndef _NO_RASPBERRY_PI_
#  include <wiringPi.h>
#else
#  include "gpio.h"
#endif

#include "lib/curl.h"
#include "lib/json.h"
#include "lib/lua.h"

#include "daemon.h"
#include "growatt.h"

bool Daemon::shutdown {false};

//***************************************************************************
// Widgets
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
   "SymbolValue",
   "Spacer",
   "Time",
   "SymbolText",
   "BarChart",
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
// Default Value Types
//***************************************************************************

Daemon::ValueTypes Daemon::defaultValueTypes[] =
{
   // expression, title

   { "^VA",    "Messwerte" },
   { "^SD",    "Status Laufzeiten" },
   { "^DO",    "Digitale Ausgänge" },
   { "^DI",    "Digitale Eingänge" },
   { "^W1",    "One Wire Sensoren" },
   { "^SC",    "Skripte" },
   { "^AO",    "Analog Ausgänge" },
   { "^AI",    "Analog Eingänge" },
   { "^Sp",    "Weitere Sensoren" },
   { "^DZL",   "DECONZ Lampen" },
   { "^DZS",   "DECONZ Sensoren" },
   { "^HM.*",  "Home Matic" },
   { "^P4.*",  "P4 Daemon" },
   { "^WEA",   "Wetter" },
   { "^CV",    "Calculated Values" },

   { "",      "" }
};

const char* Daemon::getTitleOfType(const char* type)
{
   for (int i = 0; defaultValueTypes[i].typeExpression != ""; i++)
   {
      if (rep(type, defaultValueTypes[i].typeExpression.c_str()) == success)
         return defaultValueTypes[i].title.c_str();
   }

   return type;
}

//***************************************************************************
// Widgets - Default Properties
//***************************************************************************

Daemon::DefaultWidgetProperty Daemon::defaultWidgetProperties[] =
{
   // type, address,  unit,  widgetType, minScale, maxScale scaleStep, showPeak

   { "-",        na,   "*",      wtMeter,        0,        45,      10, false },
   { "SPACER",   na,   "*",      wtSpace,        0,         0,       0, false },
   { "TIME",     na, "txt",  wtPlainText,        0,         0,       0, true },
   { "DO",       na,   "*",     wtSymbol,        0,         0,       0, false },
   { "DI",       na,   "*",     wtSymbol,        0,         0,       0, false },
   { "AO",       na,   "*",      wtMeter,        0,        45,      10, false },
   { "AI",       na,   "*",      wtMeter,        0,        45,      10, false },
   { "SD",       na,   "*",      wtChart,        0,      2000,       0, true },
   { "SC",       na,    "",       wtText,        0,         0,       0, false },
   { "SC",       na, "zst",     wtSymbol,        0,         0,       0, false },
   { "SC",       na,   "*",      wtMeter,        0,        40,      10, true },
   { "SP",       na,    "",       wtText,        0,         0,       0, false },
   { "SP",       na,   "%", wtMeterLevel,        0,       100,      20, false },
   { "SP",       na, "kWh",      wtChart,        0,        50,       0, true },
   { "SP",       na,   "W",      wtMeter,        0,      3000,       0, true },
   { "SP",       na, "txt",  wtPlainText,        0,         0,       0, true },
   { "SP",       na,   "*",      wtMeter,        0,       100,      10, true },
   { "UD",       na, "txt",       wtText,        0,         0,       0, false },
   { "UD",       na, "zst", wtSymbolText,        0,         0,       0, false },
   { "UD",       na,   "*",       wtText,        0,         0,       0, false },
   { "W1",       na,   "*", wtMeterLevel,        0,        40,      10, true },
   { "VA",       na,   "%", wtMeterLevel,        0,       100,      20, true },
   { "VA",       na,   "*",      wtMeter,        0,        45,      10, true },
   { "HMB",      na,   "*",wtSymbolValue,        0,         0,       0, true },
   { "DZL",      na,   "*",     wtSymbol,        0,         0,       0, true },
   { "DZS",      na, "zst",     wtSymbol,        0,         0,       0, true },
   { "DZS",      na,  "°C", wtMeterLevel,        0,        45,      10, true },
   { "DZS",      na,   "%",      wtChart,        0,         0,       0, true },
   { "DZS",      na, "hPa",      wtChart,        0,         0,       0, true },
   { "DZS",      na, "mov",     wtSymbol,        0,         0,       0, true },
   { "DZS",      na,  "lx",      wtChart,        0,         0,       0, true },
   { "DZS",      na,   "*",      wtMeter,        0,        45,      12, true },
   { "WEA",      na,   "*",  wtPlainText,        0,         0,       0, true },
   { "" }
};

Daemon::DefaultWidgetProperty* Daemon::getDefalutProperty(const char* type, const char* unit, int address)
{
   for (int i = 0; defaultWidgetProperties[i].type != ""; i++)
   {
      if (defaultWidgetProperties[i].type == type)
      {
         bool addressMatch = defaultWidgetProperties[i].address == na || defaultWidgetProperties[i].address == address;
         bool unitMatch = defaultWidgetProperties[i].unit == "*" || defaultWidgetProperties[i].unit == unit;

         if (unitMatch && addressMatch)
            return &defaultWidgetProperties[i];
      }
   }

   return &defaultWidgetProperties[0]; // the default of the defaluts
}

//***************************************************************************
// Widget Defaults 2 Json
//***************************************************************************

int Daemon::widgetDefaults2Json(json_t* jDefaults, const char* type, const char* unit, const char* name, int address)
{
   std::string result;
   DefaultWidgetProperty* defProperty = getDefalutProperty(type, unit, address);

   const char* color {"rgb(255, 255, 255)"};
   const char* colorOn {"rgb(235, 197, 5)"};
   const char* symbol {""};
   const char* symbolOn {""};

   if (defProperty->unit == "mov")
   {
      symbol = "mdi:mdi-walk";
   }
   else if (strcmp(type, "HMB") == 0)
   {
      symbol = "mdi:mdi-blinds";
      symbolOn = "mdi:mdi-blinds-open";
      color = "rgb(0, 99, 162)";
      colorOn = "rgb(255, 255, 255)";
   }
   else if (strcmp(type, "DZL") == 0)
   {
      symbol = "mdi:mdi-lightbulb-variant-outline";
      symbolOn = "mdi:mdi-lightbulb-variant";
      color = "rgb(255, 255, 255)";
      colorOn = "rgb(235, 197, 5)";
   }

   json_object_set_new(jDefaults, "widgettype", json_integer(defProperty->widgetType));
   json_object_set_new(jDefaults, "unit", json_string(unit));
   json_object_set_new(jDefaults, "scalemax", json_integer(defProperty->maxScale));
   json_object_set_new(jDefaults, "scalemin", json_integer(defProperty->minScale));
   json_object_set_new(jDefaults, "scalestep", json_integer(defProperty->scaleStep));
   json_object_set_new(jDefaults, "showpeak", json_boolean(defProperty->showPeak));
   json_object_set_new(jDefaults, "imgon", json_string(getImageFor(type, name, unit, true)));
   json_object_set_new(jDefaults, "imgoff", json_string(getImageFor(type, name, unit, false)));
   json_object_set_new(jDefaults, "symbol", json_string(symbol));
   json_object_set_new(jDefaults, "symbolOn", json_string(symbolOn));
   json_object_set_new(jDefaults, "color", json_string(color));
   json_object_set_new(jDefaults, "colorOn", json_string(colorOn));

   return done;
}

//***************************************************************************
// Get Image For
//***************************************************************************

const char* Daemon::getImageFor(const char* type, const char* title, const char* unit, int value)
{
   const char* imagePath = "img/icon/unknown.png";

   if (strcmp(type, "DZL") == 0 || strcasestr(title, "Licht") || strcasestr(title, "Light"))
      imagePath = value ? "img/icon/light-on.png" : "img/icon/light-off.png";
   else if (strcasestr(title, "Pump"))
      imagePath = value ? "img/icon/pump-on.gif" : "img/icon/pump-off.png";
   else if (strcasestr(title, "Steckdose") || strcasestr(title, "Plug") )
      imagePath = value ? "img/icon/plug-on.png" : "img/icon/plug-off.png";
   else if (strcasestr(title, "UV-C"))
      imagePath = value ? "img/icon/uvc-on.png" : "img/icon/uvc-off.png";
   else if (strcasestr(title, "Shower") || strcasestr(title, "Dusche"))
      imagePath = value ? "img/icon/shower-on.png" : "img/icon/shower-off.png";
   else if (strcasestr(title, "VDR"))
      imagePath = value ? "img/icon/vdr-on.png" : "img/icon/vdr-off.png";
   else if (strcasestr(title, "VPN"))
      imagePath = value ? "img/icon/vpn-on.png" : "img/icon/vpn-off.png";
   else if (strcasestr(title, "SATIP"))
      imagePath = value ? "img/icon/satip-on.png" : "img/icon/satip-off.png";
   else if (strcasestr(title, "Music") || strcasestr(title, "Musik"))
      imagePath = value ? "img/icon/note-on.png" : "img/icon/note-off.png";
   else if (strcasestr(title, "Fan") || strcasestr(title, "Lüfter"))
      imagePath = value ? "img/icon/fan-on.png" : "img/icon/fan-off.png";
   else if (strcasestr(title, "Desktop"))
      imagePath = value ? "img/icon/desktop-on.png" : "img/icon/desktop-off.png";
   else
      imagePath = value ? "img/icon/boolean-on.png" : "img/icon/boolean-off.png";

   return imagePath;
}

//***************************************************************************
// Object
//***************************************************************************

int isDST()
{
   struct tm tm;
   time_t t = time(0);

   localtime_r(&t, &tm);
   tm.tm_isdst = -1;  // force DST auto detect
   mktime(&tm);

   return tm.tm_isdst;
}

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

   webSock = new cWebSock(this, httpPath);
}

Daemon::~Daemon()
{
   exit();

   delete webSock;

   free(mailScript);
   free(stateMailTo);
   free(errorMailTo);

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
      tell(eloAlways, "Error: Dumping json message for event '%s' failed", event);
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

   initLocale();

   // initialize the dictionary

   char* dictPath {};
   asprintf(&dictPath, "%s/database.dat", confDir);

   if (dbDict.in(dictPath) != success)
   {
      tell(eloAlways, "Fatal: Dictionary not loaded, aborting!");
      return 1;
   }

   tell(eloAlways, "Dictionary '%s' loaded", dictPath);
   free(dictPath);

   while ((status = initDb()) != success && !doShutDown())
   {
      exitDb();
      tell(eloAlways, "Retrying in %d seconds", 10);
      doSleep(10);
   }

   deconz.init(this, connection);

   // ---------------------------------
   // check users - add default user if empty

   int userCount {0};
   tableUsers->countWhere("", userCount);

   if (userCount <= 0)
   {
      tell(eloAlways, "Initially adding default user (" NAME "/" NAME ")");

      md5Buf defaultPwd;
      createMd5(NAME, defaultPwd);
      tableUsers->clear();
      tableUsers->setValue("USER", NAME);
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

   // homeMaticInterface

   if (homeMaticInterface)
   {
      mqttSensorTopics.push_back(TARGET "2mqtt/homematic/rpcresult");
      mqttSensorTopics.push_back(TARGET "2mqtt/homematic/events");

      if (mqttCheckConnection() == success && !isEmpty(mqttUrl))
      {
         const char* request = "{ \"method\" : \"listDevices\" }";
         mqttWriter->write(TARGET "2mqtt/homematic/rpccall", request);
         tell(eloHomeMatic, "-> (home-matic) '%s' to '%s'", TARGET "2mqtt/homematic/rpccall", request);
      }
      else
      {
         tell(eloAlways, "Error: Can't request home-matic data, MQTT connection failed");
      }
   }

   // init web socket ...

   while (webSock->init(webPort, webSocketPingTime, confDir, webSsl) != success)
   {
      tell(eloAlways, "Retrying in 2 seconds");
      sleep(2);
   }

   initArduino();
   performMqttRequests();
   initScripts();
   loadStates();           // load states of outputs on last exit
   lmcInit();

   if (!isEmpty(deconz.getHttpUrl()) && !isEmpty(deconz.getApiKey()))
      deconz.initDevices();

   if (!isEmpty(deconz.getHttpUrl()) && isEmpty(deconz.getApiKey()))
   {
      tell(eloAlways, "DECONZ: No API key for '%s' jet, try to query", deconz.getHttpUrl());
      std::string result;
      int status = deconz.queryApiKey(result);

      if (status != success)
      {
         tell(eloAlways, "DECONZ: Key result was '%s' (%d)", result.c_str(), status);
         tell(eloAlways, "ACHTUNG: Initiale Registrierung bei deconz '%s' fehlgeschlagen! "   \
              "Unlock the deCONZ gateway and restart homectrld within the 60 seconds", deconz.getHttpUrl());
      }
      else
      {
         setConfigItem("deconzApiKey", result.c_str());
         deconz.setApiKey(result.c_str());
         deconz.initDevices();
      }
   }

   addValueFact(1, "CV", 1, "Calc Sensor 1", "", "");

   initialized = true;

   return success;
}

int Daemon::initLocale()
{
   setenv("TZ", "CET", 1);
   tzset();  // init timezone environment
   tell(eloAlways, "Daylight (%d); Timezone is (%ld) now it's %ld", isDST(), timezone, time(0));

   // set a locale to "" means 'reset it to the environment'
   // as defined by the ISO-C standard the locales after start are C

   const char* locale {};

   setlocale(LC_ALL, "");
   locale = setlocale(LC_ALL, 0);  // 0 for query the setting

   if (!locale)
   {
      tell(eloAlways, "Info: Detecting locale setting for LC_ALL failed");
      return fail;
   }

   tell(eloInfo, "Current locale is %s", locale);

   if ((strcasestr(locale, "UTF-8") != 0) || (strcasestr(locale, "UTF8") != 0))
      tell(eloInfo, "Detected UTF-8");

   return done;
}

int Daemon::exit()
{
   for (auto it = sensors["DO"].begin(); it != sensors["DO"].end(); ++it)
      gpioWrite(it->first, false, false);

   lmcExit();
   deconz.exit();
   mqttDisconnect();
   exitDb();

   return success;
}

//***************************************************************************
// Init Sensor
//***************************************************************************

int Daemon::initSensorByFact(std::string type, uint address)
{
   cDbRow* fact = valueFactRowOf(type.c_str(), address);

   if (!fact)
   {
      tell(eloAlways, "Warning: valuefact for %s:0x%02x not found!", type.c_str(), address);
      return fail;
   }

   sensors[type][address].type = type;
   sensors[type][address].address = address;
   sensors[type][address].record = fact->hasValue("RECORD", "A");
   sensors[type][address].name = fact->getStrValue("NAME");
   sensors[type][address].unit = fact->getStrValue("UNIT");
   sensors[type][address].factor = fact->getIntValue("FACTOR");
   sensors[type][address].group = fact->getIntValue("GROUPID");
   sensors[type][address].invertDO = !fact->hasValue("INVERT", "N");

   if (type == "DO" || type == "DI" || type == "DZL")
      sensors[type][address].kind = "status";

   if (sensors[type][address].unit == "txt")
      sensors[type][address].kind = "text";

   if (!fact->getValue("USRTITLE")->isEmpty())
      sensors[type][address].title = fact->getStrValue("USRTITLE");
   else if (!fact->getValue("TITLE")->isEmpty())
      sensors[type][address].title = fact->getStrValue("TITLE");
   else
      sensors[type][address].title = fact->getStrValue("NAME");

   if (type == "AI" && !fact->getValue("CALIBRATION")->isEmpty())
   {
      json_t* jCal = jsonLoad(fact->getStrValue("CALIBRATION"));

      if (jCal)
      {
         aiSensors[address].calPointA = getDoubleFromJson(jCal, "pointA");
         aiSensors[address].calPointB = getDoubleFromJson(jCal, "pointB");
         aiSensors[address].calPointValueA = getDoubleFromJson(jCal, "valueA");
         aiSensors[address].calPointValueB = getDoubleFromJson(jCal, "valueB");
         aiSensors[address].round = getDoubleFromJson(jCal, "round");
         aiSensors[address].calCutBelow = getDoubleFromJson(jCal, "calCutBelow", -10000.0);

         json_decref(jCal);
      }
   }

   tell(eloDebug, "Debug: Init sensor %s/0x%02x - '%s'", type.c_str(), address, sensors[type][address].title.c_str());

   return success;
}

//***************************************************************************
// Init digital Output
//***************************************************************************

int Daemon::initOutput(uint pin, int opt, OutputMode mode, const char* name, uint rights)
{
   addValueFact(pin, "DO", 1, name, "", "", rights);

   sensors["DO"][pin].opt = opt;
   sensors["DO"][pin].mode = mode;

   pinMode(pin, OUTPUT);
   gpioWrite(pin, false, false);

   return done;
}

//***************************************************************************
// Init digital Input
//***************************************************************************

int Daemon::initInput(uint pin, const char* name)
{
   pinMode(pin, INPUT);

   if (!isEmpty(name))
   {
      addValueFact(pin, "DI", 1, name);
      sensors["DI"][pin].state = gpioRead(pin);
   }

   return done;
}

//***************************************************************************
// Init Scripts
//***************************************************************************

int Daemon::initScripts()
{
   char* path {};
   int count {0};

   // clear removed scripts ...

   tableScripts->clear();

   for (int f = selectScripts->find(); f; f = selectScripts->fetch())
   {
      if (!fileExists(tableScripts->getStrValue("PATH")))
      {
         char* stmt {};
         asprintf(&stmt, "%s = '%s'", tableScripts->getField("PATH")->getDbName(), tableScripts->getStrValue("PATH"));
         tableScripts->deleteWhere("%s", stmt);
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
         char* stmt {};
         asprintf(&stmt, "%s = %ld and %s = 'SC'",
                  tableValueFacts->getField("ADDRESS")->getDbName(), tableValueFacts->getIntValue("ADDRESS"),
                  tableValueFacts->getField("TYPE")->getDbName());
         tableValueFacts->deleteWhere("%s", stmt);
         free(stmt);
         tell(eloAlways, "Removed valuefact 'SC/%ld'", tableValueFacts->getIntValue("ADDRESS"));
      }
   }

   tableValueFacts->reset();

   // check for new scripts

   FileList scripts;

   asprintf(&path, "%s/scripts.d", confDir);
   int status = getFileList(path, DT_REG, "sh", false, &scripts, count);

   if (status != success)
   {
      free(path);
      return status;
   }

   for (const auto& script : scripts)
   {
      char* scriptPath {};
      uint addr {0};
      char* cmd {};
      std::string result;

      asprintf(&scriptPath, "%s/%s", path, script.name.c_str());

      // get address

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

      cDbRow* factRow = valueFactRowOf("SC", addr);

      if (factRow && !factRow->hasValue("STATE", "A"))
      {
         tell(eloDebug, "Skipping deactivated script '%s'", scriptPath);
         free(scriptPath);
         continue;
      }

      // execute script

      const char* url = strrchr(mqttUrl, '/');
      if (url)
         url++;
      else
         url = mqttUrl;

      asprintf(&cmd, "%s %s %d 'mqtt://%s/%s'", scriptPath, "init", addr, url, TARGET "2mqtt/scripts");
      tell(eloDetail, "Calling '%s'", cmd);
      result = executeCommand(cmd);
      free(cmd);

      json_t* oData = jsonLoad(result.c_str());

      if (!oData)
      {
         free(scriptPath);
         continue;
      }

      std::string kind = getStringFromJson(oData, "kind", "status");
      const char* title = getStringFromJson(oData, "title");
      const char* unit = getStringFromJson(oData, "unit");
      const char* choices = getStringFromJson(oData, "choices");
      double value = getDoubleFromJson(oData, "value");
      const char* text = getStringFromJson(oData, "text");
      bool valid = getBoolFromJson(oData, "valid", true);

      if (kind == "text")
         unit = "";
      else if (kind == "status")
         unit = "zst";

      auto tuple = split(script.name, '.');
      addValueFact(addr, "SC", 1, !isEmpty(title) ? title : script.name.c_str(), unit, tuple[0].c_str(), urControl, choices);
      tell(eloAlways, "Init script value of 'SC:%d' to %.2f", addr, value);

      sensors["SC"][addr].kind = kind;
      sensors["SC"][addr].last = time(0);
      sensors["SC"][addr].valid = valid;

      if (kind == "status")
         sensors["SC"][addr].state = (bool)value;
      else if (kind == "trigger")
         sensors["SC"][addr].state = (bool)value;
      else if (kind == "text")
         sensors["SC"][addr].text = text;
      else if (kind == "value")
         sensors["SC"][addr].value = value;

      tell(eloDetail, "Info: Found script '%s' addr (%d), unit '%s'; result was [%s]", scriptPath, addr, unit, result.c_str());
      free(scriptPath);
      json_decref(oData);
   }

   free(path);

   return success;
}

//***************************************************************************
// Call Script
//***************************************************************************

int Daemon::callScript(int addr, const char* command)
{
   if (commandThreads.find(addr) != commandThreads.end())
   {
      if (commandThreads[addr].active)
      {
         tell(eloAlways, "Info: Skipping call of script 'SC:0x%02x', already running", addr);
         return done;
      }
   }

   tableScripts->clear();
   tableScripts->setValue("ID", addr);

   if (!tableScripts->find())
   {
      tell(eloAlways, "Fatal: Script for 'SC:0x%02x' not found", addr);
      return fail;
   }

   char* cmd {};
   const char* url = strrchr(mqttUrl, '/');
   if (url)
      url++;
   else
      url = mqttUrl;

   asprintf(&cmd, "%s %s %d 'mqtt://%s/%s'", tableScripts->getStrValue("PATH"), command, addr, url, TARGET "2mqtt/scripts");

   tell(eloDetail, "Info: Calling '%s' ..", cmd);
   int result = executeCommandAsync(addr, cmd);
   tell(eloDetail, ".. done");

   tableScripts->reset();

   if (result == success && strstr("start|stop|toggle", command))
   {
      sensors["SC"][addr].working = true;

      json_t* ojData = json_object();
      sensor2Json(ojData, "SC", addr);

      char* tuple {};
      asprintf(&tuple, "%s:0x%02x", "SC", addr);
      jsonSensorList[tuple] = ojData;
      free(tuple);

      pushDataUpdate("update", 0L);
   }

   return result;
}

//***************************************************************************
// Value Fact Of
//***************************************************************************

cDbRow* Daemon::valueFactRowOf(const char* type, uint addr)
{
   tableValueFacts->clear();

   tableValueFacts->setValue("ADDRESS", (long)addr);
   tableValueFacts->setValue("TYPE", type);

   if (!tableValueFacts->find())
      return nullptr;

   return tableValueFacts->getRow();
}

//***************************************************************************
// Get Sensor
//***************************************************************************

Daemon::SensorData* Daemon::getSensor(const char* type, int addr)
{
   if (isEmpty(type))
      return nullptr;

   auto itType = sensors.find(type);

   if (itType == sensors.end())
      return nullptr;

   auto itSensor = itType->second.find(addr);

   if (itSensor == itType->second.end())
      return nullptr;

   return &itSensor->second;
}

//***************************************************************************
// Set Special Value
//***************************************************************************

void Daemon::setSpecialValue(uint addr, double value, const std::string& text)
{
   sensors["SP"][addr].last = time(0);
   sensors["SP"][addr].value = value;
   sensors["SP"][addr].text = text;
   sensors["SP"][addr].kind = text == "" ? "value" : "text";
   sensors["SP"][addr].valid = sensors["SP"][addr].kind == "text" ? true : !isNan(value);
}

//***************************************************************************
// Init/Exit Database
//***************************************************************************

cDbFieldDef xmlTimeDef("XML_TIME", "xmltime", cDBS::ffAscii, 20, cDBS::ftData);
cDbFieldDef rangeFromDef("RANGE_FROM", "rfrom", cDBS::ffDateTime, 0, cDBS::ftData);
cDbFieldDef rangeToDef("RANGE_TO", "rto", cDBS::ffDateTime, 0, cDBS::ftData);
cDbFieldDef avgValueDef("AVG_VALUE", "avalue", cDBS::ffFloat, 122, cDBS::ftData);
cDbFieldDef maxValueDef("MAX_VALUE", "mvalue", cDBS::ffFloat, 122, cDBS::ftData);
cDbFieldDef rangeEndDef("time", "time", cDBS::ffDateTime, 0, cDBS::ftData);

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

      tell(eloDb, "Checking database connection ...");

      if (connection->attachConnection() != success)
      {
         tell(eloAlways, "Error: Initial database connect failed");
         return fail;
      }

      tell(eloDb, "Checking table structure and indices ...");

      for (auto t = dbDict.getFirstTableIterator(); t != dbDict.getTableEndIterator(); t++)
      {
         cDbTable* table = new cDbTable(connection, t->first.c_str());

         if (strstr(table->TableName(), "information_schema"))
         {
            tell(eloAlways, "Skipping check of table '%s'", t->first.c_str());
            delete table;
            continue;
         }

         tell(eloDb, "Checking table '%s'", t->first.c_str());

         if (!table->exist())
         {
            if ((status += table->createTable()) != success)
            {
               continue;
               delete table;
            }
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

      tell(eloDb, "Checking table structure and indices succeeded");
   }

   // ------------------------
   // create/open tables
   // ------------------------

   tableTableStatistics = new cDbTable(connection, "information_schema.TABLES", true /*read-only*/);
   if (tableTableStatistics->open() != success) return fail;

   tableValueFacts = new cDbTable(connection, "valuefacts");
   if (tableValueFacts->open() != success) return fail;

   tableValueTypes = new cDbTable(connection, "valuetypes");
   if (tableValueTypes->open() != success) return fail;

   tableSamples = new cDbTable(connection, "samples");
   if (tableSamples->open() != success) return fail;

   tablePeaks = new cDbTable(connection, "peaks");
   if (tablePeaks->open() != success) return fail;

   tableConfig = new cDbTable(connection, "config");
   if (tableConfig->open() != success) return fail;

   tableScripts = new cDbTable(connection, "scripts");
   if (tableScripts->open() != success) return fail;

   tableSensorAlert = new cDbTable(connection, "sensoralert");
   if (tableSensorAlert->open() != success) return fail;

   tableUsers = new cDbTable(connection, "users");
   if (tableUsers->open() != success) return fail;

   tableGroups = new cDbTable(connection, "groups");
   if (tableGroups->open() != success) return fail;

   tableDashboards = new cDbTable(connection, "dashboards");
   if (tableDashboards->open() != success) return fail;

   tableDashboardWidgets = new cDbTable(connection, "dashboardwidgets");
   if (tableDashboardWidgets->open() != success) return fail;

   tableSchemaConf = new cDbTable(connection, "schemaconf");
   if (tableSchemaConf->open() != success) return fail;

   tableHomeMatic = new cDbTable(connection, "homematic");
   if (tableHomeMatic->open() != success) return fail;

   // prepare statements

   selectTableStatistic = new cDbStatement(tableTableStatistics);

   selectTableStatistic->build("select ");
   selectTableStatistic->bindAllOut();
   selectTableStatistic->build(" from %s where ", tableTableStatistics->TableName());
   selectTableStatistic->bind("SCHEMA", cDBS::bndIn | cDBS::bndSet);
   selectTableStatistic->build(" order by (data_length + index_length) desc");

   status += selectTableStatistic->prepare();

   // ------------------

   selectActiveValueFacts = new cDbStatement(tableValueFacts);

   selectActiveValueFacts->build("select ");
   selectActiveValueFacts->bindAllOut();
   selectActiveValueFacts->build(" from %s where", tableValueFacts->TableName());
   selectActiveValueFacts->build(" state = 'A' or record = 'A'");
   selectActiveValueFacts->build(" order by type, address");

   status += selectActiveValueFacts->prepare();

   // ------------------

   selectValueFactsByType = new cDbStatement(tableValueFacts);

   selectValueFactsByType->build("select ");
   selectValueFactsByType->bindAllOut();
   selectValueFactsByType->build(" from %s where ", tableValueFacts->TableName());
   selectValueFactsByType->bind("TYPE", cDBS::bndIn | cDBS::bndSet);

   status += selectValueFactsByType->prepare();

   // ------------------

   selectAllValueTypes = new cDbStatement(tableValueTypes);

   selectAllValueTypes->build("select ");
   selectAllValueTypes->bindAllOut();
   selectAllValueTypes->build(" from %s", tableValueTypes->TableName());

   status += selectAllValueTypes->prepare();

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
   // ein sample avg / Stunde

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
   // ein sample avg / 360 Minuten (6 Stunden)

   selectSamplesRange360 = new cDbStatement(tableSamples);

   selectSamplesRange360->build("select ");
   selectSamplesRange360->bind("ADDRESS", cDBS::bndOut);
   selectSamplesRange360->bind("TYPE", cDBS::bndOut, ", ");
   selectSamplesRange360->bindTextFree("date_format(time, '%Y-%m-%dT%H:%i')", &xmlTime, ", ", cDBS::bndOut);
   selectSamplesRange360->bindTextFree("avg(value)", &avgValue, ", ", cDBS::bndOut);
   selectSamplesRange360->bindTextFree("max(value)", &maxValue, ", ", cDBS::bndOut);
   selectSamplesRange360->build(" from %s where ", tableSamples->TableName());
   selectSamplesRange360->bind("ADDRESS", cDBS::bndIn | cDBS::bndSet);
   selectSamplesRange360->bind("TYPE", cDBS::bndIn | cDBS::bndSet, " and ");
   selectSamplesRange360->bindCmp(0, "TIME", &rangeFrom, ">=", " and ");
   selectSamplesRange360->bindCmp(0, "TIME", &rangeTo, "<=", " and ");
   selectSamplesRange360->build(" group by date(time), floor(hour(time)/6)");
   selectSamplesRange360->build(" order by time");

   status += selectSamplesRange360->prepare();

   // ------------------
   // select samples for chart data
   // ein sample avg / month

   selectSamplesRangeMonth = new cDbStatement(tableSamples);

   selectSamplesRangeMonth->build("select ");
   selectSamplesRangeMonth->bind("ADDRESS", cDBS::bndOut);
   selectSamplesRangeMonth->bind("TYPE", cDBS::bndOut, ", ");
   selectSamplesRangeMonth->bindTextFree("date_format(time, '%Y-%m')", &xmlTime, ", ", cDBS::bndOut);
   selectSamplesRangeMonth->bindTextFree("avg(value)", &avgValue, ", ", cDBS::bndOut);
   selectSamplesRangeMonth->bindTextFree("max(value)", &maxValue, ", ", cDBS::bndOut);
   selectSamplesRangeMonth->build(" from %s where ", tableSamples->TableName());
   selectSamplesRangeMonth->bind("ADDRESS", cDBS::bndIn | cDBS::bndSet);
   selectSamplesRangeMonth->bind("TYPE", cDBS::bndIn | cDBS::bndSet, " and ");
   selectSamplesRangeMonth->bindCmp(0, "TIME", &rangeFrom, ">=", " and ");
   selectSamplesRangeMonth->bindCmp(0, "TIME", &rangeTo, "<=", " and ");
   selectSamplesRangeMonth->build(" group by year(time), month(time)");
   selectSamplesRangeMonth->build(" order by time");

   status += selectSamplesRangeMonth->prepare();

   // ------------------
   // ein sample (Summe) pro Monat für den Tages 'max Wert'

   // select date_format(time, '%Y-%m'), sum(value) from samples
   //  where type = '?' and address = ?
   //   and time in (select max(time) from samples where
   //     type = 'GROWATT' and address = 53
   //    group by year(time), month(time), day(time)) group by year(time), month(time);

   selectSamplesRangeMonthOfDayMax = new cDbStatement(tableSamples);

   selectSamplesRangeMonthOfDayMax->build("select ");
   selectSamplesRangeMonthOfDayMax->bind("ADDRESS", cDBS::bndOut);
   selectSamplesRangeMonthOfDayMax->bind("TYPE", cDBS::bndOut, ", ");
   selectSamplesRangeMonthOfDayMax->bindTextFree("date_format(time, '%Y-%m')", &xmlTime, ", ", cDBS::bndOut);
   selectSamplesRangeMonthOfDayMax->bindTextFree("sum(value)", &maxValue, ", ", cDBS::bndOut);
   selectSamplesRangeMonthOfDayMax->build(" from %s where ", tableSamples->TableName());
   selectSamplesRangeMonthOfDayMax->bind("ADDRESS", cDBS::bndIn | cDBS::bndSet);
   selectSamplesRangeMonthOfDayMax->bind("TYPE", cDBS::bndIn | cDBS::bndSet, " and ");
   selectSamplesRangeMonthOfDayMax->bindCmp(0, "TIME", &rangeFrom, ">=", " and ");
   selectSamplesRangeMonthOfDayMax->bindCmp(0, "TIME", &rangeTo, "<=", " and ");
   selectSamplesRangeMonthOfDayMax->build(" and time in (select max(time) from %s where ", tableSamples->TableName());
   selectSamplesRangeMonthOfDayMax->bind("ADDRESS", cDBS::bndIn | cDBS::bndSet);
   selectSamplesRangeMonthOfDayMax->bind("TYPE", cDBS::bndIn | cDBS::bndSet, " and ");
   selectSamplesRangeMonthOfDayMax->build(" group by year(time), month(time), day(time))");
   selectSamplesRangeMonthOfDayMax->build(" group by year(time), month(time)");
   selectSamplesRangeMonthOfDayMax->build(" order by time");

   status += selectSamplesRangeMonthOfDayMax->prepare();

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

   selectDashboards = new cDbStatement(tableDashboards);

   selectDashboards->build("select ");
   selectDashboards->bindAllOut();
   selectDashboards->build(" from %s order by ord", tableDashboards->TableName());

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
   selectDashboardWidgetsFor->bindAllOut();
   selectDashboardWidgetsFor->build(" from %s where ", tableDashboardWidgets->TableName());
   selectDashboardWidgetsFor->bind("DASHBOARDID", cDBS::bndIn | cDBS::bndSet);
   selectDashboardWidgetsFor->build(" order by ord");

   status += selectDashboardWidgetsFor->prepare();

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

   if (status == success)
      tell(eloDb, "Connection to database established");

   int gCount {0};

   if (connection->query(gCount, "select * from groups") == success)
   {
      if (!gCount)
      {
         connection->query("insert into groups set name='Heizung'");
         connection->query("update valuefacts set groupid = 1 where groupid is null or groupid = 0");
      }
   }

   // ------------------

   selectHomeMaticByUuid = new cDbStatement(tableHomeMatic);

   selectHomeMaticByUuid->build("select ");
   selectHomeMaticByUuid->bindAllOut();
   selectHomeMaticByUuid->build(" from %s where ", tableHomeMatic->TableName());
   selectHomeMaticByUuid->bind("UUID", cDBS::bndIn | cDBS::bndSet);

   status += selectHomeMaticByUuid->prepare();

   // -------------------

   tableValueFacts->clear();

   for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
      initSensorByFact(tableValueFacts->getStrValue("TYPE"), tableValueFacts->getIntValue("ADDRESS"));

   selectActiveValueFacts->freeResult();

   /*
   // patch dashbors widget options to default
   tableDashboardWidgets->clear();
   tableDashboardWidgets->setValue("DASHBOARDID", 5);
   for (int f = selectDashboardWidgetsFor->find(); f; f = selectDashboardWidgetsFor->fetch())
   {
      if (tableDashboardWidgets->hasValue("TYPE", "DZL"))
      {
         tableValueFacts->clear();
         tableValueFacts->setValue("TYPE", tableDashboardWidgets->getStrValue("TYPE"));
         tableValueFacts->setValue("ADDRESS", tableDashboardWidgets->getIntValue("ADDRESS"));

         if (tableValueFacts->find())
         {
            json_t* jDefaults = json_object();
            widgetDefaults2Json(jDefaults, tableValueFacts->getStrValue("TYPE"),
                                tableValueFacts->getStrValue("UNIT"), "",
                                tableValueFacts->getIntValue("ADDRESS"));
            char* opts = json_dumps(jDefaults, JSON_REAL_PRECISION(4));
            tell(eloAlways, "PATCH: %s/%ld to '%s'", tableValueFacts->getStrValue("TYPE"), tableValueFacts->getIntValue("ADDRESS"), opts);
            tableDashboardWidgets->setValue("WIDGETOPTS", opts);
            tableDashboardWidgets->store();
            free(opts);
            json_decref(jDefaults);
         }
         tableValueFacts->reset();
      }
   }
   selectDashboardWidgetsFor->freeResult();
   */

   int count {0};

   // -------------------
   // init table valuetypes - if emty

   count = 0;
   tableValueTypes->countWhere("1=1", count);

   if (!count)
   {
      tableValueFacts->clear();
      for (int f = selectAllValueFacts->find(); f; f = selectAllValueFacts->fetch())
      {
         tableValueTypes->clear();
         tableValueTypes->setValue("TYPE", tableValueFacts->getStrValue("TYPE"));

         if (!tableValueTypes->find())
         {
            tableValueTypes->setValue("TITLE", getTitleOfType(tableValueFacts->getStrValue("TYPE")));
            tableValueTypes->store();
         }
      }
      selectAllValueFacts->freeResult();
   }

   // -------------------
   // create at least one dashboard

   count = 0;
   tableDashboards->countWhere("1=1", count);

   if (!count)
   {
      tell(eloInfo, "Starting with empty dashboard");
      tableDashboards->setValue("TITLE", "");
      tableDashboards->setValue("SYMBOL", "mdi:mdi-home");
      tableDashboards->store();
   }

   return status;
}

int Daemon::exitDb()
{
   delete tableTableStatistics;    tableTableStatistics = nullptr;
   delete tableSamples;            tableSamples = nullptr;
   delete tablePeaks;              tablePeaks = nullptr;
   delete tableValueFacts;         tableValueFacts = nullptr;
   delete tableValueTypes;         tableValueTypes = nullptr;
   delete tableConfig;             tableConfig = nullptr;
   delete tableUsers;              tableUsers = nullptr;
   delete tableGroups;             tableGroups = nullptr;
   delete tableScripts;            tableScripts = nullptr;
   delete tableSensorAlert;        tableSensorAlert = nullptr;
   delete tableDashboards;         tableDashboards = nullptr;
   delete tableDashboardWidgets;   tableDashboardWidgets = nullptr;
   delete tableSchemaConf;         tableSchemaConf = nullptr;
   delete tableHomeMatic;          tableHomeMatic = nullptr;

   delete selectTableStatistic;    selectTableStatistic = nullptr;
   delete selectAllGroups;         selectAllGroups = nullptr;
   delete selectActiveValueFacts;  selectActiveValueFacts = nullptr;
   delete selectValueFactsByType;  selectValueFactsByType = nullptr;
   delete selectAllValueFacts;     selectAllValueFacts = nullptr;
   delete selectAllValueTypes;     selectAllValueTypes = nullptr;
   delete selectAllConfig;         selectAllConfig = nullptr;
   delete selectAllUser;           selectAllUser = nullptr;
   delete selectMaxTime;           selectMaxTime = nullptr;
   delete selectSamplesRange;      selectSamplesRange = nullptr;
   delete selectSamplesRange60;    selectSamplesRange60 = nullptr;
   delete selectSamplesRange360;   selectSamplesRange360 = nullptr;
   delete selectSamplesRangeMonth; selectSamplesRangeMonth = nullptr;
   delete selectSamplesRangeMonthOfDayMax; selectSamplesRangeMonthOfDayMax = nullptr;
   delete selectSampleInRange;     selectSampleInRange = nullptr;
   delete selectScriptByPath;      selectScriptByPath = nullptr;
   delete selectScripts;           selectScripts = nullptr;
   delete selectSensorAlerts;      selectSensorAlerts = nullptr;
   delete selectAllSensorAlerts;   selectAllSensorAlerts = nullptr;
   delete selectDashboards;        selectDashboards = nullptr;
   delete selectDashboardById;     selectDashboardById = nullptr;
   delete selectSchemaConfByState; selectSchemaConfByState = nullptr;
   delete selectAllSchemaConf;     selectAllSchemaConf = nullptr;
   delete selectHomeMaticByUuid;   selectHomeMaticByUuid = nullptr;
   delete selectDashboardWidgetsFor; selectDashboardWidgetsFor = nullptr;

   delete connection;              connection = nullptr;

   return done;
}

//***************************************************************************
// Read Configuration
//***************************************************************************

int Daemon::readConfiguration(bool initial)
{
   // init configuration

   if (argEloquence != eloAlways)
   {
      eloquence = argEloquence;
      argEloquence = eloAlways;
   }
   else
   {
      char* elo {};
      getConfigItem("eloquence", elo, "");
      eloquence = Elo::stringToEloquence(elo);
      tell(eloDetail, "Info: Eloquence configured to '%s' => 0x%04x", elo, eloquence);
      free(elo);
   }

   tell(eloAlways, "Info: Eloquence set to 0x%04x", eloquence);

   getConfigItem("interval", interval, interval);
   getConfigItem("arduinoInterval", arduinoInterval, arduinoInterval);
   getConfigItem("webPort", webPort, webPort);
   getConfigItem("webUrl", webUrl);
   getConfigItem("webSsl", webSsl);
   getConfigItem("iconSet", iconSet, "light");

   char* tmp {};
   getConfigItem("schema", tmp);
   free(tmp);

   char* port {};
   asprintf(&port, "%d", webPort);
   if (isEmpty(webUrl) || !strstr(webUrl, port))
   {
      asprintf(&webUrl, "http%s://%s:%d", webSsl ? "s" : "", getFirstIp(), webPort);
      setConfigItem("webUrl", webUrl);
   }
   free(port);

   // getConfigItem("invertDO", invertDO, yes);

   char* addrs {};
   getConfigItem("addrsDashboard", addrs, "");
   addrsDashboard = split(addrs, ',');
   free(addrs);

   getConfigItem("mail", sendMails, no);
   getConfigItem("mailScript", mailScript);
   getConfigItem("stateMailTo", stateMailTo);
   getConfigItem("errorMailTo", errorMailTo);

   getConfigItem("aggregateInterval", aggregateInterval);
   getConfigItem("aggregateHistory", aggregateHistory);

   // DECONZ

   char* deconzUrl {};
   getConfigItem("deconzHttpUrl", deconzUrl, "");

   if (!isEmpty(deconzUrl))
   {
      deconz.setHttpUrl(deconzUrl);
      free(deconzUrl);
      char* deconzKey {};
      getConfigItem("deconzApiKey", deconzKey, "");

      if (!isEmpty(deconzKey))
         deconz.setApiKey(deconzKey);
      free(deconzKey);
   }

   getConfigItem("instanceName", instanceName);

   // location

   getConfigItem("latitude", latitude);
   getConfigItem("longitude", longitude);

   // OpenWeatherMap

   std::string apiKey = openWeatherApiKey ? openWeatherApiKey : "";
   getConfigItem("openWeatherApiKey", openWeatherApiKey);

   if (!isEmpty(openWeatherApiKey) && openWeatherApiKey != apiKey)
      updateWeather();

   // MQTT

   getConfigItem("mqttUser", mqttUser, mqttUser);
   getConfigItem("mqttPassword", mqttPassword, mqttPassword);

   std::string url = mqttUrl ? mqttUrl : "";
   getConfigItem("mqttUrl", mqttUrl);

   if (url != mqttUrl)
   {
      tell(eloAlways, "Config of MQTT url changed from '%s' to '%s', disconnecting", url.c_str(), mqttUrl);
      mqttDisconnect();
   }

   char* sensorTopics {};
   getConfigItem("mqttSensorTopics", sensorTopics, "+/w1/#");
   mqttSensorTopics = split(sensorTopics, ',');
   free(sensorTopics);
   mqttSensorTopics.push_back(TARGET "2mqtt/ping/#");
   mqttSensorTopics.push_back(TARGET "2mqtt/light/+/set/#");
   mqttSensorTopics.push_back(TARGET "2mqtt/command/#");
   mqttSensorTopics.push_back(TARGET "2mqtt/nodered/#");
   mqttSensorTopics.push_back(TARGET "2mqtt/scripts/#");

   getConfigItem("arduinoTopic", arduinoTopic, "");

   if (!isEmpty(arduinoTopic))
      mqttSensorTopics.push_back(std::string(arduinoTopic) + "/out");

   getConfigItem("homeMaticInterface", homeMaticInterface, false);

   getConfigItem("lmcHost", lmcHost, "");
   getConfigItem("lmcPort", lmcPort, 9090);
   getConfigItem("lmcPlayerMac", lmcPlayerMac, "");

   // Home Automation MQTT

   getConfigItem("mqttDataTopic", mqttHaDataTopic, TARGET "2mqtt/<TYPE>/<NAME>/state");
   getConfigItem("mqttSendWithKeyPrefix", mqttHaSendWithKeyPrefix, "");
   getConfigItem("mqttHaveConfigTopic", mqttHaHaveConfigTopic, false);

   if (mqttHaDataTopic[strlen(mqttHaDataTopic)-1] == '/')
      mqttHaDataTopic[strlen(mqttHaDataTopic)-1] = '\0';

   if (isEmpty(mqttHaDataTopic) || isEmpty(mqttUrl))
      mqttHaInterfaceStyle = misNone;
   else if (strstr(mqttHaDataTopic, "<NAME>"))
      mqttHaInterfaceStyle = misMultiTopic;
   else if (strstr(mqttHaDataTopic, "<GROUP>"))
      mqttHaInterfaceStyle = misGroupedTopic;
   else
      mqttHaInterfaceStyle = misSingleTopic;

   std::string styleName = "None";

   if (mqttHaInterfaceStyle == misMultiTopic)
      styleName = "MultiTopic";
   else if (mqttHaInterfaceStyle == misGroupedTopic)
      styleName = "GroupedTopic";
   else if (mqttHaInterfaceStyle == misSingleTopic)
      styleName = "SingleTopic";

   tell(eloAlways, "MQTT interface style set to '%s' for [%s]", styleName.c_str(), mqttHaDataTopic);

   for (int f = selectAllGroups->find(); f; f = selectAllGroups->fetch())
      groups[tableGroups->getIntValue("ID")].name = tableGroups->getStrValue("NAME");

   selectAllGroups->freeResult();

   return done;
}

//***************************************************************************
// Do Sleep
//***************************************************************************

void Daemon::doSleep(int t)
{
   time_t end = time(0) + t;

   while (time(0) < end && !doShutDown())
      usleep(5000);
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

int Daemon::standbyUntil()
{
   while (time(0) < nextRefreshAt && !doShutDown())
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
   static time_t lastPingAt {0};

   if (!initialized)
      return done;

   if (!connection || !connection->isConnected())
      return fail;

   if (lastPingAt+2 <= time(0))
   {
      lastPingAt = time(0);
      json_t* oJson = json_object();
      pushOutMessage(oJson, "ping", 0);
   }

   tell(eloDebug2, "loop ...");

   atMeanwhile();

   dispatchClientRequest();
   dispatchDeconz();
   performMqttRequests();
   performJobs();
   performLmcUpdates();

   return done;
}

//***************************************************************************
// Loop
//***************************************************************************

int Daemon::loop()
{
   tell(eloAlways, "%s started", TARGET);

   scheduleAggregate();

   while (!doShutDown())
   {
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

      standbyUntil();

      if (doLoop() != success)
         continue;

      // refresh expected?

      if (time(0) < nextRefreshAt)
         continue;

      nextRefreshAt = time(0) + interval;

      // aggregate

      if (aggregateHistory && nextAggregateAt <= time(0))
         aggregate();

      // work

      updateWeather();
      updateSensors();  // update some sensors for wich we get no trigger

      performData(0L);
      updateScriptSensors();
      process();
      storeSamples();
      afterUpdate();

      initialRun = false;
   }

   return success;
}

//***************************************************************************
// Store Samples
//***************************************************************************

int Daemon::storeSamples()
{
   int count {0};
   int skipped {0};

   lastSampleTime = time(0);
   tell(eloInfo, "Store samples ..");
   connection->startTransaction();

   if (mqttHaInterfaceStyle == misSingleTopic)
       oHaJson = json_object();

   for (const auto& typeSensorsIt : sensors)
   {
      for (const auto& sensorIt : typeSensorsIt.second)
      {
         const SensorData* sensor = &sensorIt.second;

         if (!sensor->record || sensor->type == "WEA")
            continue;

         if (store(lastSampleTime, sensor) == success)
            count++;
         else
            skipped++;
      }
   }

   lastStore = time(0);
   connection->commit();
   tell(eloInfo, "Stored %d samples, skipped %d", count, skipped);

   return success;
}

//***************************************************************************
// Store
//***************************************************************************

int Daemon::store(time_t now, const SensorData* sensor)
{
   if (!sensor->type.length())
   {
      tell(eloDebug, "Debug: Missing type of '%s:0x%02x' (%s)",
           sensor->type.c_str(), sensor->address, sensor->name.c_str());
      return ignore;
   }

   if (sensor->disabled)
   {
      tell(eloDebug, "Debug: Sensor of '%s:0x%02x' (%s) disabled",
           sensor->type.c_str(), sensor->address, sensor->name.c_str());
      return ignore;
   }

   if (isNan(sensor->value))
   {
      tell(eloAlways, "Warning: Data of '%s:0x%02x' (%s) is NaN, skipping store",
           sensor->type.c_str(), sensor->address, sensor->name.c_str());
      return ignore;
   }

   if (!sensor->last)
   {
      tell(lastStore ? eloAlways : eloDetail, "Warning: Missing data stamp of '%s:0x%02x' (%s), skipping store",
           sensor->type.c_str(), sensor->address, sensor->name.c_str());
      return ignore;
   }

   if (sensor->last <= lastStore)
   {
      tell(eloDetail, "Info: No update for '%s:0x%02x' (%s) until last store, skipping store (%s / %s)",
           sensor->type.c_str(), sensor->address, sensor->name.c_str(), l2pTime(sensor->last).c_str(), l2pTime(lastStore).c_str());
      return ignore;
   }

   tableSamples->clear();

   tableSamples->setValue("TIME", now);
   tableSamples->setValue("ADDRESS", (long)sensor->address);
   tableSamples->setValue("TYPE", sensor->type.c_str());
   tableSamples->setValue("AGGREGATE", "S");
   tableSamples->setValue("SAMPLES", 1);

   if (sensor->kind == "status")
      tableSamples->setValue("VALUE", (double)sensor->state);
   else if (sensor->kind == "value")
      tableSamples->setValue("VALUE", sensor->value);

   // send text if set, independent of 'kind'

   if (sensor->text != "")
      tableSamples->setValue("TEXT", sensor->text.c_str());

   tableSamples->store();

   // peaks

   tablePeaks->clear();

   tablePeaks->setValue("ADDRESS", (long)sensor->address);
   tablePeaks->setValue("TYPE", sensor->type.c_str());

   if (!tablePeaks->find())
   {
      tablePeaks->setValue("MIN", sensor->value);
      tablePeaks->setValue("MAX", sensor->value);
      tablePeaks->store();
   }
   else
   {
      if (sensor->value > tablePeaks->getFloatValue("MAX"))
         tablePeaks->setValue("MAX", sensor->value);

      if (sensor->value < tablePeaks->getFloatValue("MIN"))
         tablePeaks->setValue("MIN", sensor->value);

      tablePeaks->store();
   }

   return success;
}

//***************************************************************************
// Process
//***************************************************************************

int Daemon::process()
{
   // calculate CV sensors by LUA

   for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
   {
      if (!tableValueFacts->hasValue("TYPE", "CV"))
         continue;

      const char* type = tableValueFacts->getStrValue("TYPE");
      uint address = tableValueFacts->getIntValue("ADDRESS");
      char* key {};
      asprintf(&key, "%s:0x%02x", type, address);

      tell(eloLua, "LUA: Processing '%s'", key);

      if (tableValueFacts->getValue("CALIBRATION")->isEmpty())
      {
         tell(eloLua, "LUA: Skip processing '%s' with empty script", key);
         free(key);
         continue;
      }

      if (!sensors[type][address].last)
         getConfigItem(key, sensors[type][address].value, 0);

      std::string expression = tableValueFacts->getStrValue("CALIBRATION");
      bool update {false};
      std::string searchKey = getStringBetween(expression, "{", "}");

      while (searchKey.length())
      {
         std::string what = "{" + searchKey + "}";
         auto tuple = split(searchKey, ':');

         if (tuple.size() >= 2 && sensors.find(tuple[0].c_str()) != sensors.end())
         {
            uint matchAddr = strtol(tuple[1].c_str(), nullptr, 0);

            if (sensors[tuple[0].c_str()].find(matchAddr) != sensors[tuple[0].c_str()].end())
            {
               if (tuple.size() == 3 && tuple[2] == "last")
               {
                  expression = strReplace(what, sensors[tuple[0].c_str()][matchAddr].last, expression);
                  tell(eloDebug, "Debug: LUA: Replace %s:0x%x with '%ld'", tuple[0].c_str(), matchAddr, sensors[tuple[0].c_str()][matchAddr].last);
               }
               else
               {
                  expression = strReplace(what, sensors[tuple[0].c_str()][matchAddr].value, expression, ".");
                  tell(eloDebug, "Debug: LUA: Replace %s:0x%x with '%f'", tuple[0].c_str(), matchAddr, sensors[tuple[0].c_str()][matchAddr].value);
               }

               if (sensors[tuple[0].c_str()][matchAddr].last > sensors[type][address].last)
               {
                  // tell(eloLua, "LUA: %s (%ld) newer than %s (%ld)", searchKey.c_str(), sensors[tuple[0].c_str()][matchAddr].last, key, sensors[type][address].last);
                  update = true;
               }
            }
            else
            {
               tell(eloAlways, "Warning: LUA: (%s) Sensor '%s:0x%x' not found, ignoring", key, searchKey.c_str(), matchAddr);
               break;
            }
         }
         else
         {
            tell(eloAlways, "Warning: LUA: (%s) Sensor type '%s' not found, ignoring", key, searchKey.c_str());
            break;
         }

         searchKey = getStringBetween(expression, "{", "}");
      }

      if (!update)
      {
         tell(eloLua, "LUA: '%s' skipping call, no change of input parameters", key);
         free(key);
         continue;
      }

      // call LUA script

      std::vector<std::string> arguments;
      Lua::Result res;
      Lua lua;

      if (lua.executeExpression(expression.c_str(), arguments, res) != success)
      {
         tell(eloAlways, "Error: Calling LUA expression '%s' with (%zu) arguments failed",
              expression.c_str(), arguments.size());
         free(key);
         continue;
      }

      double oldValue = sensors[type][address].value;

      if (res.type == Lua::tDouble)
      {
         tell(eloLua, "LUA '%s' result was %f [%s]", key, res.dValue, expression.c_str());
         sensors[type][address].value = res.dValue;
      }
      else if (res.type == Lua::tInteger)
      {
         tell(eloLua, "LUA '%s' result was %d [%s]", key, res.iValue, expression.c_str());
         sensors[type][address].value = res.iValue;
      }
      else
         tell(eloAlways, "LUA: '%s' got unexpected type (%d)", key, res.type);

      sensors[type][address].last = time(0);
      sensors[type][address].valid = true;
      setConfigItem(key, sensors[type][address].value);

      // tell(eloLua, "LUA '%s' changed from %f to %f", key, oldValue, sensors[type][address].value);

      if (oldValue != sensors[type][address].value)
      {
         // update WS
         {
            // tell(eloLua, "LUA '%s' update WS to %f", key, sensors[type][address].value);

            json_t* ojData = json_object();

            sensor2Json(ojData, type, address);
            json_object_set_new(ojData, "value", json_real(sensors[type][address].value));

            jsonSensorList[key] = ojData;
            pushDataUpdate("update", 0L);
         }

         mqttHaPublish(sensors[type][address]);
         mqttNodeRedPublishSensor(sensors[type][address]);
      }

      free(key);
   }

   selectActiveValueFacts->freeResult();

   return done;
}

//***************************************************************************
// Update Script Sensors
//***************************************************************************

void Daemon::updateScriptSensors()
{
   tableValueFacts->clear();

   tell(eloInfo, "Update script sensors");

   for (const auto& cmdThreadCtl : commandThreads)
   {
      if (!cmdThreadCtl.second.active)
         pthread_join(cmdThreadCtl.second.pThread, 0);
   }

   // std::erase_if(commandThreads, [](const auto& item)
   //    { auto const& [key, value] = item; return !value.active; });

   for (auto it = commandThreads.begin(), it_next = it; it != commandThreads.end(); it = it_next)
   {
      ++it_next;

      if (it->second.active)
      {
         tell(eloInfo, "removing thread for '%s'", it->second.command.c_str());
         commandThreads.erase(it);
      }
   }

   for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
   {
      if (!tableValueFacts->hasValue("TYPE", "SC"))
         continue;

      uint addr = tableValueFacts->getIntValue("ADDRESS");
      callScript(addr, "status");
   }

   selectActiveValueFacts->freeResult();
}

//***************************************************************************
// After Update
//***************************************************************************

void Daemon::afterUpdate()
{
   sensorAlertCheck(lastSampleTime);

   char* path {};
   asprintf(&path, "%s/after-update.sh", confDir);

   if (fileExists(path))
   {
      tell(eloInfo, "Calling '%s'", path);
      system(path);
   }

   free(path);
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
// Add To Alert Mail
//***************************************************************************

int Daemon::add2AlertMail(cDbRow* alertRow, const char* title, double value, const char* unit)
{
   char* sensor {};

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
// Update Conf Tables
//***************************************************************************

int Daemon::updateSchemaConfTable()
{
   const int step = 20;
   int y = 50;
   int added = 0;

   tableValueFacts->clear();

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

         tableSchemaConf->store();
         added++;
      }
   }

   selectActiveValueFacts->freeResult();
   tell(eloInfo, "Added %d html schema configurations", added);

   return success;
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
         tell(eloDebug2, "Debug2: %d in time range '%d-%d'", l2hhmm(t), it->from, it->to);
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
   struct tm tm = {0};
   time_t now {0};

   if (!aggregateHistory)
   {
      tell(eloInfo, "NO aggregateHistory configured!");
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
   char* stmt {};
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

      if (tableSamples->deleteWhere("%s", stmt) == success)
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
   char* command {};
   int result {0};

   asprintf(&command, "%s '%s' '%s' '%s' '%s'", mailScript, subject, body, mimeType, receiver);
   result = system(command);
   free(command);

   if (eloquence & eloDebug)
      tell(eloInfo, "Send mail '%s' with [%s] to '%s'", subject, body, receiver);
   else
      tell(eloInfo, "Send mail '%s' to '%s'", subject, receiver);

   return result;
}

//***************************************************************************
// Load Html Header
//***************************************************************************

int Daemon::loadHtmlHeader()
{
   char* file {};

   // load only once at first call

   if (htmlHeader.length())
      return done;

   asprintf(&file, "%s/mail-head.html", confDir);

   if (fileExists(file))
   {
      MemoryStruct data;
      if (loadFromFile(file, &data) == success)
      {
         data.append("\0");
         htmlHeader = data.memory;
      }
   }

   free(file);

   if (htmlHeader.length())
      return success;

   htmlHeader =
      "  <head>\n"
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
      "  </head>\n";

   return success;
}

//***************************************************************************
// Add Value Fact
//***************************************************************************

int Daemon::addValueFact(int addr, const char* type, int factor, const char* name, const char* unit,
                         const char* aTitle, int rights, const char* choices, SensorOptions options)

{
   const char* title = !isEmpty(aTitle) ? aTitle : name;

   // check / add to valueTypes

   tableValueTypes->clear();
   tableValueTypes->setValue("TYPE", type);

   if (!tableValueTypes->find())
   {
      tableValueTypes->setValue("TITLE", getTitleOfType(type));
      tableValueTypes->store();
   }

   tableValueFacts->clear();
   tableValueFacts->setValue("TYPE", type);
   tableValueFacts->setValue("ADDRESS", addr);

   //

   if (!tableValueFacts->find())
   {
      tell(eloAlways, "Add ValueFact '%u' '%s'", addr, type);

      tableValueFacts->setValue("NAME", name);
      tableValueFacts->setValue("TITLE", title);
      tableValueFacts->setValue("FACTOR", factor);
      tableValueFacts->setValue("RIGHTS", rights);
      tableValueFacts->setValue("STATE", "D");
      tableValueFacts->setValue("OPTIONS", options);

      if (!isEmpty(unit))
         tableValueFacts->setValue("UNIT", unit);

      if (!isEmpty(choices))
         tableValueFacts->setValue("CHOICES", choices);

      tableValueFacts->store();
      initSensorByFact(type, addr);
      return 1;                               // 1 for 'added'
   }

   // already exist, update ...

   tableValueFacts->clearChanged();

   tableValueFacts->setValue("NAME", name);
   tableValueFacts->setValue("TITLE", title);
   tableValueFacts->setValue("FACTOR", factor);
   tableValueFacts->setValue("RIGHTS", rights);

   // if (tableValueFacts->getValue("OPTIONS")->isNull())
   tableValueFacts->setValue("OPTIONS", options);

   // if (tableValueFacts->getValue("UNIT")->isNull())
   if (!tableValueFacts->hasValue("UNIT", unit))
      tableValueFacts->setValue("UNIT", unit);

   if (!isEmpty(choices))
      tableValueFacts->setValue("CHOICES", choices);

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

int Daemon::dispatchMqttHaCommandRequest(json_t* jData, const char* topic)
{
   auto it = hassCmdTopicMap.find(topic);

   if (it != hassCmdTopicMap.end())
   {
      const char* state = getStringFromJson(jData, "state", "");

      if (isEmpty(state))
         return fail;

      for (auto itOutput = sensors["DO"].begin(); itOutput != sensors["DO"].end(); ++itOutput)
      {
         bool bState = strcmp(state, "ON") == 0;

         if (it->second == itOutput->second.name)
         {
            gpioWrite(itOutput->first, bState);
            break;
         }
      }
   }

   return success;
}

//***************************************************************************
// Dispatch Node-Red Command's Request
//   Format:  '[{ "command" : "set", "id" : 'SC:0x9', "value" : "on|off" }, ...]'
//***************************************************************************

int Daemon::dispatchNodeRedCommands(const char* topic, json_t* jObject)
{
   if (json_is_array(jObject))
   {
      size_t index {0};
      json_t* jCommand {};

      json_array_foreach(jObject, index, jCommand)
         dispatchNodeRedCommand(jCommand);
   }
   else
   {
      dispatchNodeRedCommand(jObject);
   }

   return success;
}

//***************************************************************************
// Dispatch Node-Red Command Request
//   Format:  '{ "command" : "set", "id" : 'SC:0x9', "value" : "on|off", "bri" : 100 }'
//***************************************************************************

int Daemon::dispatchNodeRedCommand(json_t* jObject)
{
   const char* command = getStringFromJson(jObject, "command", "set");
   const char* key = getStringFromJson(jObject, "id");

   if (!key)
   {
      tell(eloAlways, "Error: Node-Red: Got unexpected id '%s' from node-red, commad was '%s'", key, command);
      return fail;
   }

   int bri = getIntFromJson(jObject, "bri", na);
   int transitiontime = getIntFromJson(jObject, "transitiontime", na);
   const char* sState = getStringFromJson(jObject, "state", "off");
   if (!sState) sState = "off";
   bool state = strcasecmp(sState, "on") == 0;

   auto tuple = split(key, ':');
   uint address = strtol(tuple[1].c_str(), nullptr, 0);
   tell(eloNodeRed, "Node-Red: switch %s to %d (%d/%d)", key, state, bri, transitiontime);
   toggleIo(address, tuple[0].c_str(), state, bri, transitiontime);

   return success;
}

//***************************************************************************
// Update Weather
//
// Format:
//   { "weather":[{"id":804,"main":"Clouds","description":"Bedeckt","icon":"04d"}],
//     "main":{"temp":8.49,"feels_like":8.49,"temp_min":7.1,"temp_max":9.55,"pressure":1008,"humidity":89},
//     "visibility":10000,
//     "wind":{"speed":0.89,"deg":238,"gust":3.58},
//     "clouds":{"all":100},
//     "dt":1640765212,
//     "sys":{"type":2,"id":2012565,"country":"DE","sunrise":1640762680,"sunset":1640791732},
//     "timezone":3600, "id":2955014, "name":"Assenheim", "cod":200}
//
//  https://openweathermap.org/current#min
//  https://openweathermap.org/forecast5
//***************************************************************************

int Daemon::updateWeather()
{
   static time_t nextWeatherAt {0};

   if (isEmpty(openWeatherApiKey))
      return done;

   if (time(0) < nextWeatherAt)
      return done;

   nextWeatherAt = time(0) + 30 * tmeSecondsPerMinute;

   cCurl curl;
   curl.init();

   MemoryStruct data;
   int size {0};
   char* url {};

   asprintf(&url, "http://api.openweathermap.org/data/2.5/forecast?appid=%s&units=metric&lang=de&lat=%f&lon=%f",
            openWeatherApiKey, latitude, longitude);

   tell(eloWeather, "-> (openweathermap) [%s]", url);
   int status = curl.downloadFile(url, size, &data, 2);

   if (status != success)
   {
      tell(eloAlways, "Error: Requesting weather at '%s' failed", url);
      free(url);
      return fail;
   }

   free(url);
   tell(eloWeather, "<- (openweathermap) [%s]", data.memory);

   json_t* jData = jsonLoad(data.memory);

   if (!jData)
      return fail;

   const char* city = getStringByPath(jData, "city/name");
   json_t* jArray = getObjectFromJson(jData, "list");

   if (!jArray)
      return fail;

   json_t* jWeather = json_object();
   json_t* jForecasts = json_array();

   json_object_set_new(jWeather, "forecasts", jForecasts);
   json_object_set_new(jWeather, "city", json_string(city));

   size_t index {0};
   json_t* jObj {};

   json_array_foreach(jArray, index, jObj)
   {
      if (index == 0)
         weather2json(jWeather, jObj);

      json_t* jFcItem = json_object();
      json_array_append_new(jForecasts, jFcItem);
      weather2json(jFcItem, jObj);
   }

   addValueFact(1, "WEA", 1, "weather", "txt", "Wetter");
   sensors["WEA"][1].kind = "text";
   sensors["WEA"][1].last = time(0);
   sensors["WEA"][1].valid = true;

   char* p = json_dumps(jWeather, JSON_REAL_PRECISION(4));
   sensors["WEA"][1].text = p;
   free(p);
   json_decref(jWeather);

   {
      json_t* ojData = json_object();
      sensor2Json(ojData, "WEA", 1);
      json_object_set_new(ojData, "text", json_string(sensors["WEA"][1].text.c_str()));

      char* tuple {};
      asprintf(&tuple, "%s:0x%02x", "WEA", 1);
      jsonSensorList[tuple] = ojData;
      free(tuple);

      pushDataUpdate("update", 0L);
   }

   json_decref(jData);

   curl.exit();

   return done;
}

//***************************************************************************
// Weather 2 Json
//***************************************************************************

int Daemon::weather2json(json_t* jWeather, json_t* owmWeather)
{
   json_object_set_new(jWeather, "stime", json_string(l2pTime(getIntFromJson(owmWeather, "dt")).c_str()));
   json_object_set_new(jWeather, "time", json_integer(getIntFromJson(owmWeather, "dt")));
   json_object_set_new(jWeather, "detail", json_string(getStringByPath(owmWeather, "weather[0]/description")));
   json_object_set_new(jWeather, "icon", json_string(getStringByPath(owmWeather, "weather[0]/icon")));
   json_object_set_new(jWeather, "temp", json_real(getDoubleByPath(owmWeather, "main/temp")));
   json_object_set_new(jWeather, "tempfeels", json_real(getDoubleByPath(owmWeather, "main/feels_like")));
   json_object_set_new(jWeather, "tempmax", json_real(getDoubleByPath(owmWeather, "main/temp_max")));
   json_object_set_new(jWeather, "tempmin", json_real(getDoubleByPath(owmWeather, "main/temp_min")));
   json_object_set_new(jWeather, "humidity", json_real(getIntByPath(owmWeather, "main/humidity")));
   json_object_set_new(jWeather, "pressure", json_real(getIntByPath(owmWeather, "main/pressure")));
   json_object_set_new(jWeather, "windspeed", json_real(getDoubleByPath(owmWeather, "wind/speed")));
   json_object_set_new(jWeather, "windgust", json_real(getDoubleByPath(owmWeather, "wind/gust")));
   json_object_set_new(jWeather, "winddir", json_real(getIntByPath(owmWeather, "wind/deg")));

   return done;
}

//***************************************************************************
// Dispatch Deconz
//   Format: {"type": "DZL", "address": 29, "state": false, "bri": 80}
//           {"type": "DZS", "address": 25, "btnevent": 1002}
//           {"type": "DZS", "address": 33, "presence": true}
//***************************************************************************

int Daemon::dispatchDeconz()
{
   cMyMutexLock lock(&Deconz::messagesInMutex);

   while (!Deconz::messagesIn.empty())
   {
      tell(eloDeconz, "<- (DECONZ) '%s'", Deconz::messagesIn.front().c_str());

      json_t* oData = jsonLoad(Deconz::messagesIn.front().c_str());

      const char* type = getStringFromJson(oData, "type");
      uint address = getIntFromJson(oData, "address");
      SensorData* sensor = getSensor(type, address);

      if (!sensor)
      {
         json_decref(oData);      // free the json object
         Deconz::messagesIn.pop();
         return done;
      }

      bool state = getBoolFromJson(oData, "state");
      double value = getDoubleFromJson(oData, "value");
      int bri = getIntFromJson(oData, "bri", na);
      int hue = getIntFromJson(oData, "hue", 0);
      int sat = getIntFromJson(oData, "sat", 0);

      if (getObjectFromJson(oData, "state"))
         sensor->state = state;
      else if (getObjectFromJson(oData, "value"))
         sensor->value = value;
      else if (getObjectFromJson(oData, "btnevent"))
         sensor->value = getIntFromJson(oData, "btnevent");
      else if (getObjectFromJson(oData, "presence"))
         sensor->state = getBoolFromJson(oData, "presence");

      if (getObjectFromJson(oData, "bri"))
         sensor->value = bri;

      if (getObjectFromJson(oData, "hue"))
         sensor->hue = hue;

      if (getObjectFromJson(oData, "sat"))
         sensor->sat = sat;

      sensor->last = time(0);
      sensor->valid = true;

      int battery = getIntFromJson(oData, "battery", na);
      if (battery != na)
         sensor->battery = battery;

      // send update to WS
      {
         json_t* ojData = json_object();
         sensor2Json(ojData, type, address);

         if (getObjectFromJson(oData, "state"))
         {
            json_object_set_new(ojData, "value", json_integer(sensor->state));
            if (getObjectFromJson(oData, "bri"))
               json_object_set_new(ojData, "score", json_integer(bri));
            if (getObjectFromJson(oData, "hue"))
               json_object_set_new(ojData, "hue", json_integer(hue));
            if (getObjectFromJson(oData, "sat"))
               json_object_set_new(ojData, "sat", json_integer(sat));
         }
         else if (getObjectFromJson(oData, "value"))
            json_object_set_new(ojData, "value", json_real(sensor->value));

         if (sensor->battery != na)
            json_object_set_new(ojData, "battery", json_integer(sensor->battery));

         char* tuple {};
         asprintf(&tuple, "%s:0x%02x", type, address);
         jsonSensorList[tuple] = ojData;
         free(tuple);

         pushDataUpdate("update", 0L);
      }

      mqttHaPublish(sensors[type][address]);
      mqttNodeRedPublishSensor(sensors[type][address]);

      json_decref(oData);      // free the json object
      Deconz::messagesIn.pop();
   }

   return success;
}

//***************************************************************************
// Homematic Interface (via node-red)
//***************************************************************************

int Daemon::dispatchHomematicRpcResult(const char* message)
{
   json_t* jData = jsonLoad(message);

   if (!jData)
   {
      tell(eloAlways, "Error: (home-matic) Can't parse json in '%s'", message);
      return fail;
   }

   if (!json_is_array(jData))
      tell(eloHomeMatic, "<- (home-matic) '%s'", message);

   size_t index {0};
   json_t* jItem {};

   json_array_foreach(jData, index, jItem)
   {
      std::string hmType = getStringFromJson(jItem, "TYPE", "");
      const char* uuid = getStringFromJson(jItem, "ADDRESS");

      if (!uuid || hmType != "BLIND")
         continue;

      if (eloquence & eloHomeMatic)
      {
         char* p = json_dumps(jItem, JSON_REAL_PRECISION(4));
         tell(eloHomeMatic, "<- (home-matic) '%s'", p);
         free(p);
      }

      tell(eloHomeMatic, "%zu) '%s', type '%s', direction %d", index, uuid, hmType.c_str(),
           getIntFromJson(jItem, "DIRECTION", na));

      // create value fact for home-matic sensors/actors

      tableHomeMatic->clear();
      tableHomeMatic->setValue("UUID", uuid);

      if (!selectHomeMaticByUuid->find())
      {
         tableHomeMatic->setValue("TYPE", "HMB");
         tableHomeMatic->insert();

         tell(eloDebugHomeMatic, "HomeMatic: Added new '%s', uuid '%s'", hmType.c_str(), uuid);

         tableHomeMatic->clear();
         tableHomeMatic->setValue("UUID", uuid);
         selectHomeMaticByUuid->find();
      }

      int address = tableHomeMatic->getIntValue("ADDRESS");

      tableHomeMatic->setValue("KIND", hmType.c_str());
      tableHomeMatic->setValue("NAME", uuid);
      tableHomeMatic->store();
      selectHomeMaticByUuid->freeResult();

      homeMaticUuids[address] = uuid;

      // HMB -> Home-Matic-Blind

      addValueFact(address, "HMB", 1, uuid, "%", "", urControl);
      sensors["HMB"][address].lastDir = getIntFromJson(jItem, "DIRECTION") == 2 ? dirClose : dirOpen;
      sensors["HMB"][address].value = 100;   // #TODO: request the actual value/postiton !
      sensors["HMB"][address].last = time(0);
      sensors["HMB"][address].valid = true;
   }

   json_decref(jData);
   return done;
}

int Daemon::dispatchHomematicEvents(const char* message)
{
   // { "val" : 0.25,
   //   "hm" : { "device"      : "OEQ1853240",
   //            "channel"     : "OEQ1853240:1",
   //            "channelType" : "BLIND",
   //            "datapoint"   : "LEVEL",
   //            "valueStable" : 0.25 }

   tell(eloHomeMatic, "<- (home-matic) '%s'", message);

   json_t* jData = jsonLoad(message);

   if (!jData)
   {
      tell(eloAlways, "Error: (home-matic) Can't parse json in '%s'", message);
      return fail;
   }

   const char* uuid = getStringByPath(jData, "/hm/channel");
   std::string channelType = getStringByPath(jData, "/hm/channelType");
   std::string datapoint = getStringByPath(jData, "/hm/datapoint");

   if (!uuid || channelType != "BLIND")
      return done;

   tableHomeMatic->clear();
   tableHomeMatic->setValue("UUID", uuid);

   if (!selectHomeMaticByUuid->find())
   {
      json_decref(jData);
      return done;
   }

   const char* type = tableHomeMatic->getStrValue("TYPE");
   long address = tableHomeMatic->getIntValue("ADDRESS");
   double value {sensors[type][address].value};

   tell(eloDebug, "Debug: Got (home-matic) '%s' last value is %d", datapoint.c_str(), (int)sensors[type][address].value);
   selectHomeMaticByUuid->freeResult();

   sensors[type][address].last = time(0);
   sensors[type][address].valid = true;

   // for blinds:
   //  offen -> state true (on)
   //           100% (value 100)

   if (datapoint == "LEVEL")
   {
      value = getDoubleFromJson(jData, "val") * 100;     // to [%]
      sensors[type][address].state = value == 100;        // on wenn ganz offen (offen entspricht=> 100%)
      sensors[type][address].value = value;
   }
   else if (datapoint == "WORKING")
      sensors[type][address].working = getBoolFromJson(jData, "val");
   else if (datapoint == "DIRECTION")
   {
      if (getIntFromJson(jData, "val") != 0)
         sensors[type][address].lastDir = getIntFromJson(jData, "val") == 2 ? dirClose : dirOpen;
   }
   else if (datapoint == "LEVEL_NOTWORKING")
   {
      value = getDoubleFromJson(jData, "val") * 100;    // to [%]
      sensors[type][address].working = false;
   }

   {
      json_t* ojData = json_object();
      sensor2Json(ojData, type, address);
      json_object_set_new(ojData, "value", json_real(value));

      char* tuple {};
      asprintf(&tuple, "%s:0x%02lx", type, address);
      jsonSensorList[tuple] = ojData;
      free(tuple);

      pushDataUpdate("update", 0L);
   }

   mqttHaPublish(sensors[type][address]);
   mqttNodeRedPublishSensor(sensors[type][address]);
   json_decref(jData);

   return done;
}

std::string buildStringFromCamelCase(std::string camelCaseStr)
{
	size_t n {0};

	while (n+1 < camelCaseStr.length())
   {
		if ((islower(camelCaseStr[n]) || isdigit(camelCaseStr[n])) && isupper(camelCaseStr[n+1]))
      {
			camelCaseStr = camelCaseStr.substr(0,n+1) +" " + camelCaseStr.substr(n+1);
			n++;
		}

		n++;
	}

   return camelCaseStr;
}

//***************************************************************************
// GroWatt Interface
//
// Input:
// {
//   "meta": {
//     "mac": "C8:C9:A3:8B:0F:0C"
//   },
//   "data": {
//     "0": {
//       "name": "InverterStatus",
//       "unit": "",
//       "val": 1,
//       "text": "Normal Operation"
//     },
//     "1": {
//       "name": "InputPower",
//       "unit": "W",
//       "val": 684.6
//     },
//     "3": {
//       "name": "PV1Voltage",
//       "unit": "V",
//       "val": 97
//     },
//     "4": {
//       "name": "PV1InputCurrent",
//       "unit": "A",
//       "val": 7.6
//     },
//     ...
//***************************************************************************

int Daemon::dispatchGrowattEvents(const char* message)
{
   json_t* jData = jsonLoad(message);

   if (!jData)
   {
      tell(eloAlways, "Error: (GroWatt) Can't parse json in '%s'", message);
      return fail;
   }

   tell(eloGroWatt, "<- (GroWatt) '%s'", message);
   // json_t* jHoldingRegisters = getObjectFromJson(jData, "param");  // not used yet
   json_t* jInputRegisters = getObjectFromJson(jData, "data");

   if (jInputRegisters)
   {
      const char* key {};
      json_t* jObj {};

      json_object_foreach(jInputRegisters, key, jObj)
      {
         const char* type {"GROWATT"};
         std::string title = getStringFromJson(jObj, "name");
         const char* unit = getStringFromJson(jObj, "unit");
         int address = GroWatt::getAddressOf(title.c_str());  // get old address
         // int address = atoi(key);                          // the new address

         if (address < 0)
         {
            tell(eloAlways, "Warning: Failed to lookup address of Growatt sensor '%s'", key);
            continue;
         }

         std::string text = getStringFromJson(jObj, "text", "");
         json_t* jValue  = getObjectFromJson(jObj, "val");
         double value = json_is_integer(jValue) ? json_integer_value(jValue) : json_real_value(jValue);

         sensors[type][address].kind = "value";
         sensors[type][address].value = value;
         sensors[type][address].last = time(0);
         sensors[type][address].valid = true;

         if (address == 51)                     // Laufzeit in Sekunden
         {
            char duration[100] {};
            toElapsed(value, duration);
            text = duration;
         }

         if (!text.empty())
         {
            sensors[type][address].kind = "text";
            sensors[type][address].text = text;
         }

         // tell(eloAlways, "update samples set address = %s where address = %d and type = 'GROWATT'", key, address);
         // tell(eloAlways, "update valuefacts set address = %s where address = %d and type = 'GROWATT'", key, address);

         addValueFact(address, type, 1, title.c_str(), unit, title.c_str());

         // send update to WS
         {
            json_t* ojData = json_object();
            sensor2Json(ojData, type, address);

            if (sensors[type][address].kind == "status")
               json_object_set_new(ojData, "value", json_integer(sensors[type][address].state));
            else
               json_object_set_new(ojData, "value", json_real(sensors[type][address].value));

            json_object_set_new(ojData, "text", json_string(sensors[type][address].text.c_str()));

            if (sensors[type][address].image.length())
               json_object_set_new(ojData, "image", json_string(sensors[type][address].image.c_str()));

            char* tuple {};
            asprintf(&tuple, "%s:0x%02x", type, address);
            jsonSensorList[tuple] = ojData;
            free(tuple);

            pushDataUpdate("update", 0L);
         }

         mqttHaPublish(sensors[type][address]);
         mqttNodeRedPublishSensor(sensors[type][address]);
      }
   }

   json_decref(jData);
   process();

   return done;
}

//***************************************************************************
// Dispatch RTL 433 MHz
//  (rtl_433)
//     { "time":"2023-07-14 20:45:32", "model":"Nexus-T", "id":60, "channel":1, "battery_ok":1, "temperature_C":14.8}
//***************************************************************************

int Daemon::dispatchRtl433(const char* message)
{
   tell(eloInfo, "RTL433 <-'%s'", message);

   json_t* jData = jsonLoad(message);

   if (!jData)
   {
      tell(eloAlways, "Error: (RTL433) Can't parse json in '%s'", message);
      return fail;
   }

   std::string  kind = "value";
   const char* type = "RTL433";
   int address = getIntFromJson(jData, "id");
   const char* unit = "°C";
   // const char* timeStr = getStringFromJson(jData, "time");
   const char* title = getStringFromJson(jData, "model");
   double value = getDoubleFromJson(jData, "temperature_C");

   addValueFact(address, type, 1, title, unit, title);

   sensors[type][address].valid = true;
   sensors[type][address].kind = kind;
   sensors[type][address].value = value;
   sensors[type][address].last = time(0);

   // send update to WS
   {
      json_t* ojData = json_object();
      sensor2Json(ojData, type, address);

      if (kind == "status")
         json_object_set_new(ojData, "value", json_integer(sensors[type][address].state));
      else
         json_object_set_new(ojData, "value", json_real(sensors[type][address].value));

      json_object_set_new(ojData, "text", json_string(sensors[type][address].text.c_str()));

      if (sensors[type][address].image.length())
         json_object_set_new(ojData, "image", json_string(sensors[type][address].image.c_str()));

      char* tuple {};
      asprintf(&tuple, "%s:0x%02x", type, address);
      jsonSensorList[tuple] = ojData;
      free(tuple);

      pushDataUpdate("update", 0L);
   }

   mqttHaPublish(sensors[type][address]);
   mqttNodeRedPublishSensor(sensors[type][address]);
   json_decref(jData);

   return done;
}

//***************************************************************************
// Dispatch Other
//  (p4d2mqtt/p4/Heizung)
//     { "value": 77.0, "type": "P4VA", "address": 1, "unit": "°C", "title": "Abgas" }
//     { "state": 1, "address": 5, "type": "SMI260", "kind": "status", "title": "I Power On", "unit": "" }
//***************************************************************************

int Daemon::dispatchOther(const char* topic, const char* message)
{
   // tell(eloMqtt, "<- (%s) '%s'", topic, message);

   json_t* jData = jsonLoad(message);

   if (!jData)
   {
      tell(eloAlways, "Error: (%s) Can't parse json in '%s'", topic, message);
      return fail;
   }

   const char* type = getStringFromJson(jData, "type");
   int address = getIntFromJson(jData, "address");
   const char* unit = getStringFromJson(jData, "unit");
   const char* title = getStringFromJson(jData, "title");
   std::string kind = getStringFromJson(jData, "kind", "");
   const char* image = getStringFromJson(jData, "image", "");

   if (!type)
   {
      tell(eloAlways, "Error: Missing 'type' in '%s' (dispatchOther) [%s]", topic, message);
      return fail;
   }

   if (strcmp(type, "SC") != 0)
   {
      if (!title)
      {
         tell(eloAlways, "Error: Missing 'title' in '%s' (dispatchOther) [%s]", topic, message);
         return fail;
      }

      addValueFact(address, type, 1, title, unit, title);
      sensors[type][address].last = time(0);
      // tell(eloAlways, "Setting time for sensor '%s:%d' to %ld", type, address, sensors[type][address].last);
   }
   else  // SC - script sensor (send result async via MQTT)
   {
      sensors["SC"][address].working = false;      // reset 'working' state
      sensors[type][address].last = time(0) + 1;   // workaround due to async result of SC (if last is identical it's never stored)
   }

   sensors[type][address].valid = true;
   sensors[type][address].kind = getStringFromJson(jData, "kind", "value");
   sensors[type][address].state = getBoolFromJson(jData, "state");
   sensors[type][address].value = getDoubleFromJson(jData, "value");
   sensors[type][address].text = getStringFromJson(jData, "text", "");
   sensors[type][address].image = image;

   // send update to WS
   {
      json_t* ojData = json_object();
      sensor2Json(ojData, type, address);

      if (kind == "status")
         json_object_set_new(ojData, "value", json_integer(sensors[type][address].state));
      else
         json_object_set_new(ojData, "value", json_real(sensors[type][address].value));

      json_object_set_new(ojData, "text", json_string(sensors[type][address].text.c_str()));

      if (sensors[type][address].image.length())
         json_object_set_new(ojData, "image", json_string(sensors[type][address].image.c_str()));

      char* tuple {};
      asprintf(&tuple, "%s:0x%02x", type, address);
      jsonSensorList[tuple] = ojData;
      free(tuple);

      pushDataUpdate("update", 0L);
   }

   mqttHaPublish(sensors[type][address]);
   mqttNodeRedPublishSensor(sensors[type][address]);
   json_decref(jData);

   return done;
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
   else if (def)
   {
      value = strdup(def);
      setConfigItem(name, value);  // store the default
   }

   tableConfig->reset();

   return success;
}

int Daemon::setConfigItem(const char* name, const char* value)
{
   tell(eloDebug, "Debug: Storing config '%s' with value '%s'", name, value);
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
   char* txt {};

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
   char txt[16] {};

   snprintf(txt, sizeof(txt), "%ld", value);

   return setConfigItem(name, txt);
}

int Daemon::getConfigItem(const char* name, double& value, double def)
{
   char* txt {};

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

   tell(eloDebug, "Debug: getConfigItem '%s' now %.2f [%s]", name, value, txt);
   free(txt);

   return success;
}

int Daemon::setConfigItem(const char* name, double value)
{
   char txt[16+TB] {};
   snprintf(txt, sizeof(txt), "%.2f", value);
   return setConfigItem(name, txt);
}

int Daemon::getConfigItem(const char* name, bool& value, bool def)
{
   char* txt {};

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
   char txt[16] {};

   snprintf(txt, sizeof(txt), "%d", value ? 1 : 0);

   return setConfigItem(name, txt);
}

//***************************************************************************
// Get Config Time Range Item
//***************************************************************************

int Daemon::getConfigTimeRangeItem(const char* name, std::vector<Range>& ranges)
{
   char* tmp {};

   getConfigItem(name, tmp, "");
   ranges.clear();

   for (const char* r = strtok(tmp, ","); r; r = strtok(0, ","))
   {
      uint fromHH {0}, fromMM {0}, toHH {0}, toMM {0};

      if (sscanf(r, "%u:%u-%u:%u", &fromHH, &fromMM, &toHH, &toMM) == 4)
      {
         uint from = fromHH*100 + fromMM;
         uint to = toHH*100 + toMM;

         ranges.push_back({from, to});
         tell(eloDebug2, "Schedule range %d - %d for '%s'", from, to, name);
      }
      else
      {
         tell(eloAlways, "Error: Unexpected range '%s' for '%s'", r, name);
      }
   }

   free(tmp);

   return success;
}

//***************************************************************************
// Toggle Color
//***************************************************************************

int Daemon::toggleColor(uint addr, const char* type, int hue, int sat, int bri)
{
   cDbRow* fact = valueFactRowOf(type, addr);

   if (!fact)
      return fail;

   if (strcmp(type, "DZL") == 0)
      deconz.color(type, addr, hue, sat, bri);

   return success;
}

//***************************************************************************
// Digital IO Stuff
//***************************************************************************

int Daemon::toggleIo(uint addr, const char* type, int state, int bri, int transitiontime)
{
   cDbRow* fact = valueFactRowOf(type, addr);

   if (!fact)
      return fail;

   bool newState = state == na ? !sensors[type][addr].state : state;

   if (strcmp(type, "DO") == 0)
      gpioWrite(addr, newState);
   else if (strcmp(type, "SC") == 0)
      callScript(addr, "toggle");
   else if (strcmp(type, "DZL") == 0)
      deconz.toggle(type, addr, newState, bri, transitiontime);

   else if (strcmp(type, "HMB") == 0)
   {
      double value = sensors[type][addr].value;
      tell(eloDebug, "Debug: toggleIo - last direction = '%s', level was %d", sensors[type][addr].lastDir == dirOpen ? "open" : "close", (int)sensors[type][addr].value);

      if (bri == na && (sensors[type][addr].lastDir == dirOpen || sensors[type][addr].value == 100))
         value = 0;
      else if (bri == na && (sensors[type][addr].lastDir == dirClose || sensors[type][addr].value == 0))
         value = 100;
      else
         value = bri;

      tell(eloDebug, "Debug: toggleIo - sending level %d", (int)sensors[type][addr].value);

      sensors[type][addr].state = newState;
      mqttNodeRedPublishAction(sensors[type][addr], value);

      /* char* request {};
      asprintf(&request, "{ \"method\" : \"getDeviceDescription\", \"parameters\" : [\"%s\"] }", uuid);
      mqttWriter->write(TARGET "2mqtt/homematic/rpccall", request);
      tell(eloHomeMatic, "-> (home-matic) '%s' to '%s'", TARGET "2mqtt/homematic/rpccall", request);
      free(request);*/
   }

   return success;
}

int Daemon::toggleIoNext(uint pin)
{
   if (sensors["DO"][pin].state)
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
   json_object_set_new(ojData, "value", json_integer(sensors["DO"][pin].state));
   json_object_set_new(ojData, "last", json_integer(sensors["DO"][pin].last));
   // json_object_set_new(ojData, "next", json_integer(sensors["DO"][pin].next));
   json_object_set_new(ojData, "valid", json_boolean(sensors["DO"][pin].valid));

   // has to be moved to facts?
   json_object_set_new(ojData, "mode", json_string(sensors["DO"][pin].mode == omManual ? "manual" : "auto"));
   json_object_set_new(ojData, "options", json_integer(sensors["DO"][pin].opt));
}

int Daemon::toggleOutputMode(uint pin)
{
   // allow mode toggle only if more than one option is given

   if (sensors["DO"][pin].opt & ooAuto && sensors["DO"][pin].opt & ooUser)
   {
      OutputMode mode = sensors["DO"][pin].mode == omAuto ? omManual : omAuto;
      sensors["DO"][pin].mode = mode;

      storeStates();

      json_t* ojData = json_object();
      pin2Json(ojData, pin);

      char* tuple {};
      asprintf(&tuple, "%s:0x%02x", "DO", pin);
      jsonSensorList[tuple] = ojData;
      free(tuple);

      pushDataUpdate("update", 0L);
   }

   return success;
}

void Daemon::gpioWrite(uint pin, bool state, bool store)
{
   sensors["DO"][pin].state = state;
   sensors["DO"][pin].last = time(0);
   sensors["DO"][pin].valid = true;

   // if (!state)
   //    sensors["DO"][pin].next = 0;

   // invert the state on 'invertDO' - most relay board are active at 'false'

   digitalWrite(pin, sensors["DO"][pin].invertDO ? !state : state);

   if (store)
      storeStates();

   performJobs();

   // send update to WS
   {
      json_t* ojData = json_object();
      pin2Json(ojData, pin);

      char* tuple {};
      asprintf(&tuple, "%s:0x%02x", "DO", pin);
      jsonSensorList[tuple] = ojData;
      free(tuple);

      pushDataUpdate("update", 0L);
   }

   mqttHaPublish(sensors["DO"][pin]);
   mqttNodeRedPublishSensor(sensors["DO"][pin]);
}

bool Daemon::gpioRead(uint pin)
{
   sensors["DI"][pin].state = digitalRead(pin);
   return sensors["DI"][pin].state;
}

//***************************************************************************
// Publish Special Value
//***************************************************************************

void Daemon::publishSpecialValue(int addr)
{
   cDbRow* fact = valueFactRowOf("SP", addr);

   if (!fact)
      return ;

   json_t* ojData = json_object();

   sensor2Json(ojData, "SP", addr);

   if (sensors["SP"][addr].kind == "text")
      json_object_set_new(ojData, "text", json_string(sensors["SP"][addr].text.c_str()));
   else if (sensors["SP"][addr].kind == "value")
      json_object_set_new(ojData, "value", json_real(sensors["SP"][addr].value));

   if (sensors["SP"][addr].disabled)
      json_object_set_new(ojData, "disabled", json_boolean(true));

   char* tuple {};
   asprintf(&tuple, "SP:0x%02x", addr);
   jsonSensorList[tuple] = ojData;
   free(tuple);

   pushDataUpdate("update", 0L);
}

//***************************************************************************
// Store/Load States to DB
//  used to recover at restart
//***************************************************************************

int Daemon::storeStates()
{
   ulong value {0};
   ulong mode {0};

   if (!ioStatesLoaded)
      return done;

   for (const auto& output : sensors["DO"])
   {
      if (output.second.state)
         value += pow(2, output.first);

      if (output.second.mode == omManual)
         mode += pow(2, output.first);

      setConfigItem("ioStates", (long)value);
      setConfigItem("ioModes", (long)mode);

      tell(eloDebug2, "Info: iomode bit '%s/%d' %d [%s] stored", output.second.name.c_str(), output.first, output.second.mode, bin2string(mode));
      tell(eloDebug2, "Info: iostate bit '%s/%d' %d [%s] stored", output.second.name.c_str(), output.first, output.second.state, bin2string(value));
   }

   tell(eloDebug2, "Info: Stored iostates: [%s]", bin2string((ulong)value));
   tell(eloDebug2, "Info: Stored iomodes: [%s]", bin2string((ulong)mode));

   return done;
}

int Daemon::loadStates()
{
   long value {0};
   long mode {0};

   getConfigItem("ioStates", value, 0);
   getConfigItem("ioModes", mode, 0);

   // #TODO - use only for GPIO Pins (we need a other type for P4 'DO' !!

   if (!value)
   {
      tell(eloAlways, "Info: No iostates found!");
      ioStatesLoaded = true;
      return done;
   }

   tell(eloDebug2, "Info: Loaded iostates: [%s]", bin2string((ulong)value));
   tell(eloDebug2, "Info: Loaded iomodes: [%s]", bin2string((ulong)mode));

   for (const auto& output : sensors["DO"])
   {
      if (sensors["DO"][output.first].opt & ooUser)
      {
         gpioWrite(output.first, value & (long)pow(2, output.first), false);
         tell(eloDebug2, "Info: iomode '%s/%d' recovered to %d", output.second.name.c_str(), output.first, output.second.mode);
         tell(eloDebug2, "Info: iostate '%s/%d' recovered to %d", output.second.name.c_str(), output.first, output.second.state);
      }
   }

   if (mode)
   {
      for (const auto& output : sensors["DO"])
      {
         if (sensors["DO"][output.first].opt & ooAuto && sensors["DO"][output.first].opt & ooUser)
         {
            OutputMode m = mode & (long)pow(2, output.first) ? omManual : omAuto;
            sensors["DO"][output.first].mode = m;
         }
      }
   }

   ioStatesLoaded = true;

   return done;
}

//***************************************************************************
// Arduino Stuff ...
//***************************************************************************

int Daemon::dispatchArduinoMsg(const char* message)
{
   json_t* jObject = jsonLoad(message);

   if (!jObject)
   {
      tell(eloAlways, "Error: Can't parse json in '%s'", message);
      return fail;
   }

   const char* event = getStringFromJson(jObject, "event", "<null>");

   if (strcmp(event, "update") == 0)
   {
      json_t* jArray = getObjectFromJson(jObject, "analog");
      size_t index {0};
      json_t* jValue {};

      json_array_foreach(jArray, index, jValue)
      {
         const char* name = getStringFromJson(jValue, "name");
         std::string unit = getStringFromJson(jValue, "unit", "");
         double value = getDoubleFromJson(jValue, "value");
         time_t stamp = getIntFromJson(jValue, "time", 0);

         if (!stamp)
            stamp = time(0);

         if (stamp < time(0)-300)
         {
            tell(eloInfo, "Skipping old (%ld seconds) arduino value", time(0)-stamp);
            continue;
         }

         if (unit == "dig")
            unit = "%";

         if (unit == "")
            unit = atoi(name+1) <= 7 ? "%" : "mV";

         updateAnalogInput(name, value, stamp, unit.c_str());
      }

      process();
   }
   else if (strcmp(event, "init") == 0)
      initArduino();
   else
      tell(eloAlways, "Got unexpected event '%s' from arduino", event);

   json_decref(jObject);

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
      tell(eloAlways, "Error: Dumping json message failed");
      return fail;
   }

   if (mqttReader->isConnected())
   {
      std::string topic = std::string(arduinoTopic) + "/in";
      mqttReader->write(topic.c_str(), p);
      tell(eloDebug, "DEBUG: PushMessage to arduino [%s]", p);
   }

   free(p);

   return success;
}

void Daemon::updateAnalogInput(const char* id, double value, time_t stamp, const char* unit)
{
   uint input = atoi(id+1);

   // the Ardoino read the analog inputs with a resolution of 12 bits (3.3V => 4095)

   std::string name = "Analog Input (Arduino) " + std::to_string(input);
   addValueFact(input, "AI", 1, name.c_str(), unit);

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", (int)input);
   tableValueFacts->setValue("TYPE", "AI");

   if (!tableValueFacts->find() || !tableValueFacts->hasValue("STATE", "A"))
   {
      tableValueFacts->reset();
//      tell(eloAlways, "Debug: Input A%d (%s:0x%02x) skipped", input,
//           sensors["AI"][input].type.c_str(), sensors["AI"][input].address);

      return ;
   }

   double m = (aiSensors[input].calPointB - aiSensors[input].calPointA) / (aiSensors[input].calPointValueB - aiSensors[input].calPointValueA);
   double b = aiSensors[input].calPointB - m * aiSensors[input].calPointValueB;
   double dValue = m * value + b;

   if (aiSensors[input].round)
   {
      dValue = std::llround(dValue*aiSensors[input].round) / aiSensors[input].round;
      tell(eloDebug, "Rounded %.2f to %.2f", m * value + b, dValue);
   }

   if (dValue < aiSensors[input].calCutBelow)
      dValue = 0.0;

   sensors["AI"][input].value = dValue;
   sensors["AI"][input].last = stamp;
   sensors["AI"][input].valid = true;

   tell(eloDebug, "Debug: Input A%d (%s:0x%02x): %.3f%s [%.2f] from '%s'", input,
        sensors["AI"][input].type.c_str(), sensors["AI"][input].address,
        sensors["AI"][input].value, tableValueFacts->getStrValue("UNIT"),
        value, l2pTime(sensors["AI"][input].last).c_str());

   // ----------------------------------

   json_t* ojData = json_object();

   sensor2Json(ojData, "AI", input);

   json_object_set_new(ojData, "value", json_real(sensors["AI"][input].value));
   json_object_set_new(ojData, "plain", json_real(value)); // plain sensor value (without calibration conversion)

   char* tuple {};
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
   json_t* jArray = jsonLoad(message);

   if (!jArray)
   {
      tell(eloAlways, "Error: Can't parse json in '%s'", message);
      return fail;
   }

   size_t index {0};
   json_t* jValue {};

   json_array_foreach(jArray, index, jValue)
   {
      const char* name = getStringFromJson(jValue, "name");
      double value = getDoubleFromJson(jValue, "value");
      time_t stamp = getIntFromJson(jValue, "time");

      if (stamp < time(0)-300)
      {
         tell(eloInfo, "Skipping old (%ld seconds) w1 value", time(0)-stamp);
         continue;
      }

      updateW1(name, value, stamp);
   }

   json_decref(jArray);
   cleanupW1();
   process();

   return success;
}

void Daemon::updateW1(const char* id, double value, time_t stamp)
{
   uint address = toW1Id(id);

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", (int)address);
   tableValueFacts->setValue("TYPE", "W1");

   if (!tableValueFacts->find())
      addValueFact(address, "W1", 1, id, "°C");

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", (int)address);
   tableValueFacts->setValue("TYPE", "W1");

   if (!tableValueFacts->find() || !tableValueFacts->hasValue("STATE", "A"))
      return ;

   bool changed = sensors["W1"][address].value != value;

   sensors["W1"][address].value = value;
   sensors["W1"][address].valid = true;
   sensors["W1"][address].last = stamp;

   json_t* ojData = json_object();

   sensor2Json(ojData, "W1", address);
   json_object_set_new(ojData, "value", json_real(value));

   char* tuple {};
   asprintf(&tuple, "W1:0x%02x", address);
   jsonSensorList[tuple] = ojData;
   free(tuple);

   pushDataUpdate("update", 0L);

   if (changed)
   {
      mqttHaPublish(sensors["W1"][address]);
      mqttNodeRedPublishSensor(sensors["W1"][address]);
   }

   tableValueFacts->reset();
}

void Daemon::cleanupW1()
{
   uint detached {0};

   for (auto& it : sensors["W1"])
   {
      if (it.second.valid && it.second.last < time(0) - 5*tmeSecondsPerMinute)
      {
         tell(eloAlways, "Info: Missing w1 sensor '%x', removing it from list", it.first);
         it.second.valid = false;
         detached++;
      }
   }
}

bool Daemon::existW1(uint address)
{
   auto it = sensors["W1"].find(address);
   return it != sensors["W1"].end();
}

double Daemon::valueOfW1(uint address, time_t& last)
{
   last = 0;

   auto it = sensors["W1"].find(address);

   if (it == sensors["W1"].end())
      return 0;

   last = sensors["W1"][address].last;

   return sensors["W1"][address].value;
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

//***************************************************************************
// Execute Command
//***************************************************************************

void* Daemon::cmdThread(void* user)
{
   char buffer[128] {'\0'};
   std::string result {""};

   ThreadControl* threadCtl = (ThreadControl*)user;
   time_t timeoutAt = time(0) + threadCtl->timeout;
   threadCtl->active = true;

   FILE* pipe = popen(threadCtl->command.c_str(), "r");

   while (time(0) < timeoutAt && !threadCtl->cancel && fgets(buffer, sizeof(buffer), pipe))
      result += buffer;

   pclose(pipe);
   threadCtl->result = result;
   threadCtl->active = false;

   return nullptr;
}

int Daemon::executeCommandAsync(uint address, const char* cmd)
{
   commandThreads[address].command = cmd;

   if (pthread_create(&commandThreads[address].pThread, NULL, cmdThread, &commandThreads[address]))
   {
      tell(eloAlways, "Error: Failed to start command thread");
      return fail;
   }

   return success;
}
