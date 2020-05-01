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
#include "lib/mqtt.h"

#include "service.h"
#include "p4io.h"
#include "w1.h"
#include "lib/curl.h"
#include "HISTORY.h"

#ifdef MQTT_HASS
#  include "lib/mqtt.h"
#endif

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

class P4d : public FroelingService
{
   public:

      // object

      P4d();
      ~P4d();

      int init();
	   int loop();
	   int setup();
	   int initialize(int truncate = no);

      static void downF(int aSignal) { shutdown = yes; }

   protected:

      int exit();
      int initDb();
      int exitDb();
      int readConfiguration();

      int standby(int t);
      int standbyUntil(time_t until);
      int meanwhile();

      int update();
      int updateState(Status* state);
      void scheduleTimeSyncIn(int offset = 0);
      int scheduleAggregate();
      int aggregate();

      int updateErrors();
      int performWebifRequests();
      int cleanupWebifRequests();

      int store(time_t now, const char* name, const char* title, const char* unit, const char* type, int address, double value,
                uint factor, uint groupid, const char* text = 0);

#ifdef MQTT_HASS
      int mqttPublishSensor(const char* name, const char* title, const char* unit, double theValue, const char* text = 0, bool forceConfig = false);
      int jsonAddValue(json_t* obj, const char* name, const char* title, const char* unit, double theValue, uint groupid, const char* text = 0, bool forceConfig = false);
      int mqttCheckConnection();
      int mqttWrite(json_t* obj, uint groupid);
#endif

      void addParameter2Mail(const char* name, const char* value);

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

      cDbStatement* selectActiveValueFacts {nullptr};
      cDbStatement* selectAllValueFacts {nullptr};
      cDbStatement* selectAllGroups {nullptr};
      cDbStatement* selectPendingJobs {nullptr};
      cDbStatement* selectAllMenuItems {nullptr};
      cDbStatement* selectSensorAlerts {nullptr};
      cDbStatement* selectSampleInRange {nullptr};
      cDbStatement* selectPendingErrors {nullptr};
      cDbStatement* selectMaxTime {nullptr};
      cDbStatement* selectHmSysVarByAddr {nullptr};
      cDbStatement* selectScriptByName {nullptr};
      cDbStatement* selectScript {nullptr};
      cDbStatement* cleanupJobs {nullptr};

      cDbValue rangeEnd;

      time_t nextAt;
      time_t startedAt;
      time_t nextAggregateAt {0};

      Sem* sem {nullptr};

      P4Request* request {nullptr};
      Serial* serial {nullptr};

      W1 w1;                       // for one wire sensors
      cCurl* curl {nullptr};

      Status currentState;
      string mailBody;
      string mailBodyHtml;
      bool initialRun {true};

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

#ifdef MQTT_HASS

      MqTTPublishClient* mqttWriter {nullptr};
      MqTTSubscribeClient* mqttReader {nullptr};

#endif // HASS

      // config

      int interval {60};
      int stateCheckInterval {10};
      char* ttyDevice {nullptr};
      int aggregateInterval {15};         // aggregate interval in minutes
      int aggregateHistory {0};           // history in days

      int mail {no};
      int htmlMail {no};
      char* mailScript {nullptr};
      char* stateMailAtStates {nullptr};
      char* stateMailTo {nullptr};
      char* errorMailTo {nullptr};
      int errorsPending {0};
      int tSync {no};
      time_t nextTimeSyncAt {0};
      int maxTimeLeak {10};
      MemoryStruct htmlHeader;

      string alertMailBody;
      string alertMailSubject;

      //

      static int shutdown;
};
