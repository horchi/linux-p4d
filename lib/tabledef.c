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

   { "address",          ffUInt,       4, fiAddress,       ftPrimary },
   { "type",             ffAscii,      2, fiType,          ftPrimary },
   { "aggregate",        ffAscii,      1, fiAggregate,     ftPrimary },
   { "time",             ffDateTime,   0, fiTime,          ftPrimary },

   // meta

   { "inssp",            ffInt,       10, fiInsSp,         ftMeta },
   { "updsp",            ffInt,       10, fiUpdSp,         ftMeta },
                                                                   
   // data
                                                                   
   { "value",            ffFloat,    122, fiValue,         ftData }, 
   { "text",             ffAscii,     50, fiText,          ftData }, 
   { "samples",          ffInt,        3, fiSamples,       ftData },

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

   { "address",          ffUInt,       4, fiAddress,       ftPrimary },
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
// Menu
//***************************************************************************

cDbService::FieldDef cTableMenu::fields[] =
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
   { "type"    ,         ffUInt,       1, fiType,          ftData },
   { "address",          ffUInt,       4, fiAddress,       ftData },
   { "title",            ffAscii,    100, fiTitle,         ftData },

   { "state",            ffAscii,      1, fiState,         ftData },
   { "unit",             ffAscii,      5, fiUnit,          ftData },
   { "value",            ffAscii,    100, fiValue,         ftData },
   
   { "unknown1",         ffInt,        4, fiUnknown1,      ftData },
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
   { "showtext",         ffInt,        1, fiShowText,      ftData },
   { "xpos",             ffInt,        0, fiXPos,          ftData },
   { "ypos",             ffInt,        0, fiYPos,          ftData },

   { 0 }
};

//***************************************************************************
// Config Fields
//***************************************************************************

cDbService::FieldDef cTableConfig::fields[] =
{
   // name               format     size index           type

   { "owner",            ffAscii,    40, fiOwner,        ftPrimary },
   { "name",             ffAscii,    40, fiName,         ftPrimary },

   { "inssp",            ffInt,       0, fiInsSp,        ftMeta },
   { "updsp",            ffInt,       0, fiUpdSp,        ftMeta },

   { "value",            ffAscii,   100, fiValue,        ftData },

   { 0 }
};

//***************************************************************************
// Errors
//***************************************************************************

cDbService::FieldDef cTableErrors::fields[] =
{
   // name               format     size index           type

   { "id",               ffUInt,     11,  fiId,           ftPrimary | ftAutoinc },

   { "inssp",            ffInt,       0,  fiInsSp,        ftMeta },
   { "updsp",            ffInt,       0,  fiUpdSp,        ftMeta },

   { "time",             ffDateTime,  0,  fiTime,         ftData },
   { "number",           ffInt,      10,  fiNumber,       ftData },
   { "info",             ffInt,      10,  fiInfo,         ftData },
   { "state",            ffAscii,    10,  fiState,        ftData },
   { "text",             ffAscii,    100, fiText,         ftData },

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
   { "data",             ffAscii,    100, fiData,          ftData },

   { 0 }
};

cDbService::IndexDef cTableJobs::indices[] =
{
   // index             fields  

   { "state",           { fiState, na      }, 0 },
   { "requestat",       { fiReqAt, na      }, 0 },

   { 0 }
};

//***************************************************************************
// Sensor Alert
//***************************************************************************

cDbService::FieldDef cTableSensorAlert::fields[] =
{
   // name               format     size  index            type 

   // primary key

   { "id",               ffUInt,      11, fiId,            ftPrimary | ftAutoinc },

   // meta                                                         

   { "inssp",            ffInt,       10, fiInsSp,         ftMeta },
   { "updsp",            ffInt,       10, fiUpdSp,         ftMeta },

   // data

   { "address",          ffUInt,       4, fiAddress,       ftData },
   { "type",             ffAscii,      2, fiType,          ftData },

   { "state",            ffAscii,      1, fiState,         ftData },

   { "min",              ffInt,       10, fiMin,           ftData },
   { "max",              ffInt,       10, fiMax,           ftData },

   { "rangem",           ffInt,       10, fiRangeM,        ftData },
   { "delta",            ffInt,       10, fiDelta,         ftData },

   { "maddress",         ffAscii,    100, fiMAddress,      ftData },
   { "msubject",         ffAscii,    100, fiMSubject,      ftData },
   { "mbody",            ffText,    2000, fiMBody,         ftData },

   { "lastalert",        ffInt,       10, fiLastAlert,     ftData },
   { "maxrepeat",        ffInt,       10, fiMaxRepeat,     ftData },

   { 0 }
};
