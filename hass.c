//***************************************************************************
// p4d / Linux - Heizungs Manager
// File hass.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 01.03.2018  Jörg Wendel
//***************************************************************************

#include "p4d.h"

//***************************************************************************
// Push Value to Home Assistant
//***************************************************************************

#ifdef MQTT_HASS

int P4d::hassPush(const char* name, const char* title, const char* unit, double value, const char* text)
{
   int status = success;

   // check/prepare reader/writer connection

   if (hassCheckConnection() != success)
      return fail;

   // check if state topic already exists

   std::string message;
   char* stateTopic = 0;
   asprintf(&stateTopic, "p4d2mqtt/sensor/%s/state", name);

   mqttReader->subscribe(stateTopic);

   if ((status = mqttReader->read(&message)) != success)
   {
      char* configTopic = 0;
      char* configJson = 0;

      if (status != MqTTClient::wrnNoMessagePending)
         return fail;

      // topic don't exists -> create sensor

      if (strcmp(unit, "°") == 0)
         unit = "°C";

      tell(1, "Info: Sensor '%s' not found at home assistants MQTT, "
           "sendig config message", name);

      asprintf(&configTopic, "homeassistant/sensor/%s/config", name);
      asprintf(&configJson, "{"
               "\"unit_of_measurement\" : \"%s\","
               "\"value_template\"      : \"{{ value_json.value }}\","
               "\"state_topic\"         : \"p4d2mqtt/sensor/%s/state\","
               "\"name\"                : \"%s\","
               "\"unique_id\"           : \"%s_p4d2mqtt\""
               "}",
               unit, name, title, name);

      mqttWriter->write(configTopic, configJson);

      free(configTopic);
      free(configJson);
   }

   mqttReader->unsubscribe();

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
      mqttWriter = new MqTTPublishClient(hassMqttUrl, "p4d_publisher");
      mqttWriter->setConnectTimeout(15); // seconds
   }

   if (!mqttReader)
   {
      mqttReader = new MqTTSubscribeClient(hassMqttUrl, "p4d_subscriber");
      mqttReader->setConnectTimeout(15); // seconds
      mqttReader->setTimeout(100);       // milli seconds
   }

   if (!mqttWriter->isConnected())
   {
      if (mqttWriter->connect() != success)
      {
         tell(0, "Error: MQTT: Connecting publisher to '%s' failed", hassMqttUrl);
         return fail;
      }

      tell(0, "MQTT: Connecting publisher to '%s' succeeded", hassMqttUrl);
   }

   if (!mqttReader->isConnected())
   {
      if (mqttReader->connect() != success)
      {
         tell(0, "Error: MQTT: Connecting subscriber to '%s' failed", hassMqttUrl);
         return fail;
      }

      tell(0, "MQTT: Connecting subscriber to '%s' succeeded", hassMqttUrl);
   }

   return success;
}

#else

int P4d::hassPush(const char* name, const char* title, const char* unit, double value, const char* text)
{
   return success;
}

#endif
