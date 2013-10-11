/*
 * chart.c: P4 Charting Client
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <errno.h>
#include <mgl2/mgl.h>

#include "lib/tabledef.h"
#include "lib/common.h"

//***************************************************************************
// Globals
//***************************************************************************

cDbConnection* connection;
Table* sDb;
Table* sfDb;
const char* dbhost = "localhost";
const char* dbname = "";
const char* dbuser = "";
const char* dbpass = "";
int dbport = 3306;

//***************************************************************************
// init / exit
//***************************************************************************

int initDb()
{
   static int initialized = no;

   if (!initialized)
   {
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

   sDb = new Table(connection, "samples", cSampleFields::fields);
   
   if (sDb->open() != success)
   {
      tell(0, "Could not access database '%s:%d' (%s)", 
           cDbConnection::getHost(), cDbConnection::getPort(), sDb->TableName());

      return fail;
   }

   sfDb = new Table(connection, "sensorfacts", cSensorFactFields::fields);
   
   if (sfDb->open() != success)
   {
      tell(0, "Could not access database '%s:%d' (%s)", 
           cDbConnection::getHost(), cDbConnection::getPort(), sfDb->TableName());

      return fail;
   }

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
   printf("Usage: %s {chart|actual} [options]\n"
          "  chart        - create sensor chart\n"
          "  actual       - dump actual data to ascii file (format as needed by VDRs gtft plugin)\n"
          "    -f <file>      - output file\n"
          "    -h <host>      - database host\n"
          "    -P <port>      - database port\n"
          "    -d <name>      - database name\n"
          "    -u <user>      - database user\n"
          "    -p <pass>      - database password\n"
          "    -l <lecvel>    - log level {0-4}\n"
          "    -i <interval>  - inverval fpr charts [h] (default 10)\n",
          name);
}

//***************************************************************************
// Chart
//***************************************************************************

struct Sensor
{
   string name;
   string title;
   string color;
   string unit;
   mglData xdat;
   mglData ydat;
};

//***************************************************************************
// Dirty code but mathgl don't support 'normal' UFT-8 or ISO Codes :(
//***************************************************************************

const char* toMglCode(const char* from)
{
   static char mglcd[1000+TB] = "";
   const char* p = from;
   string str = "";

   *mglcd = 0;

   if (!from)
      return "";

   while (*p)
   {
      if (strncmp(p, "°", strlen("°")) == 0)
      {
         str += "\\utf0x0b0 ";
         p += strlen("°");
      }
      else if (strncmp(p, "ä", strlen("ä")) == 0)
      {
         str += "\\utf0x0e4 ";
         p += strlen("ä");
      }
      else if (strncmp(p, "Ä", strlen("Ä")) == 0)
      {
         str += "\\utf0x0c4 ";
         p += strlen("Ä");
      }
      else if (strncmp(p, "ö", strlen("ö")) == 0)
      {
         str += "\\utf0x0f6 ";
         p += strlen("ö");
      }
      else if (strncmp(p, "Ö", strlen("Ö")) == 0)
      {
         str += "\\utf0x0d6 ";
         p += strlen("Ö");
      }
      else if (strncmp(p, "ü", strlen("ü")) == 0)
      {
         str += "\\utf0x0fc ";
         p += strlen("ü");
      }
      else if (strncmp(p, "Ü", strlen("Ü")) == 0)
      {
         str += "\\utf0x0dc ";
         p += strlen("Ü");
      }
      else if (strncmp(p, "ß", strlen("ß")) == 0)
      {
         str += "\\utf0x0df ";
         p += strlen("ß");
      }

      else 
         str += *p++;
   }

   strcpy(mglcd, str.c_str());
   tell(2, "Converted '%s' to '%s'", from, mglcd);   

   return mglcd;
}

int chart(const char* sensorList, const char* file, int interval)
{
   std::vector<Sensor> sensors;
   std::vector<Sensor>::iterator it;

   const char* colors = "krGbcymhwRgBCYMHW";

   int multiAxis = no;
   mglGraph *gr = new mglGraph(0, 1360, 768); // 1024, 300); 
   long st, et;

   // ---------------------------
   // fill mglData

   // select s.time, s.value, f.unit, f.title 
   //   from samples s, sensorfacts f 
   //     where s.sensorid = f.sensorid 
   //     and s.time > DATE_SUB(NOW(),INTERVAL 24 HOUR) 
   //     and f.name = 'tempKombi' 
   //   order by time;

   cDbStatement* stmt = new cDbStatement(sDb);
   
   stmt->build("select ");
   stmt->setBindPrefix("s.");
   stmt->bind(cSampleFields::fiTime, cDBS::bndOut);
   stmt->bind(cSampleFields::fiValue, cDBS::bndOut, ", ");
   stmt->setBindPrefix("f.");
   stmt->bind(sfDb->getValue(cSensorFactFields::fiUnit), cDBS::bndOut, ", ");
   stmt->bind(sfDb->getValue(cSensorFactFields::fiTitle), cDBS::bndOut, ", ");
   stmt->build(" from %s s, %s f where ", sDb->TableName(), sfDb->TableName());
   stmt->build("s.sensorid = f.sensorid ");
   stmt->build("and s.%s > DATE_SUB(NOW(),INTERVAL %d HOUR)", 
            sDb->getField(cSampleFields::fiTime)->name, interval);
   stmt->bind(sfDb->getValue(cSensorFactFields::fiName), cDBS::bndIn | cDBS::bndSet, " and ");
   stmt->build(" order by %s;", sDb->getField(cSampleFields::fiTime)->name);
   stmt->prepare();

   // --------------------
   // fill sensor list

   string lastUnit = "";
   char* ss = strdup(sensorList);
   char* b = ss;
   char* e = 0;
   Sensor s;
   int _min = 999999;
   int _max = -999999;

   while (e = strchr(b, ','))
   {
      *e = 0;

      s.color = "";

      if (char* c = strchr(b, ':'))
      {
         *c = 0;
         s.color = c+1;
      }

      s.name = b;

      tell(2, "Added sensor: %s, color '%s'", s.name.c_str(), s.color.c_str());
      sensors.push_back(s);

      b = e+1;
   }
   
   s.color = "";

   if (char* c = strchr(b, ':'))
   {
      *c = 0;
      s.color = c+1;
   }

   s.name = b;
   sensors.push_back(s);
   tell(2, "Added sensor: %s, color '%s'", s.name.c_str(), s.color.c_str());

   free(ss);

   for (it = sensors.begin(); it != sensors.end(); it++)
   {
      int i = 0;
      
      sDb->clear();
      sfDb->clear();
      sfDb->setValue(cSensorFactFields::fiName, (*it).name.c_str());

      for (int f = stmt->find(); f; f = stmt->fetch())
      {
         if (i)
         {
            (*it).ydat.Insert('x', i, 1);
            (*it).xdat.Insert('x', i, 1);
         }
         else
         {
            (*it).title = toMglCode(sfDb->getValue(cSensorFactFields::fiTitle)->getStrValue());
            (*it).unit = toMglCode(sfDb->getValue(cSensorFactFields::fiUnit)->getStrValue());

            if (lastUnit.length() && lastUnit != (*it).unit)
               multiAxis = yes;
            
            lastUnit = (*it).unit;

            st = sDb->getRow()->getValue(cSampleFields::fiTime)->getTimeValue();
         }
         
         (*it).xdat.a[i] = sDb->getRow()->getValue(cSampleFields::fiTime)->getTimeValue();
         (*it).ydat.a[i] = sDb->getFloatValue(cSampleFields::fiValue);
         
         et = sDb->getRow()->getValue(cSampleFields::fiTime)->getTimeValue();

         i++;
      }

      if (_min > (*it).ydat.Minimal())
         _min = (*it).ydat.Minimal();

      if (_max < (*it).ydat.Maximal())
         _max = (*it).ydat.Maximal();

      stmt->freeResult();
      tell(1, "added %d samples for '%s' in color '%s'", i, (*it).name.c_str(), (*it).color.c_str());
   }

   // some settings

   gr->SubPlot(1, 1, 0,"");

   // gr->SetMeshNum(5);
   // gr->SetRotatedText(true);
   // gr->SetQuality(3); // MGL_DRAW_HIGH);
   // gr->SetTuneTicks(false, 0.5);
   // gr->SetBarWidth(0.1);
   // gr->SetMarkSize(1);
   // gr->Title(sfDb->getValue(cSensorFactFields::fiTitle)->getStrValue());

   gr->SetFontSizePT(4);
   gr->Clf(40, 40, 40);      // background color (RGB)

   gr->SetOrigin(NAN, NAN);
   gr->SetRange('x', st, et);

   if (interval >= 48)
      gr->SetTicksTime('x', 0, "%d.%m.%y");
   else
      gr->SetTicksTime('x', 0, "%H:%M");

   if (!multiAxis)
   {
      double off = _max / 10;
      gr->SetRange('y', _min-off, _max+off);
      gr->Label('y', lastUnit.c_str());
      gr->Axis();
      gr->Grid("", ":");
   }
   else
   {
      gr->Axis("x");
      gr->Grid("x", ":");
   }

   // data plot

   int pos = 0;

   for (it = sensors.begin(); it != sensors.end(); it++, pos++)
   {
      char* tmp;
      double off = (*it).ydat.Maximal() / 10;

      double xRange = (*it).xdat.Maximal() - (*it).xdat.Minimal();
      double scaleOff = xRange / 29;

      char c[100];  

      strcpy(c , (*it).color.c_str());

      if (multiAxis)
      {
         if (isEmpty((*it).color.c_str()))
         {
            c[0] = colors[pos];
            c[1] = 0;
         }

         gr->SetRange('y',(*it).ydat.Minimal()-off, (*it).ydat.Maximal()+off);
         gr->SetOrigin((*it).xdat.Minimal() - (pos-1)*scaleOff, NAN);
         gr->Label('y', string(string("#") + c + string("{") + (*it).unit + string("}")).c_str(), 1.18);

         gr->SetOrigin((*it).xdat.Minimal() - pos*scaleOff, NAN);
         gr->Axis("y", c);

         gr->Grid("y", string(c + string(":")).c_str());
      }

      asprintf(&tmp, "legend '%s'", (*it).title.c_str());
      gr->Plot((*it).xdat, (*it).ydat, c, tmp);

      free(tmp);
   }

   gr->Legend();
   gr->WriteJPEG(file);

   delete stmt;
   delete gr;

   return 0;
}

//***************************************************************************
// Actual
//***************************************************************************

int actual(const char* file)
{
   FILE* fp;
   char line[500];
   long lastTime;

   // select max(time) from samples;

   cDbStatement* selMaxTime = new cDbStatement(sDb);
   
   selMaxTime->build("select max(");
   selMaxTime->bind(cSampleFields::fiTime, cDBS::bndOut);
   selMaxTime->build(") from %s;", sDb->TableName());
   selMaxTime->prepare();

   sDb->clear();

   if (!selMaxTime->find())
      return done;

   lastTime = sDb->getRow()->getValue(cSampleFields::fiTime)->getTimeValue();

   selMaxTime->freeResult();
   delete selMaxTime;
   selMaxTime = 0;

   // select s.value, f.name, f.title, f.unit
   //   from samples s, sensorfacts f 
   //     where s.sensorid = f.sensorid and s.time = ?;

   cDbStatement* s = new cDbStatement(sDb);

   s->build("select ");
   s->setBindPrefix("s.");
   s->bind(cSampleFields::fiValue, cDBS::bndOut);
   s->bind(cSampleFields::fiText, cDBS::bndOut, ", ");
   s->setBindPrefix("f.");
   s->bind(sfDb->getValue(cSensorFactFields::fiName), cDBS::bndOut, ", ");
   s->bind(sfDb->getValue(cSensorFactFields::fiTitle), cDBS::bndOut, ", ");
   s->bind(sfDb->getValue(cSensorFactFields::fiUnit), cDBS::bndOut, ", ");
   s->build(" from %s s, %s f where ", sDb->TableName(), sfDb->TableName());
   s->build("s.sensorid = f.sensorid ");
   s->setBindPrefix("s.");
   s->bind(sDb->getValue(cSampleFields::fiTime), cDBS::bndIn | cDBS::bndSet, "and ");
   s->build(";");
   s->prepare();

   sDb->clear();
   sfDb->clear();
   sDb->setValue(cSampleFields::fiTime, lastTime);  //-60*60);

   fp = fopen(file, "w");

   if (fp)
   {
      sprintf(line, "// %s\n", l2pTime(lastTime).c_str());
      fputs(line, fp);

      sprintf(line, "var varTime = %ld;\n", lastTime);
      fputs(line, fp);

      for (int f = s->find(); f; f = s->fetch())
      {
         char* name =strdup(sfDb->getRow()->getValue(cSensorFactFields::fiName)->getStrValue());
         
         if (isEmpty(name))
            continue;

         name[0] = toupper(name[0]);

         fprintf(fp, "// --------------------------------------------\n");

         double v =  sDb->getRow()->getValue(cSampleFields::fiValue)->getFloatValue();

         if (v != int(v))
            fprintf(fp, "var var%sValue = %2.1f;\n", name, v);
         else
            fprintf(fp, "var var%sValue = %d;\n", name, (int)v);
         
         fprintf(fp, "var var%sTitle = %s;\n", name,
                 sfDb->getRow()->getValue(cSensorFactFields::fiTitle)->getStrValue());
                 
         fprintf(fp, "var var%sUnit = %s;\n", name,
                 sfDb->getRow()->getValue(cSensorFactFields::fiUnit)->getStrValue());

         fprintf(fp, "var var%sText = %s;\n", name,
                 sDb->getRow()->getValue(cSampleFields::fiText)->getStrValue());
      
         free(name);
      }
   }
   else 
      tell(0, "Error: can't open file '%s' for writing", file, strerror(errno));
   
   s->freeResult();
   delete s;
   
   fclose(fp);

   return 0;
}

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char** argv)
{
   int doChart = no;
   int doActual = no;
   const char* sensors = 0;
   const char* file = 0;
   int interval = 10;
   
   loglevel = 0;
   logstdout = yes;

   if (argc == 1 || argc > 1 && (argv[1][0] == '?' || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)))
   {
      showUsage(argv[0]);
      return 0;
   }

   // Parse command line

   for (int i = 0; argv[i]; i++)
   {

      if (strcmp(argv[1], "chart") == 0)
         doChart = yes;
      else if (strcmp(argv[1], "actual") == 0)
         doActual = yes;

      if (argv[i][0] != '-' || strlen(argv[i]) != 2)
         continue;

      switch (argv[i][1])
      {
         case 'l': if (argv[i+1]) loglevel = atoi(argv[++i]); break;
         case 'i': if (argv[i+1]) interval = atoi(argv[++i]); break;
         case 'P': if (argv[i+1]) dbport = atoi(argv[++i]);   break;
         case 'd': if (argv[i+1]) dbname = argv[++i];         break;
         case 'h': if (argv[i+1]) dbhost = argv[++i];         break;
         case 'u': if (argv[i+1]) dbuser = argv[++i];         break;
         case 'p': if (argv[i+1]) dbpass = argv[++i];         break;
         case 's': if (argv[i+1]) sensors = argv[++i];        break;
         case 'f': if (argv[i+1]) file = argv[++i];           break;
      }
   }
  
   if (!doActual && !doChart)
   {
      showUsage(argv[0]);
      return 0;
   }

   // init database connection

   if (initDb() != success)
      return 1;

   // work ..

   if (doChart)
   {
      if (isEmpty(sensors))
         tell(0, "Missing sensors");
      else
         chart(sensors, file, interval);
   }
   else
      actual(file);

   // exit

   exitDb();
   cDbConnection::exit();

   return 0;
}
