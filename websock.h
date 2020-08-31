//***************************************************************************
// Web Socket Interface
// File websock.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 16.04.2020 - Jörg Wendel
//***************************************************************************

#pragma once

//***************************************************************************
// Includes
//***************************************************************************

#include <queue>
#include <jansson.h>
#include <libwebsockets.h>

//***************************************************************************
// Class Web Service
//***************************************************************************

class cWebService
{
   public:

      enum Event
      {
         evUnknown,
         evLogin,
         evLogout,
         evToggleIo,
         evToggleIoNext,
         evToggleMode,
         evStoreConfig,
         evGetToken,
         evStoreIoSetup,
         evChartData,
         evLogMessage,
         evUserConfig,
         evChangePasswd,
         evResetPeaks,
         evGroupConfig,

         evCount
      };

      enum ClientType
      {
         ctWithLogin = 0,
         ctActive
      };

      enum UserRights
      {
         urView        = 0x01,
         urControl     = 0x02,
         urFullControl = 0x04,
         urSettings    = 0x08,
         urAdmin       = 0x10
      };

      static const char* toName(Event event);
      static Event toEvent(const char* name);

      static const char* events[];
};

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
         ClientType type;
         int tftprio;
         std::queue<std::string> messagesOut;
         cMyMutex messagesOutMutex;
         void* wsi;

         void pushMessage(const char* p)
         {
            cMyMutexLock lock(&messagesOutMutex);
            messagesOut.push(p);
         }

         void cleanupMessageQueue()
         {
            cMyMutexLock lock(&messagesOutMutex);

            // just in case client is not connected and wasted messages are pending

            tell(0, "Info: Flushing (%zu) old 'wasted' messages of client (%p)", messagesOut.size(), wsi);

            while (!messagesOut.empty())
               messagesOut.pop();
         }

         std::string buffer;   // for chunked messages
      };

      cWebSock(cWebInterface* aProcess, const char* aHttpPath);
      virtual ~cWebSock();

      int init(int aPort, int aTimeout);
      int exit();

      int service();
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
      lws_protocols protocols[3];
      lws_http_mount mounts[1];
#if defined (LWS_LIBRARY_VERSION_MAJOR) && (LWS_LIBRARY_VERSION_MAJOR >= 4)
      lws_retry_bo_t retry;
#endif

      // statics

      static lws_context* context;
      static cWebInterface* singleton;
      static char* httpPath;
      static char* epgImagePath;
      static int timeout;
      static std::map<void*,Client> clients;
      static cMyMutex clientsMutex;
      static MsgType msgType;

      // only used in callback

      static char* msgBuffer;
      static int msgBufferSize;
};
