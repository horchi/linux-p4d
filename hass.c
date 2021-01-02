//***************************************************************************
// p4d / Linux - Heizungs Manager
// File hass.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 25.04.2020  Jörg Wendel
//***************************************************************************

#include <jansson.h>

// #include "lib/json.h"
#include "p4d.h"

//***************************************************************************
// Push Value to Home Assistant
//***************************************************************************

int P4d::mqttPublishSensor(const char* name, const char* title, const char* unit,
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

      mqttReader->subscribe(sDataTopic.c_str());
      status = mqttReader->read(&message, 100);
      tp = mqttReader->getLastReadTopic();

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

         asprintf(&configTopic, "homeassistant/sensor/%s/config", sName.c_str());
         asprintf(&configJson, "{"
                  "\"unit_of_measurement\" : \"%s\","
                  "\"value_template\"      : \"{{ value_json.value }}\","
                  "\"state_topic\"         : \"p4d2mqtt/sensor/%s/state\","
                  "\"name\"                : \"%s\","
                  "\"unique_id\"           : \"%s_p4d2mqtt\""
                  "}",
                  unit, sName.c_str(), title, sName.c_str());

         mqttWriter->writeRetained(configTopic, configJson);

         free(configTopic);
         free(configJson);
      }

      mqttReader->unsubscribe(sDataTopic.c_str());
   }

   // publish actual value

   json_t* oValue = json_object();

   if (!isEmpty(text))
      json_object_set_new(oValue, "value", json_string(text));
   else
      json_object_set_new(oValue, "value", json_real(value));

   char* j = json_dumps(oValue, JSON_PRESERVE_ORDER); // |JSON_REAL_PRECISION(5));

   mqttWriter->writeRetained(sDataTopic.c_str(), j);

   json_decref(oValue);

   return success;
}

//***************************************************************************
// Json Add Value
//***************************************************************************

int P4d::jsonAddValue(json_t* obj, const char* name, const char* title, const char* unit,
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

int P4d::mqttWrite(json_t* obj, uint groupid)
{
   std::string sDataTopic = mqttDataTopic;

   // check/prepare connection

   if (mqttCheckConnection() != success)
      return fail;

   char* message = json_dumps(obj, JSON_REAL_PRECISION(4));
   tell(3, "Debug: JSON: [%s]", message);

   if (mqttInterfaceStyle == misGroupedTopic)
      sDataTopic = strReplace("<GROUP>", groups[groupid].name, sDataTopic);

   return mqttWriter->write(sDataTopic.c_str(), message);
}

//***************************************************************************
// Check MQTT Connection
//***************************************************************************

int P4d::mqttCheckConnection()
{
   if (!mqttWriter)
      mqttWriter = new Mqtt();

   if (!mqttWriter->isConnected())
   {
      if (mqttWriter->connect(mqttUrl, mqttUser, mqttPassword) != success)
      {
         tell(0, "Error: MQTT: Connecting publisher to '%s' failed", mqttUrl);
         return fail;
      }

      tell(0, "MQTT: Connecting publisher to '%s' succeeded", mqttUrl);
   }

   if (mqttHaveConfigTopic)
   {
      if (!mqttReader)
         mqttReader = new Mqtt();

      if (!mqttReader->isConnected())
      {
         if (mqttReader->connect(mqttUrl, mqttUser, mqttPassword) != success)
         {
            tell(0, "Error: MQTT: Connecting subscriber to '%s' failed", mqttUrl);
            return fail;
         }

         tell(0, "MQTT: Connecting subscriber to '%s' succeeded", mqttUrl);
      }
   }

   return success;
}
