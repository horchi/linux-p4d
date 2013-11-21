//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File p4d.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
// Date 04.11.2010 - 19.11.2013  Jörg Wendel
//***************************************************************************

#ifndef _P4D_H_
#define _P4D_H_

//***************************************************************************
// Includes
//***************************************************************************

#include "service.h"
#include "p4io.h"

#include "lib/tabledef.h"

#define VERSION "0.0.1"
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
// Class
//***************************************************************************

class Linpellet : public FroelingService
{
   public:

      // object

      Linpellet();
      ~Linpellet();

	   int loop();
	   int setup() { return done; }

      static void downF(int aSignal) { shutdown = yes; }

   protected:

      int initDb();
      int exitDb();
      int store(time_t now, Parameter* p);
      int storeSensor(Parameter* p);
      int sendMail();

      int doShutDown() { return shutdown; }

      // data

      cTableSamples* table;
      cTableSensorFacts* tableSensors;
      cDbConnection* connection;
      P4Packet* tlg;

      static int shutdown;
};

//***************************************************************************
#endif // _P4D_H_
