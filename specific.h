//***************************************************************************
// Automation Control
// File specific.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 16.04.2020 - Jörg Wendel
//***************************************************************************

#pragma once

#include "daemon.h"
#include "p4io.h"

//***************************************************************************
// Class P4d
//***************************************************************************

class P4d : public Daemon, public FroelingService
{
   public:

      P4d();
      virtual ~P4d();

      const char* myTitle() override { return "Heizung"; }
      int init() override;
      int exit() override;

      enum PinMappings
      {
      };

      enum AnalogInputs
      {
      };

      enum SpecialValues  // 'SP'
      {
      };

   protected:

      int initDb() override;
      int exitDb() override;

      int standbyUntil() override;
      int readConfiguration(bool initial) override;
      int applyConfigurationSpecials() override;
      int loadStates() override;
      int atMeanwhile() override;

      int updateSensors() override;
      void afterUpdate() override;
      int updateErrors();
      int doLoop() override;
      int updateState();
      void scheduleTimeSyncIn(int offset = 0);
      int sendErrorMail();
      int sendStateMail();
      int isMailState();

      int process() override;
      int performJobs() override;
      void logReport() override;
      bool onCheckRights(long client, Event event, uint rights) override;
      int dispatchOther(const char* topic, const char* message) override;
      int dispatchNodeRedCommand(json_t* jObject) override;
      int dispatchMqttS3200CommandRequest(json_t* jData, const char* topic);
      std::list<ConfigItemDef>* getConfiguration() override { return &configuration; }

      int updateTimeRangeData();
      int initMenu(bool updateParameters = false);
      int updateParameter(cDbTable* tableMenu);
      int calcStateDuration();

      int initValueFacts(bool truncate = false);
      const char* getTextImage(const char* key, const char* text) override;

      // WS request

      int dispatchSpecialRequest(Event event, json_t* oObject, long client) override;
      int performLogin(json_t* oObject) override;
      int performPellets(json_t* array, long client);
      int performPelletsAdd(json_t* array, long client);
      int performCommand(json_t* obj, long client) override;
      int performErrors(long client);
      int performMenu(json_t* oObject, long client);
      int performMenuByParent(json_t* oObject, long client, int parent);
      int performMenuBySearch(json_t* oObject, long client, const char* search);
      int performParEditRequest(json_t* oObject, long client);
      int performTimeParEditRequest(json_t* oObject, long client);
      int performParStore(json_t* oObject, long client);
      int performTimeParStore(json_t* oObject, long client);

      int commands2Json(json_t* obj) override;
      int s3200State2Json(json_t* obj);
      int configChoice2json(json_t* obj, const char* name) override;

      // config

      int stateCheckInterval {10};
      char* ttyDevice {nullptr};

      int tSync {no};
      int maxTimeLeak {10};
      char* stateMailAtStates {nullptr};
      double consumptionPerHour {0};
      char* knownStates {nullptr};
      char* heatingType {nullptr};

      // data

      cDbTable* tablePellets {nullptr};
      cDbTable* tableMenu {nullptr};
      cDbTable* tableErrors {nullptr};
      cDbTable* tableTimeRanges {nullptr};

      cDbValue minValue;
      cDbValue endTime;

      cDbStatement* selectAllMenuItems {nullptr};
      cDbStatement* selectMenuItemsByParent {nullptr};
      cDbStatement* selectMenuItemsByChild {nullptr};
      cDbStatement* selectAllErrors {nullptr};
      cDbStatement* selectPendingErrors {nullptr};

      cDbStatement* selectAllPellets {nullptr};
      cDbStatement* selectStokerHours {nullptr};
      cDbStatement* selectStateDuration {nullptr};

      Sem* sem {nullptr};
      P4Request* request {nullptr};
      Serial* serial {nullptr};
      Status currentState;
      bool stateChanged {false};

      time_t nextStateAt {0};
      std::map<int,time_t> stateDurations;
      int errorsPending {0};
      time_t nextTimeSyncAt {0};

      // statics

      static std::list<ConfigItemDef> configuration;
};
