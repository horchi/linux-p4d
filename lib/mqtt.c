//***************************************************************************
// p4d / Linux - Heizungs Manager
// File mqtt.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 05.03.2018  Jörg Wendel
//***************************************************************************

#include <unistd.h>
#include <time.h>

#include "mqtt.h"

//***************************************************************************
// MQTT Client
//***************************************************************************

void MqTTClient::setUsername(const char* username)
{
   free((void*)options.username);
   options.username = username ? strdup(username) : 0;
}

void MqTTClient::setPassword(const char* password)
{
   free((void*)options.password);
   options.password = password ? strdup(password) : 0;
}

int MqTTClient::connect()
{
   disconnect();

   connected = false;
   lastResult = MQTTClient_create(&client, uri, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);

   if (lastResult != MQTTCLIENT_SUCCESS)
   {
      tell(0, "Error: Creating client for '%s' failed", uri);
      return fail;
   }

   lastResult = MQTTClient_connect(client, &options);

   if (lastResult != MQTTCLIENT_SUCCESS)
   {
      tell(0, "Error: Connecting to '%s' failed", uri);
      MQTTClient_destroy(&client);
      return fail;
   }

   connected = true;

   return success;
}

int MqTTClient::disconnect()
{
   if (!connected)
      return success;

   lastResult = MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   connected = false;

   if (lastResult)
      return fail;

   return success;
}

//***************************************************************************
// Publish Client
//***************************************************************************
//***************************************************************************
// Write
//***************************************************************************

int MqTTPublishClient::write(const char* topic, const char* msg, size_t size)
{
   MQTTClient_message message = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;

   if (!msg || !topic)
      return success;

   if (!size)
      size = strlen(msg); // + 1;

   message.payload = (void*)msg;
   message.payloadlen = size;
   message.qos = 0;
   message.retained = 1;

   // lastResult = MQTTCLIENT_SUCCESS;
   lastResult = MQTTClient_publishMessage(client, topic, &message, &token);

   if (lastResult != MQTTCLIENT_SUCCESS)
   {
      tell(0, "Error: Writing to topic '%s' failed", topic);
      disconnect();
      return fail;
   }

   lastResult = MQTTClient_waitForCompletion(client, token, 10000L);

   if (lastResult != MQTTCLIENT_SUCCESS)
   {
      tell(0, "Error: Writing not completed");
      disconnect();
      return fail;
   }

   tell(1, "-> (%s)[%s]", topic, msg);

   return success;
}

//***************************************************************************
// Subscribe Client
//***************************************************************************
//***************************************************************************
// Disconnect
//***************************************************************************

int MqTTSubscribeClient::disconnect()
{
   unsubscribe();
   return MqTTClient::disconnect();
}

//***************************************************************************
// Subscribe
//***************************************************************************

int MqTTSubscribeClient::subscribe(const char* aTopic)
{
   if (!connected || !aTopic)
   {
      tell(0, "Error: Can't subscribe, not connected or topic missing");
      return fail;
   }

   if (topic.length())
      unsubscribe();

   topic = aTopic;
   lastResult = MQTTClient_subscribe(client, topic.c_str(), 2);

   if (lastResult != MQTTCLIENT_SUCCESS)
   {
      tell(0, "Error: Subscribing to '%s' failed", topic.c_str());
      disconnect();
      return fail;
   }

   tell(3, "Subscribing to topic '%s' succeeded", topic.c_str());

   return success;
}

//***************************************************************************
// Unsubscribe
//***************************************************************************

int MqTTSubscribeClient::unsubscribe()
{
   if (!connected || !topic.length())
      return success;

   lastResult = MQTTClient_unsubscribe(client, topic.c_str());
   topic.clear();

   if (lastResult != MQTTCLIENT_SUCCESS)
   {
      tell(0, "Error: Unsubscribing to '%s' failed", topic.c_str());
      return fail;
   }

   return success;
}

//***************************************************************************
// Read
//***************************************************************************

int MqTTSubscribeClient::read(std::string* message)
{
   MQTTClient_message* _message = 0;
   char* receivedTopic = 0;
   int receivedTopicLen = 0;

   message->clear();

   int lastResult = MQTTClient_receive(client, &receivedTopic, &receivedTopicLen, &_message, timeout);
   MQTTClient_free(receivedTopic);

   if (lastResult != MQTTCLIENT_SUCCESS && lastResult != MQTTCLIENT_TOPICNAME_TRUNCATED)
   {
      tell(0, "Error: Reading topic '%s' failed, result was (%d)", topic.c_str(), lastResult);

      if (_message)
         MQTTClient_freeMessage(&_message);

      disconnect();

      return fail;
   }

   if (!_message)
      return wrnNoMessagePending;

   *message = (const char*)_message->payload;
   MQTTClient_freeMessage(&_message);

   return success;
}
