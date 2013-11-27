/*
 * tabledef.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "tabledef.h"

//***************************************************************************
// Sensor Samples
//***************************************************************************

cDbService::FieldDef cTableSamples::fields[] =
{
   // name               format     size  index            type

   // primary key

   { "address",          ffInt,        4, fiAddress,       ftPrimary },
   { "type",             ffAscii,      2, fiType,          ftPrimary },
   { "time",             ffDateTime,   0, fiTime,          ftPrimary },

   // meta

   { "inssp",            ffInt,       10, fiInsSp,         ftMeta },
   { "updsp",            ffInt,       10, fiUpdSp,         ftMeta },
                                                                   
   // data
                                                                   
   { "value",            ffFloat,     62, fiValue,         ftData }, 
   { "text",             ffAscii,     50, fiText,          ftData }, 

   { 0 }
};

//***************************************************************************
// Value Facts
//***************************************************************************

cDbService::FieldDef cTableValueFacts::fields[] =
{
   // name               format     size  index            type 

   // primary key

   { "address",          ffInt,        4, fiAddress,       ftPrimary },
   { "type",             ffAscii,      2, fiType,          ftPrimary },

   // meta                                                         

   { "inssp",            ffInt,       10, fiInsSp,         ftMeta },
   { "updsp",            ffInt,       10, fiUpdSp,         ftMeta },

   // data

   { "state",            ffAscii,      1, fiState,         ftData },
   { "unit",             ffAscii,      5, fiUnit,          ftData },
   { "factor",           ffInt,        4, fiFactor,        ftData },
   { "name",             ffAscii,    100, fiName,          ftData },
   { "title",            ffAscii,    100, fiTitle,         ftData },

   { "res1",             ffInt,        4, fiRes1,          ftData },

   { 0 }
};

//***************************************************************************
// Parameter Facts
//***************************************************************************

cDbService::FieldDef cTableParameterFacts::fields[] =
{
   // name               format     size  index            type 

   // primary key

   { "address",          ffInt,        4, fiAddress,       ftPrimary },

   // meta                                                         

   { "inssp",            ffInt,       10, fiInsSp,         ftMeta },
   { "updsp",            ffInt,       10, fiUpdSp,         ftMeta },

   // data

   { "state",            ffAscii,      1, fiState,         ftData },
   { "unit",             ffAscii,      5, fiUnit,          ftData },
   { "factor",           ffInt,        4, fiFactor,        ftData },
   { "title",            ffAscii,    100, fiTitle,         ftData },

   { "res1",             ffInt,        4, fiRes1,          ftData },

   { 0 }
};

//***************************************************************************
// Jobs
//***************************************************************************

cDbService::FieldDef cTableJobs::fields[] =
{
   // name               format     size  index            type 

   // primary key

   { "id",               ffInt,       11, fiId,            ftPrimary | ftAutoinc },

   // meta                                                         

   { "inssp",            ffInt,       10, fiInsSp,         ftMeta },
   { "updsp",            ffInt,       10, fiUpdSp,         ftMeta },

   // data

   { "requestat",        ffDateTime,   0, fiReqAt,         ftData },
   { "doneat",           ffDateTime,   0, fiDoneAt,        ftData },
   { "state",            ffAscii,      1, fiState,         ftData },
   { "command",          ffAscii,    100, fiCommand,       ftData },

   { 0 }
};
