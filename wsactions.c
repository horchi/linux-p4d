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

#include <algorithm>

#include "lib/json.h"

#include "p4d.h"

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
// Perform WS Client Login / Logout
//***************************************************************************

int P4d::performLogin(json_t* oObject)
{
   long client = getLongFromJson(oObject, "client");
   const char* user = getStringFromJson(oObject, "user", "");
   const char* token = getStringFromJson(oObject, "token", "");
   json_t* aRequests = json_object_get(oObject, "requests");

   tableUsers->clear();
   tableUsers->setValue("USER", user);

   wsClients[(void*)client].user = user;
   wsClients[(void*)client].dataUpdates = false;

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
   cWebSock::setClientType((lws*)client, wsClients[(void*)client].type);

   //

   json_t* oJson = json_object();
   config2Json(oJson);
   pushOutMessage(oJson, "config", client);

   oJson = json_object();
   daemonState2Json(oJson);
   pushOutMessage(oJson, "daemonstate", client);

   // perform requests

   size_t index;
   json_t* oRequest;

   json_array_foreach(aRequests, index, oRequest)
   {
      const char* name = getStringFromJson(oRequest, "name");

      if (isEmpty(name))
         continue;

      tell(0, "Got request '%s'", name);

      if (strcmp(name, "data") == 0)
      {
         wsClients[(void*)client].dataUpdates = true;
         update(true, client);     // push the data ('init')
      }
      else if (wsClients[(void*)client].rights & urAdmin && strcmp(name, "syslog") == 0)
         performSyslog(client);
      else if (wsClients[(void*)client].rights & urSettings && strcmp(name, "configdetails") == 0)
         performConfigDetails(client);
      else if (wsClients[(void*)client].rights & urAdmin && strcmp(name, "userdetails") == 0)
         performUserDetails(client);
      else if (wsClients[(void*)client].rights & urAdmin && strcmp(name, "iosettings") == 0)
         performIoSettings(client);
      else if (wsClients[(void*)client].rights & urAdmin && strcmp(name, "groups") == 0)
         performGroups(client);
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

int P4d::performIoSettings(long client)
{
   if (client <= 0)
      return done;

   json_t* oJson = json_array();
   valueFacts2Json(oJson);
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
// Perform WS ChartData request
//***************************************************************************

std::vector<std::string> split(const std::string& str, char delim)
{
   std::vector<std::string> strings;
   size_t start;
   size_t end {0};

   while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
   {
      end = str.find(delim, start);
      strings.push_back(str.substr(start, end - start));
   }

   return strings;
}

int P4d::performChartData(json_t* oObject, long client)
{
   if (client <= 0)
      return done;

   int range = getIntFromJson(oObject, "range", 3);                // Anzahl der Tage
   time_t rangeStart = getLongFromJson(oObject, "start", 0);       // Start Datum (unix timestamp)
   const char* sensors = getStringFromJson(oObject, "sensors");    // Kommata getrennte Liste der Sensoren
   int widget = getIntFromJson(oObject, "widget", no);  //
   const char* id = getStringFromJson(oObject, "id", "");

   cDbStatement* select = widget ? selectSamplesRange60 : selectSamplesRange;

   if (isEmpty(sensors))
      sensors = chart1;

   tell(eloAlways, "Selecting chats data for '%s' ..", sensors);

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

      if (!active)
      {
         free(id);
         continue;
      }

      free(id);

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
   tell(eloAlways, ".. done");
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
         tell(0, "User not exists, ignoring 'resetpwd' request");
      else
      {
         tell(0, "Reset password of user '%s'", user);
         tableUsers->setValue("PASSWD", passwd);
         tableUsers->store();
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

   return done;
}

//***************************************************************************
// Store Configuration
//***************************************************************************

int P4d::storeConfig(json_t* obj, long client)
{
   const char* key;
   json_t* jValue;

   json_object_foreach(obj, key, jValue)
   {
      tell(3, "Debug: Storing config item '%s' with '%s'", key, json_string_value(jValue));
      setConfigItem(key, json_string_value(jValue));
   }

   readConfiguration();

   json_t* oJson = json_object();
   config2Json(oJson);
   pushOutMessage(oJson, "config", client);

   return done;
}

int P4d::storeIoSetup(json_t* array, long client)
{
   size_t index;
   json_t* jObj;

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

   performIoSettings(client);

   return done;
}

int P4d::storeGroups(json_t* oObject, long client)
{
   const char* action = getStringFromJson(oObject, "action");

   if (strcmp(action, "store") == 0)
   {
      json_t* array = json_object_get(oObject, "groups");
      json_t* jObj;
      size_t index;

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
         // tell(0, "Setting name of group (%d) to '%s' %d changes", id, name, tableGroups->getChanges());

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
      tableConfig->clear();
      tableConfig->setValue("OWNER", myName());
      tableConfig->setValue("NAME", it.name.c_str());

      if (tableConfig->find())
      {
         json_t* oDetail = json_object();
         json_array_append_new(obj, oDetail);

         json_object_set_new(oDetail, "name", json_string(tableConfig->getStrValue("NAME")));
         json_object_set_new(oDetail, "type", json_integer(it.type));
         json_object_set_new(oDetail, "value", json_string(tableConfig->getStrValue("VALUE")));
         json_object_set_new(oDetail, "category", json_string(it.category));
         json_object_set_new(oDetail, "title", json_string(it.title));
         json_object_set_new(oDetail, "descrtiption", json_string(it.description));
      }

      tableConfig->reset();
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

int P4d::valueFacts2Json(json_t* obj)
{
   tableValueFacts->clear();

   for (int f = selectAllValueFacts->find(); f; f = selectAllValueFacts->fetch())
   {
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
// Daemon Status 2 Json
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

//***************************************************************************
// Sensor 2 Json
//***************************************************************************

int P4d::sensor2Json(json_t* obj, cDbTable* table)
{
   double peak {0.0};

   tablePeaks->clear();
   tablePeaks->setValue("ADDRESS", table->getIntValue("ADDRESS"));
   tablePeaks->setValue("TYPE", table->getStrValue("TYPE"));

   json_object_set_new(obj, "widgettype", json_integer(
      getWidgetTypeOf(
         table->getStrValue("TYPE"),
         tableValueFacts->getStrValue("UNIT"),
         table->getIntValue("ADDRESS"))));

   if (tablePeaks->find())
      peak = tablePeaks->getFloatValue("MAX");

   tablePeaks->reset();

   json_object_set_new(obj, "address", json_integer((ulong)table->getIntValue("ADDRESS")));
   json_object_set_new(obj, "type", json_string(table->getStrValue("TYPE")));
   json_object_set_new(obj, "name", json_string(table->getStrValue("NAME")));

   if (!table->getValue("USRTITLE")->isEmpty())
      json_object_set_new(obj, "title", json_string(table->getStrValue("USRTITLE")));
   else
      json_object_set_new(obj, "title", json_string(table->getStrValue("TITLE")));

   json_object_set_new(obj, "unit", json_string(table->getStrValue("UNIT")));
   json_object_set_new(obj, "scalemax", json_integer(table->getIntValue("MAXSCALE")));
   json_object_set_new(obj, "rights", json_integer(table->getIntValue("RIGHTS")));

   json_object_set_new(obj, "peak", json_real(peak));


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
      return wtGauge;

   if (unit == "°" || unit == "%" || unit == "V" || unit == "A") // 'Volt/Ampere/Prozent/Temperatur
      return wtGauge;

   if (!unit.length())
      return wtSymbol;

   return wtValue;
}
