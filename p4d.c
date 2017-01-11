//***************************************************************************
// p4d / Linux - Heizungs Manager
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
#include <libxml/parser.h>

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
   tableSmartConf = 0;
   tableValueFacts = 0;
   tableMenu = 0;
   tableConfig = 0;
   tableErrors = 0;
   tableTimeRanges = 0;
   selectHmSysVarByAddr = 0;

   selectActiveValueFacts = 0;
   selectAllValueFacts = 0;
   selectPendingJobs = 0;
   selectAllMenuItems = 0;
   selectSensorAlerts = 0;
   selectSampleInRange = 0;
   selectHmSysVarByAddr = 0;
   cleanupJobs = 0;

   nextAt = time(0);
   startedAt = time(0);
   nextAggregateAt = 0;
   nextTimeSyncAt = 0;

   mailBody = "";
   mailBodyHtml = "";
   mail = no;
   htmlMail = no;
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
   curl = new cCurl();
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
   delete curl;

   cDbConnection::exit();
}

//***************************************************************************
// Init / Exit
//***************************************************************************

int P4d::init()
{
   char* dictPath = 0;

   curl->init();

   // initialize the dictionary

   asprintf(&dictPath, "%s/p4d.dat", confDir);

   if (dbDict.in(dictPath) != success)
   {
      tell(0, "Fatal: Dictionary not loaded, aborting!");
      return 1;
   }

   tell(0, "Dictionary '%s' loaded", dictPath);
   free(dictPath);

   // initialize the database resources

   if (initDb() != success)
      return fail;

   readConfiguration();

   // prepare one wire sensors

   w1.scan();

   return success;
}

int P4d::exit()
{
   exitDb();
   serial->close();
   curl->exit();

   return success;
}

//***************************************************************************
// Init/Exit Database
//***************************************************************************

cDbFieldDef rangeEndDef("time", "time", cDBS::ffDateTime, 0, cDBS::ftData);

int P4d::initDb()
{
   static int initial = yes;
   int status = success;

   if (connection)
      exitDb();

   tell(eloAlways, "Try conneting to database");

   connection = new cDbConnection();

   if (initial)
   {
      // ------------------------------------------
      // initially create/alter tables and indices
      // ------------------------------------------

      tell(0, "Checking database connection ...");

      if (connection->attachConnection() != success)
      {
         tell(0, "Fatal: Initial database connect failed, aborting");
         return abrt;
      }

      std::map<std::string, cDbTableDef*>::iterator t;

      tell(0, "Checking table structure and indices ...");

      for (t = dbDict.getFirstTableIterator(); t != dbDict.getTableEndIterator(); t++)
      {
         cDbTable* table = new cDbTable(connection, t->first.c_str());

         tell(1, "Checking table '%s'", t->first.c_str());

         if (!table->exist())
         {
            if ((status += table->createTable()) != success)
               continue;
         }
         else
         {
            status += table->validateStructure();
         }

         status += table->createIndices();

         delete table;
      }

      connection->detachConnection();

      if (status != success)
         return abrt;

      tell(0, "Checking table structure and indices succeeded");
   }

   // ------------------------
   // create/open tables
   // ------------------------

   tableValueFacts = new cDbTable(connection, "valuefacts");
   if (tableValueFacts->open() != success) return fail;

   tableErrors = new cDbTable(connection, "errors");
   if (tableErrors->open() != success) return fail;

   tableMenu = new cDbTable(connection, "menu");
   if (tableMenu->open() != success) return fail;

   tableSamples = new cDbTable(connection, "samples");
   if (tableSamples->open() != success) return fail;

   tableJobs = new cDbTable(connection, "jobs");
   if (tableJobs->open() != success) return fail;

   tableSensorAlert = new cDbTable(connection, "sensoralert");
   if (tableSensorAlert->open() != success) return fail;

   tableSchemaConf = new cDbTable(connection, "schemaconf");
   if (tableSchemaConf->open() != success) return fail;

   tableSmartConf = new cDbTable(connection, "smartconfig");
   if (tableSmartConf->open() != success) return fail;

   tableConfig = new cDbTable(connection, "config");
   if (tableConfig->open() != success) return fail;

   tableTimeRanges = new cDbTable(connection, "timeranges");
   if (tableTimeRanges->open() != success) return fail;

   tableHmSysVars = new cDbTable(connection, "hmsysvars");
   if (tableHmSysVars->open() != success) return fail;

   // prepare statements

   selectActiveValueFacts = new cDbStatement(tableValueFacts);

   selectActiveValueFacts->build("select ");
   selectActiveValueFacts->bindAllOut();
   selectActiveValueFacts->build(" from %s where ", tableValueFacts->TableName());
   selectActiveValueFacts->bind("STATE", cDBS::bndIn | cDBS::bndSet);

   status += selectActiveValueFacts->prepare();

   // ------------------

   selectAllValueFacts = new cDbStatement(tableValueFacts);

   selectAllValueFacts->build("select ");
   selectAllValueFacts->bindAllOut();
   selectAllValueFacts->build(" from %s", tableValueFacts->TableName());

   status += selectAllValueFacts->prepare();

   // ----------------

   selectAllMenuItems = new cDbStatement(tableMenu);

   selectAllMenuItems->build("select ");
   selectAllMenuItems->bindAllOut();
   selectAllMenuItems->build(" from %s", tableMenu->TableName());

   status += selectAllMenuItems->prepare();

   // ------------------

   selectPendingJobs = new cDbStatement(tableJobs);

   selectPendingJobs->build("select ");
   selectPendingJobs->bind("ID", cDBS::bndOut);
   selectPendingJobs->bind("REQAT", cDBS::bndOut, ", ");
   selectPendingJobs->bind("STATE", cDBS::bndOut, ", ");
   selectPendingJobs->bind("COMMAND", cDBS::bndOut, ", ");
   selectPendingJobs->bind("ADDRESS", cDBS::bndOut, ", ");
   selectPendingJobs->bind("DATA", cDBS::bndOut, ", ");
   selectPendingJobs->build(" from %s where state = 'P'", tableJobs->TableName());

   status += selectPendingJobs->prepare();

   // ------------------

   selectSensorAlerts = new cDbStatement(tableSensorAlert);

   selectSensorAlerts->build("select ");
   selectSensorAlerts->bindAllOut();
   selectSensorAlerts->build(" from %s where state = 'A'", tableSensorAlert->TableName());
   selectSensorAlerts->bind("KIND", cDBS::bndIn | cDBS::bndSet, " and ");

   status += selectSensorAlerts->prepare();

   // ------------------
   // select * from samples
   //    where type = ? and address = ?
   //     and time <= ?
   //     and time > ?

   rangeEnd.setField(&rangeEndDef);

   selectSampleInRange = new cDbStatement(tableSamples);

   selectSampleInRange->build("select ");
   selectSampleInRange->bind("ADDRESS", cDBS::bndOut);
   selectSampleInRange->bind("TYPE", cDBS::bndOut, ", ");
   selectSampleInRange->bind("TIME", cDBS::bndOut, ", ");
   selectSampleInRange->bind("VALUE", cDBS::bndOut, ", ");
   selectSampleInRange->build(" from %s where ", tableSamples->TableName());
   selectSampleInRange->bind("ADDRESS", cDBS::bndIn | cDBS::bndSet);
   selectSampleInRange->bind("TYPE", cDBS::bndIn | cDBS::bndSet, " and ");
   selectSampleInRange->bindCmp(0, &rangeEnd, "<=", " and ");
   selectSampleInRange->bindCmp(0, "TIME", 0, ">", " and ");
   selectSampleInRange->build(" order by time");

   status += selectSampleInRange->prepare();

   // ------------------

   selectHmSysVarByAddr = new cDbStatement(tableHmSysVars);

   selectHmSysVarByAddr->build("select ");
   selectHmSysVarByAddr->bindAllOut();
   selectHmSysVarByAddr->build(" from %s where ", tableHmSysVars->TableName());
   selectHmSysVarByAddr->bind("ADDRESS", cDBS::bndIn | cDBS::bndSet);
   selectHmSysVarByAddr->bind("ATYPE", cDBS::bndIn | cDBS::bndSet, " and ");

   status += selectHmSysVarByAddr->prepare();

   // ------------------

   cleanupJobs = new cDbStatement(tableJobs);

   cleanupJobs->build("delete from %s where ", tableJobs->TableName());
   cleanupJobs->bindCmp(0, "REQAT", 0, "<");

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
   delete tableSmartConf;          tableSmartConf = 0;
   delete tableErrors;             tableErrors = 0;
   delete tableConfig;             tableConfig = 0;
   delete tableTimeRanges;         tableTimeRanges = 0;
   delete tableHmSysVars;          tableHmSysVars = 0;

   delete selectActiveValueFacts;  selectActiveValueFacts = 0;
   delete selectAllValueFacts;     selectAllValueFacts = 0;
   delete selectPendingJobs;       selectPendingJobs = 0;
   delete selectAllMenuItems;      selectAllMenuItems = 0;
   delete selectSensorAlerts;      selectSensorAlerts = 0;
   delete selectSampleInRange;     selectSampleInRange = 0;
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

   free(webUser);
   free(webPass);

   // init configuration

   getConfigItem("mail", mail, no);
   getConfigItem("htmlMail", htmlMail, no);
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
      tableSmartConf->truncate();
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
   initMenu();

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
      char oldState = tableValueFacts->getStrValue("STATE")[0];
      char state = oldState;

      printf("%s 0x%04lx '%s' (%s)",
             tableValueFacts->getStrValue("TYPE"),
             tableValueFacts->getIntValue("ADDRESS"),
             tableValueFacts->getStrValue("UNIT"),
             tableValueFacts->getStrValue("TITLE"));

      printf(" - aufzeichnen? (%s): ", oldState == 'A' ? "Y/n" : "y/N");

      if ((res = fgets(buf, 100, stdin)) && strlen(res) > 1)
         state = toupper(res[0]) == 'Y' ? 'A' : 'D';

      if (state != oldState && tableValueFacts->find())
      {
         tableValueFacts->setCharValue("STATE", state);
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
      int addr = tableValueFacts->getIntValue("ADDRESS");
      const char* type = tableValueFacts->getStrValue("TYPE");
      y += step;

      tableSchemaConf->clear();
      tableSchemaConf->setValue("ADDRESS", addr);
      tableSchemaConf->setValue("TYPE", type);

      if (!tableSchemaConf->find())
      {
         tableSchemaConf->setValue("KIND", "value");
         tableSchemaConf->setValue("STATE", "A");
         tableSchemaConf->setValue("COLOR", "black");
         tableSchemaConf->setValue("XPOS", 12);
         tableSchemaConf->setValue("YPOS", y);

         tableSchemaConf->store();
         added++;
      }

      tableSmartConf->clear();
      tableSmartConf->setValue("ADDRESS", addr);
      tableSmartConf->setValue("TYPE", type);

      if (!tableSmartConf->find())
      {
         tableSmartConf->setValue("KIND", "value");
         tableSmartConf->setValue("STATE", "A");
         tableSmartConf->setValue("COLOR", "black");
         tableSmartConf->setValue("XPOS", 12);
         tableSmartConf->setValue("YPOS", y);

         tableSmartConf->store();
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
   int modified;

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
   modified = 0;

   for (status = request->getFirstValueSpec(&v); status != Fs::wrnLast;
        status = request->getNextValueSpec(&v))
   {
      if (status != success)
         continue;

      tell(eloDebug, "%3d) 0x%04x %4d '%s' (%04d) '%s'",
           count, v.address, v.factor, v.unit, v.unknown, v.description);

      // update table

      tableValueFacts->clear();
      tableValueFacts->setValue("ADDRESS", v.address);
      tableValueFacts->setValue("TYPE", "VA");

      if (!tableValueFacts->find())
      {
         tableValueFacts->setValue("NAME", v.name);
         tableValueFacts->setValue("STATE", "D");
         tableValueFacts->setValue("UNIT", v.unit);
         tableValueFacts->setValue("FACTOR", v.factor);
         tableValueFacts->setValue("TITLE", v.description);
         tableValueFacts->setValue("RES1", v.unknown);

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
   modified = 0;

   for (int f = selectAllMenuItems->find(); f; f = selectAllMenuItems->fetch())
   {
      char* name = 0;
      const char* type = 0;
      int structType = tableMenu->getIntValue("TYPE");
      string sname = tableMenu->getStrValue("TITLE");

      switch (structType)
      {
         case mstDigOut: type = "DO"; break;
         case mstDigIn:  type = "DI"; break;
         case mstAnlOut: type = "AO"; break;
      }

      if (!type)
         continue;

      // update table

      tableValueFacts->clear();
      tableValueFacts->setValue("ADDRESS", tableMenu->getIntValue("ADDRESS"));
      tableValueFacts->setValue("TYPE", type);
      tableValueFacts->clearChanged();

      if (!tableValueFacts->find())
      {
         tableValueFacts->setValue("STATE", "D");
         added++;
         modified--;
      }

      removeCharsExcept(sname, nameChars);
      asprintf(&name, "%s_0x%x", sname.c_str(), (int)tableMenu->getIntValue("ADDRESS"));

      tableValueFacts->setValue("NAME", name);
      tableValueFacts->setValue("UNIT", tableMenu->getStrValue("UNIT"));
      tableValueFacts->setValue("FACTOR", 1);
      tableValueFacts->setValue("TITLE", tableMenu->getStrValue("TITLE"));

      if (tableValueFacts->getChanges())
      {
         tableValueFacts->store();
         modified++;
      }

      free(name);
      count++;
   }

   selectAllMenuItems->freeResult();
   tell(eloAlways, "Checked %d digital lines, added %d, modified %d", count, added, modified);

   // ---------------------------------
   // at least add value definitions for special data

   count = 0;
   added = 0;
   modified = 0;

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", udState);      // 1  -> Kessel Status
   tableValueFacts->setValue("TYPE", "UD");            // UD -> User Defined

   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue("NAME", "Status");
      tableValueFacts->setValue("STATE", "A");
      tableValueFacts->setValue("UNIT", "zst");
      tableValueFacts->setValue("FACTOR", 1);
      tableValueFacts->setValue("TITLE", "Heizungsstatus");

      tableValueFacts->store();
      added++;
   }

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", udMode);       // 2  -> Kessel Mode
   tableValueFacts->setValue("TYPE", "UD");            // UD -> User Defined

   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue("NAME", "Betriebsmodus");
      tableValueFacts->setValue("STATE", "A");
      tableValueFacts->setValue("UNIT", "zst");
      tableValueFacts->setValue("FACTOR", 1);
      tableValueFacts->setValue("TITLE", "Betriebsmodus");

      tableValueFacts->store();
      added++;
   }

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", udTime);       // 3  -> Kessel Zeit
   tableValueFacts->setValue("TYPE", "UD");            // UD -> User Defined

   if (!tableValueFacts->find())
   {
      tableValueFacts->setValue("NAME", "Uhrzeit");
      tableValueFacts->setValue("STATE", "A");
      tableValueFacts->setValue("UNIT", "T");
      tableValueFacts->setValue("FACTOR", 1);
      tableValueFacts->setValue("TITLE", "Datum Uhrzeit der Heizung");

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
      modified = 0;

      for (W1::SensorList::iterator it = list->begin(); it != list->end(); ++it)
      {
         // update table

         tableValueFacts->clear();
         tableValueFacts->setValue("ADDRESS", (int)W1::toId(it->first.c_str()));
         tableValueFacts->setValue("TYPE", "W1");

         if (!tableValueFacts->find())
         {
            tableValueFacts->setValue("NAME", it->first.c_str());
            tableValueFacts->setValue("STATE", "D");
            tableValueFacts->setValue("UNIT", "°");
            tableValueFacts->setValue("FACTOR", 1);
            tableValueFacts->setValue("TITLE", it->first.c_str());

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
// Synchronize HM System Variables
//***************************************************************************

int P4d::hmSyncSysVars()
{
   char* hmUrl = 0;
   char* hmHost = 0;
   xmlDoc* document = 0;
   xmlNode* root = 0;
   int readOptions = 0;
   MemoryStruct data;
   int count = 0;
   int size = 0;

#if LIBXML_VERSION >= 20900
   readOptions |=  XML_PARSE_HUGE;
#endif

   getConfigItem("hmHost", hmHost, "");

   if (isEmpty(hmHost))
      return done;

   tell(eloAlways, "Updating HomeMatic system variables");
   asprintf(&hmUrl, "http://%s/config/xmlapi/sysvarlist.cgi", hmHost);

   if (curl->downloadFile(hmUrl, size, &data) != success)
   {
      tell(0, "Error: Requesting sysvar list at homematic '%s' failed", hmUrl);
      free(hmUrl);
      return fail;
   }

   free(hmUrl);

   tell(3, "Got [%s]", data.memory ? data.memory : "<null>");

   if (document = xmlReadMemory(data.memory, data.size, "", 0, readOptions))
      root = xmlDocGetRootElement(document);

   if (!root)
   {
      tell(0, "Error: Failed to parse XML document [%s]", data.memory ? data.memory : "<null>");
      return fail;
   }

   for (xmlNode* node = root->children; node; node = node->next)
   {
      xmlChar* id = xmlGetProp(node, (xmlChar*)"ise_id");
      xmlChar* name = xmlGetProp(node, (xmlChar*)"name");
      xmlChar* type = xmlGetProp(node, (xmlChar*)"type");
      xmlChar* unit = xmlGetProp(node, (xmlChar*)"unit");
      xmlChar* visible = xmlGetProp(node, (xmlChar*)"visible");
      xmlChar* min = xmlGetProp(node, (xmlChar*)"min");
      xmlChar* max = xmlGetProp(node, (xmlChar*)"max");
      xmlChar* time = xmlGetProp(node, (xmlChar*)"timestamp");
      xmlChar* value = xmlGetProp(node, (xmlChar*)"value");

      tableHmSysVars->clear();
      tableHmSysVars->setValue("ID", atol((const char*)id));
      tableHmSysVars->find();
      tableHmSysVars->setValue("NAME", (const char*)name);
      tableHmSysVars->setValue("TYPE", atol((const char*)type));
      tableHmSysVars->setValue("UNIT", (const char*)unit);
      tableHmSysVars->setValue("VISIBLE", strcmp((const char*)visible, "true") == 0);
      tableHmSysVars->setValue("MIN", (const char*)min);
      tableHmSysVars->setValue("MAX", (const char*)max);
      tableHmSysVars->setValue("TIME", atol((const char*)time));
      tableHmSysVars->setValue("VALUE", (const char*)value);
      tableHmSysVars->store();

      xmlFree(id);
      xmlFree(name);
      xmlFree(type);
      xmlFree(unit);
      xmlFree(visible);
      xmlFree(min);
      xmlFree(max);
      xmlFree(time);
      xmlFree(value);

      count++;
   }

   tell(eloAlways, "Upate of (%d) HomeMatic system variables succeeded", count);

   return success;
}

//***************************************************************************
// Initialize Menu Structure
//***************************************************************************

int P4d::initMenu()
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

      tableMenu->setValue("STATE", "D");
      tableMenu->setValue("UNIT", m.type == mstAnlOut && isEmpty(m.unit) ? "%" : m.unit);

      tableMenu->setValue("PARENT", m.parent);
      tableMenu->setValue("CHILD", m.child);
      tableMenu->setValue("ADDRESS", m.address);
      tableMenu->setValue("TITLE", m.description);

      tableMenu->setValue("TYPE", m.type);
      tableMenu->setValue("UNKNOWN1", m.unknown1);
      tableMenu->setValue("UNKNOWN2", m.unknown2);

      tableMenu->insert();

      count++;
   }

   tell(eloAlways, "Read %d menu items", count);

   return success;
}

//***************************************************************************
// Store
//***************************************************************************

int P4d::store(time_t now, const char* type, int address, double value,
               unsigned int factor, const char* text)
{
   static time_t lastHmFailAt = 0;

   double theValue = value / (double)factor;

   tableSamples->clear();

   tableSamples->setValue("TIME", now);
   tableSamples->setValue("ADDRESS", address);
   tableSamples->setValue("TYPE", type);
   tableSamples->setValue("AGGREGATE", "S");

   tableSamples->setValue("VALUE", theValue);
   tableSamples->setValue("TEXT", text);
   tableSamples->setValue("SAMPLES", 1);

   tableSamples->store();

   // HomeMatic

   if (lastHmFailAt < time(0) - 3*tmeSecondsPerMinute)  // on fail retry not before 3 minutes
   {
      char* hmHost = 0;
      char* hmUrl = 0;
      MemoryStruct data;
      int size = 0;

      getConfigItem("hmHost", hmHost, "");

      if (!isEmpty(hmHost))
      {
         tableHmSysVars->clear();
         tableHmSysVars->setValue("ADDRESS", address);
         tableHmSysVars->setValue("ATYPE", type);

         if (selectHmSysVarByAddr->find())
         {
            char* buf;

            asprintf(&buf, "%f", theValue);
            tableHmSysVars->setValue("VALUE", buf);
            free(buf);

            tableHmSysVars->setValue("TIME", now);
            tableHmSysVars->update();

            asprintf(&hmUrl, "http://%s/config/xmlapi/statechange.cgi?ise_id=%ld&new_value=%f;",
                     hmHost, tableHmSysVars->getIntValue("ID"), theValue);

            if (curl->downloadFile(hmUrl, size, &data) != success)
            {
               tell(0, "Error: Requesting sysvar change at homematic %s failed [%s]", hmHost, hmUrl);
               lastHmFailAt = time(0);
               free(hmUrl);
               return fail;
            }

            tell(1, "Info: Call of [%s] succeeded", hmUrl);
            free(hmUrl);
         }

         selectHmSysVarByAddr->freeResult();
      }
   }
   else
   {
      tell(1, "Skipping HomeMatic request due to error within the last 3 minutes");
   }

   return success;
}

//***************************************************************************
// Schedule Time Sync In
//***************************************************************************

void P4d::scheduleTimeSyncIn(int offset)
{
   struct tm tm = {0};
   time_t now;

   now = time(0);
   localtime_r(&now, &tm);

   tm.tm_sec = 0;
   tm.tm_min = 0;
   tm.tm_hour = 23;
   tm.tm_isdst = -1;               // force DST auto detect

   nextTimeSyncAt = mktime(&tm);
   nextTimeSyncAt += offset;
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
         sendStateMail();

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
   static time_t nextReportAt = 0;

   int status;
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

   if (!nextTimeSyncAt)
      scheduleTimeSyncIn();

   if (tSync && maxTimeLeak && labs(state->time - now) > maxTimeLeak)
   {
      if (now > nextReportAt)
      {
         tell(eloAlways, "Time drift is %ld seconds", state->time - now);
         nextReportAt = now + 2 * tmeSecondsPerMinute;
      }

      if (now > nextTimeSyncAt)
      {
         scheduleTimeSyncIn(tmeSecondsPerDay);

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
   tableValueFacts->setValue("STATE", "A");

   for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
   {
      int addr = tableValueFacts->getIntValue("ADDRESS");
      double factor = tableValueFacts->getIntValue("FACTOR");
      const char* title = tableValueFacts->getStrValue("TITLE");
      const char* type = tableValueFacts->getStrValue("TYPE");
      const char* unit = tableValueFacts->getStrValue("UNIT");
      const char* name = tableValueFacts->getStrValue("NAME");

      if (!tableValueFacts->getValue("USRTITLE")->isEmpty())
         title = tableValueFacts->getStrValue("USRTITLE");

      if (tableValueFacts->hasValue("TYPE", "VA"))
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

         addParameter2Mail(title, num);
      }

      else if (tableValueFacts->hasValue("TYPE", "DO"))
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

      else if (tableValueFacts->hasValue("TYPE", "DI"))
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

      else if (tableValueFacts->hasValue("TYPE", "AO"))
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

      else if (tableValueFacts->hasValue("TYPE", "W1"))
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

      else if (tableValueFacts->hasValue("TYPE", "UD"))
      {
         switch (tableValueFacts->getIntValue("ADDRESS"))
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
               addParameter2Mail(title, currentState.modeinfo);

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
   afterUpdate();

   sensorAlertCheck(now);

   updateErrors();

   return success;
}

//***************************************************************************
// After Update
//***************************************************************************

void P4d::afterUpdate()
{
   char* path = 0;

   asprintf(&path, "%s/after-update.sh", confDir);

   if (fileExists(path))
   {
      tell(0, "Calling '%s'", path);
      system(path);
   }

   free(path);
}

//***************************************************************************
// Sensor Alert Check
//***************************************************************************

void P4d::sensorAlertCheck(time_t now)
{
   tableSensorAlert->clear();
   tableSensorAlert->setValue("KIND", "M");

   // iterate over all alert roules ..

   for (int f = selectSensorAlerts->find(); f; f = selectSensorAlerts->fetch())
   {
      alertMailBody = "";
      alertMailSubject = "";

      performAlertCheck(tableSensorAlert->getRow(), now, 0);
   }

   selectSensorAlerts->freeResult();
}

//***************************************************************************
// Perform Alert Check
//***************************************************************************

int P4d::performAlertCheck(cDbRow* alertRow, time_t now, int recurse)
{
   int alert = 0;

   // data from alert row

   int addr = alertRow->getIntValue("ADDRESS");
   const char* type = alertRow->getStrValue("TYPE");

   int id = alertRow->getIntValue("ID");
   int lgop = alertRow->getIntValue("LGOP");
   time_t lastAlert = alertRow->getIntValue("LASTALERT");
   int maxRepeat = alertRow->getIntValue("MAXREPEAT");

   int minIsNull = alertRow->getValue("MIN")->isNull();
   int maxIsNull = alertRow->getValue("MAX")->isNull();
   int min = alertRow->getIntValue("MIN");
   int max = alertRow->getIntValue("MAX");

   int rangeIsNull = alertRow->getValue("RANGEM")->isNull();
   int deltaIsNull = alertRow->getValue("DELTA")->isNull();
   int range = alertRow->getIntValue("RANGEM");
   int delta = alertRow->getIntValue("DELTA");

   // lookup value facts

   tableValueFacts->clear();
   tableValueFacts->setValue("ADDRESS", addr);
   tableValueFacts->setValue("TYPE", type);

   // lookup samples

   tableSamples->clear();
   tableSamples->setValue("ADDRESS", addr);
   tableSamples->setValue("TYPE", type);
   tableSamples->setValue("AGGREGATE", "S");
   tableSamples->setValue("TIME", now);

   if (!tableSamples->find() || !tableValueFacts->find())
   {
      tell(eloAlways, "Info: Can't perform sensor check for %s/%d", type, addr);
      return 0;
   }

   // data from samples and value facts

   double value = tableSamples->getFloatValue("VALUE");

   const char* title = tableValueFacts->getStrValue("TITLE");
   const char* unit = tableValueFacts->getStrValue("UNIT");

   // -------------------------------
   // check min / max threshold

   if (!minIsNull || !maxIsNull)
   {
      if ((!minIsNull && value < min) || (!maxIsNull && value > max))
      {
         tell(eloAlways, "%d) Alert for sensor %s/0x%x, value %.2f not in range (%d - %d)",
              id, type, addr, value, min, max);

         // max one alert mail per maxRepeat [minutes]

         if (!lastAlert || lastAlert < time(0)- maxRepeat * tmeSecondsPerMinute)
         {
            alert = 1;
            add2AlertMail(alertRow, title, value, unit);
         }
      }
   }

   // -------------------------------
   // check range delta

   if (!rangeIsNull && !deltaIsNull)
   {
      // select value of this sensor around 'time = (now - range)'

      time_t rangeStartAt = time(0) - range*tmeSecondsPerMinute;
      time_t rangeEndAt = rangeStartAt + interval;

      tableSamples->clear();
      tableSamples->setValue("ADDRESS", addr);
      tableSamples->setValue("TYPE", type);
      tableSamples->setValue("AGGREGATE", "S");
      tableSamples->setValue("TIME", rangeStartAt);
      rangeEnd.setValue(rangeEndAt);

      if (selectSampleInRange->find())
      {
         double oldValue = tableSamples->getFloatValue("VALUE");

         if (labs(value - oldValue) > delta)
         {
            tell(eloAlways, "%d) Alert for sensor %s/0x%x , value %.2f changed more than %d in %d minutes",
                 id, type, addr, value, delta, range);

            // max one alert mail per maxRepeat [minutes]

            if (!lastAlert || lastAlert < time(0)- maxRepeat * tmeSecondsPerMinute)
            {
               alert = 1;
               add2AlertMail(alertRow, title, value, unit);
            }
         }
      }

      selectSampleInRange->freeResult();
      tableSamples->reset();
   }

   // ---------------------------
   // Check sub rules recursive

   if (alertRow->getIntValue("SUBID") > 0)
   {
      if (recurse > 50)
      {
         tell(eloAlways, "Info: Aborting recursion after 50 steps, seems to be a config error!");
      }
      else
      {
         tableSensorAlert->clear();
         tableSensorAlert->setValue("ID", alertRow->getIntValue("SUBID"));

         if (tableSensorAlert->find())
         {
            int sAlert = performAlertCheck(tableSensorAlert->getRow(), now, recurse+1);

            switch (lgop)
            {
               case loAnd:    alert = alert &&  sAlert; break;
               case loOr:     alert = alert ||  sAlert; break;
               case loAndNot: alert = alert && !sAlert; break;
               case loOrNot:  alert = alert || !sAlert; break;
            }
         }
      }
   }

   // ---------------------------------
   // update master row and send mail

   if (alert && !recurse)
   {
      tableSensorAlert->clear();
      tableSensorAlert->setValue("ID", id);

      if (tableSensorAlert->find())
      {
         tableSensorAlert->setValue("LASTALERT", time(0));
         tableSensorAlert->update();

         sendAlertMail(tableSensorAlert->getStrValue("MADDRESS"));
      }
   }

   return alert;
}

//***************************************************************************
// Add Parameter To Mail
//***************************************************************************

void P4d::addParameter2Mail(const char* name, const char* value)
{
   char buf[500+TB];

   mailBody += string(name) + " = " + string(value) + "\n";

   sprintf(buf, "        <tr><td>%s</td><td>%s</td></tr>\n", name, value);

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
   tm.tm_isdst = -1;               // force DST auto detect

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

   if (connection->query(aggCount, "%s", stmt) == success)
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

      tableErrors->setValue("TIME", e.time);
      tableErrors->setValue("NUMBER", e.number);
      tableErrors->setValue("INFO", e.info);
      tableErrors->setValue("STATE", Fs::errState2Text(e.state));
      tableErrors->setValue("TEXT", e.text);

      tableErrors->insert();
   }

   return success;
}

//***************************************************************************
// Send Mail
//***************************************************************************

int P4d::sendAlertMail(const char* to)
{
   // check

   if (isEmpty(to) || isEmpty(mailScript))
      return done;

   if (alertMailBody.empty())
      alertMailBody = "- undefined -";

   if (htmlMail)
   {
      char* html = 0;

      alertMailBody = strReplace("\n", "<br>\n", alertMailBody);

      const char* htmlHead =
         "<head>"
         "  <style type=\"text/css\">\n"
         "    caption { background: #095BA6; font-family: Arial Narrow; color: #fff; font-size: 18px; }\n"
         "  </style>\n"
         "</head>\n";

      asprintf(&html,
               "<html>\n"
               " %s"
               " <body>\n"
               "  %s\n"
               " </body>\n"
               "</html>\n",
               htmlHead, alertMailBody.c_str());

      alertMailBody = html;
      free(html);
   }

   // send mail

   sendMail(to, alertMailSubject.c_str(), alertMailBody.c_str(),
            htmlMail ? "text/html" : "text/plain");

   return success;
}

//***************************************************************************
// Send Mail
//***************************************************************************

int P4d::add2AlertMail(cDbRow* alertRow, const char* title,
                           double value, const char* unit)
{
   char* webUrl = 0;
   char* sensor = 0;
   char* htmlWebUrl = 0;

   string subject = alertRow->getStrValue("MSUBJECT");
   string body = alertRow->getStrValue("MBODY");
   int addr = alertRow->getIntValue("ADDRESS");
   const char* type = alertRow->getStrValue("TYPE");

   int min = alertRow->getIntValue("MIN");
   int max = alertRow->getIntValue("MAX");
   int range = alertRow->getIntValue("RANGEM");
   int delta = alertRow->getIntValue("DELTA");
   int maxRepeat = alertRow->getIntValue("MAXREPEAT");

   if (!body.length())
      body = "- undefined -";

   getConfigItem("webUrl", webUrl, "http://");
   asprintf(&htmlWebUrl, "<a href=\"%s\">S 3200</a>", webUrl);

   // prepare

   asprintf(&sensor, "%s/0x%x", type, addr);

   // templating

   subject = strReplace("%sensorid%", sensor, subject);
   subject = strReplace("%value%", value, subject);
   subject = strReplace("%unit%", strcmp(unit, "°") == 0 ? "°C" : unit, subject);
   subject = strReplace("%title%", title, subject);
   subject = strReplace("%min%", (long)min, subject);
   subject = strReplace("%max%", (long)max, subject);
   subject = strReplace("%range%", (long)range, subject);
   subject = strReplace("%delta%", (long)delta, subject);
   subject = strReplace("%time%", l2pTime(time(0)).c_str(), subject);
   subject = strReplace("%repeat%", (long)maxRepeat, subject);
   subject = strReplace("%weburl%", webUrl, subject);

   body = strReplace("%sensorid%", sensor, body);
   body = strReplace("%value%", value, body);
   body = strReplace("%unit%", strcmp(unit, "°") == 0 ? "°C" : unit, body);
   body = strReplace("%title%", title, body);
   body = strReplace("%min%", (long)min, body);
   body = strReplace("%max%", (long)max, body);
   body = strReplace("%range%", (long)range, body);
   body = strReplace("%delta%", (long)delta, body);
   body = strReplace("%time%", l2pTime(time(0)).c_str(), body);
   body = strReplace("%repeat%", (long)maxRepeat, body);
   body = strReplace("%weburl%", htmlMail ? htmlWebUrl : webUrl, body);

   alertMailSubject += string(" ") + subject;
   alertMailBody += string("\n") + body;

   free(sensor);
   free(htmlWebUrl);

   return success;
}

//***************************************************************************
// Send State Mail
//***************************************************************************

int P4d::sendStateMail()
{
   char* webUrl = 0;
   const char* receiver = 0;
   string subject = "";
   string errorBody = "";

   // check

   if (isEmpty(mailScript) || !mailBody.length())
      return fail;

   // get web url ..

   getConfigItem("webUrl", webUrl, "http://to-be-configured");

   // build mail ..

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
      sendMail(receiver, subject.c_str(), mailBody.c_str(), "text/plain");
   }
   else
   {
      char* html = 0;

      loadHtmlHeader();

      asprintf(&html,
               "<html>\n"
               " %s\n"
               "  <body>\n"
               "   <font face=\"Arial\"><br>WEB Interface: <a href=\"%s\">S 3200</a><br></font>\n"
               "   %s\n"
               "   <br>\n"
               "   <table>\n"
               "     <thead>\n"
               "       <tr class=\"head\">\n"
               "         <th><font>Parameter</font></th>\n"
               "         <th><font>Wert</font></th>\n"
               "       </tr>\n"
               "     </thead>\n"
               "     <tbody>\n"
               "%s"
               "     </tbody>\n"
               "   </table>\n"
               "   <br></br>\n"
               "  </body>\n"
               "</html>\n",
               htmlHeader.memory, webUrl, errorBody.c_str(), mailBodyHtml.c_str());

      // send HTML mail

      sendMail(receiver, subject.c_str(), html, "text/html");

      free(html);
   }

   //

   return success;
}

int P4d::sendMail(const char* receiver, const char* subject, const char* body, const char* mimeType)
{
   char* command = 0;

   asprintf(&command, "%s '%s' '%s' '%s' %s", mailScript,
            subject, body, mimeType, receiver);

   system(command);
   free(command);

   tell(eloAlways, "Send mail '%s' with [%s] to '%s'",
        subject, body, receiver);

   return done;
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
// Load Html Header
//***************************************************************************

int P4d::loadHtmlHeader()
{
   char* file;

   // load only once at first call

   if (!htmlHeader.isEmpty())
      return done;

   asprintf(&file, "%s/mail-head.html", confDir);

   if (fileExists(file))
      if (loadFromFile(file, &htmlHeader) == success)
         htmlHeader.append("\0");

   free(file);

   if (!htmlHeader.isEmpty())
      return success;

   htmlHeader.clear();

   htmlHeader.append("  <head>\n"
                     "    <style type=\"text/css\">\n"
                     "      table {"
                     "        border: 1px solid #d2d2d2;\n"
                     "        border-collapse: collapse;\n"
                     "      }\n"
                     "      table tr.head {\n"
                     "        background-color: #004d8f;\n"
                     "        color: #fff;\n"
                     "        font-weight: bold;\n"
                     "        font-family: Helvetica, Arial, sans-serif;\n"
                     "        font-size: 12px;\n"
                     "      }\n"
                     "      table tr th,\n"
                     "      table tr td {\n"
                     "        padding: 4px;\n"
                     "        text-align: left;\n"
                     "      }\n"
                     "      table tr:nth-child(1n) td {\n"
                     "        background-color: #fff;\n"
                     "      }\n"
                     "      table tr:nth-child(2n+2) td {\n"
                     "        background-color: #eee;\n"
                     "      }\n"
                     "      td {\n"
                     "        color: #333;\n"
                     "        font-family: Helvetica, Arial, sans-serif;\n"
                     "        font-size: 12px;\n"
                     "        border: 1px solid #D2D2D2;\n"
                     "      }\n"
                     "      </style>\n"
                     "  </head>"
                     "\0");

   return success;
}

//***************************************************************************
// Stored Parameters
//***************************************************************************

int P4d::getConfigItem(const char* name, char*& value, const char* def)
{
   free(value);
   value = 0;

   tableConfig->clear();
   tableConfig->setValue("OWNER", "p4d");
   tableConfig->setValue("NAME", name);

   if (tableConfig->find())
      value = strdup(tableConfig->getStrValue("VALUE"));
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
   tableConfig->setValue("OWNER", "p4d");
   tableConfig->setValue("NAME", name);
   tableConfig->setValue("VALUE", value);

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
