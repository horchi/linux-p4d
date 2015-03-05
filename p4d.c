//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File p4d.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 05.03.2015  Jörg Wendel
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <stdio.h>
#include <unistd.h>

#include "p4d.h"

int P4d::shutdown = no;

//***************************************************************************
// Object
//***************************************************************************

P4d::P4d()
{
   connection = 0;
   tableSamples = 0;
   tableJobs = 0;
   tableSensorAlert = 0;
   tableSchemaConf = 0;
   tableValueFacts = 0;
   tableMenu = 0;
   tableConfig = 0;
   tableErrors = 0;

   selectActiveValueFacts = 0;
   selectAllValueFacts = 0;
   selectPendingJobs = 0;
   selectAllMenuItems = 0;
   selectSensorAlerts = 0;
   cleanupJobs = 0;

   nextAt = time(0);
   startedAt = time(0);
   nextAggregateAt = 0;

   mailBody = "";
   mailBodyHtml = "";
   mail = no;
   mailScript = 0;
   stateMailAtStates = 0;
   stateMailTo = 0;
   errorMailTo = 0;
   tSync = no;
   maxTimeLeak = 10;

   cDbConnection::init();
   cDbConnection::setEncoding("utf8");
   cDbConnection::setHost(dbHost);
   cDbConnection::setPort(dbPort);
   cDbConnection::setName(dbName);
   cDbConnection::setUser(dbUser);
   cDbConnection::setPass(dbPass);

   sem = new Sem(0x3da00001);
   serial = new Serial;
   request = new P4Request(serial);
}

P4d::~P4d()
{
   exit();

   free(mailScript);
   free(stateMailAtStates);
   free(stateMailTo);
   free(errorMailTo);

   delete serial;
   delete request;
   delete sem;

   cDbConnection::exit();
}

//***************************************************************************
// Init / Exit
//***************************************************************************

int P4d::init()
{
   if (initDb() != success)
      return fail;

   readConfiguration();

   w1.scan();

   return success;
}

int P4d::exit()
{
   exitDb();
   serial->close();

   return success;
}

//***************************************************************************
// Init/Exit Database
//***************************************************************************

int P4d::initDb()
{
   int status = success;

   if (connection)
      exitDb();

   tell(eloAlways, "Try conneting to database");

   connection = new cDbConnection();

   tableValueFacts = new cTableValueFacts(connection);
   if (tableValueFacts->open() != success) return fail;

   tableErrors = new cTableErrors(connection);
   if (tableErrors->open() != success) return fail;

   tableMenu = new cTableMenu(connection);
   if (tableMenu->open() != success) return fail;

   tableSamples = new cTableSamples(connection);
   if (tableSamples->open() != success) return fail;

   tableJobs = new cTableJobs(connection);
   if (tableJobs->open() != success) return fail;

   tableSensorAlert = new cTableSensorAlert(connection);
   if (tableSensorAlert->open() != success) return fail;

   tableSchemaConf = new cTableSchemaConf(connection);
   if (tableSchemaConf->open() != success) return fail;

   tableConfig = new cTableConfig(connection);
   if (tableConfig->open() != success) return fail;

   // prepare statements

   selectActiveValueFacts = new cDbStatement(tableValueFacts);

   selectActiveValueFacts->build("select ");
   selectActiveValueFacts->bind(cTableValueFacts::fiAddress, cDBS::bndOut);
   selectActiveValueFacts->bind(cTableValueFacts::fiType, cDBS::bndOut, ", ");
   selectActiveValueFacts->bind(cTableValueFacts::fiUnit, cDBS::bndOut, ", ");
   selectActiveValueFacts->bind(cTableValueFacts::fiFactor, cDBS::bndOut, ", ");
   selectActiveValueFacts->bind(cTableValueFacts::fiTitle, cDBS::bndOut, ", ");
   selectActiveValueFacts->bind(cTableValueFacts::fiName, cDBS::bndOut, ", ");
   selectActiveValueFacts->build(" from %s where ", tableValueFacts->TableName());
   selectActiveValueFacts->bind(cTableValueFacts::fiState, cDBS::bndIn | cDBS::bndSet);

   status += selectActiveValueFacts->prepare();

   // ------------------

   selectAllValueFacts = new cDbStatement(tableValueFacts);

   selectAllValueFacts->build("select ");
   selectAllValueFacts->bind(cTableValueFacts::fiAddress, cDBS::bndOut);
   selectAllValueFacts->bind(cTableValueFacts::fiType, cDBS::bndOut, ", ");
   selectAllValueFacts->bind(cTableValueFacts::fiState, cDBS::bndOut, ", ");
   selectAllValueFacts->bind(cTableValueFacts::fiUnit, cDBS::bndOut, ", ");
   selectAllValueFacts->bind(cTableValueFacts::fiFactor, cDBS::bndOut, ", ");
   selectAllValueFacts->bind(cTableValueFacts::fiTitle, cDBS::bndOut, ", ");
   selectAllValueFacts->build(" from %s", tableValueFacts->TableName());

   status += selectAllValueFacts->prepare();

   // ----------------

   selectAllMenuItems = new cDbStatement(tableMenu);

   selectAllMenuItems->build("select ");
   selectAllMenuItems->bind(cTableMenu::fiId, cDBS::bndOut);
   selectAllMenuItems->bind(cTableMenu::fiAddress, cDBS::bndOut, ", ");
   selectAllMenuItems->bind(cTableMenu::fiType, cDBS::bndOut, ", ");
   selectAllMenuItems->bind(cTableMenu::fiUnit, cDBS::bndOut, ", ");
   selectAllMenuItems->bind(cTableMenu::fiValue, cDBS::bndOut, ", ");
   selectAllMenuItems->bind(cTableMenu::fiTitle, cDBS::bndOut, ", ");
   selectAllMenuItems->build(" from %s", tableMenu->TableName());

   status += selectAllMenuItems->prepare();

   // ------------------

   selectPendingJobs = new cDbStatement(tableJobs);

   selectPendingJobs->build("select ");
   selectPendingJobs->bind(cTableJobs::fiId, cDBS::bndOut);
   selectPendingJobs->bind(cTableJobs::fiReqAt, cDBS::bndOut, ", ");
   selectPendingJobs->bind(cTableJobs::fiState, cDBS::bndOut, ", ");
   selectPendingJobs->bind(cTableJobs::fiCommand, cDBS::bndOut, ", ");
   selectPendingJobs->bind(cTableJobs::fiAddress, cDBS::bndOut, ", ");
   selectPendingJobs->bind(cTableJobs::fiData, cDBS::bndOut, ", ");
   selectPendingJobs->build(" from %s where state = 'P'", tableJobs->TableName());

   status += selectPendingJobs->prepare();

   // ------------------

   selectSensorAlerts = new cDbStatement(tableSensorAlert);

   selectSensorAlerts->build("select ");
   selectSensorAlerts->bind(cTableSensorAlert::fiId, cDBS::bndOut);
   selectSensorAlerts->bind(cTableSensorAlert::fiAddress, cDBS::bndOut, ", ");
   selectSensorAlerts->bind(cTableSensorAlert::fiType, cDBS::bndOut, ", ");
   selectSensorAlerts->bind(cTableSensorAlert::fiMin, cDBS::bndOut, ", ");
   selectSensorAlerts->bind(cTableSensorAlert::fiMax, cDBS::bndOut, ", ");
   selectSensorAlerts->bind(cTableSensorAlert::fiRangeM, cDBS::bndOut, ", ");
   selectSensorAlerts->bind(cTableSensorAlert::fiDelta, cDBS::bndOut, ", ");
   selectSensorAlerts->bind(cTableSensorAlert::fiMAddress, cDBS::bndOut, ", ");
   selectSensorAlerts->bind(cTableSensorAlert::fiMSubject, cDBS::bndOut, ", ");
   selectSensorAlerts->bind(cTableSensorAlert::fiMBody, cDBS::bndOut, ", ");
   selectSensorAlerts->build(" from %s where state = 'A'", tableSensorAlert->TableName());
   selectSensorAlerts->bind(cTableSensorAlert::fiAddress, cDBS::bndIn | cDBS::bndSet, " and ");
   selectSensorAlerts->bind(cTableSensorAlert::fiType, cDBS::bndIn | cDBS::bndSet, " and ");

   status += selectSensorAlerts->prepare();

   // ------------------

   cleanupJobs = new cDbStatement(tableJobs);

   cleanupJobs->build("delete from %s where ", tableJobs->TableName());
   cleanupJobs->bindCmp(0, cTableJobs::fiReqAt, 0, "<");

   status += cleanupJobs->prepare();
   
   // ------------------

   if (status == success)
      tell(eloAlways, "Connection to database established");  

   return status;
}

int P4d::exitDb()
{
   delete tableSamples;            tableSamples = 0;
   delete tableValueFacts;         tableValueFacts = 0;
   delete tableMenu;               tableMenu = 0;
   delete tableJobs;               tableJobs = 0;
   delete tableSensorAlert;        tableSensorAlert = 0;
   delete tableSchemaConf;         tableSchemaConf = 0;
   delete tableErrors;             tableErrors = 0;
   delete tableConfig;             tableConfig = 0;

   delete selectActiveValueFacts;  selectActiveValueFacts = 0;
   delete selectAllValueFacts;     selectAllValueFacts = 0;
   delete selectPendingJobs;       selectPendingJobs = 0;
   delete selectAllMenuItems;      selectAllMenuItems = 0;
   delete selectSensorAlerts;      selectSensorAlerts = 0;
   delete cleanupJobs;             cleanupJobs = 0;

   delete connection;              connection = 0;

   return done;
}

//***************************************************************************
// Read Configuration
//***************************************************************************

int P4d::readConfiguration()
{
   char* webUser = 0;
   char* webPass = 0;
   md5Buf defaultPwd;
   
   // init default user and password

   createMd5("p4-3200", defaultPwd);
   getConfigItem("user", webUser, "p4");
   getConfigItem("passwd", webPass, defaultPwd);

   // init configuration

   getConfigItem("mail", mail, no);
   getConfigItem("mailScript", mailScript, "/usr/local/bin/p4d-mail.sh");
   getConfigItem("stateMailStates", stateMailAtStates, "0,1,3,19");
   getConfigItem("stateMailTo", stateMailTo);
   getConfigItem("errorMailTo", errorMailTo);

   getConfigItem("tsync", tSync, no);
   getConfigItem("maxTimeLeak", maxTimeLeak, 10);

   return done;
}

//***************************************************************************
// Initialize
//***************************************************************************

int P4d::initialize(int truncate)
{
   if (!connection)
      return fail;

   // truncate config tables ?

   if (truncate)
   {
      tell(eloAlways, "Truncate configuration tables!");
      
      tableValueFacts->truncate();
      tableSchemaConf->truncate();
      tableMenu->truncate();
   }

   sem->p();

   if (serial->open(ttyDeviceSvc) != success)
   {
      sem->v();
      return fail;
   }

   tell(eloAlways, "Requesting value facts from s 3200");
   updateValueFacts();
   tell(eloAlways, "Update html schema configuration");
   updateSchemaConfTable();
   tell(eloAlways, "Requesting menu structure from s 3200");
   updateMenu();

   serial->close();

   sem->v();

   return done;
}

//***************************************************************************
// Setup
//***************************************************************************

int P4d::setup()
{
   if (!connection)
      return fail;

   for (int f = selectAllValueFacts->find(); f; f = selectAllValueFacts->fetch())
   {
      char* res;
      char buf[100+TB]; *buf = 0;
      char oldState = tableValueFacts->getStrValue(cTableValueFacts::fiState)[0];
      char state = oldState;

      printf("%s 0x%04llx '%s' (%s)", 
             tableValueFacts->getStrValue(cTableValueFacts::fiType),
             tableValueFacts->getIntValue(cTableValueFacts::fiAddress),
             tableValueFacts->getStrValue(cTableValueFacts::fiUnit),
             tableValueFacts->getStrValue(cTableValueFacts::fiTitle));

      printf(" - aufzeichnen? (%s): ", oldState == 'A' ? "Y/n" : "y/N");
            
      if ((res = fgets(buf, 100, stdin)) && strlen(res) > 1)
         state = toupper(res[0]) == 'Y' ? 'A' : 'D';

      if (state != oldState && tableValueFacts->find())
      {
         tableValueFacts->setCharValue(cTableValueFacts::fiState, state);
         tableValueFacts->store();
      }
   }

   selectAllValueFacts->freeResult();

   tell(eloAlways, "Update html schema configuration");
   updateSchemaConfTable();

   return done;
}

//***************************************************************************
// Update Conf Tables
//***************************************************************************

int P4d::updateSchemaConfTable()
{
   const int step = 20;
   int y = 50;
   int added = 0;

   for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
   {
      unsigned int addr = tableValueFacts->getIntValue(cTableValueFacts::fiAddress);
      const char* type = tableValueFacts->getStrValue(cTableValueFacts::fiType);
      y += step;

      tableSchemaConf->clear();
      tableSchemaConf->setValue(cTableSchemaConf::fiAddress, addr);
      tableSchemaConf->setValue(cTableSchemaConf::fiType, type);
   
      if (!tableSchemaConf->find())
      {
         tableSchemaConf->setValue(cTableSchemaConf::fiKind, "value");
         tableSchemaConf->setValue(cTableSchemaConf::fiState, "A");
         tableSchemaConf->setValue(cTableSchemaConf::fiColor, "black");
         tableSchemaConf->setValue(cTableSchemaConf::fiXPos, 12);
         tableSchemaConf->setValue(cTableSchemaConf::fiYPos, y);

         tableSchemaConf->store();
         added++;
      }
   }

   selectActiveValueFacts->freeResult();
   tell(eloAlways, "Added %d html schema configurations", added);

   return success;
}

//***************************************************************************
// Update Value Facts
//***************************************************************************

int P4d::updateValueFacts()
{
   int status;
   Fs::ValueSpec v;
   int count;
   int added;

   // check serial communication

   if (request->check() != success)
   {
      serial->close();
      return fail;
   }

   // ---------------------------------
   // Add the sensor definitions delivered by the S 3200

   count = 0;
   added = 0;

   for (status = request->getFirstValueSpec(&v); status != Fs::wrnLast; 
        status = request->getNextValueSpec(&v))
   {
      if (status != success)
         continue;

      tell(eloDebug, "%3d) 0x%04x %4d '%s' (%04d) '%s'",
           count, v.address, v.factor, v.unit, v.unknown, v.description);
      
      // update table
      
      tableValueFacts->clear();
      tableValueFacts->setValue(cTableValueFacts::fiAddress, v.address);
      tableValueFacts->setValue(cTableValueFacts::fiType, "VA");
      
      if (!tableValueFacts->find())
      {
         tableValueFacts->setValue(cTableValueFacts::fiName, v.name);
         tableValueFacts->setValue(cTableValueFacts::fiState, "D");
         tableValueFacts->setValue(cTableValueFacts::fiUnit, v.unit);
         tableValueFacts->setValue(cTableValueFacts::fiFactor, v.factor);
         tableValueFacts->setValue(cTableValueFacts::fiTitle, v.description);
         tableValueFacts->setValue(cTableValueFacts::fiRes1, v.unknown);
         
         tableValueFacts->store();
         added++;
      }

      count++;
   }

   tell(eloAlways, "Read %d value facts, added %d", count, added);

   // ---------------------------------
   // add default for digital outputs

   added = 0;
   count = 0;

   for (int f = selectAllMenuItems->find(); f; f = selectAllMenuItems->fetch())
   {
      const char* type = 0;
      int structType = tableMenu->getIntValue(cTableMenu::fiType);

      count++;

      switch (structType)
      {
         case mstDigOut: type = "DO"; break;
         case mstDigIn:  type = "DI"; break;
         case mstAnlOut: type = "AO"; break;
      }

      if (type)
      {
         // update table
         
         tableValueFacts->clear();
         tableValueFacts->setValue(cTableValueFacts::fiAddress, tableMenu->getIntValue(cTableMenu::fiAddress));
         tableValueFacts->setValue(cTableValueFacts::fiType, type);
         
         if (!tableValueFacts->find())
         {
            string name = tableMenu->getStrValue(cTableMenu::fiTitle);

            removeCharsExcept(name, nameChars);
            tableValueFacts->setValue(cTableValueFacts::fiName, name.c_str());
            tableValueFacts->setValue(cTableValueFacts::fiState, "D");
            tableValueFacts->setValue(cTableValueFacts::fiUnit, tableMenu->getStrValue(cTableMenu::fiUnit));
            tableValueFacts->setValue(cTableValueFacts::fiFactor, 1);
            tableValueFacts->setValue(cTableValueFacts::fiTitle, tableMenu->getStrValue(cTableMenu::fiTitle));
            
            tableValueFacts->store();
            added++;
         }
      }
   }

   selectAllMenuItems->freeResult();
   tell(eloAlways, "Checked %d digital lines, added %d", count, added);

   // ---------------------------------
   // at least add value definitions for special data

   count = 0;
   added = 0;

   tableValueFacts->clear();
   tableValueFacts->setValue(cTableValueFacts::fiAddress, udState);      // 1  -> Kessel Status
   tableValueFacts->setValue(cTableValueFacts::fiType, "UD");            // UD -> User Defined
   
   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue(cTableValueFacts::fiName, "Status");
      tableValueFacts->setValue(cTableValueFacts::fiState, "A");
      tableValueFacts->setValue(cTableValueFacts::fiUnit, "zst");
      tableValueFacts->setValue(cTableValueFacts::fiFactor, 1);
      tableValueFacts->setValue(cTableValueFacts::fiTitle, "Heizungsstatus");
      
      tableValueFacts->store();
      added++;
   }

   tableValueFacts->clear();
   tableValueFacts->setValue(cTableValueFacts::fiAddress, udMode);       // 2  -> Kessel Mode
   tableValueFacts->setValue(cTableValueFacts::fiType, "UD");            // UD -> User Defined
   
   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue(cTableValueFacts::fiName, "Betriebsmodus");
      tableValueFacts->setValue(cTableValueFacts::fiState, "A");
      tableValueFacts->setValue(cTableValueFacts::fiUnit, "zst");
      tableValueFacts->setValue(cTableValueFacts::fiFactor, 1);
      tableValueFacts->setValue(cTableValueFacts::fiTitle, "Betriebsmodus");
      
      tableValueFacts->store();
      added++;
   }

   tableValueFacts->clear();
   tableValueFacts->setValue(cTableValueFacts::fiAddress, udTime);       // 3  -> Kessel Zeit
   tableValueFacts->setValue(cTableValueFacts::fiType, "UD");            // UD -> User Defined
   
   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue(cTableValueFacts::fiName, "Uhrzeit");
      tableValueFacts->setValue(cTableValueFacts::fiState, "A");
      tableValueFacts->setValue(cTableValueFacts::fiUnit, "T");
      tableValueFacts->setValue(cTableValueFacts::fiFactor, 1);
      tableValueFacts->setValue(cTableValueFacts::fiTitle, "Datum Uhrzeit der Heizung");
      
      tableValueFacts->store();
      added++;
   }

   tell(eloAlways, "Added %d user defined values", added);

   // ---------------------------------
   // add one wire sensor data

   if (w1.scan() == success)
   {
      W1::SensorList* list = w1.getList();

      // yes, we have one-wire sensors

      count = 0;
      added = 0;
      
      for (W1::SensorList::iterator it = list->begin(); it != list->end(); ++it)
      {
         // update table
         
         tableValueFacts->clear();
         tableValueFacts->setIntValue(cTableValueFacts::fiAddress, W1::toId(it->first.c_str()));
         tableValueFacts->setValue(cTableValueFacts::fiType, "W1");
         
         if (!tableValueFacts->find())
         {
            tableValueFacts->setValue(cTableValueFacts::fiName, it->first.c_str());
            tableValueFacts->setValue(cTableValueFacts::fiState, "D");
            tableValueFacts->setValue(cTableValueFacts::fiUnit, "°");
            tableValueFacts->setValue(cTableValueFacts::fiFactor, 1);
            tableValueFacts->setValue(cTableValueFacts::fiTitle, it->first.c_str());
            
            tableValueFacts->store();
            added++;
         }
         
         count++;
      }
      
      tell(eloAlways, "Found %d one wire sensors, added %d", count, added);
   }
      
   return success;
}

//***************************************************************************
// Update Menu Structure
//***************************************************************************

int P4d::updateMenu()
{
   int status;
   Fs::MenuItem m;
   int count = 0;

   // check serial communication

   if (request->check() != success)
   {
      serial->close();
      return fail;
   }

   tableMenu->truncate();
   tableMenu->clear();

   // ...

   for (status = request->getFirstMenuItem(&m); status != Fs::wrnLast;
        status = request->getNextMenuItem(&m))
   {
      if (status == wrnSkip)
         continue;

      if (status != success)
         break;

      tell(eloDebug, "%3d) Address: 0x%4x, parent: 0x%4x, child: 0x%4x; '%s'", 
           count++, m.parent, m.address, m.child, m.description);

      // update table    

      tableMenu->clear();

      tableMenu->setValue(cTableMenu::fiState, "D");
      tableMenu->setValue(cTableMenu::fiUnit, m.type == mstAnlOut && isEmpty(m.unit) ? "%" : m.unit);
      
      tableMenu->setValue(cTableMenu::fiParent, m.parent);
      tableMenu->setValue(cTableMenu::fiChild, m.child);
      tableMenu->setValue(cTableMenu::fiAddress, m.address);
      tableMenu->setValue(cTableMenu::fiTitle, m.description);
      
      tableMenu->setValue(cTableMenu::fiType, m.type);
      tableMenu->setValue(cTableMenu::fiUnknown1, m.unknown1);
      tableMenu->setValue(cTableMenu::fiUnknown2, m.unknown2);

      tableMenu->insert();

      count++;
   }

   tell(eloAlways, "Read %d menu items", count);
   
   return success;
}

//***************************************************************************
// Store
//***************************************************************************

int P4d::store(time_t now, const char* type, unsigned int address, double value, 
                unsigned int factor, const char* text)
{
   tableSamples->clear();

   tableSamples->setValue(cTableSamples::fiTime, now);
   tableSamples->setIntValue(cTableSamples::fiAddress, address);
   tableSamples->setValue(cTableSamples::fiType, type);
   tableSamples->setValue(cTableSamples::fiAggregate, "S");

   tableSamples->setValue(cTableSamples::fiValue, value / (double)factor);
   tableSamples->setValue(cTableSamples::fiText, text);
   tableSamples->setValue(cTableSamples::fiSamples, 1);

   tableSamples->store();
   
   return success;
}

//***************************************************************************
// standby
//***************************************************************************

int P4d::standby(int t)
{
   time_t end = time(0) + t;

   while (time(0) < end && !doShutDown())
   {
      meanwhile();
      usleep(50000);
   }

   return done;
}

int P4d::standbyUntil(time_t until)
{
   while (time(0) < until && !doShutDown())
   {
      meanwhile();
      usleep(50000);
   }

   return done;
}

//***************************************************************************
// Meanwhile
//***************************************************************************

int P4d::meanwhile()
{
   static time_t lastCleanup = time(0);

   if (!connection || !connection->isConnected())
      return fail;
   
   performWebifRequests();

   if (lastCleanup < time(0) - 6*tmeSecondsPerHour)
   {
      cleanupWebifRequests();
      lastCleanup = time(0);
   }

   return done;
}

//***************************************************************************
// Loop
//***************************************************************************

int P4d::loop()
{
   int status;
   time_t nextStateAt = 0;
   int lastState = na;

   // info

   if (mail && !isEmpty(stateMailTo))
      tell(eloAlways, "Mail at states '%s' to '%s'", stateMailAtStates, stateMailTo);

   if (mail && !isEmpty(errorMailTo))
      tell(eloAlways, "Mail at errors to '%s'", errorMailTo);

   // init

   scheduleAggregate();

   sem->p();
   serial->open(ttyDeviceSvc);
   sem->v();

   while (!doShutDown())
   {
      int stateChanged = no;

      // check db connection

      while (!doShutDown() && (!connection || !connection->isConnected()))
      {
         if (initDb() == success)
            break;
         else
            exitDb();

         tell(eloAlways, "Retrying in 10 seconds");
         standby(10);
      }

      meanwhile();

      standbyUntil(min(nextStateAt, nextAt));

      // aggregate

      if (aggregateHistory && nextAggregateAt <= time(0))
         aggregate();

      // update/check state
      
      status = updateState(&currentState);

      if (status != success)
      {
         sem->p();
         serial->close();
         tell(eloAlways, "Error reading serial interface, repopen now!");
         status = serial->open(ttyDeviceSvc);
         sem->v();

         if (status != success)
         {
            tell(eloAlways, "Retrying in 10 seconds");
            standby(10);
         }            
         continue;
      }

      stateChanged = lastState != currentState.state;
      
      if (stateChanged)
      {
         lastState = currentState.state;
         nextAt = time(0);              // force on state change
         
         tell(eloAlways, "State changed to '%s'", currentState.stateinfo);
      }
      
      nextStateAt = stateCheckInterval ? time(0) + stateCheckInterval : nextAt;

      // work expected?

      if (time(0) < nextAt)
         continue;

      // check serial connection

      sem->p();
      status = request->check();
      sem->v();

      if (status != success)
      {
         serial->close();
         tell(eloAlways, "Error reading serial interface");
         serial->open(ttyDeviceSvc);

         continue;
      }

      // perform update 

      nextAt = time(0) + interval;
      nextStateAt = stateCheckInterval ? time(0) + stateCheckInterval : nextAt;
      mailBody = "";
      mailBodyHtml = "";

      sem->p();
      update();

      // mail 

      if (mail && stateChanged)
         sendMail();

      sem->v();
   }
   
   serial->close();

   return success;
}

//***************************************************************************
// Update State
//***************************************************************************

int P4d::updateState(Status* state)
{
   static time_t nextSyncAt = 0;
   static time_t nextReportAt = 0;

   int status;
   struct tm tm = {0};
   time_t now;

   // get state

   sem->p();
   tell(eloDetail, "Checking state ...");
   status = request->getStatus(state);
   now = time(0);
   sem->v();

   if (status != success)
      return status;

   // ----------------------
   // check time sync

   if (!nextSyncAt)
   {
      localtime_r(&now, &tm);

      tm.tm_sec = 0;
      tm.tm_min = 0;
      tm.tm_hour = 23;

      nextSyncAt = mktime(&tm);
   }

   if (tSync && maxTimeLeak && labs(state->time - now) > maxTimeLeak)
   {
      if (now > nextReportAt)
      {
         tell(eloAlways, "Time drift is %ld seconds", state->time - now);
         nextReportAt = now + 2 * tmeSecondsPerMinute;
      }

      if (now > nextSyncAt)
      {
         localtime_r(&nextSyncAt, &tm);
         
         tm.tm_sec = 0;
         tm.tm_min = 0;
         tm.tm_hour = 23;
         
         nextSyncAt = mktime(&tm);
         nextSyncAt += tmeSecondsPerDay;
         
         tell(eloAlways, "Time drift is %ld seconds, syncing now", state->time - now);
         
         sem->p();
         
         if (request->syncTime() == success)
            tell(eloAlways, "Time sync succeeded");
         else
            tell(eloAlways, "Time sync failed");
         
         sleep(2);   // S-3200 need some seconds to store time :o

         status = request->getStatus(state);
         now = time(0);
         
         sem->v();
         
         tell(eloAlways, "Time drift now %ld seconds", state->time - now);
      }
   }

   return status;
}

//***************************************************************************
// Update
//***************************************************************************

int P4d::update()
{
   int status;
   int count = 0;
   time_t now = time(0);
   char num[100];

   w1.update();

   tell(eloDetail, "Reading values ...");

   tableValueFacts->clear();
   tableValueFacts->setValue(cTableValueFacts::fiState, "A");

   for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
   {
      unsigned int addr = tableValueFacts->getIntValue(cTableValueFacts::fiAddress);
      double factor = tableValueFacts->getIntValue(cTableValueFacts::fiFactor);
      const char* title = tableValueFacts->getStrValue(cTableValueFacts::fiTitle);
      const char* type = tableValueFacts->getStrValue(cTableValueFacts::fiType);
      const char* unit = tableValueFacts->getStrValue(cTableValueFacts::fiUnit);
      const char* name = tableValueFacts->getStrValue(cTableValueFacts::fiName);

      if (tableValueFacts->hasValue(cTableValueFacts::fiType, "VA"))
      {
         Value v(addr);

         if ((status = request->getValue(&v)) != success)
         {
            tell(eloAlways, "Getting value 0x%04x failed, error %d", addr, status);
            continue;
         }
            
         store(now, type, v.address, v.value, factor);
         sprintf(num, "%.2f", v.value / factor);

         if (strcmp(unit, "°") == 0)
            strcat(num, "°C");
         else
            strcat(num, unit);

         sensorAlertCheck(type, addr, v.value/factor, unit);
         addParameter2Mail(title, num);
      }

      else if (tableValueFacts->hasValue(cTableValueFacts::fiType, "DO"))
      {
         Fs::IoValue v(addr);

         if (request->getDigitalOut(&v) != success)
         {
            tell(eloAlways, "Getting digital out 0x%04x failed, error %d", addr, status);
            continue;
         }

         store(now, type, v.address, v.state, factor);
         sprintf(num, "%d", v.state);
         addParameter2Mail(title, num);
      }

      else if (tableValueFacts->hasValue(cTableValueFacts::fiType, "DI"))
      {
         Fs::IoValue v(addr);

         if (request->getDigitalIn(&v) != success)
         {
            tell(eloAlways, "Getting digital in 0x%04x failed, error %d", addr, status);
            continue;
         }

         store(now, type, v.address, v.state, factor);
         sprintf(num, "%d", v.state);
         addParameter2Mail(title, num);
      }

      else if (tableValueFacts->hasValue(cTableValueFacts::fiType, "AO"))
      {
         Fs::IoValue v(addr);

         if (request->getAnalogOut(&v) != success)
         {
            tell(eloAlways, "Getting analog out 0x%04x failed, error %d", addr, status);
            continue;
         }

         store(now, type, v.address, v.state, factor);
         sprintf(num, "%d", v.state);
         addParameter2Mail(title, num);
      }

      else if (tableValueFacts->hasValue(cTableValueFacts::fiType, "W1"))
      {
         double value = w1.valueOf(name);
            
         store(now, type, addr, value, factor);
         sprintf(num, "%.2f", value / factor);

         if (strcmp(unit, "°") == 0)
            strcat(num, "°C");
         else
            strcat(num, unit);
         
         addParameter2Mail(title, num);
      }

      else if (tableValueFacts->hasValue(cTableValueFacts::fiType, "UD"))
      {
         switch (tableValueFacts->getIntValue(cTableValueFacts::fiAddress))
         {
            case udState:
            {
               store(now, type, udState, currentState.state, factor, currentState.stateinfo);
               addParameter2Mail(title, currentState.stateinfo);
                  
               break;
            }
            case udMode:
            {
               store(now, type, udMode, currentState.mode, factor, currentState.modeinfo);
               addParameter2Mail(title, currentState.stateinfo);
                  
               break;
            }
            case udTime:
            {
               struct tm tim = {0};
               char date[100];
               
               localtime_r(&currentState.time, &tim);
               strftime(date, 100, "%A, %d. %b. %G %H:%M:%S", &tim);
               
               store(now, type, udTime, currentState.time, factor, date);
               addParameter2Mail(title, date);
                  
               break;
            }
         }
      }

      count++;
   }

   selectActiveValueFacts->freeResult();
   tell(eloAlways, "Processed %d samples, state is '%s'", count, currentState.stateinfo);

   updateErrors();

   return success;
}

//***************************************************************************
// Sensor Alert Check
//***************************************************************************

void P4d::sensorAlertCheck(const char* type, unsigned int addr, double value, const char* unit)
{
   tableSensorAlert->clear();
   tableSensorAlert->setValue(cTableSensorAlert::fiType, type);
   tableSensorAlert->setValue(cTableSensorAlert::fiAddress, addr);

   // iterate over all alert roules of this sensor ...

   for (int f = selectSensorAlerts->find(); f; f = selectSensorAlerts->fetch())
   {
      int minIsNull = tableSensorAlert->getValue(cTableSensorAlert::fiMin)->isNull();
      int maxIsNull = tableSensorAlert->getValue(cTableSensorAlert::fiMax)->isNull();
      int min = tableSensorAlert->getIntValue(cTableSensorAlert::fiMin);
      int max = tableSensorAlert->getIntValue(cTableSensorAlert::fiMax);

      int rangeIsNull = tableSensorAlert->getValue(cTableSensorAlert::fiRangeM)->isNull();
      int deltaIsNull = tableSensorAlert->getValue(cTableSensorAlert::fiDelta)->isNull();
      int range = tableSensorAlert->getIntValue(cTableSensorAlert::fiRangeM);
      int delta = tableSensorAlert->getIntValue(cTableSensorAlert::fiDelta);

      int id = tableSensorAlert->getIntValue(cTableSensorAlert::fiId);
      const char* subject = tableSensorAlert->getStrValue(cTableSensorAlert::fiMSubject);
      const char* to = tableSensorAlert->getStrValue(cTableSensorAlert::fiMAddress);

      if (!minIsNull || !maxIsNull)
      {
         if ((!minIsNull && value < min) || (!maxIsNull && value > max))
         {
            tell(eloAlways, "%d) Alert for sensor %s/0x%x, value %.2f not in range (%d - %d)",
                 id, type, addr, value, min, max);
            
            tell(eloAlways, "Sending mail '%s' to '%s'", subject, to);
            // #TODO - create and send the mail
         }
      }

      if (!rangeIsNull && !deltaIsNull)
      {
         // select value of this sensor around 'time = (now - range)'
         
         tableSamples->clear();
         
         tableSamples->setValue(cTableSamples::fiTime, time(0) - range*tmeSecondsPerMinute);
         tableSamples->setIntValue(cTableSamples::fiAddress, addr);
         tableSamples->setValue(cTableSamples::fiType, type);
         tableSamples->setValue(cTableSamples::fiAggregate, "S");
         
         // #TODO ... select nearest sample at given time!
         
         if (tableSamples->find())
         {
            double oldValue = tableSamples->getDoubleValue(cTableSamples::fiValue);

            if (labs(value - oldValue) > delta)
               tell(eloAlways, "%d) Alert for sensor %s/0x%x , value %.2f changed more than %d in %d minutes", 
                    id, type, addr, value, delta, range);

            tell(eloAlways, "Sending mail '%s' to '%s'", subject, to);
            // #TODO - create and send the mail
         }

         tableSamples->reset();
      }
   }

   selectSensorAlerts->freeResult();
}

//***************************************************************************
// Add Parameter To Mail
//***************************************************************************

void P4d::addParameter2Mail(const char* name, const char* value)
{
   char buf[500];

   mailBody += string(name) + " = " + string(value) + "\n";

   sprintf(buf, "      <tr><td><font face=\"Arial\">%s</font></td><td><font face=\"Arial\">%s</font></td></tr>\n", name, value);

   mailBodyHtml += buf;
}

//***************************************************************************
// Schedule Aggregate
//***************************************************************************

int P4d::scheduleAggregate()
{
   struct tm tm = { 0 };
   time_t now;

   if (!aggregateHistory)
      return done;

   // calc today at 01:00:00

   now = time(0);
   localtime_r(&now, &tm);
   
   tm.tm_sec = 0;
   tm.tm_min = 0;
   tm.tm_hour = 1;
   
   nextAggregateAt = mktime(&tm);
   
   // if in the past ... skip to next day ...

   if (nextAggregateAt <= time(0))
      nextAggregateAt += tmeSecondsPerDay;

   tell(eloAlways, "Scheduled aggregation for '%s' with interval of %d minutes", 
        l2pTime(nextAggregateAt).c_str(), aggregateInterval);

   return success;
}

//***************************************************************************
// Aggregate
//***************************************************************************

int P4d::aggregate()
{
   char* stmt = 0;
   time_t history = time(0) - (aggregateHistory * tmeSecondsPerDay);
   int aggCount = 0;

   asprintf(&stmt, 
            "replace into samples "
            "  select address, type, 'A' as aggregate, "
            "    CONCAT(DATE(time), ' ', SEC_TO_TIME((TIME_TO_SEC(time) DIV %d) * %d)) + INTERVAL %d MINUTE time, "
            "    unix_timestamp(sysdate()) as inssp, unix_timestamp(sysdate()) as updsp, "
            "    round(sum(value)/count(*), 2) as value, text, count(*) samples "
            "  from "
            "    samples "
            "  where "
            "    aggregate != 'A' and "
            "    time <= from_unixtime(%ld) "
            "  group by "
            "    CONCAT(DATE(time), ' ', SEC_TO_TIME((TIME_TO_SEC(time) DIV %d) * %d)) + INTERVAL %d MINUTE, address, type;",
            aggregateInterval * tmeSecondsPerMinute, aggregateInterval * tmeSecondsPerMinute, aggregateInterval,
            history,
            aggregateInterval * tmeSecondsPerMinute, aggregateInterval * tmeSecondsPerMinute, aggregateInterval);
  
   tell(eloAlways, "Starting aggregation ...");

   if (connection->query(aggCount, stmt) == success)
   {
      int delCount = 0;

      tell(eloDebug, "Aggregation: [%s]", stmt);
      free(stmt);

      // Einzelmesspunkte löschen ...

      asprintf(&stmt, "aggregate != 'A' and time <= from_unixtime(%ld)", history);
      
      if (tableSamples->deleteWhere(stmt, delCount) == success)
      {
         tell(eloAlways, "Aggregation with interval of %d minutes done; "
              "Created %d aggregation rows, deleted %d sample rows", 
              aggregateInterval, aggCount, delCount);
      }
   }

   free(stmt);

   // schedule even in case of error!

   scheduleAggregate();

   return success;
}

//***************************************************************************
// Update Errors
//***************************************************************************

int P4d::updateErrors()
{
   int status;
   Fs::ErrorInfo e;

   tableErrors->truncate();

   for (status = request->getFirstError(&e); status == success; status = request->getNextError(&e))
   {
      tableErrors->clear();

      tableErrors->setValue(cTableErrors::fiTime, e.time);
      tableErrors->setValue(cTableErrors::fiNumber, e.number);
      tableErrors->setValue(cTableErrors::fiInfo, e.info);
      tableErrors->setValue(cTableErrors::fiState, Fs::errState2Text(e.state));
      tableErrors->setValue(cTableErrors::fiText, e.text);

      tableErrors->insert();
   }
   
   return success;
}

//***************************************************************************
// Send Mail
//***************************************************************************

int P4d::sendMail()
{
   const char* receiver = 0;
   string subject = "";
   string errorBody = "";

   // check

   if (isEmpty(mailScript) || !mailBody.length())
      return fail;

   if (currentState.state == wsError)
   {
      Fs::ErrorInfo e;

      subject = "Heizung: STÖRUNG";
      receiver = errorMailTo;

      for (int status = request->getFirstError(&e); status == success; status = request->getNextError(&e))
      {
         // nur Fehler der letzten 2 Tage 

         if (e.time > time(0) - 2*tmeSecondsPerDay)
         {  
            char* ct = strdup(ctime(&e.time));
            char* line;
            
            ct[strlen(ct)-1] = 0;   // remove linefeed of ctime()

            asprintf(&line, "%s:  %03d/%03d  %s (%s)\n", ct, e.number, e.info, e.text, Fs::errState2Text(e.state));
            errorBody += line;

            free(line);
            free(ct);
         }
      }

      mailBody = errorBody + string("\n\n") + mailBody;
   }

   else if (isMailState())
   {
      subject = "Heizung - Status: " + string(currentState.stateinfo);
      receiver = stateMailTo;
   }
   
   if (isEmpty(receiver))
      return done;
   
   // send mail ...

   if (!htmlMail)
   {
      char* command = 0;

      asprintf(&command, "%s '%s' '%s' '%s' %s", mailScript, 
               subject.c_str(), mailBody.c_str(), "text/plain", receiver);
      
      system(command);
      free(command);
   }
   else
   {
      char* command = 0;
      char* html = 0;
      
      const char* htmlHead = 
         "<head>"
         "  <style type=\"text/css\">\n"
         "    table { font-size: 14px; border-collapse: collapse; table-layout: auto; }\n"
         "    td, th { border: 1px solid #000; }\n"
         "    th { background-color: #095BA6; font-family: Arial Narrow; color: #fff; }\n"
         "    td { font-family: Arial; }\n"
         "    caption { background: #095BA6; font-family: Arial Narrow; color: #fff; font-size: 18px; }\n"
         "  </style>\n"
         "</head>\n";
     
      asprintf(&html, 
               "<html>\n"
               " %s"
               " <body>\n"
               "%s"
               "  <br></br>\n"
               "  <table>\n"
               "  <caption>S 3200<caption>"
               "   <thead>\n"
               "    <tr>\n"
               "     <th><font>Parameter</font></th>\n"
               "     <th><font>Wert</font></th>\n"
               "    </tr>\n"
               "   </thead>\n"
               "   <tbody>\n"
               "%s"
               "   </tbody>\n"
               "  </table>\n"
               "  <br></br>\n"
               " </body>\n"
               "</html>\n",
               htmlHead, errorBody.c_str(), mailBodyHtml.c_str());

      // send HTML mail
      
      asprintf(&command, "%s '%s' '%s' '%s' %s", mailScript, 
               subject.c_str(), html, "text/html", receiver);
      
      system(command);
      
      free(command);
      free(html);
   }

   // 

   tell(eloAlways, "Send mail '%s' with [%s] to '%s'", 
        subject.c_str(), mailBody.c_str(), receiver);

   return success;
}

//***************************************************************************
// Is Mail State
//***************************************************************************

int P4d::isMailState()
{
   int result = no;
   char* mailStates = 0;

   if (isEmpty(stateMailAtStates))
      return yes;

   mailStates = strdup(stateMailAtStates);

   for (const char* p = strtok(mailStates, ","); p; p = strtok(0, ","))
   {
      if (atoi(p) == currentState.state)
      {
         result = yes;
         break;
      }
   }

   free(mailStates);

   return result;
}

//***************************************************************************
// Stored Parameters
//***************************************************************************

int P4d::getConfigItem(const char* name, char*& value, const char* def)
{
   free(value);
   value = 0;

   tableConfig->clear();
   tableConfig->setValue(cTableConfig::fiOwner, "p4d");
   tableConfig->setValue(cTableConfig::fiName, name);

   if (tableConfig->find())
      value = strdup(tableConfig->getStrValue(cTableConfig::fiValue));
   else
   {
      value = strdup(def);
      setConfigItem(name, value);  // store the default
   }
      
   tableConfig->reset();

   return success;
}

int P4d::setConfigItem(const char* name, const char* value)
{
   tell(eloAlways, "Storing '%s' with value '%s'", name, value);
   tableConfig->clear();
   tableConfig->setValue(cTableConfig::fiOwner, "p4d");
   tableConfig->setValue(cTableConfig::fiName, name);
   tableConfig->setValue(cTableConfig::fiValue, value);

   return tableConfig->store();
}

int P4d::getConfigItem(const char* name, int& value, int def)
{
   char* txt = 0;
   
   getConfigItem(name, txt);

   if (!isEmpty(txt))
      value = atoi(txt);
   else if (isEmpty(txt) && def != na)
      value = def;
   else
      value = 0;

   free(txt);

   return success;
}

int P4d::setConfigItem(const char* name, int value)
{
   char txt[16];

   snprintf(txt, sizeof(txt), "%d", value);

   return setConfigItem(name, txt);
}
