
#include <stdint.h>   // uint_64_t
#include <sys/time.h>

#include <unistd.h>
#include <stdio.h>

#include "common.h"
#include "db.h"
#include "dbdict.h"
#include "curl.h"

cDbConnection* connection = 0;

//***************************************************************************
// Init Connection
//***************************************************************************

void initConnection()
{
   cDbConnection::init();

   cDbConnection::setEncoding("utf8");
   cDbConnection::setHost("localhost");
   // cDbConnection::setPort();
   cDbConnection::setName("p4");
   cDbConnection::setUser("p4");
   cDbConnection::setPass("p4");
   cDbConnection::setConfPath("/etc/p4d");

   connection = new cDbConnection();
}

void exitConnection()
{
   cDbConnection::exit();

   if (connection)
      delete connection;
}

//***************************************************************************
//
//***************************************************************************

//***************************************************************************
// chkStatement1
//***************************************************************************

void chkStatement1()
{
   time_t now = time(0);
   double value = 12.3;
   unsigned int factor = 2;

   tell(0, "---------------------------------------------------");

#define __NEW

#ifdef __NEW

   cDbTable* db = new cDbTable(connection, "samples");

   if (db->open() != success)
   {
      tell(0, "Could not access database '%s:%d' (%s)",
           cDbConnection::getHost(), cDbConnection::getPort(), db->TableName());

      return ;
   }

   db->clear();

   db->setValue("TIME", now);
   db->setValue("ADDRESS", 1234);
   db->setValue("TYPE", "VA");
   db->setValue("AGGREGATE", "S");

   db->setValue("VALUE", value / (double)factor);
   db->setValue("TEXT", "text test 1234546576890");
   db->setValue("SAMPLES", 1);

#else

   cTableSamples* db = new cTableSamples(connection);

   if (db->open() != success)
   {
      tell(0, "Could not access database '%s:%d' (%s)",
           cDbConnection::getHost(), cDbConnection::getPort(), db->TableName());

      return ;
   }

   db->clear();

   db->setValue(cTableSamples::fiTime, now);
   db->setIntValue(cTableSamples::fiAddress, 1234);
   db->setValue(cTableSamples::fiType, "VA");
   db->setValue(cTableSamples::fiAggregate, "S");

   db->setValue(cTableSamples::fiValue, value / (double)factor);
   db->setValue(cTableSamples::fiText, "text test 1234546576890");
   db->setValue(cTableSamples::fiSamples, 1);

#endif

   db->store();

   tell(0, "---------------------------------------------------");

   delete db;
}

cRetBuf tst()
{
   char x[200];

   sprintf(x, "test '%s'", "hallo");

   return x;
}

//***************************************************************************
// Main
//***************************************************************************

#include <libxml/parser.h>

int main(int argc, char** argv)
{
   logstdout = yes;
   loglevel = 2;

   MemoryStruct data;
   cCurl curl;
   int size = 0;

   curl.init();

   curl.downloadFile("http://192.168.200.151//config/xmlapi/sysvarlist.cgi", size, &data);
   curl.exit();

   // tell(0, "%s", data.memory);

   xmlDoc* document = 0;
   xmlNode* root = 0;
   int readOptions = 0;

#if LIBXML_VERSION >= 20900
   readOptions |=  XML_PARSE_HUGE;
#endif

   tell(1, "Got [%s]", data.memory ? data.memory : "<null>");

   if (document = xmlReadMemory(data.memory, data.size, "", 0, readOptions))
      root = xmlDocGetRootElement(document);

   if (!root)
   {
      tell(0, "Error: Failed to parse XML document [%s]", data.memory ? data.memory : "<null>");
      return fail;
   }

   for (xmlNode* node = root->children; node; node = node->next)
   {
      xmlChar* id = xmlGetProp(node, (xmlChar*)"ise_id");
      xmlChar* name = xmlGetProp(node, (xmlChar*)"name");
      xmlChar* type = xmlGetProp(node, (xmlChar*)"type");
      xmlChar* unit = xmlGetProp(node, (xmlChar*)"unit");
      xmlChar* visible = xmlGetProp(node, (xmlChar*)"visible");
      xmlChar* min = xmlGetProp(node, (xmlChar*)"min");
      xmlChar* max = xmlGetProp(node, (xmlChar*)"max");
      xmlChar* time = xmlGetProp(node, (xmlChar*)"timestamp");
      xmlChar* value = xmlGetProp(node, (xmlChar*)"value");

      if (strcmp((const char*)visible, "true") == 0)
         tell(0, "  SysVar (%d): '%s' [%s ; unit: %s]", atoi((const char*)id), name, value, unit);

      xmlFree(id);
      xmlFree(name);
      xmlFree(type);
      xmlFree(unit);
      xmlFree(visible);
      xmlFree(min);
      xmlFree(max);
      xmlFree(time);
      xmlFree(value);

      // for (xmlAttr* attr = node->properties; NULL != attr; attr = attr->next)
      // {
      //    xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
      //    tell(0, "     %s: %s", attr->name, value);
      //    xmlFree(value);
      // }
   }

   return 0;

//    Sem* sem = new Sem(0x3da00001);

//    sem->p();
//    printf("got lock\n");

//    sleep(5);

//    sem->v();
//    printf("freed lock\n");

   cRetBuf b = tst();

   printf("%s\n", *b);

   // read deictionary

   if (dbDict.in("/etc/p4d/p4d.dat") != success)
   {
      tell(0, "Invalid dictionary configuration, aborting!");
      return 1;
   }

   initConnection();
   chkStatement1();
   exitConnection();

   return 0;
}
