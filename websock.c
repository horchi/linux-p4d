/**
 *  websock.c
 *
 *  (c) 2017-2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#include <jansson.h>
#include <dirent.h>

#include "lib/json.h"

#include "websock.h"

int cWebSock::wsLogLevel {LLL_ERR | LLL_WARN}; // LLL_INFO | LLL_NOTICE | LLL_WARN | LLL_ERR
lws_context* cWebSock::context {nullptr};
char* cWebSock::msgBuffer {nullptr};
int cWebSock::msgBufferSize {0};
int cWebSock::timeout {0};
cWebSock::MsgType cWebSock::msgType {mtNone};
std::map<void*,cWebSock::Client> cWebSock::clients;
cMyMutex cWebSock::clientsMutex;
char* cWebSock::httpPath {nullptr};

cWebInterface* cWebSock::singleton {nullptr};

//***************************************************************************
// Init
//***************************************************************************

cWebSock::cWebSock(cWebInterface* aProcess, const char* aHttpPath)
{
   cWebSock::singleton = aProcess;
   httpPath = strdup(aHttpPath);
}

cWebSock::~cWebSock()
{
   exit();

   free(httpPath);
   free(msgBuffer);
}

int cWebSock::init(int aPort, int aTimeout)
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
   protocols[1].rx_buffer_size = 80*1024;

   protocols[2].name = 0;
   protocols[2].callback = 0;
   protocols[2].per_session_data_size = 0;
   protocols[2].rx_buffer_size = 0;

   // mounts

   lws_http_mount mount {0};

   mount.mount_next = (lws_http_mount*)nullptr;
   mount.mountpoint = "/";
   mount.origin = httpPath;
   mount.mountpoint_len = 1;
   mount.cache_max_age = true ? 86400 : 604800;
   mount.cache_reusable = 1;
   mount.cache_revalidate = true ? 1 : 0;
   mount.cache_intermediaries = 1;
   mount.origin_protocol = LWSMPRO_FILE;
   mounts[0] = mount;

   // setup websocket context info

   memset(&info, 0, sizeof(info));
   info.port = port;
   info.protocols = protocols;
#if defined (LWS_LIBRARY_VERSION_MAJOR) && (LWS_LIBRARY_VERSION_MAJOR < 4)
   info.iface = nullptr;
#endif
   info.ssl_cert_filepath = nullptr;
   info.ssl_private_key_filepath = nullptr;

#if defined (LWS_LIBRARY_VERSION_MAJOR) && (LWS_LIBRARY_VERSION_MAJOR >= 4)
   retry.secs_since_valid_ping = timeout;
   retry.secs_since_valid_hangup = timeout + 10;
   info.retry_and_idle_policy = &retry;
#else
   info.ws_ping_pong_interval = timeout;
#endif

   info.gid = -1;
   info.uid = -1;
   info.options = 0;
   info.mounts = mounts;

   // create libwebsocket context representing this server

   context = lws_create_context(&info);

   if (!context)
   {
      tell(0, "Error: libwebsocket init failed");
      return fail;
   }

   tell(0, "Listener at port (%d) established", port);
   tell(1, "using libwebsocket version '%s'", lws_get_library_version());

   return success;
}

void cWebSock::writeLog(int level, const char* line)
{
   if (wsLogLevel & level)
      tell(2, "WS: %s", line);
}

int cWebSock::exit()
{
   // lws_context_destroy(context);  #TODO ?

   return success;
}

//***************************************************************************
// Service
//***************************************************************************

int cWebSock::service()
{
#if defined (LWS_LIBRARY_VERSION_MAJOR) && (LWS_LIBRARY_VERSION_MAJOR >= 4)
   lws_service(context, 0);    // timeout parameter is not supported by the lib anymore
#else
   lws_service(context, 100);
#endif

   return done;
}

//***************************************************************************
// Perform Data
//***************************************************************************

int cWebSock::performData(MsgType type)
{
   int count = 0;

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
   char clientIp[50+TB] = "";

   if (wsi)
   {
      int fd;

      if ((fd = lws_get_socket_fd(wsi)))
         lws_get_peer_addresses(wsi, fd, clientName, sizeof(clientName), clientIp, sizeof(clientIp));
   }

   *clientInfo = clientName + std::string("/") + clientIp;
}

//***************************************************************************
// HTTP Callback
//***************************************************************************

int cWebSock::callbackHttp(lws* wsi, lws_callback_reasons reason, void* user, void* in, size_t len)
{
   SessionData* sessionData = (SessionData*)user;
   std::string clientInfo = "unknown";

   tell(4, "DEBUG: 'callbackHttp' got (%d)", reason);

   switch (reason)
   {
      case LWS_CALLBACK_CLOSED:
      {
         getClientInfo(wsi, &clientInfo);

         tell(2, "DEBUG: Got unecpected LWS_CALLBACK_CLOSED for client '%s' (%p)",
              clientInfo.c_str(), (void*)wsi);
         break;
      }
      case LWS_CALLBACK_HTTP_BODY:
      {
         const char* message = (const char*)in;
         int s = lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI);

         tell(1, "DEBUG: Got unecpected LWS_CALLBACK_HTTP_BODY with [%.*s] lws_hdr_total_length is (%d)",
              (int)len+1, message, s);
         break;
      }

      case LWS_CALLBACK_CLIENT_WRITEABLE:
      {
         tell(2, "HTTP: Client writeable");
         break;
      }

      case LWS_CALLBACK_HTTP_WRITEABLE:
      {
         int res;

         tell(3, "HTTP: LWS_CALLBACK_HTTP_WRITEABLE");

         // data to write?

         if (!sessionData->dataPending)
         {
            tell(1, "Info: No more session data pending");
            return -1;
         }

         int m = lws_get_peer_write_allowance(wsi);

         if (!m)
            tell(3, "right now, peer can't handle anything :o");
         else if (m != -1 && m < sessionData->payloadSize)
            tell(0, "peer can't handle %d but %d is needed", m, sessionData->payloadSize);
         else if (m != -1)
            tell(3, "all fine, peer can handle %d bytes", m);

         res = lws_write(wsi, (unsigned char*)sessionData->buffer+sizeLwsPreFrame,
                         sessionData->payloadSize, LWS_WRITE_HTTP);

         if (res < 0)
            tell(0, "Failed writing '%s'", sessionData->buffer+sizeLwsPreFrame);
         else
            tell(2, "WROTE '%s' (%d)", sessionData->buffer+sizeLwsPreFrame, res);

         free(sessionData->buffer);
         memset(sessionData, 0, sizeof(SessionData));

         if (lws_http_transaction_completed(wsi))
            return -1;

         break;
      }

      case LWS_CALLBACK_HTTP:
      {
         int res;
         char* file = 0;
         const char* url = (char*)in;

         memset(sessionData, 0, sizeof(SessionData));

         tell(1, "HTTP: Requested uri: (%ld) '%s'", (ulong)len, url);

         // data or file request ...

         if (strncmp(url, "/data/", 6) == 0)
         {
            // data request

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
               tell(2, "HTTP: Failed, uri: '%s' (%d)", url, res);
               return -1;
            }

            tell(3, "HTTP: Done, uri: '%s'", url);
         }

         break;
      }

      case LWS_CALLBACK_HTTP_FILE_COMPLETION:
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
      case LWS_CALLBACK_HTTP_BIND_PROTOCOL:
      case LWS_CALLBACK_HTTP_DROP_PROTOCOL:
#if LWS_LIBRARY_VERSION_MAJOR >= 3
      case LWS_CALLBACK_HTTP_CONFIRM_UPGRADE:
      case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
#endif
         break;

      default:
         tell(1, "DEBUG: Unhandled 'callbackHttp' got (%d)", reason);
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
   }

   return lws_serve_http_file(wsi, path, mime, 0, 0);
}

//***************************************************************************
// Callback for my Protocol
//***************************************************************************

int cWebSock::callbackWs(lws* wsi, lws_callback_reasons reason, void* user, void* in, size_t len)
{
   std::string clientInfo = "unknown";

   tell(4, "DEBUG: 'callback' got (%d)", reason);

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

            tell(4, "DEBUG: Write 'PING' to '%s' (%p)", clientInfo.c_str(), (void*)wsi);
               lws_write(wsi, (unsigned char*)buffer + sizeLwsPreFrame, 0, LWS_WRITE_PING);
         }

         else if (msgType == mtData)
         {
            while (!clients[wsi].messagesOut.empty() && !lws_send_pipe_choked(wsi))
            {
               int res;
               std::string msg;  // take a copy of the message for short mutex lock!

               // mutex lock context
               {
                  cMyMutexLock clock(&clientsMutex);
                  cMyMutexLock lock(&clients[wsi].messagesOutMutex);
                  msg = clients[wsi].messagesOut.front();
                  clients[wsi].messagesOut.pop();
               }

               int msgSize = strlen(msg.c_str());
               int neededSize = sizeLwsFrame + msgSize;
               char* newBuffer = 0;

               if (neededSize > msgBufferSize)
               {
                  if (!(newBuffer = (char*)realloc(msgBuffer, neededSize)))
                  {
                     tell(0, "Fatal: Can't allocate memory!");
                     break;
                  }

                  msgBuffer = newBuffer;
                  msgBufferSize = neededSize;
               }

               strncpy(msgBuffer + sizeLwsPreFrame, msg.c_str(), msgSize);

               tell(2, "DEBUG: Write (%d) -> %.*s -> to '%s' (%p)\n", msgSize, msgSize,
                    msgBuffer+sizeLwsPreFrame, clientInfo.c_str(), (void*)wsi);

               res = lws_write(wsi, (unsigned char*)msgBuffer + sizeLwsPreFrame, msgSize, LWS_WRITE_TEXT);

               if (res != msgSize)
                  tell(0, "Error: Only (%d) bytes of (%d) sended", res, msgSize);
            }
         }

         break;
      }

      case LWS_CALLBACK_RECEIVE:                           // data from client
      {
         json_t *oData, *oObject;
         json_error_t error;
         Event event;

         tell(3, "DEBUG: 'LWS_CALLBACK_RECEIVE' [%.*s]", (int)len, (const char*)in);

         {
            cMyMutexLock lock(&clientsMutex);

            if (lws_is_first_fragment(wsi))
               clients[wsi].buffer.clear();

            clients[wsi].buffer.append((const char*)in, len);

            if (!lws_is_final_fragment(wsi))
            {
               tell(3, "DEBUG: Got %zd bytes, have now %zd -> more to get", len, clients[wsi].buffer.length());
               break;
            }

            oData = json_loadb(clients[wsi].buffer.c_str(), clients[wsi].buffer.length(), 0, &error);

            if (!oData)
            {
               tell(0, "Error: Ignoring invalid jason request [%s]", clients[wsi].buffer.c_str());
               tell(0, "Error decoding json: %s (%s, line %d column %d, position %d)",
                    error.text, error.source, error.line, error.column, error.position);
               break;
            }

            clients[wsi].buffer.clear();
         }

         char* message = strndup((const char*)in, len);
         const char* strEvent = getStringFromJson(oData, "event", "<null>");
         event = cWebService::toEvent(strEvent);
         oObject = json_object_get(oData, "object");

         tell(3, "DEBUG: Got '%s'", message);

         if (event == evLogin)                              // { "event" : "login", "object" : { "type" : "foo" } }
         {
            atLogin(wsi, message, clientInfo.c_str(), oObject);
         }
         else if (event == evLogout)                         // { "event" : "logout", "object" : { } }
         {
            tell(3, "DEBUG: Got '%s'", message);
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
            tell(0, "Browser message: '%s'", getStringFromJson(oObject, "message", "<null>"));
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
            tell(1, "Debug: Ignoring '%s' request of client (%p) without login [%s]", strEvent, (void*)wsi, message);
            } */

         json_decref(oData);
         free(message);

         break;
      }

      case LWS_CALLBACK_ESTABLISHED:                       // someone connecting
      {
         tell(1, "Client '%s' connected (%p), ping time set to (%d)", clientInfo.c_str(), (void*)wsi, timeout);
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
         tell(2, "DEBUG: Got 'PONG' from client '%s' (%p)", clientInfo.c_str(), (void*)wsi);
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
         tell(1, "DEBUG: Unhandled 'callbackWs' got (%d)", reason);
         break;
   }

   return 0;
}

//***************************************************************************
// At Login / Logout
//***************************************************************************

void cWebSock::atLogin(lws* wsi, const char* message, const char* clientInfo, json_t* object)
{
   tell(1, "Client login '%s' (%p) [%s]", clientInfo, (void*)wsi, message);

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
   tell(1, "%s '%s' (%p)", clientInfo, message, (void*)wsi);

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
      else
         tell(0, "client %ld not found!", (ulong)wsi);
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
