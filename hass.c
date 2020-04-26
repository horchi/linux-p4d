//***************************************************************************
// p4d / Linux - Heizungs Manager
// File hass.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 25.04.2020  Jörg Wendel
//***************************************************************************

#include "p4d.h"

//***************************************************************************
// Push Value to Home Assistant
//***************************************************************************

#ifdef MQTT_HASS

int P4d::hassPush(const char* name, const char* title, const char* unit,
                  double value, const char* text, bool forceConfig)
{
   int status = success;

   if (isEmpty(mqttDataTopic))
      return done;

   // check/prepare reader/writer connection

   if (hassCheckConnection() != success)
      return fail;

   // check if state topic already exists

   std::string message;
   char* stateTopic = 0;

   std::string sName = name;
   sName = strReplace("ß", "ss", sName);
   sName = strReplace("ü", "ue", sName);
   sName = strReplace("ö", "oe", sName);
   sName = strReplace("ä", "ae", sName);

   asprintf(&stateTopic, "%s%s%s/state",
            mqttDataTopic[strlen(mqttDataTopic)-1] == '/' ? "" : "/",
            mqttDataTopic, sName.c_str());

   if (mqttHaveConfigTopic)
   {
      mqttReader->subscribe(stateTopic);
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

   if (!isEmpty(text))
      asprintf(&valueJson, "{ \"value\" : \"%s\" }", text);
   else
      asprintf(&valueJson, "{ \"value\" : \"%.2f\" }", value);

   mqttWriter->write(stateTopic, valueJson);

   free(valueJson);
   free(stateTopic);

   return success;
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
