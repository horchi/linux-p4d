//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File main.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 24.12.2013  Jörg Wendel
//***************************************************************************

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include "p4d.h"

char* confDir = (char*)confDirDefault;

// defaults 

char dbHost[100+TB] = "localhost";
int  dbPort;
char dbName[100+TB] = "p4";
char dbUser[100+TB] = "p4";
char dbPass[100+TB] = "p4";

char ttyDeviceSvc[100+TB] = "/dev/ttyUSB1";
int  interval = 120;
int  stateCheckInterval = 10;
int  aggregateInterval = 15;     // aggregate interval in minutes
int  aggregateHistory = 0;       // history in days

int htmlMail = yes;

//***************************************************************************
// Configuration
//***************************************************************************

int atConfigItem(const char* Name, const char* Value)
{
   // Parse setup parameters and store values.
   
   if      (!strcasecmp(Name, "dbHost"))      sstrcpy(dbHost, Value, sizeof(dbHost));
   else if (!strcasecmp(Name, "dbPort"))      dbPort = atoi(Value);
   else if (!strcasecmp(Name, "dbName"))      sstrcpy(dbName, Value, sizeof(dbName));
   else if (!strcasecmp(Name, "dbUser"))      sstrcpy(dbUser, Value, sizeof(dbUser));
   else if (!strcasecmp(Name, "dbPass"))      sstrcpy(dbPass, Value, sizeof(dbPass));
   
   else if (!strcasecmp(Name, "logLevel"))            loglevel = atoi(Value);
   else if (!strcasecmp(Name, "interval"))            interval = atoi(Value);
   else if (!strcasecmp(Name, "stateCheckInterval"))  stateCheckInterval = atoi(Value);
   else if (!strcasecmp(Name, "ttyDeviceSvc"))        sstrcpy(ttyDeviceSvc, Value, sizeof(ttyDeviceSvc));

   else if (!strcasecmp(Name, "aggregateInterval"))  aggregateInterval = atoi(Value);
   else if (!strcasecmp(Name, "aggregateHistory"))   aggregateHistory = atoi(Value);

   else if (!strcasecmp(Name, "htmlMail"))           htmlMail = atoi(Value);

   return success;
}

//***************************************************************************
// Read Config
//***************************************************************************

int readConfig()
{
   int count = 0;
   FILE* f;
   char* line = 0;
   size_t size = 0;
   char* value;
   char* name;
   char* fileName;

   asprintf(&fileName, "%s/p4d.conf", confDir);

   if (!fileName || access(fileName, F_OK) != 0)
   {
      printf("Cannot access configuration file '%s'\n", fileName ? fileName : "<null>");
      free(fileName);
      return fail;
   }

   f = fopen(fileName, "r");

   while (getline(&line, &size, f) > 0)
   {
      char* p = strchr(line, '#');
      if (p) *p = 0;

      allTrim(line);

      if (isEmpty(line))
         continue;

      if (!(value = strchr(line, '=')))
         continue;
      
      *value = 0;
      value++;
      lTrim(value);
      name = line;
      allTrim(name);

      if (atConfigItem(name, value) != success)
      {
         printf("Found unexpected parameter '%s', aborting\n", name);
         free(fileName);
         return fail;
      }

      count++;
   }

   free(line);
   fclose(f);

   tell(eloAlways, "Read %d option from %s", count , fileName);

   free(fileName);

   return success;
}

void showUsage(const char* bin)
{
   printf("Usage: %s [-n][-c <config-dir>][-l <log-level>][-t]\n", bin);
   printf("    -n              don't daemonize\n");
   printf("    -t              log to stdout\n");
   printf("    -v              show version\n");
   printf("    -s              setup\n");
   printf("    -i              update configuration tables\n");
   printf("    -I              truncate and initialze configuration tables\n");
   printf("    -c <config-dir> use config in <config-dir>\n");
   printf("    -l <log-level>  set log level\n");
}

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char** argv)
{
   P4d* job;
   int nofork = no;
   int pid;
   int setup = no;
   int init = no;
   int truncOnInit = no;
   int _stdout = na;
   int _level = na;

   logstdout = yes;

   // Usage ..

   if (argc > 1 && (argv[1][0] == '?' || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)))
   {
      showUsage(argv[0]);
      return 0;
   }

   // Parse command line

   for (int i = 0; argv[i]; i++)
   {
      if (argv[i][0] != '-' || strlen(argv[i]) != 2)
         continue;

      switch (argv[i][1])
      {
         case 'l': if (argv[i+1]) _level = atoi(argv[i+1]); break;
         case 't': _stdout = yes;                           break;
         case 'n': nofork = yes;                            break;
         case 'c': if (argv[i+1]) confDir = argv[i+1];      break;
         case 's': setup = yes;                             break;
         case 'i': init = yes;                              break;
         case 'I': truncOnInit = yes; init = yes;           break;
         case 'v': printf("Version %s\n", VERSION);         return 1;
      }
   }

   // read configuration ..

   if (readConfig() != success)
      return 1;

   if (_level != na)  loglevel = _level;

   job = new DEAMON();

   if (job->init() != success)
   {
      printf("Initialization failed, see syslog for details\n");
      delete job;
      return 1;
   }

   if (setup)
      return job->setup();
   
   if (init)
      return job->initialize(truncOnInit);

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

   ::signal(SIGINT, DEAMON::downF);
   ::signal(SIGTERM, DEAMON::downF);
   // ::signal(SIGHUP, DEAMON::triggerF);

   if (_stdout != na) 
      logstdout = _stdout;
   else
      logstdout = no;

   // do work ...

   job->loop();

   // shutdown

   tell(eloAlways, "shutdown");

   delete job;

   return 0;
}
