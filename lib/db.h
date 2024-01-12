/*
 * db.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __DB_H
#define __DB_H

#include <linux/unistd.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include <mysql.h>

#include <list>

#include "common.h"
#include "dbdict.h"

class cDbTable;
class cDbConnection;

//***************************************************************************
// cDbValue
//***************************************************************************

class cDbValue : public cDbService
{
   public:

      cDbValue(cDbFieldDef* f = 0)
      {
         field = 0;
         strValue = 0;
         ownField = 0;
         changed = 0;

         if (f) setField(f);
      }

      cDbValue(const char* name, FieldFormat format, int size)
      {
         strValue = 0;
         changed = 0;
         ownField = new cDbFieldDef(name, name, format, size, ftData, 0);

         field = ownField;
         strValue = (char*)calloc(field->getSize()+TB, sizeof(char));

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
         changed = 0;
      }

      void clearChanged()
      {
         changed = 0;
      }

      virtual void setField(cDbFieldDef* f)
      {
         free();
         field = f;

         if (field)
            strValue = (char*)calloc(field->getSize()+TB, sizeof(char));
      }

      virtual cDbFieldDef* getField()      { return field; }
      virtual const char* getName()        { return field->getName(); }
      virtual const char* getDbName()      { return field->getDbName(); }

      void setNull()
      {
         int c = changed;
         int n = nullValue;

         clear();
         changed = c;

         if (!n)
            changed++;
      }

      void __attribute__ ((format(printf, 2, 3))) sPrintf(const char* format, ...)
      {
         va_list more;
         char* buf = 0;

         if (!format)
            return ;

         va_start(more, format);
         vasprintf(&buf, format, more);

         setValue(buf);

         ::free(buf);
      }

      void setValue(const char* value, int size = 0)
      {
         int modified = no;

         if (field->getFormat() != ffAscii && field->getFormat() != ffText &&
             field->getFormat() != ffMText && field->getFormat() != ffMlob)
         {
            tell(eloAlways, "Setting invalid field format for '%s', expected ASCII, TEXT or MLOB",
                 field->getName());
            return;
         }

         if (field->getFormat() == ffMlob && !size)
         {
            tell(eloAlways, "Missing size for MLOB field '%s'", field->getName());
            return;
         }

         if (value && size)
         {
            if (size > field->getSize())
            {
               tell(eloAlways, "Warning, size of %d for '%s' exeeded, got %d bytes!",
                    field->getSize(), field->getName(), size);

               size = field->getSize();
            }

            if (memcmp(strValue, value, size) != 0 || isNull())
               modified = yes;

            clear();
            memcpy(strValue, value, size);
            strValue[size] = 0;
            strValueSize = size;
            nullValue = 0;
         }

         else if (value)
         {
            if (strlen(value) > (size_t)field->getSize())
               tell(eloAlways, "Warning, size of %d for '%s' exeeded (needed %ld) [%s]",
                    field->getSize(), field->getName(), (long)strlen(value), value);

            if (strcmp(strValue, value) != 0 || isNull())
               modified = yes;

            clear();
            sprintf(strValue, "%.*s", field->getSize(), value);
            strValueSize = strlen(strValue);
            nullValue = 0;
         }

         if (modified)     // increment changed after calling clear()
            changed++;
      }

      void setCharValue(char value)
      {
         char tmp[2] = "";
         tmp[0] = value;
         tmp[1] = 0;
         setValue(tmp);
      }

      void setValue(int value)
      {
         setValue((long)value);
      }

      void setValue(long value)
      {
         if (field->getFormat() == ffInt || field->getFormat() == ffUInt)
         {
            if (numValue != value || isNull())
               changed++;

            numValue = value;
            nullValue = 0;
         }
         else if (field->getFormat() == ffDateTime)
         {
            struct tm tm;
            time_t v = value;
            time_t o = getTimeValue();

            memset(&tm, 0, sizeof(tm));
            localtime_r(&v, &tm);

            timeValue.year = tm.tm_year + 1900;
            timeValue.month = tm.tm_mon + 1;
            timeValue.day = tm.tm_mday;

            timeValue.hour = tm.tm_hour;
            timeValue.minute = tm.tm_min;
            timeValue.second = tm.tm_sec;

            nullValue = 0;

            if (o != getTimeValue())
               changed++;
         }
         else
         {
            tell(eloAlways, "Setting invalid field format for '%s'", field->getName());
         }
      }

      void setValue(long long value)
      {
         if (field->getFormat() == ffInt || field->getFormat() == ffUInt)
         {
            if (numValue != value || isNull())
               changed++;

            numValue = value;
            nullValue = 0;
         }
         else
         {
            tell(eloAlways, "Setting invalid field format for '%s'", field->getName());
         }
      }

      void setValue(double value)
      {
         if (field->getFormat() == ffInt || field->getFormat() == ffUInt)
         {
            if (numValue != value || isNull())
               changed++;

            numValue = value;
            nullValue = 0;
         }
         else if (field->getFormat() == ffBigInt || field->getFormat()  == ffUBigInt)
         {
            if (longlongValue != value || isNull())
               changed++;

            longlongValue = value;
            nullValue = 0;
         }
         else if (field->getFormat() == ffFloat)
         {
            if (floatValue != value || isNull())
               changed++;

            floatValue = value;
            nullValue = 0;
         }
         else
         {
            tell(eloAlways, "Setting invalid field format for '%s'", field->getName());
         }
      }

      void setBigintValue(int64_t value)
      {
         if (field->getFormat() == ffInt || field->getFormat() == ffUInt)
         {
            if (numValue != value)
               changed++;

            numValue = value;
            nullValue = 0;
         }

         else if (field->getFormat() == ffBigInt || field->getFormat() == ffUBigInt)
         {
            if (longlongValue != value)
               changed++;

            longlongValue = value;
            nullValue = 0;
         }
      }

      int hasValue(long value)
      {
         if (field->getFormat() == ffInt || field->getFormat() == ffUInt)
            return numValue == value;

         if (field->getFormat() == ffDateTime)
            return no; // to be implemented!

         tell(eloAlways, "Setting invalid field format for '%s'", field->getName());

         return no;
      }

      int hasValue(double value)
      {
         if (field->getFormat() == ffInt || field->getFormat() == ffUInt)
            return numValue == value;

         if (field->getFormat() == ffBigInt || field->getFormat() == ffUBigInt)
            return longlongValue == value;

         if (field->getFormat() == ffFloat)
            return floatValue == value;

         tell(eloAlways, "Setting invalid field format for '%s'", field->getName());

         return no;
      }

      int hasValue(const char* value)
      {
         if (!value)
            value = "";

         if (field->getFormat() != ffAscii && field->getFormat() != ffText &&
             field->getFormat() != ffMText && field->getFormat() != ffMlob)
         {
            tell(eloAlways, "Checking invalid field format for '%s', expected ASCII or TEXT",
                 field->getName());
            return no;
         }

         return strcmp(getStrValue(), value) == 0;
      }

      int hasCharValue(char value)
      {
         if (field->getFormat() != ffAscii)
         {
            tell(eloAlways, "Checking invalid field format for '%s', expected ASCII or TEXT",
                 field->getName());
            return no;
         }

         return getStrValueSize() == 1 && getCharValue() == value;
      }

      time_t getTimeValue()
      {
         struct tm tm;
         memset(&tm, 0, sizeof(tm));

         tm.tm_isdst = -1;               // force DST auto detect
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
      char getCharValue()                  { return !isNull() && strValue ? strValue[0] : 0; }
      long getIntValue()                   { return !isNull() ? numValue : 0; }

      int64_t getBigintValue()
      {
         if (isNull())
            return 0;

         if (field->getFormat() == ffBigInt || field->getFormat() == ffUBigInt)
            return longlongValue;

         return numValue;
      }

      float getFloatValue()                { return !isNull() ? floatValue : 0; }
      int isNull()                         { return nullValue; }
      int getChanges()                     { return changed; }

      int isEmpty()
      {
         if (isNull())
            return yes;

         if (field->getFormat() == ffInt || field->getFormat() == ffUInt)
            return numValue == 0;
         else if (field->getFormat() == ffDateTime)
            return no;
         else if (field->getFormat() == ffAscii || field->getFormat() == ffText ||
                  field->getFormat() == ffMText || field->getFormat() == ffMlob)
            return ::isEmpty(strValue);
         else if (field->getFormat() == ffFloat)
            return floatValue == 0;

         return no;
      }

      char* getStrValueRef()               { return strValue; }
      long* getIntValueRef()               { return &numValue; }
      int64_t* getBigIntValueRef()         { return &longlongValue; }
      MYSQL_TIME* getTimeValueRef()        { return &timeValue; }
      float* getFloatValueRef()            { return &floatValue; }
      my_bool* getNullRef()                { return &nullValue; }

   private:

      cDbFieldDef* ownField;
      cDbFieldDef* field;
      long numValue;
      int64_t longlongValue;
      float floatValue;
      MYSQL_TIME timeValue;
      char* strValue;
      unsigned long strValueSize;
      my_bool nullValue;
      int changed;
};

//***************************************************************************
// cDbStatement
//***************************************************************************

class cDbStatement : public cDbService
{
   public:

      cDbStatement(cDbTable* aTable);
      cDbStatement(cDbConnection* aConnection, const char* sText = "");
      virtual ~cDbStatement();

      int execute(int noResult = no);
      int find();
      int fetch();
      int freeResult();
      void clear();

      // interface

      virtual int __attribute__ ((format(printf, 2, 3))) build(const char* format, ...);

      void setBindPrefix(const char* p) { bindPrefix = p; }
      void clrBindPrefix()              { bindPrefix = 0; }
      int bind(const char* fname, int mode, const char* delim = 0);
      int bind(cDbValue* value, int mode, const char* delim = 0);
      int bind(cDbTable* aTable, cDbFieldDef* field, int mode, const char* delim);
      int bind(cDbTable* aTable, const char* fname, int mode, const char* delim);
      int bind(cDbFieldDef* field, int mode, const char* delim = 0);
      int bindAllOut(const char* delim = 0);

      int bindCmp(const char* ctable, cDbValue* value, const char* comp, const char* delim = 0);
      int bindCmp(const char* ctable, cDbFieldDef* field, cDbValue* value, const char* comp, const char* delim = 0);
      int bindCmp(const char* ctable, const char* fname, cDbValue* value, const char* comp, const char* delim = 0);

      int bindText(const char* text, cDbValue* value, const char* comp, const char* delim = 0);
      int bindTextFree(const char* text, cDbValue* value, const char* delim = 0, int mode = bndIn);
      int bindInChar(const char* ctable, const char* fname, cDbValue* value = 0, const char* delim = 0);

      int appendBinding(cDbValue* value, BindType bt);  // use this interface method seldom from external and with care!

      // ..

      int prepare();
      int getAffected()    { return affected; }
      int getResultCount();
      int getLastInsertId();
      const char* asText() { return stmtTxt.c_str(); }
      cDbTable* getTable() { return table; }
      void showStat();

      // data

      static int explain;         // debug explain

   private:

      std::string stmtTxt;
      MYSQL_STMT* stmt;
      int affected;
      cDbConnection* connection;
      cDbTable* table;
      int inCount;
      MYSQL_BIND* inBind;         // to db
      int outCount;
      MYSQL_BIND* outBind;        // from db (result)
      MYSQL_RES* metaResult;
      const char* bindPrefix;
      int firstExec;              // debug explain
      int buildErrors;

      unsigned long callsPeriod;
      unsigned long callsTotal;
      double duration;
};

//***************************************************************************
// cDbStatements
//***************************************************************************

class cDbStatements
{
   public:

      cDbStatements()  { statisticPeriod = time(0); }
      ~cDbStatements() {};

      void append(cDbStatement* s)  { statements.push_back(s); }
      void remove(cDbStatement* s)  { statements.remove(s); }

      void showStat(const char* name)
      {
         tell(eloAlways, "Statement statistic of last %ld seconds from '%s':", time(0) - statisticPeriod, name);

         for (std::list<cDbStatement*>::iterator it = statements.begin() ; it != statements.end(); ++it)
         {
            if (*it)
               (*it)->showStat();
         }

         statisticPeriod = time(0);
      }

   private:

      time_t statisticPeriod {0};
      std::list<cDbStatement*> statements;
};

//***************************************************************************
// Class Database Row
//***************************************************************************

#define GET_FIELD(name) \
   cDbFieldDef* f = tableDef->getField(name);  \
   if (!f) \
   { \
      tell(eloAlways, "Fatal: Field '%s.%s' not defined (missing in dictionary)", tableDef->getName(), name); \
      return ; \
   } \

#define GET_FIELD_RES(name, def) \
   cDbFieldDef* f = tableDef->getField(name);  \
   if (!f) \
   { \
      tell(eloAlways, "Fatal: Field '%s.%s' not defined (missing in dictionary)", tableDef->getName(), name); \
      return def; \
   } \

class cDbRow : public cDbService
{
   public:

      cDbRow(cDbTableDef* t)
      {
         set(t);
      }

      cDbRow(const char* name)
      {
         cDbTableDef* t = dbDict.getTable(name);

         if (t)
            set(t);
         else
            tell(eloAlways, "Fatal: Table '%s' missing in dictionary '%s'!", name, dbDict.getPath());
      }

      virtual ~cDbRow() { delete[] dbValues; }

      void set(cDbTableDef* t)
      {
         std::map<std::string, cDbFieldDef*>::iterator f;

         tableDef = t;
         dbValues = new cDbValue[tableDef->fieldCount()];

         for (f = tableDef->dfields.begin(); f != tableDef->dfields.end(); ++f)
            dbValues[f->second->getIndex()].setField(f->second);
      }

      void clear()
      {
         for (int f = 0; f < tableDef->fieldCount(); f++)
            dbValues[f].clear();
      }

      void clearChanged()
      {
         for (int f = 0; f < tableDef->fieldCount(); f++)
            dbValues[f].clearChanged();
      }

      int getChanges()
      {
         int count = 0;

         for (int f = 0; f < tableDef->fieldCount(); f++)
            count += dbValues[f].getChanges();

         return count;
      }

      std::string getChangedFields()
      {
         std::string s = "";

         for (int f = 0; f < tableDef->fieldCount(); f++)
         {
            if (dbValues[f].getChanges())
            {
               if (s.length())
                  s += ",";

               s += dbValues[f].getName() + std::string("=");

               if (dbValues[f].getField()->hasFormat(ffInt) || dbValues[f].getField()->hasFormat(ffUInt))
                  s += num2Str((int)dbValues[f].getIntValue());
               else
                  s += dbValues[f].getStrValue();
            }
         }

         return s;
      }

      virtual cDbFieldDef* getField(int id)                     { return tableDef->getField(id); }
      virtual cDbFieldDef* getField(const char* name)           { return tableDef->getField(name); }
      virtual cDbFieldDef* getFieldByDbName(const char* dbname) { return tableDef->getFieldByDbName(dbname); }
      virtual int fieldCount()                                  { return tableDef->fieldCount(); }

      void setValue(cDbFieldDef* f, const char* value,
                    int size = 0)                           { dbValues[f->getIndex()].setValue(value, size); }
      void setValue(cDbFieldDef* f, int value)              { dbValues[f->getIndex()].setValue(value); }
      void setValue(cDbFieldDef* f, long value)             { dbValues[f->getIndex()].setValue(value); }
      void setValue(cDbFieldDef* f, double value)           { dbValues[f->getIndex()].setValue(value); }
      void setBigintValue(cDbFieldDef* f, int64_t value)    { dbValues[f->getIndex()].setBigintValue(value); }
      void setCharValue(cDbFieldDef* f, char value)         { dbValues[f->getIndex()].setCharValue(value); }

      void setValue(const char* n, const char* value,
                    int size = 0)                           { GET_FIELD(n); dbValues[f->getIndex()].setValue(value, size); }
      void setValue(const char* n, int value)               { GET_FIELD(n); dbValues[f->getIndex()].setValue(value); }
      void setValue(const char* n, long value)              { GET_FIELD(n); dbValues[f->getIndex()].setValue(value); }
      void setValue(const char* n, long long value)         { GET_FIELD(n); dbValues[f->getIndex()].setValue(value); }
      void setValue(const char* n, double value)            { GET_FIELD(n); dbValues[f->getIndex()].setValue(value); }
      void setBigintValue(const char* n, int64_t value)     { GET_FIELD(n); dbValues[f->getIndex()].setBigintValue(value); }
      void setCharValue(const char* n, char value)          { GET_FIELD(n); dbValues[f->getIndex()].setCharValue(value); }

      int hasValue(cDbFieldDef* f, const char* value) const { return dbValues[f->getIndex()].hasValue(value); }
      int hasCharValue(cDbFieldDef* f, char  value)   const { return dbValues[f->getIndex()].hasCharValue(value); }
      int hasValue(cDbFieldDef* f, long value)        const { return dbValues[f->getIndex()].hasValue(value); }
      int hasValue(cDbFieldDef* f, double value)      const { return dbValues[f->getIndex()].hasValue(value); }

      int hasValue(const char* n, const char* value)  const { GET_FIELD_RES(n, no); return dbValues[f->getIndex()].hasValue(value); }
      int hasCharValue(const char* n, char value)     const { GET_FIELD_RES(n, no); return dbValues[f->getIndex()].hasCharValue(value); }
      int hasValue(const char* n, long value)         const { GET_FIELD_RES(n, no); return dbValues[f->getIndex()].hasValue(value); }
      int hasValue(const char* n, double value)       const { GET_FIELD_RES(n, no); return dbValues[f->getIndex()].hasValue(value); }

      cDbValue* getValue(cDbFieldDef* f)                    { return &dbValues[f->getIndex()]; }
      cDbValue* getValue(const char* n)                     { GET_FIELD_RES(n, 0); return &dbValues[f->getIndex()]; }

      time_t  getTimeValue(cDbFieldDef* f)            const { return dbValues[f->getIndex()].getTimeValue(); }
      const char* getStrValue(cDbFieldDef* f)         const { return dbValues[f->getIndex()].getStrValue(); }
      long getIntValue(cDbFieldDef* f)                const { return dbValues[f->getIndex()].getIntValue(); }
      int64_t getBigintValue(cDbFieldDef* f)          const { return dbValues[f->getIndex()].getBigintValue(); }
      float getFloatValue(cDbFieldDef* f)             const { return dbValues[f->getIndex()].getFloatValue(); }
      int isNull(cDbFieldDef* f)                      const { return dbValues[f->getIndex()].isNull(); }

      time_t  getTimeValue(const char* n)             const { GET_FIELD_RES(n, 0);   return dbValues[f->getIndex()].getTimeValue(); }
      const char* getStrValue(const char* n)          const { GET_FIELD_RES(n, "");  return dbValues[f->getIndex()].getStrValue(); }
      long getIntValue(const char* n)                 const { GET_FIELD_RES(n, 0);   return dbValues[f->getIndex()].getIntValue(); }
      int64_t getBigintValue(const char* n)           const { GET_FIELD_RES(n, 0);   return dbValues[f->getIndex()].getBigintValue(); }
      float getFloatValue(const char* n)              const { GET_FIELD_RES(n, 0);   return dbValues[f->getIndex()].getFloatValue(); }
      int isNull(const char* n)                       const { GET_FIELD_RES(n, yes); return dbValues[f->getIndex()].isNull(); }

      cDbTableDef* getTableDef()                      { return tableDef; }

   protected:

      cDbTableDef* tableDef;
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
         close();
      }

      int isConnected() { return getMySql() != 0; }

      int attachConnection()
      {
         if (!mysql)
         {
            connectDropped = yes;

            tell(eloDb, "Calling mysql_init(%ld)", syscall(__NR_gettid));

            if (!(mysql = mysql_init(0)))
               return errorSql(this, "attachConnection(init)");

            if (!mysql_real_connect(mysql, dbHost, dbUser, dbPass, dbName, dbPort, 0, 0))
            {
               errorSql(this, "connecting to database");
               tell(eloAlways, "Error, connecting to database at '%s' on port (%d) failed", dbHost, dbPort);
               close();
               return fail;
            }

            connectDropped = no;

            // init encoding

            if (encoding && *encoding)
            {
               static int first {yes};

               if (mysql_set_character_set(mysql, encoding))
                  errorSql(this, "init(character_set)");

               if (first)
               {
                  tell(eloAlways, "SQL client character now '%s'", mysql_character_set_name(mysql));
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
            close();
      }

      void close()
      {
         if (mysql)
         {
            tell(eloDb, "Closing mysql connection and calling mysql_thread_end(%ld)", syscall(__NR_gettid));

            mysql_close(mysql);
            mysql_thread_end();
            mysql = 0;
            attached = 0;
         }
      }

      int check()
      {
         if (!isConnected())
            return fail;

         query("SELECT SYSDATE();");
         queryReset();

         return isConnected() ? success : fail;
      }

      virtual int __attribute__ ((format(printf, 2, 3))) query(const char* format, ...)
      {
         va_list more;

         if (!format)
            return fail;

         va_start(more, format);

         return vquery(format, more);
      }

      virtual int __attribute__ ((format(printf, 3, 4))) query(int& count, const char* format, ...)
      {
         int status;
         va_list more;

         count = 0;

         if (!format)
            return fail;

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

      // escapeSqlString - only need to be used in string statements not in bind values!!

      virtual std::string escapeSqlString(const char* str)
      {
         std::string result = "";

         if (!isConnected())
            return result;

         int length = strlen(str);
         int bufferSize = length*2 + TB;

         char* buffer = (char*)malloc(bufferSize);
         mysql_real_escape_string(getMySql(), buffer, str, length);
         result = buffer;
         free(buffer);

         return result;
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
            tell(eloAlways, "Fatal: Can't execute sql file '%s'; Error was '%s'", file, strerror(errno));
            return fail;
         }

         buffer = (char*)malloc(size+1);

         while ((res = fread(buffer+nread, 1, 1000, f)))
         {
            nread += res;
            size += 1000;
            buffer = srealloc(buffer, size+1);
         }

         fclose(f);
         buffer[nread] = 0;

         // execute statement

         tell(eloDebug, "Executing '%s'", buffer);

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
         if (connectDropped)
            close();

         return mysql;
      }

      int getAttachedCount()                         { return attached; }
      void showStat(const char* name = "")           { statements.showStat(name); }
      int errorSql(cDbConnection* mysql, const char* prefix, MYSQL_STMT* stmt = 0, const char* stmtTxt = 0);

      // data

      cDbStatements statements;         // all statements of this connection

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
      static void setConfPath(const char* cpath)     { free(confPath); confPath = strdup(cpath); }
      static const char* getConfPath()               { return confPath; }

      // -----------------------------------------------------------
      // init() and exit() must exactly called 'once' per process

      static int init()
      {
         int status = success;

         initMutex.Lock();

         if (!initThreads)
         {
            tell(eloDebugDb, "Info: Calling mysql_library_init()");

            if (mysql_library_init(0, 0, 0))
            {
               tell(eloAlways, "Error: mysql_library_init() failed");
               status = fail;
            }
         }
         else
         {
            tell(eloDebugDb, "Info: Skipping calling mysql_library_init(), it's already done!");
         }

         initThreads++;
         initMutex.Unlock();

         return status;
      }

      static int exit()
      {
         initMutex.Lock();

         initThreads--;

         if (!initThreads)
         {
            tell(eloDetail, "Info: Released the last usage of mysql_lib, calling mysql_library_end() now");
            mysql_library_end();

            free(dbHost);
            free(dbUser);
            free(dbPass);
            free(dbName);
            free(encoding);
            free(confPath);
         }
         else
         {
            tell(eloDetail, "Info: The mysql_lib is still in use, skipping mysql_library_end() call");
         }

         initMutex.Unlock();

         return done;
      }

   private:

      MYSQL* mysql;

      // int initialized;
      int attached;
      int inTact;
      int connectDropped;

      static cMyMutex initMutex;
      static int initThreads;

      static char* encoding;
      static char* confPath;

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

      cDbTable(cDbConnection* aConnection, const char* name, bool readOnly = false);
      virtual ~cDbTable();

      virtual const char* TableName()           { return tableDef ? tableDef->getName() : "<unknown>"; }
      virtual int fieldCount()                  { return tableDef->fieldCount(); }
      cDbFieldDef* getField(int f)              { return tableDef->getField(f); }
      cDbFieldDef* getField(const char* name)   { return tableDef->getField(name); }

      virtual int open(int allowAlter = 0);     // 0 - off, 1 - on, 2 on with allow drop unused columns
      virtual int close();
      virtual int attach();
      virtual int detach();
      int isAttached()     { return attached; }

      virtual int find();
      virtual void reset() { reset(stmtSelect); }

      virtual int find(cDbStatement* stmt);
      virtual int fetch(cDbStatement* stmt);
      virtual void reset(cDbStatement* stmt);

      virtual int insert(time_t inssp = 0);
      virtual int update(time_t updsp = 0);
      virtual int store();

      virtual int __attribute__ ((format(printf, 2, 3))) deleteWhere(const char* where, ...);
      virtual int countWhere(const char* where, int& count, const char* what = 0);
      virtual int truncate();

      // interface to cDbRow

      void clear()                                                    { row->clear(); }
      void clearChanged()                                             { row->clearChanged(); }
      int getChanges()                                                { return row->getChanges(); }
      std::string getChangedFields()                                  { return row->getChangedFields(); }
      void setValue(cDbFieldDef* f, const char* value, int size = 0)  { row->setValue(f, value, size); }
      void setValue(cDbFieldDef* f, int value)                        { row->setValue(f, value); }
      void setValue(cDbFieldDef* f, long value)                       { row->setValue(f, value); }
      void setValue(cDbFieldDef* f, double value)                     { row->setValue(f, value); }
      void setBigintValue(cDbFieldDef* f, int64_t value)              { row->setBigintValue(f, value); }
      void setCharValue(cDbFieldDef* f, char value)                   { row->setCharValue(f, value); }

      void setValue(const char* n, const char* value, int size = 0)   { row->setValue(n, value, size); }
      void setValue(const char* n, int value)                         { row->setValue(n, value); }
      void setValue(const char* n, long value)                        { row->setValue(n, value); }
      void setValue(const char* n, long long value)                   { row->setValue(n, value); }
      void setValue(const char* n, double value)                      { row->setValue(n, value); }
      void setBigintValue(const char* n, int64_t value)               { row->setBigintValue(n, value); }
      void setCharValue(const char* n, char value)                    { row->setCharValue(n, value); }

      void copyValues(cDbRow* r, int types = ftData);

      int hasValue(cDbFieldDef* f, const char* value)                 { return row->hasValue(f, value); }
      int hasCharValue(cDbFieldDef* f, char value)                    { return row->hasCharValue(f, value); }
      int hasValue(cDbFieldDef* f, long value)                        { return row->hasValue(f, value); }
      int hasValue(cDbFieldDef* f, double value)                      { return row->hasValue(f, value); }

      int hasValue(const char* n, const char* value)                  { return row->hasValue(n, value); }
      int hasCharValue(const char* n, char value)                     { return row->hasCharValue(n, value); }
      int hasValue(const char* n, long value)                         { return row->hasValue(n, value); }
      int hasValue(const char* n, double value)                       { return row->hasValue(n, value); }

      const char* getStrValue(cDbFieldDef* f)       const             { return row->getStrValue(f); }
      long getIntValue(cDbFieldDef* f)              const             { return row->getIntValue(f); }
      int64_t getBigintValue(cDbFieldDef* f)        const             { return row->getBigintValue(f); }
      float getFloatValue(cDbFieldDef* f)           const             { return row->getFloatValue(f); }
      time_t getTimeValue(cDbFieldDef* f)           const             { return row->getTimeValue(f); }
      int isNull(cDbFieldDef* f)                    const             { return row->isNull(f); }

      const char* getStrValue(const char* n)        const             { return row->getStrValue(n); }
      long getIntValue(const char* n)               const             { return row->getIntValue(n); }
      int64_t getBigintValue(const char* n)         const             { return row->getBigintValue(n); }
      float getFloatValue(const char* n)            const             { return row->getFloatValue(n); }
      time_t getTimeValue(const char* n)            const             { return row->getTimeValue(n); }
      int isNull(const char* n)                     const             { return row->isNull(n); }

      cDbValue* getValue(cDbFieldDef* f)                              { return row->getValue(f); }
      cDbValue* getValue(const char* fname)                           { return row->getValue(fname); }
      int init(cDbValue*& dbvalue, const char* fname)                 { dbvalue = row->getValue(fname); return dbvalue ? success : fail; }
      cDbRow* getRow()                                                { return row; }

      cDbTableDef* getTableDef()                                      { return tableDef; }
      cDbConnection* getConnection()                                  { return connection; }
      MYSQL* getMySql()                                               { return connection->getMySql(); }
      int isConnected()                                               { return connection && connection->getMySql(); }

      int getLastInsertId()                                           { return lastInsertId; }

      virtual int exist(const char* name = 0);
      virtual int validateStructure(int allowAlter = 1);        // 0 - off, 1 - on, 2 on with allow drop unused columns
      virtual int createTable();
      virtual int createIndices();

   protected:

      virtual int init(int allowAlter = 0);                     // 0 - off, 1 - on, 2 on with allow drop unused columns
      virtual int checkIndex(const char* idxName, int& fieldCount);
      virtual int alterModifyField(cDbFieldDef* def);
      virtual int alterAddField(cDbFieldDef* def);
      virtual int alterDropField(const char* name);

      // data

      bool isReadOnly {false};
      cDbRow* row {nullptr};
      int holdInMemory {no};    // hold table additionally in memory (not implemented yet)
      int attached {no};
      int lastInsertId {na};

      cDbConnection* connection {nullptr};
      cDbTableDef* tableDef {nullptr};

      // basic statements

      cDbStatement* stmtSelect {nullptr};
      cDbStatement* stmtInsert {nullptr};
      cDbStatement* stmtUpdate {nullptr};
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

         tell(eloAlways, "Creating view '%s' using definition in '%s'",
              name, file);

         status = connection->executeSqlFile(file);

         free(file);

         return status;
      }

      int drop()
      {
         tell(eloAlways, "Drop view '%s'", name);

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

         tell(eloDebugDb, "Calling '%s'", name);

         stmt.build("call %s", name);

         if (stmt.prepare() != success || stmt.execute() != success)
            return fail;

         tell(eloDebugDb, "'%s' suceeded", name);

         return success;
      }

      int created()
      {
         if (!connection || !connection->getMySql())
            return no;

         cDbStatement stmt(connection);

         stmt.build("show %s status where name = '%s'",
                    type == ptProcedure ? "procedure" : "function", name);

         if (stmt.prepare() != success || stmt.execute() != success)
         {
            tell(eloAlways, "%s check of '%s' failed",
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

         tell(eloDetail, "Creating %s '%s'",
              type == ptProcedure ? "procedure" : "function", name);

         status = connection->executeSqlFile(file);

         free(file);

         return status;
      }

      int drop()
      {
         tell(eloDetail, "Drop %s '%s'", type == ptProcedure ? "procedure" : "function", name);

         return connection->query("drop %s %s", type == ptProcedure ? "procedure" : "function", name);
      }

      static int existOnFs(const char* path, const char* name)
      {
         int state;
         char* file = 0;

         asprintf(&file, "%s/%s.sql", path, name);
         state = fileExists(file);

         free(file);

         return state;
      }

   protected:

      cDbConnection* connection;
      ProcType type;
      char* name;

};

//***************************************************************************
#endif //__DB_H
