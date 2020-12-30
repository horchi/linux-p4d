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

#include "lib/db.h"
#include "lib/curl.h"
#include "lib/mqtt.h"

#include "service.h"
#include "p4io.h"
#include "w1.h"
#include "websock.h"

#include "HISTORY.h"

#define confDirDefault "/etc/p4d"

extern char dbHost[];
extern int  dbPort;
extern char dbName[];
extern char dbUser[];
extern char dbPass[];

extern char* confDir;

//***************************************************************************
// Class P4d
//***************************************************************************

class P4d : public FroelingService, public cWebInterface
{
   public:

      // object

      P4d();
      virtual ~P4d();

      const char* myName() override { return "p4d"; }

      int init();
	   int loop();
	   int setup();
	   int initialize(int truncate = no);

      static void downF(int aSignal) { shutdown = yes; }

   protected:

      enum WidgetType
      {
         wtSymbol = 0,  // == 0
         wtChart,       // == 1
         wtText,        // == 2
         wtValue,       // == 3
         wtGauge1       // == 4
      };

      enum ConfigItemType
      {
         ctInteger = 0,
         ctNum,
         ctString,
         ctBool,
         ctRange,
         ctChoice
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
      int updateParameter(cDbTable* tableMenu);
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
      int add2AlertMail(cDbRow* alertRow, const char* title, double value, const char* unit);
      int sendAlertMail(const char* to);
      int sendStateMail();
      int sendErrorMail();
      int sendMail(const char* receiver, const char* subject, const char* body, const char* mimeType);

      int updateSchemaConfTable();
      int initValueFacts();
      int updateTimeRangeData();
      int initMenu();
      int updateScripts();
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
      int pushDataUpdate(const char* title, long client);

      int pushInMessage(const char* data) override;
      std::queue<std::string> messagesIn;
      cMyMutex messagesInMutex;
      cCondVar loopCondition;
      cMyMutex loopMutex;

      int replyResult(int status, const char* message, long client);
      int performWebSocketPing();
      int performLogin(json_t* oObject);
      int performLogout(json_t* oObject);
      int performInitTables(json_t* oObject, long client);
      int performTokenRequest(json_t* oObject, long client);
      int performSyslog(long client);
      int performConfigDetails(long client);
      int performUserDetails(long client);
      int performIoSettings(json_t* oObject, long client);
      int performGroups(long client);
      int performErrors(long client);
      int performMenu(json_t* oObject, long client);
      int performSchema(json_t* oObject, long client);
      int performAlerts(json_t* oObject, long client);
      int performSendMail(json_t* oObject, long client);
      int performAlertTestMail(int id, long client);
      int performParEditRequest(json_t* oObject, long client);
      int performTimeParEditRequest(json_t* oObject, long client);
      int performParStore(json_t* oObject, long client);
      int performTimeParStore(json_t* oObject, long client);
      int performChartData(json_t* oObject, long client);
      int performUserConfig(json_t* oObject, long client);
      int performPasswChange(json_t* oObject, long client);
      int storeConfig(json_t* obj, long client);
      int storeIoSetup(json_t* array, long client);
      int storeAlerts(json_t* oObject, long client);
      int storeSchema(json_t* oObject, long client);
      int storeGroups(json_t* array, long client);
      int resetPeaks(json_t* obj, long client);
      int performChartbookmarks(long client);
      int storeChartbookmarks(json_t* array, long client);

      int config2Json(json_t* obj);
      int configDetails2Json(json_t* obj);
      int configChoice2json(json_t* obj, const char* name);
      int userDetails2Json(json_t* obj);
      int valueFacts2Json(json_t* obj, bool filterActive);
      int groups2Json(json_t* obj);
      int daemonState2Json(json_t* obj);
      int s3200State2Json(json_t* obj);
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
      // cDbTable* tableSmartConf {nullptr};
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
      cDbStatement* selectMenuItemsByParent {nullptr};
      cDbStatement* selectMenuItemsByChild {nullptr};
      cDbStatement* selectSensorAlerts {nullptr};
      cDbStatement* selectAllSensorAlerts {nullptr};
      cDbStatement* selectSchemaConfByState {nullptr};
      cDbStatement* selectAllSchemaConf {nullptr};
      cDbStatement* selectSampleInRange {nullptr};   // for alert check
      cDbStatement* selectSamplesRange {nullptr};    // for chart
      cDbStatement* selectSamplesRange60 {nullptr};  // for chart

      cDbStatement* selectPendingErrors {nullptr};
      cDbStatement* selectAllErrors {nullptr};
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
         std::string page;
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

      int webPort {1111};
      char* webUrl {nullptr};

      char* ttyDevice {nullptr};
      char* heatingType {nullptr};
      int aggregateInterval {15};         // aggregate interval in minutes
      int aggregateHistory {0};           // history in days

      std::vector<std::string> addrsDashboard;
      std::vector<std::string> addrsList;

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
      char* chartSensors {nullptr};

      std::string alertMailBody;
      std::string alertMailSubject;
      std::map<int,double> vaValues;
      std::map<std::string, json_t*> jsonSensorList;

      //

      static std::list<ConfigItemDef> configuration;
      static int shutdown;
};
