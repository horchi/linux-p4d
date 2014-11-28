//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File w1.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 07.02.2014  Jörg Wendel
//***************************************************************************

#include <dirent.h>

#include "w1.h"

//***************************************************************************
// Show W1 Sensors
//***************************************************************************

int W1::show()
{
   for (SensorList::iterator it = sensors.begin(); it != sensors.end(); ++it)
      tell(0, "%s: %2.3f", it->first.c_str(), it->second);
   
   return done;
}

int W1::update()
{
   for (SensorList::iterator it = sensors.begin(); it != sensors.end(); ++it)
   {
      char line[100+TB];
      FILE* in;
      char* path;
      
      asprintf(&path, "%s/%s/w1_slave", w1Path, it->first.c_str());
      
      if (!(in = fopen(path, "r")))
      {
         tell(eloAlways, "Error: Opening '%s' failed, %m", path);
         free(path);
         continue;
      }
      
      while (fgets(line, 100, in))
      {
         char* p;
         
         line[strlen(line)-1] = 0;
         
         if ((p = strstr(line, " t=")))
         {
            double temp = atoi(p+3) / 1000.0;
            sensors[it->first] = temp;
         }
      }
      
      fclose(in);
      free(path);
   }

   return done;
}

//***************************************************************************
// Scan
//***************************************************************************

int W1::scan()
{
   DIR* dir;
   dirent* dp;

   if (!(dir = opendir(w1Path)))
   {
      tell(0, "Error: Opening directory '%s' failed, %m", w1Path);
      return fail;
   }

   while ((dp = readdir(dir)))
   {
      if (strncmp(dp->d_name, "28-", 3) == 0)
         sensors[dp->d_name] = 0;
   }

   closedir(dir);

   return done;
}

//***************************************************************************
// To ID
//***************************************************************************

unsigned int W1::toId(const char* name)
{
   const char* p;
   int len = strlen(name);
   
   // use 4 minor bytes as id
   
   if (len <= 2)
      return na;
   
   if (len <= 8)
      p = name;
   else
      p = name + (len - 8);
   
   return strtoull(p, 0, 16);
}
