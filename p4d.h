//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File p4d.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 05.03.2015  Jörg Wendel
//***************************************************************************

#ifndef _P4D_H_
#define _P4D_H_

//***************************************************************************
// Includes
//***************************************************************************

#include "lib/tabledef.h"

#include "service.h"
#include "p4io.h"
#include "w1.h"

#define VERSION "0.1.18"
#define confDirDefault "/etc"

extern char dbHost[];
extern int  dbPort;
extern char dbName[];
extern char dbUser[];
extern char dbPass[];

extern char ttyDeviceSvc[];
extern int  interval;
extern int  stateCheckInterval;
extern int aggregateInterval;        // aggregate interval in minutes
extern int aggregateHistory;         // history in days

extern int htmlMail;

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
      int scheduleAggregate();
      int aggregate();

      int updateErrors();
      int performWebifRequests();
      int cleanupWebifRequests();

      int store(time_t now, const char* type, unsigned int address, double value, 
                unsigned int factor, const char* text = 0);

      void addParameter2Mail(const char* name, const char* value);
      void sensorAlertCheck(const char* type, unsigned int addr, const char* title, double value, const char* unit);

      int sendMail();
      int sendAlertMail(cDbRow* alerRow, const char* title, double value, const char* unit);

      int updateSchemaConfTable();
      int updateValueFacts();
      int updateMenu();
      int isMailState();

      int getConfigItem(const char* name, char*& value, const char* def = "");
      int setConfigItem(const char* name, const char* value);
      int getConfigItem(const char* name, int& value, int def = na);
      int setConfigItem(const char* name, int value);
      
      int doShutDown() { return shutdown; }

      // data

      cDbConnection* connection;

      cTableSamples* tableSamples;
      cTableValueFacts* tableValueFacts;
      cTableMenu* tableMenu;
      cTableErrors* tableErrors;
      cTableJobs* tableJobs;
      cTableSensorAlert* tableSensorAlert;
      cTableSchemaConf* tableSchemaConf;
      cTableConfig* tableConfig;

      cDbStatement* selectActiveValueFacts;
      cDbStatement* selectAllValueFacts;
      cDbStatement* selectPendingJobs;
      cDbStatement* selectAllMenuItems;
      cDbStatement* selectSensorAlerts;
      cDbStatement* cleanupJobs;

      time_t nextAt;
      time_t startedAt;
      Sem* sem;

      P4Request* request;
      Serial* serial;    

      W1 w1;                       // for one wire sensors

      Status currentState;
      string mailBody;
      string mailBodyHtml;

      // config

      int mail;
      char* mailScript;
      char* stateMailAtStates;
      char* stateMailTo;
      char* errorMailTo;
      int tSync;
      int maxTimeLeak;

      time_t nextAggregateAt;

      // 

      static int shutdown;
};

//***************************************************************************
#endif // _P4D_H_
