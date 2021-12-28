//***************************************************************************
// Automation Control
// File main.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 01.12.2021  Jörg Wendel
//***************************************************************************

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include "specific.h"

char* confDir = (char*)confDirDefault;

// defaults

char dbHost[100+TB] = "localhost";
int  dbPort;
char dbName[100+TB] = TARGET;
char dbUser[100+TB] = TARGET;
char dbPass[100+TB] = TARGET;

//***************************************************************************
// Configuration
//***************************************************************************

int atConfigItem(const char* Name, const char* Value)
{
   // Parse setup parameters and store values.

   if      (!strcasecmp(Name, "dbHost")) sstrcpy(dbHost, Value, sizeof(dbHost));
   else if (!strcasecmp(Name, "dbPort")) dbPort = atoi(Value);
   else if (!strcasecmp(Name, "dbName")) sstrcpy(dbName, Value, sizeof(dbName));
   else if (!strcasecmp(Name, "dbUser")) sstrcpy(dbUser, Value, sizeof(dbUser));
   else if (!strcasecmp(Name, "dbPass")) sstrcpy(dbPass, Value, sizeof(dbPass));

   return success;
}

//***************************************************************************
// Read Config
//***************************************************************************

int readConfig()
{
   int count {0};
   char* line {nullptr};
   size_t size {0};
   char* value {nullptr};
   char* name {nullptr};
   char* fileName {nullptr};

   asprintf(&fileName, "%s/daemon.conf", confDir);

   if (!fileName || access(fileName, F_OK) != 0)
   {
      printf("Cannot access configuration file '%s'\n", fileName ? fileName : "<null>");
      free(fileName);
      return fail;
   }

   FILE* f = fopen(fileName, "r");

   while (getline(&line, &size, f) > 0)
   {
      char* p = strchr(line, '#');

      if (p && (!*(p-1) || *(p-1) != '\\'))
         *p = 0;
      else if (p && *(p-1) && *(p-1) == '\\')
      {
         std::string s = strReplace("\\#", "#", line);
         free(line);
         line = strdup(s.c_str());
         size = strlen(line+TB);
      }

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
   printf("Usage: %s [-n][-c <config-dir>][-l <eloquence>][-t]\n", bin);
   printf("    -n              don't daemonize\n");
   printf("    -t              log to stdout\n");
   printf("    -T              log to stdout during init\n");
   printf("    -v              show version\n");
   printf("    -c <config-dir> use config in <config-dir>\n");
   printf("    -l <eloquence>  set eloquence\n");
}

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char** argv)
{
   CLASS* job;
   int nofork = no;
   int pid;
   bool _stdout {false};
   bool _stdoutOnInit {false};
   Eloquence _eloquence {eloAlways};

   logstdout = true;

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
         case 'l': if (argv[i+1]) _eloquence = (Eloquence)atoi(argv[i+1]); break;
         case 't': _stdout = true;                          break;
         case 'T': _stdoutOnInit = true;                    break;
         case 'n': nofork = yes;                            break;
         case 'c': if (argv[i+1]) confDir = argv[i+1];      break;
         case 'v': printf("Version %s\n", VERSION);         return 1;
      }
   }

   if (!_stdoutOnInit)
      logstdout = _stdout;

   // read configuration ..

   if (readConfig() != success)
      return 1;

   argEloquence = _eloquence;

   // fork daemon

   if (!nofork)
   {
      if ((pid = fork()) < 0)
      {
         printf("Can't fork daemon, %s\n", strerror(errno));
         return 1;
      }

      // exit parent

      if (pid != 0)
         return 0;

      // i'am the child!
   }

   // int AFTER fork !!!

   job = new CLASS();

   if (job->init() != success)
   {
      printf("Initialization failed, see syslog for details\n");
      delete job;
      return 1;
   }

   // register SIGINT

   ::signal(SIGINT, CLASS::downF);
   ::signal(SIGTERM, CLASS::downF);
   // ::signal(SIGHUP, CLASS::triggerF);

   // after init ... switch to log file

   logstdout = _stdout;

   // do work ...

   job->loop();

   // shutdown

   tell(eloAlways, "shutdown");

   delete job;

   return 0;
}
