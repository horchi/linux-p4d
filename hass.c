//***************************************************************************
// p4d / Linux - Heizungs Manager
// File hass.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 25.04.2020  Jörg Wendel
//***************************************************************************

#include "p4d.h"

#include <jansson.h>

//***************************************************************************
// Push Value to Home Assistant
//***************************************************************************

#ifdef MQTT_HASS

int P4d::hassPush(const char* name, const char* title, const char* unit,
                  double value, const char* text, bool forceConfig)
{
   // check/prepare connection

   if (hassCheckConnection() != success)
      return fail;

   int status = success;
   std::string message;
   std::string sName = name;

   sName = strReplace("ß", "ss", sName);
   sName = strReplace("ü", "ue", sName);
   sName = strReplace("ö", "oe", sName);
   sName = strReplace("ä", "ae", sName);

   // mqttDataTopic like "p4d2mqtt/sensor/<NAME>/state"

   std::string sDataTopic = mqttDataTopic;
   sDataTopic = strReplace("<NAME>", sName, sDataTopic);

   if (mqttHaveConfigTopic)
   {
      // check if state topic already exists

      mqttReader->subscribe(sDataTopic.c_str());
      status = mqttReader->read(&message);

      if (status != success && status != MqTTClient::wrnNoMessagePending)
         return fail;

      if (forceConfig || status == MqTTClient::wrnNoMessagePending)
      {
         char* configTopic = 0;
         char* configJson = 0;

         // topic don't exists -> create sensor

         if (strcmp(unit, "°") == 0)
            unit = "°C";
         else if (strcmp(unit, "Heizungsstatus") == 0 ||
                  strcmp(unit, "zst") == 0 ||
                  strcmp(unit, "T") == 0)
            unit = "";

         tell(1, "Info: Sensor '%s' not found at home assistants MQTT, "
              "sendig config message", sName.c_str());

         asprintf(&configTopic, "homeassistant/sensor/%s/config", sName.c_str());
         asprintf(&configJson, "{"
                  "\"unit_of_measurement\" : \"%s\","
                  "\"value_template\"      : \"{{ value_json.value }}\","
                  "\"state_topic\"         : \"p4d2mqtt/sensor/%s/state\","
                  "\"name\"                : \"%s\","
                  "\"unique_id\"           : \"%s_p4d2mqtt\""
                  "}",
                  unit, sName.c_str(), title, sName.c_str());

         mqttWriter->write(configTopic, configJson);

         free(configTopic);
         free(configJson);
      }

      mqttReader->unsubscribe();
   }

   // publish actual value

   char* valueJson = 0;
   json_t* oValue = json_object();

   if (!isEmpty(text))
      asprintf(&valueJson, "%s", text);
   else
      asprintf(&valueJson, "%.2f", value);

   json_object_set_new(oValue, "value", json_string(valueJson));

   char* j = json_dumps(oValue, JSON_PRESERVE_ORDER); // |JSON_REAL_PRECISION(2));

   mqttWriter->write(sDataTopic.c_str(), j);

   json_decref(oValue);
   free(valueJson);

   return success;
}

//***************************************************************************
// Json Add Value
//***************************************************************************

int P4d::jsonAddValue(json_t* obj, const char* name, const char* title, const char* unit,
                      double theValue, uint groupid, const char* text, bool forceConfig)
{
   char* value = 0;
   std::string group = groups[groupid];
   std::string sName = name;
   bool newGroup {false};

   json_t* oSensor = json_object();
   json_t* oGroup = json_object_get(obj, group.c_str());

   if (!oGroup)
   {
      oGroup = json_object();
      newGroup = true;
   }

   sName = strReplace("ß", "ss", sName);
   sName = strReplace("ü", "ue", sName);
   sName = strReplace("ö", "oe", sName);
   sName = strReplace("ä", "ae", sName);

   if (!isEmpty(text))
      asprintf(&value, "%s", text);
   else
      asprintf(&value, "%.2f", theValue);

   if (strcmp(unit, "°") == 0)
      unit = "°C";

   // create json

   json_object_set_new(oSensor, "value", json_string(value));

   if (forceConfig)
   {
      json_object_set_new(oSensor, "unit", json_string(unit));
      json_object_set_new(oSensor, "description", json_string(title));
   }

   json_object_set_new(oGroup, sName.c_str(), oSensor);

   if (newGroup)
      json_object_set_new(obj, group.c_str(), oGroup);

   free(value);

   return success;
}

//***************************************************************************
// MQTT Write
//***************************************************************************

int P4d::mqttWrite(json_t* obj)
{
   // check/prepare connection

   if (hassCheckConnection() != success)
      return fail;

   char* message = json_dumps(oJson, JSON_PRESERVE_ORDER); // |JSON_REAL_PRECISION(2));
   tell(3, "Debug: JSON: [%s]", message);

   return mqttWriter->write(mqttDataTopic, message);
}

//***************************************************************************
// Check MQTT Connection
//***************************************************************************

int P4d::hassCheckConnection()
{
   if (!mqttWriter)
   {
      mqttWriter = new MqTTPublishClient(mqttUrl, "p4d_publisher");
      mqttWriter->setConnectTimeout(15);   // seconds
   }

   if (!mqttWriter->isConnected())
   {
      if (mqttWriter->connect() != success)
      {
         tell(0, "Error: MQTT: Connecting publisher to '%s' failed", mqttUrl);
         return fail;
      }

      tell(0, "MQTT: Connecting publisher to '%s' succeeded", mqttUrl);
   }

   if (mqttHaveConfigTopic)
   {
      if (!mqttReader)
      {
         mqttReader = new MqTTSubscribeClient(mqttUrl, "p4d_subscriber");
         mqttReader->setConnectTimeout(15); // seconds
         mqttReader->setTimeout(100);       // milli seconds
      }

      if (!mqttReader->isConnected())
      {
         if (mqttReader->connect() != success)
         {
            tell(0, "Error: MQTT: Connecting subscriber to '%s' failed", mqttUrl);
            return fail;
         }

         tell(0, "MQTT: Connecting subscriber to '%s' succeeded", mqttUrl);
      }
   }

   return success;
}

#endif
