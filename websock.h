//***************************************************************************
// Web Socket Interface
// File websock.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 16.04.2020 - Jörg Wendel
//***************************************************************************

#pragma once

#include <queue>
#include <jansson.h>
#include <libwebsockets.h>

#include "webservice.h"

//***************************************************************************
// Class Web Interface
//***************************************************************************

class cWebInterface : public cWebService
{
   public:

      virtual const char* myName() = 0;
      virtual int pushInMessage(const char* data) = 0;
};

//***************************************************************************
// Class cWebSock
//***************************************************************************

class cWebSock : public cWebService
{
   public:

      enum MsgType
      {
         mtNone = na,
         mtPing,    // 0
         mtData     // 1
      };

      enum Protokoll
      {
         sizeLwsPreFrame  = LWS_SEND_BUFFER_PRE_PADDING,
         sizeLwsPostFrame = LWS_SEND_BUFFER_POST_PADDING,
         sizeLwsFrame     = sizeLwsPreFrame + sizeLwsPostFrame
      };

      struct SessionData
      {
         char* buffer;
         int bufferSize;
         int payloadSize;
         int dataPending;
      };

      struct Client
      {
         ~Client() { free(msgBuffer); }

         ClientType type;
         int tftprio;
         std::queue<std::string> messagesOut;
         cMyMutex messagesOutMutex;
         void* wsi;

         // buffer to send the payload in chunks

         unsigned char* msgBuffer {nullptr};
         int msgBufferSize {0};
         int msgBufferPayloadSize {0};
         int msgBufferSendOffset {0};
         bool msgBufferDataPending() { return msgBufferSendOffset < msgBufferPayloadSize; }

         // push next message

         void pushMessage(const char* p)
         {
            cMyMutexLock lock(&messagesOutMutex);
            messagesOut.push(p);
         }

         void cleanupMessageQueue()
         {
            cMyMutexLock lock(&messagesOutMutex);

            // just in case client is not connected and wasted messages are pending

            tell(eloAlways, "Info: Flushing (%zu) old 'wasted' messages of client (%p)", messagesOut.size(), wsi);

            while (!messagesOut.empty())
               messagesOut.pop();
         }

         std::string buffer;   // for chunked messages
      };

      cWebSock(cWebInterface* aProcess, const char* aHttpPath);
      virtual ~cWebSock();

      int init(int aPort, int aTimeout, const char* confDir, bool ssl = false);
      int exit();

      int performData(MsgType type);

      // static interface

      void pushOutMessage(const char* p, lws* wsi = 0);
      void setClientType(lws* wsi, ClientType type);

   private:

      // callback methods

      static int wsLogLevel;
      static int callbackHttp(lws* wsi, lws_callback_reasons reason, void* user, void* in, size_t len);
      static int callbackWs(lws* wsi, lws_callback_reasons reason, void* user, void* in, size_t len);

      // static interface for callbacks

      static void atLogin(lws* wsi, const char* message, const char* clientInfo, json_t* object);
      static void atLogout(lws* wsi, const char* message, const char* clientInfo);
      static int getClientCount();
      static void writeLog(int level, const char* line);

      static int serveFile(lws* wsi, const char* path);
      static int dispatchDataRequest(lws* wsi, SessionData* sessionData, const char* url);

      static const char* methodOf(const char* url);
      static const char* getStrParameter(lws* wsi, const char* name, const char* def = 0);
      static int getIntParameter(lws* wsi, const char* name, int def = na);

      //

      int port {na};
      char* certFile {nullptr};
      char* certKeyFile {nullptr};
      lws_protocols protocols[3];
      lws_http_mount mounts[1];
#if defined (LWS_LIBRARY_VERSION_MAJOR) && (LWS_LIBRARY_VERSION_MAJOR >= 4)
      lws_retry_bo_t retry;
#endif

      // sync thread stuff

      struct ThreadControl
      {
         bool active {false};
         bool close {false};
         cWebSock* webSock {nullptr};
         int timeout {60};
      };

      pthread_t syncThread {0};
      ThreadControl threadCtl;

      static void* syncFct(void* user);
      static int service(ThreadControl* threadCtl);

      // statics

      static lws_context* context;
      static cWebInterface* singleton;
      static char* httpPath;
      static char* epgImagePath;
      static int timeout;
      static std::map<void*,Client> clients;
      static cMyMutex clientsMutex;
      static MsgType msgType;
      static std::map<std::string, std::string> htmlTemplates;
};
