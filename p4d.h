//***************************************************************************
// p4d / Linux - Heizungs Manager
// File p4d.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 25.04.2020  Jörg Wendel
//***************************************************************************

#pragma once

//***************************************************************************
// Includes
//***************************************************************************

#include <jansson.h>
#include <libwebsockets.h>

#include <unordered_map>

#include "lib/db.h"

#include "service.h"
#include "p4io.h"
#include "w1.h"
#include "lib/curl.h"
#include "lib/mqtt.h"

#include "HISTORY.h"

#define confDirDefault "/etc/p4d"

extern char dbHost[];
extern int  dbPort;
extern char dbName[];
extern char dbUser[];
extern char dbPass[];

extern char* confDir;

class P4d;
typedef P4d MainClass;

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

      cWebSock(const char* aHttpPath);
      virtual ~cWebSock();

      int init(int aPort, int aTimeout);
      int exit();

      int service();
      int performData(MsgType type);

      // status callback methods

      static int wsLogLevel;
      static int callbackHttp(lws* wsi, lws_callback_reasons reason, void* user, void* in, size_t len);
      static int callbackWs(lws* wsi, lws_callback_reasons reason, void* user, void* in, size_t len);

      // static interface

      static void atLogin(lws* wsi, const char* message, const char* clientInfo, json_t* object);
      static void atLogout(lws* wsi, const char* message, const char* clientInfo);
      static int getClientCount();
      static void pushMessage(const char* p, lws* wsi = 0);
      static void writeLog(int level, const char* line);
      static void setClientType(lws* wsi, ClientType type);

   private:

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

//***************************************************************************
// Class P4d
//***************************************************************************

class P4d : public FroelingService, public cWebService
{
   public:

      // object

      P4d();
      ~P4d();

      int init();
	   int loop();
	   int setup();
	   int initialize(int truncate = no);

      static const char* myName()    { return "p4d"; }
      static void downF(int aSignal) { shutdown = yes; }

      // public static message interface to web thread

      static int pushInMessage(const char* data);
      static std::queue<std::string> messagesIn;
      static cMyMutex messagesInMutex;

   protected:

      enum WidgetType
      {
         wtSymbol = 0,  // == 0
         wtGauge,       // == 1
         wtText,        // == 2
         wtValue        // == 3
      };

      enum ConfigItemType
      {
         ctInteger = 0,
         ctNum,
         ctString,
         ctBool,
         ctRange
      };

      struct ConfigItemDef
      {
         std::string name;
         ConfigItemType type;
         bool internal;
         const char* category;
         const char* title;
         const char* description;
      };

      int exit();
      int initDb();
      int exitDb();
      int readConfiguration();
      int applyConfigurationSpecials();

      int standby(int t);
      int standbyUntil(time_t until);
      int meanwhile();

      int update(bool webOnly = false, long client = 0);   // called each (at least) 'interval'
      int updateState(Status* state);
      void scheduleTimeSyncIn(int offset = 0);
      int scheduleAggregate();
      int aggregate();

      int updateErrors();
      int performWebifRequests();
      int cleanupWebifRequests();
      int dispatchClientRequest();
      bool checkRights(long client, Event event, json_t* oObject);

      int store(time_t now, const char* name, const char* title, const char* unit, const char* type, int address, double value,
                uint factor, uint groupid, const char* text = 0);

      int mqttPublishSensor(const char* name, const char* title, const char* unit, double theValue, const char* text = 0, bool forceConfig = false);
      int jsonAddValue(json_t* obj, const char* name, const char* title, const char* unit, double theValue, uint groupid, const char* text = 0, bool forceConfig = false);
      int mqttCheckConnection();
      int mqttWrite(json_t* obj, uint groupid);

      void addParameter2Mail(const char* name, const char* value);

      std::string getScriptSensor(int address);
      void afterUpdate();
      void sensorAlertCheck(time_t now);
      int performAlertCheck(cDbRow* alertRow, time_t now, int recurse = 0, int force = no);
      int add2AlertMail(cDbRow* alertRow, const char* title,
                            double value, const char* unit);
      int sendAlertMail(const char* to);
      int sendStateMail();
      int sendErrorMail();
      int sendMail(const char* receiver, const char* subject, const char* body, const char* mimeType);

      int updateSchemaConfTable();
      int updateValueFacts();
      int updateTimeRangeData();
      int initMenu();
      int updateScripts();
      int callScript(const char* scriptName, const char*& result);
      int hmUpdateSysVars();
      int hmSyncSysVars();

      int isMailState();
      int loadHtmlHeader();

      int getConfigItem(const char* name, char*& value, const char* def = "");
      int setConfigItem(const char* name, const char* value);
      int getConfigItem(const char* name, int& value, int def = na);
      int setConfigItem(const char* name, int value);

      int doShutDown() { return shutdown; }

      // web

      int pushOutMessage(json_t* obj, const char* title, long client = 0);

      int performWebSocketPing();
      int performLogin(json_t* oObject);
      int performLogout(json_t* oObject);
      int performTokenRequest(json_t* oObject, long client);
      int performSyslog(long client);
      int performConfigDetails(long client);
      int performUserDetails(long client);
      int performIoSettings(long client);
      int performGroups(long client);
      int performChartData(json_t* oObject, long client);
      int performUserConfig(json_t* oObject, long client);
      int performPasswChange(json_t* oObject, long client);
      int storeConfig(json_t* obj, long client);
      int storeIoSetup(json_t* array, long client);
      int storeGroups(json_t* array, long client);
      int resetPeaks(json_t* obj, long client);

      int config2Json(json_t* obj);
      int configDetails2Json(json_t* obj);
      int userDetails2Json(json_t* obj);
      int valueFacts2Json(json_t* obj);
      int groups2Json(json_t* obj);
      int daemonState2Json(json_t* obj);
      int sensor2Json(json_t* obj, cDbTable* table);
      void pin2Json(json_t* ojData, int pin);

      bool fileExistsAtWeb(const char* file);
      const char* getImageOf(const char* title, const char* usrTitle, int value);
      const char* getStateImage(int state);
      WidgetType getWidgetTypeOf(std::string type, std::string unit, uint address);

      // data

      bool initialized {false};
      cDbConnection* connection {nullptr};

      cDbTable* tableSamples {nullptr};
      cDbTable* tablePeaks {nullptr};
      cDbTable* tableValueFacts {nullptr};
      cDbTable* tableGroups {nullptr};
      cDbTable* tableMenu {nullptr};
      cDbTable* tableErrors {nullptr};
      cDbTable* tableJobs {nullptr};
      cDbTable* tableSensorAlert {nullptr};
      cDbTable* tableSchemaConf {nullptr};
      cDbTable* tableSmartConf {nullptr};
      cDbTable* tableConfig {nullptr};
      cDbTable* tableTimeRanges {nullptr};
      cDbTable* tableHmSysVars {nullptr};
      cDbTable* tableScripts {nullptr};
      cDbTable* tableUsers {nullptr};

      cDbStatement* selectActiveValueFacts {nullptr};
      cDbStatement* selectAllValueFacts {nullptr};
      cDbStatement* selectAllGroups {nullptr};
      cDbStatement* selectPendingJobs {nullptr};
      cDbStatement* selectAllMenuItems {nullptr};
      cDbStatement* selectSensorAlerts {nullptr};

      cDbStatement* selectSampleInRange {nullptr};   // for alert check
      cDbStatement* selectSamplesRange {nullptr};    // for chart
      cDbStatement* selectSamplesRange60 {nullptr};  // for chart

      cDbStatement* selectPendingErrors {nullptr};
      cDbStatement* selectMaxTime {nullptr};
      cDbStatement* selectHmSysVarByAddr {nullptr};
      cDbStatement* selectScriptByName {nullptr};
      cDbStatement* selectScript {nullptr};
      cDbStatement* cleanupJobs {nullptr};
      cDbStatement* selectAllConfig {nullptr};
      cDbStatement* selectAllUser {nullptr};

      cDbValue xmlTime;
      cDbValue rangeFrom;
      cDbValue rangeTo;
      cDbValue rangeEnd;
      cDbValue avgValue;
      cDbValue maxValue;

      time_t nextAt;
      time_t startedAt;
      time_t nextAggregateAt {0};

      Sem* sem {nullptr};

      P4Request* request {nullptr};
      Serial* serial {nullptr};

      W1 w1;                       // for one wire sensors
      cCurl* curl {nullptr};

      Status currentState;
      std::string mailBody;
      std::string mailBodyHtml;
      bool initialRun {true};

      // WS Interface stuff

      cWebSock* webSock {nullptr};
      time_t nextWebSocketPing {0};
      int webSocketPingTime {60};
      const char* httpPath {"/var/lib/p4"};

      struct WsClient    // Web Socket Client
      {
         std::string user;
         uint rights;                  // rights mask
         bool dataUpdates {false};     // true if interested on data update
         ClientType type {ctActive};
      };

      std::map<void*,WsClient> wsClients;

      // MQTT Interface stuff

      enum MqttInterfaceStyle
      {
         misNone,
         misSingleTopic,     // one topic for all sensors
         misMultiTopic,      // separate topic for each sensors
         misGroupedTopic     // one topic for each group (Baugruppe)
      };

      struct Group
      {
         std::string name;
         json_t* oJson {nullptr};
      };

      MqttInterfaceStyle mqttInterfaceStyle {misNone};
      char* mqttUrl {nullptr};
      char* mqttUser {nullptr};
      char* mqttPassword {nullptr};
      char* mqttDataTopic {nullptr};
      int mqttHaveConfigTopic {yes};
      json_t* oJson {nullptr};
      std::map<int,Group> groups;

      Mqtt* mqttWriter {nullptr};
      Mqtt* mqttReader {nullptr};

      // config

      int interval {60};
      int stateCheckInterval {10};
      char* ttyDevice {nullptr};
      char* heatingType {nullptr};
      int aggregateInterval {15};         // aggregate interval in minutes
      int aggregateHistory {0};           // history in days

      int mail {no};
      char* mailScript {nullptr};
      char* stateMailAtStates {nullptr};
      char* stateMailTo {nullptr};
      char* errorMailTo {nullptr};
      int errorsPending {0};
      int tSync {no};
      int stateAni {no};
      time_t nextTimeSyncAt {0};
      int maxTimeLeak {10};
      MemoryStruct htmlHeader;
      char* sensorScript {nullptr};
      char* chart1 {nullptr};
      char* chart2 {nullptr};

      std::string alertMailBody;
      std::string alertMailSubject;

      //

      static std::list<ConfigItemDef> configuration;
      static int shutdown;
};
