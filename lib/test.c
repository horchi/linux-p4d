
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
   Table::setConfPath("/etc");

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
   
   Table* db = new Table(connection, "samples", cSampleFields::fields);
   
   if (db->open() != success)
   { 
      tell(0, "Could not access database '%s:%d' (%s)", 
           cDbConnection::getHost(), cDbConnection::getPort(), db->TableName());
      
      return ;
   }

   tell(0, "---------------------------------------------------");

    // prepare statement to mark wasted DVB events
   
   cDbStatement* s = new cDbStatement(db);
   
   db->setValue(cSampleFields::fiSensorId, 1);
   db->setValue(cSampleFields::fiTime, time(0));
   db->setValue(cSampleFields::fiValue, atof("22.3"));

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
   
   Table* db = new Table(connection, "sensorfacts", cSensorFactFields::fields);
   
   if (db->open() != success)
   { 
      tell(0, "Could not access database '%s:%d' (%s)", 
           cDbConnection::getHost(), cDbConnection::getPort(), db->TableName());
      
      return ;
   }

   tell(0, "---------------------------------------------------");

   // temp

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 3);
   db->setValue(cSensorFactFields::fiName,  "tempSensor1");
   db->setValue(cSensorFactFields::fiUnit,  "°C");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 4);
   db->setValue(cSensorFactFields::fiName,  "tempSensor2");
   db->setValue(cSensorFactFields::fiUnit,  "°C");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 5);
   db->setValue(cSensorFactFields::fiName,  "tempSensor3");
   db->setValue(cSensorFactFields::fiUnit,  "°C");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 6);
   db->setValue(cSensorFactFields::fiName,  "tempSensor4");
   db->setValue(cSensorFactFields::fiUnit,  "°C");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 7);
   db->setValue(cSensorFactFields::fiName,  "tempSensor5");
   db->setValue(cSensorFactFields::fiUnit,  "°C");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 8);
   db->setValue(cSensorFactFields::fiName,  "tempSensor6");
   db->setValue(cSensorFactFields::fiUnit,  "°C");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 9);
   db->setValue(cSensorFactFields::fiName,  "tempSensor7");
   db->setValue(cSensorFactFields::fiUnit,  "°C");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 10);
   db->setValue(cSensorFactFields::fiName,  "tempSensor8");
   db->setValue(cSensorFactFields::fiUnit,  "°C");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   // hum

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 11);
   db->setValue(cSensorFactFields::fiName,  "humSensor1");
   db->setValue(cSensorFactFields::fiUnit,  "%");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 12);
   db->setValue(cSensorFactFields::fiName,  "humSensor2");
   db->setValue(cSensorFactFields::fiUnit,  "%");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 13);
   db->setValue(cSensorFactFields::fiName,  "humSensor3");
   db->setValue(cSensorFactFields::fiUnit,  "%");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 14);
   db->setValue(cSensorFactFields::fiName,  "humSensor4");
   db->setValue(cSensorFactFields::fiUnit,  "%");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 15);
   db->setValue(cSensorFactFields::fiName,  "humSensor5");
   db->setValue(cSensorFactFields::fiUnit,  "%");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 16);
   db->setValue(cSensorFactFields::fiName,  "humSensor6");
   db->setValue(cSensorFactFields::fiUnit,  "%");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 17);
   db->setValue(cSensorFactFields::fiName,  "humSensor7");
   db->setValue(cSensorFactFields::fiUnit,  "%");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 18);
   db->setValue(cSensorFactFields::fiName,  "humSensor8");
   db->setValue(cSensorFactFields::fiUnit,  "%");
   db->setValue(cSensorFactFields::fiTitle, "");
   db->store();

   // kombi sensor

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 19);
   db->setValue(cSensorFactFields::fiName,  "tempKombi");
   db->setValue(cSensorFactFields::fiUnit,  "°C");
   db->setValue(cSensorFactFields::fiTitle, "Temperatur außen");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 20);
   db->setValue(cSensorFactFields::fiName,  "humKombi");
   db->setValue(cSensorFactFields::fiUnit,  "%");
   db->setValue(cSensorFactFields::fiTitle, "Luftfeuchte außen");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 21);
   db->setValue(cSensorFactFields::fiName,  "windKombi");
   db->setValue(cSensorFactFields::fiUnit,  "km/h");
   db->setValue(cSensorFactFields::fiTitle, "Wind");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 22);
   db->setValue(cSensorFactFields::fiName,  "rainvolKombi");
   db->setValue(cSensorFactFields::fiUnit,  "l/m²");
   db->setValue(cSensorFactFields::fiTitle, "Niederschalgsmenge");
   db->store();

   db->clear();
   db->setValue(cSensorFactFields::fiSensorId, 23);
   db->setValue(cSensorFactFields::fiName,  "rainKombi");
   db->setValue(cSensorFactFields::fiUnit,  "1|0");
   db->setValue(cSensorFactFields::fiTitle, "Regen");
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
