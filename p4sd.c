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
   tableValueFacts = 0;
   tableParameterFacts = 0;
   selectActiveValueFacts = 0;

   cDbConnection::init();
   cDbConnection::setEncoding("utf8");
   cDbConnection::setHost(dbHost);
   cDbConnection::setPort(dbPort);
   cDbConnection::setName(dbName);
   cDbConnection::setUser(dbUser);
   cDbConnection::setPass(dbPass);

   serial = new Serial;
   request = new P4Request(serial);
   request->setFixFwDateBug(fixFwDateBug);

   init();
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

   initDb();

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

   tell(eloAlways, "Connection to database established");  

   return status;
}

int P4sd::exitDb()
{
   delete tableSamples;        tableSamples = 0;
   delete tableValueFacts;     tableValueFacts = 0;
   delete tableParameterFacts; tableParameterFacts = 0;
   delete tableJobs;           tableJobs = 0;

   delete selectActiveValueFacts;  selectActiveValueFacts = 0;

   delete connection;      connection = 0;

   return done;
}

//***************************************************************************
// Setup
//***************************************************************************

int P4sd::setup()
{
   int status;

   if (!connection)
      return fail;

   tell(eloAlways, "Getting value facs from s 3200");
   updateValueFacts();
   tell(eloAlways, "Getting parameter facs from s 3200");
   updateParameterFacts();

   cDbStatement* selectAll = new cDbStatement(tableValueFacts);

   selectAll->build("select ");
   selectAll->bind(cTableValueFacts::fiAddress, cDBS::bndOut);
   selectAll->bind(cTableValueFacts::fiType, cDBS::bndOut, ", ");
   selectAll->bind(cTableValueFacts::fiState, cDBS::bndOut, ", ");
   selectAll->bind(cTableValueFacts::fiUnit, cDBS::bndOut, ", ");
   selectAll->bind(cTableValueFacts::fiFactor, cDBS::bndOut, ", ");
   selectAll->bind(cTableValueFacts::fiTitle, cDBS::bndOut, ", ");
   selectAll->build(" from %s", tableValueFacts->TableName());

   status = selectAll->prepare();

   if (status != success)
   {
      tell(eloAlways, "Error ....");
      delete selectAll;
   }

   for (int f = selectAll->find(); f; f = selectAll->fetch())
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

   selectAll->freeResult();
   delete selectAll;
   
   return done;
}

//***************************************************************************
// Update Value Facts
//***************************************************************************

int P4sd::updateValueFacts()
{
   int status;
   Fs::ValueSpec v;
   int count = 0;

   // check serial communication

   if (request->check() != success)
   {
      serial->close();
      return fail;
   }

   // ...

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
      }

      count++;
   }

   // at least add value definitions for special data

   tableValueFacts->clear();
   tableValueFacts->setValue(cTableValueFacts::fiAddress, udState);   // 1  -> Kessel Status
   tableValueFacts->setValue(cTableValueFacts::fiType, "UD");         // UD -> User Defined
   
   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue(cTableValueFacts::fiName, "Status");
      tableValueFacts->setValue(cTableValueFacts::fiState, "A");
      tableValueFacts->setValue(cTableValueFacts::fiUnit, "zst");
      tableValueFacts->setValue(cTableValueFacts::fiFactor, 1);
      tableValueFacts->setValue(cTableValueFacts::fiTitle, "Heizungsstatus");
      tableValueFacts->setValue(cTableValueFacts::fiRes1, 0.0);
      
      tableValueFacts->store();
   }
   
   count++;

   tell(eloAlways, "Read %d value facts", count);
   
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
      if (status != success)
         continue;

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
// Refresh
//***************************************************************************

int P4sd::loop()
{
   time_t lastAt = 0;
   int lastState = na;
   int stateChanged = no;
   Fs::Status state;
   int status;

   while (!doShutDown())
   {
      // check db connection

      while (!doShutDown() && (!tableSamples || !tableSamples->isConnected()))
      {
         if (initDb() == success)
            break;
         else
            exitDb();

         tell(eloAlways, "Retrying in 10 seconds");
         sleep(10);
      }

      if (time(0) < lastAt + interval)
      {
         sleep(1);
         continue;
      }

      // check serial connection

      if (request->check() != success)
      {
         serial->close();
         tell(eloAlways, "Error reading serial interface");
         serial->open(ttyDeviceSvc);

         continue;
      }

      lastAt = time(0);

      if (request->getStatus(&state) == success)
      {
         stateChanged = lastState != state.state;
         lastState = state.state;
      }

      int count = 0;
      string mailBody = "";
      tableValueFacts->setValue(cTableValueFacts::fiState, "A");

      for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
      {
         double factor = tableValueFacts->getIntValue(cTableValueFacts::fiFactor);
         const char* title = tableValueFacts->getStrValue(cTableValueFacts::fiTitle);

         if (tableValueFacts->hasValue(cTableValueFacts::fiType, "VA"))
         {
            unsigned int addr = tableValueFacts->getIntValue(cTableValueFacts::fiAddress);
            Value v(addr);

            if ((status = request->getValue(&v)) != success)
            {
               tell(eloAlways, "Getting value 0x%04x failed, error %d", addr, status);
               continue;
            }
            
            store(lastAt, "VA", v.address, v.value, factor);

            if (stateChanged)
            {
               char num[100];
               sprintf(num, "%.2f", v.value / factor);
               mailBody += string(title) + " = " + string(num) + "\n";
            }
         }

         else if (tableValueFacts->hasValue(cTableValueFacts::fiType, "UD"))
         {
            switch (tableValueFacts->getIntValue(cTableValueFacts::fiAddress))
            {
               case udState:
               {
                  store(lastAt, "UD", udState, state.state, factor, state.stateinfo);

                  if (stateChanged)
                     mailBody += string(title) 
                        + " = " + string(state.stateinfo) + "\n";
                  
                  break;
               }
            }
         }

         count++;
      }

      selectActiveValueFacts->freeResult();
      tell(eloAlways, "Processed %d samples, state is '%s'", 
           count, state.stateinfo);

      if (mail && stateChanged)
         sendMail(mailBody.c_str(), &state);
   }
   
   return success;
}

//***************************************************************************
// Send Mail
//***************************************************************************

int P4sd::sendMail(const char* body, Status* state)
{
   char* command = 0;
   const char* receiver = 0;
   string subject = "";
   string errorBody = "";

   // check

   if (isEmpty(mailScript) || isEmpty(body))
      return fail;

   if (state->state == wsError)
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

      errorBody += string("\n") + body;
      body = errorBody.c_str();
   }

   else if (isMailState(state->state))
   {
      subject = "Heizung - Status: " + string(state->stateinfo);
      receiver = stateMailTo;
   }
   
   if (isEmpty(receiver))
      return done;
   
   // send mail
   
   asprintf(&command, "%s '%s' '%s' %s", mailScript, 
            subject.c_str(), body,
            receiver);
   
   system(command);
   
   free(command);
   
   tell(eloAlways, "Send mail '%s' with [%s] to '%s'", 
        subject.c_str(), body, receiver);

   return success;
}

//***************************************************************************
// Is Mail State
//***************************************************************************

int P4sd::isMailState(int state)
{
   if (isEmpty(stateMailAtStates))
      return yes;

   for (const char* p = strtok(stateMailAtStates, ":,"); p; p = strtok(0, ":,"))
   {
      if (atoi(p) == state)
         return yes;
   }

   return no;
}
