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
#include <stdarg.h>

#include <sstream>

#include <mysql/mysql.h>

#include "common.h"

class cDbTable;
class cDbConnection;

using namespace std;

//***************************************************************************
// cDbService
//***************************************************************************

class cDbService
{
   public:

      enum Misc
      {
         maxIndexFields = 20
      };

      enum FieldFormat
      {
         ffInt,
         ffUInt,
         ffBigInt,
         ffUBigInt,
         ffAscii,      // -> VARCHAR
         ffText,
         ffMlob,       // -> MEDIUMBLOB
         ffFloat,
         ffDateTime,
         ffCount
      };

      enum FieldType
      {
         ftData       = 1,
         ftPrimary    = 2,
         ftMeta       = 4,
         ftCalc       = 8,
         ftAutoinc    = 16,
         ftDef0       = 32
      };

      struct FieldDef
      {
         const char* name;
         FieldFormat format;
         int size;
         int index;
         int type;
      };

      enum BindType
      {
         bndIn  = 0x001,
         bndOut = 0x002,
         bndSet = 0x004
      };

      enum ProcType
      {
         ptProcedure, 
         ptFunction
      };

      struct IndexDef
      {
         const char* name;
         int fields[maxIndexFields+1];
         int order;                     // not implemented yet
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
         strValue = 0;

         ownField = new FieldDef;
         ownField->name = strdup(name);
         ownField->format = format;
         ownField->size = size;
         ownField->type = ftData;

         field = ownField;

         clear();
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
         longlongValue = 0;
         floatValue = 0;
         memset(&timeValue, 0, sizeof(timeValue));

         nullValue = 1;
         initialized = no;
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
            tell(0, "Setting invalid field format for '%s', expected '%s' got string", 
                 field->name, cDBS::toString(field->format));
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

      void setCharValue(char value)
      {
         char tmp[2];
         tmp[0] = value;
         tmp[1] = 0;
         
         setValue(tmp);
      }

      void setIntValue(long long value)
      { 
         if (field->format == ffInt || field->format == ffUInt)
         {
            numValue = value;
            nullValue = 0;
         }
         
         else if (field->format == ffBigInt || field->format == ffUBigInt)
         {
            longlongValue = value;
            nullValue = 0;
         }
      }

      void setValue(double value)
      { 
         if (field->format == ffInt || field->format == ffUInt)
         {
            numValue = value;
            nullValue = 0;
         }
         else if (field->format == ffBigInt || field->format == ffUBigInt)
         {
            longlongValue = value;
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

            memset(&tm, 0, sizeof(tm));
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
         if (field->format == ffInt || field->format == ffUInt)
            return numValue == value;

         if (field->format == ffBigInt || field->format == ffUBigInt)
            return longlongValue == value;

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
         
         memset(&tm, 0, sizeof(tm));

         tm.tm_isdst = -1;                   // force DST auto detect
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
      long long getIntValue()              
      { 
         if (isNull())
            return 0;

         return field->format == ffBigInt || field->format == ffUBigInt ? longlongValue : numValue; 
      }

      float getDoubleValue()               { return !isNull() ? floatValue : 0; }
      int isNull()                         { return nullValue; }

      char* getStrValueRef()               { return strValue; }
      long* getIntValueRef()               { return &numValue; }
      long long* getBigIntValueRef()       { return &longlongValue; }

      MYSQL_TIME* getTimeValueRef()        { return &timeValue; }
      float* getFloatValueRef()            { return &floatValue; }
      my_bool* getNullRef()                { return &nullValue; }

   private:

      FieldDef* ownField;
      FieldDef* field;
      long numValue;
      long long longlongValue;
      float floatValue;
      MYSQL_TIME timeValue;
      char* strValue;
      unsigned long strValueSize;
      my_bool nullValue;
      int initialized;
};

//***************************************************************************
// cDbStatement
//***************************************************************************

class cDbStatement : public cDbService
{
   public:

      cDbStatement(cDbTable* aTable);
      cDbStatement(cDbConnection* aConnection, const char* stmt = "");

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
      int getResultCount();
      const char* asText() { return stmtTxt.c_str(); }

   private:

      void clear();
      int appendBinding(cDbValue* value, BindType bt);

      string stmtTxt;
      MYSQL_STMT* stmt;
      int affected;
      cDbConnection* connection;
      cDbTable* table;
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

      virtual FieldDef* getField(int f)            { return f < 0 ? 0 : fieldDef+f; }
      virtual int fieldCount()                     { return count; }

      void setValue(int f, const char* value, 
                    int size = 0)                  { dbValues[f].setValue(value, size); }
      void setValue(int f, double value)           { dbValues[f].setValue(value); }
      void setIntValue(int f, long long value)     { dbValues[f].setIntValue(value); }
      void setCharValue(int f, char value)         { dbValues[f].setCharValue(value); }
      int hasValue(int f, double value)      const { return dbValues[f].hasValue(value); }
      int hasValue(int f, const char* value) const { return dbValues[f].hasValue(value); }
      cDbValue* getValue(int f)                    { return &dbValues[f]; }

      const char* getStrValue(int f)         const { return dbValues[f].getStrValue(); }
      long long getIntValue(int f)           const { return dbValues[f].getIntValue(); }
      float getDoubleValue(int f)            const { return dbValues[f].getDoubleValue(); }
      int isNull(int f)                      const { return dbValues[f].isNull(); }

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
         connectDropped = yes;
      }

      virtual ~cDbConnection()
      {
         if (mysql) 
         {
            mysql_close(mysql);
            mysql_thread_end();
         }
      }

      int attachConnection()
      { 
         static int first = yes;

         if (!mysql)
         {
            connectDropped = yes;

            if (!(mysql = mysql_init(0)))
               return errorSql(this, "attachConnection(init)");

            if (!mysql_real_connect(mysql, dbHost, 
                                    dbUser, dbPass, dbName, dbPort, 0, 0)) 
            {
               mysql_close(mysql);
               mysql = 0;
               tell(0, "Error, connecting to database at '%s' on port (%d) failed",
                    dbHost, dbPort);

               return fail;
            }

            connectDropped = no;

            // init encoding 
            
            if (encoding && *encoding)
            {
               if (mysql_set_character_set(mysql, encoding))
                  errorSql(this, "init(character_set)");

               if (first)
               {
                  tell(0, "SQL client character now '%s'", mysql_character_set_name(mysql));
                  first = no;
               }
            }
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

      int isConnected() { return getMySql() > 0; }

      int check()
      {
         if (!isConnected())
            return fail;

         query("SELECT SYSDATE();");
         queryReset();

         return isConnected() ? success : fail;
      }

      virtual int query(const char* format, ...)
      {
         va_list more;
         
         if (format)
            va_start(more, format);

         return vquery(format, more);
      }

      virtual int query(int& count, const char* format, ...)
      {
         int status;
         va_list more;
         
         count = 0;

         if (format)
            va_start(more, format);

         if ((status = vquery(format, more)) == success)
         {
            MYSQL_RES* res;
            MYSQL_ROW data;
            
            // get affected rows ..
            
            if ((res = mysql_store_result(getMySql())))
            {
               data = mysql_fetch_row(res);
               
               if (data)
                  count = atoi(data[0]);
               
               mysql_free_result(res);
            }
         }

         return status; 
      }

      virtual int vquery(const char* format, va_list more)
      {
         int status = 1;
         MYSQL* h = getMySql();

         if (h && format)
         {
            char* stmt;
            
            vasprintf(&stmt, format, more);
            
            if ((status = mysql_query(h, stmt)))
               errorSql(this, stmt);

            free(stmt);
         }

         return status ? fail : success;
      }

      virtual void queryReset()
      {
         if (getMySql())
         {
            MYSQL_RES* result = mysql_use_result(getMySql());
            mysql_free_result(result);
         }
      }

      virtual int executeSqlFile(const char* file)
      {
         FILE* f;
         int res;
         char* buffer;
         int size = 1000;
         int nread = 0;

         if (!getMySql())
            return fail;
         
         if (!(f = fopen(file, "r")))
         {
            tell(0, "Fatal: Can't access '%s'; %s", file, strerror(errno));
            return fail;
         }
         
         buffer = (char*)malloc(size+1);
   
         while ((res = fread(buffer+nread, 1, 1000, f)))
         {
            char* b;

            nread += res;
            size += 1000;

            if (!(b = (char*)realloc(buffer, size+1)))
               free(buffer);
                
            buffer = b;
         }
         
         fclose(f);
         buffer[nread] = 0;
         
         // execute statement
         
         tell(2, "Executing '%s'", buffer);
         
         if (query("%s", buffer))
         {
            free(buffer);
            return errorSql(this, "executeSqlFile()");
         }

         free(buffer);

         return success;
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

      int errorSql(cDbConnection* mysql, const char* prefix, MYSQL_STMT* stmt = 0, const char* stmtTxt = 0);

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
      int connectDropped;

      static char* encoding;

      // connecting data

      static char* dbHost;
      static int dbPort;
      static char* dbName;              // database name
      static char* dbUser;
      static char* dbPass;
};

//***************************************************************************
// cDbTable
//***************************************************************************

class cDbTable : public cDbService
{
   public:

      cDbTable(cDbConnection* aConnection, const char* aName, FieldDef* f, IndexDef* i = 0);
      virtual ~cDbTable();

      virtual const char* TableName() { return tableName; }

      virtual int open();
      virtual int close();

      virtual int find();
      virtual void reset() { reset(stmtSelect); }

      virtual int find(cDbStatement* stmt);
      virtual int fetch(cDbStatement* stmt);
      virtual void reset(cDbStatement* stmt);

      virtual int insert();
      virtual int update();
      virtual int store();

      virtual int deleteWhere(const char* where, int& count);
      virtual int countWhere(const char* where, int& count, const char* what = 0);
      virtual int truncate();

      // interface to cDbRow

      void clear()                                           { row->clear(); }
      void setValue(int f, const char* value, int size = 0)  { row->setValue(f, value, size); }
      void setValue(int f, double value)                     { row->setValue(f, value); }
      void setIntValue(int f, long long value)               { row->setIntValue(f, value); }
      void setCharValue(int f, char value)                   { row->setCharValue(f, value); }
      int hasValue(int f, const char* value)                 { return row->hasValue(f, value); }
      int hasValue(int f, double value)                      { return row->hasValue(f, value); }
      const char* getStrValue(int f)       const             { return row->getStrValue(f); }
      long long getIntValue(int f)         const             { return row->getIntValue(f); }
      float getDoubleValue(int f)          const             { return row->getDoubleValue(f); }
      int isNull(int f)                    const             { return row->isNull(f); }

      cDbValue* getValue(int f)                              { return row->getValue(f); }
      FieldDef* getField(int f)                              { return row->getField(f); }
      int fieldCount()                                       { return row->fieldCount(); }
      cDbRow* getRow()                                       { return row; }

      cDbConnection* getConnection()                         { return connection; }
      MYSQL* getMySql()                                      { return connection->getMySql(); }
      int isConnected()                                      { return connection && connection->getMySql(); }

      virtual IndexDef* getIndex(int i)                      { return indices+i; }
      virtual int exist(const char* name = 0);
      virtual int createTable();

      // static stuff

      static void setConfPath(const char* cpath)             { free(confPath); confPath = strdup(cpath); }

   protected:

      virtual int init();
      virtual int createIndices();
      virtual int checkIndex(const char* idxName, int& fieldCount);

      virtual void copyValues(cDbRow* r);

      // data

      char* tableName;
      cDbRow* row;
      int holdInMemory;        // hold table additionally in memory (not implemented yet)

      IndexDef* indices;

      // basic statements

      cDbStatement* stmtSelect;
      cDbStatement* stmtInsert;
      cDbStatement* stmtUpdate;
      cDbConnection* connection;

      // statics

      static char* confPath;
};

//***************************************************************************
// cDbView
//***************************************************************************

class cDbView : public cDbService
{
   public:

      cDbView(cDbConnection* c, const char* aName)
      {
         connection = c;
         name = strdup(aName);
      }

      ~cDbView() { free(name); }

      int exist()
      {
         if (connection->getMySql())
         {
            MYSQL_RES* result = mysql_list_tables(connection->getMySql(), name);
            MYSQL_ROW tabRow = mysql_fetch_row(result);
            mysql_free_result(result);
         
            return tabRow ? yes : no;
         }

         return no;
      }

      int create(const char* path, const char* sqlFile)
      {
         int status;
         char* file = 0;
         
         asprintf(&file, "%s/%s", path, sqlFile);
         
         tell(0, "Creating view '%s' using definition in '%s'", 
              name, file);
         
         status = connection->executeSqlFile(file);
         
         free(file);
         
         return status;
      }

      int drop()
      {
         tell(0, "Drop view '%s'", name);

         return connection->query("drop view %s", name);
      }

   protected:

      cDbConnection* connection;
      char* name;
};

//***************************************************************************
// cDbProcedure
//***************************************************************************

class cDbProcedure : public cDbService
{
   public:

      cDbProcedure(cDbConnection* c, const char* aName, ProcType pt = ptProcedure)
      {
         connection = c;
         type = pt;
         name = strdup(aName);
      }

      ~cDbProcedure() { free(name); }

      const char* getName() { return name; }

      int call(int ll = 1)
      {
         if (!connection || !connection->getMySql())
            return fail;

         cDbStatement stmt(connection);

         tell(ll, "Calling '%s'", name);

         stmt.build("call %s", name);

         if (stmt.prepare() != success || stmt.execute() != success)
            return fail;

         tell(ll, "'%s' suceeded", name);

         return success;
      }

      int exist()
      {
         if (!connection || !connection->getMySql())
            return fail;

         cDbStatement stmt(connection);
         
         stmt.build("show %s status where name = '%s'", 
                    type == ptProcedure ? "procedure" : "function", name);
         
         if (stmt.prepare() != success || stmt.execute() != success)
         {
            tell(0, "%s check of '%s' failed",
                 type == ptProcedure ? "Procedure" : "Function", name);
            return no;
         }
         else
         {  
            if (stmt.getResultCount() != 1)
               return no;
         }

         return yes;
      }

      int create(const char* path)
      {
         int status;
         char* file = 0;

         asprintf(&file, "%s/%s.sql", path, name);

         tell(1, "Creating %s '%s'", 
              type == ptProcedure ? "procedure" : "function", name);

         status = connection->executeSqlFile(file);

         free(file);

         return status;
      }

      int drop()
      {
         tell(1, "Drop %s '%s'", 
              type == ptProcedure ? "procedure" : "function", name);

         return connection->query("drop %s %s", 
                                  type == ptProcedure ? "procedure" : "function", name);
      }

   protected:

      cDbConnection* connection;
      ProcType type;
      char* name;
      
};

//***************************************************************************
#endif //__DB_H
