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

class cSensorFactFields : public cDbService
{
   public:

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

class cSampleFields : public cDbService
{
   public:

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

class cValueAddressFields : public cDbService
{
   public:

      enum FieldIndex
      {
         fiAddress,     // Sensor Address

         fiInsSp,
         fiUpdSp,

         fiTitle,
         fiFactor,
         fiUnit,
         fiRes1,

         fiCount
      };

      static FieldDef fields[];
};

//***************************************************************************
#endif //__TABLEDEF_H
