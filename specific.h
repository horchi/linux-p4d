//***************************************************************************
// Automation Control
// File specific.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 16.04.2020 - Jörg Wendel
//***************************************************************************

#pragma once

#include "daemon.h"

//***************************************************************************
// Class P4d
//***************************************************************************

class P4d : public Daemon
{
   public:

      P4d();
      virtual ~P4d();

      const char* myTitle() override { return "Heizung"; }
      int init() override;

      enum PinMappings
      {
      };

      enum AnalogInputs
      {
      };

      enum SpecialValues  // 'SP'
      {
      };

      int setup();
      int initialize(bool truncate = false);

   protected:

      int initDb() override;
      int exitDb() override;

      int readConfiguration(bool initial) override;
      int applyConfigurationSpecials() override;
      int loadStates() override;
      int atMeanwhile() override;

      void afterUpdate() override;
      int updateErrors();
      int sendErrorMail();
      int process() override;
      int performJobs() override;
      void logReport() override;
      void onIoSettingsChange() override { updateSchemaConfTable(); }

      int dispatchMqttCommandRequest(json_t* jData, const char* topic) override;

      std::list<ConfigItemDef>* getConfiguration() override { return &configuration; }

      int updateTimeRangeData();
      int initMenu(bool updateParameters = false);
      int updateParameter(cDbTable* tableMenu);
      int calcStateDuration();
      void sensorAlertCheck(time_t now);
      int performAlertCheck(cDbRow* alertRow, time_t now, int recurse = 0, int force = no);
      int add2AlertMail(cDbRow* alertRow, const char* title, double value, const char* unit);
      int sendAlertMail(const char* to);
      int initValueFacts(bool truncate = false);
      int updateSchemaConfTable();

      // WS request

      int dispatchSpecialRequest(Event event, json_t* oObject, long client) override;
      int performInitTables(json_t* oObject, long client);
      int performUpdateTimeRanges(json_t* array, long client);
      int performPellets(json_t* array, long client);
      int performPelletsAdd(json_t* array, long client);

      int performErrors(long client);
      int performMenu(json_t* oObject, long client);
      int performSchema(json_t* oObject, long client);
      int storeAlerts(json_t* oObject, long client);
      int performAlerts(json_t* oObject, long client);
      int performAlertTestMail(int id, long client) override;
      int performParEditRequest(json_t* oObject, long client);
      int performTimeParEditRequest(json_t* oObject, long client);
      int performParStore(json_t* oObject, long client);
      int performTimeParStore(json_t* oObject, long client);
      int storeSchema(json_t* oObject, long client);

      // data

      cDbTable* tablePellets {nullptr};
      cDbTable* tableMenu {nullptr};
      cDbTable* tableSensorAlert {nullptr};
      cDbTable* tableSchemaConf {nullptr};
      cDbTable* tableErrors {nullptr};
      cDbTable* tableTimeRanges {nullptr};

      cDbValue minValue;
      cDbValue endTime;
      cDbValue rangeEnd;

      cDbStatement* selectAllMenuItems {nullptr};
      cDbStatement* selectMenuItemsByParent {nullptr};
      cDbStatement* selectMenuItemsByChild {nullptr};
      cDbStatement* selectSensorAlerts {nullptr};
      cDbStatement* selectAllSensorAlerts {nullptr};
      cDbStatement* selectSampleInRange {nullptr};    // for alert check
      cDbStatement* selectAllErrors {nullptr};
      cDbStatement* selectPendingErrors {nullptr};

      cDbStatement* selectAllPellets {nullptr};
      cDbStatement* selectStokerHours {nullptr};
      cDbStatement* selectStateDuration {nullptr};
      cDbStatement* selectSchemaConfByState {nullptr};
      cDbStatement* selectAllSchemaConf {nullptr};

      int errorsPending {0};

      // config

      // statics

      static std::list<ConfigItemDef> configuration;
};
