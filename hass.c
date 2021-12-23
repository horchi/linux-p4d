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

int Daemon::mqttHaPublishSensor(SensorData& sensor, bool forceConfig)
{
   // check/prepare reader/writer connection

   if (isEmpty(mqttHassUrl))
      return done;

   mqttCheckConnection();

   // check if state topic already exists

   int status {success};
   MemoryStruct message;
   std::string tp;
   std::string sName = sensor.name;

   IoType iot = sensor.type == "DO" || sensor.type == "SC" ? iotLight : iotSensor;

   sName = strReplace("ß", "ss", sName);
   sName = strReplace("ü", "ue", sName);
   sName = strReplace("ö", "oe", sName);
   sName = strReplace("ä", "ae", sName);
   sName = strReplace(" ", "_", sName);

   // mqttDataTopic like "XXX2mqtt/<TYPE>/<NAME>/state"

   std::string sDataTopic = mqttDataTopic;
   sDataTopic = strReplace("<NAME>", sName, sDataTopic);
   sDataTopic = strReplace("<TYPE>", iot == iotLight ? "light" : "sensor", sDataTopic);

   if (mqttHaveConfigTopic && !sensor.title.length())
   {
      if (!mqttHassReader->isConnected())
         return fail;

      // Interface description:
      //   https://www.home-assistant.io/docs/mqtt/discovery/

      mqttHassReader->subscribe(sDataTopic.c_str());
      status = mqttHassReader->read(&message, 100);
      tp = mqttHassReader->getLastReadTopic();

      tell(eloHaMqtt, "Config topic '%s', state %d", tp.c_str(), status);

      if (status != success && status != Mqtt::wrnTimeout)
         return fail;

      if (forceConfig || status != success || !tp.length())
      {
         char* configTopic {nullptr};
         char* configJson {nullptr};

         if (!mqttHassWriter->isConnected())
            return fail;

         // topic don't exists -> create sensor

         if (strcmp(sensor.unit.c_str(), "°") == 0)
            sensor.unit = "°C";
         else if (sensor.unit == "Heizungsstatus" ||
                  sensor.unit == "zst" ||
                  sensor.unit == "txt")
            sensor.unit = "";

         tell(eloHaMqtt, "Info: Sensor '%s' not found at home assistants MQTT, "
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
                     sName.c_str(), cmdTopic, myTitle(), sensor.title.c_str(), sName.c_str());

            hassCmdTopicMap[cmdTopic] = sensor.name;
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
                     sensor.unit.c_str(), sName.c_str(), sensor.title.c_str(), myTitle(), sName.c_str());
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

   if (sensor.kind == "status")
   {
      json_object_set_new(oValue, "state", json_string(sensor.value ? "ON" :"OFF"));
      json_object_set_new(oValue, "brightness", json_integer(255));
   }
   else if (sensor.text.length())
      json_object_set_new(oValue, "value", json_string(sensor.text.c_str()));
   else if (sensor.kind == "value")
      json_object_set_new(oValue, "value", json_real(sensor.value));

   char* j = json_dumps(oValue, JSON_PRESERVE_ORDER); // |JSON_REAL_PRECISION(5));
   mqttHassWriter->writeRetained(sDataTopic.c_str(), j);
   free(j);
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
            tell(eloAlways, "Error: Ignoring invalid json in '%s'", message.memory);
            tell(eloAlways, "Error decoding json: %s (%s, line %d column %d, position %d)",
                 error.text, error.source, error.line, error.column, error.position);
            return fail;
         }

         tell(eloHaMqtt, "<- (%s) [%s]", mqttHassCommandReader->getLastReadTopic().c_str(), message.memory);
         dispatchMqttHaCommandRequest(jData, mqttHassCommandReader->getLastReadTopic().c_str());
      }
   }

   if (!isEmpty(mqttUrl) && mqttReader->isConnected())
   {
      // tell(eloMqttHome, "Try reading topic '%s'", mqttReader->getTopic());

      while (mqttReader->read(&message, 10) == success)
      {
         if (isEmpty(message.memory))
            continue;

         lastMqttRead = time(0);

         std::string tp = mqttReader->getLastReadTopic();
         tell(eloMqttHome, "<- (%s) [%s] retained %d", tp.c_str(), message.memory, mqttReader->isRetained());

         if (strstr(tp.c_str(), "2mqtt/ping"))
            ;
         else if (strstr(tp.c_str(), "2mqtt/w1"))
            dispatchW1Msg(message.memory);
         else if (strstr(tp.c_str(), "2mqtt/arduino/out"))
            dispatchArduinoMsg(message.memory);
         else if (strstr(tp.c_str(), "2mqtt/homematic/rpcresult"))
            dispatchHomematicRpcResult(message.memory);
         else if (strstr(tp.c_str(), "2mqtt/homematic/events"))
            dispatchHomematicEvents(message.memory);
      }
   }

   if (!isEmpty(mqttNodeRedUrl) && mqttNodeRedReader->isConnected())
   {
      while (mqttNodeRedReader->read(&message, 10) == success)
      {
         if (isEmpty(message.memory))
            continue;

         tell(eloNodeRed, "<- (node-red) [%s]", message.memory);

         json_error_t error;
         json_t* jData = json_loads(message.memory, 0, &error);

         if (!jData)
         {
            tell(eloAlways, "Error: Ignoring invalid json in '%s'", message.memory);
            tell(eloAlways, "Error decoding json: %s (%s, line %d column %d, position %d)",
                 error.text, error.source, error.line, error.column, error.position);
            return fail;
         }

         dispatchNodeRedCommands(mqttNodeRedReader->getLastReadTopic().c_str(), jData);
      }
   }

   // last successful W1 read at least in last 5 minutes?

   if (lastMqttRead < time(0)-300 && lastMqttRecover < time(0)-60)
   {
      lastMqttRecover = time(0);
      tell(eloMqttHome, "Info: No update from MQTT since '%s', disconnect from MQTT to force recover", l2pTime(lastMqttRead).c_str());
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
   delete mqttWriter;            mqttWriter = nullptr;
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

   if (!mqttWriter)
      mqttWriter = new Mqtt();

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

   if (!isEmpty(mqttUrl) && !mqttWriter->isConnected())
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
            tell(eloAlways, "Error: MQTT: Connecting subscriber to '%s' failed", mqttHassUrl);
         }
         else
         {
            mqttHassCommandReader->subscribe(TARGET "2mqtt/light/+/set/#");
            tell(eloHaMqtt, "MQTT: Connecting command subscriber to '%s' succeeded", mqttHassUrl);
         }
      }

      if (!mqttHassWriter->isConnected())
      {
         if (mqttHassWriter->connect(mqttHassUrl, mqttHassUser, mqttHassPassword) != success)
            tell(eloAlways, "Error: MQTT: Connecting publisher to '%s' failed", mqttHassUrl);
         else
            tell(eloHaMqtt, "MQTT: Connecting publisher to '%s' succeeded", mqttHassUrl);
      }

      if (!mqttHassReader->isConnected())
      {
         if (mqttHassReader->connect(mqttHassUrl, mqttHassUser, mqttHassPassword) != success)
            tell(eloAlways, "Error: MQTT: Connecting subscriber to '%s' failed", mqttHassUrl);
         else
            tell(eloHaMqtt, "MQTT: Connecting subscriber to '%s' succeeded", mqttHassUrl);

         // subscription is done in hassPush
      }
   }

   if (!isEmpty(mqttUrl))
   {
      if (!mqttWriter->isConnected())
      {
         if (mqttWriter->connect(mqttUrl) != success)
            tell(eloAlways, "Error: MQTT: Connecting publisher to '%s' failed", mqttUrl);
         else
            tell(eloHaMqtt, "MQTT: Connecting publisher to '%s' succeeded", mqttUrl);
      }

      if (!mqttReader->isConnected())
      {
         if (mqttReader->connect(mqttUrl) != success)
         {
            tell(eloAlways, "Error: MQTT: Connecting subscriber to '%s' failed", mqttUrl);
         }
         else
         {
            for (const auto& t : mqttSensorTopics)
            {
               mqttReader->subscribe(t.c_str());
               tell(eloMqttHome, "MQTT: Connecting subscriber to '%s' - '%s' succeeded", mqttUrl, t.c_str());
            }
         }
      }
   }

   if (!isEmpty(mqttNodeRedUrl))
   {
      if (!mqttNodeRedReader->isConnected())
      {
         if (mqttNodeRedReader->connect(mqttNodeRedUrl) != success)
         {
            tell(eloAlways, "Error: MQTT: Connecting node-red subscriber to '%s' failed", mqttNodeRedUrl);
         }
         else
         {
            mqttNodeRedReader->subscribe(TARGET "2mqtt/command/#");
            tell(eloNodeRed, "MQTT: Connecting node-red subscriber to '%s' - '%s' succeeded", mqttNodeRedUrl, TARGET "2mqtt/command/#");
            mqttNodeRedReader->subscribe(TARGET "2mqtt/nodered/#");
            tell(eloNodeRed, "MQTT: Connecting node-red subscriber to '%s' - '%s' succeeded", mqttNodeRedUrl, TARGET "2mqtt/nodered/#");
         }
      }

      if (!mqttNodeRedWriter->isConnected())
      {
         if (mqttNodeRedWriter->connect(mqttNodeRedUrl) != success)
            tell(eloAlways, "Error: MQTT: Connecting node-red publisher to '%s' failed", mqttNodeRedUrl);
         else
            tell(eloNodeRed, "MQTT: Connecting node-red publisher to '%s' succeeded", mqttNodeRedUrl);
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
   tell(eloHaMqtt, "Debug: JSON: [%s]", message);

   if (mqttInterfaceStyle == misGroupedTopic)
      sDataTopic = strReplace("<GROUP>", groups[groupid].name, sDataTopic);

   int status = mqttHassWriter->write(sDataTopic.c_str(), message);
   free(message);

   return status;
}

//***************************************************************************
// Publish to Node-Red (on change)
//***************************************************************************

int Daemon::mqttNodeRedPublishSensor(SensorData& sensor)
{
   return mqttNodeRedPublishAction(sensor, sensor.value, true);
}

int Daemon::mqttNodeRedPublishAction(SensorData& sensor, double value, bool publishOnly)
{
   if (!mqttNodeRedWriter || !mqttNodeRedWriter->isConnected())
       return done;

   json_t* oJson = json_object();
   char* key {nullptr};

   asprintf(&key, "%s:0x%02x", sensor.type.c_str(), sensor.address);
   json_object_set_new(oJson, "id", json_string(key));
   json_object_set_new(oJson, "type", json_string(sensor.type.c_str()));
   json_object_set_new(oJson, "name", json_string(sensor.title.c_str()));
   json_object_set_new(oJson, "unit", json_string(sensor.unit.c_str()));
   json_object_set_new(oJson, "state", json_string(sensor.state ? "on" : "off"));
   json_object_set_new(oJson, "value", json_real(value));

   if (!publishOnly)
   {
      // an action is to be triggered

      if (sensor.type == "HMB")
      {
         json_object_set_new(oJson, "action", json_string(sensor.working ? "STOP" : "LEVEL"));
         json_object_set_new(oJson, "uuid", json_string(homeMaticUuids[sensor.address].c_str()));
      }
      else
         json_object_set_new(oJson, "action", json_string("ACTION"));
   }
   else
      json_object_set_new(oJson, "action", json_string("CHANGE"));

   char* message = json_dumps(oJson, JSON_REAL_PRECISION(4));
   tell(eloNodeRed, "-> (node-red) (%s) [%s]", TARGET "2mqtt/changes", message);

   int status = mqttNodeRedWriter->write(TARGET "2mqtt/changes", message);
   free(message);

   return status;
}

//***************************************************************************
// Json Add Value
//***************************************************************************

int Daemon::jsonAddValue(json_t* obj, SensorData& sensor, bool forceConfig)
{
   std::string sName = sensor.name;
   bool newGroup {false};
   json_t* oGroup {nullptr};
   json_t* oSensor = json_object();

   if (mqttInterfaceStyle == misSingleTopic)
   {
      oGroup = json_object_get(obj, groups[sensor.group].name.c_str());

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

   if (sensor.kind == "status")
   {
      json_object_set_new(oSensor, "state", json_string(sensor.value ? "ON" :"OFF"));
      json_object_set_new(oSensor, "brightness", json_integer(255));
   }
   else if (sensor.text.length())
      json_object_set_new(oSensor, "value", json_string(sensor.text.c_str()));
   else if (sensor.kind == "value")
      json_object_set_new(oSensor, "value", json_real(sensor.value));

   // if (strcmp(unit, "°") == 0)
   //    unit = "°C";

   // create json

   if (forceConfig)
   {
      json_object_set_new(oSensor, "unit", json_string(sensor.unit.c_str()));
      json_object_set_new(oSensor, "description", json_string(sensor.title.c_str()));
   }

   if (oGroup)
   {
      json_object_set_new(oGroup, sName.c_str(), oSensor);

      if (newGroup)
         json_object_set_new(obj, groups[sensor.group].name.c_str(), oGroup);
   }
   else
   {
      json_object_set_new(obj, sName.c_str(), oSensor);
   }

   return success;
}
