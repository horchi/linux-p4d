//***************************************************************************
// Automation Control
// File deconz.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 15.12.2021  Jörg Wendel
//***************************************************************************

#pragma once

#include "lib/common.h"

class Daemon;

//***************************************************************************
// Class Deconz
//***************************************************************************

class Deconz
{
   public:

      Deconz();
      ~Deconz();

      int init(Daemon* parent, cDbConnection* connection);
      int exit();

      void setApiKey(const char* key)  { apiKey = key; }
      void setHttpUrl(const char* url) { httpUrl = url; }

      const char* getHttpUrl() { return httpUrl.c_str(); }
      const char* getApiKey() { return apiKey.c_str(); }

      int query(json_t*& jResult, const char* method, const char* apiKey);
      int put(json_t*& jResult, const char* uuid, json_t* jData);
      int post(json_t*& jResult, const char* method, const char* payload);
      int checkResult(json_t* jArray);

      int queryApiKey(std::string& result);
      int initDevices();
      int processDevices(json_t* jData, std::string kind);

      int toggle(const char* type, uint address, bool state, int bri = na, int transitiontime = na);
      int color(const char* type, uint address, int hue, int saturation, int bri);

      static struct lws* client_wsi;  // needed???

      static cMyMutex messagesInMutex;
      static std::queue<std::string> messagesIn;

   private:

      int initWsClient();
      int exitWsClient();
      int service();
      static int callbackDeconzWs(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len);

      //

      std::string httpUrl;
      std::string apiKey;
      Daemon* daemon {};

      std::map<std::string,uint> lights;
      std::map<std::string,uint> sensors;
      static cMyMutex mapsMutex;

      cDbTable* tableDeconzLights {};
      cDbTable* tableDeconzSensors {};
      cDbStatement* selectLightByUuid {};
      cDbStatement* selectSensorByUuid {};

      lws_protocols protocols[2];

      // sync thread stuff

      struct ThreadControl
      {
         bool active {false};
         bool close {false};
         Deconz* deconz {};
         int timeout {60};
      };

      pthread_t syncThread {0};
      ThreadControl threadCtl;

      static void* syncFct(void* user);
      int atInMessage(const char* data);

      static Deconz* singleton;
      static struct lws_context* context;
};
