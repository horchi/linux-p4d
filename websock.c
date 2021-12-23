/**
 *  websock.c
 *
 *  (c) 2017-2021 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#include <jansson.h>
#include <dirent.h>

#include "lib/json.h"

#include "websock.h"

// LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG | LLL_PARSER |
// LLL_HEADER | LLL_EXT | LLL_CLIENT | LLL_LATENCY | LLL_USER | LLL_THREAD

int cWebSock::wsLogLevel {LLL_ERR | LLL_WARN};
lws_context* cWebSock::context {nullptr};
int cWebSock::timeout {0};
cWebSock::MsgType cWebSock::msgType {mtNone};
std::map<void*,cWebSock::Client> cWebSock::clients;
std::map<std::string, std::string> cWebSock::htmlTemplates;
cMyMutex cWebSock::clientsMutex;
char* cWebSock::httpPath {nullptr};

cWebInterface* cWebSock::singleton {nullptr};

//***************************************************************************
// Web Socket
//***************************************************************************

cWebSock::cWebSock(cWebInterface* aProcess, const char* aHttpPath)
{
   cWebSock::singleton = aProcess;
   httpPath = strdup(aHttpPath);
}

cWebSock::~cWebSock()
{
   exit();
   free(certFile);
   free(certKeyFile);
   free(httpPath);
}

int cWebSock::init(int aPort, int aTimeout, const char* confDir, bool ssl)
{
   lws_context_creation_info info {0};

   lws_set_log_level(wsLogLevel, writeLog);

   port = aPort;
   timeout = aTimeout;

   // setup websocket protocol

   protocols[0].name = "http-only";
   protocols[0].callback = callbackHttp;
   protocols[0].per_session_data_size = sizeof(SessionData);
   protocols[0].rx_buffer_size = 0;

   protocols[1].name = singleton->myName();
   protocols[1].callback = callbackWs;
   protocols[1].per_session_data_size = 0;
   protocols[1].rx_buffer_size = 128*1024;
   protocols[1].tx_packet_size = 0;

   protocols[2].name = 0;
   protocols[2].callback = 0;
   protocols[2].per_session_data_size = 0;
   protocols[2].rx_buffer_size = 0;

   // mounts

   bool noStore {false};       // for debugging on iOS
   lws_http_mount mount {0};

   mount.mount_next = (lws_http_mount*)nullptr;
   mount.mountpoint = "/";
   mount.origin = httpPath;
   mount.mountpoint_len = 1;
   mount.cache_max_age = noStore ? 0 : 86400;
   mount.cache_reusable = !noStore;       // 0 => no-store
   mount.cache_revalidate = 1;
   mount.cache_intermediaries = 1;

#ifdef LWS_FEATURE_MOUNT_NO_CACHE
   mount.cache_no = noStore ? 0 : 1;
#endif
   mount.origin_protocol = LWSMPRO_FILE;
   mount.basic_auth_login_file = nullptr;
   mounts[0] = mount;

   // setup websocket context info

   memset(&info, 0, sizeof(info));
   info.options = 0;
   info.mounts = mounts;
   info.gid = -1;
   info.uid = -1;
   info.port = port;
   info.protocols = protocols;
#if defined (LWS_LIBRARY_VERSION_MAJOR) && (LWS_LIBRARY_VERSION_MAJOR < 4)
   info.iface = nullptr;
#endif
   info.ssl_cert_filepath = nullptr;
   info.ssl_private_key_filepath = nullptr;

   if (ssl)
   {
      free(certFile);
      free(certKeyFile);
      asprintf(&certFile, "%s/" TARGET ".cert", confDir);
      asprintf(&certKeyFile, "%s/" TARGET ".key", confDir);
      tell(eloAlways, "Starting SSL mode with '%s' / '%s'", certFile, certKeyFile);

      info.ssl_cert_filepath = certFile;
      info.ssl_private_key_filepath = certKeyFile;
      info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT; // | LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
   }

#if defined (LWS_LIBRARY_VERSION_MAJOR) && (LWS_LIBRARY_VERSION_MAJOR >= 4)
   retry.secs_since_valid_ping = timeout;
   retry.secs_since_valid_hangup = timeout + 10;
   info.retry_and_idle_policy = &retry;
#else
   info.ws_ping_pong_interval = timeout;
#endif

   // create libwebsocket context representing this server

   context = lws_create_context(&info);

   if (!context)
   {
      tell(eloAlways, "Error: libwebsocket init failed");
      return fail;
   }

   tell(eloAlways, "WebSocket Listener at port (%d) established", port);
   tell(eloDetail, "Using libwebsocket version '%s'", lws_get_library_version());

   threadCtl.webSock = this;
   threadCtl.timeout = timeout;

   if (pthread_create(&syncThread, NULL, syncFct, &threadCtl))
   {
      tell(eloAlways, "Error: Failed to start client daemon thread");
      return fail;
   }

   return success;
}

void cWebSock::writeLog(int level, const char* line)
{
   std::string message = strReplace("\n", "", line);
   tell(eloAlways, "WS: (%d) %s", level, message.c_str());
}

int cWebSock::exit()
{
   if (syncThread)
   {
      threadCtl.close = true;
      lws_cancel_service(context);

      time_t endWait = time(0) + 2;  // 2 second for regular end

      while (threadCtl.active && time(0) < endWait)
         usleep(100);

      if (threadCtl.active)
         pthread_cancel(syncThread);
      else
         pthread_join(syncThread, 0);

      syncThread = 0;
   }

   lws_context_destroy(context);

   return success;
}

//***************************************************************************
// Sync
//***************************************************************************

void* cWebSock::syncFct(void* user)
{
   ThreadControl* threadCtl = (ThreadControl*)user;
   threadCtl->active = true;

   tell(eloDebugWebSock, " :: started syncThread");

   while (!threadCtl->close)
   {
      service(threadCtl);
   }

   threadCtl->active = false;
   // printf(" :: stopped syncThread regular\n");

   return nullptr;
}

//***************************************************************************
// Service
//***************************************************************************

int cWebSock::service(ThreadControl* threadCtl)
{
   static time_t nextWebSocketPing {0};

#if defined (LWS_LIBRARY_VERSION_MAJOR) && (LWS_LIBRARY_VERSION_MAJOR >= 4)
   lws_service(context, 0);   // timeout parameter is not supported by the lib anymore
#else
   lws_service(context, 100);
#endif

   threadCtl->webSock->performData(cWebSock::mtData);

   if (nextWebSocketPing < time(0))
   {
      threadCtl->webSock->performData(cWebSock::mtPing);
      nextWebSocketPing = time(0) + threadCtl->timeout-5;
   }

   return done;
}

//***************************************************************************
// Perform Data
//***************************************************************************

int cWebSock::performData(MsgType type)
{
   int count {0};

   cMyMutexLock lock(&clientsMutex);

   msgType = type;

   for (auto it = clients.begin(); it != clients.end(); ++it)
   {
      if (!it->second.messagesOut.empty())
         count++;
   }

   if (count || msgType == mtPing)
      lws_callback_on_writable_all_protocol(context, &protocols[1]);

   return done;
}

//***************************************************************************
// Get Client Info of connection
//***************************************************************************

void getClientInfo(lws* wsi, std::string* clientInfo)
{
   char clientName[100+TB] = "unknown";
   // char clientIp[50+TB] = "";

   if (wsi)
   {
      // int fd {0};
      // lws_get_peer_addresses take up to 10 seconds on some environments !!
      // if ((fd = lws_get_socket_fd(wsi)))
      //    lws_get_peer_addresses(wsi, fd, clientName, sizeof(clientName), clientIp, sizeof(clientIp));

      // we can use lws_get_peer_simple instead

      lws_get_peer_simple(wsi, clientName, sizeof(clientName));
   }

   *clientInfo = clientName; //  + std::string("/") + clientIp;
}

//***************************************************************************
// HTTP Callback
//***************************************************************************

int cWebSock::callbackHttp(lws* wsi, lws_callback_reasons reason, void* user, void* in, size_t len)
{
   SessionData* sessionData = (SessionData*)user;
   std::string clientInfo = "unknown";

   tell(eloDebugWebSock, "DEBUG: 'callbackHttp' got (%d)", reason);

   switch (reason)
   {
      case LWS_CALLBACK_CLOSED:
      {
         getClientInfo(wsi, &clientInfo);

         tell(eloDebugWebSock, "DEBUG: Got unecpected LWS_CALLBACK_CLOSED for client '%s' (%p)",
              clientInfo.c_str(), (void*)wsi);
         break;
      }
      case LWS_CALLBACK_HTTP_BODY:
      {
         const char* message = (const char*)in;
         int s = lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI);

         tell(eloDebugWebSock, "DEBUG: Got unecpected LWS_CALLBACK_HTTP_BODY with [%.*s] lws_hdr_total_length is (%d)",
              (int)len+1, message, s);
         break;
      }

      case LWS_CALLBACK_CLIENT_WRITEABLE:
      {
         tell(eloDebugWebSock, "HTTP: Client writeable");
         break;
      }

      case LWS_CALLBACK_HTTP_WRITEABLE:
      {
         int res;

         tell(eloDebugWebSock, "HTTP: LWS_CALLBACK_HTTP_WRITEABLE");

         // data to write?

         if (!sessionData->dataPending)
         {
            tell(eloDebugWebSock, "Info: No more session data pending");
            return -1;
         }

         int m = lws_get_peer_write_allowance(wsi);

         if (!m)
            tell(eloDebugWebSock, "Right now, peer can't handle anything :o");
         else if (m != -1 && m < sessionData->payloadSize)
            tell(eloAlways, "Peer can't handle %d but %d is needed", m, sessionData->payloadSize);
         else if (m != -1)
            tell(eloDebugWebSock, "All fine, peer can handle %d bytes", m);

         res = lws_write(wsi, (unsigned char*)sessionData->buffer+sizeLwsPreFrame,
                         sessionData->payloadSize, LWS_WRITE_HTTP);

         if (res < 0)
            tell(eloAlways, "Failed writing '%s'", sessionData->buffer+sizeLwsPreFrame);
         else
            tell(eloDebugWebSock, "WROTE '%s' (%d)", sessionData->buffer+sizeLwsPreFrame, res);

         free(sessionData->buffer);
         memset(sessionData, 0, sizeof(SessionData));

         if (lws_http_transaction_completed(wsi))
            return -1;

         break;
      }

      case LWS_CALLBACK_HTTP:
      {
         int res;
         char* file {nullptr};
         const char* url = (char*)in;

         memset(sessionData, 0, sizeof(SessionData));

         tell(eloWebSock, "HTTP: Requested url (%ld) '%s'", (ulong)len, url);

         // data or file request ...

         if (strncmp(url, "/data/", 6) == 0)
         {
            // data request

            tell(eloAlways, "Got unexpected HTTP request!");
            res = dispatchDataRequest(wsi, sessionData, url);

            if (res < 0 || (res > 0 && lws_http_transaction_completed(wsi)))
               return -1;
         }
         else
         {
            // file request

            if (strcmp(url, "/") == 0)
               url = "index.html";

            // security, force httpPath path to inhibit access to the whole filesystem

            asprintf(&file, "%s/%s", httpPath, url);
            res = serveFile(wsi, file);
            free(file);

            if (res < 0)
            {
               tell(eloAlways, "HTTP: Failed to serve url '%s' (%d)", url, res);
               return -1;
            }

            tell(eloWebSock, "HTTP: Done, url: '%s'", url);
         }

         break;
      }

      case LWS_CALLBACK_HTTP_FILE_COMPLETION:     // 15
      {
         if (lws_http_transaction_completed(wsi))
            return -1;

         break;
      }

      case LWS_CALLBACK_PROTOCOL_INIT:
      case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
      case LWS_CALLBACK_FILTER_HTTP_CONNECTION:
      case LWS_CALLBACK_CLOSED_HTTP:
      case LWS_CALLBACK_WSI_CREATE:
      case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
      case LWS_CALLBACK_ADD_POLL_FD:
      case LWS_CALLBACK_DEL_POLL_FD:
      case LWS_CALLBACK_WSI_DESTROY:
      case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
      case LWS_CALLBACK_LOCK_POLL:
      case LWS_CALLBACK_UNLOCK_POLL:
      case LWS_CALLBACK_GET_THREAD_ID:
      case LWS_CALLBACK_HTTP_BIND_PROTOCOL:       // 49
      case LWS_CALLBACK_HTTP_DROP_PROTOCOL:       // 50
#if LWS_LIBRARY_VERSION_MAJOR >= 3
      case LWS_CALLBACK_HTTP_CONFIRM_UPGRADE:
      case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
      case LWS_CALLBACK_HTTP_BODY_COMPLETION:
#endif
         break;
      case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS: // 21,
      case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS: // 22,
         break;

      default:
         tell(eloDetail, "DEBUG: Unhandled 'callbackHttp' got (%d)", reason);
         break;
   }

   return 0;
}

//***************************************************************************
// Serve File
//***************************************************************************

int cWebSock::serveFile(lws* wsi, const char* path)
{
   const char* suffix = suffixOf(path ? path : "");
   const char* mime = "text/plain";

   // LogDuration ld("serveFile", 1);

   // choose mime type based on the file extension

   if (!isEmpty(suffix))
   {
      if      (strcmp(suffix, "png") == 0)   mime = "image/png";
      else if (strcmp(suffix, "jpg") == 0)   mime = "image/jpeg";
      else if (strcmp(suffix, "jpeg") == 0)  mime = "image/jpeg";
      else if (strcmp(suffix, "gif") == 0)   mime = "image/gif";
      else if (strcmp(suffix, "svg") == 0)   mime = "image/svg+xml";
      else if (strcmp(suffix, "html") == 0)  mime = "text/html";
      else if (strcmp(suffix, "css") == 0)   mime = "text/css";
      else if (strcmp(suffix, "js") == 0)    mime = "application/javascript";
      else if (strcmp(suffix, "map") == 0)   mime = "application/json";
   }

   // printf("serve file '%s' with mime type '%s'\n", path, mime);

   return lws_serve_http_file(wsi, path, mime, 0, 0);
}

//***************************************************************************
// Callback for my Protocol
//***************************************************************************

int cWebSock::callbackWs(lws* wsi, lws_callback_reasons reason, void* user, void* in, size_t len)
{
   std::string clientInfo = "unknown";

   tell(eloDebugWebSock, "DEBUG: 'callback' got (%d)", reason);

   switch (reason)
   {
      case LWS_CALLBACK_SERVER_WRITEABLE:
      case LWS_CALLBACK_RECEIVE:
      case LWS_CALLBACK_ESTABLISHED:
      case LWS_CALLBACK_CLOSED:
      case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
      case LWS_CALLBACK_RECEIVE_PONG:
      {
         getClientInfo(wsi, &clientInfo);
         break;
      }

      default: break;
   }

   switch (reason)
   {
      case LWS_CALLBACK_SERVER_WRITEABLE:                     // data to client
      {
         if (msgType == mtPing)
         {
            char buffer[sizeLwsFrame];
            tell(eloDebugWebSock, "DEBUG: Write 'PING' to '%s' (%p)", clientInfo.c_str(), (void*)wsi);
            lws_write(wsi, (unsigned char*)buffer + sizeLwsPreFrame, 0, LWS_WRITE_PING);
         }

         else if (msgType == mtData)
         {
            if (!clients[wsi].msgBufferDataPending() && clients[wsi].messagesOut.empty())
               return 0;
            if (lws_send_pipe_choked(wsi))
               return 0;

            if (!clients[wsi].msgBufferDataPending() && !clients[wsi].messagesOut.empty())
            {
               cMyMutexLock clock(&clientsMutex);
               cMyMutexLock lock(&clients[wsi].messagesOutMutex);
               const char* msg = clients[wsi].messagesOut.front().c_str();
               int msgSize = clients[wsi].messagesOut.front().length();
               int neededSize = sizeLwsFrame + msgSize;
               unsigned char* newBuffer {nullptr};

               if (neededSize > clients[wsi].msgBufferSize)
               {
                  // tell(eloDebugWebSock, "Debug: re-allocate buffer to %d bytes", neededSize);

                  if (!(newBuffer = (unsigned char*)realloc(clients[wsi].msgBuffer, neededSize)))
                  {
                     tell(eloDebugWebSock, "Fatal: Can't allocate memory!");
                     return -1;
                  }

                  clients[wsi].msgBuffer = newBuffer;
                  clients[wsi].msgBufferSize = neededSize;
               }

               strncpy((char*)clients[wsi].msgBuffer + sizeLwsPreFrame, msg, msgSize);
               clients[wsi].msgBufferPayloadSize = msgSize;
               clients[wsi].msgBufferSendOffset = 0;
               clients[wsi].messagesOut.pop();  // remove sent message

               tell(eloWebSock, "=> (%d) %.*s -> to '%s' (%p)", msgSize, msgSize,
                    clients[wsi].msgBuffer+sizeLwsPreFrame, clientInfo.c_str(), (void*)wsi);
            }

            enum { maxChunk = 10*1024 };

            if (clients[wsi].msgBufferPayloadSize)
            {
               unsigned char* p = clients[wsi].msgBuffer + sizeLwsPreFrame + clients[wsi].msgBufferSendOffset;
               int pending = clients[wsi].msgBufferPayloadSize - clients[wsi].msgBufferSendOffset;
               int chunkSize = pending > maxChunk ? maxChunk : pending;
               int flags = lws_write_ws_flags(LWS_WRITE_TEXT, !clients[wsi].msgBufferSendOffset, pending <= maxChunk);
               int res = lws_write(wsi, p, chunkSize, (lws_write_protocol)flags);

               if (res < 0)
               {
                  tell(eloAlways, "Error: lws_write chunk failed with (%d) to (%p) failed (%d) [%.*s]", res, (void*)wsi, chunkSize, chunkSize, p);
                  return -1;
               }

               clients[wsi].msgBufferSendOffset += chunkSize;

               if (clients[wsi].msgBufferSendOffset >= clients[wsi].msgBufferPayloadSize)
               {
                  clients[wsi].msgBufferPayloadSize = 0;
                  clients[wsi].msgBufferSendOffset = 0;
               }
               else
                  lws_callback_on_writable(wsi);
            }
         }

         break;
      }

      case LWS_CALLBACK_RECEIVE:                           // data from client
      {
         json_t *oData, *oObject;
         json_error_t error;
         Event event;

         tell(eloDebugWebSock, "DEBUG: 'LWS_CALLBACK_RECEIVE' [%.*s]", (int)len, (const char*)in);

         {
            cMyMutexLock lock(&clientsMutex);

            if (lws_is_first_fragment(wsi))
               clients[wsi].buffer.clear();

            clients[wsi].buffer.append((const char*)in, len);

            if (!lws_is_final_fragment(wsi))
            {
               tell(eloDebugWebSock, "DEBUG: Got %zd bytes, have now %zd -> more to get", len, clients[wsi].buffer.length());
               break;
            }

            oData = json_loadb(clients[wsi].buffer.c_str(), clients[wsi].buffer.length(), 0, &error);

            if (!oData)
            {
               tell(eloAlways, "Error: Ignoring invalid jason request [%s]", clients[wsi].buffer.c_str());
               tell(eloAlways, "Error decoding json: %s (%s, line %d column %d, position %d)",
                    error.text, error.source, error.line, error.column, error.position);
               break;
            }

            clients[wsi].buffer.clear();
         }

         char* message = strndup((const char*)in, len);
         const char* strEvent = getStringFromJson(oData, "event", "<null>");
         event = cWebService::toEvent(strEvent);
         oObject = json_object_get(oData, "object");

         tell(eloDebugWebSock, "GOT '%s'", message);

         if (event == evLogin)                              // { "event" : "login", "object" : { "type" : "foo" } }
         {
            atLogin(wsi, message, clientInfo.c_str(), oObject);
         }
         else if (event == evLogout)                         // { "event" : "logout", "object" : { } }
         {
            clients[wsi].cleanupMessageQueue();
         }
         else if (event == evGetToken)                       // { "event" : "gettoken", "object" : { "user" : "" : "password" : md5 } }
         {
            addToJson(oData, "client", (long)wsi);
            char* p = json_dumps(oData, 0);
            singleton->pushInMessage(p);
            free(p);
         }
         else if (event == evLogMessage)                     // { "event" : "logmessage", "object" : { "message" : "....." } }
         {
            tell(eloAlways, "Browser message: '%s'", getStringFromJson(oObject, "message", "<null>"));
         }
         else //  if (clients[wsi].type == ctWithLogin)
         {
            addToJson(oData, "client", (long)wsi);
            char* p = json_dumps(oData, 0);
            singleton->pushInMessage(p);
            free(p);
         }
/*         else
         {
            tell(eloDebugWebSock, "Debug: Ignoring '%s' request of client (%p) without login [%s]", strEvent, (void*)wsi, message);
            } */

         json_decref(oData);
         free(message);

         break;
      }

      case LWS_CALLBACK_ESTABLISHED:                       // someone connecting
      {
         tell(eloWebSock, "Client '%s' connected (%p), ping time set to (%d)", clientInfo.c_str(), (void*)wsi, timeout);
         clients[wsi].wsi = wsi;
         clients[wsi].type = ctActive;
         clients[wsi].tftprio = 100;

         break;
      }

      case LWS_CALLBACK_CLOSED:                            // someone dis-connecting
      {
         atLogout(wsi, "Client disconnected", clientInfo.c_str());
         break;
      }

      case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
      {
         atLogout(wsi, "Client disconnected unsolicited", clientInfo.c_str());
         break;
      }

      case LWS_CALLBACK_RECEIVE_PONG:                      // ping / pong
      {
         tell(eloDebugWebSock, "DEBUG: Got 'PONG' from client '%s' (%p)", clientInfo.c_str(), (void*)wsi);
         break;
      }

      case LWS_CALLBACK_ADD_HEADERS:
      case LWS_CALLBACK_PROTOCOL_INIT:
      case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
#if LWS_LIBRARY_VERSION_MAJOR >= 3
      case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
      case LWS_CALLBACK_HTTP_BIND_PROTOCOL:
#endif
         break;

      default:
         tell(eloDebugWebSock, "DEBUG: Unhandled 'callbackWs' got (%d)", reason);
         break;
   }

   return 0;
}

//***************************************************************************
// At Login / Logout
//***************************************************************************

void cWebSock::atLogin(lws* wsi, const char* message, const char* clientInfo, json_t* object)
{
   tell(eloWebSock, "Client login '%s' (%p) [%s]", clientInfo, (void*)wsi, message);

   {
      cMyMutexLock lock(&clientsMutex);
      clients[wsi].type = ctActive;
   }

   json_t* obj = json_object();
   addToJson(obj, "event", "login");
   json_object_set_new(obj, "object", object);
   addToJson(object, "client", (long)wsi);

   char* p = json_dumps(obj, 0);

   singleton->pushInMessage(p);
   free(p);
}

void cWebSock::atLogout(lws* wsi, const char* message, const char* clientInfo)
{
   tell(eloDebugWebSock, "%s '%s' (%p)", clientInfo, message, (void*)wsi);

   auto it = clients.find(wsi);

   if (it == clients.end())
      return ;

   json_t* object = json_object();
   addToJson(object, "client", (long)wsi);

   json_t* obj = json_object();
   addToJson(obj, "event", "logout");
   json_object_set_new(obj, "object", object);

   char* p = json_dumps(obj, 0);
   json_decref(obj);
   singleton->pushInMessage(p);
   free(p);

   {
      cMyMutexLock lock(&clientsMutex);
      clients.erase(wsi);
   }
}

//***************************************************************************
// Client Count
//***************************************************************************

int cWebSock::getClientCount()
{
   int count = 0;

   cMyMutexLock lock(&clientsMutex);

   for (auto it = clients.begin(); it != clients.end(); ++it)
      count++;

   return count;
}

//***************************************************************************
// Set Client Type
//***************************************************************************

void cWebSock::setClientType(lws* wsi, ClientType type)
{
   cMyMutexLock lock(&clientsMutex);
   clients[wsi].type = type;
}

//***************************************************************************
// Push Message
//***************************************************************************

void cWebSock::pushOutMessage(const char* message, lws* wsi)
{
   cMyMutexLock lock(&clientsMutex);

   if (wsi)
   {
      if (clients.find(wsi) != clients.end())
         clients[wsi].pushMessage(message);
      else if ((ulong)wsi != (ulong)-1)
         tell(eloAlways, "client %ld not found!", (ulong)wsi);
   }
   else
   {
      for (auto it = clients.begin(); it != clients.end(); ++it)
         it->second.pushMessage(message);
   }
}

//***************************************************************************
// Get Integer Parameter
//***************************************************************************

int cWebSock::getIntParameter(lws* wsi, const char* name, int def)
{
   char buf[100];
   const char* arg = lws_get_urlarg_by_name(wsi, name, buf, 100);

   return arg ? atoi(arg) : def;
}

//***************************************************************************
// Get String Parameter
//***************************************************************************

const char* cWebSock::getStrParameter(lws* wsi, const char* name, const char* def)
{
   static char buf[512+TB];
   const char* arg = lws_get_urlarg_by_name(wsi, name, buf, 512);

   return arg ? arg : def;
}

//***************************************************************************
// Method Of
//***************************************************************************

const char* cWebSock::methodOf(const char* url)
{
   const char* p;

   if (url && (p = strchr(url+1, '/')))
      return p+1;

   return "";
}

//***************************************************************************
// Dispatch Data Request
//***************************************************************************

int cWebSock::dispatchDataRequest(lws* wsi, SessionData* sessionData, const char* url)
{
   int statusCode = HTTP_STATUS_NOT_FOUND;

   // const char* method = methodOf(url);

   // if (strcmp(method, "getenv") == 0)
   //    statusCode = doEnvironment(wsi, sessionData);

   return statusCode;
}
