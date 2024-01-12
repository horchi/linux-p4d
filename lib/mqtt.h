//***************************************************************************
// MQTT / Linux
// File mqtt.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 05.04.2020  Jörg Wendel
//***************************************************************************

#ifndef _MQTT_CLIENT_H
#define _MQTT_CLIENT_H

#include <queue>

#include "common.h"
#include "thread.h"

struct mqtt_response_publish;
struct mqtt_client;

//***************************************************************************
// MQTT Client
//***************************************************************************

class Mqtt
{
   public:

      enum Error
      {
         wrnTimeout = -10000,
         wrnEmptyMessage
      };

      Mqtt(int aHeartBeat = 400);
      virtual ~Mqtt();

      const char* nameOf()  { return "Mqtt"; }

      // connect / disconnect

      virtual int connect(const char* aUrl, const char* user = nullptr, const char* password = nullptr);
      virtual int disconnect();
      virtual int isConnected() { return connected; }

      // subscribe

      virtual int subscribe(const char* topic);
      virtual int unsubscribe(const char* topic);

      // read / write

      virtual int read(MemoryStruct* message, int aTimeout = 0);
      virtual int write(const char* topic, const char* message, int len = 0);
      virtual int writeRetained(const char* topic, const char* message);

      // ...

      virtual std::string getLastReadTopic()           { return lastReadTopic; }
      virtual bool isRetained()                        { return retained; }

      void appendMessage(mqtt_response_publish* theMessage);
      size_t getCount()   { return receivedMessages.size(); }

      const char* getTopic() { return theTopic.c_str(); }

   protected:

      struct Message
      {
         Message() {};
         virtual ~Message() {}

         bool duplicate {false};
         uint8_t qos {0};
         uint8_t retained {0};
         std::string topic;
         uint16_t packetId {0};
         MemoryStruct payload;
      };

      int write(const char* topic, const char* message, size_t len, uint8_t flags);
      int openSocket(const char* addr, int port);

      static void* refreshFct(void* client);
      static void publishCallback(void** user, mqtt_response_publish* published);

      std::string lastReadTopic;
      std::string theTopic;
      bool connected {false};
      ulong lastResult {0};
      mqtt_client* mqttClient {nullptr};
      int sockfd {-1};
      bool retained {false};                      // retained flag of last read
      pthread_t refreshThread {0};

      std::queue<Message*> receivedMessages;
      cMyMutex readMutex;
      cCondVar readCond;
      uint heartBeat {400};
      cMyMutex connectMutex;

      // the message buffers for the mqtt lib

      size_t sizeSendBuf {0};
      size_t sizeReceiveBuf {0};
      uint8_t* sendbuf {nullptr};   // sendbuf should be large enough to hold multiple whole mqtt messages
      uint8_t* recvbuf {nullptr};   // recvbuf should be large enough any whole mqtt message expected to be received
};

//***************************************************************************
#endif //  _MQTT_CLIENT_H
