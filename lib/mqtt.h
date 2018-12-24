//***************************************************************************
// p4d / Linux - Heizungs Manager
// File mqtt.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 05.03.2018  Jörg Wendel
//***************************************************************************

#ifndef __MQTT_H__
#define __MQTT_H__

#include <stdlib.h>
#include <string.h>

extern "C" {
   #include "MQTTClient.h"
   #include "MQTTClientPersistence.h"
}

#include "common.h"

//***************************************************************************
// Class MqTTClient
//***************************************************************************

class MqTTClient
{
   public:

      enum ClientType
      {
         MQTT_PUBLISHER,
         MQTT_SUBSCRIBER
      };

      enum Error
      {
         wrnNoMessagePending = -100
      };

      MqTTClient(const char* aUri, const char* aClientId)
         : options(MQTTClient_connectOptions_initializer),
           connected(false),
           uri(strdup(aUri)),
           clientId(strdup(aClientId)),
           timeout(2000),
           lastResult(MQTTCLIENT_SUCCESS)
      {
         options.keepAliveInterval = 20;
         options.cleansession = true;
         options.reliable = true;
      }

      virtual ~MqTTClient()
      {
         disconnect();
         free(uri);
         free(clientId);
         free((void*)options.username);
         free((void*)options.password);
      }

      virtual int connect();
      virtual int disconnect();
      virtual void yield()      { MQTTClient_yield(); };
      bool isConnected()        { return connected; }

      void setTimeout(int aTimeout){ timeout = aTimeout; }
      void setUsername(const char* username);
      void setPassword(const char* password);
      void setConnectTimeout(int timeout) { options.connectTimeout = timeout; }
      void setRetryInterval(int interval) { options.retryInterval = interval; }

      int getLastResult() const           { return lastResult; }

   protected:

      MQTTClient client;
      MQTTClient_connectOptions options;

      bool connected;
      char* uri;
      char* clientId;
      int timeout;
      int lastResult;
};

//***************************************************************************
// Class
//***************************************************************************

class MqTTPublishClient : public MqTTClient
{
   public:

      MqTTPublishClient(const char* aUri, const char* aClientId)
         : MqTTClient(aUri, aClientId) {}

      int write(const char* topic, const char* msg, size_t sz = 0);
};

//***************************************************************************
// Class
//***************************************************************************

class MqTTSubscribeClient: public MqTTClient
{
   public:

      MqTTSubscribeClient(const char* aUri, const char* aClientId)
         : MqTTClient(aUri, aClientId) {}

      int disconnect();
      int subscribe(const char* aTopic);
      int unsubscribe();
      int read(std::string* message);

   private:

      std::string topic;
};

#endif
