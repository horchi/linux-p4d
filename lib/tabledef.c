/*
 * tabledef.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "tabledef.h"

//***************************************************************************
// Sensor Facts
//***************************************************************************

cDbService::FieldDef cSensorFactFields::fields[] =
{
   // name               format     size  index               type          viewStmt

   // primary key

   { "sensorid",         ffInt,        2, fiSensorId,      ftPrimary },

   // meta                                                         

   { "inssp",            ffInt,       10, fiInsSp,         ftMeta },
   { "updsp",            ffInt,       10, fiUpdSp,         ftMeta },
                                                                   
   // data
                                                                   
   { "name",             ffAscii,     30, fiName,          ftData },
   { "unit",             ffAscii,      5, fiUnit,          ftData },
   { "title",            ffAscii,    100, fiTitle,         ftData },

   { 0 }
};

//***************************************************************************
// Sensor Samples
//***************************************************************************

cDbService::FieldDef cSampleFields::fields[] =
{
   // name               format     size  index               type          viewStmt

   // primary key

   { "sensorid",         ffInt,        2, fiSensorId,      ftPrimary },
   { "time",             ffDateTime,   0, fiTime,          ftPrimary },

   // meta                                                         

   { "inssp",            ffInt,       10, fiInsSp,         ftMeta },
   { "updsp",            ffInt,       10, fiUpdSp,         ftMeta },
                                                                   
   // data
                                                                   
   { "value",            ffFloat,      8, fiValue,         ftData }, 
   { "text",             ffAscii,     20, fiText,          ftData }, 

   { 0 }
};


//***************************************************************************
// Value Addresses
//***************************************************************************

cDbService::FieldDef cValueAddressFields::fields[] =
{
   // name               format     size  index               type          viewStmt

   // primary key

   { "address",          ffInt,        4, fiAddress,      ftPrimary },

   // meta                                                         

   { "inssp",            ffInt,       10, fiInsSp,         ftMeta },
   { "updsp",            ffInt,       10, fiUpdSp,         ftMeta },
                                                                   
   // data
                                                                   
   { "title",            ffAscii,    100, fiTitle,         ftData }, 
   { "factor",           ffInt,        4, fiFactor,        ftData }, 
   { "unit",             ffAscii,      5, fiUnit,          ftData }, 
   { "res1",             ffInt,        4, fiRes1,          ftData }, 
   { 0 }
};
