//***************************************************************************
// Automation Control
// File deconz.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 15.12.2021  Jörg Wendel
//***************************************************************************

#include "lib/curl.h"
#include "lib/json.h"

#include "websock.h"
#include "daemon.h"
#include "deconz.h"

cMyMutex Deconz::mapsMutex;

//***************************************************************************
// API documentation:
//    https://dresden-elektronik.github.io/deconz-rest-doc
//    https://dresden-elektronik.github.io/deconz-rest-doc/endpoints/lights/#example-request-data
//***************************************************************************

//***************************************************************************
// Problem, API-Key ist nach deCONZ Restart wieder weg
//  -> deCONZ trägt ihn nicht in die Tabelle ein
//  ?? Mache ich nich einen Fehler bei der Anmeldung ?
// Workaround mauell eintragen:
//   sqlite3 zll.db
//     insert into auth (apikey,createdate,lastusedate,devicetype,useragent) values('372245FAAA','2021-12-25T07:23:29','2021-12-25T07:23:29','homectrld','homectrld');
//   Dabei den Key (hier 372245FAAA) und ggf. das Datum anpassen!
//***************************************************************************

//***************************************************************************
// Deconz
//***************************************************************************

Deconz::Deconz()
{
}

Deconz::~Deconz()
{
   exitWsClient();
}

//***************************************************************************
// Init / Exit
//***************************************************************************

int Deconz::init(Daemon* parent, cDbConnection* connection)
{
   int status {success};

   daemon = parent;
   cCurl::create();      // create global cCurl instance
   curl.init();

   tableDeconzLights = new cDbTable(connection, "deconzl");
   if (tableDeconzLights->open() != success) return fail;

   tableDeconzSensors = new cDbTable(connection, "deconzs");
   if (tableDeconzSensors->open() != success) return fail;

   // ------------------

   selectLightByUuid = new cDbStatement(tableDeconzLights);

   selectLightByUuid->build("select ");
   selectLightByUuid->bindAllOut();
   selectLightByUuid->build(" from %s where ", tableDeconzLights->TableName());
   selectLightByUuid->bind("UUID", cDBS::bndIn | cDBS::bndSet);

   status += selectLightByUuid->prepare();

   // ------------------

   selectSensorByUuid = new cDbStatement(tableDeconzSensors);

   selectSensorByUuid->build("select ");
   selectSensorByUuid->bindAllOut();
   selectSensorByUuid->build(" from %s where ", tableDeconzSensors->TableName());
   selectSensorByUuid->bind("UUID", cDBS::bndIn | cDBS::bndSet);

   status += selectSensorByUuid->prepare();

   return status;
}

int Deconz::exit()
{
   curl.exit();
   cCurl::destroy();

   delete tableDeconzLights;  tableDeconzLights = nullptr;
   delete tableDeconzSensors; tableDeconzSensors = nullptr;
   delete selectLightByUuid;  selectLightByUuid = nullptr;
   delete selectSensorByUuid; selectSensorByUuid = nullptr;

   return success;
}

//***************************************************************************
// Query Api Key
//***************************************************************************

int Deconz::queryApiKey(std::string& result)
{
   json_t* jArray {nullptr};

   char* data {nullptr};
   asprintf(&data, "{ \"devicetype\": \"%s\" }", TARGET);

   if (post(jArray, "api", data) != success)
   {
      free(data);
      return fail;
   }

   free(data);
   json_t* jData = json_array_get(jArray, 0);
   const char* status {nullptr};
   json_t* jItem {nullptr};

   if (!jData)
      return fail;

   json_object_foreach(jData, status, jItem)
   {
      if (strcmp(status, "success") != 0)
      {
         result = getStringFromJson(jItem, "description");
         return fail;
      }

      result = getStringFromJson(jItem, "username");
   }

   json_decref(jArray);

   return success;
}

//***************************************************************************
// Init Devices
//***************************************************************************

int Deconz::initDevices()
{
   json_t* jData {nullptr};

   if (query(jData, "sensors", apiKey.c_str()) != success)
      return fail;

   processDevices(jData, "sensor");
   json_decref(jData);

   if (query(jData, "lights", apiKey.c_str()) != success)
      return fail;

   processDevices(jData, "light");
   json_decref(jData);

   initWsClient();

   return done;
}

//***************************************************************************
// Process Devices
//***************************************************************************

int Deconz::processDevices(json_t* jData, std::string kind)
{
   const char* sid {nullptr};   // sensor id
   json_t* jItem {nullptr};

   json_object_foreach(jData, sid, jItem)
   {
      // "11": { "hascolor":false,"lastannounced":"2021-05-10T03:02:21Z","lastseen":"2021-12-16T17:18Z","manufacturername":"IKEA of Sweden","modelid":"TRADFRI bulb E14 W op/ch 400lm","name":"wandvorn","state":{"alert":"none","bri":125,"on":false,"reachable":true},"swversion":"1.2.214","type":"Dimmable light","uniqueid":"90:fd:9f:ff:fe:0f:76:9b-01"}
      // "12": { "config": { "battery": 100, "on": true, "reachable": true, "temperature": 2000 },
      //          "ep": 1, "lastseen": "2021-12-17T14:32Z", "manufacturername": "LUMI", "mode": 1, "modelid": "lumi.sensor_switch.aq2", "name": "swflur2",
      //          "state": { "buttonevent": 1002, "lastupdated": "2021-12-17T06:08:11.192" }, "type": "ZHASwitch", "uniqueid": "00:15:8d:00:01:e8:32:a1-01-0006" },
      // "20": {"colorcapabilities":0,"ctmax":65535,"ctmin":0,"etag":"7429bb9cf02aa4da5588bc2c2e517c39","hascolor":true,"lastannounced":null,"lastseen":"2022-01-10T16:45Z",
      //          "state":{"alert":"none","bri":254,"colormode":"ct","ct":250,"on":false,"reachable":true},
      //          "swversion":"2.0.022","type":"Color temperature light","uniqueid":"14:b4:57:ff:fe:7e:7a:57-01"}

      char* type {nullptr};
      asprintf(&type, "DZ%s", kind == "sensor" ? "S" : "L");

      std::string dzType = getStringFromJson(jItem, "type", "");
      const char* uuid = getStringFromJson(jItem, "uniqueid");

      cDbTable* table = kind == "light" ? tableDeconzLights : tableDeconzSensors;
      cDbStatement* select = kind == "light" ? selectLightByUuid : selectSensorByUuid;

      table->clear();
      table->setValue("UUID", uuid);

      if (!select->find())
      {
         table->setValue("TYPE", type);
         table->insert();

         tell(eloDebugDeconz, "DECONZ: Added new '%s', uuid '%s'", kind.c_str(), uuid);

         table->clear();
         table->setValue("UUID", uuid);
         select->find();
      }

      uint address = table->getIntValue("ADDRESS");

      if (getObjectFromJson(jItem, "ctmin"))
      {
         json_t* jOpt = json_object();
         json_object_set_new(jOpt, "ctmin", json_integer(getIntFromJson(jItem, "ctmin")));
         json_object_set_new(jOpt, "ctmax", json_integer(getIntFromJson(jItem, "ctmax")));
         char* p = json_dumps(jOpt, JSON_REAL_PRECISION(4));
         table->setValue("OPTIONS", p);
         free(p);
         json_decref(jOpt);
      }

      table->setValue("KIND", kind.c_str());
      table->setValue("NAME", getStringFromJson(jItem, "name"));
      table->setValue("DZTYPE", dzType.c_str());
      table->setValue("MANUFACTURER", getStringFromJson(jItem, "manufacturername"));
      table->setValue("MODEL", getStringFromJson(jItem, "modelid"));
      table->store();

      {
         cMyMutexLock lock(&mapsMutex);

         if (kind == "light")
            lights[uuid] = address;
         else
            sensors[uuid] = address;
      }

      tell(eloDebugDeconz, "Found '%s' %d with uuid '%s'", kind.c_str(), address, uuid);

      const char* unit{""};
      int so {Daemon::soNone};

      if (kind == "sensor")
      {
         if (dzType == "ZHAPresence")
            unit = "mov";
         else if (dzType == "ZHATemperature")
            unit = "°C";
         else if (dzType == "ZHAHumidity")
            unit = "%";
         else if (dzType == "ZHAPressure")
            unit = "hPa";
         else if (dzType == "ZHALightLevel")
            unit = "lx";
      }
      else if (kind == "light")
      {
         so = Daemon::soSwitch;

         if (strcasestr(dzType.c_str(), "color"))
            so += Daemon::soColor + Daemon::soDim; // assume all color lights are dimmable
         if (strcasestr(dzType.c_str(), "dim"))
            so += Daemon::soDim;
      }

      daemon->addValueFact(address, type, 1, getStringFromJson(jItem, "name"), unit, ""/*title*/,
                           cWebService::urControl, nullptr, (Daemon::SensorOptions)so);

      int battery = getIntByPath(jItem, "config/battery", -1);

      // "state": { "alert": "none", "bri": 125, "on": false, "reachable": true }, }

      if (getObjectFromJson(jItem, "state"))
      {
         json_t* jsData = json_object();
         json_object_set_new(jsData, "type", json_string(type));
         json_object_set_new(jsData, "address", json_integer(address));

         if (battery != na)
            json_object_set_new(jsData, "battery", json_integer(battery));

         if (kind == "sensor")
         {
            if (getObjectByPath(jItem, "state/temperature"))
               json_object_set_new(jsData, "value", json_real(getIntByPath(jItem, "state/temperature")/100.0));

            if (getObjectByPath(jItem, "state/pressure"))
               json_object_set_new(jsData, "value", json_real((double)getIntByPath(jItem, "state/pressure")));

            if (getObjectByPath(jItem, "state/humidity"))
               json_object_set_new(jsData, "value", json_real(getIntByPath(jItem, "state/humidity")/100.0));

            // if (getObjectByPath(jItem, "state/buttonevent"))
            //    json_object_set_new(jsData, "btnevent", json_integer(getIntByPath(jItem, "state/buttonevent")));

            // if (getObjectByPath(jItem, "state/presence"))
            //    json_object_set_new(jsData, "presence", json_boolean(getBoolByPath(jItem, "state/presence")));
         }
         else if (kind == "light")
         {
            json_object_set_new(jsData, "state", json_boolean(getBoolByPath(jItem, "state/on")));

            if (getObjectByPath(jItem, "state/bri"))
            {
               int bri = getIntByPath(jItem, "state/bri") / 255.0 * 100.0;
               json_object_set_new(jsData, "bri", json_integer(bri));
            }

            if (getObjectByPath(jItem, "state/hue"))
            {
               int hue = getIntByPath(jItem, "state/hue") / 65535.0 * 360.0;
               json_object_set_new(jsData, "hue", json_integer(hue));
            }

            if (getObjectByPath(jItem, "state/sat"))
            {
               int sat = getIntByPath(jItem, "state/sat") / 255.0 * 100.0;
               json_object_set_new(jsData, "sat", json_integer(sat));
            }
         }

         if (dzType != "ZHASwitch")  // don't trigger switches !!!
         {
            char* p = json_dumps(jsData, JSON_REAL_PRECISION(4));

            {
               cMyMutexLock lock(&messagesInMutex);
               messagesIn.push(p);
            }

            free(p);
         }

         json_decref(jsData);

      }

      free(type);
   }

   return done;
}

//***************************************************************************
// Toggle
//***************************************************************************

int Deconz::toggle(const char* type, uint address, bool state, int bri, int transitiontime)
{
   json_t* jArray {nullptr};
   std::string result;
   std::string uuid;

   {
      cMyMutexLock lock(&mapsMutex);

      for (const auto& light : lights)
      {
         if (light.second == address)
         {
            uuid = light.first;
            break;
         }
      }
   }

   if (bri != na && bri < 5)
      state = false;

   json_t* jObj = json_object();
   json_object_set_new(jObj, "on", json_boolean(state));

   if (transitiontime != na)
      json_object_set_new(jObj, "transitiontime", json_integer(transitiontime));

   if (bri != na && state)
   {
      int dim = (int)((255 / 100.0) * bri);
      json_object_set_new(jObj, "bri", json_integer(dim));
   }

   int status {success};

   if ((status = put(jArray, uuid.c_str(), jObj)) == success)
      status = checkResult(jArray);

   if (status == success)
      tell(eloDeconz, "Toggled '%s' to %s", uuid.c_str(), state ? "on" : "off");

   json_decref(jArray);

   return status;
}

//***************************************************************************
// Color
//***************************************************************************

int Deconz::color(const char* type, uint address, int hue, int sat, int bri)
{
   json_t* jArray {nullptr};
   std::string result;
   std::string uuid;

   {
      cMyMutexLock lock(&mapsMutex);

      for (const auto& light : lights)
      {
         if (light.second == address)
         {
            uuid = light.first;
            break;
         }
      }
   }

   bri = (int)((255 / 100.0) * bri);
   sat = (int)((255 / 100.0) * sat);
   hue = (65535 / 360) * hue;

   json_t* jObj = json_object();
   json_object_set_new(jObj, "on", json_boolean(true));
   json_object_set_new(jObj, "hue", json_integer(hue));
   json_object_set_new(jObj, "sat", json_integer(sat));
   json_object_set_new(jObj, "bri", json_integer(bri));

   int status {success};

   if ((status = put(jArray, uuid.c_str(), jObj)) == success)
      status = checkResult(jArray);

   if (status == success)
      tell(eloDeconz, "Changed color of %s to %d", uuid.c_str(), hue);

   json_decref(jArray);

   return success;
}

//***************************************************************************
// Check Result
//***************************************************************************

int Deconz::checkResult(json_t* jArray)
{
   json_t* jData = json_array_get(jArray, 0);
   const char* status {nullptr};
   json_t* jItem {nullptr};

   if (!jData)
      return fail;

   json_object_foreach(jData, status, jItem)
   {
      if (strcmp(status, "success") != 0)
      {
         tell(eloAlways, "DECONZ: Error: '%s'", getStringFromJson(jItem, "description", "<unknown error>"));
         return fail;
      }
   }

   return success;
}

//***************************************************************************
// Put
//***************************************************************************

int Deconz::put(json_t*& jResult, const char* uuid, json_t* jData)
{
   // /api/<apikey>/lights/<id>/state

   std::string data;
   char* url {nullptr};
   asprintf(&url, "http://%s/api/%s/lights/%s/state", httpUrl.c_str(), apiKey.c_str(), uuid);
   tell(eloDebugDeconz, "DECONZ: REST call '%s'", url);

   char* payload = json_dumps(jData, JSON_REAL_PRECISION(4));
   json_decref(jData);

   if (curl.put(url, payload, &data) != success)
   {
      tell(eloAlways, "DECONZ: REST call to '%s' failed", url);
      free(url);
      return fail;
   }

   tell(eloDeconz, "-> (DECONZ) put '%s' '%s'; result [%s]", url, payload, data.c_str());
   free(url);

   json_error_t error;
   jResult = json_loads(data.c_str(), 0, &error);

   if (!jResult)
   {
      tell(eloAlways, "Error: Ignoring invalid script result [%s]", data.c_str());
      tell(eloAlways, "Error decoding json: %s (%s, line %d column %d, position %d)",
           error.text, error.source, error.line, error.column, error.position);
      return fail;
   }

   return success;
}

//***************************************************************************
// Post
//***************************************************************************

int Deconz::post(json_t*& jResult, const char* method, const char* payload)
{
   std::string data;

   // http://192.168.200.101:8081/api/<method>

   char* url {nullptr};
   asprintf(&url, "http://%s/%s", httpUrl.c_str(), method);
   tell(eloDebugDeconz, "DECONZ: REST call '%s'", url);

   if (curl.post(url, payload, &data) != success)
   {
      tell(eloAlways, "DECONZ: REST call to '%s' failed", url);
      free(url);
      return fail;
   }

   tell(eloDeconz, "-> (DECONZ) '%s' '%s' [%s]", url, payload, data.c_str());
   free(url);

   json_error_t error;
   jResult = json_loads(data.c_str(), 0, &error);

   if (!jResult)
   {
      tell(eloAlways, "Error: Ignoring invalid DECONZ result [%s]", data.c_str());
      tell(eloAlways, "Error decoding json: %s (%s, line %d column %d, position %d)",
           error.text, error.source, error.line, error.column, error.position);
      return fail;
   }

   return done;
}

//***************************************************************************
// Query
//***************************************************************************

int Deconz::query(json_t*& jResult, const char* method, const char* key)
{
   MemoryStruct data;
   int size {0};

   // http://192.168.200.101:8081/api/<key>/<method>

   char* url {nullptr};
   asprintf(&url, "http://%s/api/%s/%s", httpUrl.c_str(), apiKey.c_str(), method);
   tell(eloDebugDeconz, "DECONZ: REST call '%s'", url);

   if (curl.downloadFile(url, size, &data, 5) != success)
   {
      tell(eloAlways, "DECONZ: REST call to '%s' failed", url);
      free(url);
      return fail;
   }

   tell(eloDeconz, "<- (DECONZ) '%s' [%s]", url, data.memory);

   free(url);

   jResult = jsonLoad(data.memory);

   if (!jResult)
      return fail;

   return done;
}

#include <libwebsockets.h>

struct lws* Deconz::client_wsi {nullptr};
lws_context* Deconz::context {nullptr};
Deconz* Deconz::singleton {nullptr};

cMyMutex Deconz::messagesInMutex;
std::queue<std::string> Deconz::messagesIn;

//***************************************************************************
// Init / Exit Ws Client
//***************************************************************************

static lws_retry_bo retry {};

int Deconz::initWsClient()
{
   Deconz::singleton = this;

   int logs {LLL_ERR | LLL_WARN | LLL_USER}; // {LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE};
   lws_set_log_level(logs, NULL);

   retry.secs_since_valid_ping = 3;
   retry.secs_since_valid_hangup = 10;
   retry.conceal_count = LWS_RETRY_CONCEAL_ALWAYS;

   protocols[0].name = "deConz";
   protocols[0].callback = callbackDeconzWs;

   struct lws_context_creation_info info;
	memset(&info, 0, sizeof info);
	info.options = 0;
	info.port = CONTEXT_PORT_NO_LISTEN;  // client side - no listener
	info.protocols = protocols;
	info.timeout_secs = 10;
        info.retry_and_idle_policy = &retry;
	// info.connect_timeout_secs = 30;

	/*
	 * since we know this lws context is only ever going to be used with
	 * one client wsis / fds / sockets at a time, let lws know it doesn't
	 * have to use the default allocations for fd tables up to ulimit -n.
	 * It will just allocate for 1 internal and 1 (+ 1 http2 nwsi) that we
	 * will use.
	 */

	info.fd_limit_per_thread = 1 + 1 + 1;
	context = lws_create_context(&info);

	if (!context)
   {
		lwsl_err("lws init failed\n");
		return 1;
	}

   struct lws_client_connect_info ci;
	memset(&ci, 0, sizeof(ci));
	ci.context = context;
	ci.port = 443;
	ci.address = "192.168.200.101";
	ci.path = "/";
	ci.host = ci.address;
	ci.origin = ci.address;
	ci.protocol = protocols[0].name;
	ci.pwsi = &client_wsi;

	lws_client_connect_via_info(&ci);

   threadCtl.deconz = this;
   threadCtl.timeout = 30;

   if (pthread_create(&syncThread, NULL, syncFct, &threadCtl))
   {
      tell(eloAlways, "Error: Failed to start client daemon thread");
      return fail;
   }

	return success;
}

int Deconz::exitWsClient()
{
   if (syncThread)
   {
      threadCtl.close = true;
      lws_cancel_service(context);

      time_t endWait = time(0) + 2;  // 2 second for regular end

      while (threadCtl.active && time(0) < endWait)
         usleep(100);

      if (threadCtl.active)
         pthread_cancel(syncThread);
      else
         pthread_join(syncThread, 0);

      syncThread = 0;
   }

	lws_context_destroy(context);
   context = nullptr;
   return done;
}

//***************************************************************************
// Sync
//***************************************************************************

void* Deconz::syncFct(void* user)
{
   ThreadControl* threadCtl = (ThreadControl*)user;
   threadCtl->active = true;

   tell(eloDebugDeconz, " :: started DECONZ syncThread");

   while (!threadCtl->close)
   {
		lws_service(context, 0);
   }

   threadCtl->active = false;
   tell(eloDebugDeconz, " :: stopped DECONZ syncThread regular\n");

   return nullptr;
}

int Deconz::atInMessage(const char* data)
{
   // {"e":"changed","id":"3","r":"lights","state":{"alert":null,"bri":80,"on":true,"reachable":true},"t":"event","uniqueid":"00:0b:57:ff:fe:d5:39:21-01"}
   // {"e":"changed","id":"26","r":"lights","state":{"alert":null,"bri":244,"colormode":"hs","ct":500,"effect":"none","hue":64581,"on":true,"reachable":true,"sat":18,"xy":[0.3274,0.3218]},"t":"event","uniqueid":"00:12:4b:00:1e:d0:03:dd-0b"}
   // {"e":"changed","id":"31","r":"sensors","state":{"buttonevent":1002,"lastupdated":"2021-12-15T18:30:58.779"},"t":"event","uniqueid":"00:15:8d:00:02:13:87:2c-01-0012"}
   // {"e":"changed","id":"33","r":"sensors","state":{"lastupdated":"2021-12-16T18:34:17.872","presence":true},"t":"event","uniqueid":"00:15:8d:00:02:57:b7:41-01-0406"}

   json_error_t error;
   json_t* obj = json_loads(data, 0, &error);

   if (!obj)
   {
      tell(eloAlways, "Error: Ignoring invalid jason request [%s]", data);
      tell(eloAlways, "Error decoding json: %s (%s, line %d column %d, position %d)",
           error.text, error.source, error.line, error.column, error.position);
      return fail;
   }

   // process message, build message

   const char* uuid = getStringFromJson(obj, "uniqueid");
   const char* t = getStringFromJson(obj, "t");
   const char* event = getStringFromJson(obj, "e");
   std::string resource = getStringFromJson(obj, "r");
   long address {na};

   // we start with 'sensors' and 'lights', later on may be 'groups'

   {
      cMyMutexLock lock(&mapsMutex);

      if (resource == "lights")
         address = lights[uuid];
      else if (resource == "sensors")
         address = sensors[uuid];
      else
      {
         json_decref(obj);
         return done;
      }

      tell(eloDebugDeconz, "Got address %ld for uuid '%s'", address, uuid);
   }

   if (!getObjectFromJson(obj, "state"))
   {
      json_decref(obj);
      return done;
   }

   if (address == na || strcmp(t, "event") != 0 || strcmp(event, "changed") != 0)
   {
      tell(eloAlways, "Debug::: ignoring '%s' !!", uuid);
      json_decref(obj);
      return done;
   }

   json_t* jData = json_object();

   char* type {nullptr};
   asprintf(&type, "DZ%s", resource == "sensors" ? "S" : "L");

   json_object_set_new(jData, "type", json_string(type));
   json_object_set_new(jData, "address", json_integer(address));

   if (resource == "sensors")
   {
      if (getObjectByPath(obj, "state/buttonevent"))
         json_object_set_new(jData, "btnevent", json_integer(getIntByPath(obj, "state/buttonevent")));

      if (getObjectByPath(obj, "state/temperature"))
         json_object_set_new(jData, "value", json_real(getIntByPath(obj, "state/temperature")/100.0));

      if (getObjectByPath(obj, "state/pressure"))
         json_object_set_new(jData, "value", json_real((double)getIntByPath(obj, "state/pressure")));

      if (getObjectByPath(obj, "state/humidity"))
         json_object_set_new(jData, "value", json_real(getIntByPath(obj, "state/humidity")/100.0));

      if (getObjectByPath(obj, "state/presence"))
         json_object_set_new(jData, "presence", json_boolean(getBoolByPath(obj, "state/presence")));
   }
   else
   {
      json_object_set_new(jData, "state", json_boolean(getBoolByPath(obj, "state/on")));

      if (getObjectByPath(obj, "state/bri"))
      {
         int bri = getIntByPath(obj, "state/bri") / 255.0 * 100.0;
         json_object_set_new(jData, "bri", json_integer(bri));
      }
      if (getObjectByPath(obj, "state/hue"))
      {
         int hue = getIntByPath(obj, "state/hue") / 65535.0 * 360.0;
         json_object_set_new(jData, "hue", json_integer(hue));
      }
      if (getObjectByPath(obj, "state/sat"))
      {
         int sat = getIntByPath(obj, "state/sat") / 255.0 * 100.0;
         json_object_set_new(jData, "sat", json_integer(sat));
      }
   }

   free(type);

   char* p = json_dumps(jData, JSON_REAL_PRECISION(4));

   {
      cMyMutexLock lock(&messagesInMutex);
      messagesIn.push(p);
   }

   json_decref(jData);
   json_decref(obj);
   free(p);

   return success;
}

//***************************************************************************
// Websocket Client Callback
//***************************************************************************

int Deconz::callbackDeconzWs(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len)
{
	switch (reason)
   {
      case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
         tell(eloAlways, "CLIENT_CONNECTION_ERROR: %s\n", in ? (char*)in : "(null)");
         Deconz::client_wsi = nullptr;
         break;

      case LWS_CALLBACK_CLIENT_ESTABLISHED:
         tell(eloDebugDeconz, "%s: established\n", __func__);
         break;

      case LWS_CALLBACK_CLIENT_RECEIVE:
      {
         tell(eloDebugDeconz, "Debug: Rx (DECONZ) [%s]", (const char*)in);
         singleton->atInMessage((const char*)in);

         break;
      }
      case LWS_CALLBACK_CLIENT_CLOSED:
         Deconz::client_wsi = nullptr;
         break;

      default: break;
	}

	return lws_callback_http_dummy(wsi, reason, user, in, len);
}
