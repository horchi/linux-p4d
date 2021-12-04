//***************************************************************************
// Automation Control
// File w1.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 10.08.2020 - Jörg Wendel
//***************************************************************************

#pragma once

#include <stdio.h>
#include <map>
#include <limits>

#include "lib/common.h"
#include "lib/mqtt.h"

// #define W1_UDEF std::numeric_limits<double>::max()

//***************************************************************************
// Class W1
//***************************************************************************

class W1
{
   public:

      struct SensorData
      {
         double value;
         std::vector<double> values;
         bool active {false};
      };

      typedef std::map<std::string, SensorData> SensorList;

      W1(const char* aUrl);
      ~W1();

      static void downF(int aSignal) { shutdown = true; }

      int init() { return success; }
      int loop();
      int show();
      int scan();
      int update();
      bool doShutDown() { return shutdown; }

      const SensorList* getList()    { return &sensors; }
      size_t getCount()              { return sensors.size(); }

   protected:

      int mqttConnection();

      char* w1Path {nullptr};
      SensorList sensors;
      const char* mqttUrl {nullptr};
      const char* mqttTopic {nullptr};
      const char* mqttPingTopic {nullptr};
      Mqtt* mqttW1Writer {nullptr};

      static bool shutdown;
};
