/*
 * tabledef.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __TABLEDEF_H
#define __TABLEDEF_H

#include "db.h"

//***************************************************************************
// class cWdeFields
//***************************************************************************

class cTableSensorFacts : public cDbTable
{
   public:

      cTableSensorFacts(cDbConnection* aConnection)
         : cDbTable(aConnection, fields) { }

      virtual const char* TableName()    { return "sensorfacts"; }

      enum FieldIndex
      {
         fiSensorId,  // Sensor ID

         fiInsSp,
         fiUpdSp,

         fiName,      // { "Sensor1","Kombi" }
         fiUnit,      // { "°C", "%" }
         fiTitle,     // user defined title like "Garten" or "Temperatur Garten"

         fiCount
      };

      static FieldDef fields[];
};


//***************************************************************************
// class cWdeFields
//***************************************************************************

class cTableSamples : public cDbTable
{
   public:

      cTableSamples(cDbConnection* aConnection)
         : cDbTable(aConnection, fields) { }

      virtual const char* TableName()    { return "samples"; }

      enum FieldIndex
      {
         fiSensorId,     // Sensor ID
         fiTime,         // Zeitpunkt der Aufzeichnung

         fiInsSp,
         fiUpdSp,

         fiValue,
         fiText,

         fiCount
      };

      static FieldDef fields[];
};

//***************************************************************************
// class
//***************************************************************************

class cTableValueFacts : public cDbTable
{
   public:

      cTableValueFacts(cDbConnection* aConnection)
         : cDbTable(aConnection, fields) { }

      virtual const char* TableName()    { return "valuefacts"; }

      enum FieldIndex
      {
         fiAddress,     // Sensor Address

         fiInsSp,
         fiUpdSp,

         fiState,

         fiUnit,        
         fiFactor,
         fiTitle,

         fiRes1,

         fiCount
      };

      static FieldDef fields[];
};

//***************************************************************************
#endif //__TABLEDEF_H
