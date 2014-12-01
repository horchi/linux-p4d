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
         : cDbTable(aConnection, aName, fields, indices) { }

      enum FieldIndex
      {
         fiAddress,      // Sensor ID
         fiType,         // Sensor Type (VA, DI, DO, AI, AO)
         fiAggregate,    // aggregate - 'A' or single sample - NULL or 'S'
         fiTime,         // Zeitpunkt der Aufzeichnung

         fiInsSp,
         fiUpdSp,

         fiValue,
         fiText,
         fiSamples,      // sample count for aggregate ('A') rows, otherwise 1 or NULL

         fiCount
      };

      static FieldDef fields[];
      static IndexDef indices[];
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
         fiType,        // Sensor Type (VA, DI, DO, AI, AO, W1)

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
// class cTableMenu
//***************************************************************************

class cTableMenu : public cDbTable
{
   public:

      cTableMenu(cDbConnection* aConnection, const char* aName = "menu")
         : cDbTable(aConnection, aName, fields) { }

      enum FieldIndex
      {
         fiId,

         fiInsSp,
         fiUpdSp,

         fiParent,
         fiChild,
         fiType,
         fiAddress,      // Sensor Address
         fiTitle,

         fiState,
         fiUnit,
         fiValue,        // to store the last may be deprecated value for fast access

         fiUnknown1,     // 
         fiUnknown2,     // unknown data

         fiCount
      };

      static FieldDef fields[];
};

//***************************************************************************
// class cTableSchemaConf
//***************************************************************************

class cTableSchemaConf : public cDbTable
{
   public:

      cTableSchemaConf(cDbConnection* aConnection, const char* aName = "schemaconf")
         : cDbTable(aConnection, aName, fields) { }

      enum FieldIndex
      {
         fiAddress,
         fiType,

         fiInsSp,
         fiUpdSp,

         fiState,
         fiKind,  // unused, perpared for html display kind ( value,state,time,... )
         fiColor,
         fiShowUnit,
         fiShowText,
         fiXPos,
         fiYPos,

         fiCount
      };

      static FieldDef fields[];
};

//***************************************************************************
// class cTableConfig
//***************************************************************************

class cTableConfig : public cDbTable
{
   public:

      cTableConfig(cDbConnection* aConnection, const char* aName = "config")
         : cDbTable(aConnection, aName, fields) { }

      enum FieldIndex
      {
         fiOwner,
         fiName,

         fiInsSp,
         fiUpdSp,

         fiValue,

         fiCount
      };

      static FieldDef fields[];
};

//***************************************************************************
// class cTableErrors
//***************************************************************************

class cTableErrors : public cDbTable
{
   public:

      cTableErrors(cDbConnection* aConnection, const char* aName = "errors")
         : cDbTable(aConnection, aName, fields) { }

      enum FieldIndex
      {
         fiId,    
         
         fiInsSp, 
         fiUpdSp, 
         
         fiTime,  
         fiNumber,
         fiInfo,  
         fiState, 
         fiText, 

         fiCount
      };

      static FieldDef fields[];
};

//***************************************************************************
// class cTableJobs
//***************************************************************************

class cTableJobs : public cDbTable
{
   public:

      cTableJobs(cDbConnection* aConnection, const char* aName = "jobs")
         : cDbTable(aConnection, aName, fields, indices) { }

      enum FieldIndex
      {
         fiId,

         fiInsSp,
         fiUpdSp,

         fiReqAt,
         fiDoneAt,
         fiState,
         fiCommand,
         fiAddress,
         fiResult,
         fiData,

         fiCount
      };

      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
#endif //__TABLEDEF_H
