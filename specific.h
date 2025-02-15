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
      // with P4d we did not have 'DI' from Raspi (type name chash of 'DI')
      int updateInputs(bool check = true) override { return done; }
      int updateState();
      void scheduleTimeSyncIn(int offset = 0);
      int sendErrorMail();
      int sendStateMail();
      int isMailState();

      int process(bool force = false) override;
      int performJobs() override;
      void logReport() override;
      bool onCheckRights(long client, Event event, uint rights) override;
      int dispatchOther(const char* topic, const char* message) override;
      int dispatchMqttS3200CommandRequest(json_t* jData);

      int parGet(cDbTable* tableMenu, std::string& error, int result = success);
      int parSet(cDbTable* tableMenu, const char* value, std::string& error);
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
      char* ttyDevice {};

      int tSync {no};
      int maxTimeLeak {10};
      char* stateMailAtStates {};
      double consumptionPerHour {0};
      char* knownStates {};
      char* heatingType {};

      // data

      cDbTable* tablePellets {};
      cDbTable* tableMenu {};
      cDbTable* tableErrors {};
      cDbTable* tableTimeRanges {};

      cDbValue minValue;
      cDbValue endTime;

      cDbStatement* selectAllMenuItems {};
      cDbStatement* selectMenuItemsByParent {};
      cDbStatement* selectMenuItemsByChild {};
      cDbStatement* selectAllErrors {};
      cDbStatement* selectPendingErrors {};

      cDbStatement* selectAllPellets {};
      cDbStatement* selectStokerHours {};
      cDbStatement* selectStateDuration {};

      Sem* sem {};
      P4Request* request {};
      Serial* serial {};
      Status currentState;
      bool stateChanged {false};

      time_t nextStateAt {0};
      std::map<int,time_t> stateDurations;
      int errorsPending {0};
      time_t nextTimeSyncAt {0};

      static std::list<ConfigItemDef> configuration;
};
