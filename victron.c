//***************************************************************************
// Home Automation Control
// File victron.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 23.02.2024 - Jörg Wendel

//  https://www.victronenergy.com/upload/documents/VE.Direct-HEX-Protocol-Phoenix-Inverter.pdf
//***************************************************************************

#include <signal.h>

#include "lib/common.h"
#include "lib/json.h"
#include "lib/mqtt.h"

#include "lib/victron/vedef.h"
#include "lib/victron/vecom.h"
#include "lib/victron/veframehandler.h"

//***************************************************************************
// Class - Victron
//***************************************************************************

class Victron
{
   public:

      Victron(const char* aDevice, const char* aMqttUrl, const char* aMqttTopic, int aInterval, const char* aSensorType);
      ~Victron();

      int init();
      int loop();

      static void downF(int aSignal) { shutdown = true; }
      bool doShutDown() { return shutdown; }
      int show();

   protected:

      int update(bool force = false);
      int updateSettings(bool force = false);
      int readVeData();
      void createJson();
      void createJsonStructured();
      void publishJsonStructured(std::map<std::string,VePrettyData>::iterator itDef, const char* veValue);
      void publishJsonStructured(std::map<std::string,VePrettyData>::iterator itDef, uint8_t value);
      int lookupSpecialValue(std::string name, int value, std::string& result);

      int mqttConnection();
      int mqttDisconnect();
      int mqttPublish(json_t* jObject);
      int performMqttRequests();
      int dispatchMqttMessage(const char* message);
      int reconnect();

      VeCom serial;
      std::string device;
      VeDirectFrameHandler myve;

      const char* mqttUrl {};
      Mqtt* mqttWriter {};
      Mqtt* mqttReader {};
      std::string mqttTopicIn;
      std::string mqttTopicOut;
      int interval {60};
      std::string sensorType;
      bool structuredJson {true};
      Sem sem;            // to protect synchronous access to device

      static bool shutdown;
};

bool Victron::shutdown {false};

Victron::Victron(const char* aDevice, const char* aMqttUrl, const char* aMqttTopic, int aInterval, const char* aSensorType)
   : serial(B19200),
     device(aDevice),
     mqttUrl(aMqttUrl),
     mqttTopicIn(aMqttTopic),
     mqttTopicOut(aMqttTopic),
     interval(aInterval),
     sensorType(aSensorType),
     sem(0x4da00002)
{
   mqttTopicIn += "/in";
   mqttTopicOut += "/out";
}

Victron::~Victron()
{
   serial.close();
}

//***************************************************************************
// Init
//***************************************************************************

int Victron::init()
{
   if (serial.open(device.c_str()))
      return fail;

   // tell(eloAlways, "Opened serial device '%s'", device.c_str());

   if (!isEmpty(mqttUrl))
      mqttConnection();

   // read some data just to do it

   // SemLock lock(&sem);
   serial.sendPing();

   return success;
}

//***************************************************************************
// Perform MQTT Requests
//***************************************************************************

int Victron::performMqttRequests()
{
   if (isEmpty(mqttUrl))
      return done;

   if (!mqttReader->isConnected())
      return done;

   MemoryStruct message;

   // tell(eloMqtt, "Try reading topic '%s'", mqttReader->getTopic());

   while (mqttReader->read(&message, 10) == success)
   {
      if (isEmpty(message.memory))
         continue;

      dispatchMqttMessage(message.memory);
   }

   return done;
}

//***************************************************************************
// MQTT Connection
//***************************************************************************

int Victron::mqttConnection()
{
   if (!mqttWriter)
      mqttWriter = new Mqtt();

   if (!mqttReader)
      mqttReader = new Mqtt();

   if (!mqttWriter->isConnected())
   {
      if (mqttWriter->connect(mqttUrl) != success)
      {
         tell(eloAlways, "Error: MQTT: Connecting publisher to '%s' failed", mqttUrl);
         return fail;
      }

      tell(eloAlways, "MQTT: Connecting publisher to '%s' succeeded", mqttUrl);
      tell(eloAlways, "MQTT: Publish VICTRON data to topic '%s'", mqttTopicOut.c_str());
   }

   if (!mqttReader->isConnected())
   {
      if (mqttReader->connect(mqttUrl) != success)
      {
         tell(eloAlways, "Error: MQTT: Connecting subscriber to '%s' failed", mqttUrl);
         return fail;
      }

      if (mqttReader->subscribe(mqttTopicIn.c_str()) == success)
         tell(eloAlways, "MQTT: Topic '%s' at '%s' subscribed", mqttTopicIn.c_str(), mqttUrl);

      json_t* obj = json_object();
      json_object_set_new(obj, "type", json_string(sensorType.c_str()));
      json_object_set_new(obj, "action", json_string("init"));
      json_object_set_new(obj, "topic", json_string(mqttTopicIn.c_str()));

      mqttPublish(obj);
   }

   return success;
}

//***************************************************************************
// MQTT Disconnect
//***************************************************************************

int Victron::mqttDisconnect()
{
   if (mqttReader) mqttReader->disconnect();
   if (mqttWriter) mqttWriter->disconnect();

   delete mqttReader; mqttReader = nullptr;
   delete mqttWriter; mqttWriter = nullptr;

   tell(eloMqtt, "Disconnected from MQTT");

   return done;
}

//***************************************************************************
// Update
//***************************************************************************

int Victron::update(bool force)
{
   tell(eloInfo, "Updating ..");

   if (mqttConnection() != success)
      return fail;

   if (readVeData() != success)
   {
      tell(eloInfo, ".. failed");
      return fail;
   }

   if (structuredJson)
      createJsonStructured();
   else
      createJson();

   updateSettings(force);

   tell(eloInfo, ".. done");

   return done;
}

//***************************************************************************
// Update VE Settings (read settings registers)
//***************************************************************************

int Victron::updateSettings(bool force)
{
   static time_t nextAt {0};

   if (!force && time(0) < nextAt)
      return done;

   tell(eloInfo, ".. updating settings");

   nextAt = time(0) + 2*tmeSecondsPerMinute;

   double value {0};
   std::string text;

   // SemLock lock(&sem);

   serial.readRegister(VeService::regVoltageRangeMin, value);
   text += VeService::toPretty(VeService::regVoltageRangeMin, value) + "\n";
   serial.readRegister(VeService::regVoltageRangeMax, value);
   text += VeService::toPretty(VeService::regVoltageRangeMax, value) + "\n";
   serial.readRegister(VeService::regShutdownLowVoltageSet, value);
   text += VeService::toPretty(VeService::regShutdownLowVoltageSet, value) + "\n";
   serial.readRegister(VeService::regAlarmLowVoltageSet, value);
   text += VeService::toPretty(VeService::regAlarmLowVoltageSet, value) + "\n";
   serial.readRegister(VeService::regAlarmLowVoltageClear, value);
   text += VeService::toPretty(VeService::regAlarmLowVoltageClear, value) + "\n";
   serial.readRegister(VeService::regInvProtUbatDynCutoffVoltage, value);
   text += VeService::toPretty(VeService::regInvProtUbatDynCutoffVoltage, value) + "\n";

   // tell(eloAlways, "%s", text.c_str());

   const auto itDef = vePrettyData.find("_INFO");
   publishJsonStructured(itDef, text.c_str());

   // device mode setting as choice

   serial.readRegister(VeService::regDeviceMode, value);
   const auto itDef1 = vePrettyData.find("_MODE");
   publishJsonStructured(itDef1, VeService::deviceModes[(VeService::DeviceMode)value].c_str());

   return done;
}

//***************************************************************************
// Dispatch MQTT Message
//***************************************************************************

int Victron::dispatchMqttMessage(const char* message)
{
   json_t* jData {jsonLoad(message)};

   if (!jData)
      return fail;

   tell(eloAlways, "<- [%s]", message);

   const char* type = getStringFromJson(jData, "type", "");
   uint address = getIntFromJson(jData, "address");
   std::string action = getStringFromJson(jData, "action", "");

   if (strncmp(type, "VIC", 3) != 0)
      return done;

   // SemLock lock(&sem);

   if (action == "init")
   {
      update(true /*force*/);
      return done;
   }

   if (address == vdraMode)
   {
      const char* mode = getStringFromJson(jData, "value");
      VeService::DeviceMode veMode = VeService::toMode(mode);
      tell(eloAlways, "Info: Set regDeviceMode to '%s' (%d)", mode, veMode);

      serial.writeRegister(VeService::regDeviceMode, (uint8_t&)veMode);
      update(true);
   }

   // double value = getDoubleFromJson(jData, "value");

   return done;
}

//***************************************************************************
// Loop
//***************************************************************************

int Victron::loop()
{
   while (!doShutDown())
   {
      time_t nextAt = time(0) + interval;

      // performMqttRequests();
      update();

      while (!doShutDown() && time(0) < nextAt)
      {
         performMqttRequests();
         readVeData();  // continue reading unused frames ...
      }
   }

   return done;
}

//***************************************************************************
// Reconnect
//***************************************************************************

int Victron::reconnect()
{
   serial.close();
   int res = serial.open(device.c_str());
   serial.sendPing();
   return res;
}

//***************************************************************************
// Read VE Data
//***************************************************************************

int Victron::readVeData()
{
   uint8_t b {0};

   while (!doShutDown() && serial.readByte(b, 4000) == success)
   {
      if (myve.rxData(b) == done)
         return success;
   }

   reconnect();

   return fail;
}

//***************************************************************************
// Show
//***************************************************************************

int Victron::show()
{
   // SemLock lock(&sem);

   if (readVeData() != success)
   {
      tell(eloInfo, ".. failed");
      return fail;
   }

   for (int i = 0; i < myve.veEnd; i++)
   {
      if (isEmpty(myve.veName[i]) || isEmpty(myve.veValue[i]))
         continue;

      const auto itDef = vePrettyData.find(myve.veName[i]);

      if (itDef == vePrettyData.end())
      {
         tell(eloAlways, "Info: No definition for '%s' found", myve.veName[i]);
         continue;
      }

      int intValue = strtoll(myve.veValue[i], nullptr, 0);
      std::string strValue;

      if (lookupSpecialValue(itDef->first, intValue, strValue) == success)
         tell(eloAlways, "%s: %s", itDef->second.title.c_str(), strValue.c_str());
      else if (itDef->second.format == vtReal)
         tell(eloAlways, "%s: %0.2f", itDef->second.title.c_str(), (int)((atof(myve.veValue[i]) / itDef->second.factor) * 100 + 0.5) / 100.0);
      else if (itDef->second.format == vtInt)
         tell(eloAlways, "%s: %d", itDef->second.title.c_str(), intValue);
      else
         tell(eloAlways, "%s: %s", itDef->second.title.c_str(), myve.veValue[i]);
   }

   return success;
}

//***************************************************************************
// Create Json
//***************************************************************************

void Victron::createJson()
{
   json_t* oJson {json_object()};

   for (int i = 0; i < myve.veEnd; i++)
   {
      if (isEmpty(myve.veName[i]) || isEmpty(myve.veValue[i]))
         continue;

      const auto itDef = vePrettyData.find(myve.veName[i]);

      if (itDef == vePrettyData.end())
      {
         tell(eloAlways, "Info: No definition for '%s' found", myve.veName[i]);
         continue;
      }

      int intValue = strtoll(myve.veValue[i], nullptr, 0);
      std::string strValue;

      if (lookupSpecialValue(itDef->first, intValue, strValue) == success)
         json_object_set_new(oJson, itDef->second.title.c_str(), json_string(strValue.c_str()));
      else if (itDef->second.format == vtReal)
         json_object_set_new(oJson, itDef->second.title.c_str(), json_real((int)((atof(myve.veValue[i]) / itDef->second.factor) * 100 + 0.5) / 100.0));
      else if (itDef->second.format == vtInt)
         json_object_set_new(oJson, itDef->second.title.c_str(), json_integer(intValue));
      else
         json_object_set_new(oJson, itDef->second.title.c_str(), json_string(myve.veValue[i]));
   }

   mqttPublish(oJson);
}

//***************************************************************************
// Create Json Structured
//***************************************************************************

void Victron::createJsonStructured()
{
   for (int i = 0; i < myve.veEnd; i++)
   {
      if (isEmpty(myve.veName[i]) || isEmpty(myve.veValue[i]))
         continue;

      const auto itDef = vePrettyData.find(myve.veName[i]);

      if (itDef == vePrettyData.end())
      {
         tell(eloAlways, "Info: No definition for '%s' found", myve.veName[i]);
         continue;
      }

      if (!itDef->second.incStructured)
         continue;

      publishJsonStructured(itDef, myve.veValue[i]);
   }
}

void Victron::publishJsonStructured(std::map<std::string,VePrettyData>::iterator itDef, uint8_t value)
{
   return publishJsonStructured(itDef, horchi::to_string(value, 0).c_str());
}

void Victron::publishJsonStructured(std::map<std::string,VePrettyData>::iterator itDef, const char* veValue)
{
   json_t* oJson {json_object()};

   json_object_set_new(oJson, "type", json_string(sensorType.c_str()));
   json_object_set_new(oJson, "address", json_integer(itDef->second.address));
   json_object_set_new(oJson, "title", json_string(itDef->second.title.c_str()));
   json_object_set_new(oJson, "unit", json_string(itDef->second.unit.c_str()));

   if (itDef->second.format == vtReal)
      json_object_set_new(oJson, "kind", json_string("value"));
   else if (itDef->second.format == vtInt)
      json_object_set_new(oJson, "kind", json_string("value"));
   else if (itDef->second.format == vtBool)
      json_object_set_new(oJson, "kind", json_string("status"));
   else
      json_object_set_new(oJson, "kind", json_string("text"));

   int intValue = strtoll(veValue, nullptr, 0);
   std::string strValue;

   if (lookupSpecialValue(itDef->first, intValue, strValue) == success)
      json_object_set_new(oJson, "text", json_string(strValue.c_str()));
   else if (itDef->second.format == vtReal)
      json_object_set_new(oJson, "value", json_real((int)((atof(veValue) / itDef->second.factor) * 100 + 0.5) / 100.0));
   else if (itDef->second.format == vtInt)
      json_object_set_new(oJson, "value", json_integer(intValue));
   else if (itDef->second.format == vtBool)
      json_object_set_new(oJson, "state", json_boolean(intValue));
   else
      json_object_set_new(oJson, "text", json_string(veValue));

   std::string choices;

   for (const auto& m : VeService::deviceModes)
   {
      if (m.first != VeService::dmUnknown && m.first != VeService::dmHibernate && m.first != VeService::dmDeviceOn)
         choices += m.second + ",";
   }

   choices.pop_back();

   if (itDef->first == "_MODE")
      json_object_set_new(oJson, "choices", json_string(choices.c_str()));

   mqttPublish(oJson);
}

//***************************************************************************
// MQTT Publish
//***************************************************************************

int Victron::mqttPublish(json_t* jObject)
{
   char* message = json_dumps(jObject, JSON_REAL_PRECISION(8));
   mqttWriter->write(mqttTopicOut.c_str(), message);
   free(message);
   json_decref(jObject);

   return success;
}

//***************************************************************************
// Usage
//***************************************************************************

int Victron::lookupSpecialValue(std::string name, int value, std::string& result)
{
   if (name == "PID")            // Device
   {
      if (veDirectDeviceList.find(value) == veDirectDeviceList.end())
      {
         tell(eloAlways, "Info: Device pid '0x%02X' not found in device list", value);
         return fail;
      }

      result = veDirectDeviceList[value];
   }
   else if (name == "AR" || name == "WARN")   // Alarm Code / Warn Reason
   {
      if (veDirectDeviceCodeAR.find(value) == veDirectDeviceCodeAR.end())
         return fail;
      result = veDirectDeviceCodeAR[value];
   }
   else if (name == "OR")        // Off Reason / Warn Reason
   {
      // TODO: More than one bit can set!

      if (veDirectDeviceCodeOR.find(value) == veDirectDeviceCodeOR.end())
         return fail;
      result = veDirectDeviceCodeOR[value];
   }
   else if (name == "CS")        // Operation State
   {
      if (veDirectDeviceCodeCS.find(value) == veDirectDeviceCodeCS.end())
         return fail;
      result = veDirectDeviceCodeCS[value];
   }
   else if (name == "ERR")       // Error
   {
      if (veDirectDeviceCodeERR.find(value) == veDirectDeviceCodeERR.end())
         return fail;
      result = veDirectDeviceCodeERR[value];
   }
   else if (name == "MPPT")      // Tracker Operation Mode
   {
      if (veDirectDeviceCodeMPPT.find(value) == veDirectDeviceCodeMPPT.end())
         return fail;
      result = veDirectDeviceCodeMPPT[value];
   }
   else if (name == "MODE")      // Device Mode
   {
      if (veDirectDeviceCodeMODE.find(value) == veDirectDeviceCodeMODE.end())
         return fail;
      result = veDirectDeviceCodeMODE[value];
   }
   else
   {
      return fail;
   }

   return success;
}

//***************************************************************************
// Show
//***************************************************************************

int show(const char* device)
{
   Victron* job = new Victron(device, "", "", 5, "");

   if (job->init() != success)
   {
      printf("Initialization failed, see syslog for details\n");
      delete job;
      return fail;
   }

   job->show();

   delete job;

   return done;
}

//***************************************************************************
// Usage
//***************************************************************************

void showUsage(const char* bin)
{
   printf("Usage: %s <command> [-l <log-level>] [-d <device>]\n", bin);
   printf("\n");
   printf("  options:\n");
   printf("     -d <device>     serial device file (defaults to /dev/ttyVictron)\n");
   printf("     -l <eloquence>  set eloquence\n");
   printf("     -t              log to terminal\n");
   printf("     -i <interval>   interval [s] (default 10)\n");
   printf("     -u <url>        MQTT url (default tcp://localhost:1883)\n");
   printf("     -T <topic>      MQTT topic (default " TARGET "2mqtt/victron)\n");
   printf("                       <topic>/out - produce to\n");
   printf("     -n              Don't fork in background\n");
   printf("     -I <ident>      Identifier (type) of the homectld sensor (default VIC)\n");
}

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char** argv)
{
   bool nofork {false};
   int _stdout {na};

   const char* mqttTopic {TARGET "2mqtt/victron"};
   const char* mqttUrl {"tcp://localhost:1883"};
   const char* device {"/dev/ttyVictron"};
   int interval {10};
   const char* sensorType {"VIC"};
   bool showMode {false};

   // usage ..

   if (argc > 1 && (argv[1][0] == '?' || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)))
   {
      showUsage(argv[0]);
      return 0;
   }

   for (int i = 0; argv[i]; i++)
   {
      if (argv[i][0] != '-' || strlen(argv[i]) != 2)
         continue;

      switch (argv[i][1])
      {
         case 'l': if (argv[i+1]) eloquence = (Eloquence)atoi(argv[++i]); break;
         case 'u': mqttUrl = argv[i+1];                       break;
         case 'i': if (argv[i+1]) interval = atoi(argv[++i]); break;
         case 'T': if (argv[i+1]) mqttTopic = argv[i+1];      break;
         case 't': _stdout = yes;                             break;
         case 'd': if (argv[i+1]) device = argv[++i];         break;
         case 'n': nofork = true;                             break;
         case 'I': if (argv[i+1]) sensorType = argv[++i];     break;
         case 's': showMode = true;                           break;
      }
   }

   if (showMode)
      return show(device);

   // do work ...

   if (_stdout != na)
      logstdout = _stdout;
   else
      logstdout = false;

   // fork daemon

   if (!nofork)
   {
      int pid {0};

      if ((pid = fork()) < 0)
      {
         printf("Can't fork daemon, %s\n", strerror(errno));
         return 1;
      }

      if (pid != 0)
         return 0;
   }

   // create/init AFTER fork !!!

   Victron* job = new Victron(device, mqttUrl, mqttTopic, interval, sensorType);

   if (job->init() != success)
   {
      printf("Initialization failed, see syslog for details\n");
      delete job;
      return 1;
   }

   // register SIGINT

   ::signal(SIGINT, Victron::downF);
   ::signal(SIGTERM, Victron::downF);

   // do work ...

   job->loop();

   // shutdown

   tell(eloAlways, "shutdown");
   delete job;

   return 0;
}
