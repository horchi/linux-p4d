//***************************************************************************
// Automation Control
// File daemon.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 25.04.2020  Jörg Wendel
//***************************************************************************

#pragma once

#include <queue>
#include <jansson.h>

#include "lib/common.h"
#include "lib/db.h"
#include "lib/mqtt.h"

#include "HISTORY.h"

#include "pysensor.h"
#include "websock.h"
#include "service.h"
#include "p4io.h"

#define confDirDefault "/etc/" TARGET

extern char dbHost[];
extern int  dbPort;
extern char dbName[];
extern char dbUser[];
extern char dbPass[];

extern char* confDir;

//***************************************************************************
// Class Daemon
//***************************************************************************

class Daemon : public FroelingService, public cWebInterface
{
   public:

      enum Pins       // we use the 'physical' PIN numbers here!
      {
         //               1         3.3 V
         //               2         5 V
         pinGpio02     =  3,     // GPIO2 (SDA)
         //               4         5 V
         pinGpio03     =  5,     // GPIO3 (SCL)
         //               6         GND
         pinGpio04     =  7,     // GPIO4 (W1)
         pinGpio14     =  8,     // GPIO14 (TX)
         //               9         GND
         pinGpio15     = 10,     // GPIO15 (RX)
         pinGpio17     = 11,     // GPIO17
         pinGpio18     = 12,     // GPIO18
         pinGpio27     = 13,     // GPIO27
         //              14         GND
         pinGpio22     = 15,     // GPIO22
         pinGpio23     = 16,     // GPIO23
         //              17         3.3 V
         pinGpio24     = 18,     // GPIO24
         pinGpio10     = 19,     // GPIO10
         //              20         GND
         pinGpio09     = 21,     // GPIO9
         pinGpio25     = 22,     // GPIO25
         pinGpio11     = 23,     // GPIO11
         pinGpio08     = 24,     // GPIO8 (SPI)
         //              25         GND
         pinGpio07     = 26,     // GPIO7 (ID EEPROM)
         pinIdSd       = 27,     // ID_SD
         pinIdSc       = 28,     // ID_SC
         pinGpio05     = 29,     // GPIO5
         //              30         GND
         pinGpio06     = 31,     // GPIO6
         pinGpio12     = 32,     // GPIO12
         pinGpio13     = 33,     // GPIO13
         //              34         GND
         pinGpio19     = 35,     // GPIO19
         pinGpio16     = 36,     // GPIO16
         pinGpio26     = 37,     // GPIO26
         pinGpio20     = 38,     // GPIO20
         //              39         GND
         pinGpio21     = 40,     // GPIO21

         // aliases

         pinW1       = pinGpio04,
         pinSerialTx = pinGpio14,
         pinSerialRx = pinGpio15,
         pinW1Power  = pinGpio10
      };

      // object

      Daemon();
      virtual ~Daemon();

      virtual int init();
	   int loop();

      const char* myName() override  { return TARGET; }
      static void downF(int aSignal) { shutdown = true; }

      // public interface for Python

      std::string sensorJsonStringOf(const char* type, uint address);

      int setup();
      int initialize(bool truncate = false);

   protected:

      enum WidgetType
      {
         wtUnknown = -1,
         wtSymbol  = 0,  // == 0
         wtChart,        // == 1
         wtText,         // == 2
         wtValue,        // == 3
         wtGauge,        // == 4
         wtMeter,        // == 5
         wtMeterLevel,   // == 6
         wtPlainText,    // == 7  without title
         wtChoice,       // == 8  option choice
         wtCount
      };

      static const char* widgetTypes[];
      static const char* toName(WidgetType type);
      static WidgetType toType(const char* name);

      enum IoType
      {
         iotSensor = 0,
         iotLight
      };

      enum OutputMode
      {
         omAuto = 0,
         omManual
      };

      enum OutputOptions
      {
         ooUser = 0x01,        // Output can contolled by user
         ooAuto = 0x02         // Output automatic contolled
      };

      struct OutputState
      {
         bool state {false};
         OutputMode mode {omAuto};
         uint opt {ooUser};
         const char* name {nullptr};     // crash on init with nullptr :o
         std::string title;
         time_t last {0};                // last switch time
         time_t next {0};                // calculated next switch time
         bool valid {false};             // set if the value is valid
      };

      struct Range
      {
         uint from;
         uint to;

         bool inRange(uint t) const { return (from <= t && to >= t); }
      };

      enum ConfigItemType
      {
         ctInteger = 0,
         ctNum,
         ctString,
         ctBool,
         ctRange,
         ctChoice,
         ctMultiSelect
      };

      struct ConfigItemDef
      {
         std::string name;
         ConfigItemType type;
         const char* def {nullptr};
         bool internal {false};
         const char* category {nullptr};
         const char* title {nullptr};
         const char* description {nullptr};
      };

      std::map<uint,OutputState> digitalOutputStates;
      std::map<int,bool> digitalInputStates;

      int exit();
      int initLocale();
      virtual int initDb();
      virtual int exitDb();
      virtual int readConfiguration(bool initial);
      virtual int applyConfigurationSpecials() { return done; }

      int addValueFact(int addr, const char* type, int factor, const char* name,
                       const char* unit, const char* title, bool active, int maxScale = na);
      int addValueFact(int addr, const char* type, const char* name, const char* unit,
                       WidgetType widgetType, int minScale = 0, int maxScale = na, int rights = 0, const char* choices = nullptr);

      int initOutput(uint pin, int opt, OutputMode mode, const char* name, uint rights = urControl);
      int initInput(uint pin, const char* name);
      int initScripts();

      int standby(int t);
      int standbyUntil(time_t until);
      int meanwhile();
      virtual int atMeanwhile() { return done; }

      int update(bool webOnly = false, long client = 0);   // called each (at least) 'interval'
      int updateState(Status* state);
      virtual int process() { return done; }               // called each 'interval'
      virtual int performJobs() { return done; }           // called every loop (1 second)
      int performWebSocketPing();
      int dispatchClientRequest();
      bool checkRights(long client, Event event, json_t* oObject);
      void updateScriptSensors();
      int callScript(int addr, const char* command, const char* name, const char* title);
      std::string executePython(PySensor* pySensor, const char* command);
      bool isInTimeRange(const std::vector<Range>* ranges, time_t t);

      int store(time_t now, const char* name, const char* title, const char* unit, const char* type, int address,
                double value, uint factor, uint groupid, const char* text = 0);

      cDbRow* valueFactOf(const char* type, int addr);
      void setSpecialValue(uint addr, double value, const std::string& text = "");

      int performMqttRequests();
      int mqttCheckConnection();
      int mqttDisconnect();
      int mqttPublishSensor(IoType iot, const char* name, const char* title, const char* unit, double value, const char* text, bool forceConfig);
      int mqttWrite(json_t* obj, uint groupid);
      int jsonAddValue(json_t* obj, const char* name, const char* title, const char* unit, double theValue, uint groupid, const char* text = 0, bool forceConfig = false);

      int scheduleAggregate();
      int aggregate();

      int sendStateMail();
      int isMailState();
      int loadHtmlHeader();
      int sendErrorMail();
      int sendMail(const char* receiver, const char* subject, const char* body, const char* mimeType);

      int calcStateDuration();
      void scheduleTimeSyncIn(int offset = 0);
      int updateErrors();
      int updateParameter(cDbTable* tableMenu);
      int dispatchMqttCommandRequest(const char* jString);

      void addParameter2Mail(const char* name, const char* value);

      std::string getScriptSensor(int address);
      void afterUpdate();
      void sensorAlertCheck(time_t now);
      int performAlertCheck(cDbRow* alertRow, time_t now, int recurse = 0, int force = no);
      int add2AlertMail(cDbRow* alertRow, const char* title, double value, const char* unit);
      int sendAlertMail(const char* to);

      int updateSchemaConfTable();
      int initValueFacts();
      int updateTimeRangeData();
      int initMenu(bool updateParameters = false);
      int updateScripts();

      int getConfigItem(const char* name, char*& value, const char* def = "");
      int setConfigItem(const char* name, const char* value);
      int getConfigItem(const char* name, int& value, int def = na);
      int getConfigItem(const char* name, long& value, long def = na);
      int setConfigItem(const char* name, long value);
      int getConfigItem(const char* name, double& value, double def = na);
      int setConfigItem(const char* name, double value);
      int getConfigItem(const char* name, bool& value, bool def = false);
      int setConfigItem(const char* name, bool value);

      int getConfigTimeRangeItem(const char* name, std::vector<Range>& ranges);

      bool doShutDown() { return shutdown; }

      void gpioWrite(uint pin, bool state, bool store = true);
      bool gpioRead(uint pin);
      virtual void logReport() { return ; }

      // web

      int pushOutMessage(json_t* obj, const char* title, long client = 0);
      int pushDataUpdate(const char* title, long client);

      int pushInMessage(const char* data) override;
      std::queue<std::string> messagesIn;
      cMyMutex messagesInMutex;
      cCondVar loopCondition;
      cMyMutex loopMutex;

      int replyResult(int status, const char* message, long client);
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
      int performReset(json_t* obj, long client);
      int performChartbookmarks(long client);
      int storeChartbookmarks(json_t* array, long client);
      int performUpdateTimeRanges(json_t* array, long client);
      int performPellets(json_t* array, long client);
      int performPelletsAdd(json_t* array, long client);

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

      const char* getImageFor(const char* title, int value);
      const char* getStateImage(int state);
      int toggleIo(uint addr, const char* type);
      int toggleIoNext(uint pin);
      int toggleOutputMode(uint pin);

      WidgetType getWidgetTypeOf(std::string type, std::string unit, uint address);

      virtual int storeStates();
      virtual int loadStates();

      // arduino

      // int dispatchArduinoMsg(const char* message);
      // int initArduino();
      // void updateAnalogInput(const char* id, double value, time_t stamp);

      // W1

      int initW1();
      bool existW1(const char* id);
      int dispatchW1Msg(const char* message);
      double valueOfW1(const char* id, time_t& last);
      uint toW1Id(const char* name);
      void updateW1(const char* id, double value, time_t stamp);
      void cleanupW1();

      // data

      bool initialized {false};
      cDbConnection* connection {nullptr};

      cDbTable* tableSamples {nullptr};
      cDbTable* tablePeaks {nullptr};
      cDbTable* tableValueFacts {nullptr};
      cDbTable* tableGroups {nullptr};
      cDbTable* tableMenu {nullptr};
      cDbTable* tableErrors {nullptr};
      cDbTable* tableSensorAlert {nullptr};
      cDbTable* tableSchemaConf {nullptr};
      cDbTable* tableConfig {nullptr};
      cDbTable* tableTimeRanges {nullptr};
      cDbTable* tableScripts {nullptr};
      cDbTable* tableUsers {nullptr};
      cDbTable* tablePellets {nullptr};

      cDbStatement* selectActiveValueFacts {nullptr};
      cDbStatement* selectAllValueFacts {nullptr};
      cDbStatement* selectAllConfig {nullptr};
      cDbStatement* selectAllUser {nullptr};
      cDbStatement* selectMaxTime {nullptr};
      cDbStatement* selectSamplesRange {nullptr};     // for chart
      cDbStatement* selectSamplesRange60 {nullptr};   // for chart
      cDbStatement* selectSamplesRange720 {nullptr};  // for chart
      cDbStatement* selectScriptByPath {nullptr};

      cDbStatement* selectAllGroups {nullptr};
      cDbStatement* selectAllMenuItems {nullptr};

      cDbStatement* selectMenuItemsByParent {nullptr};
      cDbStatement* selectMenuItemsByChild {nullptr};
      cDbStatement* selectSensorAlerts {nullptr};
      cDbStatement* selectAllSensorAlerts {nullptr};
      cDbStatement* selectSchemaConfByState {nullptr};
      cDbStatement* selectAllSchemaConf {nullptr};
      cDbStatement* selectStokerHours {nullptr};
      cDbStatement* selectSampleInRange {nullptr};    // for alert check
      cDbStatement* selectStateDuration {nullptr};

      cDbStatement* selectPendingErrors {nullptr};
      cDbStatement* selectAllErrors {nullptr};
      cDbStatement* selectScriptByName {nullptr};
      cDbStatement* selectScript {nullptr};
      cDbStatement* selectAllPellets {nullptr};

      cDbValue xmlTime;
      cDbValue rangeFrom;
      cDbValue rangeTo;
      cDbValue rangeEnd;
      cDbValue avgValue;
      cDbValue endTime;
      cDbValue maxValue;
      cDbValue minValue;

      time_t nextRefreshAt {0};
      time_t startedAt {0};
      time_t nextAggregateAt {0};

      Sem* sem {nullptr};

      P4Request* request {nullptr};
      Serial* serial {nullptr};

      Status currentState;
      bool stateChanged {false};
      std::string mailBodyHtml;
      bool initialRun {true};

      // WS Interface stuff

      cWebSock* webSock {nullptr};
      time_t nextWebSocketPing {0};
      int webSocketPingTime {60};
      const char* httpPath {"/var/lib/" TARGET};

      struct WsClient    // Web Socket Client
      {
         std::string user;
         uint rights;                  // rights mask
         std::string page;
         ClientType type {ctActive};
      };

      std::map<void*,WsClient> wsClients;

      // MQTT stuff

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
      char* mqttHassUrl {nullptr};
      char* mqttUser {nullptr};
      char* mqttPassword {nullptr};
      time_t lastMqttConnectAt {0};
      char* mqttDataTopic {nullptr};
      int mqttHaveConfigTopic {yes};
      json_t* oJson {nullptr};
      std::map<int,Group> groups;

      Mqtt* mqttReader {nullptr};
      Mqtt* mqttHassWriter {nullptr};
      Mqtt* mqttHassReader {nullptr};
      Mqtt* mqttHassCommandReader {nullptr};

      // config

      int interval {60};
      int stateCheckInterval {10};
      char* knownStates {nullptr};
      double consumptionPerHour {0};

      int webPort {1111};
      char* webUrl {nullptr};
      bool webSsl {false};
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
      char* iconSet {nullptr};
      time_t nextTimeSyncAt {0};
      int maxTimeLeak {10};
      MemoryStruct htmlHeader;

      int invertDO {no};

      char* sensorScript {nullptr};
      char* chartSensors {nullptr};

      // live data

      struct AiSensorData
      {
         time_t last {0};
         double value {0.0};
         double calPointA {0.0};
         double calPointB {10.0};
         double calPointValueA {0};
         double calPointValueB {10};
         double round {0.0};                // e.g. 50.0 -> 0.02; 20.0 -> 0.05;
         bool disabled {false};
         bool valid {false};                // set if the value is valid
      };

      std::map<int,AiSensorData> aiSensors;

      struct W1SensorData
      {
         time_t last {0};
         double value {0.0};
         bool valid {false};                // set if the value is valid
      };

      std::map<std::string,W1SensorData> w1Sensors;

      struct SensorData        // Sensor Data
      {
         std::string kind {"value"};
         time_t last {0};
         double value {0.0};
         bool state {0};
         std::string text;
         bool disabled {false};
         bool valid {false};                // set if the value, text or state is valid
         PySensor* pySensor {nullptr};
      };

      std::map<int,SensorData> spSensors;
      std::map<int,SensorData> scSensors;

      std::map<std::string,json_t*> jsonSensorList;

      virtual std::list<ConfigItemDef>* getConfiguration() = 0;

      std::string alertMailBody;
      std::string alertMailSubject;
      std::map<int,double> vaValues;
      std::map<int,time_t> stateDurations;

      // statics

      static bool shutdown;
};