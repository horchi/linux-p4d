
#include <stdint.h>   // uint_64_t
#include <sys/time.h>

#include <unistd.h>
#include <stdio.h>
#include <string>

#include "common.h"
#include "db.h"
#include "tabledef.h"

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
   cDbConnection::setName("wde1");
   cDbConnection::setUser("wde1");
   cDbConnection::setPass("wde1");
   cDbTable::setConfPath("/etc");

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
// 
//***************************************************************************

void chkStatement1()
{
   int status;
   
   cTableSamples* db = new cTableSamples(connection);
   
   if (db->open() != success)
   { 
      tell(0, "Could not access database '%s:%d' (%s)", 
           cDbConnection::getHost(), cDbConnection::getPort(), db->TableName());
      
      return ;
   }

   tell(0, "---------------------------------------------------");

    // prepare statement to mark wasted DVB events
   
   cDbStatement* s = new cDbStatement(db);
   
   db->setValue(cTableSamples::fiAddress, 1);
   db->setValue(cTableSamples::fiType, "VA");
   db->setValue(cTableSamples::fiTime, time(0));
   db->setValue(cTableSamples::fiValue, atof("22.3"));

   db->store();

//    s->build("update %s set ", db->TableName());
//    s->bind(cEventFields::fiSensorId, cDBS::bndIn | cDBS::bndSet);
//    s->bind(cEventFields::fiTime, cDBS::bndIn | cDBS::bndSet, ", ");
//    s->build(" where ");
//    s->bind(cEventFields::fiChannelId, cDBS::bndIn | cDBS::bndSet);
//    s->bind(cEventFields::fiSource, cDBS::bndIn | cDBS::bndSet, " and ");

//    s->bindCmp(0, endTime, ">", " and ");

//    s->bindCmp(0, cEventFields::fiStartTime, 0, "<" ,  " and ");
//    s->bindCmp(0, cEventFields::fiTableId,   0, ">" ,  " and (");
//    s->bindCmp(0, cEventFields::fiTableId,   0, "=" ,  " or (");
//    s->bindCmp(0, cEventFields::fiVersion,   0, "<>" , " and ");
//    s->build("));");
   
//    s->prepare();
   
   tell(0, "---------------------------------------------------");

   delete s;
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

//  initConnection();
//  chkStatement2();
//  exitConnection();

   return 0;
}
