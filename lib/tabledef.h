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
// class cTableSamples
//***************************************************************************

class cTableSamples : public cDbTable
{
   public:

      cTableSamples(cDbConnection* aConnection, const char* aName = "samples")
         : cDbTable(aConnection, aName, fields ) { }

      enum FieldIndex
      {
         fiAddress,      // Sensor ID
         fiType,         // Sensor Type (VA, DI, DO, AI, AO)
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
// class cTableSensorFacts
//***************************************************************************

class cTableSensorFacts : public cDbTable
{
   public:

      cTableSensorFacts(cDbConnection* aConnection, const char* aName = "sensorfacts")
         : cDbTable(aConnection, aName, fields) { }

      enum FieldIndex
      {
         fiSensorId,  // Sensor ID

         fiInsSp,
         fiUpdSp,

         fiName,      // 
         fiUnit,      // { "°C", "%" }
         fiTitle,     // user defined title 

         fiCount
      };

      static FieldDef fields[];
};

//***************************************************************************
// class cTableValueFacts
//***************************************************************************

class cTableValueFacts : public cDbTable
{
   public:

      cTableValueFacts(cDbConnection* aConnection, const char* aName = "valuefacts")
         : cDbTable(aConnection, aName, fields) { }

      enum FieldIndex
      {
         fiAddress,     // Sensor Address
         fiType,        // Sensor Type (VA, DI, DO, AI, AO)

         fiInsSp,
         fiUpdSp,

         fiState,

         fiUnit,        
         fiFactor,
         fiName,
         fiTitle,

         fiRes1,       // unknown data

         fiCount
      };

      static FieldDef fields[];
};


//***************************************************************************
// class cTableParameterFacts
//***************************************************************************

class cTableParameterFacts : public cDbTable
{
   public:

      cTableParameterFacts(cDbConnection* aConnection, const char* aName = "parameterfacts")
         : cDbTable(aConnection, aName, fields) { }

      enum FieldIndex
      {
         fiAddress,     // Sensor Address

         fiInsSp,
         fiUpdSp,

         fiState,

         fiUnit,        
         fiFactor,
         fiTitle,

         fiRes1,       // unknown data

         fiCount
      };

      static FieldDef fields[];
};

//***************************************************************************
#endif //__TABLEDEF_H
