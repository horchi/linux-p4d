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

      tell(eloWebSock, "<= '%s'", messagesIn.front().c_str());
      oData = json_loads(messagesIn.front().c_str(), 0, &error);

      // get the request

      Event event = cWebService::toEvent(getStringFromJson(oData, "event", "<null>"));
      long client = getLongFromJson(oData, "client");
      oObject = json_object_get(oData, "object");
      int addr = getIntFromJson(oObject, "address");

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
            case evData:            status = performData(client);                    break;
            case evGetToken:        status = performTokenRequest(oObject, client);   break;
            case evToggleIo:        status = performToggleIo(oObject, client);       break;
            case evToggleIoNext:    status = toggleIoNext(addr);                     break;
            case evToggleMode:      status = toggleOutputMode(addr);                 break;
            case evStoreConfig:     status = storeConfig(oObject, client);           break;
            case evSetup:           status = performConfigDetails(client);           break;
            case evSystem:          status = performSystem(oObject, client);         break;
            case evSyslog:          status = performSyslog(oObject, client);         break;
            case evAlerts:          status = performAlerts(oObject, client);         break;
            case evStoreAlerts:     status = storeAlerts(oObject, client);           break;

            case evStoreIoSetup:    status = storeIoSetup(oObject, client);          break;
            case evChartData:       status = performChartData(oObject, client);      break;
            case evUserDetails:     status = performUserDetails(client);             break;
            case evStoreUserConfig: status = storeUserConfig(oObject, client);       break;
            case evChangePasswd:    status = performPasswChange(oObject, client);    break;
            case evGroups:          status = performGroups(client);                  break;
            case evGroupConfig:     status = storeGroups(oObject, client);           break;
            case evStoreDashboards: status = storeDashboards(oObject, client);       break;

            case evCommand:         status = performCommand(oObject, client);        break;
            case evForceRefresh:    status = performForceRefresh(oObject, client);   break;
            case evChartbookmarks:  status = performChartbookmarks(client);          break;
            case evImageConfig:     status = performImageConfig(oObject, client);    break;
            case evSchema:          status = performSchema(oObject, client);         break;
            case evStoreSchema:     status = storeSchema(oObject, client);           break;
            case evStoreChartbookmarks: status = storeChartbookmarks(oObject, client); break;

            default:
            {
               if (dispatchSpecialRequest(event,oObject, client) == ignore)
                  tell(eloAlways, "Error: Received unexpected client request '%s' at [%s]",
                       cWebService::toName(event), messagesIn.front().c_str());
            }
         }
      }
      else
      {
         if (client == na)
            tell(eloAlways, "Insufficient rights to '%s', missing login", getStringFromJson(oData, "event", "<null>"));
         else
            tell(eloAlways, "Insufficient rights to '%s' for user '%s'", getStringFromJson(oData, "event", "<null>"),
                 wsClients[(void*)client].user.c_str());
         // replyResult(fail, "Insufficient rights", client);
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
      case evCommand:             return rights & urFullControl;
      case evSyslog:              return rights & urAdmin;
      case evSystem:              return rights & urAdmin;
      case evForceRefresh:        return true;

      case evChartbookmarks:      return rights & urView;
      case evStoreChartbookmarks: return rights & urSettings;
      case evAlerts:              return rights & urSettings;
      case evStoreAlerts:         return rights & urSettings;
      case evStoreDashboards:     return rights & urFullControl;
      case evGroups:              return rights & urSettings;
      case evImageConfig:         return rights & urSettings;

      case evSchema:              return rights & urView;
      case evStoreSchema:         return rights & urSettings;

      default: break;
   }

   if (onCheckRights(client, event, rights))
      return true;

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
      tell(eloAlways, "Error: Web request failed with '%s' (%d)", message, status);

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
      tell(eloAlways, "Error: Requested web action failed with '%s' (%d)", message, status);

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

   tell(eloDebugWebSock, "Now %zu clients in list", wsClients.size());

   if (tableUsers->find() && tableUsers->hasValue("TOKEN", token))
   {
      wsClients[(void*)client].type = ctWithLogin;
      wsClients[(void*)client].rights = tableUsers->getIntValue("RIGHTS");
   }
   else
   {
      wsClients[(void*)client].type = ctActive;
      wsClients[(void*)client].rights = urView;  // allow view without login
      tell(eloAlways, "Warning: Unknown user '%s' or token mismatch connected!", user);

      json_t* oJson = json_object();
      json_object_set_new(oJson, "user", json_string(user));
      json_object_set_new(oJson, "state", json_string("reject"));
      json_object_set_new(oJson, "value", json_string(""));
      pushOutMessage(oJson, "token", client);
   }

   tell(eloWebSock, "Login of client 0x%x; user '%s'; type is %d; page %s", (unsigned int)client, user,
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

   oJson = json_array();
   valueTypes2Json(oJson);
   pushOutMessage(oJson, "valuetypes", client);

   oJson = json_object();
   valueFacts2Json(oJson, false);
   pushOutMessage(oJson, "valuefacts", client);

   oJson = json_object();
   dashboards2Json(oJson);
   pushOutMessage(oJson, "dashboards", client);

   oJson = json_array();
   images2Json(oJson);
   pushOutMessage(oJson, "images", client);

   oJson = json_array();
   groups2Json(oJson);
   pushOutMessage(oJson, "grouplist", client);

   oJson = json_array();
   commands2Json(oJson);
   pushOutMessage(oJson, "commands", client);

   performData(client, "init");

   return done;
}

//***************************************************************************
// Perform Data
//***************************************************************************

int Daemon::performData(long client, const char* event)
{
   for (auto sj : jsonSensorList)
      json_decref(sj.second);

   jsonSensorList.clear();

   for (const auto& typeSensorsIt : sensors)
   {
      for (const auto& sensorIt : typeSensorsIt.second)
      {
         const SensorData* sensor = &sensorIt.second;

         if (!sensor->type.length())
            continue;

         json_t* ojData = json_object();
         char* key {nullptr};
         asprintf(&key, "%s:0x%02x", sensor->type.c_str(), sensor->address);
         jsonSensorList[key] = ojData;

         sensor2Json(ojData, sensor->type.c_str(), sensor->address);

         // #TODO - check validity like != nan and not older than 2 minutes
         // if (isNan(aiSensors[addr].value) || aiSensors[addr].last < time(0)-120)
         //   continue ...

         json_object_set_new(ojData, "last", json_integer(sensor->last));

         if (sensor->kind == "status")
         {
            json_object_set_new(ojData, "value", json_integer(sensor->state));
            json_object_set_new(ojData, "score", json_integer(sensor->value));
            json_object_set_new(ojData, "hue", json_integer(sensor->hue));
            json_object_set_new(ojData, "sat", json_integer(sensor->sat));
         }
         else if (sensor->kind == "value")
            json_object_set_new(ojData, "value", json_real(sensor->value));
         else if (!sensor->kind.length())
            tell(eloAlways, "Info: Missing 'kind' property for sensor '%s'", key);

         // send text/image if set, independent of 'kind'

         if (sensor->image != "")
            json_object_set_new(ojData, "image", json_string(sensor->image.c_str()));

         if (sensor->text != "")
         {
            json_object_set_new(ojData, "text", json_string(sensor->text.c_str()));
            const char* txtImage = getTextImage(key, sensor->text.c_str());

            if (sensor->image == "" && txtImage)
               json_object_set_new(ojData, "image", json_string(txtImage));
         }

         if (sensor->battery != na)
            json_object_set_new(ojData, "battery", json_integer(sensor->battery));

         if (sensor->disabled)
            json_object_set_new(ojData, "disabled", json_boolean(true));

         if (sensor->type ==  "DO")    // Digital apecial properties for DO
         {
            json_object_set_new(ojData, "mode", json_string(sensor->mode == omManual ? "manual" : "auto"));
            json_object_set_new(ojData, "options", json_integer(sensor->opt));
            json_object_set_new(ojData, "next", json_integer(sensor->next));
         }

         free(key);
      }
   }

   pushDataUpdate(event ? event : "update", client);

   return done;
}

//***************************************************************************
// Perform Page Change
//***************************************************************************

int Daemon::performPageChange(json_t* oObject, long client)
{
   std::string page = getStringFromJson(oObject, "page", "");

   wsClients[(void*)client].page = page;

   if (page == "list")
   {
      json_t* oJson = json_object();
      daemonState2Json(oJson);
      pushOutMessage(oJson, "daemonstate", client);
   }

   return done;
}

int Daemon::performLogout(json_t* oObject)
{
   long client = getLongFromJson(oObject, "client");
   tell(eloDebugWebSock, "Logout of client 0x%x", (unsigned int)client);
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

   tell(eloWebSock, "Token request of client 0x%x for user '%s'", (unsigned int)client, user);

   tableUsers->clear();
   tableUsers->setValue("USER", user);

   if (tableUsers->find())
   {
      if (tableUsers->hasValue("PASSWD", passwd))
      {
         tell(eloWebSock, "Token request for user '%s' succeeded", user);
         json_object_set_new(oJson, "user", json_string(user));
         json_object_set_new(oJson, "rights", json_integer(tableUsers->getIntValue("RIGHTS")));
         json_object_set_new(oJson, "state", json_string("confirm"));
         json_object_set_new(oJson, "value", json_string(tableUsers->getStrValue("TOKEN")));
         pushOutMessage(oJson, "token", client);
      }
      else
      {
         tell(eloWebSock, "Token request for user '%s' failed, wrong password", user);
         json_object_set_new(oJson, "user", json_string(user));
         json_object_set_new(oJson, "rights", json_integer(0));
         json_object_set_new(oJson, "state", json_string("reject"));
         json_object_set_new(oJson, "value", json_string(""));
         pushOutMessage(oJson, "token", client);
      }
   }
   else
   {
      tell(eloWebSock, "Token request for user '%s' failed, unknown user", user);
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
// Perform Toggle IO
//***************************************************************************

int Daemon::performToggleIo(json_t* oObject, long client)
{
   int addr = getIntFromJson(oObject, "address");
   const char* type = getStringFromJson(oObject, "type");
   std::string action = getStringFromJson(oObject, "action");

   if (action == "toggle")
      return toggleIo(addr, type);

   if (action == "dim")
   {
      int value = getIntFromJson(oObject, "value");
      return toggleIo(addr, type, true, value);
   }

   if (action == "color")
   {
      int hue = getIntFromJson(oObject, "hue");
      int sat = getIntFromJson(oObject, "saturation");
      int bri = getIntFromJson(oObject, "bri");
      tell(eloAlways, "Changing color of %s:0x%02x to %d", type, addr, hue);
      return toggleColor(addr, type, hue, sat, bri);
   }

   return done;
}

//***************************************************************************
// Alert Test Mail
//***************************************************************************

int Daemon::performAlertTestMail(int id, long client)
{
   tell(eloDetail, "Test mail for alert (%d) requested", id);

   if (isEmpty(mailScript))
      return replyResult(fail, "missing mail script", client);

   if (!fileExists(mailScript))
      return replyResult(fail, "mail script not found", client);

   if (!selectMaxTime->find())
      tell(eloAlways, "Warning: Got no result by 'select max(time) from samples'");

   time_t  last = tableSamples->getTimeValue("TIME");
   selectMaxTime->freeResult();

   tableSensorAlert->clear();
   tableSensorAlert->setValue("ID", id);

   if (!tableSensorAlert->find())
      return replyResult(fail, "requested alert ID not found", client);

   alertMailBody = "";
   alertMailSubject = "";

   if (!performAlertCheck(tableSensorAlert->getRow(), last, 0, yes/*force*/))
      return replyResult(fail, "send failed", client);

   tableSensorAlert->reset();

   return replyResult(success, "mail sended", client);
}

//***************************************************************************
// Perform WS Sensor Alert Request
//***************************************************************************

int Daemon::performAlerts(json_t* oObject, long client)
{
   json_t* oArray = json_array();

   tableSensorAlert->clear();

   for (int f = selectAllSensorAlerts->find(); f; f = selectAllSensorAlerts->fetch())
   {
      json_t* oData = json_object();
      json_array_append_new(oArray, oData);

      json_object_set_new(oData, "id", json_integer(tableSensorAlert->getIntValue("ID")));
      json_object_set_new(oData, "kind", json_string(tableSensorAlert->getStrValue("ID")));
      json_object_set_new(oData, "subid", json_integer(tableSensorAlert->getIntValue("SUBID")));
      json_object_set_new(oData, "lgop", json_integer(tableSensorAlert->getIntValue("LGOP")));
      json_object_set_new(oData, "type", json_string(tableSensorAlert->getStrValue("TYPE")));
      json_object_set_new(oData, "address", json_integer(tableSensorAlert->getIntValue("ADDRESS")));
      json_object_set_new(oData, "state", json_string(tableSensorAlert->getStrValue("STATE")));
      json_object_set_new(oData, "min", json_integer(tableSensorAlert->getIntValue("MIN")));
      json_object_set_new(oData, "max", json_integer(tableSensorAlert->getIntValue("MAX")));
      json_object_set_new(oData, "rangem", json_integer(tableSensorAlert->getIntValue("RANGEM")));
      json_object_set_new(oData, "delta", json_integer(tableSensorAlert->getIntValue("DELTA")));
      json_object_set_new(oData, "maddress", json_string(tableSensorAlert->getStrValue("MADDRESS")));
      json_object_set_new(oData, "msubject", json_string(tableSensorAlert->getStrValue("MSUBJECT")));
      json_object_set_new(oData, "mbody", json_string(tableSensorAlert->getStrValue("MBODY")));
      json_object_set_new(oData, "maxrepeat", json_integer(tableSensorAlert->getIntValue("MAXREPEAT")));

      //json_object_set_new(oData, "lastalert", json_integer(0));
   }

   selectAllSensorAlerts->freeResult();
   pushOutMessage(oArray, "alerts", client);

   return done;
}

//***************************************************************************
// Store Sensor Alerts
//***************************************************************************

int Daemon::storeAlerts(json_t* oObject, long client)
{
   const char* action = getStringFromJson(oObject, "action", "");

   if (strcmp(action, "delete") == 0)
   {
      int alertid = getIntFromJson(oObject, "alertid", na);

      tableSensorAlert->deleteWhere("id = %d", alertid);

      performAlerts(0, client);
      replyResult(success, "Sensor Alert gelöscht", client);
   }

   else if (strcmp(action, "store") == 0)
   {
      json_t* array = json_object_get(oObject, "alerts");
      size_t index {0};
      json_t* jObj {nullptr};

      json_array_foreach(array, index, jObj)
      {
         int id = getIntFromJson(jObj, "id", na);
         tell(eloAlways, "Store alert %d", id);
         tableSensorAlert->clear();

         if (id != na)
         {
            tableSensorAlert->setValue("ID", id);

            if (!tableSensorAlert->find())
            {
               tell(eloAlways, "Warning: Alert id %d not found, aborting", id);
               continue;
            }
         }

         tableSensorAlert->clearChanged();
         tableSensorAlert->setValue("STATE", getStringFromJson(jObj, "state"));
         tableSensorAlert->setValue("MAXREPEAT", getIntFromJson(jObj, "maxrepeat"));

         tableSensorAlert->setValue("ADDRESS", getIntFromJson(jObj, "address"));
         tableSensorAlert->setValue("TYPE", getStringFromJson(jObj, "type"));
         tableSensorAlert->setValue("MIN", getIntFromJson(jObj, "min"));
         tableSensorAlert->setValue("MAX", getIntFromJson(jObj, "max"));
         tableSensorAlert->setValue("DELTA", getIntFromJson(jObj, "delta"));
         tableSensorAlert->setValue("RANGEM", getIntFromJson(jObj, "rangem"));
         tableSensorAlert->setValue("MADDRESS", getStringFromJson(jObj, "maddress"));
         tableSensorAlert->setValue("MSUBJECT", getStringFromJson(jObj, "msubject"));
         tableSensorAlert->setValue("MBODY", getStringFromJson(jObj, "mbody"));
         tableSensorAlert->setValue("KIND", "M");

         if (id == na)
            tableSensorAlert->insert();
         else if (tableSensorAlert->getChanges())
            tableSensorAlert->update();
      }

      performAlerts(0, client);
      replyResult(success, "Konfiguration gespeichert", client);
   }

   return success;
}

//***************************************************************************
// Perform System Data
//***************************************************************************

int Daemon::performSystem(json_t* oObject, long client)
{
   tableTableStatistics->clear();
   tableTableStatistics->setValue("SCHEMA", connection->getName());

   json_t* jObject = json_object();
   json_t* jArray = json_array();
   json_object_set_new(jObject, "tables", jArray);

   for (int f = selectTableStatistic->find(); f; f = selectTableStatistic->fetch())
   {
      json_t* jItem = json_object();
      json_array_append_new(jArray, jItem);

      json_object_set_new(jItem, "name", json_string(tableTableStatistics->getStrValue("NAME")));
      json_object_set_new(jItem, "tblsize", json_string(bytesPretty(tableTableStatistics->getIntValue("DATASZ"), 2)));
      json_object_set_new(jItem, "idxsize", json_string(bytesPretty(tableTableStatistics->getIntValue("INDEXSZ"), 2)));
      json_object_set_new(jItem, "rows", json_integer(tableTableStatistics->getIntValue("ROWS")));
   }

   selectTableStatistic->freeResult();

   FsStat stat;

   jArray = json_array();
   json_object_set_new(jObject, "filesystems", jArray);

   if (fsStat("/", &stat) == success)
   {
      json_t* jItem = json_object();
      json_array_append_new(jArray, jItem);

      json_object_set_new(jItem, "name", json_string("/"));
      json_object_set_new(jItem, "total", json_string(bytesPretty(stat.total, 2)));
      json_object_set_new(jItem, "available", json_string(bytesPretty(stat.available, 2)));
   }

   pushOutMessage(jObject, "system", client);

   return done;
}

//***************************************************************************
// Perform Syslog Request
//***************************************************************************

int Daemon::performSyslog(json_t* oObject, long client)
{
   const char* name {"/var/log/" TARGET ".log"};

   if (!client)
      return done;

   const char* log = getStringFromJson(oObject, "log");
   json_t* oJson = json_object();
   std::vector<std::string> lines;
   std::string result;

   if (!isEmpty(log))
      name = log;

   if (loadTailLinesFromFile(name, 150, lines) == success)
   {
      for (auto it = lines.rbegin(); it != lines.rend(); ++it)
         result += *it + "\n";
   }

   result += "...\n...\n";

   json_object_set_new(oJson, "lines", json_string(result.c_str()));
   pushOutMessage(oJson, "syslog", client);

   return done;
}

//***************************************************************************
// Perform Config Data Request
//***************************************************************************

int Daemon::performConfigDetails(long client)
{
   if (!client)
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
   if (!client)
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
   if (!client)
      return done;

   json_t* oJson = json_array();
   groups2Json(oJson);
   pushOutMessage(oJson, "groups", client);

   return done;
}

//***************************************************************************
// Perform Send Mail
//***************************************************************************

int Daemon::performTestMail(json_t* oObject, long client)
{
   int alertid = getIntFromJson(oObject, "alertid", na);

   if (alertid != na)
      return performAlertTestMail(alertid, client);

   const char* subject = "Test Mail";
   const char* body = "Test";

   tell(eloDebugWebSock, "Test mail requested with: '%s/%s'", subject, body);

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
   if (!client)
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

   if (strcmp(id, "chart") == 0)
   {
      setConfigItem("chartRange", range);

      if (!isEmpty(sensors))
      {
         tell(eloDebugWebSock, "Storing sensors '%s' for chart", sensors);
         setConfigItem("chartSensors", sensors);
      }
   }

   tell(eloWebSock, "Selecting chart data for sensors '%s' with range %.1f ..", sensors, range);

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

   json_t* aAvailableSensors {nullptr};

   if (!widget)
      aAvailableSensors = json_array();

   for (int f = selectActiveValueFacts->find(); f; f = selectActiveValueFacts->fetch())
   {
      if (!tableValueFacts->hasValue("RECORD", "A"))
         continue;

      char* id {nullptr};
      asprintf(&id, "%s:0x%02lx", tableValueFacts->getStrValue("TYPE"), tableValueFacts->getIntValue("ADDRESS"));

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

      char* key {nullptr};
      asprintf(&key, "%s:0x%02lx", tableValueFacts->getStrValue("TYPE"), tableValueFacts->getIntValue("ADDRESS"));
      json_object_set_new(oSample, "key", json_string(key));
      free(key);

      json_object_set_new(oSample, "sensor", json_string(sensor));
      json_t* oData = json_array();
      json_object_set_new(oSample, "data", oData);
      free(sensor);

      tableSamples->clear();
      tableSamples->setValue("TYPE", tableValueFacts->getStrValue("TYPE"));
      tableSamples->setValue("ADDRESS", tableValueFacts->getIntValue("ADDRESS"));

      tell(eloDebugWebSock, " selecting '%s - %s' for '%s:0x%02lx'",
           l2pTime(rangeFrom.getTimeValue()).c_str(),
           l2pTime(rangeTo.getTimeValue()).c_str(),
           tableValueFacts->getStrValue("TYPE"), tableValueFacts->getIntValue("ADDRESS"));

      uint count {0};

      for (int f = select->find(); f; f = select->fetch())
      {
         // tell(eloDebugWebSock, "0x%x: '%s' : %0.2f", (uint)tableSamples->getStrValue("ADDRESS"),
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

      tell(eloDebugWebSock, " collected %d samples'", count);
      select->freeResult();
   }

   if (!widget)
      json_object_set_new(oMain, "sensors", aAvailableSensors);

   json_object_set_new(oMain, "rows", oJson);
   json_object_set_new(oMain, "id", json_string(id));
   selectActiveValueFacts->freeResult();
   tell(eloDebugWebSock, ".. done");
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
   int count {0};

   if (!client)
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
         replyResult(fail, "User alredy exists, ignoring 'add' request", client);
      else
      {
         char* token {nullptr};
         asprintf(&token, "%s_%s_%s", getUniqueId(), l2pTime(time(0)).c_str(), user);
         tell(eloWebSock, "Add user '%s' with token [%s]", user, token);
         tableUsers->setValue("PASSWD", passwd);
         tableUsers->setValue("TOKEN", token);
         tableUsers->setValue("RIGHTS", urView);
         tableUsers->store();
         count++;
         free(token);
      }
   }
   else if (strcmp(action, "store") == 0)
   {
      if (!exists)
         replyResult(fail, "User not exists, ignoring 'store' request", client);
      else
      {
         tell(eloWebSock, "Store settings for user '%s'", user);
         tableUsers->setValue("RIGHTS", rights);
         tableUsers->store();
         count++;
      }
   }
   else if (strcmp(action, "del") == 0)
   {
      if (!exists)
         replyResult(fail, "User not exists, ignoring 'del' request", client);
      else
      {
         tell(eloWebSock, "Delete user '%s'", user);
         tableUsers->deleteWhere(" user = '%s'", user);
         count++;
      }
   }
   else if (strcmp(action, "resetpwd") == 0)
   {
      if (!exists)
         replyResult(fail, "User not exists, ignoring 'resetpwd' request", client);
      else
      {
         tell(eloWebSock, "Reset password of user '%s'", user);
         tableUsers->setValue("PASSWD", passwd);
         tableUsers->store();
         replyResult(success, "Passwort gespeichert", client);
         count++;
      }
   }
   else if (strcmp(action, "resettoken") == 0)
   {
      if (!exists)
         replyResult(fail, "User not exists, ignoring 'resettoken' request", client);
      else
      {
         char* token {nullptr};
         asprintf(&token, "%s_%s_%s", getUniqueId(), l2pTime(time(0)).c_str(), user);
         tell(eloWebSock, "Reset token of user '%s' to '%s'", user, token);
         tableUsers->setValue("TOKEN", token);
         tableUsers->store();
         count++;
         free(token);
      }
   }

   if (count)
      replyResult(success, "Gespeichert", client);

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
   if (!client)
      return done;

   const char* user = getStringFromJson(oObject, "user");
   const char* passwd = getStringFromJson(oObject, "passwd");

   if (strcmp(wsClients[(void*)client].user.c_str(), user) != 0)
   {
      tell(eloWebSock, "Warning: User '%s' tried to change password of '%s'",
           wsClients[(void*)client].user.c_str(), user);
      return done;
   }

   tableUsers->clear();
   tableUsers->setValue("USER", user);

   if (tableUsers->find())
   {
      tell(eloWebSock, "User '%s' changed password", user);
      tableUsers->setValue("PASSWD", passwd);
      tableUsers->store();
      replyResult(success, "Passwort gespeichert", client);
   }

   tableUsers->reset();

   return done;
}

//***************************************************************************
// Perform Schema Data
//***************************************************************************

int Daemon::performSchema(json_t* oObject, long client)
{
   if (!client)
      return done;

   json_t* oArray = json_array();

   tableSchemaConf->clear();

   for (int f = selectAllSchemaConf->find(); f; f = selectAllSchemaConf->fetch())
   {
      tableValueFacts->clear();
      tableValueFacts->setValue("ADDRESS", tableSchemaConf->getIntValue("ADDRESS"));
      tableValueFacts->setValue("TYPE", tableSchemaConf->getStrValue("TYPE"));

      if (!tableSchemaConf->hasValue("TYPE", "UC") &&
          !tableSchemaConf->hasValue("TYPE", "CN") &&
          (!tableValueFacts->find() || !tableValueFacts->hasValue("STATE", "A")))
         continue;

      json_t* oData = json_object();
      json_array_append_new(oArray, oData);

      addFieldToJson(oData, tableSchemaConf, "ADDRESS");
      addFieldToJson(oData, tableSchemaConf, "TYPE");
      addFieldToJson(oData, tableSchemaConf, "STATE");
      addFieldToJson(oData, tableSchemaConf, "KIND");
      addFieldToJson(oData, tableSchemaConf, "WIDTH");
      addFieldToJson(oData, tableSchemaConf, "HEIGHT");
      addFieldToJson(oData, tableSchemaConf, "SHOWUNIT");
      addFieldToJson(oData, tableSchemaConf, "SHOWTITLE");
      addFieldToJson(oData, tableSchemaConf, "USRTEXT");
      addFieldToJson(oData, tableSchemaConf, "FUNCTION", true, "fct");

      const char* properties = tableSchemaConf->getStrValue("PROPERTIES");
      if (isEmpty(properties))
         properties = "{}";
      json_error_t error;
      json_t* o = json_loads(properties, 0, &error);
      json_object_set_new(oData, "properties", o);
   }

   selectAllSchemaConf->freeResult();

   pushOutMessage(oArray, "schema", client);
   performData(client);

   return done;
}

//***************************************************************************
// Store Schema
//***************************************************************************

int Daemon::storeSchema(json_t* oObject, long client)
{
   if (!client)
      return done;

   size_t index {0};
   json_t* jObj {nullptr};

   json_array_foreach(oObject, index, jObj)
   {
      int address = getIntFromJson(jObj, "address");
      const char* type = getStringFromJson(jObj, "type");
      int deleted = getBoolFromJson(jObj, "deleted");

      tableSchemaConf->clear();
      tableSchemaConf->setValue("ADDRESS", address);
      tableSchemaConf->setValue("TYPE", type);

      if (tableSchemaConf->find() && deleted)
      {
         tableSchemaConf->deleteWhere("%s = '%s' and %s = %d",
                                      tableSchemaConf->getField("TYPE")->getName(), type,
                                      tableSchemaConf->getField("ADDRESS")->getName(), address);
         continue;
      }

      tableSchemaConf->setValue("FUNCTION", getStringFromJson(jObj, "fct"));
      tableSchemaConf->setValue("USRTEXT", getStringFromJson(jObj, "usrtext"));
      tableSchemaConf->setValue("KIND", getIntFromJson(jObj, "kind"));
      tableSchemaConf->setValue("WIDTH", getIntFromJson(jObj, "width"));
      tableSchemaConf->setValue("HEIGHT", getIntFromJson(jObj, "height"));
      tableSchemaConf->setValue("SHOWTITLE", getIntFromJson(jObj, "showtitle"));
      tableSchemaConf->setValue("SHOWUNIT", getIntFromJson(jObj, "showunit"));
      tableSchemaConf->setValue("STATE", getStringFromJson(jObj, "state"));

      json_t* jProp = json_object_get(jObj, "properties");
      char* p = json_dumps(jProp, JSON_REAL_PRECISION(4));

      if (tableSchemaConf->getField("PROPERTIES")->getSize() < (int)strlen(p))
         tell(eloWebSock, "Warning, Ignoring properties of %s:0x%x due to field limit of %d bytes",
              type, address, tableSchemaConf->getField("PROPERTIES")->getSize());
      else
         tableSchemaConf->setValue("PROPERTIES", p);

      tableSchemaConf->store();
      free(p);
   }

   tableSchemaConf->reset();
   replyResult(success, "Konfiguration gespeichert", client);

   return done;
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
      tell(eloDebugWebSock, "Debug: Storing config item '%s' with '%s'", key, json_string_value(jValue));
      setConfigItem(key, json_string_value(jValue));
      count++;
   }

   // create link for the stylesheet

   const char* name = getStringFromJson(obj, "style");

   if (!isEmpty(name) && strcmp(name, oldStyle) != 0)
   {
      tell(eloWebSock, "Info: Creating link 'stylesheet.css' to '%s'", name);
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
// Store Dashboards
//***************************************************************************

int Daemon::storeDashboards(json_t* obj, long client)
{
   const char* action = getStringFromJson(obj, "action", "");

   if (strcmp(action, "order") == 0)              // reorder dashboards
   {
      // {"action": "order", "order": ["16", "25", "14", "27"]},

      size_t index {0};
      json_t* jId {nullptr};
      json_t* array = getObjectFromJson(obj, "order");

      json_array_foreach(array, index, jId)
      {
         int dashboardId = json_integer_value(jId);
         tableDashboards->clear();
         tableDashboards->setValue("ID", dashboardId);

         if (tableDashboards->find())
         {
            tableDashboards->setValue("ORDER", (long)index);
            tableDashboards->store();
         }
      }
   }
   else if (strcmp(action, "move") == 0)          // move widget to other dashboard
   {
      const char* key = getStringFromJson(obj, "key");
      int fromId = getIntFromJson(obj, "from");
      int toId = getIntFromJson(obj, "to");
      auto tuple = split(key, ':');

      tell(eloAlways, "Move widget '%s' from dashboard %d to %d", key, fromId, toId);

      tableDashboardWidgets->clear();
      tableDashboardWidgets->setValue("DASHBOARDID", fromId);
      tableDashboardWidgets->setValue("TYPE", tuple[0].c_str());
      tableDashboardWidgets->setValue("ADDRESS", strtol(tuple[1].c_str(), nullptr, 0));

      if (!tableDashboardWidgets->find())
         return done;

      std::string options = tableDashboardWidgets->getStrValue("WIDGETOPTS");

      tableDashboardWidgets->deleteWhere("dashboardid = %d and type = '%s' and address = %ld", fromId, tuple[0].c_str(), strtol(tuple[1].c_str(), nullptr, 0));

      tableDashboardWidgets->clear();
      tableDashboardWidgets->setValue("DASHBOARDID", toId);
      tableDashboardWidgets->setValue("TYPE", tuple[0].c_str());
      tableDashboardWidgets->setValue("ADDRESS", strtol(tuple[1].c_str(), nullptr, 0));
      tableDashboardWidgets->setValue("WIDGETOPTS", options.c_str());
      tableDashboardWidgets->insert();

      json_t* oJson = json_object();
      dashboards2Json(oJson);
      pushOutMessage(oJson, "dashboards", client);
   }
   else
   {
      const char* dashboardIdStr {nullptr};
      json_t* jObj {nullptr};

      json_object_foreach(obj, dashboardIdStr, jObj)
      {
         long dashboardId = atol(dashboardIdStr);
         const char* dashboardTitle = getStringFromJson(jObj, "title", "<new>");
         const char* dashboardSymbol = getStringFromJson(jObj, "symbol");
         const char* action = getStringFromJson(jObj, "action", "store");

         if (strcmp(action, "delete") == 0)
         {
            tell(eloDebugWebSock, "Debug: Deleting dashboard '%ld'", dashboardId);

            tableDashboards->deleteWhere("%s = %ld", tableDashboards->getField("ID")->getDbName(), dashboardId);
            tableDashboardWidgets->deleteWhere("%s = %ld", tableDashboardWidgets->getField("DASHBOARDID")->getDbName(), dashboardId);
            return done;
         }

         if (dashboardId == -1)
         {
            tableDashboards->clear();
            tableDashboards->setValue("TITLE", dashboardTitle);
            if (dashboardSymbol)
               tableDashboards->setValue("SYMBOL", dashboardSymbol);
            tableDashboards->store();
            tell(eloWebSock, "Created new dashboard '%ld/%s'!", dashboardId, dashboardTitle);
            dashboardId = tableDashboards->getLastInsertId();
         }

         tableDashboards->clear();
         tableDashboards->setValue("ID", dashboardId);

         if (!tableDashboards->find())
         {
            tell(eloDebugWebSock, "Debug: Storing dashboard '%s' failed", dashboardTitle);
            return fail;
         }

         // dashboard settings

         json_t* jOptions = getObjectFromJson(jObj, "options");
         char* options = json_dumps(jOptions, JSON_REAL_PRECISION(4));

         tableDashboards->setValue("TITLE", dashboardTitle);

         if (!isEmpty(options) || tableDashboards->getValue("OPTS")->isEmpty())
            tableDashboards->setValue("OPTS", !isEmpty(options) ? options : "{}");

         if (dashboardSymbol)
            tableDashboards->setValue("SYMBOL", dashboardSymbol);
         tableDashboards->store();
         tableDashboards->reset();
         free(options);
         json_decref(jOptions);

         // the widgets

         int ord {0};
         const char* id {nullptr};
         json_t* jData {nullptr};
         json_t* jWidgets = getObjectFromJson(jObj, "widgets");

         if (jWidgets)
         {
            tell(eloDebugWebSock, "Debug: Storing widgets of dashboard '%s'", dashboardTitle);
            tableDashboardWidgets->deleteWhere("%s = %ld", tableDashboardWidgets->getField("DASHBOARDID")->getDbName(), dashboardId);

            json_object_foreach(jWidgets, id, jData)
            {
               auto tuple = split(id, ':');
               char* opts = json_dumps(jData, JSON_REAL_PRECISION(4));

               tableDashboardWidgets->clear();
               tableDashboardWidgets->setValue("DASHBOARDID", dashboardId);
               tableDashboardWidgets->setValue("ORDER", ord++);
               tableDashboardWidgets->setValue("TYPE", tuple[0].c_str());
               tableDashboardWidgets->setValue("ADDRESS", strtol(tuple[1].c_str(), nullptr, 0));

               if (isEmpty(opts))
               {
                  tableValueFacts->clear();
                  tableValueFacts->setValue("TYPE", tuple[0].c_str());
                  tableValueFacts->setValue("ADDRESS", strtol(tuple[1].c_str(), nullptr, 0));

                  if (tableValueFacts->find())
                  {
                     const char* title = !tableValueFacts->getValue("USRTITLE")->isEmpty() ? tableValueFacts->getStrValue("USRTITLE") : tableValueFacts->getStrValue("NAME");
                     json_t* jDefaults = json_object();
                     widgetDefaults2Json(jDefaults, tableValueFacts->getStrValue("TYPE"),
                                         tableValueFacts->getStrValue("UNIT"), title,
                                         tableValueFacts->getIntValue("ADDRESS"));

                     opts = json_dumps(jDefaults, JSON_REAL_PRECISION(4));
                     json_decref(jDefaults);
                  }
                  else
                  {
                     if (strstr(id, "SPACER:"))
                        opts = strdup("{ \"widgettype\": 10}");
                     else if (strstr(id, "TIME:"))
                        opts = strdup("{ \"widgettype\": 11}");
                     else
                        opts = strdup("{}");
                     tell(eloDetail, "Info: No valuefact for '%s' adding default options [%s]", id, opts);
                  }
               }

               tableDashboardWidgets->setValue("WIDGETOPTS", opts);
               tableDashboardWidgets->store();

               tell(eloDebugWebSock, "Debug: Dashboard widget '%s' with [%s]", id, opts);
               free(opts);
            }
         }
      }
   }

   return done;
}

//***************************************************************************
// Perform Force Refresh
//***************************************************************************

int Daemon::performForceRefresh(json_t* obj, long client)
{
   json_t* oJson = json_object();
   dashboards2Json(oJson);
   pushOutMessage(oJson, "dashboards", client);

   performData(client, "init");

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
      bool record = getBoolFromJson(jObj, "record");

      tableValueFacts->clear();
      tableValueFacts->setValue("TYPE", type);
      tableValueFacts->setValue("ADDRESS", addr);

      if (!tableValueFacts->find())
         continue;

      tableValueFacts->clearChanged();

      if (isElementSet(jObj, "state"))
         tableValueFacts->setValue("STATE", state||record ? "A" : "D");
      if (isElementSet(jObj, "record"))
         tableValueFacts->setValue("RECORD", record ? "A" : "D");
      if (isElementSet(jObj, "usrtitle"))
         tableValueFacts->setValue("USRTITLE", getStringFromJson(jObj, "usrtitle"));
      if (isElementSet(jObj, "unit"))
         tableValueFacts->setValue("UNIT", getStringFromJson(jObj, "unit"));
      if (isElementSet(jObj, "groupid"))
         tableValueFacts->setValue("GROUPID", getIntFromJson(jObj, "groupid"));

      if (tableValueFacts->getChanges())
      {
         tableValueFacts->store();
         initSensorByFact(type, addr);
         tell(eloWebSock, "STORED valuefact for %s:%d", type, addr);
      }
   }

   json_t* oJson = json_object();
   valueFacts2Json(oJson, false);
   pushOutMessage(oJson, "valuefacts", client);
   updateSchemaConfTable();

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
            tell(eloAlways, "Error: Group (%d) not found!", id);
            continue;
         }

         tableGroups->clearChanged();
         tableGroups->setValue("NAME", name);

         if (tableGroups->getChanges())
         {
            tableGroups->store();
            tell(eloDebugWebSock, "STORED %d - name '%s'", id, name);
         }
      }
   }
   else if (strcmp(action, "add") == 0)
   {
      const char* name = getStringFromJson(oObject, "group");
      tell(eloWebSock, "Add group '%s'", name);

      tableGroups->clear();
      tableGroups->setValue("NAME", name);
      tableGroups->insert();
   }
   else if (strcmp(action, "del") == 0)
   {
      int id = getIntFromJson(oObject, "id");
      tell(eloWebSock, "Delete group '%d'", id);

      tableGroups->clear();
      tableGroups->setValue("ID", id);

      if (!tableGroups->find())
         tell(eloWebSock, "Group not exists, ignoring 'del' request");
      else
      {
         tell(eloWebSock, "Delete group %d", id);
         tableGroups->deleteWhere(" id = '%d'", id);
      }
   }

   performGroups(client);

   return replyResult(success, "Konfiguration gespeichert", client);
}

//***************************************************************************
// Perform Image Config
//***************************************************************************

#include "lib/base64.h"

int Daemon::performImageConfig(json_t* obj, long client)
{
   const char* action = getStringFromJson(obj, "action");

   if (strcmp(action, "delete") == 0)
   {
      const char* path = getStringFromJson(obj, "path");
      char* tmp {nullptr};
      asprintf(&tmp, "%s/%s", httpPath, path);
      tell(eloAlways, "Deleting image '%s'", tmp);
      removeFile(tmp);
      free(tmp);
   }
   else if (strcmp(action, "upload") == 0)
   {
      const char* name = getStringFromJson(obj, "name");
      const char* data = getStringFromJson(obj, "data");
      const char* p = strchr(data, ',');

      if (p)
      {
         // "data:image/png;base64,/9j/4AAQSkZJRg........."

         const char* mediaTpe = strchr(data, '/');
         mediaTpe++;
         char suffix[20];
         sprintf(suffix, "%.*s", (int)(strchr(mediaTpe, ';') - mediaTpe), mediaTpe);

         p++;

         std::string out;
         macaron::Base64::Decode(p, out);

         char* tmp {nullptr};
         asprintf(&tmp, "%s/img/user/%s.%s", httpPath, name, suffix);

         if (out.size())
         {
            storeToFile(tmp, out.c_str(), out.size());
            tell(eloWebSock, "Stored image to '%s' with %zu bytes", tmp, out.size());
         }

         free(tmp);
      }
   }
   else
   {
      return done;
   }

   json_t* oJson = json_array();
   images2Json(oJson);
   pushOutMessage(oJson, "images", client);

   return done;
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
      json_object_set_new(oDetail, "description", json_string(it.description));

      if (it.type == ctChoice || it.type == ctMultiSelect || it.type == ctBitSelect)
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

//***************************************************************************
// Config Choice to Json
//***************************************************************************

int Daemon::configChoice2json(json_t* obj, const char* name)
{
   if (strcmp(name, "eloquence") == 0)
   {
      json_t* oArray = json_array();

      for (int i = 0; Elo::eloquences[i]; i++)
         json_array_append_new(oArray, json_string(Elo::eloquences[i]));

      json_object_set_new(obj, "options", oArray);
   }
   else if (strcmp(name, "style") == 0)
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
   else if (strcmp(name, "background") == 0)
   {
      FileList options;
      int count {0};
      char* path {nullptr};

      json_array_append_new(obj, json_string(""));

      asprintf(&path, "%s/img/backgrounds", httpPath);

      if (getFileList(path, DT_REG, nullptr, true, &options, count) == success)
      {
         sortFileList(options);

         json_t* oArray = json_array();
         json_object_set_new(obj, "options", oArray);
         json_array_append_new(oArray, json_string(""));

         for (const auto& img : options)
         {
            char* imgPath {nullptr};
            asprintf(&imgPath, "%s/%s", img.path.c_str()+strlen(httpPath)+1, img.name.c_str());
            json_array_append_new(oArray, json_string(imgPath));
            free(imgPath);
         }
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
// Value Types 2 Json
//***************************************************************************

int Daemon::valueTypes2Json(json_t* obj)
{
   tableValueTypes->clear();

   for (int f = selectAllValueTypes->find(); f; f = selectAllValueTypes->fetch())
   {
      json_t* oData = json_object();
      json_array_append_new(obj, oData);

      json_object_set_new(oData, "type", json_string(tableValueTypes->getStrValue("TYPE")));
      json_object_set_new(oData, "title", json_string(tableValueTypes->getStrValue("TITLE")));
   }

   selectAllValueTypes->freeResult();

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
      std::string type = tableValueFacts->getStrValue("TYPE");
      asprintf(&key, "%s:0x%02lx", type.c_str(), tableValueFacts->getIntValue("ADDRESS"));
      json_t* oData = json_object();
      json_object_set_new(obj, key, oData);
      free(key);

      json_object_set_new(oData, "address", json_integer((ulong)tableValueFacts->getIntValue("ADDRESS")));
      json_object_set_new(oData, "type", json_string(type.c_str()));
      json_object_set_new(oData, "state", json_boolean(tableValueFacts->hasValue("STATE", "A")));
      json_object_set_new(oData, "record", json_boolean(tableValueFacts->hasValue("RECORD", "A")));
      json_object_set_new(oData, "name", json_string(tableValueFacts->getStrValue("NAME")));
      json_object_set_new(oData, "title", json_string(tableValueFacts->getStrValue("TITLE")));
      json_object_set_new(oData, "usrtitle", json_string(tableValueFacts->getStrValue("USRTITLE")));
      json_object_set_new(oData, "unit", json_string(tableValueFacts->getStrValue("UNIT")));
      json_object_set_new(oData, "rights", json_integer(tableValueFacts->getIntValue("RIGHTS")));
      json_object_set_new(oData, "options", json_integer(tableValueFacts->getIntValue("OPTIONS")));
      // #TODO check actor properties if dimmable ...
      json_object_set_new(oData, "dim", json_boolean(type == "DZL" || type == "HMB"));

      if (!tableValueFacts->getValue("CHOICES")->isNull())
         json_object_set_new(oData, "choices", json_string(tableValueFacts->getStrValue("CHOICES")));

      // widget in valuefacts only used or list view!

      json_t* jDefaults = json_object();
      widgetDefaults2Json(jDefaults, type.c_str(),
                          tableValueFacts->getStrValue("UNIT"),
                          tableValueFacts->getStrValue("NAME"),
                          tableValueFacts->getIntValue("ADDRESS"));

      json_object_set_new(oData, "widget", jDefaults);

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
// Dashboards 2 Json
//***************************************************************************

int Daemon::dashboards2Json(json_t* obj)
{
   tableDashboards->clear();

   for (int f = selectDashboards->find(); f; f = selectDashboards->fetch())
   {
      json_t* oDashboard = json_object();
      char* tmp {nullptr};
      asprintf(&tmp, "%ld", tableDashboards->getIntValue("ID"));
      json_object_set_new(obj, tmp, oDashboard);
      free(tmp);
      json_object_set_new(oDashboard, "title", json_string(tableDashboards->getStrValue("TITLE")));
      json_object_set_new(oDashboard, "symbol", json_string(tableDashboards->getStrValue("SYMBOL")));
      json_object_set_new(oDashboard, "order", json_integer(tableDashboards->getIntValue("ORDER")));

      json_t* oOpts = json_loads(tableDashboards->getStrValue("OPTS"), 0, nullptr);
      json_object_set_new(oDashboard, "options", oOpts);

      json_t* oWidgets = json_object();
      json_object_set_new(oDashboard, "widgets", oWidgets);

      tableDashboardWidgets->clear();
      tableDashboardWidgets->setValue("DASHBOARDID", tableDashboards->getIntValue("ID"));

      for (int w = selectDashboardWidgetsFor->find(); w; w = selectDashboardWidgetsFor->fetch())
      {
         char* key {nullptr};
         asprintf(&key, "%s:0x%02lx", tableDashboardWidgets->getStrValue("TYPE"), tableDashboardWidgets->getIntValue("ADDRESS"));
         json_t* oWidgetOpts = json_loads(tableDashboardWidgets->getStrValue("WIDGETOPTS"), 0, nullptr);
         json_object_set_new(oWidgets, key, oWidgetOpts);
         free(key);
      }

      selectDashboardWidgetsFor->freeResult();
   }

   selectDashboards->freeResult();

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
// Commands 2 Json
//***************************************************************************

int Daemon::commands2Json(json_t* obj)
{
   json_t* jCommand = json_object();
   json_array_append_new(obj, jCommand);
   json_object_set_new(jCommand, "title", json_string("Reset Peaks"));
   json_object_set_new(jCommand, "cmd", json_string("peaks"));
   json_object_set_new(jCommand, "description", json_string("Peaks zurücksetzen"));

   jCommand = json_object();
   json_array_append_new(obj, jCommand);
   json_object_set_new(jCommand, "title", json_string("Test Mail"));
   json_object_set_new(jCommand, "cmd", json_string("testmail"));
   json_object_set_new(jCommand, "description", json_string("Test Mail senden"));

   return done;
}

//***************************************************************************
// Perform Command
//***************************************************************************

int Daemon::performCommand(json_t* obj, long client)
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
   else if (what == "testmail")
   {
      performTestMail(obj, client);
   }
   else
      return fail;

   return replyResult(success, "success", client);
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

//***************************************************************************
// Sensor 2 Json
//***************************************************************************

int Daemon::sensor2Json(json_t* obj, const char* type, uint address)
{
   double peakMax {0.0};
   double peakMin {0.0};

   tablePeaks->clear();
   tablePeaks->setValue("TYPE", type); // table->getStrValue("TYPE"));
   tablePeaks->setValue("ADDRESS", (long)address); // table->getIntValue("ADDRESS"));

   if (tablePeaks->find())
   {
      peakMax = tablePeaks->getFloatValue("MAX");
      peakMin = tablePeaks->getFloatValue("MIN");
   }

   tablePeaks->reset();

   json_object_set_new(obj, "address", json_integer(address)); // (ulong)table->getIntValue("ADDRESS")));
   json_object_set_new(obj, "type", json_string(type)); //table->getStrValue("TYPE")));
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
   asprintf(&path, "%s/img", httpPath);

   if (getFileList(path, DT_REG, nullptr, true, &images, count) == success)
   {
      sortFileList(images);

      for (const auto& img : images)
      {
         char* imgPath {nullptr};
         asprintf(&imgPath, "%s/%s", img.path.c_str()+strlen(httpPath)+1, img.name.c_str());
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
