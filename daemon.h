//***************************************************************************
// Automation Control
// File daemon.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 01.12.2021  Jörg Wendel
//***************************************************************************

#pragma once

#include <queue>
#include <jansson.h>

#include "lib/common.h"
#include "lib/db.h"
#include "lib/mqtt.h"

#include "HISTORY.h"

#include "websock.h"
#include "deconz.h"

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

class Daemon : public cWebInterface
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

      enum SensorOptions
      {
         soNone   = 0x00,
         soSwitch = 0x01,
         soDim    = 0x02,
         soColor  = 0x04
      };

      // object

      Daemon();
      virtual ~Daemon();

      int loop();
      virtual int init();

      const char* myName() override  { return TARGET; }
      virtual const char* myTitle()  { return "Daemon"; }
      static void downF(int aSignal) { shutdown = true; }

      int addValueFact(int addr, const char* type, int factor, const char* name, const char* unit = "",
                       const char* title = nullptr, int rights = 0, const char* choices = nullptr, SensorOptions options = soNone);

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
         wtSymbolValue,  // == 9
         wtSpace,        // == 10
         wtTime,         // == 11  // dummy to display current time at WEBIF
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

      enum Direction
      {
         dirOpen  = 1,
         dirClose = 2
      };

      struct SensorData        // Sensor Data
      {
         bool record {false};
         std::string kind {"value"};       // { value | status | text | trigger }
         time_t last {0};
         double value {0.0};
         bool working {false};             // actually working/moving (eg for blinds)
         Direction lastDir {dirOpen};
         bool state {false};
         std::string text;
         std::string image;
         bool disabled {false};
         bool valid {false};               // set if the value, text or state is valid
         time_t next {0};                  // calculated next switch time
         int battery {na};
         int hue;                         // 0-360° hue
         int sat;                         // 0-100% saturation

         // fact

         std::string type;
         uint address {0};
         std::string name;
         std::string title;
         std::string unit;
         int factor {1};
         int rights {cWebService::urView};
         int group {0};

         // 'DO' specials

         OutputMode mode {omAuto};
         uint opt {ooUser};
      };

      struct Range
      {
         uint from;
         uint to;
         bool inRange(uint t) const { return (from <= t && to >= t); }
      };

      enum LogicalOperator
      {
         loAnd,
         loOr,
         loAndNot,
         loOrNot
      };

      enum ConfigItemType
      {
         ctInteger = 0,
         ctNum,
         ctString,
         ctBool,
         ctRange,
         ctChoice,
         ctMultiSelect,
         ctBitSelect,
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

      struct DefaultWidgetProperty
      {
         std::string type;
         int address {na};
         std::string unit;

         WidgetType widgetType {wtMeter};
         int minScale {0};
         int maxScale {100};
         int scaleStep {0};
         bool showPeak {false};
      };

      static DefaultWidgetProperty defaultWidgetProperties[];
      static DefaultWidgetProperty* getDefalutProperty(const char* type, const char* unit, int address = 0);
      int widgetDefaults2Json(json_t* jDefaults, const char* type, const char* unit, const char* name, int address = 0);

      struct ValueTypes
      {
         std::string typeExpression;
         std::string title;
      };

      static ValueTypes defaultValueTypes[];
      static const char* getTitleOfType(const char* type);

      virtual int exit();
      virtual int initLocale();
      virtual int initDb();
      virtual int exitDb();
      virtual int readConfiguration(bool initial);
      virtual int applyConfigurationSpecials() { return done; }

      int initSensorByFact(std::string type, uint address);
      int initOutput(uint pin, int opt, OutputMode mode, const char* name, uint rights = urControl);
      int initInput(uint pin, const char* name);
      int initScripts();

      int standby(int t);
      virtual int standbyUntil();
      int meanwhile();
      virtual int atMeanwhile() { return done; }

      virtual int updateSensors() { return done; }
      int storeSamples();
      void updateScriptSensors();
      virtual int doLoop()     { return done; }
      virtual void afterUpdate();

      void sensorAlertCheck(time_t now);
      int performAlertCheck(cDbRow* alertRow, time_t now, int recurse = 0, int force = no);
      int add2AlertMail(cDbRow* alertRow, const char* title, double value, const char* unit);
      int sendAlertMail(const char* to);

      virtual int process() { return done; }               // called each 'interval'
      virtual int performJobs() { return done; }           // called every loop (1 second)
      int performWebSocketPing();
      int dispatchClientRequest();
      virtual int dispatchSpecialRequest(Event event, json_t* oObject, long client) { return ignore; }
      virtual int dispatchMqttHaCommandRequest(json_t* jData, const char* topic);
      virtual int dispatchNodeRedCommands(const char* topic, json_t* jObject);
      virtual int dispatchNodeRedCommand(json_t* jObject);
      virtual int dispatchDeconz();
      virtual int dispatchHomematicRpcResult(const char* message);
      virtual int dispatchHomematicEvents(const char* message);
      virtual int dispatchOther(const char* topic, const char* message);
      bool checkRights(long client, Event event, json_t* oObject);
      virtual bool onCheckRights(long client, Event event, uint rights) { return false; }
      int callScript(int addr, const char* command, const char* name, const char* title);
      bool isInTimeRange(const std::vector<Range>* ranges, time_t t);

      int updateWeather();
      int weather2json(json_t* jWeather, json_t* owmWeather);

      int store(time_t now, const SensorData* sensor);

      cDbRow* valueFactOf(const char* type, uint addr);
      SensorData* getSensor(const char* type, int addr);
      void setSpecialValue(uint addr, double value, const std::string& text = "");

      int performMqttRequests();
      int mqttCheckConnection();
      int mqttDisconnect();
      int mqttHaPublish(SensorData& sensor, bool forceConfig = false);
      int mqttHaPublishSensor(SensorData& sensor, bool forceConfig = false);
      int mqttNodeRedPublishSensor(SensorData& sensor);
      int mqttNodeRedPublishAction(SensorData& sensor, double value, bool publishOnly = false);
      int mqttHaWrite(json_t* obj, uint groupid);
      int jsonAddValue(json_t* obj, SensorData& sensor, bool forceConfig = false);
      int updateSchemaConfTable();

      int scheduleAggregate();
      int aggregate();

      int loadHtmlHeader();
      int sendMail(const char* receiver, const char* subject, const char* body, const char* mimeType);

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

      int pushOutMessage(json_t* obj, const char* event, long client = 0);
      int pushDataUpdate(const char* event, long client);

      int pushInMessage(const char* data) override;
      std::queue<std::string> messagesIn;
      cMyMutex messagesInMutex;

      int replyResult(int status, const char* message, long client);
      virtual int performLogin(json_t* oObject);
      int performLogout(json_t* oObject);
      int performTokenRequest(json_t* oObject, long client);
      int performPageChange(json_t* oObject, long client);
      int performToggleIo(json_t* oObject, long client);
      int performSystem(json_t* oObject, long client);
      int performSyslog(json_t* oObject, long client);
      int performConfigDetails(long client);
      int performGroups(long client);
      int performTestMail(json_t* oObject, long client);
      int storeAlerts(json_t* oObject, long client);
      int performAlerts(json_t* oObject, long client);
      int performAlertTestMail(int id, long client);

      int performData(long client, const char* event = nullptr);
      int performChartData(json_t* oObject, long client);
      int performUserDetails(long client);
      int storeUserConfig(json_t* oObject, long client);
      int performPasswChange(json_t* oObject, long client);
      int performForceRefresh(json_t* obj, long client);
      int storeConfig(json_t* obj, long client);
      int storeDashboards(json_t* obj, long client);
      virtual int storeIoSetup(json_t* array, long client);
      int storeGroups(json_t* array, long client);
      int performChartbookmarks(long client);
      int storeChartbookmarks(json_t* array, long client);
      int performImageConfig(json_t* obj, long client);
      int performSchema(json_t* oObject, long client);
      int storeSchema(json_t* oObject, long client);
      virtual int performCommand(json_t* obj, long client);
      virtual const char* getTextImage(const char* key, const char* text) { return nullptr; }

      int widgetTypes2Json(json_t* obj);
      int config2Json(json_t* obj);
      int configDetails2Json(json_t* obj);
      int userDetails2Json(json_t* obj);
      virtual int configChoice2json(json_t* obj, const char* name);

      int valueTypes2Json(json_t* obj);
      int valueFacts2Json(json_t* obj, bool filterActive);
      int dashboards2Json(json_t* obj);
      int groups2Json(json_t* obj);
      virtual int commands2Json(json_t* obj);
      int daemonState2Json(json_t* obj);
      int sensor2Json(json_t* obj, const char* type, uint address);
      int images2Json(json_t* obj);
      void pin2Json(json_t* ojData, int pin);
      void publishSpecialValue(int addr);
      bool webFileExists(const char* file, const char* base = nullptr);

      const char* getImageFor(const char* type, const char* title, const char* unit, int value);
      int toggleIo(uint addr, const char* type, int state = na, int bri = na, int transitiontime = na);
      int toggleColor(uint addr, const char* type, int color, int sat, int bri);
      int toggleIoNext(uint pin);
      int toggleOutputMode(uint pin);

      virtual int storeStates();
      virtual int loadStates();

      // arduino

      int dispatchArduinoMsg(const char* message);
      int initArduino();
      void updateAnalogInput(const char* id, double value, time_t stamp);

      // W1

      bool existW1(uint address);
      int dispatchW1Msg(const char* message);
      double valueOfW1(uint address, time_t& last);
      uint toW1Id(const char* name);
      void updateW1(const char* id, double value, time_t stamp);
      void cleanupW1();

      // data

      bool initialized {false};
      cDbConnection* connection {nullptr};

      cDbTable* tableTableStatistics {nullptr};
      cDbTable* tableSamples {nullptr};
      cDbTable* tablePeaks {nullptr};
      cDbTable* tableValueFacts {nullptr};
      cDbTable* tableValueTypes {nullptr};
      cDbTable* tableConfig {nullptr};
      cDbTable* tableScripts {nullptr};
      cDbTable* tableSensorAlert {nullptr};
      cDbTable* tableUsers {nullptr};
      cDbTable* tableGroups {nullptr};
      cDbTable* tableDashboards {nullptr};
      cDbTable* tableDashboardWidgets {nullptr};
      cDbTable* tableSchemaConf {nullptr};
      cDbTable* tableHomeMatic {nullptr};

      cDbStatement* selectTableStatistic {nullptr};
      cDbStatement* selectAllGroups {nullptr};
      cDbStatement* selectAllValueTypes {nullptr};
      cDbStatement* selectActiveValueFacts {nullptr};
      cDbStatement* selectValueFactsByType {nullptr};
      cDbStatement* selectAllValueFacts {nullptr};
      cDbStatement* selectAllConfig {nullptr};
      cDbStatement* selectAllUser {nullptr};
      cDbStatement* selectMaxTime {nullptr};
      cDbStatement* selectSamplesRange {nullptr};     // for chart
      cDbStatement* selectSamplesRange60 {nullptr};   // for chart
      cDbStatement* selectScriptByPath {nullptr};
      cDbStatement* selectScripts {nullptr};
      cDbStatement* selectSensorAlerts {nullptr};
      cDbStatement* selectAllSensorAlerts {nullptr};
      cDbStatement* selectSampleInRange {nullptr};    // for alert check

      cDbStatement* selectDashboards {nullptr};
      cDbStatement* selectDashboardById {nullptr};
      cDbStatement* selectDashboardWidgetsFor {nullptr};
      cDbStatement* selectSchemaConfByState {nullptr};
      cDbStatement* selectAllSchemaConf {nullptr};
      cDbStatement* selectHomeMaticByUuid {nullptr};

      cDbValue xmlTime;
      cDbValue rangeFrom;
      cDbValue rangeTo;
      cDbValue avgValue;
      cDbValue maxValue;
      cDbValue rangeEnd;

      time_t nextRefreshAt {0};
      time_t startedAt {0};
      time_t nextAggregateAt {0};
      time_t lastSampleTime {0};

      std::vector<std::string> addrsDashboard;
      std::vector<std::string> addrsList;

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
         json_t* oHaJson {nullptr};
      };

      char* mqttUrl {nullptr};
      char* mqttUser {nullptr};
      char* mqttPassword {nullptr};

      char* mqttDataTopic {nullptr};
      char* mqttSendWithKeyPrefix {nullptr};
      bool mqttHaveConfigTopic {false};
      MqttInterfaceStyle mqttInterfaceStyle {misNone};

      Mqtt* mqttReader {nullptr};
      Mqtt* mqttWriter {nullptr};
      std::vector<std::string> mqttSensorTopics;
      std::vector<std::string> configTopicsFor;

      time_t lastMqttConnectAt {0};
      std::map<std::string,std::string> hassCmdTopicMap; // 'topic' to 'name' map

      json_t* oHaJson {nullptr};
      std::map<int,Group> groups;

      // config

      double latitude {50.30};
      double longitude {8.79};
      char* openWeatherApiKey {nullptr};
      int interval {60};
      int arduinoInterval {10};

      int webPort {0};
      char* webUrl {nullptr};
      bool webSsl {false};
      char* iconSet {nullptr};
      int aggregateInterval {15};         // aggregate interval in minutes
      int aggregateHistory {0};           // history in days

      int mail {no};
      char* mailScript {nullptr};
      char* stateMailTo {nullptr};
      char* errorMailTo {nullptr};
      std::string htmlHeader;
      int invertDO {no};

      Deconz deconz;
      bool homeMaticInterface {false};
      std::map<uint,std::string> homeMaticUuids;

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

      std::map<int,AiSensorData> aiSensors;   // #TODO #FIXME -> to be ported to sensors!!
      std::map<std::string,std::map<int,SensorData>> sensors;

      virtual std::list<ConfigItemDef>* getConfiguration() = 0;

      std::string alertMailBody;
      std::string alertMailSubject;

      std::map<std::string,json_t*> jsonSensorList;

      // statics

      static bool shutdown;
};
