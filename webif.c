//***************************************************************************
// p4d / Linux - Heizungs Manager
// File webif.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 10.02.2014  Jörg Wendel
//***************************************************************************

#include "p4d.h"

//***************************************************************************
// Perform WEBIF Requests
//***************************************************************************

int P4d::performWebifRequests()
{
   tableJobs->clear();

   for (int f = selectPendingJobs->find(); f; f = selectPendingJobs->fetch())
   {
      int start = time(0);
      int addr = tableJobs->getIntValue("ADDRESS");
      const char* command = tableJobs->getStrValue("COMMAND");
      const char* data = tableJobs->getStrValue("DATA");
      int jobId = tableJobs->getIntValue("ID");

      tableJobs->find();
      tableJobs->setValue("DONEAT", time(0));
      tableJobs->setValue("STATE", "D");

      tell(eloAlways, "Processing WEBIF job %d '%s:0x%04x/%s'",
           jobId, command, addr, data);

      if (strcasecmp(command, "check-login") == 0)
      {
         char* webUser = 0;
         char* webPass = 0;
         md5Buf defaultPwd;

         createMd5("p4-3200", defaultPwd);

         getConfigItem("user", webUser, "p4");
         getConfigItem("passwd", webPass, defaultPwd);

         char* user = strdup(data);
         char* pwd = 0;

         if ((pwd = strchr(user, ':')))
         {
            *pwd = 0; pwd++;

            tell(eloDetail, "%s/%s", pwd, webPass);

            if (strcmp(webUser, user) == 0 && strcmp(pwd, webPass) == 0)
               tableJobs->setValue("RESULT", "success:login-confirmed");
            else
               tableJobs->setValue("RESULT", "fail:login-denied");
         }

         free(webPass);
         free(webUser);
         free(user);
      }

      else if (strcasecmp(command, "update-schemacfg") == 0)
      {
         updateSchemaConfTable();
         tableJobs->setValue("RESULT", "success:done");
      }

      else if (strcasecmp(command, "write-config") == 0)
      {
         char* name = strdup(data);
         char* value = 0;

         if ((value = strchr(name, ':')))
         {
            *value = 0; value++;

            setConfigItem(name, value);

            tableJobs->setValue("RESULT", "success:stored");
         }

         free(name);

         // read the config from table to apply changes

         readConfiguration();
      }

      else if (strcasecmp(command, "read-config") == 0)
      {
         char* name = strdup(data);
         char* buf = 0;
         char* value = 0;
         char* def = 0;

         if ((def = strchr(name, ':')))
         {
            *def = 0;
            def++;
         }

         getConfigItem(name, value, def ? def : "");

         asprintf(&buf, "success:%s", value);
         tableJobs->setValue("RESULT", buf);

         free(name);
         free(buf);
         free(value);
      }

      else if (strcasecmp(command, "getp") == 0)
      {
         tableMenu->clear();
         tableMenu->setValue("ID", addr);

         if (tableMenu->find())
         {
            int type = tableMenu->getIntValue("TYPE");
            unsigned int paddr = tableMenu->getIntValue("ADDRESS");

            ConfigParameter p(paddr);

            if (request->getParameter(&p) == success)
            {
               char* buf = 0;
               cRetBuf value = ConfigParameter::toNice(p.value, type);

               // special for time min/max/default

               if (type == mstParZeit)
                  ;  // #TODO

               asprintf(&buf, "success#%s#%s#%d#%d#%d#%d", *value, type == 0x0a ? "Uhr" : p.unit,
                        p.def, p.min, p.max, p.digits);
               tableJobs->setValue("RESULT", buf);

               free(buf);
            }
         }
      }

      else if (strcasecmp(command, "setp") == 0)
      {
         int status;

         tableMenu->clear();
         tableMenu->setValue("ID", addr);

         if (tableMenu->find())
         {
            int type = tableMenu->getIntValue("TYPE");
            int paddr = tableMenu->getIntValue("ADDRESS");

            ConfigParameter p(paddr);

            // Set Value

            if (ConfigParameter::toValue(data, type, p.value) == success)
            {
               tell(eloAlways, "Storing value '%s/%d' for parameter at address 0x%x", data, p.value, paddr);

               if ((status = request->setParameter(&p)) == success)
               {
                  char* buf = 0;
                  cRetBuf value = ConfigParameter::toNice(p.value, type);

                  // store job result

                  asprintf(&buf, "success#%s#%s#%d#%d#%d#%d", *value, p.unit,
                           p.def, p.min, p.max, p.digits);
                  tableJobs->setValue("RESULT", buf);
                  free(buf);

                  // update menu table

                  tableMenu->setValue("VALUE", value);
                  tableMenu->setValue("UNIT", p.unit);
                  tableMenu->update();
               }
               else
               {
                  tell(eloAlways, "Set of parameter failed, error %d", status);

                  if (status == P4Request::wrnNonUpdate)
                     tableJobs->setValue("RESULT", "fail#no update");
                  else if (status == P4Request::wrnOutOfRange)
                     tableJobs->setValue("RESULT", "fail#out of range");
                  else
                     tableJobs->setValue("RESULT", "fail#communication error");
               }
            }
            else
            {
               tell(eloAlways, "Set of parameter failed, wrong format");
               tableJobs->setValue("RESULT", "fail#format error");
            }
         }
         else
         {
            tell(eloAlways, "Set of parameter failed, id 0x%x not found", addr);
            tableJobs->setValue("RESULT", "fail#id not found");
         }
      }

      else if (strcasecmp(command, "gettrp") == 0)
      {
         // first update the time range data
         //  s3200 support no single request of a time range parameter

         // don't update since it takes to long (assume table is up to date)
         // updateTimeRangeData();

         // now read it from the table

         tableTimeRanges->clear();
         tableTimeRanges->setValue("ADDRESS", addr);

         if (tableTimeRanges->find())
         {
            char* buf = 0;
            char fName[10+TB];
            char tName[10+TB];
            int n = atoi(data);

            sprintf(fName, "FROM%d", n);
            sprintf(tName, "TO%d", n);

            asprintf(&buf, "success#%s#%s#%s",
                     tableTimeRanges->getStrValue(fName),
                     tableTimeRanges->getStrValue(tName),
                     "Zeitraum");
            tableJobs->setValue("RESULT", buf);

            free(buf);
         }
      }

      else if (strcasecmp(command, "settrp") == 0)
      {
         int status = success;
         Fs::TimeRanges t(addr);
         char fName[10+TB];
         char tName[10+TB];
         int rangeNo;
         char valueFrom[100+TB];
         char valueTo[100+TB];

         // parse rangeNo and value from data

         if (sscanf(data, "%d#%[^#]#%[^#]", &rangeNo, valueFrom, valueTo) != 3)
         {
            tell(eloAlways, "Parsing of '%s' failed", data);
            status = fail;
         }

         rangeNo--;

         // get actual values from table

         tableTimeRanges->clear();
         tableTimeRanges->setValue("ADDRESS", addr);

         if (status == success && tableTimeRanges->find())
         {
            for (int n = 0; n < 4; n++)
            {
               sprintf(fName, "FROM%d", n+1);
               sprintf(tName, "TO%d", n+1);

               status += t.setTimeRange(n, tableTimeRanges->getStrValue(fName), tableTimeRanges->getStrValue(tName));
            }

            // override the 'rangeNo' with new value

            status += t.setTimeRange(rangeNo, valueFrom, valueTo);

            if (status == success)
            {
               tell(eloAlways, "Storing '%s' for time range '%d' of parameter 0x%x", t.getTimeRange(rangeNo), rangeNo+1, t.address);

               if ((status = request->setTimeRanges(&t)) == success)
               {
                  char* buf = 0;

                  // store job result

                  asprintf(&buf, "success#%s#%s#%s", t.getTimeRangeFrom(rangeNo), t.getTimeRangeTo(rangeNo), "Zeitbereich");
                  tableJobs->setValue("RESULT", buf);
                  free(buf);

                  // update time range table

                  sprintf(fName, "FROM%d", rangeNo+1);
                  sprintf(tName, "TO%d", rangeNo+1);
                  tableTimeRanges->setValue(fName, t.getTimeRangeFrom(rangeNo));
                  tableTimeRanges->setValue(tName, t.getTimeRangeTo(rangeNo));
                  tableTimeRanges->update();
               }
               else
               {
                  tell(eloAlways, "Set of time range parameter failed, error %d", status);

                  if (status == P4Request::wrnNonUpdate)
                     tableJobs->setValue("RESULT", "fail#no update");
                  else if (status == P4Request::wrnOutOfRange)
                     tableJobs->setValue("RESULT", "fail#out of range");
                  else
                     tableJobs->setValue("RESULT", "fail#communication error");
               }
            }
            else
            {
               tell(eloAlways, "Set of time range parameter failed, wrong format");
               tableJobs->setValue("RESULT", "fail#format error");
            }
         }
         else
         {
            tell(eloAlways, "Set of time range parameter failed, addr 0x%x for '%s' not found", addr, data);
            tableJobs->setValue("RESULT", "fail#id not found");
         }
      }

      else if (strcasecmp(command, "getv") == 0)
      {
         Value v(addr);

         tableValueFacts->clear();
         tableValueFacts->setValue("TYPE", "VA");
         tableValueFacts->setValue("ADDRESS", addr);

         if (tableValueFacts->find())
         {
            double factor = tableValueFacts->getIntValue("FACTOR");
            const char* unit = tableValueFacts->getStrValue("UNIT");

            if (request->getValue(&v) == success)
            {
               char* buf = 0;

               asprintf(&buf, "success:%.2f%s", v.value / factor, unit);
               tableJobs->setValue("RESULT", buf);
               free(buf);
            }
         }
      }

      else if (strcasecmp(command, "initmenu") == 0)
      {
         initMenu();
         tableJobs->setValue("RESULT", "success:done");
      }

      else if (strcasecmp(command, "updatehm") == 0)
      {
         if (hmSyncSysVars() == success)
            tableJobs->setValue("RESULT", "success:done");
         else
            tableJobs->setValue("RESULT", "fail:error");
      }

      else if (strcasecmp(command, "p4d-state") == 0)
      {
         struct tm tim = {0};

         double averages[3];
         char dt[10];
         char d[100];
         char* buf;

         memset(averages, 0, sizeof(averages));
         localtime_r(&nextAt, &tim);
         strftime(dt, 10, "%H:%M:%S", &tim);
         toElapsed(time(0)-startedAt, d);

         getloadavg(averages, 3);

         asprintf(&buf, "success:%s#%s#%s#%3.2f %3.2f %3.2f",
                  dt, VERSION, d, averages[0], averages[1], averages[2]);

         tableJobs->setValue("RESULT", buf);
         free(buf);
      }

      else if (strcasecmp(command, "s3200-state") == 0)
      {
         struct tm tim = {0};
         char date[100];
         char* buf = 0;

         localtime_r(&currentState.time, &tim);
         strftime(date, 100, "%A, %d. %b. %G %H:%M:%S", &tim);

         asprintf(&buf, "success:%s#%d#%s#%s", date,
                  currentState.state, currentState.stateinfo,
                  currentState.modeinfo);

         tableJobs->setValue("RESULT", buf);
         free(buf);
      }

      else if (strcasecmp(command, "initvaluefacts") == 0)
      {
         updateValueFacts();
         tableJobs->setValue("RESULT", "success:done");
      }

      else if (strcasecmp(command, "updatemenu") == 0)
      {
         tableMenu->clear();

         for (int f = selectAllMenuItems->find(); f; f = selectAllMenuItems->fetch())
         {
            int type = tableMenu->getIntValue("TYPE");
            int paddr = tableMenu->getIntValue("ADDRESS");

            if (type == 0x07 || type == 0x08 || type == 0x0a ||
                type == 0x40 || type == 0x39 || type == 0x32)
            {
               Fs::ConfigParameter p(paddr);

               if (request->getParameter(&p) == success)
               {
                  cRetBuf value = ConfigParameter::toNice(p.value, type);

                  if (tableMenu->find())
                  {
                     tableMenu->setValue("VALUE", value);
                     tableMenu->setValue("UNIT", p.unit);
                     tableMenu->update();
                  }
               }
            }

            else if (type == mstFirmware)
            {
               Fs::Status s;

               if (request->getStatus(&s) == success)
               {
                  if (tableMenu->find())
                  {
                     tableMenu->setValue("VALUE", s.version);
                     tableMenu->setValue("UNIT", "");
                     tableMenu->update();
                  }
               }
            }

            else if (type == mstDigOut || type == mstDigIn || type == mstAnlOut)
            {
               int status;
               Fs::IoValue v(paddr);

               if (type == mstDigOut)
                  status = request->getDigitalOut(&v);
               else if (type == mstDigIn)
                  status = request->getDigitalIn(&v);
               else
                  status = request->getAnalogOut(&v);

               if (status == success)
               {
                  char* buf = 0;

                  if (type == mstAnlOut)
                  {
                     if (v.mode == 0xff)
                        asprintf(&buf, "%d (A)", v.state);
                     else
                        asprintf(&buf, "%d (%d)", v.state, v.mode);
                  }
                  else
                     asprintf(&buf, "%s (%c)", v.state ? "on" : "off", v.mode);

                  if (tableMenu->find())
                  {
                     tableMenu->setValue("VALUE", buf);
                     tableMenu->setValue("UNIT", "");
                     tableMenu->update();
                  }

                  free(buf);
               }
            }

            else if (type == mstMesswert || type == mstMesswert1)
            {
               int status;
               Fs::Value v(paddr);

               tableValueFacts->clear();
               tableValueFacts->setValue("TYPE", "VA");
               tableValueFacts->setValue("ADDRESS", paddr);

               if (tableValueFacts->find())
               {
                  double factor = tableValueFacts->getIntValue("FACTOR");
                  const char* unit = tableValueFacts->getStrValue("UNIT");

                  status = request->getValue(&v);

                  if (status == success)
                  {
                     char* buf = 0;
                     asprintf(&buf, "%.2f", v.value / factor);

                     if (tableMenu->find())
                     {
                        tableMenu->setValue("VALUE", buf);

                        if (strcmp(unit, "°") == 0)
                           tableMenu->setValue("UNIT", "°C");
                        else
                           tableMenu->setValue("UNIT", unit);

                        tableMenu->update();
                     }

                     free(buf);
                  }
               }
            }
         }

         selectAllMenuItems->freeResult();

         updateTimeRangeData();

         tableJobs->setValue("RESULT", "success:done");
      }

      else
      {
         tell(eloAlways, "Warning: Ignoring unknown job '%s'", command);
         tableJobs->setValue("RESULT", "fail:unknown command");
      }

      tableJobs->store();

      tell(eloAlways, "Processing WEBIF job %d done with '%s' after %ld seconds",
           jobId, tableJobs->getStrValue("RESULT"),
           time(0) - start);
   }

   selectPendingJobs->freeResult();

   return success;
}

//***************************************************************************
// Update Time Range Data
//***************************************************************************

int P4d::updateTimeRangeData()
{
   Fs::TimeRanges t;
   int status;
   char fName[10+TB];
   char tName[10+TB];

   // update / insert time ranges

   for (status = request->getFirstTimeRanges(&t); status != Fs::wrnLast; status = request->getNextTimeRanges(&t))
   {
      tableTimeRanges->clear();
      tableTimeRanges->setValue("ADDRESS", t.address);

      for (int n = 0; n < 4; n++)
      {
         sprintf(fName, "FROM%d", n+1);
         sprintf(tName, "TO%d", n+1);
         tableTimeRanges->setValue(fName, t.getTimeRangeFrom(n));
         tableTimeRanges->setValue(tName, t.getTimeRangeTo(n));
      }

      tableTimeRanges->store();
      tableTimeRanges->reset();
   }

   return done;
}

//***************************************************************************
// Cleanup WEBIF Requests
//***************************************************************************

int P4d::cleanupWebifRequests()
{
   int status;

   // delete jobs older than 2 days

   tell(eloAlways, "Cleanup jobs table with history of 2 days");

   tableJobs->clear();
   tableJobs->setValue("REQAT", time(0) - 2*tmeSecondsPerDay);
   status = cleanupJobs->execute();

   return status;
}
