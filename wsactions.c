//***************************************************************************
// p4d / Linux - Heizungs Manager
// File wsactions.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 25.08.2020  Jörg Wendel
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <dirent.h>
#include <algorithm>

#include "lib/json.h"
#include "p4d.h"

//***************************************************************************
// Dispatch Client Requests
//***************************************************************************

int P4d::dispatchClientRequest()
{
   int status = fail;
   json_error_t error;
   json_t *oData, *oObject;

   cMyMutexLock lock(&messagesInMutex);

   while (!messagesIn.empty())
   {
      // dispatch message like
      //   => { "event" : "toggleio", "object" : { "address" : "122", "type" : "DO" } }

      tell(1, "DEBUG: Got '%s'", messagesIn.front().c_str());
      oData = json_loads(messagesIn.front().c_str(), 0, &error);

      // get the request

      Event event = cWebService::toEvent(getStringFromJson(oData, "event", "<null>"));
      long client = getLongFromJson(oData, "client");
      oObject = json_object_get(oData, "object");
      // int addr = getIntFromJson(oObject, "address");
      // const char* type = getStringFromJson(oObject, "type");

      // rights ...

      if (checkRights(client, event, oObject))
      {
         // dispatch client request

         tell(2, "Dispatch event %d '%s'", event, toName(event));

         switch (event)
         {
            case evLogin:          status = performLogin(oObject);                  break;
            case evLogout:         status = performLogout(oObject);                 break;
            case evGetToken:       status = performTokenRequest(oObject, client);   break;
            // case evToggleIo:      status = toggleIo(addr, type);                   break;
            // case evToggleIoNext:  status = toggleIoNext(addr);                     break;
            // case evToggleMode:    status = toggleOutputMode(addr);                 break;
            case evInitTables:     status = performInitTables(oObject, client);     break;
            case evStoreConfig:    status = storeConfig(oObject, client);           break;
            case evIoSetup:        status = performIoSettings(oObject, client);     break;
            case evStoreIoSetup:   status = storeIoSetup(oObject, client);          break;
            case evGroupConfig:    status = storeGroups(oObject, client);           break;
            case evChartData:      status = performChartData(oObject, client);      break;
            case evUserConfig:     status = performUserConfig(oObject, client);     break;
            case evChangePasswd:   status = performPasswChange(oObject, client);    break;
            case evResetPeaks:     status = resetPeaks(oObject, client);            break;
            case evMenu:           status = performMenu(oObject, client);           break;
            case evAlerts:         status = performAlerts(oObject, client);         break;
            case evSendMail:       status = performSendMail(oObject, client);       break;
            case evStoreAlerts:    status = storeAlerts(oObject, client);           break;
            case evStoreSchema:    status = storeSchema(oObject, client);           break;
            case evParEditRequest: status = performParEditRequest(oObject, client); break;
            case evParStore:       status = performParStore(oObject, client);       break;
            case evChartbookmarks: status = performChartbookmarks(client);             break;
            case evStoreChartbookmarks: status = storeChartbookmarks(oObject, client); break;

            default: tell(0, "Error: Received unexpected client request '%s' at [%s]",
                          toName(event), messagesIn.front().c_str());
         }
      }
      else
      {
         tell(0, "Insufficient rights to '%s' for user '%s'",
              getStringFromJson(oData, "event", "<null>"),
              wsClients[(void*)client].user.c_str());
      }

      json_decref(oData);      // free the json object
      messagesIn.pop();
   }

   return status;
}

bool P4d::checkRights(long client, Event event, json_t* oObject)
{
   uint rights = wsClients[(void*)client].rights;

   switch (event)
   {
      case evLogin:               return true;
      case evLogout:              return true;
      case evGetToken:            return true;
      // case evToggleIoNext:        return rights & urControl;
      // case evToggleMode:          return rights & urFullControl;
      case evStoreConfig:         return rights & urSettings;
      case evIoSetup:             return rights & urView;
      case evStoreIoSetup:        return rights & urSettings;
      case evGroupConfig:         return rights & urSettings;
      case evChartData:           return rights & urView;
      case evUserConfig:          return rights & urAdmin;
      case evChangePasswd:        return true;   // check will done in performPasswChange()
      case evResetPeaks:          return rights & urFullControl;
      case evMenu:                return rights & urView;
      case evAlerts:              return rights & urSettings;
      case evStoreAlerts:         return rights & urSettings;
      case evStoreSchema:         return rights & urSettings;
      case evParEditRequest:      return rights & urControl;
      case evParStore:            return rights & urControl;
      case evSendMail:            return rights & urSettings;
      case evChartbookmarks:      return rights & urView;
      case evStoreChartbookmarks: return rights & urSettings;
      case evInitTables:          return rights & urSettings;
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

int P4d::performWebSocketPing()
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

int P4d::replyResult(int status, const char* message, long client)
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
// Perform WS Client Login / Logout
//***************************************************************************

int P4d::performLogin(json_t* oObject)
{
   long client = getLongFromJson(oObject, "client");
   const char* user = getStringFromJson(oObject, "user", "");
   const char* token = getStringFromJson(oObject, "token", "");
   const char* page = getStringFromJson(oObject, "page", "");
   json_t* aRequests = json_object_get(oObject, "requests");

   tableUsers->clear();
   tableUsers->setValue("USER", user);

   wsClients[(void*)client].user = user;
   wsClients[(void*)client].page = page;

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

   tell(0, "Login of client 0x%x; user '%s'; type is %d", (unsigned int)client, user, wsClients[(void*)client].type);
   webSock->setClientType((lws*)client, wsClients[(void*)client].type);

   //

   json_t* oJson = json_object();
   config2Json(oJson);
   pushOutMessage(oJson, "config", client);

   oJson = json_object();
   daemonState2Json(oJson);
   pushOutMessage(oJson, "daemonstate", client);

   oJson = json_object();
   s3200State2Json(oJson);
   pushOutMessage(oJson, "s3200-state", client);

   // perform requests

   size_t index {0};
   json_t* oRequest {nullptr};

   json_array_foreach(aRequests, index, oRequest)
   {
      const char* name = getStringFromJson(oRequest, "name");

      if (isEmpty(name))
         continue;

      tell(0, "Got request '%s'", name);

      if (strcmp(name, "data") == 0)
         update(true, client);     // push the data ('init')
      else if (wsClients[(void*)client].rights & urAdmin && strcmp(name, "syslog") == 0)
         performSyslog(client);
      else if (wsClients[(void*)client].rights & urSettings && strcmp(name, "configdetails") == 0)
         performConfigDetails(client);
      else if (wsClients[(void*)client].rights & urAdmin && strcmp(name, "userdetails") == 0)
         performUserDetails(client);
      else if (wsClients[(void*)client].rights & urAdmin && strcmp(name, "iosettings") == 0)
         performIoSettings(nullptr, client);
      else if (wsClients[(void*)client].rights & urAdmin && strcmp(name, "groups") == 0)
         performGroups(client);
      else if (strcmp(name, "errors") == 0)
         performErrors(client);
      else if (strcmp(name, "menu") == 0)
         performMenu(0, client);
      else if (strcmp(name, "schema") == 0)
         performSchema(0, client);
      else if (strcmp(name, "alerts") == 0)
         performAlerts(0, client);
      else if (strcmp(name, "chartdata") == 0)
         performChartData(oRequest, client);
   }

   return done;
}

int P4d::performLogout(json_t* oObject)
{
   long client = getLongFromJson(oObject, "client");
   tell(0, "Logout of client 0x%x", (unsigned int)client);
   wsClients.erase((void*)client);
   return done;
}

//***************************************************************************
// Perform WS Token Request
//***************************************************************************

int P4d::performTokenRequest(json_t* oObject, long client)
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
// Perform WS Init Tables
//***************************************************************************

int P4d::performInitTables(json_t* oObject, long client)
{
   const char* action = getStringFromJson(oObject, "action");

   if (isEmpty(action))
      return replyResult(fail, "missing action", client);

   if (strcmp(action, "valuefacts") == 0)
   {
      initValueFacts();
      updateTimeRangeData();
   }
   else if (strcmp(action, "menu") == 0)
   {
      initMenu();
   }

   return replyResult(success, "... init abgeschlossen!", client);
}

//***************************************************************************
// Perform WS Syslog Request
//***************************************************************************

int P4d::performSyslog(long client)
{
   if (client <= 0)
      return done;

   json_t* oJson = json_object();
   const char* name = "/var/log/syslog";
   std::vector<std::string> lines;
   std::string result;

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
// Perform WS Config Data Request
//***************************************************************************

int P4d::performConfigDetails(long client)
{
   if (client <= 0)
      return done;

   json_t* oJson = json_array();
   configDetails2Json(oJson);
   pushOutMessage(oJson, "configdetails", client);

   return done;
}

//***************************************************************************
// Perform WS User Data Request
//***************************************************************************

int P4d::performUserDetails(long client)
{
   if (client <= 0)
      return done;

   json_t* oJson = json_array();
   userDetails2Json(oJson);
   pushOutMessage(oJson, "userdetails", client);

   return done;
}

//***************************************************************************
// Perform WS IO Setting Data Request
//***************************************************************************

int P4d::performIoSettings(json_t* oObject, long client)
{
   if (client <= 0)
      return done;

   bool filterActive = false;

   if (oObject)
      filterActive = getBoolFromJson(oObject, "filter", false);

   json_t* oJson = json_array();
   valueFacts2Json(oJson, filterActive);
   pushOutMessage(oJson, "valuefacts", client);

   return done;
}

//***************************************************************************
// Perform WS Groups Data Request
//***************************************************************************

int P4d::performGroups(long client)
{
   if (client <= 0)
      return done;

   json_t* oJson = json_array();
   groups2Json(oJson);
   pushOutMessage(oJson, "groups", client);

   return done;
}

//***************************************************************************
// Perform WS Error Data Request
//***************************************************************************

int P4d::performErrors(long client)
{
   if (client <= 0)
      return done;

   json_t* oJson = json_array();

   tableErrors->clear();

   for (int f = selectAllErrors->find(); f; f = selectAllErrors->fetch())
   {
      json_t* oData = json_object();
      json_array_append_new(oJson, oData);

      time_t t = std::max(std::max(tableErrors->getTimeValue("TIME1"), tableErrors->getTimeValue("TIME4")), tableErrors->getTimeValue("TIME2"));
      std::string strTime = l2pTime(t);
      uint duration {0};

      if (tableErrors->getValue("TIME2")->isNull())
         duration = time(0) - tableErrors->getTimeValue("TIME1");
      else
         duration = tableErrors->getTimeValue("TIME2") - tableErrors->getTimeValue("TIME1");

      json_object_set_new(oData, "state", json_string(tableErrors->getStrValue("STATE")));
      json_object_set_new(oData, "text", json_string(tableErrors->getStrValue("TEXT")));
      json_object_set_new(oData, "duration", json_integer(duration));
      json_object_set_new(oData, "time", json_string(strTime.c_str()));
   }

   selectAllErrors->freeResult();

   pushOutMessage(oJson, "errors", client);

   return done;
}

//***************************************************************************
// Perform WS Menu Request
//***************************************************************************

int P4d::performMenu(json_t* oObject, long client)
{
   if (client <= 0)
      return done;

   int parent {1};
   int last {0};
   char* title {nullptr};

   if (oObject)
      parent = getIntFromJson(oObject, "parent", 1);

   json_t* oJson = json_object();
   json_t* oArray = json_array();

   tableMenu->clear();
   tableMenu->setValue("CHILD", parent);

   if (selectMenuItemsByChild->find())
   {
      last = tableMenu->getIntValue("PARENT");
      title = strdup(tableMenu->getStrValue("title"));
   }

   selectMenuItemsByChild->freeResult();

   tableMenu->clear();
   tableMenu->setValue("PARENT", parent);

   for (int f = selectMenuItemsByParent->find(); f; f = selectMenuItemsByParent->fetch())
   {
      int type = tableMenu->getIntValue("TYPE");
      int address = tableMenu->getIntValue("ADDRESS");
      int child = tableMenu->getIntValue("CHILD");
      char* title = strdup(tableMenu->getStrValue("TITLE"));

      if (isEmpty(rTrim(title)))
      {
         free(title);
         continue;
      }

      if (title[strlen(title)-1] == ':')
         title[strlen(title)-1] = '\0';

      bool timeGroup = (type == mstGroup1 || type == mstGroup2) && strcmp(title, "Zeiten") == 0 && (child == 230 || child == 350 || child == 430 || child == 573);

      if (type == mstBus || type == mstReset)
         continue;

      // this 3 'special' addresses takes a long while and don't deliver any usefull data

      if (address == 9997 || address == 9998 || address == 9999)
         continue;

      updateParameter(tableMenu);

      if (!child && !address && tableMenu->getValue("VALUE")->isNull())
         continue;

      if (!timeGroup)
      {

         json_t* oData = json_object();
         json_array_append_new(oArray, oData);

         json_object_set_new(oData, "id", json_integer(tableMenu->getIntValue("ID")));
         json_object_set_new(oData, "parent", json_integer(tableMenu->getIntValue("PARENT")));
         json_object_set_new(oData, "child", json_integer(child));
         json_object_set_new(oData, "type", json_integer(type));
         json_object_set_new(oData, "address", json_integer(address));
         json_object_set_new(oData, "title", json_string(title));
         json_object_set_new(oData, "unit", json_string(tableMenu->getStrValue("UNIT")));

         if (type == mstMesswert || type == mstMesswert1)
            json_object_set_new(oData, "value", json_real(vaValues[address]));
         else
            json_object_set_new(oData, "value", json_string(tableMenu->getStrValue("VALUE")));

         if (type == mstPar || type == mstParSet || type == mstParSet1 || type == mstParSet2 ||
             type == mstParDig || type == mstParZeit)
            json_object_set_new(oData, "editable", json_boolean(true));
      }

      else
      {
         int baseAddr {0};

         switch (child)
         {
            case 230: baseAddr = 0x00 + (address * 7); break;   // Boiler 'n'
            case 350: baseAddr = 0x38 + (address * 7); break;   // Heizkreis 'n'
            case 430: baseAddr = 0xb6 + (address * 7); break;   // Puffer 'n'
               // case ???: baseAddr = 0xd2 + (address * 7); break    // Kessel
            case 573: baseAddr = 0xd9 + (address * 7); break;   // Zirkulation
         }

         // updateTimeRangeData();

         for (int wday = 0; wday < 7; wday++)
         {
            int trAddr = baseAddr + wday;
            char* dayTitle {nullptr};
            asprintf(&dayTitle, "Zeiten '%s'", toWeekdayName(wday));

            tableTimeRanges->clear();
            tableTimeRanges->setValue("ADDRESS", trAddr);

            json_t* oData = json_object();
            json_array_append_new(oArray, oData);

            json_object_set_new(oData, "id", json_integer(0));
            json_object_set_new(oData, "parent", json_integer(0));
            json_object_set_new(oData, "child", json_integer(0));
            json_object_set_new(oData, "type", json_integer(0));
            json_object_set_new(oData, "address", json_integer(0));
            json_object_set_new(oData, "title", json_string(dayTitle));
            json_object_set_new(oData, "unit", json_string(""));

            free(dayTitle);

            if (tableTimeRanges->find())
            {
               for (int n = 1; n < 5; n++)
               {
                  char* rTitle {nullptr};
                  char* value {nullptr};
                  char* from {nullptr};
                  char* to {nullptr};

                  asprintf(&rTitle, "Range %d", n);
                  asprintf(&from, "from%d", n);
                  asprintf(&to, "to%d", n);
                  asprintf(&value, "%s - %s", tableTimeRanges->getStrValue(from), tableTimeRanges->getStrValue(to));

                  json_t* oData = json_object();
                  json_array_append_new(oArray, oData);

                  json_object_set_new(oData, "id", json_integer(0));
                  json_object_set_new(oData, "parent", json_integer(0));
                  json_object_set_new(oData, "child", json_integer(0));
                  json_object_set_new(oData, "type", json_integer(0));
                  json_object_set_new(oData, "address", json_integer(trAddr));
                  json_object_set_new(oData, "title", json_string(rTitle));
                  json_object_set_new(oData, "unit", json_string(""));
                  json_object_set_new(oData, "value", json_string(value));

                  free(rTitle);
                  free(value);
                  free(from);
                  free(to);
               }

               tableTimeRanges->reset();
            }
         }
      }

      free(title);
   }

   selectMenuItemsByParent->freeResult();

   json_object_set_new(oJson, "items", oArray);
   json_object_set_new(oJson, "parent", json_integer(parent));
   json_object_set_new(oJson, "last", json_integer(last));
   json_object_set_new(oJson, "title", json_string(title));

   pushOutMessage(oJson, "menu", client);

   return done;
}

//***************************************************************************
// Perform WS Scehma Data
//***************************************************************************

int P4d::performSchema(json_t* oObject, long client)
{
   if (client <= 0)
      return done;

   json_t* oArray = json_array();

   tableSchemaConf->clear();

   for (int f = selectAllSchemaConf->find(); f; f = selectAllSchemaConf->fetch())
   {
      tableValueFacts->clear();
      tableValueFacts->setValue("ADDRESS", tableSchemaConf->getIntValue("ADDRESS"));
      tableValueFacts->setValue("TYPE", tableSchemaConf->getStrValue("TYPE"));

      if (!tableSchemaConf->hasValue("TYPE", "UC") && (!tableValueFacts->find() || !tableValueFacts->hasValue("STATE", "A")))
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
   update(true, client);     // push the data ('init')

   return done;
}

//***************************************************************************
// Store Schema
//***************************************************************************

int P4d::storeSchema(json_t* oObject, long client)
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
         tell(0, "Warning, Ignoring properties of %s:0x%x due to field limit of %d bytes",
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
// Perform WS Sensor Alert Request
//***************************************************************************

int P4d::performAlerts(json_t* oObject, long client)
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
// Perform Send Mail
//***************************************************************************

int P4d::performSendMail(json_t* oObject, long client)
{
   int alertid = getIntFromJson(oObject, "alertid", na);

   if (alertid != na)
      return performAlertTestMail(alertid, client);

   const char* subject = getStringFromJson(oObject, "subject");
   const char* body = getStringFromJson(oObject, "body");

   tell(eloDetail, "Test mail requested with: '%s/%s'", subject, body);

   if (isEmpty(mailScript))
      return replyResult(fail, "missing mail script", client);

   if (!fileExists(mailScript))
      return replyResult(fail, "mail script not found", client);

   if (isEmpty(stateMailTo))
      return replyResult(fail, "missing receiver", client);

   if (sendMail(stateMailTo, subject, body, "text/plain") != success)
      return replyResult(fail, "send failed", client);

   return replyResult(success, "mail sended", client);
}

int P4d::performAlertTestMail(int id, long client)
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
// Perform WS Parameter Edit Request
//***************************************************************************

int P4d::performParEditRequest(json_t* oObject, long client)
{
   if (client <= 0)
      return done;

   int id = getIntFromJson(oObject, "id", na);

   tableMenu->clear();
   tableMenu->setValue("ID", id);

   if (!tableMenu->find())
   {
      tell(0, "Info: Id %d for 'pareditrequest' not found!", id);
      return fail;
   }

   int type = tableMenu->getIntValue("TYPE");
   unsigned int address = tableMenu->getIntValue("ADDRESS");
   const char* title = tableMenu->getStrValue("TITLE");

   tableMenu->reset();
   sem->p();

   ConfigParameter p(address);

   if (request->getParameter(&p) == success)
   {
      cRetBuf value = ConfigParameter::toNice(p.value, type);

      json_t* oJson = json_object();
      json_object_set_new(oJson, "id", json_integer(id));
      json_object_set_new(oJson, "type", json_integer(type));
      json_object_set_new(oJson, "address", json_integer(address));
      json_object_set_new(oJson, "title", json_string(title));
      json_object_set_new(oJson, "unit", json_string(type == mstParZeit ? "Uhr" : p.unit));
      json_object_set_new(oJson, "value", json_string(value));
      json_object_set_new(oJson, "def", json_integer(p.def));
      json_object_set_new(oJson, "min", json_integer(p.min));
      json_object_set_new(oJson, "max", json_integer(p.max));
      json_object_set_new(oJson, "digits", json_integer(p.digits));

      pushOutMessage(oJson, "pareditrequest", client);
   }

   sem->v();

   return done;
}

//***************************************************************************
// Perform WS Parameter Store
//***************************************************************************

int P4d::performParStore(json_t* oObject, long client)
{
   if (client <= 0)
      return done;

   json_t* oJson = json_object();
   int status {fail};
   int id = getIntFromJson(oObject, "id", na);
   const char* value = getStringFromJson(oObject, "value");

   tableMenu->clear();
   tableMenu->setValue("ID", id);

   if (!tableMenu->find())
   {
      replyResult(fail, "Parameter not found", client);
      tell(0, "Info: Id %d for 'parstore' not found!", id);
   }
   else
   {
      int parent = tableMenu->getIntValue("PARENT");
      int type = tableMenu->getIntValue("TYPE");
      unsigned int address = tableMenu->getIntValue("ADDRESS");
      ConfigParameter p(address);

      if ((status = ConfigParameter::toValue(value, type, p.value)) == success)
      {
         tell(eloAlways, "Storing value '%s/%d' for parameter at address 0x%x", value, p.value, address);
         sem->p();

         if ((status = request->setParameter(&p)) == success)
         {
            tableMenu->setValue("VALUE", ConfigParameter::toNice(p.value, type));
            tableMenu->setValue("UNIT", p.unit);
            tableMenu->update();
            json_object_set_new(oJson, "parent", json_integer(parent));
            sem->v();

            replyResult(status, "Parameter gespeichert", client);
            return performMenu(oJson, client);
         }

         sem->v();
         tell(eloAlways, "Set of parameter failed, error %d", status);

         if (status == P4Request::wrnNonUpdate)
            replyResult(status, "Value identical, ignoring request", client);
         else if (status == P4Request::wrnOutOfRange)
            replyResult(status, "Value ot of range", client);
         else
            replyResult(status, "Serial communication error", client);
      }
      else
      {
         replyResult(status, "Value format error", client);
         tell(eloAlways, "Set of parameter failed, wrong format");
      }

      tableMenu->reset();
   }

   return done;
}

//***************************************************************************
// Perform WS ChartData request
//***************************************************************************

int P4d::performChartData(json_t* oObject, long client)
{
   if (client <= 0)
      return done;

   int range = getIntFromJson(oObject, "range", 3);                // Anzahl der Tage
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
      tell(0, "storing sensors '%s' for chart", sensors);
      setConfigItem("chart", sensors);
      getConfigItem("chart", chartSensors);
   }
   else if (isEmpty(sensors))
      sensors = chartSensors;

   tell(eloDebug, "Selecting chats data for '%s' ..", sensors);

   auto sList = split(sensors, ',');

   json_t* oMain = json_object();
   json_t* oJson = json_array();

   if (!rangeStart)
      rangeStart = time(0) - (range*tmeSecondsPerDay);

   rangeFrom.setValue(rangeStart);
   rangeTo.setValue(rangeStart + (range*tmeSecondsPerDay));

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
            json_object_set_new(oRow, "y", json_real(avgValue.getFloatValue()));
      }

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
// Store User Configuration
//***************************************************************************

int P4d::performUserConfig(json_t* oObject, long client)
{
   if (client <= 0)
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

int P4d::performPasswChange(json_t* oObject, long client)
{
   if (client <= 0)
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
// Reset Peaks
//***************************************************************************

int P4d::resetPeaks(json_t* obj, long client)
{
   tablePeaks->truncate();
   setConfigItem("peakResetAt", l2pTime(time(0)).c_str());

   json_t* oJson = json_object();
   config2Json(oJson);
   pushOutMessage(oJson, "config", client);

   return replyResult(success, "Peaks zurückgesetzt", client);
}

//***************************************************************************
// Store Configuration
//***************************************************************************

int P4d::storeConfig(json_t* obj, long client)
{
   const char* key {0};
   json_t* jValue {nullptr};
   int oldWebPort = webPort;
   char* oldStyle {nullptr};

   getConfigItem("style", oldStyle, "");

   json_object_foreach(obj, key, jValue)
   {
      tell(3, "Debug: Storing config item '%s' with '%s'", key, json_string_value(jValue));
      setConfigItem(key, json_string_value(jValue));
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

   readConfiguration();

   json_t* oJson = json_object();
   config2Json(oJson);
   pushOutMessage(oJson, "config", client);

   if (oldWebPort != webPort)
      replyResult(success, "Konfiguration gespeichert. Web Port geändert, bitte p4d neu Starten!", client);
   else if (strcmp(name, oldStyle) != 0)
      replyResult(success, "Konfiguration gespeichert. Das Farbschema wurde geändert, mit STRG-Umschalt-r neu laden!", client);
   else
      replyResult(success, "Konfiguration gespeichert", client);

   free(oldStyle);

   return done;
}

int P4d::storeIoSetup(json_t* array, long client)
{
   size_t index {0};
   json_t* jObj {nullptr};

   json_array_foreach(array, index, jObj)
   {
      int addr = getIntFromJson(jObj, "address");
      const char* type = getStringFromJson(jObj, "type");
      int state = getIntFromJson(jObj, "state");
      const char* usrTitle = getStringFromJson(jObj, "usrtitle", "");
      int maxScale = getIntFromJson(jObj, "scalemax");

      tableValueFacts->clear();
      tableValueFacts->setValue("ADDRESS", addr);
      tableValueFacts->setValue("TYPE", type);

      if (!tableValueFacts->find())
         continue;

      tableValueFacts->clearChanged();
      tableValueFacts->setValue("STATE", state ? "A" : "D");
      tableValueFacts->setValue("USRTITLE", usrTitle);

      if (maxScale >= 0)
         tableValueFacts->setValue("MAXSCALE", maxScale);

      if (tableValueFacts->getChanges())
      {
         tableValueFacts->store();
         tell(2, "STORED %s:%d - usrtitle: '%s'; scalemax: %d; state: %d", type, addr, usrTitle, maxScale, state);
      }

      tell(3, "Debug: %s:%d - usrtitle: '%s'; scalemax: %d; state: %d", type, addr, usrTitle, maxScale, state);
   }

   updateSchemaConfTable();
   performIoSettings(nullptr, client);

   return replyResult(success, "Konfiguration gespeichert", client);
}

int P4d::storeGroups(json_t* oObject, long client)
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
// Store Sensor Alerts
//***************************************************************************

int P4d::storeAlerts(json_t* oObject, long client)
{
   const char* action = getStringFromJson(oObject, "action");

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

         tableSensorAlert->clear();

         if (id != na)
         {
            tableSensorAlert->setValue("ID", id);

            if (!tableSensorAlert->find())
               continue;
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
// Chart Bookmarks
//***************************************************************************

int P4d::storeChartbookmarks(json_t* array, long client)
{
   char* bookmarks = json_dumps(array, JSON_REAL_PRECISION(4));
   setConfigItem("chartBookmarks", bookmarks);
   free(bookmarks);

   performChartbookmarks(client);

   return done; // replyResult(success, "Bookmarks gespeichert", client);
}

int P4d::performChartbookmarks(long client)
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
// Config 2 Json Stuff
//***************************************************************************

int P4d::config2Json(json_t* obj)
{
   for (const auto& it : configuration)
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

int P4d::configDetails2Json(json_t* obj)
{
   for (const auto& it : configuration)
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

      if (it.type == ctChoice)
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

int P4d::configChoice2json(json_t* obj, const char* name)
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
         }

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
         }

         json_object_set_new(obj, "options", oArray);
      }

      free(path);
   }

   return done;
}

//***************************************************************************
// User Details 2 Json
//***************************************************************************

int P4d::userDetails2Json(json_t* obj)
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

int P4d::valueFacts2Json(json_t* obj, bool filterActive)
{
   tableValueFacts->clear();

   for (int f = selectAllValueFacts->find(); f; f = selectAllValueFacts->fetch())
   {
      if (filterActive && !tableValueFacts->hasValue("STATE", "A"))
         continue;

      json_t* oData = json_object();
      json_array_append_new(obj, oData);

      json_object_set_new(oData, "address", json_integer((ulong)tableValueFacts->getIntValue("ADDRESS")));
      json_object_set_new(oData, "type", json_string(tableValueFacts->getStrValue("TYPE")));
      json_object_set_new(oData, "state", json_integer(tableValueFacts->hasValue("STATE", "A")));
      json_object_set_new(oData, "name", json_string(tableValueFacts->getStrValue("NAME")));
      json_object_set_new(oData, "title", json_string(tableValueFacts->getStrValue("TITLE")));
      json_object_set_new(oData, "usrtitle", json_string(tableValueFacts->getStrValue("USRTITLE")));
      json_object_set_new(oData, "unit", json_string(tableValueFacts->getStrValue("UNIT")));
      json_object_set_new(oData, "scalemax", json_integer(tableValueFacts->getIntValue("MAXSCALE")));
      json_object_set_new(oData, "value", json_real(tableValueFacts->getFloatValue("VALUE")));
   }

   selectAllValueFacts->freeResult();

   return done;
}

//***************************************************************************
// Groups 2 Json
//***************************************************************************

int P4d::groups2Json(json_t* obj)
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
// Status 2 Json
//***************************************************************************

int P4d::daemonState2Json(json_t* obj)
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

int P4d::s3200State2Json(json_t* obj)
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

int P4d::sensor2Json(json_t* obj, cDbTable* table)
{
   double peakMax {0.0};
   double peakMin {0.0};

   tablePeaks->clear();
   tablePeaks->setValue("ADDRESS", table->getIntValue("ADDRESS"));
   tablePeaks->setValue("TYPE", table->getStrValue("TYPE"));

   json_object_set_new(obj, "widgettype", json_integer(
      getWidgetTypeOf(
         table->getStrValue("TYPE"),
         tableValueFacts->getStrValue("UNIT"),
         table->getIntValue("ADDRESS"))));

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
   json_object_set_new(obj, "scalemax", json_integer(table->getIntValue("MAXSCALE")));
   json_object_set_new(obj, "rights", json_integer(table->getIntValue("RIGHTS")));

   json_object_set_new(obj, "peak", json_real(peakMax));
   json_object_set_new(obj, "peakmin", json_real(peakMin));

   return done;
}

//***************************************************************************
// Get Image Of
//***************************************************************************

const char* P4d::getImageOf(const char* title, const char* usrtitle, int value)
{
   const char* imagePath = "unknown.jpg";

   if (strcasestr(title, "Pump") || strcasestr(usrtitle, "Pump"))
      imagePath = value ? "img/icon/pump-on.gif" : "img/icon/pump-off.png";
   else if (strcasestr(title, "Steckdose"))
      imagePath = value ? "img/icon/plug-on.png" : "img/icon/plug-off.png";
   else if (strcasestr(title, "UV-C"))
      imagePath = value ? "img/icon/uvc-on.png" : "img/icon/uvc-off.png";
   else if (strcasestr(title, "Licht"))
      imagePath = value ? "img/icon/light-on.png" : "img/icon/light-off.png";
   else if (strcasestr(title, "Shower") || strcasestr(title, "Dusche"))
      imagePath = value ? "img/icon/shower-on.png" : "img/icon/shower-off.png";
   else
      imagePath = value ? "img/icon/boolean-on.png" : "img/icon/boolean-off.png";

   return imagePath;
}

bool P4d::fileExistsAtWeb(const char* file)
{
   static char path[512];
   sprintf(path, "%s/%s", httpPath, file);
   return fileExists(path);
}

const char* P4d::getStateImage(int state)
{
   bool stateAni {true};

   if (state <= 0)
      return "img/state/state-error.gif";
   else if (state == 1)
      return stateAni && fileExistsAtWeb("img/state/ani/state-fireoff.gif") ? "img/state/ani/state-fireoff.gif" : "img/state/state-fireoff.gif";
   else if (state == 2)
      return stateAni && fileExistsAtWeb("img/state/ani/state-heatup.gif") ? "img/state/ani/state-heatup.gif" : "img/state/state-heatup.gif";
   else if (state == 3)
      return stateAni && fileExistsAtWeb("img/state/ani/state-fire.gif") ? "img/state/ani/state-fire.gif" : "img/state/state-fire.gif";
   else if (state == 4)
      return stateAni && fileExistsAtWeb("img/state/ani/state-firehold.gif") ? "img/state/ani/state-firehold.gif" : "img/state/state-firehold.gif";
   else if (state == 5)
      return stateAni && fileExistsAtWeb("img/state/ani/state-fireoff.gif") ? "img/state/ani/state-fireoff.gif" : "img/state/state-fireoff.gif";
   else if (state == 6)
      return stateAni && fileExistsAtWeb("img/state/ani/state-dooropen.gif") ? "img/state/ani/state-dooropen.gif" : "img/state/state-dooropen.gif";
   else if (state == 7)
      return stateAni && fileExistsAtWeb("img/state/ani/state-preparation.gif") ? "img/state/ani/state-preparation.gif" : "img/state/state-preparation.gif";
   else if (state == 8)
      return stateAni && fileExistsAtWeb("img/state/ani/state-warmup.gif") ? "img/state/ani/state-warmup.gif" : "img/state/state-warmup.gif";
   else if (state == 9)
      return stateAni && fileExistsAtWeb("img/state/ani/state-heatup.gif") ? "img/state/ani/state-heatup.gif" : "img/state/state-heatup.gif";
   else if (state == 15 || state == 70 || state == 69)
      return stateAni && fileExistsAtWeb("img/state/ani/state-clean.gif") ? "img/state/ani/state-clean.gif" : "img/state/state-clean.gif";
   else if ((state >= 10 && state <= 14) || state == 35 || state == 16)
      return stateAni && fileExistsAtWeb("img/state/ani/state-wait.gif") ? "img/state/ani/state-wait.gif" : "img/state/state-wait.gif";
   else if (state == 60 || state == 61  || state == 72)
      return stateAni && fileExistsAtWeb("img/state/ani/state-shfire.gif") ? "img/state/ani/state-shfire.gif" : "img/state/state-shfire.gif";

   static char buffer[100];
   sprintf(buffer, "img/type/heating-%s.png", heatingType);
   return buffer;
}

P4d::WidgetType P4d::getWidgetTypeOf(std::string type, std::string unit, uint address)
{
   if (type == "DI" || type == "DO")
      return wtSymbol;

   if (type == "UD" && address == udState)
      return wtSymbol;

   if (type == "UD")
      return wtText;

   if (type == "AO")
      return wtChart;

   if (unit == "°C" || unit == "°" || unit == "%" || unit == "V" || unit == "A") // 'Volt/Ampere/Prozent/Temperatur
      return wtChart;

   if (!unit.length())
      return wtSymbol;

   return wtValue;
}
