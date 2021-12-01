//***************************************************************************
// Automation Control
// File wsactions.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 16.04.2020 - Jörg Wendel
//***************************************************************************

#include <dirent.h>
#include <algorithm>
#include <cmath>

#include "lib/json.h"
#include "daemon.h"

//***************************************************************************
// Dispatch Client Requests
//***************************************************************************

int Daemon::dispatchClientRequest()
{
   int status {fail};
   json_error_t error;
   json_t *oData, *oObject;

   cMyMutexLock lock(&messagesInMutex);

   while (!messagesIn.empty())
   {
      // dispatch message like
      //   => { "event" : "toggleio", "object" : { "address" : "122", "type" : "DO" } }

      tell(2, "<= '%s'", messagesIn.front().c_str());
      oData = json_loads(messagesIn.front().c_str(), 0, &error);

      // get the request

      Event event = cWebService::toEvent(getStringFromJson(oData, "event", "<null>"));
      long client = getLongFromJson(oData, "client");
      oObject = json_object_get(oData, "object");
      int addr = getIntFromJson(oObject, "address");
      const char* type = getStringFromJson(oObject, "type");

      // rights ...

      if (checkRights(client, event, oObject))
      {
         // dispatch client request

         switch (event)
         {
            case evLogin:           status = performLogin(oObject);                  break;
            case evLogout:          status = performLogout(oObject);                 break;
            case evInit:
            case evPageChange:      status = performPageChange(oObject, client);     break;
            case evData:            status = update(true, client);                   break;
            case evGetToken:        status = performTokenRequest(oObject, client);   break;
            case evToggleIo:        status = toggleIo(addr, type);                   break;
            case evToggleIoNext:    status = toggleIoNext(addr);                     break;
            case evToggleMode:      status = toggleOutputMode(addr);                 break;
            case evStoreConfig:     status = storeConfig(oObject, client);           break;
            case evSetup:           status = performConfigDetails(client);           break;

            case evStoreIoSetup:    status = storeIoSetup(oObject, client);          break;
            case evChartData:       status = performChartData(oObject, client);      break;
            case evUserDetails:     status = performUserDetails(client);             break;
            case evStoreUserConfig: status = storeUserConfig(oObject, client);       break;
            case evChangePasswd:    status = performPasswChange(oObject, client);    break;
            case evGroups:          status = performGroups(client);                  break;
            case evGroupConfig:     status = storeGroups(oObject, client);           break;
            case evList:            status = update(true, client);                   break;

            case evReset:           status = performReset(oObject, client);          break;
            case evSendMail:        status = performSendMail(oObject, client);       break;
            case evSyslog:          status = performSyslog(oObject, client);         break;
            case evForceRefresh:    status = update(true, client);                   break;
            case evChartbookmarks:  status = performChartbookmarks(client);          break;
            case evStoreChartbookmarks: status = storeChartbookmarks(oObject, client);     break;
            default:
            {
               if (dispatchSpecialRequest(event,oObject, client) == ignore)
                  tell(0, "Error: Received unexpected client request '%s' at [%s]",
                       cWebService::toName(event), messagesIn.front().c_str());
            }
         }
      }
      else
      {
         if (client == na)
            tell(0, "Insufficient rights to '%s', missing login", getStringFromJson(oData, "event", "<null>"));
         else
         tell(0, "Insufficient rights to '%s' for user '%s'", getStringFromJson(oData, "event", "<null>"),
              wsClients[(void*)client].user.c_str());
         replyResult(fail, "Insufficient rights", client);
      }

      json_decref(oData);      // free the json object
      messagesIn.pop();
   }

   return status;
}

bool Daemon::checkRights(long client, Event event, json_t* oObject)
{
   uint rights = urView;
   auto it = wsClients.find((void*)client);

   if (it != wsClients.end())
      rights = wsClients[(void*)client].rights;

   switch (event)
   {
      case evLogin:               return true;
      case evLogout:              return true;
      case evGetToken:            return true;
      case evData:                return true;
      case evInit:                return true;
      case evPageChange:          return true;
      case evToggleIoNext:        return rights & urControl;
      case evToggleMode:          return rights & urFullControl;
      case evStoreConfig:         return rights & urSettings;
      case evSetup:               return rights & urView;
      case evStoreIoSetup:        return rights & urSettings;
      case evGroupConfig:         return rights & urSettings;
      case evChartData:           return rights & urView;
      case evStoreUserConfig:     return rights & urAdmin;
      case evUserDetails:         return rights & urAdmin;
      case evChangePasswd:        return true;   // check will done in performPasswChange()
      case evReset:               return rights & urFullControl;
      case evSendMail:            return rights & urSettings;
      case evSyslog:              return rights & urAdmin;
      case evForceRefresh:        return true;

      case evChartbookmarks:      return rights & urView;
      case evStoreChartbookmarks: return rights & urSettings;

      case evMenu:                return rights & urView;
      case evAlerts:              return rights & urSettings;
      case evStoreAlerts:         return rights & urSettings;
      case evGroups:              return rights & urSettings;
      case evList:                return rights & urView;
      case evSchema:              return rights & urView;
      case evStoreSchema:         return rights & urSettings;
      case evParEditRequest:      return rights & urFullControl;
      case evParStore:            return rights & urFullControl;

      case evInitTables:          return rights & urSettings;
      case evUpdateTimeRanges:    return rights & urFullControl;
      case evPellets:             return rights & urControl;
      case evPelletsAdd:          return rights & urFullControl;
      case evErrors:              return rights & urView;

      default: break;
   }

   if (event == evToggleIo && rights & urControl)
   {
      int addr = getIntFromJson(oObject, "address");
      const char* type = getStringFromJson(oObject, "type");

      tableValueFacts->clear();
      tableValueFacts->setValue("ADDRESS", addr);
      tableValueFacts->setValue("TYPE", type);

      if (tableValueFacts->find())
      {
         tableValueFacts->reset();
         return rights & tableValueFacts->getIntValue("RIGHTS");
      }
   }

   return false;
}

//***************************************************************************
// WS Ping
//***************************************************************************

int Daemon::performWebSocketPing()
{
   if (nextWebSocketPing < time(0))
   {
      webSock->performData(cWebSock::mtPing);
      nextWebSocketPing = time(0) + webSocketPingTime-5;
   }

   return done;
}

//***************************************************************************
// Reply Result
//***************************************************************************

int Daemon::replyResult(int status, const char* message, long client)
{
   if (status != success)
      tell(0, "Error: Web request failed with '%s' (%d)", message, status);

   json_t* oJson = json_object();
   json_object_set_new(oJson, "status", json_integer(status));
   json_object_set_new(oJson, "message", json_string(message));
   pushOutMessage(oJson, "result", client);

   return status;
}

//***************************************************************************
// Reply Result - the new version!!!
//***************************************************************************
/*
int Daemon::replyResult(int status, long client, const char* format, ...)
{
   if (!client)
      return done;

   char* message {nullptr};
   va_list ap;

   va_start(ap, format);
   vasprintf(&message, format, ap);
   va_end(ap);

   if (status != success)
      tell(0, "Error: Requested web action failed with '%s' (%d)", message, status);

   json_t* oJson = json_object();
   json_object_set_new(oJson, "status", json_integer(status));
   json_object_set_new(oJson, "message", json_string(message));
   pushOutObject(job, oJson, "result", client);

   free(message);

   return status;
}
*/
//***************************************************************************
// Perform WS Client Login / Logout
//***************************************************************************

int Daemon::performLogin(json_t* oObject)
{
   long client = getLongFromJson(oObject, "client");
   const char* user = getStringFromJson(oObject, "user", "");
   const char* token = getStringFromJson(oObject, "token", "");
   const char* page = getStringFromJson(oObject, "page", "");

   tableUsers->clear();
   tableUsers->setValue("USER", user);

   wsClients[(void*)client].user = user;
   wsClients[(void*)client].page = page;

   // tell(eloAlways, "Now %zu clients in list", wsClients.size());

   if (tableUsers->find() && tableUsers->hasValue("TOKEN", token))
   {
      wsClients[(void*)client].type = ctWithLogin;
      wsClients[(void*)client].rights = tableUsers->getIntValue("RIGHTS");
   }
   else
   {
      wsClients[(void*)client].type = ctActive;
      wsClients[(void*)client].rights = urView;  // allow view without login
      tell(0, "Warning: Unknown user '%s' or token mismatch connected!", user);

      json_t* oJson = json_object();
      json_object_set_new(oJson, "user", json_string(user));
      json_object_set_new(oJson, "state", json_string("reject"));
      json_object_set_new(oJson, "value", json_string(""));
      pushOutMessage(oJson, "token", client);
   }

   tell(0, "Login of client 0x%x; user '%s'; type is %d; page %s", (unsigned int)client, user,
        wsClients[(void*)client].type, wsClients[(void*)client].page.c_str());
   webSock->setClientType((lws*)client, wsClients[(void*)client].type);

   //

   json_t* oJson = json_object();
   config2Json(oJson);
   pushOutMessage(oJson, "config", client);

   oJson = json_object();
   widgetTypes2Json(oJson);
   pushOutMessage(oJson, "widgettypes", client);

   oJson = json_object();
   daemonState2Json(oJson);
   pushOutMessage(oJson, "daemonstate", client);

   oJson = json_object();
   valueFacts2Json(oJson, false);
   pushOutMessage(oJson, "valuefacts", client);

   oJson = json_array();
   images2Json(oJson);
   pushOutMessage(oJson, "images", client);

   oJson = json_object();
   s3200State2Json(oJson);
   pushOutMessage(oJson, "s3200-state", client);

   oJson = json_array();
   groups2Json(oJson);
   pushOutMessage(oJson, "grouplist", client);

   update(true, client);

   return done;
}

//***************************************************************************
// Perform Page Change
//***************************************************************************

int Daemon::performPageChange(json_t* oObject, long client)
{
   const char* page = getStringFromJson(oObject, "page", "");

   wsClients[(void*)client].page = page;

   return done;
}

int Daemon::performLogout(json_t* oObject)
{
   long client = getLongFromJson(oObject, "client");
   tell(0, "Logout of client 0x%x", (unsigned int)client);
   wsClients.erase((void*)client);
   return done;
}

//***************************************************************************
// Perform WS Token Request
//***************************************************************************

int Daemon::performTokenRequest(json_t* oObject, long client)
{
   json_t* oJson = json_object();
   const char* user = getStringFromJson(oObject, "user", "");
   const char* passwd  = getStringFromJson(oObject, "password", "");

   tell(0, "Token request of client 0x%x for user '%s'", (unsigned int)client, user);

   tableUsers->clear();
   tableUsers->setValue("USER", user);

   if (tableUsers->find())
   {
      if (tableUsers->hasValue("PASSWD", passwd))
      {
         tell(0, "Token request for user '%s' succeeded", user);
         json_object_set_new(oJson, "user", json_string(user));
         json_object_set_new(oJson, "rights", json_integer(tableUsers->getIntValue("RIGHTS")));
         json_object_set_new(oJson, "state", json_string("confirm"));
         json_object_set_new(oJson, "value", json_string(tableUsers->getStrValue("TOKEN")));
         pushOutMessage(oJson, "token", client);
      }
      else
      {
         tell(0, "Token request for user '%s' failed, wrong password", user);
         json_object_set_new(oJson, "user", json_string(user));
         json_object_set_new(oJson, "rights", json_integer(0));
         json_object_set_new(oJson, "state", json_string("reject"));
         json_object_set_new(oJson, "value", json_string(""));
         pushOutMessage(oJson, "token", client);
      }
   }
   else
   {
      tell(0, "Token request for user '%s' failed, unknown user", user);
      json_object_set_new(oJson, "user", json_string(user));
      json_object_set_new(oJson, "rights", json_integer(0));
      json_object_set_new(oJson, "state", json_string("reject"));
      json_object_set_new(oJson, "value", json_string(""));

      pushOutMessage(oJson, "token", client);
   }

   tableUsers->reset();

   return done;
}

//***************************************************************************
// Perform Syslog Request
//***************************************************************************

int Daemon::performSyslog(json_t* oObject, long client)
{
   const char* name {"/var/log/" TARGET ".log"};

   if (client == 0)
      return done;

   const char* log = getStringFromJson(oObject, "log");
   json_t* oJson = json_object();
   std::vector<std::string> lines;
   std::string result;

   if (!isEmpty(log))
      name = log;

   if (loadLinesFromFile(name, lines, false) == success)
   {
      const int maxLines {150};
      int count {0};

      for (auto it = lines.rbegin(); it != lines.rend(); ++it)
      {
         if (count++ >= maxLines)
         {
            result += "...\n...\n";
            break;
         }

         result += *it;
      }
   }

   json_object_set_new(oJson, "lines", json_string(result.c_str()));
   pushOutMessage(oJson, "syslog", client);

   return done;
}

//***************************************************************************
// Perform Config Data Request
//***************************************************************************

int Daemon::performConfigDetails(long client)
{
   if (client == 0)
      return done;

   json_t* oJson = json_array();
   configDetails2Json(oJson);
   pushOutMessage(oJson, "configdetails", client);

   return done;
}

//***************************************************************************
// Perform WS User Data Request
//***************************************************************************

int Daemon::performUserDetails(long client)
{
   if (client == 0)
      return done;

   json_t* oJson = json_array();
   userDetails2Json(oJson);
   pushOutMessage(oJson, "userdetails", client);

   return done;
}

//***************************************************************************
// Perform WS Groups Data Request
//***************************************************************************

int Daemon::performGroups(long client)
{
   if (client == 0)
      return done;

   json_t* oJson = json_array();
   groups2Json(oJson);
   pushOutMessage(oJson, "groups", client);

   return done;
}

//***************************************************************************
// Perform Send Mail
//***************************************************************************

int Daemon::performSendMail(json_t* oObject, long client)
{
   int alertid = getIntFromJson(oObject, "alertid", na);

   if (alertid != na)
      return performAlertTestMail(alertid, client);

   const char* subject = getStringFromJson(oObject, "subject");
   const char* body = getStringFromJson(oObject, "body");

   tell(eloDetail, "Test mail requested with: '%s/%s'", subject, body);

   if (isEmpty(mailScript))
      return replyResult(fail, "Missing mail script", client);

   if (!fileExists(mailScript))
   {
      char* buf {nullptr};
      asprintf(&buf, "Mail script '%s' not found", mailScript);
      replyResult(fail, buf, client);
      delete buf;
      return fail;
   }

   if (isEmpty(stateMailTo))
      return replyResult(fail, "Missing receiver", client);

   if (sendMail(stateMailTo, subject, body, "text/plain") != success)
   {
      const char* message = "Sending mail failed\n"
         "Check your '/etc/msmtprc' and configure your mail account.\n\n"
         " For example 'gmx':\n"
         "defaults\n"
         "auth           on\n"
         "tls            on\n"
         "tls_trust_file /etc/ssl/certs/ca-certificates.crt\n"
         "logfile        /var/log/msmtp.log\n"
         "\n"
         "account        myaccount\n"
         "host           mail.gmx.net\n"
         "port           587\n"
         "\n"
         "from           you@gmx.de\n"
         "user           your-user@gmx.de\n"
         "password       your-passwd\n"
         "\n"
         "# Default\n"
         "account default : myaccount\n";

      return replyResult(fail, message, client);
   }

   return replyResult(success, "mail sended", client);
}

//***************************************************************************
// Perform WS ChartData request
//***************************************************************************

int Daemon::performChartData(json_t* oObject, long client)
{
   if (client == 0)
      return done;

   double range = getDoubleFromJson(oObject, "range", 1);          // Anzahl der Tage
   time_t rangeStart = getLongFromJson(oObject, "start", 0);       // Start Datum (unix timestamp)
   const char* sensors = getStringFromJson(oObject, "sensors");    // Kommata getrennte Liste der Sensoren
   const char* id = getStringFromJson(oObject, "id", "");

   // the id is one of {"chart" "chartwidget" "chartdialog"}

   bool widget = strcmp(id, "chart") != 0;
   cDbStatement* select = widget ? selectSamplesRange60 : selectSamplesRange;

   if (!widget)
      performChartbookmarks(client);

   if (strcmp(id, "chart") == 0 && !isEmpty(sensors))
   {
      tell(2, "storing sensors '%s' for chart", sensors);
      setConfigItem("chart", sensors);
      getConfigItem("chart", chartSensors);
   }
   else if (isEmpty(sensors))
      sensors = chartSensors;

   tell(eloInfo, "Selecting chart data for sensors '%s' with range %.1f ..", sensors, range);

   std::vector<std::string> sList;

   if (sensors)
      sList = split(sensors, ',');

   json_t* oMain = json_object();
   json_t* oJson = json_array();

   if (!rangeStart)
      rangeStart = time(0) - (range*tmeSecondsPerDay);

   rangeFrom.setValue(rangeStart);
   rangeTo.setValue(rangeStart + (int)(range*tmeSecondsPerDay));

   tableValueFacts->clear();
   tableValueFacts->setValue("STATE", "A");

   json_t* aAvailableSensors {nullptr};

   if (!widget)
      aAvailableSensors = json_array();

   for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
   {
      char* id {nullptr};
      asprintf(&id, "%s:0x%lx", tableValueFacts->getStrValue("TYPE"), tableValueFacts->getIntValue("ADDRESS"));

      bool active = std::find(sList.begin(), sList.end(), id) != sList.end();  // #PORT
      const char* usrtitle = tableValueFacts->getStrValue("USRTITLE");
      const char* title = tableValueFacts->getStrValue("TITLE");

      if (!isEmpty(usrtitle))
         title = usrtitle;

      if (!widget)
      {
         json_t* oSensor = json_object();
         json_object_set_new(oSensor, "id", json_string(id));
         json_object_set_new(oSensor, "title", json_string(title));
         json_object_set_new(oSensor, "active", json_integer(active));
         json_array_append_new(aAvailableSensors, oSensor);
      }

      free(id);

      if (!active)
         continue;

      json_t* oSample = json_object();
      json_array_append_new(oJson, oSample);

      char* sensor {nullptr};
      asprintf(&sensor, "%s%lu", tableValueFacts->getStrValue("TYPE"), tableValueFacts->getIntValue("ADDRESS"));
      json_object_set_new(oSample, "title", json_string(title));
      json_object_set_new(oSample, "sensor", json_string(sensor));
      json_t* oData = json_array();
      json_object_set_new(oSample, "data", oData);
      free(sensor);

      tableSamples->clear();
      tableSamples->setValue("TYPE", tableValueFacts->getStrValue("TYPE"));
      tableSamples->setValue("ADDRESS", tableValueFacts->getIntValue("ADDRESS"));

      tell(eloDebug, " selecting '%s - %s' for '%s:0x%02lx'",
           l2pTime(rangeFrom.getTimeValue()).c_str(),
           l2pTime(rangeTo.getTimeValue()).c_str(),
           tableValueFacts->getStrValue("TYPE"), tableValueFacts->getIntValue("ADDRESS"));

      uint count {0};

      for (int f = select->find(); f; f = select->fetch())
      {
         // tell(eloAlways, "0x%x: '%s' : %0.2f", (uint)tableSamples->getStrValue("ADDRESS"),
         //      xmlTime.getStrValue(), tableSamples->getFloatValue("VALUE"));

         json_t* oRow = json_object();
         json_array_append_new(oData, oRow);

         json_object_set_new(oRow, "x", json_string(xmlTime.getStrValue()));

         if (tableValueFacts->hasValue("TYPE", "DO"))
            json_object_set_new(oRow, "y", json_integer(maxValue.getIntValue()*10));
         else
         {
            // double avg = avgValue.getFloatValue();
            // avg = std::llround(avg*20) / 20.0;                  // round to .05
            json_object_set_new(oRow, "y", json_real(avgValue.getFloatValue()));
         }
         count++;
      }

      tell(eloDebug, " collected %d samples'", count);
      select->freeResult();
   }

   if (!widget)
      json_object_set_new(oMain, "sensors", aAvailableSensors);

   json_object_set_new(oMain, "rows", oJson);
   json_object_set_new(oMain, "id", json_string(id));
   selectActiveValueFacts->freeResult();
   tell(eloDebug, ".. done");
   pushOutMessage(oMain, "chartdata", client);

   return done;
}

//***************************************************************************
// Chart Bookmarks
//***************************************************************************

int Daemon::storeChartbookmarks(json_t* array, long client)
{
   char* bookmarks = json_dumps(array, JSON_REAL_PRECISION(4));
   setConfigItem("chartBookmarks", bookmarks);
   free(bookmarks);

   performChartbookmarks(client);

   return done; // replyResult(success, "Bookmarks gespeichert", client);
}

int Daemon::performChartbookmarks(long client)
{
   char* bookmarks {nullptr};
   getConfigItem("chartBookmarks", bookmarks, "{[]}");
   json_error_t error;
   json_t* oJson = json_loads(bookmarks, 0, &error);
   pushOutMessage(oJson, "chartbookmarks", client);
   free(bookmarks);

   return done;
}

//***************************************************************************
// Store User Configuration
//***************************************************************************

int Daemon::storeUserConfig(json_t* oObject, long client)
{
   if (client == 0)
      return done;

   int rights = getIntFromJson(oObject, "rights", na);
   const char* user = getStringFromJson(oObject, "user");
   const char* passwd = getStringFromJson(oObject, "passwd");
   const char* action = getStringFromJson(oObject, "action");

   tableUsers->clear();
   tableUsers->setValue("USER", user);
   int exists = tableUsers->find();

   if (strcmp(action, "add") == 0)
   {
      if (exists)
         tell(0, "User alredy exists, ignoring 'add' request");
      else
      {
         char* token {nullptr};
         asprintf(&token, "%s_%s_%s", getUniqueId(), l2pTime(time(0)).c_str(), user);
         tell(0, "Add user '%s' with token [%s]", user, token);
         tableUsers->setValue("PASSWD", passwd);
         tableUsers->setValue("TOKEN", token);
         tableUsers->setValue("RIGHTS", urView);
         tableUsers->store();
         free(token);
      }
   }
   else if (strcmp(action, "store") == 0)
   {
      if (!exists)
         tell(0, "User not exists, ignoring 'store' request");
      else
      {
         tell(0, "Store settings for user '%s'", user);
         tableUsers->setValue("RIGHTS", rights);
         tableUsers->store();
      }
   }
   else if (strcmp(action, "del") == 0)
   {
      if (!exists)
         tell(0, "User not exists, ignoring 'del' request");
      else
      {
         tell(0, "Delete user '%s'", user);
         tableUsers->deleteWhere(" user = '%s'", user);
      }
   }
   else if (strcmp(action, "resetpwd") == 0)
   {
      if (!exists)
         replyResult(fail, "User not exists, ignoring 'resetpwd' request", client);
      else
      {
         tell(0, "Reset password of user '%s'", user);
         tableUsers->setValue("PASSWD", passwd);
         tableUsers->store();
         replyResult(success, "Passwort gespeichert", client);
      }
   }
   else if (strcmp(action, "resettoken") == 0)
   {
      if (!exists)
         tell(0, "User not exists, ignoring 'resettoken' request");
      else
      {
         char* token {nullptr};
         asprintf(&token, "%s_%s_%s", getUniqueId(), l2pTime(time(0)).c_str(), user);
         tell(0, "Reset token of user '%s' to '%s'", user, token);
         tableUsers->setValue("TOKEN", token);
         tableUsers->store();
         free(token);
      }
   }

   tableUsers->reset();

   json_t* oJson = json_array();
   userDetails2Json(oJson);
   pushOutMessage(oJson, "userdetails", client);

   return done;
}

//***************************************************************************
// Perform password Change
//***************************************************************************

int Daemon::performPasswChange(json_t* oObject, long client)
{
   if (client == 0)
      return done;

   const char* user = getStringFromJson(oObject, "user");
   const char* passwd = getStringFromJson(oObject, "passwd");

   if (strcmp(wsClients[(void*)client].user.c_str(), user) != 0)
   {
      tell(0, "Warning: User '%s' tried to change password of '%s'",
           wsClients[(void*)client].user.c_str(), user);
      return done;
   }

   tableUsers->clear();
   tableUsers->setValue("USER", user);

   if (tableUsers->find())
   {
      tell(0, "User '%s' changed password", user);
      tableUsers->setValue("PASSWD", passwd);
      tableUsers->store();
      replyResult(success, "Passwort gespeichert", client);
   }

   tableUsers->reset();

   return done;
}

//***************************************************************************
// Perform Reset
//***************************************************************************

int Daemon::performReset(json_t* obj, long client)
{
   std::string what = getStringFromJson(obj, "what");

   if (what == "peaks")
   {
      tablePeaks->truncate();
      setConfigItem("peakResetAt", l2pTime(time(0)).c_str());

      json_t* oJson = json_object();
      config2Json(oJson);
      pushOutMessage(oJson, "config", client);
   }

   return replyResult(success, "Peaks zurückgesetzt", client);
}

//***************************************************************************
// Store Configuration
//***************************************************************************

int Daemon::storeConfig(json_t* obj, long client)
{
   const char* key {nullptr};
   json_t* jValue {nullptr};
   int oldWebPort = webPort;
   char* oldStyle {nullptr};
   int count {0};

   getConfigItem("style", oldStyle, "");

   json_object_foreach(obj, key, jValue)
   {
      tell(3, "Debug: Storing config item '%s' with '%s'", key, json_string_value(jValue));
      setConfigItem(key, json_string_value(jValue));
      count++;
   }

   // create link for the stylesheet

   const char* name = getStringFromJson(obj, "style");

   if (!isEmpty(name) && strcmp(name, oldStyle) != 0)
   {
      tell(1, "Info: Creating link 'stylesheet.css' to '%s'", name);
      char* link {nullptr};
      char* target {nullptr};
      asprintf(&link, "%s/stylesheet.css", httpPath);
      asprintf(&target, "%s/stylesheet-%s.css", httpPath, name);
      createLink(link, target, true);
      free(link);
      free(target);
   }

   // reload configuration

   readConfiguration(false);

   json_t* oJson = json_object();
   config2Json(oJson);
   pushOutMessage(oJson, "config", client);

   if (oldWebPort != webPort)
      replyResult(success, "Konfiguration gespeichert. Web Port geändert, bitte " TARGET " neu Starten!", client);
   else if (!isEmpty(name) && strcmp(name, oldStyle) != 0)
      replyResult(success, "Konfiguration gespeichert. Das Farbschema wurde geändert, mit STRG-Umschalt-r neu laden!", client);
   else if (count > 1)  // on drag&drop its only one parameter
      replyResult(success, "Konfiguration gespeichert", client);

   free(oldStyle);

   return done;
}

//***************************************************************************
// Store IO Setup
//***************************************************************************

int Daemon::storeIoSetup(json_t* array, long client)
{
   size_t index {0};
   json_t* jObj {nullptr};

   json_array_foreach(array, index, jObj)
   {
      int addr = getIntFromJson(jObj, "address");
      const char* type = getStringFromJson(jObj, "type");
      bool state = getBoolFromJson(jObj, "state");

      tableValueFacts->clear();
      tableValueFacts->setValue("ADDRESS", addr);
      tableValueFacts->setValue("TYPE", type);

      if (!tableValueFacts->find())
         continue;

      tableValueFacts->clearChanged();

      if (isElementSet(jObj, "state"))
         tableValueFacts->setValue("STATE", state ? "A" : "D");
      if (isElementSet(jObj, "usrtitle"))
         tableValueFacts->setValue("USRTITLE", getStringFromJson(jObj, "usrtitle"));
      if (isElementSet(jObj, "unit"))
         tableValueFacts->setValue("UNIT", getStringFromJson(jObj, "unit"));

      if (isElementSet(jObj, "widget"))
      {
         json_t* oWidgetOptions = getObjectFromJson(jObj, "widget");

         if (oWidgetOptions)
         {
            char* widgetOptions = json_dumps(oWidgetOptions, JSON_REAL_PRECISION(4));
            tableValueFacts->setValue("WIDGETOPT", widgetOptions);
         }
      }

      if (tableValueFacts->getChanges())
      {
         tableValueFacts->store();
         tell(2, "STORED valuefact for %s:%d", type, addr);
      }
   }

   json_t* oJson = json_object();
   valueFacts2Json(oJson, false);
   pushOutMessage(oJson, "valuefacts", client);

   return replyResult(success, "Konfiguration gespeichert", client);
}

//***************************************************************************
// Store Groups
//***************************************************************************

int Daemon::storeGroups(json_t* oObject, long client)
{
   const char* action = getStringFromJson(oObject, "action");

   if (strcmp(action, "store") == 0)
   {
      json_t* array = json_object_get(oObject, "groups");
      json_t* jObj {nullptr};
      size_t index {0};

      if (!array)
         return fail;

      json_array_foreach(array, index, jObj)
      {
         int id = getIntFromJson(jObj, "id");
         const char* name = getStringFromJson(jObj, "name", "");

         tableGroups->clear();
         tableGroups->setValue("ID", id);

         if (!tableGroups->find())
         {
            tell(0, "Error: Group (%d) not found!", id);
            continue;
         }

         tableGroups->clearChanged();
         tableGroups->setValue("NAME", name);

         if (tableGroups->getChanges())
         {
            tableGroups->store();
            tell(2, "STORED %d - name '%s'", id, name);
         }
      }
   }
   else if (strcmp(action, "add") == 0)
   {
      const char* name = getStringFromJson(oObject, "group");
      tell(0, "Add group '%s'", name);

      tableGroups->clear();
      tableGroups->setValue("NAME", name);
      tableGroups->insert();
   }
   else if (strcmp(action, "del") == 0)
   {
      int id = getIntFromJson(oObject, "id");
      tell(0, "delete group '%d'", id);

      tableGroups->clear();
      tableGroups->setValue("ID", id);

      if (!tableGroups->find())
         tell(0, "Group not exists, ignoring 'del' request");
      else
      {
         tell(0, "Delete group %d", id);
         tableGroups->deleteWhere(" id = '%d'", id);
      }
   }

   performGroups(client);

   return replyResult(success, "Konfiguration gespeichert", client);
}

//***************************************************************************
// Config 2 Json
//***************************************************************************

int Daemon::config2Json(json_t* obj)
{
   for (const auto& it : *getConfiguration())
   {
      tableConfig->clear();
      tableConfig->setValue("OWNER", myName());
      tableConfig->setValue("NAME", it.name.c_str());

      if (tableConfig->find())
         json_object_set_new(obj, tableConfig->getStrValue("NAME"), json_string(tableConfig->getStrValue("VALUE")));

      tableConfig->reset();
   }

   return done;
}

//***************************************************************************
// Config Details 2 Json
//***************************************************************************

int Daemon::configDetails2Json(json_t* obj)
{
   for (const auto& it : *getConfiguration())
   {
      if (it.internal)
         continue;

      json_t* oDetail = json_object();
      json_array_append_new(obj, oDetail);

      json_object_set_new(oDetail, "name", json_string(it.name.c_str()));
      json_object_set_new(oDetail, "type", json_integer(it.type));
      json_object_set_new(oDetail, "category", json_string(it.category));
      json_object_set_new(oDetail, "title", json_string(it.title));
      json_object_set_new(oDetail, "descrtiption", json_string(it.description));

      if (it.type == ctChoice || it.type == ctMultiSelect)
         configChoice2json(oDetail, it.name.c_str());

      tableConfig->clear();
      tableConfig->setValue("OWNER", myName());
      tableConfig->setValue("NAME", it.name.c_str());

      if (tableConfig->find())
         json_object_set_new(oDetail, "value", json_string(tableConfig->getStrValue("VALUE")));

      tableConfig->reset();
   }

   return done;
}

int Daemon::configChoice2json(json_t* obj, const char* name)
{
   if (strcmp(name, "style") == 0)
   {
      FileList options;
      int count {0};

      if (getFileList(httpPath, DT_REG, "css", false, &options, count) == success)
      {
         json_t* oArray = json_array();

         for (const auto& opt : options)
         {
            if (strncmp(opt.name.c_str(), "stylesheet-", strlen("stylesheet-")) != 0)
               continue;

            char* p = strdup(strchr(opt.name.c_str(), '-'));
            *(strrchr(p, '.')) = '\0';
            json_array_append_new(oArray, json_string(p+1));
            free(p);
         }

         json_object_set_new(obj, "options", oArray);
      }
   }
   else if (strcmp(name, "heatingType") == 0)
   {
      FileList options;
      int count {0};
      char* path {nullptr};

      asprintf(&path, "%s/img/type", httpPath);

      if (getFileList(path, DT_REG, "png", false, &options, count) == success)
      {
         json_t* oArray = json_array();

         for (const auto& opt : options)
         {
            if (strncmp(opt.name.c_str(), "heating-", strlen("heating-")) != 0)
               continue;

            char* p = strdup(strchr(opt.name.c_str(), '-'));
            *(strrchr(p, '.')) = '\0';
            json_array_append_new(oArray, json_string(p+1));
            free(p);
         }

         json_object_set_new(obj, "options", oArray);
      }

      free(path);
   }
   else if (strcmp(name, "iconSet") == 0)
   {
      FileList options;
      int count {0};
      char* path {nullptr};

      asprintf(&path, "%s/img/state", httpPath);

      if (getFileList(path, DT_DIR, nullptr, false, &options, count) == success)
      {
         json_t* oArray = json_array();

         for (const auto& opt : options)
            json_array_append_new(oArray, json_string(opt.name.c_str()));

         json_object_set_new(obj, "options", oArray);
      }

      free(path);
   }
   else if (strcmp(name, "schema") == 0)
   {
      FileList options;
      int count {0};
      char* path {nullptr};

      asprintf(&path, "%s/img/schema", httpPath);

      if (getFileList(path, DT_REG, "png", false, &options, count) == success)
      {
         json_t* oArray = json_array();

         for (const auto& opt : options)
         {
            if (strncmp(opt.name.c_str(), "schema-", strlen("schema-")) != 0)
               continue;

            char* p = strdup(strchr(opt.name.c_str(), '-'));
            *(strrchr(p, '.')) = '\0';
            json_array_append_new(oArray, json_string(p+1));
            free(p);
         }

         json_object_set_new(obj, "options", oArray);
      }

      free(path);
   }
   else if (strcmp(name, "addrsDashboard") == 0 || strcmp(name, "addrsList") == 0)
   {
      json_t* oArray = json_array();

      tableValueFacts->clear();
      tableValueFacts->setValue("STATE", "A");

      for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
      {
         const char* title = tableValueFacts->getStrValue("USRTITLE");

         if (isEmpty(title))
            title = tableValueFacts->getStrValue("TITLE");

         json_t* oOpt = json_object();

         char* tmp;
         asprintf(&tmp, "%s:0x%02lx", tableValueFacts->getStrValue("TYPE"), tableValueFacts->getIntValue("ADDRESS"));

         json_object_set_new(oOpt, "value", json_string(tmp));
         json_object_set_new(oOpt, "label", json_string(title));
         json_array_append_new(oArray, oOpt);

         // json_array_append_new(oArray, json_string(tmp));
         free(tmp);
      }

      selectActiveValueFacts->freeResult();

      json_object_set_new(obj, "options", oArray);
   }
   else if (strcmp(name, "stateMailStates") == 0)
   {
      json_t* oArray = json_array();

      for (int i = 0; stateInfos[i].code != na; i++)
      {
         json_t* oOpt = json_object();
         json_object_set_new(oOpt, "value", json_string(std::to_string(stateInfos[i].code).c_str()));
         json_object_set_new(oOpt, "label", json_string(stateInfos[i].title));
         json_array_append_new(oArray, oOpt);
      }

      json_object_set_new(obj, "options", oArray);
   }

   return done;
}

//***************************************************************************
// User Details 2 Json
//***************************************************************************

int Daemon::userDetails2Json(json_t* obj)
{
   for (int f = selectAllUser->find(); f; f = selectAllUser->fetch())
   {
      json_t* oDetail = json_object();
      json_array_append_new(obj, oDetail);

      json_object_set_new(oDetail, "user", json_string(tableUsers->getStrValue("USER")));
      json_object_set_new(oDetail, "rights", json_integer(tableUsers->getIntValue("RIGHTS")));
   }

   selectAllUser->freeResult();

   return done;
}

//***************************************************************************
// Value Facts 2 Json
//***************************************************************************

int Daemon::valueFacts2Json(json_t* obj, bool filterActive)
{
   tableValueFacts->clear();

   for (int f = selectAllValueFacts->find(); f; f = selectAllValueFacts->fetch())
   {
      if (filterActive && !tableValueFacts->hasValue("STATE", "A"))
         continue;

      char* key {nullptr};
      asprintf(&key, "%s:%lu", tableValueFacts->getStrValue("TYPE"), tableValueFacts->getIntValue("ADDRESS"));
      json_t* oData = json_object();
      json_object_set_new(obj, key, oData);
      free(key);

      json_object_set_new(oData, "address", json_integer((ulong)tableValueFacts->getIntValue("ADDRESS")));
      json_object_set_new(oData, "type", json_string(tableValueFacts->getStrValue("TYPE")));
      json_object_set_new(oData, "state", json_boolean(tableValueFacts->hasValue("STATE", "A")));
      json_object_set_new(oData, "name", json_string(tableValueFacts->getStrValue("NAME")));
      json_object_set_new(oData, "title", json_string(tableValueFacts->getStrValue("TITLE")));
      json_object_set_new(oData, "usrtitle", json_string(tableValueFacts->getStrValue("USRTITLE")));
      json_object_set_new(oData, "unit", json_string(tableValueFacts->getStrValue("UNIT")));

      if (!tableValueFacts->getValue("CHOICES")->isNull())
         json_object_set_new(oData, "choices", json_string(tableValueFacts->getStrValue("CHOICES")));

      const char* widgetOptions = tableValueFacts->getStrValue("WIDGETOPT");
      if (isEmpty(widgetOptions))
         widgetOptions = "{}";
      json_error_t error;
      json_t* oWidgetOptions = json_loads(widgetOptions, 0, &error);
      if (oWidgetOptions)
         json_object_set_new(oData, "widget", oWidgetOptions);
      else
         tell(1, "Error decoding json: %s (%s, line %d column %d, position %d) [%s]",
              error.text, error.source, error.line, error.column, error.position, widgetOptions);

      tableGroups->clear();
      tableGroups->setValue("ID", tableValueFacts->getIntValue("GROUPID"));

      if (tableGroups->find())
      {
         json_object_set_new(oData, "groupid", json_integer(tableGroups->getIntValue("ID")));
         json_object_set_new(oData, "group", json_string(tableGroups->getStrValue("NAME")));
      }

      tableGroups->reset();
   }

   selectAllValueFacts->freeResult();

   return done;
}

//***************************************************************************
// Groups 2 Json
//***************************************************************************

int Daemon::groups2Json(json_t* obj)
{
   tableGroups->clear();

   for (int f = selectAllGroups->find(); f; f = selectAllGroups->fetch())
   {
      json_t* oData = json_object();
      json_array_append_new(obj, oData);

      json_object_set_new(oData, "id", json_integer((ulong)tableGroups->getIntValue("ID")));
      json_object_set_new(oData, "name", json_string(tableGroups->getStrValue("NAME")));
   }

   selectAllGroups->freeResult();

   return done;
}

//***************************************************************************
// Widget Types 2 Json
//***************************************************************************

int Daemon::widgetTypes2Json(json_t* obj)
{
   for (int type = wtUnknown+1; type < wtCount; type++)
      json_object_set_new(obj, toName((WidgetType)type), json_integer(type));

   return done;
}

//***************************************************************************
// Daemon Status 2 Json
//***************************************************************************

int Daemon::daemonState2Json(json_t* obj)
{
   double averages[3] {0.0, 0.0, 0.0};
   char d[100];

   toElapsed(time(0)-startedAt, d);
   getloadavg(averages, 3);

   json_object_set_new(obj, "state", json_integer(success));
   json_object_set_new(obj, "version", json_string(VERSION));
   json_object_set_new(obj, "runningsince", json_string(d));
   json_object_set_new(obj, "average0", json_real(averages[0]));
   json_object_set_new(obj, "average1", json_real(averages[1]));
   json_object_set_new(obj, "average2", json_real(averages[2]));

   return done;
}

int Daemon::s3200State2Json(json_t* obj)
{
   json_object_set_new(obj, "time", json_integer(currentState.time));
   json_object_set_new(obj, "state", json_integer(currentState.state));
   json_object_set_new(obj, "stateinfo", json_string(currentState.stateinfo));
   json_object_set_new(obj, "mode", json_integer(currentState.mode));
   json_object_set_new(obj, "modeinfo", json_string(currentState.modeinfo));
   json_object_set_new(obj, "version", json_string(currentState.version));
   json_object_set_new(obj, "image", json_string(getStateImage(currentState.state)));

   return done;
}

//***************************************************************************
// Sensor 2 Json
//***************************************************************************

int Daemon::sensor2Json(json_t* obj, cDbTable* table)
{
   double peakMax {0.0};
   double peakMin {0.0};

   tablePeaks->clear();
   tablePeaks->setValue("ADDRESS", table->getIntValue("ADDRESS"));
   tablePeaks->setValue("TYPE", table->getStrValue("TYPE"));

   if (tablePeaks->find())
   {
      peakMax = tablePeaks->getFloatValue("MAX");
      peakMin = tablePeaks->getFloatValue("MIN");
   }

   tablePeaks->reset();

   json_object_set_new(obj, "address", json_integer((ulong)table->getIntValue("ADDRESS")));
   json_object_set_new(obj, "type", json_string(table->getStrValue("TYPE")));
   json_object_set_new(obj, "name", json_string(table->getStrValue("NAME")));

   if (!table->getValue("USRTITLE")->isEmpty())
      json_object_set_new(obj, "title", json_string(table->getStrValue("USRTITLE")));
   else
      json_object_set_new(obj, "title", json_string(table->getStrValue("TITLE")));

   const char* unit = table->getStrValue("UNIT");
   json_object_set_new(obj, "unit", json_string(strcmp(unit, "°") == 0 ? "°C" : unit));

   json_object_set_new(obj, "rights", json_integer(table->getIntValue("RIGHTS")));

   json_object_set_new(obj, "peak", json_real(peakMax));
   json_object_set_new(obj, "peakmin", json_real(peakMin));

   return done;
}

//***************************************************************************
// images2Json
//***************************************************************************

int Daemon::images2Json(json_t* obj)
{
   FileList images;
   int count {0};
   char* path {nullptr};

   json_array_append_new(obj, json_string(""));
   asprintf(&path, "%s/img/icon", httpPath);

   if (getFileList(path, DT_REG, nullptr, false, &images, count) == success)
   {
      sortFileList(images);

      for (const auto& img : images)
      {
         char* imgPath {nullptr};
         asprintf(&imgPath, "img/icon/%s", img.name.c_str());
         json_array_append_new(obj, json_string(imgPath));
         free(imgPath);
      }
   }

   free(path);

   return done;
}

//***************************************************************************
// Web File Exists
//***************************************************************************

bool Daemon::webFileExists(const char* file, const char* base)
{
   char* path {nullptr};
   asprintf(&path, "%s/%s/%s", httpPath, base ? base : "", file);
   bool exist = fileExists(path);
   free(path);

   return exist;
}

// Daemon::WidgetType Daemon::getWidgetTypeOf(std::string type, std::string unit, uint address)
// {
//    if (type == "DI" || type == "DO")
//       return wtSymbol;

//    if (type == "UD" && address == udState)
//       return wtSymbol;

//    if (type == "UD")
//       return wtText;

//    if (type == "AO")
//       return wtChart;

//    if (type == "SD")
//       return wtChart;

//    if (unit == "°C" || unit == "°" || unit == "%" || unit == "V" || unit == "A" || unit == "U") // Volt/Ampere/Prozent/Temperatur/Umin
//       return wtChart;

//    if (!unit.length())
//       return wtSymbol;

//    return wtValue;
// }
