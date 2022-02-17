//***************************************************************************
// Automation Control
// File w1.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 07.02.2014  Jörg Wendel
//***************************************************************************

#include <signal.h>
#include <dirent.h>
#include <vector>
#include <numeric>
#include <cmath>

#include "w1.h"
#include "lib/json.h"

bool W1::shutdown {false};

W1::W1(const char* aUrl, const char* topic)
{
   w1Path = strdup("/sys/bus/w1/devices");
   mqttTopic = topic;
   mqttPingTopic = std::string(topic) + "/ping";
   mqttUrl = aUrl;
}

W1::~W1()
{
   free(w1Path);
}

//***************************************************************************
// Show W1 Sensors
//***************************************************************************

int W1::show()
{
   for (auto it = sensors.begin(); it != sensors.end(); ++it)
      tell(eloAlways, "%s: %2.3f, %sactive", it->first.c_str(), it->second.value, it->second.active ? "" : "in-");

   return done;
}

//***************************************************************************
// Scan
//***************************************************************************

int W1::scan()
{
   DIR* dir {nullptr};
   dirent* dp {nullptr};

   if (!(dir = opendir(w1Path)))
   {
      tell(eloAlways, "Info: No One-Wire sensors found, path '%s' not exist (%s)", w1Path, strerror(errno));
      return fail;
   }

   while ((dp = readdir(dir)))
   {
      if (strlen(dp->d_name) < 3 || strstr(dp->d_name, "bus_master"))
          continue;

      char* path {nullptr};
      asprintf(&path, "%s/%s/w1_slave", w1Path, dp->d_name);
      bool exist = fileExists(path);
      free(path);

      if (!exist)
      {
         tell(eloDetail, "%s seems not to be a temperatur sensor, skipping", dp->d_name);
         continue;
      }

      if (sensors.find(dp->d_name) == sensors.end())
      {
         tell(eloAlways, "One Wire Sensor '%s' attached", dp->d_name);
         sensors[dp->d_name].value = 0;
         sensors[dp->d_name].active = true;
      }
      else if (!sensors[dp->d_name].active)
      {
         tell(eloAlways, "One Wire Sensor '%s' activated again", dp->d_name);
         sensors[dp->d_name].value = 0;
         sensors[dp->d_name].active = true;
      }
   }

   closedir(dir);

   return done;
}

//***************************************************************************
// Action
//***************************************************************************

int W1::loop()
{
   const int updateCycle {10};   // seconds

   while (!doShutDown())
   {
      time_t nextAt = time(0) + updateCycle;
      update();

      while (!doShutDown() && time(0) < nextAt)
         sleep(1);
   }

   return done;
}

//***************************************************************************
// Update
//***************************************************************************

int W1::update()
{
   tell(eloDetail, "Updating ...");

   scan();

   uint count {0};
   json_t* oJson = json_array();

   for (auto it = sensors.begin(); it != sensors.end(); it++)
   {
      char line[100+TB];
      FILE* in {nullptr};
      char* path {nullptr};

      if (!it->second.active)
         continue;

      asprintf(&path, "%s/%s/w1_slave", w1Path, it->first.c_str());

      tell(eloDetail, "Query '%s'", it->first.c_str());

      if (!(in = fopen(path, "r")))
      {
         tell(eloAlways, "Error: Opening '%s' failed, error was '%s'", path, strerror(errno));
         tell(eloAlways, "One Wire Sensor '%s' seems to be detached, removing it", path);
         it->second.active = false;
         free(path);
         continue;
      }

      while (fgets(line, 100, in))
      {
         char* p;
         line[strlen(line)-1] = 0;

         if (strstr(line, " crc="))
         {
            if (!strstr(line, " YES"))
            {
               tell(eloAlways, "Error: CRC check for '%s' failed in [%s], skipping sample", it->first.c_str(), line);
               break;
            }
         }

         else if ((p = strstr(line, " t=")))
         {
            double value = atoi(p+3) / 1000.0;

            if (value == 85 || value == -85)
            {
               // at error we get sometimes +85 or -85 from the sensor

               tell(eloAlways, "Error: Ignoring invalid value (%0.2f) of w1 sensor '%s'", value, it->first.c_str());
               break;
            }

            if (sensors[it->first].values.size() >= 3)
            {
               double sum = std::accumulate(sensors[it->first].values.cbegin(), sensors[it->first].values.cend(), 0);
               double average = sum / sensors[it->first].values.size();
               double delta = std::abs(average - value);

               tell(eloDetail, "Info: %s : %0.2f, the average of the last %zd samples is %0.2f (delta is %0.2f)",
                    it->first.c_str(), value, sensors[it->first].values.size(), average, average - value);

               if (delta > 5)
                  tell(eloDetail, "Warning: delta %0.2f", delta);
            }

            if (sensors[it->first].values.size() >= 3)
               sensors[it->first].values.erase(sensors[it->first].values.begin());

            sensors[it->first].values.push_back(value);
            sensors[it->first].value = value;

            json_t* ojData = json_object();
            json_array_append_new(oJson, ojData);
            count++;

            json_object_set_new(ojData, "name", json_string(it->first.c_str()));
            json_object_set_new(ojData, "value", json_real(value));
            json_object_set_new(ojData, "time", json_integer(time(0)));

            tell(eloDebug, "%s : %0.2f", it->first.c_str(), value);
         }
      }

      fclose(in);
      free(path);

      // take a breath .. due to a forum post we wait 1 second:
      //   -> "jede Lesung am Bus zieht die Leitungen runter und da alle Sensoren am selben Bus hängen, deswegen min. 1s zwischen den Lesungen"

      sleep(1);
   }

   if (mqttConnection() != success)
      return fail;

   char* p = json_dumps(oJson, JSON_REAL_PRECISION(4));
   json_decref(oJson);

   if (count)
      mqttW1Writer->writeRetained(mqttTopic.c_str(), p);
   else
      mqttW1Writer->write(mqttPingTopic.c_str(), "{\"ping\" : true, \"sender\" : \"" TARGET "\"}");

   free(p);

   tell(eloDetail, " ... done");

   return done;
}

//***************************************************************************
// MQTT Connection
//***************************************************************************

int W1::mqttConnection()
{
   if (!mqttW1Writer)
      mqttW1Writer = new Mqtt();

   if (!mqttW1Writer->isConnected())
   {
      if (mqttW1Writer->connect(mqttUrl) != success)
      {
         tell(eloAlways, "Error: MQTT: Connecting publisher to '%s' failed", mqttUrl);
         return fail;
      }

      tell(eloAlways, "MQTT: Connecting publisher to '%s' succeeded", mqttUrl);
      tell(eloAlways, "MQTT: Publishe W1 data to topic '%s'", mqttTopic.c_str());
   }

   return success;
}

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char** argv)
{
   W1* job;
   int nofork = no;
   int pid;
   int _stdout = na;
   const char* topic = TARGET "2mqtt/w1";
   Eloquence _eloquence {eloAlways};
   const char* url = "tcp://localhost:1883";

   logstdout = yes;

   // Usage ..

   if (argc > 1 && (argv[1][0] == '?' || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)))
   {
      // showUsage(argv[0]);  // to be implemented!
      return 0;
   }

   // Parse command line

   for (int i = 0; argv[i]; i++)
   {
      if (argv[i][0] != '-' || strlen(argv[i]) != 2)
         continue;

      switch (argv[i][1])
      {
         case 'u': url = argv[i+1];                         break;
         case 'l': if (argv[i+1]) _eloquence = (Eloquence)atoi(argv[i+1]); break;
         case 'T': if (argv[i+1]) topic = argv[i+1];        break;
         case 't': _stdout = yes;                           break;
         case 'n': nofork = yes;                            break;
      }
   }

   if (_stdout != na)
      logstdout = _stdout;
   else
      logstdout = no;

   eloquence = _eloquence;

   job = new W1(url, topic);

   if (job->init() != success)
   {
      printf("Initialization failed, see syslog for details\n");
      delete job;
      return 1;
   }

   // fork daemon

   if (!nofork)
   {
      if ((pid = fork()) < 0)
      {
         printf("Can't fork daemon, %s\n", strerror(errno));
         return 1;
      }

      if (pid != 0)
         return 0;
   }

   // register SIGINT

   ::signal(SIGINT, W1::downF);
   ::signal(SIGTERM, W1::downF);

   // do work ...

   job->loop();

   // shutdown

   tell(eloAlways, "shutdown");

   delete job;

   return 0;
}
