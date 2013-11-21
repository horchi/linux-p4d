//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File p4sd.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
// Date 04.11.2010 - 19.11.2013  Jörg Wendel
//***************************************************************************

#ifndef _P4SD_H_
#define _P4SD_H_

//***************************************************************************
// Includes
//***************************************************************************

#include "service.h"
#include "p4io.h"

#include "lib/tabledef.h"

#define VERSION "0.0.2"
#define confDirDefault "/etc"

extern char dbHost[];
extern int  dbPort;
extern char dbName[];
extern char dbUser[];
extern char dbPass[];

extern char ttyDevice[];
extern char ttyDeviceSvc[];
extern int  interval;

extern int  mail;
extern char mailScript[];
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

	   int loop();
	   int setup();

      static void downF(int aSignal) { shutdown = yes; }

   protected:

      int init();
      int exit();
      int initDb();
      int exitDb();

      int store(time_t now, Value* v, unsigned int factor);
      int sendMail();
      int updateValueFacts();

      int doShutDown() { return shutdown; }

      // data

      cDbConnection* connection;

      cTableSamples* tableSamples;
      cTableValueFacts* tableValueFacts;
      cTableSensorFacts* tableSensors;

      cDbStatement* selectActiveValues;

      P4Request* request;
      Serial* serial;    

      static int shutdown;
};

//***************************************************************************
#endif // _P4SD_H_
