//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File p4d.cc
// Date 04.11.10 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <stdio.h>
#include <unistd.h>

#include <vector>

#include <p4d.h>
#include <lib/tabledef.h>

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

   tell(0, "Try conneting to database");

   connection = new cDbConnection();

   table = new Table(connection, "samples", cSampleFields::fields);
   
   if (table->open() != success)
   {
      tell(0, "Could not access database '%s:%d' (%s)", 
           cDbConnection::getHost(), cDbConnection::getPort(), table->TableName());
      
      exitDb();
      
      return fail;
   }

   tableSensors = new Table(connection, "sensorfacts", cSensorFactFields::fields);
   
   if (tableSensors->open() != success)
   {
      tell(0, "Could not access database '%s:%d' (%s)", 
           cDbConnection::getHost(), cDbConnection::getPort(), tableSensors->TableName());
      
      exitDb();
      
      return fail;
   }

   tell(0, "Connection to database established");  

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
   table->setValue(cSampleFields::fiTime, now);
   table->setValue(cSampleFields::fiSensorId, p->index);

   if (p->index != parError)
      table->setValue(cSampleFields::fiValue, p->value);

   if (!isEmpty(allTrim(p->text)))
      table->setValue(cSampleFields::fiText, allTrim(p->text));

   table->store();
   
   return success;
}

int Linpellet::storeSensor(Parameter* p)
{
   tableSensors->clear();
   tableSensors->setValue(cSensorFactFields::fiSensorId, p->index);

   if (!tableSensors->find())
   {
      string name = p->name;

      removeChars(name, ".:-,;_?=!§$%&/()\"'+* ");

      tableSensors->setValue(cSensorFactFields::fiName, name.c_str());
      tableSensors->setValue(cSensorFactFields::fiUnit, p->unit);
      tableSensors->setValue(cSensorFactFields::fiTitle, 
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
      tell(0, "Missing at least one parameter %d/%d, got %d parameters - aborting mail check [%s]", 
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

      tell(0, "Send mail '%s' with [%s] to '%s'", 
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

         tell(0, "Retrying in 10 seconds");
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
