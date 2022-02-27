//***************************************************************************
// Automation Control
// File hass.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 25.04.2020  Jörg Wendel
//***************************************************************************

#include <jansson.h>
#include <algorithm>

#include "lib/json.h"
#include "daemon.h"

//***************************************************************************
// Push Value to MQTT for Home Automation Systems
//***************************************************************************

int Daemon::mqttHaPublish(SensorData& sensor, bool forceConfig)
{
   if (isEmpty(mqttUrl))
      return done;

   if (mqttCheckConnection() != success)
      return fail;

   if (mqttInterfaceStyle == misGroupedTopic)
   {
      if (!groups[sensor.group].oHaJson)
         groups[sensor.group].oHaJson = json_object();
   }

   if (mqttInterfaceStyle == misSingleTopic || mqttInterfaceStyle == misGroupedTopic)
      jsonAddValue(mqttInterfaceStyle == misSingleTopic ? oHaJson : groups[sensor.group].oHaJson,
                   sensor, forceConfig);
   else if (mqttInterfaceStyle == misMultiTopic)
      mqttHaPublishSensor(sensor, forceConfig);

   // write

   if (mqttInterfaceStyle == misSingleTopic)
   {
      mqttHaWrite(oHaJson, 0);
      json_decref(oHaJson);
      oHaJson = nullptr;
   }

   else if (mqttInterfaceStyle == misGroupedTopic)
   {
      tell(eloDebug2, "Debug: Writing MQTT for %zu groups", groups.size());

      for (auto it : groups)
      {
         if (it.second.oHaJson)
         {
            mqttHaWrite(it.second.oHaJson, it.first);
            json_decref(groups[it.first].oHaJson);
            groups[it.first].oHaJson = nullptr;
         }
      }
   }

   return done;
}

//***************************************************************************
// Push Value to MQTT for Home Automation Systems
//***************************************************************************

int Daemon::mqttHaPublishSensor(SensorData& sensor, bool forceConfig)
{
   if (sensor.type == "")
      return done;

   // check if state topic already exists

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

   if (mqttHaveConfigTopic && sensor.title.length())
   {
      if (!mqttWriter->isConnected())
         return fail;

      // Interface description:
      //   https://www.home-assistant.io/docs/mqtt/discovery/

      bool exists = std::find(configTopicsFor.begin(), configTopicsFor.end(), sDataTopic) != configTopicsFor.end();

      if (forceConfig || !exists)
      {
         char* configTopic {nullptr};
         char* configJson {nullptr};

         // topic don't exists -> create sensor

         if (strcmp(sensor.unit.c_str(), "°") == 0)
            sensor.unit = "°C";
         else if (sensor.unit == "Heizungsstatus" ||
                  sensor.unit == "zst" ||
                  sensor.unit == "txt")
            sensor.unit = "";

         asprintf(&configTopic, "homeassistant/%s/%s/%s/config",
                  iot == iotLight ? "light" : "sensor", myTitle(), sName.c_str());

         tell(eloMqtt, "Info: Sensor '%s' not found at home assistants MQTT, "
              "sendig config message to '%s'", sName.c_str(), configTopic);

         if (iot == iotLight)
         {
            char* cmdTopic {nullptr};
            asprintf(&cmdTopic, TARGET "2mqtt/light/%s/set", sName.c_str());

            asprintf(&configJson, "{"
                     "\"state_topic\"         : \"%s\","
                     "\"command_topic\"       : \"%s\","
                     "\"name\"                : \"%s %s\","
                     "\"unique_id\"           : \"%s_" TARGET "2mqtt\","
                     "\"schema\"              : \"json\","
                     "\"brightness\"          : \"false\""
                     "}",
                     sDataTopic.c_str(), cmdTopic, myTitle(), sensor.title.c_str(), sName.c_str());

            hassCmdTopicMap[cmdTopic] = sensor.name;
            free(cmdTopic);
         }
         else
         {
            asprintf(&configJson, "{"
                     "\"state_topic\"         : \"%s\","
                     "\"unit_of_measurement\" : \"%s\","
                     "\"value_template\"      : \"{{ value_json.value }}\","
                     "\"name\"                : \"%s %s\","
                     "\"unique_id\"           : \"%s_" TARGET "2mqtt\""
                     "}",
                     sDataTopic.c_str(), sensor.unit.c_str(), sensor.title.c_str(), myTitle(), sName.c_str());
         }

         mqttWriter->writeRetained(configTopic, configJson);
         configTopicsFor.push_back(sDataTopic);

         free(configTopic);
         free(configJson);
      }
   }

   // publish actual value

   if (!mqttWriter->isConnected())
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
   mqttWriter->writeRetained(sDataTopic.c_str(), j);
   free(j);
   json_decref(oValue);

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

   if (mqttInterfaceStyle == misGroupedTopic)
      sDataTopic = strReplace("<GROUP>", groups[groupid].name, sDataTopic);

   int status = mqttWriter->write(sDataTopic.c_str(), message);
   free(message);

   return status;
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

   if (isEmpty(mqttUrl))
      return done;

   if (!lastMqttRead)
      lastMqttRead = time(0);

   mqttCheckConnection();

   if (!mqttReader->isConnected())
      return done;

   MemoryStruct message;

   // tell(eloMqtt, "Try reading topic '%s'", mqttReader->getTopic());

   while (mqttReader->read(&message, 10) == success)
   {
      if (isEmpty(message.memory))
         continue;

      lastMqttRead = time(0);

      std::string tp = mqttReader->getLastReadTopic();
      tell(eloMqtt, "<- (%s) [%s] retained %d", tp.c_str(), message.memory, mqttReader->isRetained());

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

      else if (strstr(tp.c_str(), "2mqtt/light/"))
      {
         json_t* jData = jsonLoad(message.memory);

         if (jData)
         {
            dispatchMqttHaCommandRequest(jData, tp.c_str());
            json_decref(jData);
         }
      }

      else if (strstr(tp.c_str(), "2mqtt/command") || strstr(tp.c_str(), "2mqtt/nodered"))
      {
         json_t* jData = jsonLoad(message.memory);

         if (jData)
         {
            dispatchNodeRedCommands(tp.c_str(), jData);
            json_decref(jData);
         }
      }
      else
      {
         dispatchOther(tp.c_str(), message.memory);
      }
   }

   // last successful MQTT read at least in last 5 minutes?

   if (lastMqttRead < time(0)-300 && lastMqttRecover < time(0)-60)
   {
      lastMqttRecover = time(0);
      tell(eloAlways, "No update from MQTT since '%s', disconnect from MQTT to force recover", l2pTime(lastMqttRead).c_str());
      mqttDisconnect();
   }

   return success;
}

//***************************************************************************
// Check MQTT Disconnect
//***************************************************************************

int Daemon::mqttDisconnect()
{
   if (mqttReader) mqttReader->disconnect();
   if (mqttWriter) mqttWriter->disconnect();

   delete mqttReader; mqttReader = nullptr;
   delete mqttWriter; mqttWriter = nullptr;

   tell(eloMqtt, "Disconnected from MQTT");

   return done;
}

//***************************************************************************
// Check MQTT Connection and Connect
//***************************************************************************

int Daemon::mqttCheckConnection()
{
   const char* mqttPingTopic = TARGET "2mqtt/ping";
   static time_t lastMqttPing {0};

   if (isEmpty(mqttUrl))
      return done;

   if (!mqttReader)
      mqttReader = new Mqtt();

   if (!mqttWriter)
      mqttWriter = new Mqtt();

   if (mqttReader->isConnected() && mqttWriter->isConnected())
   {
      if (lastMqttPing < time(0)-30)
      {
         lastMqttPing = time(0);
         mqttWriter->write(mqttPingTopic, "{\"ping\" : true, \"sender\" : \"" TARGET "\"}");
      }

      return success;
   }

   // retry connect all 20 seconds

   if (lastMqttConnectAt >= time(0) - 20)
      return fail;

   if (lastMqttConnectAt)
      tell(eloAlways, "Error: MQTT connection broken, trying reconnect");

   lastMqttConnectAt = time(0);

   // connect reader

   if (!mqttWriter->isConnected())
   {
      if (mqttWriter->connect(mqttUrl, mqttUser, mqttPassword) != success)
      {
         tell(eloAlways, "Error: MQTT: Connecting publisher to '%s' failed", mqttUrl);
         return fail;
      }

      tell(eloMqtt, "MQTT: Connecting publisher to '%s' succeeded", mqttUrl);
   }

   // connect writer

   if (!mqttReader->isConnected())
   {
      if (mqttReader->connect(mqttUrl, mqttUser, mqttPassword) != success)
      {
         tell(eloAlways, "Error: MQTT: Connecting subscriber to '%s' failed", mqttUrl);
         return fail;
      }

      tell(eloMqtt, "MQTT: Connecting subscriber to '%s' succeeded", mqttUrl);

      for (const auto& t : mqttSensorTopics)
      {
         mqttReader->subscribe(t.c_str());
         tell(eloMqtt, "MQTT: Connecting subscriber to '%s' - '%s' succeeded", mqttUrl, t.c_str());
      }
   }

   return success;
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
   if (!mqttWriter || !mqttWriter->isConnected())
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
   json_decref(oJson);
   tell(eloNodeRed, "-> (node-red) (%s) [%s]", TARGET "2mqtt/changes", message);

   int status = mqttWriter->write(TARGET "2mqtt/changes", message);
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

   // create json

   if (!isEmpty(mqttSendWithKeyPrefix))
   {
      std::string kType = mqttSendWithKeyPrefix + sensor.type;
      json_object_set_new(obj, "type", json_string(kType.c_str()));
      json_object_set_new(obj, "address", json_integer(sensor.address));
      json_object_set_new(obj, "unit", json_string(sensor.unit.c_str()));
      json_object_set_new(obj, "title", json_string(sensor.title.c_str()));
      json_object_set_new(obj, "kind", json_string(sensor.kind.c_str()));

      if (sensor.kind == "status")
      {
         json_object_set_new(obj, "state", json_boolean(sensor.state));
         json_object_set_new(obj, "brightness", json_integer(sensor.value));
      }
      else if (sensor.text.length())
         json_object_set_new(obj, "text", json_string(sensor.text.c_str()));
      else if (sensor.kind == "value")
         json_object_set_new(obj, "value", json_real(sensor.value));

      char* key {nullptr};
      asprintf(&key, "%s:0x%02x", sensor.type.c_str(), sensor.address);
      const char* image = getTextImage(key, sensor.text.c_str());
      if (!isEmpty(image))
         json_object_set_new(obj, "image", json_string(image));
      free(key);

      return success;
   }
   else if (forceConfig)
   {
      json_object_set_new(oSensor, "unit", json_string(sensor.unit.c_str()));
      json_object_set_new(oSensor, "description", json_string(sensor.title.c_str()));
   }

   if (sensor.kind == "status")
   {
      json_object_set_new(oSensor, "state", json_string(sensor.state ? "ON" :"OFF"));
      json_object_set_new(oSensor, "brightness", json_integer(255));
   }
   else if (sensor.text.length())
      json_object_set_new(oSensor, "value", json_string(sensor.text.c_str()));
   else if (sensor.kind == "value")
      json_object_set_new(oSensor, "value", json_real(sensor.value));

   if (mqttInterfaceStyle == misSingleTopic)
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
