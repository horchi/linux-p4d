//***************************************************************************
// p4d / Linux - Heizungs Manager
// File hass.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 25.04.2020  Jörg Wendel
//***************************************************************************

#include "lib/json.h"
#include "daemon.h"

//***************************************************************************
// Push Value to Home Assistant
//***************************************************************************

int Daemon::mqttPublishSensor(IoType iot, const char* name, const char* title, const char* unit,
                              double value, const char* text, bool forceConfig)
{
   // check/prepare connection

   if (mqttCheckConnection() != success)
      return fail;

   int status {success};
   MemoryStruct message;
   std::string tp;
   std::string sName = name;

   sName = strReplace("ß", "ss", sName);
   sName = strReplace("ü", "ue", sName);
   sName = strReplace("ö", "oe", sName);
   sName = strReplace("ä", "ae", sName);
   sName = strReplace(" ", "_", sName);

   // mqttDataTopic like "p4d2mqtt/sensor/<NAME>/state"

   std::string sDataTopic = mqttDataTopic;
   sDataTopic = strReplace("<NAME>", sName, sDataTopic);

   if (mqttHaveConfigTopic)
   {
      // Interface description:
      //   https://www.home-assistant.io/docs/mqtt/discovery/

      mqttHassReader->subscribe(sDataTopic.c_str());
      status = mqttHassReader->read(&message, 100);
      tp = mqttHassReader->getLastReadTopic();

      if (status != success && status != Mqtt::wrnTimeout)
         return fail;

      if (forceConfig || status == Mqtt::wrnTimeout)
      {
         char* configTopic {0};
         char* configJson {0};

         // topic don't exists -> create sensor

         if (strcmp(unit, "°") == 0)
            unit = "°C";
         else if (strcmp(unit, "Heizungsstatus") == 0 ||
                  strcmp(unit, "zst") == 0 ||
                  strcmp(unit, "T") == 0)
            unit = "";

         tell(1, "Info: Sensor '%s' not found at home assistants MQTT, sendig config message", sName.c_str());

         asprintf(&configTopic, "homeassistant/%s/pool/%s/config", iot == iotLight ? "light" : "sensor", sName.c_str());
         asprintf(&configJson, "{"
                  "\"unit_of_measurement\" : \"%s\","
                  "\"value_template\"      : \"{{ value_json.value }}\","
                  "\"state_topic\"         : \"p4d2mqtt/sensor/%s/state\","
                  "\"name\"                : \"%s\","
                  "\"unique_id\"           : \"%s_p4d2mqtt\""
                  "}",
                  unit, sName.c_str(), title, sName.c_str());

         mqttHassWriter->writeRetained(configTopic, configJson);

         free(configTopic);
         free(configJson);
      }

      mqttHassReader->unsubscribe(sDataTopic.c_str());
   }

   // publish actual value

   json_t* oValue = json_object();

   if (!isEmpty(text))
      json_object_set_new(oValue, "value", json_string(text));
   else
      json_object_set_new(oValue, "value", json_real(value));

   char* j = json_dumps(oValue, JSON_PRESERVE_ORDER); // |JSON_REAL_PRECISION(5));

   mqttHassWriter->writeRetained(sDataTopic.c_str(), j);

   json_decref(oValue);

   return success;
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

//***************************************************************************
// MQTT Write
//***************************************************************************

int Daemon::mqttWrite(json_t* obj, uint groupid)
{
   std::string sDataTopic = mqttDataTopic;

   // check/prepare connection

   if (mqttCheckConnection() != success)
      return fail;

   char* message = json_dumps(obj, JSON_REAL_PRECISION(4));
   tell(3, "Debug: JSON: [%s]", message);

   if (mqttInterfaceStyle == misGroupedTopic)
      sDataTopic = strReplace("<GROUP>", groups[groupid].name, sDataTopic);

   return mqttHassWriter->write(sDataTopic.c_str(), message);
}

//***************************************************************************
// Perform MQTT Requests
//   - check 'mqttHassCommandReader' for commands of e.g. a home automation
//***************************************************************************

int Daemon::performMqttRequests()
{
   static time_t lastMqttRead {0};
   static time_t lastMqttRecover {0};

   if (!lastMqttRead)
      lastMqttRead = time(0);

   if (mqttCheckConnection() != success)
      return fail;

   MemoryStruct message;
   std::string topic;

   if (mqttHassCommandReader->read(&message, 10) == success)
   {
      topic = mqttHassCommandReader->getLastReadTopic();
      tell(eloAlways, "<- (%s) [%s]", topic.c_str(), message.memory);

      dispatchMqttCommandRequest(message.memory);
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

         if (strcmp(tp.c_str(), TARGET "2mqtt/w1") == 0)
            dispatchW1Msg(message.memory);
         // else if (strcmp(tp.c_str(), TARGET "2mqtt/arduino/out") == 0)
         //    dispatchArduinoMsg(message.memory);
      }
   }

   // last successful W1 read at least in last 5 minutes?

   if (lastMqttRead < time(0)-300 && lastMqttRecover < time(0)-60)
   {
      lastMqttRecover = time(0);
      tell(eloAlways, "Error: No update from poolMQTT since '%s', "
           "disconnect from MQTT to force recover", l2pTime(lastMqttRead).c_str());
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
   // if (mqttHassCommandReader)    mqttHassCommandReader->disconnect();
   // if (mqttHassWriter)           mqttHassWriter->disconnect();
   // if (mqttHassReader)           mqttHassReader->disconnect();

   delete mqttReader;            mqttReader = nullptr;
   // delete mqttHassCommandReader; mqttHassCommandReader = nullptr;
   // delete mqttHassWriter;        mqttHassWriter = nullptr;
   // delete mqttHassReader;        mqttHassReader = nullptr;

   return done;
}

//***************************************************************************
// Check MQTT Connection
//***************************************************************************

int Daemon::mqttCheckConnection()
{
   if (!mqttReader)
      mqttReader = new Mqtt();

   if (!mqttHassWriter)
      mqttHassWriter = new Mqtt();

   if (!mqttHassReader)
      mqttHassReader = new Mqtt();

   if (!mqttHassCommandReader)
      mqttHassCommandReader = new Mqtt();

   if (mqttHassCommandReader->isConnected() && mqttHassWriter->isConnected() && mqttHassReader->isConnected())
      return success;

   // if (lastMqttConnectAt >= time(0) - 60)
   //    return fail;

   // lastMqttConnectAt = time(0);

   if (!mqttHassWriter->isConnected())
   {
      if (mqttHassWriter->connect(mqttHassUrl, mqttUser, mqttPassword) != success)
      {
         tell(0, "Error: MQTT: Connecting publisher to '%s' failed", mqttHassUrl);
         return fail;
      }

      tell(0, "MQTT: Connecting publisher to '%s' succeeded", mqttHassUrl);
   }

   if (!mqttHassCommandReader->isConnected())
   {
      if (mqttHassCommandReader->connect(mqttHassUrl) != success)
      {
         tell(0, "Error: MQTT: Connecting subscriber to '%s' failed", mqttHassUrl);
         return fail;
      }

      mqttHassCommandReader->subscribe("mqtt2p4d/command");
      tell(0, "MQTT: Connecting command subscriber to '%s' succeeded", mqttHassUrl);
   }

   if (mqttHaveConfigTopic)
   {
      if (!mqttHassReader->isConnected())
      {
         if (mqttHassReader->connect(mqttHassUrl, mqttUser, mqttPassword) != success)
         {
            tell(0, "Error: MQTT: Connecting subscriber to '%s' failed", mqttHassUrl);
            return fail;
         }

         tell(0, "MQTT: Connecting subscriber to '%s' succeeded", mqttHassUrl);
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
            mqttReader->subscribe(TARGET "2mqtt/w1");
            tell(0, "MQTT: Connecting subscriber to '%s' - '%s' succeeded", mqttUrl, TARGET "2mqtt/w1");
            mqttReader->subscribe(TARGET "2mqtt/arduino/out");
            tell(0, "MQTT: Connecting subscriber to '%s' - '%s' succeeded", mqttUrl, TARGET "2mqtt/arduino/out");
         }
      }
   }

   return success;
}
