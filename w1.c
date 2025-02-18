//***************************************************************************
// Automation Control
// File w1.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 2010-2022  Jörg Wendel
//***************************************************************************

#include <signal.h>
#include <dirent.h>
#include <vector>
#include <numeric>
#include <cmath>

#ifndef _NO_RASPBERRY_PI_
#  include <wiringPi.h>
#else
#  include "gpio.h"
#endif

#include "HISTORY.h"
#include "w1.h"
#include "lib/json.h"

bool W1::shutdown {false};
int W1::w1PowerPin {na};
int W1::w1Count {na};

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
// Action
//***************************************************************************

int W1::loop()
{
   const int updateCycle {10};   // seconds

   wiringPiSetupPhys();                // we use the 'physical' PIN numbers
   pinMode(w1PowerPin, OUTPUT);
   digitalWrite(w1PowerPin, false);    // false -> on

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
// Scan
//***************************************************************************

int W1::scan()
{
   DIR* dir {};
   dirent* dp {};

   if (!(dir = opendir(w1Path)))
   {
      tell(eloAlways, "Info: No One-Wire sensors found, path '%s' not exist (%s)", w1Path, strerror(errno));
      return fail;
   }

   while ((dp = readdir(dir)))
   {
      if (strlen(dp->d_name) < 3 || strstr(dp->d_name, "bus_master"))
          continue;

      char* path {};
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
// Check
//***************************************************************************

int W1::check()
{
   scan();

   if (w1Count == na || w1PowerPin == na)
      return done;

   int count {0};

   for (const auto& sensorIt : sensors)
   {
      if (sensorIt.second.active)
         count++;
   }

   if (count < w1Count)
   {
      // missing at least one sensor

      tell(eloAlways, "Warning: %d W1 sensors present, expecting %d, "
           "reseting power line to force a re-initialization, standby for 10 seconds",
           count, w1Count);
      digitalWrite(w1PowerPin, true);     // true -> off
      sleep(2);
      digitalWrite(w1PowerPin, false);    // false -> on

      tell(eloAlways, "Warning: W1 power line at pin (%d) reseted, standby for 20 seconds", w1PowerPin);
      sleep(20);
      scan();
   }

   return done;
}

//***************************************************************************
// Update
//***************************************************************************

int W1::update()
{
   tell(eloInfo, "Updating ...");

   check();

   uint count {0};
   json_t* oJson = json_array();

   for (auto it = sensors.begin(); it != sensors.end(); it++)
   {
      char line[100+TB] {};
      FILE* in {};
      char* path {};

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

            // myString tmp = it->first;
            // #TODO deactivated - check later
            // if (!tmp.starts_with("3b-") && (value == 85 || value < -55 || value > 125))
            // {
            //    // at error we get sometimes like +85 or -85 from the sensor :o

            //    tell(eloAlways, "Error: Ignoring invalid value (%0.2f) of w1 sensor '%s'", value, it->first.c_str());
            //    break;
            // }

            if (sensors[it->first].values.size() >= 3)
            {
               double sum = std::accumulate(sensors[it->first].values.cbegin(), sensors[it->first].values.cend(), 0);
               double average = sum / sensors[it->first].values.size();
               double delta = std::abs(average - value);

               tell(eloDetail, "Info: %s : %0.2f, the average of the last %zd samples is %0.2f (delta is %0.2f)",
                    it->first.c_str(), value, sensors[it->first].values.size(), average, average - value);

               if (delta > 5)
                  tell(eloDetail, "Info: delta %0.2f", delta);
            }

            if (sensors[it->first].values.size() >= 3)
               sensors[it->first].values.erase(sensors[it->first].values.begin());

            sensors[it->first].values.push_back(value);
            sensors[it->first].value = value;

            json_t* ojData = json_object();
            json_array_append_new(oJson, ojData);
            count++;

            // round one wire sensors to 0.5° (TODO make it configurable?)

            value = round(2.0 * value) / 2.0;     // to 0.50
            // value = round(4.0 * value) / 4.0;  // to 0.25

            json_object_set_new(ojData, "name", json_string(it->first.c_str()));
            json_object_set_new(ojData, "value", json_real(value));
            json_object_set_new(ojData, "time", json_integer(time(0)));

            tell(eloDebug, "%s : %0.2f", it->first.c_str(), value);
         }
      }

      fclose(in);
      free(path);

      // take a breath ..
      //   -> "jedes lesen des Bus zieht die Leitungen runter und da alle Sensoren am selben Bus hängen
      //      daher min. 1 Sekunde Pause zwischen den Zugriffen"

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

   tell(eloInfo, " ... done");

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
      tell(eloAlways, "MQTT: Publish W1 data to topic '%s'", mqttTopic.c_str());
   }

   return success;
}

//***************************************************************************
// Usage
//***************************************************************************

int showUsage(const char* bin)
{
   printf("Usage: %s <command> [-l <log-level>] [-d <device>]\n", bin);
   printf("  options:\n");
   printf("     -u <url>        MQTT url\n");
   printf("     -T <topic>      MQTT topic\n");
   printf("     -c <count>      <count> of expected W1 sensors (optional)\n");
   printf("                      if not <count> sensors present and <power pin> is configured\n");
   printf("                      power line will be reset once per interval (optional)\n");
   printf("     -p <GPIO pin>   physical number of GPIO pin of W1 power supply (optional)\n");
   printf("     -l <eloquence>  set eloquence\n");
   printf("     -n              don't demonize (optional)\n");
   printf("     -t              log to terminal (optional)\n");
   printf("     -v              show version\n");

   return 0;
}

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char** argv)
{
   bool nofork {false};
   int _stdout {na};
   W1* job {};
   const char* topic {TARGET "2mqtt/w1"};
   Eloquence _eloquence {eloAlways};
   const char* url {"tcp://localhost:1883"};

   // Usage ..

   if (argc > 1 && (argv[1][0] == '?' || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)))
      return showUsage(argv[0]);

   // Parse command line

   for (int i = 0; argv[i]; i++)
   {
      if (argv[i][0] != '-' || strlen(argv[i]) != 2)
         continue;

      switch (argv[i][1])
      {
         case 'u': url = argv[i+1];                                        break;
         case 'T': if (argv[i+1]) topic = argv[i+1];                       break;
         case 'c': if (argv[i+1]) W1::w1Count = atoi(argv[i+1]);           break;
         case 'p': if (argv[i+1]) W1::w1PowerPin = atoi(argv[i+1]);        break;
         case 'l': if (argv[i+1]) _eloquence = (Eloquence)atoi(argv[i+1]); break;
         case 't': _stdout = yes;                                          break;
         case 'n': nofork = true;                                          break;
         case 'v': printf("Version %s\n", VERSION);                        return 1;
      }
   }

   if (_stdout != na)
      logstdout = _stdout;
   else
      logstdout = false;

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
      int pid;

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
