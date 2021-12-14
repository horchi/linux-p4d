//***************************************************************************
// Automation Control
// File hass.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 25.04.2020  Jörg Wendel
//***************************************************************************

#include <jansson.h>

#include "lib/json.h"
#include "daemon.h"

//***************************************************************************
// Push Value to MQTT for Home Automation Systems
//***************************************************************************

int Daemon::mqttHaPublishSensor(IoType iot, const char* name, const char* title, const char* unit,
                                double value, const char* text, bool forceConfig)
{
   // check/prepare reader/writer connection

   if (isEmpty(mqttHassUrl))
      return done;

   mqttCheckConnection();

   // check if state topic already exists

   int status {success};
   MemoryStruct message;
   std::string tp;
   std::string sName = name;

   sName = strReplace("ß", "ss", sName);
   sName = strReplace("ü", "ue", sName);
   sName = strReplace("ö", "oe", sName);
   sName = strReplace("ä", "ae", sName);
   sName = strReplace(" ", "_", sName);

   // mqttDataTopic like "XXX2mqtt/<TYPE>/<NAME>/state"

   std::string sDataTopic = mqttDataTopic;
   sDataTopic = strReplace("<NAME>", sName, sDataTopic);
   sDataTopic = strReplace("<TYPE>", iot == iotLight ? "light" : "sensor", sDataTopic);

   if (mqttHaveConfigTopic && !isEmpty(title))
   {
      if (!mqttHassReader->isConnected())
         return fail;

      // Interface description:
      //   https://www.home-assistant.io/docs/mqtt/discovery/

      mqttHassReader->subscribe(sDataTopic.c_str());
      status = mqttHassReader->read(&message, 100);
      tp = mqttHassReader->getLastReadTopic();

      tell(eloDebug, "config topic '%s', state %d", tp.c_str(), status);

      if (status != success && status != Mqtt::wrnTimeout)
         return fail;

      if (forceConfig || status != success || !tp.length())
      {
         char* configTopic {nullptr};
         char* configJson {nullptr};

         if (!mqttHassWriter->isConnected())
            return fail;

         // topic don't exists -> create sensor

         if (strcmp(unit, "°") == 0)
            unit = "°C";
         else if (strcmp(unit, "Heizungsstatus") == 0 ||
                  strcmp(unit, "zst") == 0 ||
                  strcmp(unit, "T") == 0)
            unit = "";

         tell(1, "Info: Sensor '%s' not found at home assistants MQTT, "
              "sendig config message", sName.c_str());

         asprintf(&configTopic, "homeassistant/%s/%s/%s/config",
                  iot == iotLight ? "light" : "sensor", myTitle(), sName.c_str());

         if (iot == iotLight)
         {
            char* cmdTopic {nullptr};
            asprintf(&cmdTopic, TARGET "2mqtt/light/%s/set", sName.c_str());

            asprintf(&configJson, "{"
                     "\"state_topic\"         : \"" TARGET "2mqtt/light/%s/state\","
                     "\"command_topic\"       : \"%s\","
                     "\"name\"                : \"%s %s\","
                     "\"unique_id\"           : \"%s_" TARGET "2mqtt\","
                     "\"schema\"              : \"json\","
                     "\"brightness\"          : \"false\""
                     "}",
                     sName.c_str(), cmdTopic, myTitle(), title, sName.c_str());

            hassCmdTopicMap[cmdTopic] = name;
            free(cmdTopic);
         }
         else
         {
            asprintf(&configJson, "{"
                     "\"unit_of_measurement\" : \"%s\","
                     "\"value_template\"      : \"{{ value_json.value }}\","
                     "\"state_topic\"         : \"" TARGET "2mqtt/sensor/%s/state\","
                     "\"name\"                : \"%s %s\","
                     "\"unique_id\"           : \"%s_" TARGET "2mqtt\""
                     "}",
                     unit, sName.c_str(), title, myTitle(), sName.c_str());
         }

         mqttHassWriter->writeRetained(configTopic, configJson);

         free(configTopic);
         free(configJson);
      }

      mqttHassReader->unsubscribe(sDataTopic.c_str());
   }

   // publish actual value

   if (!mqttHassWriter->isConnected())
      return fail;

   json_t* oValue = json_object();

   if (iot == iotLight)
   {
      json_object_set_new(oValue, "state", json_string(value ? "ON" :"OFF"));
      json_object_set_new(oValue, "brightness", json_integer(255));
   }
   else if (!isEmpty(text))
      json_object_set_new(oValue, "value", json_string(text));
   else
      json_object_set_new(oValue, "value", json_real(value));

   char* j = json_dumps(oValue, JSON_PRESERVE_ORDER); // |JSON_REAL_PRECISION(5));
   mqttHassWriter->writeRetained(sDataTopic.c_str(), j);
   json_decref(oValue);

   return success;
}

//***************************************************************************
// Perform MQTT Requests
//   - check 'mqttHassCommandReader' for commands of a home automation
//   - check 'mqttReader' for data from the W1 service and the arduino, etc ...
//      -> w1mqtt (W1 service) is running localy and provide data of the W1 sensors
//      -> the arduino provide the values of his analog inputs
//***************************************************************************

int Daemon::performMqttRequests()
{
   static time_t lastMqttRead {0};
   static time_t lastMqttRecover {0};

   if (!lastMqttRead)
      lastMqttRead = time(0);

   mqttCheckConnection();

   MemoryStruct message;

   if (!isEmpty(mqttHassUrl) && mqttHassCommandReader->isConnected())
   {
      if (mqttHassCommandReader->read(&message, 10) == success)
      {
         json_error_t error;
         json_t* jData = json_loads(message.memory, 0, &error);

         if (!jData)
         {
            tell(0, "Error: Ignoring invalid json in '%s'", message.memory);
            tell(0, "Error decoding json: %s (%s, line %d column %d, position %d)",
                 error.text, error.source, error.line, error.column, error.position);
            return fail;
         }

         tell(eloAlways, "<- (%s) [%s]", mqttHassCommandReader->getLastReadTopic().c_str(), message.memory);
         dispatchMqttHaCommandRequest(jData, mqttHassCommandReader->getLastReadTopic().c_str());
      }
   }

   if (!isEmpty(mqttUrl) && mqttReader->isConnected())
   {
      // tell(0, "Try reading topic '%s'", mqttReader->getTopic());

      while (mqttReader->read(&message, 10) == success)
      {
         if (isEmpty(message.memory))
            continue;

         lastMqttRead = time(0);

         std::string tp = mqttReader->getLastReadTopic();
         tell(eloAlways, "<- (%s) [%s] retained %d", tp.c_str(), message.memory, mqttReader->isRetained());

         if (strcmp(tp.c_str(), TARGET "2mqtt/w1/ping") == 0)
            ;
         else if (strcmp(tp.c_str(), TARGET "2mqtt/w1") == 0)
            dispatchW1Msg(message.memory);
         else if (strcmp(tp.c_str(), TARGET "2mqtt/arduino/out") == 0)
            dispatchArduinoMsg(message.memory);
      }
   }

   if (!isEmpty(mqttNodeRedUrl) && mqttNodeRedReader->isConnected())
   {
      while (mqttNodeRedReader->read(&message, 10) == success)
      {
         if (isEmpty(message.memory))
            continue;

         tell(eloAlways, "<- (node-red) [%s]", message.memory);

         json_error_t error;
         json_t* jData = json_loads(message.memory, 0, &error);

         if (!jData)
         {
            tell(0, "Error: Ignoring invalid json in '%s'", message.memory);
            tell(0, "Error decoding json: %s (%s, line %d column %d, position %d)",
                 error.text, error.source, error.line, error.column, error.position);
            return fail;
         }

         dispatchNodeRedCommand(jData);
      }
   }

   // last successful W1 read at least in last 5 minutes?

   if (lastMqttRead < time(0)-300 && lastMqttRecover < time(0)-60)
   {
      lastMqttRecover = time(0);
      tell(eloAlways, "Info: No update from MQTT since '%s', disconnect from MQTT to force recover", l2pTime(lastMqttRead).c_str());
      mqttDisconnect();
   }

   return success;
}

//***************************************************************************
// Check MQTT Disconnect
//***************************************************************************

int Daemon::mqttDisconnect()
{
   if (mqttReader)               mqttReader->disconnect();
   if (mqttNodeRedReader)        mqttNodeRedReader->disconnect();
   if (mqttNodeRedWriter)        mqttNodeRedWriter->disconnect();
   if (mqttHassCommandReader)    mqttHassCommandReader->disconnect();
   if (mqttHassWriter)           mqttHassWriter->disconnect();
   if (mqttHassReader)           mqttHassReader->disconnect();

   delete mqttReader;            mqttReader = nullptr;
   delete mqttNodeRedReader;     mqttNodeRedReader = nullptr;
   delete mqttNodeRedWriter;     mqttNodeRedWriter = nullptr;
   delete mqttHassCommandReader; mqttHassCommandReader = nullptr;
   delete mqttHassWriter;        mqttHassWriter = nullptr;
   delete mqttHassReader;        mqttHassReader = nullptr;

   return done;
}

//***************************************************************************
// Check MQTT Connection and Connect
//***************************************************************************

int Daemon::mqttCheckConnection()
{
   bool renonnectNeeded {false};

   if (!mqttReader)
      mqttReader = new Mqtt();

   if (!mqttNodeRedReader)
      mqttNodeRedReader = new Mqtt();

   if (!mqttNodeRedWriter)
      mqttNodeRedWriter = new Mqtt();

   if (!mqttHassCommandReader)
      mqttHassCommandReader = new Mqtt();

   if (!mqttHassWriter)
      mqttHassWriter = new Mqtt();

   if (!mqttHassReader)
      mqttHassReader = new Mqtt();

   if (!isEmpty(mqttHassUrl) && (!mqttHassCommandReader->isConnected() || !mqttHassWriter->isConnected() || !mqttHassReader->isConnected()))
      renonnectNeeded = true;

   if (!isEmpty(mqttUrl) && !mqttReader->isConnected())
      renonnectNeeded = true;

   if (!isEmpty(mqttNodeRedUrl) && !mqttNodeRedReader->isConnected())
      renonnectNeeded = true;

   if (!isEmpty(mqttNodeRedUrl) && !mqttNodeRedWriter->isConnected())
      renonnectNeeded = true;

   if (!renonnectNeeded)
      return success;

   if (lastMqttConnectAt >= time(0) - 60)
      return fail;

   lastMqttConnectAt = time(0);

   if (!isEmpty(mqttHassUrl))
   {
      if (!mqttHassCommandReader->isConnected())
      {
         if (mqttHassCommandReader->connect(mqttHassUrl, mqttHassUser, mqttHassPassword) != success)
         {
            tell(0, "Error: MQTT: Connecting subscriber to '%s' failed", mqttHassUrl);
         }
         else
         {
            mqttHassCommandReader->subscribe(TARGET "2mqtt/light/+/set/#");
            tell(0, "MQTT: Connecting command subscriber to '%s' succeeded", mqttHassUrl);
         }
      }

      if (!mqttHassWriter->isConnected())
      {
         if (mqttHassWriter->connect(mqttHassUrl, mqttHassUser, mqttHassPassword) != success)
            tell(0, "Error: MQTT: Connecting publisher to '%s' failed", mqttHassUrl);
         else
            tell(0, "MQTT: Connecting publisher to '%s' succeeded", mqttHassUrl);
      }

      if (!mqttHassReader->isConnected())
      {
         if (mqttHassReader->connect(mqttHassUrl, mqttHassUser, mqttHassPassword) != success)
            tell(0, "Error: MQTT: Connecting subscriber to '%s' failed", mqttHassUrl);
         else
            tell(0, "MQTT: Connecting subscriber to '%s' succeeded", mqttHassUrl);

         // subscription is done in hassPush
      }
   }

   if (!isEmpty(mqttUrl))
   {
      if (!mqttReader->isConnected())
      {
         if (mqttReader->connect(mqttUrl) != success)
         {
            tell(0, "Error: MQTT: Connecting subscriber to '%s' failed", mqttUrl);
         }
         else
         {
            mqttReader->subscribe(TARGET "2mqtt/w1/#");
            tell(0, "MQTT: Connecting subscriber to '%s' - '%s' succeeded", mqttUrl, TARGET "2mqtt/w1/#");
            mqttReader->subscribe(TARGET "2mqtt/arduino/out");
            tell(0, "MQTT: Connecting subscriber to '%s' - '%s' succeeded", mqttUrl, TARGET "2mqtt/arduino/out");
         }
      }
   }

   if (!isEmpty(mqttNodeRedUrl))
   {
      if (!mqttNodeRedReader->isConnected())
      {
         if (mqttNodeRedReader->connect(mqttNodeRedUrl) != success)
         {
            tell(0, "Error: MQTT: Connecting node-red subscriber to '%s' failed", mqttNodeRedUrl);
         }
         else
         {
            mqttNodeRedReader->subscribe(TARGET "2mqtt/command/#");
            tell(0, "MQTT: Connecting node-red subscriber to '%s' - '%s' succeeded", mqttNodeRedUrl, TARGET "2mqtt/command/#");
         }
      }

      if (!mqttNodeRedWriter->isConnected())
      {
         if (mqttNodeRedWriter->connect(mqttNodeRedUrl) != success)
            tell(0, "Error: MQTT: Connecting node-red publisher to '%s' failed", mqttNodeRedUrl);
         else
            tell(0, "MQTT: Connecting node-red publisher to '%s' succeeded", mqttNodeRedUrl);
      }
   }

   return success;
}

//***************************************************************************
// MQTT Write (to Home Automation Systems)
//***************************************************************************

int Daemon::mqttHaWrite(json_t* obj, uint groupid)
{
   std::string sDataTopic = mqttDataTopic;

   // check/prepare connection

   if (mqttCheckConnection() != success)
      return fail;

   char* message = json_dumps(obj, JSON_REAL_PRECISION(4));
   tell(3, "Debug: JSON: [%s]", message);

   if (mqttInterfaceStyle == misGroupedTopic)
      sDataTopic = strReplace("<GROUP>", groups[groupid].name, sDataTopic);

   int status = mqttHassWriter->write(sDataTopic.c_str(), message);
   free(message);

   return status;
}

//***************************************************************************
// Publish to Node-Red (on change)
//***************************************************************************

int Daemon::mqttNodeRedPublishSensor(const char* type, uint address, IoType iot, const char* name,
                                     const char* title, const char* unit, double value, const char* text)
{
   if (!mqttNodeRedWriter || !mqttNodeRedWriter->isConnected())
       return done;

   json_t* oJson = json_object();

   char* key {nullptr};
   asprintf(&key, "%s:0x%02x", type, address);
   json_object_set_new(oJson, "id", json_string(key));
   json_object_set_new(oJson, "type", json_string(type));
   json_object_set_new(oJson, "address", json_integer(address));

   if (iot == iotLight)
      json_object_set_new(oJson, "state", json_string(value ? "on" : "off"));
   else
      json_object_set_new(oJson, "value", json_real(value));

   char* message = json_dumps(oJson, JSON_REAL_PRECISION(4));
   tell(2, "Debug: -> node-red (%s) [%s]", TARGET "2mqtt/changes", message);

   int status = mqttNodeRedWriter->write(TARGET "2mqtt/changes", message);
   free(message);

   return status;
}

//***************************************************************************
// Json Add Value
//***************************************************************************

int Daemon::jsonAddValue(json_t* obj, const char* name, const char* title, const char* unit,
                         double theValue, uint groupid, const char* text, bool forceConfig)
{
   std::string sName = name;
   bool newGroup {false};
   json_t* oGroup {nullptr};
   json_t* oSensor = json_object();

   if (mqttInterfaceStyle == misSingleTopic)
   {
      oGroup = json_object_get(obj, groups[groupid].name.c_str());

      if (!oGroup)
      {
         oGroup = json_object();
         newGroup = true;
      }
   }

   sName = strReplace("ß", "ss", sName);
   sName = strReplace("ü", "ue", sName);
   sName = strReplace("ö", "oe", sName);
   sName = strReplace("ä", "ae", sName);

   if (!isEmpty(text))
      json_object_set_new(oSensor, "value", json_string(text));
   else
      json_object_set_new(oSensor, "value", json_real(theValue));

   if (strcmp(unit, "°") == 0)
      unit = "°C";

   // create json

   if (forceConfig)
   {
      json_object_set_new(oSensor, "unit", json_string(unit));
      json_object_set_new(oSensor, "description", json_string(title));
   }

   if (oGroup)
   {
      json_object_set_new(oGroup, sName.c_str(), oSensor);

      if (newGroup)
         json_object_set_new(obj, groups[groupid].name.c_str(), oGroup);
   }
   else
   {
      json_object_set_new(obj, sName.c_str(), oSensor);
   }

   return success;
}
