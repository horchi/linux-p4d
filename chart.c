/*
 * chart.c: P4 Charting Client
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <errno.h>
#include <signal.h>

#include "lib/db.h"
#include "lib/common.h"

//***************************************************************************
// Globals
//***************************************************************************

cDbConnection* connection;
cDbTable* sDb;
cDbTable* sfDb;
const char* dbhost = "localhost";
const char* dbname = "";
const char* dbuser = "";
const char* dbpass = "";
const char* filter = "";

int intervall = na;
int dbport = 3306;
int shutdown;

void downF(int aSignal)
{
   shutdown = yes;
}

//***************************************************************************
// init / exit
//***************************************************************************

int initDb()
{
   static int initialized = no;

   if (!initialized)
   {
      if (dbDict.in("/etc/p4d.dat") != success)
      {
         tell(0, "Fatal: Dictionary not loaded, aborting!");
         return 1;
      }

      cDbConnection::init();
      cDbConnection::setEncoding("utf8");
      cDbConnection::setHost(dbhost);
      cDbConnection::setPort(dbport);
      cDbConnection::setName(dbname);
      cDbConnection::setUser(dbuser);
      cDbConnection::setPass(dbpass);

      initialized = yes;
   }

   connection = new cDbConnection();

   sDb = new cDbTable(connection, "samples");
   if (sDb->open() != success) return fail;

   sfDb = new cDbTable(connection, "valuefacts");
   if (sfDb->open() != success) return fail;

   tell(0, "Connection to database established");

   return success;
}

int exitDb()
{
   sDb->close();
   sfDb->close();

   delete sDb;        sDb = 0;
   delete sfDb;       sfDb = 0;
   delete connection; connection = 0;

   return done;
}

//***************************************************************************
// Usage
//***************************************************************************

void showUsage(const char* name)
{
   printf("Usage: %s [options] - dump actual data to ascii file\n"
          "    -f <file>      - output file\n"
          "    -h <host>      - database host\n"
          "    -P <port>      - database port\n"
          "    -d <name>      - database name\n"
          "    -u <user>      - database user\n"
          "    -p <pass>      - database password\n"
          "    -l <logvel>    - log level {0-4}\n"
          "    -i <intervall> - intervall in seconds\n"
          "    -t             - log to console\n"
          "    -F <filter>    - get only parameter which match name eg. 'Time, Kesseltemperatur, Abgastemperatur'\n",
          name);
}

//***************************************************************************
// Chart
//***************************************************************************

int printActual(FILE* fp, cDbStatement* s, long lastTime)
{
   char line[500];

   sprintf(line, "Uhrzeit = %s\n", l2pTime(lastTime, "%H:%M").c_str());
   fputs(line, fp);

   for (int r = s->find(); r; r = s->fetch())
   {
      const char* f = 0;
      char* token = 0;
      int min = 0;
      int max = 0;

      double value =  sDb->getRow()->getValue("VALUE")->getFloatValue();
      const char* unit = sfDb->getRow()->getValue("UNIT")->getStrValue();
      const char* title = sfDb->getRow()->getValue("TITLE")->getStrValue();
      const char* utitle = sfDb->getRow()->getValue("USRTITLE")->getStrValue();
      const char* text = sDb->getRow()->getValue("TEXT")->getStrValue();

      tell(4, "check '%s' (%s)", utitle, title);

      if (!isEmpty(utitle))
          title = utitle;

      // check and get the filter token for the curren 'title' from the filter string 'filter'

      if (!isEmpty(filter) && !(f = strcasestr(filter, title)))
         continue;

      if (f)
      {
         int len = strlen(f);

         if (const char* p = strchr(f, ','))
            len = p-f;

         asprintf(&token, "%.*s", len, f);
      }

      if (token)
      {
         char* p = token;

         // 'token' like "Puffertemperatur oben:Puffer oben"
         // Syntax: "<db title>:<display title>:<min>:<max>"

         for (int i = 1; (p = strchr(p, ':')); i++)
         {
            *p = 0;
            p++;

            switch (i)
            {
               case 1: title = p;     break;
               case 2: min = atoi(p); break;
               case 3: max = atoi(p); break;
            }
         }
      }

      if (strcmp(unit, "°") == 0)
         unit = "°C";

      if (!isEmpty(text))
         fprintf(fp, "%s = %s", title, text);
      else if (strcmp(unit, "T") == 0)
         fprintf(fp, "%s = %s", title, l2pTime(value, "%d. %H:%M").c_str());
      else if (value != int(value))
         fprintf(fp, "%s = %2.1f%s", title, value, unit);
      else
         fprintf(fp, "%s = %d%s", title, (int)value, unit);

      // ------------------------------------------------
      // set the color  ....  #TODO to be configurable !!

      const char* color = 0;

      // heading title color

      if (sfDb->getRow()->getValue("TITLE")->hasValue("Heizungsstatus"))
      {
         if (sDb->getRow()->getValue("TEXT")->hasValue("Betriebsbereit"))
            color = "#34c634";

         else if (sDb->getRow()->getValue("TEXT")->hasValue("Heizen"))
            color = "#f00";

         else if (sDb->getRow()->getValue("TEXT")->hasValue("Anheizen") ||
                  sDb->getRow()->getValue("TEXT")->hasValue("Vorwärmen") ||
                  sDb->getRow()->getValue("TEXT")->hasValue("Zünden") ||
                  sDb->getRow()->getValue("TEXT")->hasValue("Vorbereitung"))
            color = "#ffb725";

         else if (sDb->getRow()->getValue("TEXT")->hasValue("STÖRUNG"))
            color = "yellow";

         else
            color = "blue";

         if (!isEmpty(color))
            fprintf(fp, " color %s", color);
      }

      // temp value color

      if ((min || max) && sfDb->getRow()->getValue("UNIT")->hasValue("°"))
      {
         if (min && value < min)
            color = "blue";

         if (max && value > max)
            color = "red";

         if (!isEmpty(color))
            fprintf(fp, " vcolor %s", color);
      }

      fprintf(fp, "\n");
      free(token);
   }

   return done;
}

//***************************************************************************
// Actual
//***************************************************************************

int actualAscii(const char* file)
{
   FILE* fp;
   long lastTime;

   // select max(time) from samples;

   cDbStatement* selMaxTime = new cDbStatement(sDb);

   selMaxTime->build("select max(");
   selMaxTime->bind("TIME", cDBS::bndOut);
   selMaxTime->build(") from %s;", sDb->TableName());
   selMaxTime->prepare();

   // select s.value, f.name, f.title, f.unit
   //   from samples s, valuefacts f
   //     where s.address = f.address and s.type = f.type and s.time = ?;

   cDbStatement* s = new cDbStatement(sDb);

   s->build("select ");
   s->setBindPrefix("s.");
   s->bind("VALUE", cDBS::bndOut);
   s->bind("TEXT", cDBS::bndOut, ", ");
   s->setBindPrefix("f.");
   s->bind(sfDb->getValue("NAME"), cDBS::bndOut, ", ");
   s->bind(sfDb->getValue("TITLE"), cDBS::bndOut, ", ");
   s->bind(sfDb->getValue("USRTITLE"), cDBS::bndOut, ", ");
   s->bind(sfDb->getValue("UNIT"), cDBS::bndOut, ", ");
   s->build(" from %s s, %s f where ", sDb->TableName(), sfDb->TableName());
   s->build("s.address = f.address ");
   s->build("and s.type = f.type ");
   s->setBindPrefix("s.");
   s->bind(sDb->getValue("TIME"), cDBS::bndIn | cDBS::bndSet, "and ");
   s->build(";");
   s->prepare();

   shutdown = no;

   while (!shutdown)
   {
      if (isEmpty(file) || strcmp(file, "-") == 0)
         fp = stdout;
      else
         fp = fopen(file, "w");

      if (!fp)
      {
         tell(0, "Error: Can't open file '%s' for writing, error was '%s'", file, strerror(errno));
         return 0;
      }

      sDb->clear();

      // get last Time (newest data)

      if (!selMaxTime->find())
         break;

      lastTime = sDb->getRow()->getValue("TIME")->getTimeValue();
      selMaxTime->freeResult();

      // get values

      sDb->clear();
      sfDb->clear();
      sDb->setValue("TIME", lastTime);

      // print the values

      printActual(fp, s, lastTime);
      tell(0, "updated values");

      s->freeResult();

      if (fp && fp != stdout)
         fclose(fp);

      if (intervall == na)
         break;

      sleep(intervall);
   }

   tell(0, "Shutdown regular");

   delete selMaxTime;
   delete s;

   return 0;
}

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char** argv)
{
   const char* file = 0;

   loglevel = 0;
   logstdout = no;

   if (argc == 1 || (argc > 1 && (argv[1][0] == '?' || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))))
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
         case 'l': if (argv[i+1]) loglevel = atoi(argv[++i]);  break;
         case 'P': if (argv[i+1]) dbport = atoi(argv[++i]);    break;
         case 'd': if (argv[i+1]) dbname = argv[++i];          break;
         case 'h': if (argv[i+1]) dbhost = argv[++i];          break;
         case 'u': if (argv[i+1]) dbuser = argv[++i];          break;
         case 'p': if (argv[i+1]) dbpass = argv[++i];          break;
         case 'f': if (argv[i+1]) file = argv[++i];            break;
         case 'F': if (argv[i+1]) filter = argv[++i];          break;
         case 'i': if (argv[i+1]) intervall = atoi(argv[++i]); break;
         case 't': logstdout = yes;                            break;
      }
   }

   // init database connection

   if (initDb() != success)
      return 1;

   // register SIGINT

   ::signal(SIGINT, downF);
   ::signal(SIGTERM, downF);

   actualAscii(file);

   // exit

   exitDb();
   cDbConnection::exit();

   return 0;
}
