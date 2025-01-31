//***************************************************************************
// MQTT / Linux
// File mqtt.cc
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 05.04.2020  Jörg Wendel
//***************************************************************************

#include <sys/syscall.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <iostream>

#include "mqtt.h"

extern "C"
{
   #include "mqtt_c.h"
}

//***************************************************************************
// Object
//***************************************************************************

Mqtt::Mqtt(int aHeartBeat)
{
   heartBeat = aHeartBeat;
   lastResult = MQTT_OK;
   mqttClient = new mqtt_client;
   // memset(mqttClient, 0, sizeof(mqtt_client));
}

Mqtt::~Mqtt()
{
   delete[] sendbuf;
   delete[] recvbuf;
   delete mqttClient;
}

//***************************************************************************
// Called if message was received
//***************************************************************************

void Mqtt::publishCallback(void** user, mqtt_response_publish* published)
{
   ((Mqtt*)*user)->appendMessage(published);

#ifdef __MQTT_DEBUG
   std::sting topic;
   topic.assign((const char*)published->topic_name, published->topic_name_size);
   tell(eloAlways, "Received at '%s' (%d) %.*s\n", topic.c_str(), published->dup_flag,
        (int)published->application_message_size, (const char*)published->application_message);
#endif
}

void* Mqtt::refreshFct(void* client)
{
   Mqtt* mqtt = (Mqtt*)client;
   mqtt_client* cl = mqtt->mqttClient;

   while (!cl->close_now)
   {
      int result {0};

      if ((result = mqtt_sync(cl)) != MQTT_OK)
      {
         tell(eloAlways, "Error: mqtt_sync for connection '%s' failed, result was %d '%s'",
              mqtt->theTopic.c_str(), result, mqtt_error_str((MQTTErrors)result));
         mqtt->disconnect();
      }

      cCondWait::SleepMs(10);
      // tell(eloAlways, "refreshFct '%s' tid: %ld", mqtt->theTopic.c_str(), syscall(__NR_gettid));
   }

   cl->close_now = 2;

   return nullptr;
}

void Mqtt::appendMessage(mqtt_response_publish* published)
{
   Mqtt::Message* msg = new Mqtt::Message();

   msg->duplicate = published->dup_flag;
   msg->qos = published->qos_level;
   msg->retained = published->retain_flag;
   msg->topic.assign((const char*)published->topic_name, published->topic_name_size);
   msg->packetId = published->packet_id;
   msg->payload.clear();
   msg->payload.append((const char*)published->application_message, published->application_message_size);

   {
      cMyMutexLock lock(&readMutex);
      receivedMessages.push(msg);
   }

   readCond.Broadcast();
}

//***************************************************************************
// Connect
//***************************************************************************

int Mqtt::connect(const char* aUrl, const char* user, const char* password)
{
   disconnect();

   char url[512+TB] {};
   strcpy(url, aUrl);
   int port {1883};
   char* hostname {url};
   char* p {};

   if ((p = strrchr(hostname, '/')))
      hostname = p+1;

   if ((p = strchr(hostname, ':')))
   {
      *p++ = '\0';
      port = atoi(p);
   }

   cMyMutexLock lock(&connectMutex);

   sockfd = openSocket(hostname, port);

   if (sockfd == -1)
   {
      tell(eloAlways, "Error: Failed to open socket '%s'", strerror(errno));
      return fail;
   }

   delete[] sendbuf;
   delete[] recvbuf;

   sizeSendBuf = 1024 * 1024;
   sendbuf = new uint8_t[sizeSendBuf];
   sizeReceiveBuf = 1024 * 1024;
   recvbuf = new uint8_t[sizeReceiveBuf];

   // refresh thread

   if (pthread_create(&refreshThread, NULL, refreshFct, this))
   {
      tell(eloAlways, "Error: Failed to start client daemon thread");
      close(sockfd);
      sockfd = -1;
      return fail;
   }

   pthread_detach(refreshThread);
   mqttClient->publish_response_callback_state = this;
   lastResult = mqtt_init(mqttClient, sockfd, sendbuf, sizeSendBuf, recvbuf, sizeReceiveBuf, publishCallback, refreshThread, SIGUSR2);

   if (lastResult != MQTT_OK)
   {
      tell(eloAlways, "Error: Initializing client failed");
      return fail;
   }

   // #TODO - some connecting options needed?

   lastResult = mqtt_connect(mqttClient, "", NULL, NULL, 0, user, password, MQTT_CONNECT_RESERVED | MQTT_CONNECT_CLEAN_SESSION, 400);

   if (lastResult != MQTT_OK)
   {
      tell(eloAlways, "Error: Connecting to '%s:%d' failed with code (%ld)", hostname, port, lastResult);
      close(sockfd);
      return fail;
   }

   connected = true;

   return success;
}

//***************************************************************************
// Disconnect
//***************************************************************************

int Mqtt::disconnect()
{
   cMyMutexLock lock(&connectMutex);

   if (!isConnected())
      return success;

   connected = false;
   tell(eloDebug, "Debug: Disconnecting from MQTT");
   mqttClient->close_now = 1;

   time_t endWait = time(0) + 10;

   while (mqttClient->close_now != 2 && time(0) < endWait)
      usleep(100000);

   // stop the refresh thread

   tell(eloDebug, "Info: Stopping refresh thread for '%s' - '%s'", theTopic.c_str(),
        mqttClient->close_now == 2 ? "succeeded" : "aborted");

   if (mqttClient->close_now == 2)
      pthread_join(refreshThread, 0);
   else if (refreshThread)
      pthread_cancel(refreshThread);

   refreshThread = 0;

   // disconnect and close socket

   mqtt_disconnect(mqttClient);

   if (sockfd != -1)
      close(sockfd);

   sockfd = -1;

   return success;
}

//***************************************************************************
// Subscribe / Unsubscribe
//***************************************************************************

int Mqtt::subscribe(const char* topic)
{
   theTopic.clear();

   if (!isConnected() || isEmpty(topic))
   {
      tell(eloAlways, "Error: Can't subscribe, not connected or topic '%s' missing", topic);
      return fail;
   }

   lastResult = mqtt_subscribe(mqttClient, topic, 0);

   if (lastResult != MQTT_OK)
   {
      tell(eloAlways, "Error: Subscribing to '%s' failed", topic);
      disconnect();
      return fail;
   }

   theTopic = topic;
   tell(eloDebug, "Debug: Subscribing to topic '%s' succeeded", topic);

   return success;
}

int Mqtt::unsubscribe(const char* topic)
{
   theTopic.clear();

   if (!isConnected() || isEmpty(topic))
      return success;

   lastResult = mqtt_unsubscribe(mqttClient, topic);

   if (lastResult != MQTT_OK)
   {
      tell(eloAlways, "Error: Unsubscribing to '%s' failed", topic);
      return fail;
   }

   tell(eloDebug, "Debug: Unsubscribing from topic '%s' succeeded", topic);

   return success;
}

//***************************************************************************
// Read
//***************************************************************************

int Mqtt::read(MemoryStruct* message, int timeoutMs)
{
   uint64_t endAt = cTimeMs::Now() + timeoutMs;
   int tmoMs;
   Message* msg {nullptr};

   message->clear();
   lastReadTopic.clear();

   while (receivedMessages.empty())
   {
      if (timeoutMs && cTimeMs::Now() >= endAt)
         return wrnTimeout;

      if (!timeoutMs)
         tmoMs = 10000;
      else
         tmoMs = std::max((int)(endAt-cTimeMs::Now()), 1);

      readMutex.Lock();
      readCond.TimedWait(readMutex, tmoMs);
      readMutex.Unlock();
   }

   {
      cMyMutexLock lock(&readMutex);
      msg = receivedMessages.front();
      receivedMessages.pop();
   }

   message->append(msg->payload.memory, msg->payload.size);
   message->append('\0');

   lastReadTopic = msg->topic;
   retained = msg->retained;
   delete msg;

   tell(eloDebug2, "Debug2: Got message from topic '%s'", lastReadTopic.c_str());

   return success;
}

//***************************************************************************
// Write
//***************************************************************************

int Mqtt::write(const char* topic, const char* message, int len)
{
   if (isEmpty(topic))
      return wrnEmptyMessage;

   if (!len && message)
      len = strlen(message);

   return write(topic, message, len, MQTT_PUBLISH_QOS_0);
}

int Mqtt::writeRetained(const char* topic, const char* message)
{
   if (isEmpty(topic))
   {
      tell(eloAlways, "Error: Aborting write, missing topic");
      return wrnEmptyMessage;
   }

   // we can also write NULL messages !!

   return write(topic, message, message ? strlen(message) : 0, MQTT_PUBLISH_RETAIN | MQTT_PUBLISH_QOS_0);
}

int Mqtt::write(const char* topic, const char* message, size_t len, uint8_t flags)
{
   if (!isConnected())
   {
      tell(eloAlways, "Error: MQTT: Can't write to topic '%s', not connected", topic);
      return fail;
   }

   theTopic.clear();

   lastResult = mqtt_publish(mqttClient, topic, message, len, flags);

   if (lastResult != MQTT_OK)
   {
      tell(eloAlways, "Error: Writing '%.*s' (%zd) to topic '%s' failed, result was %lu '%s'", (int)len,
           message, len, topic, lastResult, mqtt_error_str((MQTTErrors)lastResult));
      disconnect();
      return fail;
   }

   theTopic = topic;
   tell(eloMqtt, "-> (%s) [%s]", topic, message);

   return success;
}

#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

int Mqtt::openSocket(const char* addr, int port)
{
   struct addrinfo hints = {0};
   int sockfd = -1;
   int rv;
   struct addrinfo *p, *servinfo;
   char service[100];

   hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
   hints.ai_socktype = SOCK_STREAM;  // Must be TCP
   sprintf(service, "%d", port);

   // get address information

   rv = getaddrinfo(addr, service, &hints, &servinfo);

   if (rv != 0)
      return -1;

   //  open first possible socket

   for (p = servinfo; p != NULL; p = p->ai_next)
   {
      sockfd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);

      if (sockfd == -1)
         continue;

      rv = ::connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);

      if (rv == -1)
         continue;

      break;
   }

   freeaddrinfo(servinfo);

   // #TODO don't set O_NONBLOCK

   if (sockfd != -1)
      fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);

   return sockfd;
}
