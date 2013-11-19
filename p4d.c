//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File p4d.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
// Date 04.11.2010 - 19.11.2013  Jörg Wendel
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <stdio.h>
#include <unistd.h>

#include <vector>

#include "p4d.h"

int Linpellet::shutdown = no;

//***************************************************************************
// Object
//***************************************************************************

Linpellet::Linpellet()
{
   connection = 0;
   table = 0;
   tableSensors = 0;

   cDbConnection::init();
   cDbConnection::setEncoding("utf8");
   cDbConnection::setHost(dbHost);
   cDbConnection::setPort(dbPort);
   cDbConnection::setName(dbName);
   cDbConnection::setUser(dbUser);
   cDbConnection::setPass(dbPass);

   tlg = new P4Packet();
}

Linpellet::~Linpellet()
{
   exitDb();

   cDbConnection::exit();

   delete tlg;
}

//***************************************************************************
// 
//***************************************************************************

int Linpellet::initDb()
{
   if (connection || table)
      exitDb();

   tell(eloAlways, "Try conneting to database");

   connection = new cDbConnection();

   table = new cTableSamples(connection);
   
   if (table->open() != success)
   {
      tell(eloAlways, "Could not access database '%s:%d' (%s)", 
           cDbConnection::getHost(), cDbConnection::getPort(), table->TableName());
      
      exitDb();
      
      return fail;
   }

   tableSensors = new cTableSensorFacts(connection);
   
   if (tableSensors->open() != success)
   {
      tell(eloAlways, "Could not access database '%s:%d' (%s)", 
           cDbConnection::getHost(), cDbConnection::getPort(), tableSensors->TableName());
      
      exitDb();
      
      return fail;
   }

   tell(eloAlways, "Connection to database established");  

   return success;
}

int Linpellet::exitDb()
{
   if (table)        table->close();
   if (tableSensors) tableSensors->close();
   
   delete table;      table = 0;
   delete connection; connection = 0;

   return done;
}

//***************************************************************************
// Store
//***************************************************************************

int Linpellet::store(time_t now, Parameter* p)
{
   table->clear();
   table->setValue(cTableSamples::fiTime, now);
   table->setValue(cTableSamples::fiSensorId, p->index);

   if (p->index != parError)
      table->setValue(cTableSamples::fiValue, p->value);

   if (!isEmpty(allTrim(p->text)))
      table->setValue(cTableSamples::fiText, allTrim(p->text));

   table->store();
   
   return success;
}

int Linpellet::storeSensor(Parameter* p)
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
// Send Mail
//***************************************************************************

int Linpellet::sendMail()
{
   static int lastState = na;
   static int lastError = na;
   static int firstCall = yes;

   std::vector<Parameter>::iterator it;
   char* command = 0;
   const char* receiver = 0;

   string subject = "";
   string body = "";

   Parameter* pState = tlg->getParameter(parState);
   Parameter* pError = tlg->getParameter(parError);

   if (!pState || !pError)
   {
      tell(eloAlways, "Missing at least one parameter %d/%d, got %d parameters - aborting mail check [%s]", 
           pState, pError, tlg->getParameters()->size(), tlg->all());

      return fail;
   }

   // init

   if (lastState == na)
      lastState = pState->value;

   if (lastError == na)
      lastError = pError->value;
      
   // check

   if (isEmpty(mailScript))
      return fail;

   int isStateChanged = pState->value != lastState && (pState->value == 0 || pState->value == 1 || pState->value == 3 || pState->value == 19);
   int isErrorChanged = pError->value != lastError;

   if (isStateChanged || isErrorChanged || firstCall)
   {   
      if (isErrorChanged)
      {
         subject = "Heizung: STÖRUNG: " + string(pError->text);
         receiver = errorMailTo;
      }
      else
      {
         subject = "Heizung - Status: " + string(pState->text);
         receiver = stateMailTo;
      }

      if (isEmpty(receiver))
         return done;

      // build mail body

      for (it = tlg->getParameters()->begin(); it != tlg->getParameters()->end(); it++)
      {
         Parameter* p = &(*it);

         body += p->name + string(" = ") + num2Str((double)p->value);
         body += p->unit;

         if (!isEmpty(p->text))
            body += string("  \"") + p->text + string("\"");
         
         body += "\n";
      }

      // send mail

      asprintf(&command, "%s '%s' '%s' %s", mailScript, 
               subject.c_str(), body.c_str(),
               receiver);
      
      system(command);

      free(command);
      firstCall = no;

      tell(eloAlways, "Send mail '%s' with [%s] to '%s'", 
           subject.c_str(), body.c_str(), receiver);
   }

   lastState = pState->value;

   return success;
}

//***************************************************************************
// Refresh
//***************************************************************************

int Linpellet::loop()
{
   static int lastState = na;

   std::vector<Parameter>::iterator it;
   int lastAt = 0;
   time_t startAt = 0;
   int doStore;
   int count;
   Parameter* pState;

   tlg->open(ttyDevice);

   while (!doShutDown())
   {
      // check db connection

      while (!doShutDown() && (!table || !table->isConnected()))
      {
         if (initDb() == success)
            break;

         tell(eloAlways, "Retrying in 10 seconds");
         sleep(10);
      }

      // try serial read

      if (tlg->read() != success)
      {   
         sleep(5);
         tlg->reopen(ttyDevice);

         continue;
      }

      pState = tlg->getParameter(parState);
      startAt = time(0);
      count = 0;
      doStore = (startAt >= lastAt + interval) || pState->value != lastState;
      lastState = pState->value;

      if (doStore)
      {
         lastAt = startAt;
         connection->startTransaction();
      }

      // process parameters ..

      for (it = tlg->getParameters()->begin(); it != tlg->getParameters()->end(); it++)
      {
         Parameter p = *it;
         
         if (*p.name) 
            tell(eloDetail, "(%2d) %-17s: %.2f %s", 
                 p.index, p.name, p.value, 
                 p.index == parError ? p.text : p.unit);

         if (doStore)
         {
            storeSensor(&p);
            store(startAt, &p);
         }

         count++;
      }

      if (mail)
         sendMail();

      // commit

      if (doStore)
      {
         connection->commit();

         
         tell(eloAlways, "Got %d parameters (%sstored), status is '%s' [%s]", 
              count, !doStore ? "not " : "", pState->text, tlg->all());
      }
   }
   
   tlg->close();
   
   return success;
}
