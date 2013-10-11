/*
 * db.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __DB_H
#define __DB_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sstream>

#include <mysql/mysql.h>

#include "common.h"

class cDbRow;
class Table;

//***************************************************************************
// cDbService
//***************************************************************************

class cDbService
{
   public:

      enum Misc
      {
         maxViewFields = 50
      };

      enum FieldIndex
      {
         fiNone = -2            // different from na !
      };

      enum FieldFormat
      {
         ffInt,
         ffAscii,     // -> VARCHAR
         ffText,
         ffMlob,      // -> MEDIUMBLOB
         ffFloat,
         ffDateTime,
         ffCount
      };

      enum FieldType
      {
         ftData       = 1,
         ftPrimary    = 2,
         ftMeta       = 4,
         ftCalc       = 8
      };

      enum ViewType
      {
         vtDbView,        //  will created as a database view and a select was prepared
         vtDbViewSelect,  //  prepared a select but avoid creating the view
         vtSelect         //  logical view only a select statement will be prepared
      };

      struct FieldDef
      {
         const char* name;
         FieldFormat format;
         int size;
         int index;
         int type;
      };

      struct Criteria
      {
         int fieldIndex;
         const char* op;
      };

      enum BindType
      {
         bndIn  = 0x001,
         bndOut = 0x002,
         bndSet = 0x004
      };

      struct ViewDef
      {
         const char* name;
         ViewType type;
         const char* from;
         Criteria crit[maxViewFields+1];
         int critCount;
         int fields[maxViewFields+1];
         int fieldCount;         
         int order;
      };

      static const char* toString(FieldFormat t);
      static const char* formats[];
};

typedef cDbService cDBS;

//***************************************************************************
// cDbValue
//***************************************************************************

class cDbValue : public cDbService
{
   public:

      cDbValue(FieldDef* f = 0)
      {
         field = 0;
         strValue = 0;
         ownField = 0;

         if (f) setField(f);
      }

      cDbValue(const char* name, FieldFormat format, int size)
      {
         ownField = new FieldDef;
         ownField->name = strdup(name);
         ownField->format = format;
         ownField->size = size;
         ownField->type = ftData;

         field = ownField;
      }

      virtual ~cDbValue()
      {
         free();
      }

      void free()
      {
         clear();
         ::free(strValue);
         strValue = 0;

         if (ownField)
         {
            ::free((char*)ownField->name);  // böser cast ;)
            delete ownField;
            ownField = 0;
         }

         field = 0;
      }

      void clear()
      {
         if (strValue)
            *strValue = 0;

         strValueSize = 0;
         numValue = 0;
         floatValue = 0;
         nullValue = 1;
      }

      virtual void setField(FieldDef* f)
      { 
         free();
         field = f;

         if (field)
            strValue = (char*)calloc(field->size+TB, sizeof(char));
      }

      virtual FieldDef* getField()    { return field; }
      virtual const char* getName()   { return field->name; }

      void setValue(const char* value, int size = 0)
      { 
         clear();

         if (field->format != ffAscii && field->format != ffText && field->format != ffMlob)
         {
            tell(0, "Setting invalid field format for '%s', expected ASCII or MLOB", field->name);
            return;
         }

         if (field->format == ffMlob && !size)
         {
            tell(0, "Missing size for MLOB field '%s'", field->name);
            return;
         }

         if (value && size)
         {
            if (size > field->size)
            {
               tell(0, "Warning, size of %d for '%s' exeeded, got %d bytes!",
                    field->size, field->name, size);

               size = field->size;
            }

            memcpy(strValue, value, size);
            strValue[size] = 0;
            strValueSize = size;
            nullValue = 0;
         }

         else if (value)
         {
            if (strlen(value) > (size_t)field->size)
               tell(0, "Warning, size of %d for '%s' exeeded [%s]", 
                    field->size, field->name, value);

            sprintf(strValue, "%.*s", field->size, value);
            strValueSize = strlen(strValue);
            nullValue = 0;
         }
      }

      void setValue(double value)
      { 
         if (field->format == ffInt)
         {
            numValue = value;
            nullValue = 0;
         }
         else if (field->format == ffFloat)
         {
            floatValue = value;
            nullValue = 0;
         }
         else if (field->format == ffDateTime)
         {
            struct tm tm;
            time_t v = value;

            localtime_r(&v, &tm);
            
            timeValue.year = tm.tm_year + 1900;
            timeValue.month = tm.tm_mon + 1;
            timeValue.day = tm.tm_mday;
            
            timeValue.hour = tm.tm_hour;
            timeValue.minute = tm.tm_min;
            timeValue.second = tm.tm_sec;

            nullValue = 0;
         }
         else
         {
            tell(0, "Setting invalid field format for '%s'", field->name);
         }
      }

      int hasValue(double value)
      {
         if (field->format == ffInt)
            return numValue == value;

         if (field->format == ffFloat)
            return floatValue == value;

         if (field->format == ffDateTime)
            return no; // to be implemented!

         tell(0, "Setting invalid field format for '%s'", field->name);

         return no;
      }

      int hasValue(float value)
      {
         if (field->format != ffFloat)
         {
            tell(0, "Checking invalid field format for '%s', expected FLOAT", field->name);
            return no;
         }

         return floatValue == value;
      }

      int hasValue(const char* value)
      { 
         if (!value)
            value = "";

         if (field->format != ffAscii && field->format != ffText)
         {
            tell(0, "Checking invalid field format for '%s', expected ASCII or MLOB", field->name);
            return no;
         }

         return strcmp(getStrValue(), value) == 0;
      }

      time_t getTimeValue()
      {
         struct tm tm;
         
         tm.tm_year = timeValue.year - 1900;
         tm.tm_mon  = timeValue.month - 1;
         tm.tm_mday  = timeValue.day;

         tm.tm_hour = timeValue.hour;
         tm.tm_min = timeValue.minute;
         tm.tm_sec = timeValue.second;
         
         return mktime(&tm);
      }

      unsigned long* getStrValueSizeRef()  { return &strValueSize; }
      unsigned long getStrValueSize()      { return strValueSize; }
      const char* getStrValue()            { return !isNull() && strValue ? strValue : ""; }
      long getIntValue()                   { return !isNull() ? numValue : 0; }
      float getFloatValue()                { return !isNull() ? floatValue : 0; }
      int isNull()                         { return nullValue; }

      char* getStrValueRef()               { return strValue; }
      long* getIntValueRef()               { return &numValue; }
      MYSQL_TIME* getTimeValueRef()        { return &timeValue; }
      float* getFloatValueRef()            { return &floatValue; }
      my_bool* getNullRef()                { return &nullValue; }

      const char* getIntValueAsStr() const 
      { 
         stringstream ss("");            // hässliches Stream-Zeugs :( aber manchmal praktisch ;)
         ss << numValue;
         strcpy(strValue, ss.str().c_str());
         return strValue;
      }

   private:

      FieldDef* ownField;
      FieldDef* field;
      long numValue;
      float floatValue;
      MYSQL_TIME timeValue;
      char* strValue;
      unsigned long strValueSize;
      my_bool nullValue;
};

//***************************************************************************
// cDbStatement
//***************************************************************************

class cDbStatement : public cDbService
{
   public:

      cDbStatement(Table* aTable);
      virtual ~cDbStatement()  { clear(); }
      
      int execute(int noResult = no);
      int find();
      int fetch();
      int freeResult();

      // interface

      int build(const char* format, ...);

      void setBindPrefix(const char* p) { bindPrefix = p; }
      void clrBindPrefix()              { bindPrefix = 0; }
      int bind(cDbValue* value, int mode, const char* delim = 0);
      int bind(int field, int mode, const char* delim = 0);

      int bindCmp(const char* table, cDbValue* value,
                  const char* comp, const char* delim = 0);
      int bindCmp(const char* table, int field, cDbValue* value, 
                  const char* comp, const char* delim = 0);

      // ..

      int prepare();
      int getAffected()    { return affected; }
      const char* asText() { return stmtTxt.c_str(); }

   private:

      void clear();
      int appendBinding(cDbValue* value, BindType bt);

      string stmtTxt;
      MYSQL_STMT* stmt;
      int affected;
      cDbRow* row;
      Table* table;
      int inCount;
      MYSQL_BIND* inBind;   // to db
      int outCount;
      MYSQL_BIND* outBind;  // from db (result)
      MYSQL_RES* metaResult;
      const char* bindPrefix;
};

//***************************************************************************
// Class Database Row
//***************************************************************************

class cDbRow : public cDbService
{
   public:

      cDbRow(FieldDef* f)
      { 
         count = 0; 
         fieldDef = 0;
         useFields(f);

         dbValues = new cDbValue[count];

         for (int f = 0; f < count; f++)
            dbValues[f].setField(getField(f));
      }

      virtual ~cDbRow() { delete[] dbValues; }

      void clear()
      {
         for (int f = 0; f < count; f++)
            dbValues[f].clear();
      }

      virtual FieldDef* getField(int f)         { return f < 0 ? 0 : fieldDef+f; }
      virtual int fieldCount()                  { return count; }

      void setValue(int f, const char* value, 
                    int size = 0)               { dbValues[f].setValue(value, size); }
      void setValue(int f, double value)        { dbValues[f].setValue(value); }
      int hasValue(int f, const char* value)    { return dbValues[f].hasValue(value); }
      int hasValue(int f, double value)         { return dbValues[f].hasValue(value); }
      cDbValue* getValue(int f)                 { return &dbValues[f]; }

      const char* getStrValue(int f)      const { return dbValues[f].getStrValue(); }
      long getIntValue(int f)             const { return dbValues[f].getIntValue(); }
      float getFloatValue(int f)          const { return dbValues[f].getFloatValue(); }
      const char* getIntValueAsStr(int f) const { return dbValues[f].getIntValueAsStr(); }
      int isNull(int f)                   const { return dbValues[f].isNull(); }

   protected:

      virtual void useFields(FieldDef* f)  { fieldDef = f; for (count = 0; (fieldDef+count)->name; count++); } 

      int count;           // field count
      FieldDef* fieldDef;
      cDbValue* dbValues;
};

//***************************************************************************
// Connection
//***************************************************************************

class cDbConnection
{
   public:

      cDbConnection()
      {
         mysql = 0;
         attached = 0;
         inTact = no;
      }

      virtual ~cDbConnection()
      {
         if (mysql) mysql_close(mysql);
      }

      int attachConnection()
      { 
         if (!mysql)
         {
            if (!(mysql = mysql_init(0)))
            {
               connectDropped = yes;
               return errorSql(this, "attachConnection(init)");
            }

            if (!mysql_real_connect(mysql, dbHost, 
                                    dbUser, dbPass, dbName, dbPort, 0, 0)) 
            {
               mysql_close(mysql);
               mysql = 0;
               connectDropped = yes;
               tell(0, "Error, connecting to database at '%s' on port (%d) failed",
                    dbHost, dbPort);

               return fail;
            }

            // init encoding 
            
            if (encoding && *encoding)
            {
               if (mysql_set_character_set(mysql, encoding))
                  errorSql(this, "init(character_set)");

               tell(0, "SQL client character now '%s'", mysql_character_set_name(mysql));               
            }

            connectDropped = no;
         }

         attached++; 

         return success; 
      }

      void detachConnection()
      { 
         attached--;

         if (!attached)
         {
            mysql_close(mysql);
            mysql_thread_end();
            mysql = 0;
         }
      }

      virtual int query(const char* statement)
      {
         int status = 1;
         MYSQL* h = getMySql();

         if (h)
         {
            if ((status = mysql_query(h, statement)))
               errorSql(this, statement);
         }

         return status ? fail : success;
      }

      virtual int startTransaction()
      { 
         inTact = yes;
         return query("START TRANSACTION"); 
      }

      virtual int commit()
      { 
         inTact = no;
         return query("COMMIT");
      }

      virtual int rollback() 
      { 
         inTact = no;
         return query("ROLLBACK"); 
      }

      virtual int inTransaction() { return inTact; }

      MYSQL* getMySql()                              
      {
         if (connectDropped && mysql)
         {
            mysql_close(mysql);
            mysql_thread_end();
            mysql = 0;
            attached = 0;
         }

         return mysql; 
      }

      int getAttachedCount()                         { return attached; }

      // --------------
      // static stuff 

      // set/get connecting data

      static void setHost(const char* s)             { free(dbHost); dbHost = strdup(s); }
      static const char* getHost()                   { return dbHost; }
      static void setName(const char* s)             { free(dbName); dbName = strdup(s); }
      static const char* getName()                   { return dbName; }
      static void setUser(const char* s)             { free(dbUser); dbUser = strdup(s); }
      static const char* getUser()                   { return dbUser; }
      static void setPass(const char* s)             { free(dbPass); dbPass = strdup(s); }
      static const char* getPass()                   { return dbPass; }
      static void setPort(int port)                  { dbPort = port; }
      static int getPort()                           { return dbPort; }
      static void setEncoding(const char* enc)       { free(encoding); encoding = strdup(enc); }
      static const char* getEncoding()               { return encoding; }

      // 

      static int errorSql(cDbConnection* mysql, const char* prefix, MYSQL_STMT* stmt = 0, const char* stmtTxt = 0);

      static int init()
      {
         if (mysql_library_init(0, 0, 0)) 
         {
            tell(0, "Error: mysql_library_init failed");
            return fail;  // return errorSql(0, "init(library_init)");
         }

         return success;
      }

      static int exit()
      {
         mysql_library_end();

         free(dbHost);
         free(dbUser);
         free(dbPass);
         free(dbName);
         free(encoding);

         return done;
      }

      MYSQL* mysql;

   private:

      int initialized;
      int attached;
      int inTact;

      static int connectDropped;
      static char* encoding;

      // connecting data

      static char* dbHost;
      static int dbPort;
      static char* dbName;              // database name
      static char* dbUser;
      static char* dbPass;
};

//***************************************************************************
// Table
//***************************************************************************

class Table : cDbService
{
   public:

      Table(cDbConnection* aConnection, const char* name, FieldDef* f, ViewDef* v = 0);
      virtual ~Table();

      const char* TableName()                 { return tableName; }

      virtual int open();
      virtual int close();

      virtual int find();
      virtual int find(int viewIndex);
      virtual int fetch(int viewIndex);
      virtual void reset(int viewIndex = na);

      virtual int find(cDbStatement* stmt);
      virtual int fetch(cDbStatement* stmt);
      virtual void reset(cDbStatement* stmt);

      virtual int insert();
      virtual int update();
      virtual int store();

      virtual int query(const char* statement);
      virtual int deleteWhere(const char* where);
      virtual int countWhere(const char* where, int& count);
      virtual int truncate();

      // interface to cDbRow

      void clear()                                           { row->clear(); }
      cDbValue* getValue(int f)                            { return row->getValue(f); }
      void setValue(int f, const char* value, int size = 0)  { row->setValue(f, value, size); }
      void setValue(int f, double value)                     { row->setValue(f, value); }
      int hasValue(int f, const char* value)                 { return row->hasValue(f, value); }
      int hasValue(int f, double value)                      { return row->hasValue(f, value); }
      const char* getStrValue(int f)       const             { return row->getStrValue(f); }
      long getIntValue(int f)              const             { return row->getIntValue(f); }
      float getFloatValue(int f)           const             { return row->getFloatValue(f); }
      int isNull(int f)                    const             { return row->isNull(f); }

      FieldDef* getField(int f)                              { return row->getField(f); }
      int fieldCount()                                       { return row->fieldCount(); }
      cDbRow* getRow()                                       { return row; }

      cDbConnection* getConnection()                         { return connection; }
      MYSQL* getMySql()                                      { return connection->getMySql(); }
      int isConnected()                                      { return connection && connection->getMySql(); }

      // view

      virtual ViewDef* getView(int f)           { return viewDef+f; }
      virtual cDbStatement* getViewStmt(int f)  { return viewStatements[f]; }

      // static stuff

      static void setConfPath(const char* cpath)             { free(confPath); confPath = strdup(cpath); }

   protected:

      virtual int init();
      virtual int createTable();
      virtual int createView(ViewDef* view);
      virtual void copyValues(cDbRow* r);

      void useView(ViewDef* v);
      void freeViewStatements();
      int createViewStatements();

      // data

      const char* tableName;
      cDbRow* row;
      int holdInMemory;        // hold table additionally in memory (not implemented yet)

      // view 

      ViewDef* viewDef;
      cDbStatement** viewStatements;
      int viewCount;

      // basic statements

      cDbStatement* stmtSelect;
      cDbStatement* stmtInsert;
      cDbStatement* stmtUpdate;
      cDbConnection* connection;

      // statics

      static char* confPath;
};

//***************************************************************************
#endif //__DB_H
