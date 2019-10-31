
//
// g++ -Ilib mqtttest.c lib/mqtt.c lib/common.c -lpthread  -lpaho-mqtt3cs -o mqtt
//

#include "lib/common.h"
#include "mqtt.h"

int main(int argc, const char** argv)
{
   MqTTPublishClient* mqttWriter {nullptr};
   MqTTSubscribeClient* mqttReader {nullptr};

   logstdout = yes;
   loglevel = 5;

   std::string hassMqttUrl = "tcp://127.0.0.1:1883";
   hassMqttUrl = "tcp://172.27.100.154:1883";

   const char* title = "Nadine";
   const char* unit = "°C";
   const char* name = "Nadine_0x03";
   double value = 12.12;

   // prepare reader/writer connection

   if (!mqttWriter)
   {
      mqttWriter = new MqTTPublishClient(hassMqttUrl.c_str(), "p4d_publisher");
      mqttWriter->setConnectTimeout(15); // seconds
   }

   if (!mqttReader)
   {
      mqttReader = new MqTTSubscribeClient(hassMqttUrl.c_str(), "p4d_subscriber");
      mqttReader->setConnectTimeout(15); // seconds
      mqttReader->setTimeout(100);       // milli seconds
   }

   if (!mqttWriter->isConnected())
   {
      if (mqttWriter->connect() != success)
      {
         tell(0, "Error: Connecting writer to '%s' failed", hassMqttUrl.c_str());
         return fail;
      }
      tell(0, "Connecting to '%s' succeeded", hassMqttUrl.c_str());
   }

   if (!mqttReader->isConnected())
   {
      if (mqttReader->connect() != success)
      {
         tell(0, "Error: Connecting subscriber to '%s' failed", hassMqttUrl.c_str());
         return fail;
      }
   }

   // check if state topic already exists

   char* stateTopic = 0;
   asprintf(&stateTopic, "p4d2mqtt/sensor/%s/state", name);

   mqttReader->subscribe(stateTopic);

   std::string message;
   int status;

   if ((status = mqttReader->read(&message)) != success)
   {
      char* configTopic = 0;
      char* configJson = 0;

      if (status != MqTTClient::wrnNoMessagePending)
         return fail;

      // topic don't exists -> create sensor

      tell(1, "Info: Sensor '%s' not found at home assistant, sendig config message", name);

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

   // publish value

   char* valueJson = 0;
   asprintf(&valueJson, "{ \"value\" : \"%.2f\" }", value);

   mqttWriter->write(stateTopic, valueJson);

   free(valueJson);
   free(stateTopic);

   delete mqttWriter;
   delete mqttReader;

   return success;
}
