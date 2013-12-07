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
                                                                   
   { "value",            ffFloat,    122, fiValue,         ftData }, 
   { "text",             ffAscii,     50, fiText,          ftData }, 

   { 0 }
};

cDbService::IndexDef cTableSamples::indices[] =
{
   // index             fields  

   { "time",          { fiTime, na      }, 0 },
   { "type",          { fiType, na      }, 0 },

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

   { "id",               ffUInt,       4, fiId,            ftPrimary | ftAutoinc },

   // meta                                                         

   { "inssp",            ffInt,       10, fiInsSp,         ftMeta },
   { "updsp",            ffInt,       10, fiUpdSp,         ftMeta },

   // data

   { "parent",           ffUInt,       5, fiParent,        ftData },
   { "child",            ffUInt,       5, fiChild,         ftData },
   { "address",          ffUInt,       4, fiAddress,       ftData },
   { "title",            ffAscii,    100, fiTitle,         ftData },

   { "state",            ffAscii,      1, fiState,         ftData },

   { "unit",             ffAscii,      5, fiUnit,          ftData },
   { "value",            ffAscii,     10, fiValue,         ftData },

   { "unknown1",         ffInt,        4, fiType,          ftData },
   { "unknown2",         ffInt,        4, fiUnknown2,      ftData },

   { 0 }
};

//***************************************************************************
// HTML Schema Configuration
//***************************************************************************

cDbService::FieldDef cTableSchemaConf::fields[] =
{
   // name               format     size  index            type 

   // primary key

   { "address",          ffUInt,       4, fiAddress,       ftPrimary },
   { "type",             ffAscii,      2, fiType,          ftPrimary },

   // meta

   { "inssp",            ffInt,       10, fiInsSp,         ftMeta },
   { "updsp",            ffInt,       10, fiUpdSp,         ftMeta },

   // data

   { "state",            ffAscii,      1, fiState,         ftData },
   { "kind",             ffAscii,     20, fiKind,          ftData },
   { "color",            ffAscii,     10, fiColor,         ftData },
   { "showunit",         ffInt,        1, fiShowUnit,      ftData },
   { "xpos",             ffInt,        0, fiXPos,          ftData },
   { "ypos",             ffInt,        0, fiYPos,          ftData },

   { 0 }
};

//***************************************************************************
// Jobs
//***************************************************************************

cDbService::FieldDef cTableJobs::fields[] =
{
   // name               format     size  index            type 

   // primary key

   { "id",               ffUInt,      11, fiId,            ftPrimary | ftAutoinc },

   // meta                                                         

   { "inssp",            ffInt,       10, fiInsSp,         ftMeta },
   { "updsp",            ffInt,       10, fiUpdSp,         ftMeta },

   // data

   { "requestat",        ffDateTime,   0, fiReqAt,         ftData },
   { "doneat",           ffDateTime,   0, fiDoneAt,        ftData },
   { "state",            ffAscii,      1, fiState,         ftData },
   { "command",          ffAscii,    100, fiCommand,       ftData },
   { "address",          ffUInt,       4, fiAddress,       ftData },
   { "result",           ffAscii,    100, fiResult,        ftData },

   { 0 }
};
