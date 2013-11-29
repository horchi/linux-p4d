//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File p4sd.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
// Date 04.11.2010 - 21.11.2013  Jörg Wendel
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <stdio.h>
#include <unistd.h>

#include <vector>

#include "p4sd.h"

int P4sd::shutdown = no;

//***************************************************************************
// Object
//***************************************************************************

P4sd::P4sd()
{
   connection = 0;
   tableSamples = 0;
   tableJobs = 0;
   tableSchemaConf = 0;
   tableValueFacts = 0;
   tableParameterFacts = 0;
   selectActiveValueFacts = 0;
   selectAllValueFacts = 0;

   mailBody = "";

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

P4sd::~P4sd()
{
   exit();

   delete serial;
   delete request;

   cDbConnection::exit();
}

//***************************************************************************
// Init / Exit
//***************************************************************************

int P4sd::init()
{
   if (serial->open(ttyDeviceSvc) != success)
      return fail;

   if (initDb() != success)
      return fail;

   if (mail && !isEmpty(stateMailTo))
      tell(eloAlways, "Mail at states '%s' to '%s'", stateMailAtStates, stateMailTo);

   return success;
}

int P4sd::exit()
{
   exitDb();
   serial->close();

   return success;
}

//***************************************************************************
// Init/Exit Database
//***************************************************************************

int P4sd::initDb()
{
   int status = success;

   if (connection)
      exitDb();

   tell(eloAlways, "Try conneting to database");

   connection = new cDbConnection();

   tableValueFacts = new cTableValueFacts(connection);
   if (tableValueFacts->open() != success) return fail;

   tableParameterFacts = new cTableParameterFacts(connection);
   if (tableParameterFacts->open() != success) return fail;

   tableSamples = new cTableSamples(connection);
   if (tableSamples->open() != success) return fail;

   tableJobs = new cTableJobs(connection);
   if (tableJobs->open() != success) return fail;

   tableSchemaConf = new cTableSchemaConf(connection);
   if (tableSchemaConf->open() != success) return fail;

   // prepare statements

   selectActiveValueFacts = new cDbStatement(tableValueFacts);

   selectActiveValueFacts->build("select ");
   selectActiveValueFacts->bind(cTableValueFacts::fiAddress, cDBS::bndOut);
   selectActiveValueFacts->bind(cTableValueFacts::fiType, cDBS::bndOut, ", ");
   selectActiveValueFacts->bind(cTableValueFacts::fiUnit, cDBS::bndOut, ", ");
   selectActiveValueFacts->bind(cTableValueFacts::fiFactor, cDBS::bndOut, ", ");
   selectActiveValueFacts->bind(cTableValueFacts::fiTitle, cDBS::bndOut, ", ");
   selectActiveValueFacts->build(" from %s where ", tableValueFacts->TableName());
   selectActiveValueFacts->bind(cTableValueFacts::fiState, cDBS::bndIn | cDBS::bndSet);

   status = selectActiveValueFacts->prepare();

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

   status = selectAllValueFacts->prepare();

   tell(eloAlways, "Connection to database established");  

   return status;
}

int P4sd::exitDb()
{
   delete tableSamples;        tableSamples = 0;
   delete tableValueFacts;     tableValueFacts = 0;
   delete tableParameterFacts; tableParameterFacts = 0;
   delete tableJobs;           tableJobs = 0;
   delete tableSchemaConf;     tableSchemaConf = 0;

   delete selectActiveValueFacts;  selectActiveValueFacts = 0;
   delete selectAllValueFacts;     selectAllValueFacts = 0;

   delete connection;      connection = 0;

   return done;
}

//***************************************************************************
// Initialize
//***************************************************************************

int P4sd::initialize(int truncate)
{
   if (!connection)
      return fail;

   // truncate config tables ?

   if (truncate)
   {
      tell(eloAlways, "Truncate configuration tables!");
      
      tableValueFacts->truncate();
      tableSchemaConf->truncate();
      tableParameterFacts->truncate();
   }

   sem->p();

   tell(eloAlways, "Getting value facts from s 3200");
   updateValueFacts();
   tell(eloAlways, "Update html schema configuration");
   updateConfTables();
   tell(eloAlways, "Getting parameter facs from s 3200");
   updateParameterFacts();

   sem->v();

   return done;
}

//***************************************************************************
// Setup
//***************************************************************************

int P4sd::setup()
{
   if (!connection)
      return fail;

   for (int f = selectAllValueFacts->find(); f; f = selectAllValueFacts->fetch())
   {
      char* res;
      char buf[100+TB]; *buf = 0;
      char oldState = tableValueFacts->getStrValue(cTableValueFacts::fiState)[0];
      char state = oldState;

      printf("%s 0x%04x '%s' (%s)", 
             tableValueFacts->getStrValue(cTableValueFacts::fiType),
             (unsigned int)tableValueFacts->getIntValue(cTableValueFacts::fiAddress),
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
   updateConfTables();

   return done;
}

//***************************************************************************
// Update Conf Tables
//***************************************************************************

int P4sd::updateConfTables()
{
   const int step = 20;
   int y = 0;
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

int P4sd::updateValueFacts()
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

   // add default fpr digital outputs

   count = 0;
   added = 0;

   for (int i = 0; Fs::diditalOutDefinitions[i].title; i++)
   {
      tableValueFacts->clear();
      tableValueFacts->setValue(cTableValueFacts::fiAddress, diditalOutDefinitions[i].address);
      tableValueFacts->setValue(cTableValueFacts::fiType, "DO");         // DO -> Digital Out
      
      if (!tableValueFacts->find())
      {
         string name = diditalOutDefinitions[i].title;
         
         removeCharsExcept(name, nameChars);

         tableValueFacts->setValue(cTableValueFacts::fiName, name.c_str());
         tableValueFacts->setValue(cTableValueFacts::fiState, "D");
         tableValueFacts->setValue(cTableValueFacts::fiUnit, "dig");
         tableValueFacts->setValue(cTableValueFacts::fiFactor, 1);
         tableValueFacts->setValue(cTableValueFacts::fiTitle, diditalOutDefinitions[i].title);
         
         tableValueFacts->store();
         added++;
      }

      count++;
   }

   tell(eloAlways, "Checked %d digital output facts, added %d", count, added);

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

   return success;
}

//***************************************************************************
// Update Parameter Facts
//***************************************************************************

int P4sd::updateParameterFacts()
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

   // ...

   for (status = request->getFirstMenuItem(&m); status != Fs::wrnLast; 
        status = request->getNextMenuItem(&m))
   {
      if (status == wrnSkip)
         continue;

      if (status != success)
         break;

      tell(eloDebug, "%3d) 0x%04x (0x%04x) '%s'", count++, m.address, m.unknown, m.description);

      // update table
      
      tableParameterFacts->clear();
      tableParameterFacts->setValue(cTableParameterFacts::fiAddress, m.address);
      
      if (!tableParameterFacts->find())
      {
         tableParameterFacts->setValue(cTableParameterFacts::fiState, "D");
         tableParameterFacts->setValue(cTableParameterFacts::fiUnit, m.unit);
         tableParameterFacts->setValue(cTableParameterFacts::fiFactor, m.factor);
         tableParameterFacts->setValue(cTableParameterFacts::fiTitle, m.description);
         tableParameterFacts->setValue(cTableParameterFacts::fiRes1, m.unknown);

         tableParameterFacts->store();
      }

      count++;
   }

   tell(eloAlways, "Read %d parameter facts", count);
   
   return success;
}

//***************************************************************************
// Store
//***************************************************************************

int P4sd::store(time_t now, const char* type, long address, double value, 
                unsigned int factor, const char* text)
{
   tableSamples->clear();

   tableSamples->setValue(cTableSamples::fiTime, now);
   tableSamples->setValue(cTableSamples::fiType, type);
   tableSamples->setValue(cTableSamples::fiAddress, address);
   tableSamples->setValue(cTableSamples::fiValue, value / (double)factor);
   tableSamples->setValue(cTableSamples::fiText, text);

   tableSamples->store();
   
   return success;
}

//***************************************************************************
// standby
//***************************************************************************

int P4sd::standby(int t)
{
   time_t end = time(0) + t;

   while (time(0) < end && !doShutDown())
      sleep(1);

   return done;
}

int P4sd::standbyUntil(time_t until)
{
   while (time(0) < until && !doShutDown())
      sleep(1);

   return done;
}

//***************************************************************************
// Loop
//***************************************************************************

int P4sd::loop()
{
   int status;
   time_t nextAt = 0;
   time_t nextStateAt = 0;
   int lastState = na;

   while (!doShutDown())
   {
      int stateChanged = no;

      // check db connection

      while (!doShutDown() && (!tableSamples || !tableSamples->isConnected()))
      {
         if (initDb() == success)
            break;
         else
            exitDb();

         tell(eloAlways, "Retrying in 10 seconds");
         standby(10);
      }

      standbyUntil(min(nextStateAt, nextAt));

      // check state

      sem->p();
      tell(eloDetail, "Checking state ...");
      status = request->getStatus(&currentState);
      sem->v();

      if (status != success)
         continue;

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

      sem->p();
      update();

      // mail 

      if (mail && stateChanged)
         sendMail();

      sem->v();
   }
   
   return success;
}

//***************************************************************************
// Update
//***************************************************************************

int P4sd::update()
{
   int status;
   int count = 0;
   time_t now = time(0);
   char num[100];

   tell(eloDetail, "Reading values ...");

   tableValueFacts->clear();
   tableValueFacts->setValue(cTableValueFacts::fiState, "A");

   for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
   {
      unsigned int addr = tableValueFacts->getIntValue(cTableValueFacts::fiAddress);
      double factor = tableValueFacts->getIntValue(cTableValueFacts::fiFactor);
      const char* title = tableValueFacts->getStrValue(cTableValueFacts::fiTitle);
      const char* type = tableValueFacts->getStrValue(cTableValueFacts::fiType);

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
         mailBody += string(title) + " = " + string(num) + "\n";
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
         mailBody += string(title) + " = " + string(num) + "\n";
      }

      else if (tableValueFacts->hasValue(cTableValueFacts::fiType, "UD"))
      {
         switch (tableValueFacts->getIntValue(cTableValueFacts::fiAddress))
         {
            case udState:
            {
               store(now, type, udState, currentState.state, factor, currentState.stateinfo);
               mailBody += string(title) + " = " + string(currentState.stateinfo) + "\n";
                  
               break;
            }
            case udMode:
            {
               store(now, type, udMode, currentState.mode, factor, currentState.modeinfo);
               mailBody += string(title) + " = " + string(currentState.modeinfo) + "\n";
                  
               break;
            }
            case udTime:
            {
               struct tm tim = {0};
               char date[100];
               
               localtime_r(&currentState.time, &tim);
               strftime(date, 100, "%A, %d. %b. %G %H:%M:%S", &tim);
               
               store(now, type, udTime, currentState.time, factor, date);
               mailBody += string(title) + " = " + string(date) + "\n";
                  
               break;
            }
         }
      }

      count++;
   }

   selectActiveValueFacts->freeResult();
   tell(eloAlways, "Processed %d samples, state is '%s'", count, currentState.stateinfo);

   return success;
}

//***************************************************************************
// Send Mail
//***************************************************************************

int P4sd::sendMail()
{
   char* command = 0;
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
         // nur Fehler der letzten 24 Stunden 

         if (e.time > time(0) - 24 * tmeSecondsPerDay)
         {  
            char* ct = strdup(ctime(&e.time));
            char* line;
            
            ct[strlen(ct)-1] = 0;   // remove linefeed of ctime()

            asprintf(&line, "%s:  %03d/%03d  '%s' (%s)\n", ct, e.number, e.info, e.text, Fs::errState2Text(e.state));
            errorBody += line;

            free(line);
            free(ct);
         }
      }

      mailBody = errorBody + string("\n\n") + mailBody;
   }

   else if (haveMailState())
   {
      subject = "Heizung - Status: " + string(currentState.stateinfo);
      receiver = stateMailTo;
   }
   
   if (isEmpty(receiver))
      return done;
   
   // send mail
   
   asprintf(&command, "%s '%s' '%s' %s", mailScript, 
            subject.c_str(), mailBody.c_str(),
            receiver);
   
   system(command);
   
   free(command);
   
   tell(eloAlways, "Send mail '%s' with [%s] to '%s'", 
        subject.c_str(), mailBody.c_str(), receiver);

   return success;
}

//***************************************************************************
// Have Mail State
//***************************************************************************

int P4sd::haveMailState()
{
   int result = no;
   char* mailStates = 0;

   if (isEmpty(stateMailAtStates))
      return yes;

   mailStates = strdup(stateMailAtStates);

   for (const char* p = strtok(mailStates, ":,"); p; p = strtok(0, ":,"))
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
