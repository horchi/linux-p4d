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
   tableValueFacts = 0;
   tableSensors = 0;

   cDbConnection::init();
   cDbConnection::setEncoding("utf8");
   cDbConnection::setHost(dbHost);
   cDbConnection::setPort(dbPort);
   cDbConnection::setName(dbName);
   cDbConnection::setUser(dbUser);
   cDbConnection::setPass(dbPass);

   serial = new Serial;
   request = new P4Request(serial);

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
   initDb();

   if (serial->open(ttyDeviceSvc) != success)
      return fail;

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
   static int initial = yes;

   if (connection || tableSamples)
      exitDb();

   tell(eloAlways, "Try conneting to database");

   connection = new cDbConnection();

   tableValueFacts = new cTableValueFacts(connection);
   if (tableValueFacts->open() != success) return fail;

   tableSamples = new cTableSamples(connection);
   if (tableSamples->open() != success) return fail;

   tableSensors = new cTableSensorFacts(connection);
   if (tableSensors->open() != success) return fail;
   
   tell(eloAlways, "Connection to database established");  
   
   if (initial)
   {
      updateValueFacts();
      initial = no;
   }

   return success;
}

int P4sd::exitDb()
{
   delete tableSamples;    tableSamples = 0;
   delete tableValueFacts; tableValueFacts = 0;
   delete tableSensors;    tableSensors = 0;

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

   cDbStatement* selectAll = new cDbStatement(tableValueFacts);

   selectAll->build("select ");
   selectAll->bind(cTableValueFacts::fiAddress, cDBS::bndOut);
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
      char buf[1024+TB]; *buf = 0;
      char state;

      printf("0x%04x '%s' ", 
             (unsigned int)tableValueFacts->getIntValue(cTableValueFacts::fiAddress),
             tableValueFacts->getStrValue(cTableValueFacts::fiTitle));

      if (tableValueFacts->getStrValue(cTableValueFacts::fiState)[0] == 'A')
         printf(" - aufzeichnen (Y/n): ");
      else
         printf(" - aufzeichnen (y/N): ");
            
      fgets(buf, 1000, stdin);
      buf[strlen(buf)-1] = 0;

      state = toupper(buf[0]) == 'Y' ? 'A' : 'D';

      if (state != tableValueFacts->getStrValue(cTableValueFacts::fiState)[0] &&
          tableValueFacts->find())
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

   for (status = request->getFirstValueSpec(&v); status != Fs::wrnLast; status = request->getNextValueSpec(&v))
   {
      if (status == success)
      {
         tell(eloDebug, "%3d) 0x%04x %4d '%s' (%04d) '%s'",
              count, v.address, v.factor, v.unit, v.unknown, v.description);

         // update table

         tableValueFacts->clear();
         tableValueFacts->setValue(cTableValueFacts::fiAddress, v.address);
         
         if (!tableValueFacts->find())
         {
//          string name = p->name;
//          removeChars(name, ".:-,;_?=!§$%&/()\"'+* ");
//          tableValueFacts->setValue(cTableValueFacts::fiName, name.c_str());

            tableValueFacts->setValue(cTableValueFacts::fiState, "D");
            tableValueFacts->setValue(cTableValueFacts::fiUnit, v.unit);
            tableValueFacts->setValue(cTableValueFacts::fiFactor, v.factor);
            tableValueFacts->setValue(cTableValueFacts::fiTitle, v.description);
            tableValueFacts->setValue(cTableValueFacts::fiRes1, v.unknown);
            
            tableValueFacts->store();
         }
      }      

      count++;
   }

   tell(eloAlways, "Read %d value facts", count);
   
   return success;
}

//***************************************************************************
// Store
//***************************************************************************

int P4sd::store(time_t now, Parameter* p)
{
   tableSamples->clear();
   tableSamples->setValue(cTableSamples::fiTime, now);
   tableSamples->setValue(cTableSamples::fiSensorId, p->index);

   if (p->index != parError)
      tableSamples->setValue(cTableSamples::fiValue, p->value);

   if (!isEmpty(allTrim(p->text)))
      tableSamples->setValue(cTableSamples::fiText, allTrim(p->text));

   tableSamples->store();
   
   return success;
}

int P4sd::storeSensor(Parameter* p)
{
   tableSensors->clear();
   tableSensors->setValue(cTableSensorFacts::fiSensorId, p->index);

   if (!tableSensors->find())
   {
      string name = p->name;

      removeChars(name, ".:-,;_?=!§$%&/()\"'+* ");

      tableSensors->setValue(cTableSensorFacts::fiName, name.c_str());
      tableSensors->setValue(cTableSensorFacts::fiUnit, p->unit);
      tableSensors->setValue(cTableSensorFacts::fiTitle, 
                             p->index == parState ? "Heizungsstatus" : p->name);

      tableSensors->store();
   }

   return success;
}

//***************************************************************************
// Refresh
//***************************************************************************

int P4sd::loop()
{
//   static int lastState = na;

//   std::vector<Parameter>::iterator it;
//    int lastAt = 0;
//    time_t startAt = 0;
//    int doStore;
//    int count;
//    Parameter* pState;

   // tlg->open(ttyDeviceSvc);

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

      sleep(10);
      tell(eloAlways, "loop");

//       // try serial read

//       if (tlg->read() != success)
//       {   
//          sleep(5);
//          tlg->reopen(ttyDeviceSvc);

//          continue;
//       }

//       pState = tlg->getParameter(parState);
//       startAt = time(0);
//       count = 0;
//       doStore = (startAt >= lastAt + interval) || pState->value != lastState;
//       lastState = pState->value;

//       if (doStore)
//       {
//          lastAt = startAt;
//          connection->startTransaction();
//       }

//       // process parameters ..

//       for (it = tlg->getParameters()->begin(); it != tlg->getParameters()->end(); it++)
//       {
//          Parameter p = *it;
         
//          if (*p.name) 
//             tell(eloDetail, "(%2d) %-17s: %.2f %s", 
//                  p.index, p.name, p.value, 
//                  p.index == parError ? p.text : p.unit);

//          if (doStore)
//          {
//             storeSensor(&p);
//             store(startAt, &p);
//          }

//          count++;
//       }

//       if (mail)
//          sendMail();

//       // commit

//       if (doStore)
//       {
//          connection->commit();
         
//          tell(eloAlways, "Got %d parameters (%sstored), status is '%s' [%s]", 
//               count, !doStore ? "not " : "", pState->text, tlg->all());
//       }
   }
   
   // tlg->close();
   
   return success;
}

//***************************************************************************
// Send Mail
//***************************************************************************

int P4sd::sendMail()
{
//    static int lastState = na;
//    static int lastError = na;
//    static int firstCall = yes;

//    std::vector<Parameter>::iterator it;
//    char* command = 0;
//    const char* receiver = 0;

//    string subject = "";
//    string body = "";

//    Parameter* pState = tlg->getParameter(parState);
//    Parameter* pError = tlg->getParameter(parError);

//    if (!pState || !pError)
//    {
//       tell(eloAlways, "Missing at least one parameter %d/%d, got %d parameters - aborting mail check [%s]", 
//            pState, pError, tlg->getParameters()->size(), tlg->all());

//       return fail;
//    }

//    // init

//    if (lastState == na)
//       lastState = pState->value;

//    if (lastError == na)
//       lastError = pError->value;
      
//    // check

//    if (isEmpty(mailScript))
//       return fail;

//    int isStateChanged = pState->value != lastState && (pState->value == 0 || pState->value == 1 || pState->value == 3 || pState->value == 19);
//    int isErrorChanged = pError->value != lastError;

//    if (isStateChanged || isErrorChanged || firstCall)
//    {   
//       if (isErrorChanged)
//       {
//          subject = "Heizung: STÖRUNG: " + string(pError->text);
//          receiver = errorMailTo;
//       }
//       else
//       {
//          subject = "Heizung - Status: " + string(pState->text);
//          receiver = stateMailTo;
//       }

//       if (isEmpty(receiver))
//          return done;

//       // build mail body

//       for (it = tlg->getParameters()->begin(); it != tlg->getParameters()->end(); it++)
//       {
//          Parameter* p = &(*it);

//          body += p->name + string(" = ") + num2Str((double)p->value);
//          body += p->unit;

//          if (!isEmpty(p->text))
//             body += string("  \"") + p->text + string("\"");
         
//          body += "\n";
//       }

//       // send mail

//       asprintf(&command, "%s '%s' '%s' %s", mailScript, 
//                subject.c_str(), body.c_str(),
//                receiver);
      
//       system(command);

//       free(command);
//       firstCall = no;

//       tell(eloAlways, "Send mail '%s' with [%s] to '%s'", 
//            subject.c_str(), body.c_str(), receiver);
//    }

//    lastState = pState->value;

   return success;
}
