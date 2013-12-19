//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File p4sd.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 08.12.2013  Jörg Wendel
//***************************************************************************

#ifndef _P4SD_H_
#define _P4SD_H_

//***************************************************************************
// Includes
//***************************************************************************

#include "service.h"
#include "p4io.h"

#include "lib/tabledef.h"

#define VERSION "0.0.4"
#define confDirDefault "/etc"

extern char dbHost[];
extern int  dbPort;
extern char dbName[];
extern char dbUser[];
extern char dbPass[];

extern char webUser[];
extern char webPass[];

extern char ttyDevice[];
extern char ttyDeviceSvc[];
extern int  interval;
extern int  stateCheckInterval;

extern int  mail;
extern char mailScript[];
extern char stateMailAtStates[];
extern char stateMailTo[];
extern char errorMailTo[];

//***************************************************************************
// Class P4sd
//***************************************************************************

class P4sd : public FroelingService
{
   public:

      // object

      P4sd();
      ~P4sd();

      int init();
	   int loop();
	   int setup();
	   int initialize(int truncate = no);

      static void downF(int aSignal) { shutdown = yes; }

   protected:

      int exit();
      int initDb();
      int exitDb();

      int standby(int t);
      int standbyUntil(time_t until);
      int meanwhile();

      int update();
      int performWebifRequests();
      int cleanupWebifRequests();

      int store(time_t now, const char* type, long address, double value, 
                unsigned int factor, const char* text = 0);
      int sendMail();
      int updateSchemaConfTable();
      int updateValueFacts();
      int updateMenu();
      int haveMailState();

      int getConfigItem(const char* name, char* value);
      int setConfigItem(const char* name, const char* value);
      
      int doShutDown() { return shutdown; }

      // data

      cDbConnection* connection;

      cTableSamples* tableSamples;
      cTableValueFacts* tableValueFacts;
      cTableMenu* tableMenu;
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

      static int shutdown;
};

//***************************************************************************
#endif // _P4SD_H_
