//***************************************************************************
// p4d / Linux - Heizungs Manager
// File p4d.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 25.04.2020  Jörg Wendel
//***************************************************************************

#ifndef _P4D_H_
#define _P4D_H_

//***************************************************************************
// Includes
//***************************************************************************

#include "lib/db.h"

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

extern char ttyDeviceSvc[];
extern int interval;
extern int stateCheckInterval;
extern int aggregateInterval;        // aggregate interval in minutes
extern int aggregateHistory;         // history in days
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
                unsigned int factor, const char* text = 0);

#ifdef MQTT_HASS
      int hassPush(const char* name, const char* title, const char* unit, double theValue, const char* text = 0, bool forceConfig = false);
      int hassCheckConnection();
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

      cDbConnection* connection {nullptr};

      cDbTable* tableSamples {nullptr};
      cDbTable* tablePeaks {nullptr};
      cDbTable* tableValueFacts {nullptr};
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
      Sem* sem {nullptr};

      P4Request* request {nullptr};
      Serial* serial {nullptr};

      W1 w1;                       // for one wire sensors
      cCurl* curl {nullptr};

      Status currentState;
      string mailBody;
      string mailBodyHtml;
      bool initialRun;

      // Home Assistant stuff

      char* mqttUrl {nullptr};
      char* mqttDataTopic {nullptr};
      int mqttHaveConfigTopic {yes};

#ifdef MQTT_HASS

      MqTTPublishClient* mqttWriter {nullptr};
      MqTTSubscribeClient* mqttReader {nullptr};

#endif // HASS

      // config

      int mail;
      int htmlMail;
      char* mailScript {nullptr};
      char* stateMailAtStates {nullptr};
      char* stateMailTo {nullptr};
      char* errorMailTo {nullptr};
      int errorsPending;
      int tSync;
      time_t nextTimeSyncAt;
      int maxTimeLeak;
      MemoryStruct htmlHeader;

      string alertMailBody;
      string alertMailSubject;

      time_t nextAggregateAt;

      //

      static int shutdown;
};

//***************************************************************************
#endif // _P4D_H_
