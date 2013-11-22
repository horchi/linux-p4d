
#include <stdint.h>   // uint_64_t
#include <sys/time.h>

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

void chkStatement2()
{
   int status;
   
   cTableSensorFacts* db = new cTableSensorFacts(connection);
   
   if (db->open() != success)
   { 
      tell(0, "Could not access database '%s:%d' (%s)", 
           cDbConnection::getHost(), cDbConnection::getPort(), db->TableName());
      
      return ;
   }

   tell(0, "---------------------------------------------------");

   // temp

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 3);
   db->setValue(cTableSensorFacts::fiName,  "tempSensor1");
   db->setValue(cTableSensorFacts::fiUnit,  "°C");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 4);
   db->setValue(cTableSensorFacts::fiName,  "tempSensor2");
   db->setValue(cTableSensorFacts::fiUnit,  "°C");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 5);
   db->setValue(cTableSensorFacts::fiName,  "tempSensor3");
   db->setValue(cTableSensorFacts::fiUnit,  "°C");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 6);
   db->setValue(cTableSensorFacts::fiName,  "tempSensor4");
   db->setValue(cTableSensorFacts::fiUnit,  "°C");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 7);
   db->setValue(cTableSensorFacts::fiName,  "tempSensor5");
   db->setValue(cTableSensorFacts::fiUnit,  "°C");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 8);
   db->setValue(cTableSensorFacts::fiName,  "tempSensor6");
   db->setValue(cTableSensorFacts::fiUnit,  "°C");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 9);
   db->setValue(cTableSensorFacts::fiName,  "tempSensor7");
   db->setValue(cTableSensorFacts::fiUnit,  "°C");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 10);
   db->setValue(cTableSensorFacts::fiName,  "tempSensor8");
   db->setValue(cTableSensorFacts::fiUnit,  "°C");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   // hum

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 11);
   db->setValue(cTableSensorFacts::fiName,  "humSensor1");
   db->setValue(cTableSensorFacts::fiUnit,  "%");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 12);
   db->setValue(cTableSensorFacts::fiName,  "humSensor2");
   db->setValue(cTableSensorFacts::fiUnit,  "%");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 13);
   db->setValue(cTableSensorFacts::fiName,  "humSensor3");
   db->setValue(cTableSensorFacts::fiUnit,  "%");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 14);
   db->setValue(cTableSensorFacts::fiName,  "humSensor4");
   db->setValue(cTableSensorFacts::fiUnit,  "%");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 15);
   db->setValue(cTableSensorFacts::fiName,  "humSensor5");
   db->setValue(cTableSensorFacts::fiUnit,  "%");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 16);
   db->setValue(cTableSensorFacts::fiName,  "humSensor6");
   db->setValue(cTableSensorFacts::fiUnit,  "%");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 17);
   db->setValue(cTableSensorFacts::fiName,  "humSensor7");
   db->setValue(cTableSensorFacts::fiUnit,  "%");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 18);
   db->setValue(cTableSensorFacts::fiName,  "humSensor8");
   db->setValue(cTableSensorFacts::fiUnit,  "%");
   db->setValue(cTableSensorFacts::fiTitle, "");
   db->store();

   // kombi sensor

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 19);
   db->setValue(cTableSensorFacts::fiName,  "tempKombi");
   db->setValue(cTableSensorFacts::fiUnit,  "°C");
   db->setValue(cTableSensorFacts::fiTitle, "Temperatur außen");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 20);
   db->setValue(cTableSensorFacts::fiName,  "humKombi");
   db->setValue(cTableSensorFacts::fiUnit,  "%");
   db->setValue(cTableSensorFacts::fiTitle, "Luftfeuchte außen");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 21);
   db->setValue(cTableSensorFacts::fiName,  "windKombi");
   db->setValue(cTableSensorFacts::fiUnit,  "km/h");
   db->setValue(cTableSensorFacts::fiTitle, "Wind");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 22);
   db->setValue(cTableSensorFacts::fiName,  "rainvolKombi");
   db->setValue(cTableSensorFacts::fiUnit,  "l/m²");
   db->setValue(cTableSensorFacts::fiTitle, "Niederschalgsmenge");
   db->store();

   db->clear();
   db->setValue(cTableSensorFacts::fiSensorId, 23);
   db->setValue(cTableSensorFacts::fiName,  "rainKombi");
   db->setValue(cTableSensorFacts::fiUnit,  "1|0");
   db->setValue(cTableSensorFacts::fiTitle, "Regen");
   db->store();

   tell(0, "---------------------------------------------------");

   delete db;
}

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char** argv)
{
   logstdout = yes;
   loglevel = 2;

//    byte* b = (byte*)argv[1];
//    int size = strlen((char*)b);

   byte b[30];
   int size = 7;

   b[0] = 0x02;
   b[1] = 0xFD;
   b[2] = 0x00;
   b[3] = 0x03;
   b[4] = 0x30;
   b[5] = 0x00;
   b[6] = 0x62;

   printf("0x%X  / %d\n", crc(b, size), crc(b, size));

//    initConnection();

//    chkStatement2();

//   exitConnection();

   return 0;
}
