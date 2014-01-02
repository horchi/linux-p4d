//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File p4d.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 24.12.2013  Jörg Wendel
//***************************************************************************

#ifndef _P4D_H_
#define _P4D_H_

//***************************************************************************
// Includes
//***************************************************************************

#include "service.h"
#include "p4io.h"

#include "lib/tabledef.h"

#define VERSION "0.1.1"
#define confDirDefault "/etc"

extern char dbHost[];
extern int  dbPort;
extern char dbName[];
extern char dbUser[];
extern char dbPass[];

extern char ttyDeviceSvc[];
extern int  interval;
extern int  stateCheckInterval;

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

      int updateErrors();
      int performWebifRequests();
      int cleanupWebifRequests();

      int store(time_t now, const char* type, long address, double value, 
                unsigned int factor, const char* text = 0);
      int sendMail();
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
      cTableSchemaConf* tableSchemaConf;
      cTableConfig* tableConfig;

      cDbStatement* selectActiveValueFacts;
      cDbStatement* selectAllValueFacts;
      cDbStatement* selectPendingJobs;
      cDbStatement* selectAllMenuItems;
      cDbStatement* cleanupJobs;

      time_t nextAt;
      Sem* sem;
      P4Request* request;
      Serial* serial;    

      Status currentState;
      string mailBody;

      // config

      int mail;
      char* mailScript;
      char* stateMailAtStates;
      char* stateMailTo;
      char* errorMailTo;
      int tSync;
      int maxTimeLeak;

      // 

      static int shutdown;
};

//***************************************************************************
#endif // _P4D_H_
