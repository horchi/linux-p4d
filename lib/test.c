
#include <stdint.h>   // uint_64_t
#include <sys/time.h>

#include <unistd.h>
#include <stdio.h>

#include "common.h"
#include "db.h"
#include "dbdict.h"

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
   db->setIntValue("ADDRESS", 1234);
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

int main(int argc, char** argv)
{
   logstdout = yes;
   loglevel = 2;

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
