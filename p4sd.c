//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File p4sd.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 08.12.2013  Jörg Wendel
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
   tableMenu = 0;
   selectActiveValueFacts = 0;
   selectAllValueFacts = 0;
   selectPendingJobs = 0;
   selectAllParameters = 0;

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

   tableMenu = new cTableMenu(connection);
   if (tableMenu->open() != success) return fail;

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

   // ----------------

   selectAllParameters = new cDbStatement(tableMenu);

   selectAllParameters->build("select ");
   selectAllParameters->bind(cTableMenu::fiId, cDBS::bndOut);
   selectAllParameters->bind(cTableMenu::fiAddress, cDBS::bndOut, ", ");
   selectAllParameters->bind(cTableMenu::fiType, cDBS::bndOut, ", ");
   selectAllParameters->bind(cTableMenu::fiUnit, cDBS::bndOut, ", ");
   selectAllParameters->bind(cTableMenu::fiValue, cDBS::bndOut, ", ");
   selectAllParameters->bind(cTableMenu::fiTitle, cDBS::bndOut, ", ");
   selectAllParameters->build(" from %s", tableMenu->TableName());

   status = selectAllParameters->prepare();

   // ------------------

   selectPendingJobs = new cDbStatement(tableJobs);

   selectPendingJobs->build("select ");
   selectPendingJobs->bind(cTableJobs::fiId, cDBS::bndOut);
   selectPendingJobs->bind(cTableJobs::fiReqAt, cDBS::bndOut, ", ");
   selectPendingJobs->bind(cTableJobs::fiState, cDBS::bndOut, ", ");
   selectPendingJobs->bind(cTableJobs::fiCommand, cDBS::bndOut, ", ");
   selectPendingJobs->bind(cTableJobs::fiAddress, cDBS::bndOut, ", ");
   selectPendingJobs->build(" from %s where state = 'P'", tableJobs->TableName());

   status = selectPendingJobs->prepare();

   // ------------------

   tell(eloAlways, "Connection to database established");  

   return status;
}

int P4sd::exitDb()
{
   delete tableSamples;        tableSamples = 0;
   delete tableValueFacts;     tableValueFacts = 0;
   delete tableMenu; tableMenu = 0;
   delete tableJobs;           tableJobs = 0;
   delete tableSchemaConf;     tableSchemaConf = 0;

   delete selectActiveValueFacts;  selectActiveValueFacts = 0;
   delete selectAllValueFacts;     selectAllValueFacts = 0;
   delete selectPendingJobs;       selectPendingJobs = 0;
   delete selectAllParameters;     selectAllParameters = 0;

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
   updateConfTables();
   tell(eloAlways, "Requesting menu structure from s 3200");
   updateMenu();

   serial->close();

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
// Update Menu Structure
//***************************************************************************

int P4sd::updateMenu()
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

      tell(eloDebug, "%3d) Address: 0x%04x, parent: 0x%04x, child: 0x%04x; '%s'", count++, m.parent, m.child, m.description);

      // update table    

      tableMenu->clear();

      tableMenu->setValue(cTableMenu::fiState, "D");
      tableMenu->setValue(cTableMenu::fiUnit, m.unit);
      
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
   {
      meanwhile();
      usleep(100000);
   }

   return done;
}

int P4sd::standbyUntil(time_t until)
{
   while (time(0) < until && !doShutDown())
   {
      meanwhile();
      usleep(100000);
   }

   return done;
}

//***************************************************************************
// Meanwhile
//***************************************************************************

int P4sd::meanwhile()
{
   tableJobs->clear();

   for (int f = selectPendingJobs->find(); f; f = selectPendingJobs->fetch())
   {
      int start = time(0);
      int addr = tableJobs->getIntValue(cTableJobs::fiAddress);
      const char* command = tableJobs->getStrValue(cTableJobs::fiCommand);

      tableJobs->find();
      tableJobs->setValue(cTableJobs::fiDoneAt, time(0));
      tableJobs->setValue(cTableJobs::fiState, "D");

      tell(eloAlways, "Processing WEBIF job %d '%s:0x%04x'", 
           tableJobs->getIntValue(cTableJobs::fiId),
           command, addr);

      if (strcasecmp(command, "getp") == 0)
      {
         Fs::ConfigParameter p(addr);

         if (request->getParameter(&p) == success)
         {
            char* buf = 0;

            asprintf(&buf, "success:%d%s", p.value, p.unit);
            tableJobs->setValue(cTableJobs::fiResult, buf);
            free(buf);
         }
      }

      else if (strcasecmp(command, "initmenu") == 0)
      {
         updateMenu();
         tableJobs->setValue(cTableJobs::fiResult, "success");
      }

      else if (strcasecmp(command, "updatemenu") == 0)
      {
         tableMenu->clear();

         for (int f = selectAllParameters->find(); f; f = selectAllParameters->fetch())
         {
            // int id = tableMenu->getIntValue(cTableMenu::fiId);
            int type = tableMenu->getIntValue(cTableMenu::fiType);
            int paddr = tableMenu->getIntValue(cTableMenu::fiAddress);

            if (type == 0x07 || type == 0x08 || type == 0x0a || 
                type == 0x40 || type == 0x39 || type == 0x32)
            {
               Fs::ConfigParameter p(paddr);
               
               if (request->getParameter(&p) == success)
               {
                  char* buf = 0;
                  
                  if (type == 0x08)
                     asprintf(&buf, "%s", p.value ? "ja" : "nein");
                  else if (type == 0x0a)
                     asprintf(&buf, "%02d:%02d", p.value/60, p.value%60);
                  else
                     asprintf(&buf, "%d", p.value);

                  if (tableMenu->find())
                  {
                     tableMenu->setValue(cTableMenu::fiValue, buf);
                     tableMenu->setValue(cTableMenu::fiUnit, p.unit);
                     tableMenu->update();
                  }

                  free(buf);
               }
            }
            else if (type == 0x11 || type == 0x13)
            {
               int status;
               Fs::IoValue v(paddr);

               if (type == 0x11)
                  status = request->getDigitalOut(&v);
               else
                  status = request->getDigitalIn(&v);

               if (status == success)
               {
                  char* buf = 0;
                  asprintf(&buf, "%s (%c)", v.state ? "on" : "off", v.mode);

                  if (tableMenu->find())
                  {
                     tableMenu->setValue(cTableMenu::fiValue, buf);
                     tableMenu->setValue(cTableMenu::fiUnit, "");
                     tableMenu->update();
                  }

                  free(buf);
               }
            }
            else if (type == 0x12)
            {
               Fs::IoValue v(paddr);
               
               if (request->getAnalogOut(&v) == success)
               {
                  char* buf = 0;

                  if (v.mode == 0xff)
                     asprintf(&buf, "%d (A)", v.state);
                  else
                     asprintf(&buf, "%d (%d)", v.state, v.mode);

                  if (tableMenu->find())
                  {
                     tableMenu->setValue(cTableMenu::fiValue, buf);
                     tableMenu->setValue(cTableMenu::fiUnit, "");
                     tableMenu->update();
                  }

                  free(buf);
               }
            }
         }

         selectAllParameters->freeResult();
         tableJobs->setValue(cTableJobs::fiResult, "success");
      }

      else
      {
         tell(eloAlways, "Warning: Ignoring unknown job '%s'", command);
         tableJobs->setValue(cTableJobs::fiResult, "fail:unknown command");
      }

      tableJobs->store();
      tell(eloAlways, "Processing WEBIF job %d done after %d seconds", 
           tableJobs->getIntValue(cTableJobs::fiId), time(0) - start);
   }

   selectPendingJobs->freeResult();

   return success;
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

   sem->p();

   if (serial->open(ttyDeviceSvc) != success)
   {
      sem->v();
      return fail;
   }

   sem->v();

   while (!doShutDown())
   {
      int stateChanged = no;

      // check db connection

      while (!doShutDown() && !tableSamples->isConnected())
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
   
   serial->close();

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
