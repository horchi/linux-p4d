/*
 * db.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <stdio.h>
#include <errmsg.h>

#include <map>

#include "db.h"

//***************************************************************************
// DB Statement
//***************************************************************************

int cDbStatement::explain = no;

cDbStatement::cDbStatement(cDbTable* aTable)
{
   table = aTable;
   connection = table->getConnection();
   stmtTxt = "";
   stmt = 0;
   inCount = 0;
   outCount = 0;
   inBind = 0;
   outBind = 0;
   affected = 0;
   metaResult = 0;
   bindPrefix = 0;
   firstExec = yes;
   buildErrors = 0;

   callsPeriod = 0;
   callsTotal = 0;
   duration = 0;

   if (connection)
      connection->statements.append(this);
}

cDbStatement::cDbStatement(cDbConnection* aConnection, const char* sText)
{
   table = 0;
   connection = aConnection;
   stmtTxt = sText;
   stmt = 0;
   inCount = 0;
   outCount = 0;
   inBind = 0;
   outBind = 0;
   affected = 0;
   metaResult = 0;
   bindPrefix = 0;
   firstExec = yes;

   callsPeriod = 0;
   callsTotal = 0;
   duration = 0;
   buildErrors = 0;

   if (connection)
      connection->statements.append(this);
}

cDbStatement::~cDbStatement()
{
   if (connection)
      connection->statements.remove(this);

   clear();
}

//***************************************************************************
// Execute
//***************************************************************************

int cDbStatement::execute(int noResult)
{
   affected = 0;

   if (!connection || !connection->getMySql())
      return fail;

   if (!stmt)
      return connection->errorSql(connection, "execute(missing statement)");

//    if (explain && firstExec)
//    {
//       firstExec = no;

//       if (strstr(stmtTxt.c_str(), "select "))
//       {
//          MYSQL_RES* result;
//          MYSQL_ROW row;
//          string q = "explain " + stmtTxt;

//          if (connection->query(q.c_str()) != success)
//             connection->errorSql(connection, "explain ", 0);
//          else if ((result = mysql_store_result(connection->getMySql())))
//          {
//             while ((row = mysql_fetch_row(result)))
//             {
//                tell(eloAlways, "EXPLAIN: %s) %s %s %s %s %s %s %s %s %s",
//                     row[0], row[1], row[2], row[3],
//                     row[4], row[5], row[6], row[7], row[8], row[9]);
//             }

//             mysql_free_result(result);
//          }
//       }
//    }

   // tell(eloAlways, "execute %d [%s]", stmt, stmtTxt.c_str());

   double start = usNow();

   if (mysql_stmt_execute(stmt))
      return connection->errorSql(connection, "execute(stmt_execute)", stmt, stmtTxt.c_str());

   duration += usNow() - start;
   callsPeriod++;
   callsTotal++;

   // out binding - if needed

   if (outCount && !noResult)
   {
      if (mysql_stmt_store_result(stmt))
         return connection->errorSql(connection, "execute(store_result)", stmt, stmtTxt.c_str());

      // fetch the first result - if any

      if (mysql_stmt_affected_rows(stmt) > 0)
         mysql_stmt_fetch(stmt);
   }
   else if (outCount)
   {
      mysql_stmt_store_result(stmt);
   }

   // result was stored (above) only if output (outCound) is expected,
   // therefore we don't need to call freeResult() after insert() or update()

   affected = mysql_stmt_affected_rows(stmt);

   return success;
}

//***************************************************************************
//
//***************************************************************************

int cDbStatement::getLastInsertId()
{
   MYSQL_RES* result = 0;
   int insertId = na;

   if ((result = mysql_store_result(connection->getMySql())) == 0 &&
       mysql_field_count(connection->getMySql()) == 0 &&
       mysql_insert_id(connection->getMySql()) != 0)
   {
      insertId = mysql_insert_id(connection->getMySql());
   }

   mysql_free_result(result);

   return insertId;
}

int cDbStatement::getResultCount()
{
   mysql_stmt_store_result(stmt);

   return mysql_stmt_affected_rows(stmt);
}

bool cDbStatement::find()
{
   if (execute() != success)
      return false;

   return getAffected() > 0;
}

int cDbStatement::fetch()
{
   if (!mysql_stmt_fetch(stmt))
      return yes;

   return no;
}

int cDbStatement::freeResult()
{
   if (metaResult)
      mysql_free_result(metaResult);

   if (stmt)
      mysql_stmt_free_result(stmt);

   return success;
}

//***************************************************************************
// Build Statements - new Interface
//***************************************************************************

int cDbStatement::build(const char* format, ...)
{
   if (format)
   {
      char* tmp;

      va_list more;
      va_start(more, format);
      vasprintf(&tmp, format, more);

      stmtTxt += tmp;
      free(tmp);
   }

   return success;
}

int cDbStatement::bind(const char* fname, int mode, const char* delim)
{
   return bind(table->getValue(fname), mode, delim);
}

int cDbStatement::bind(cDbFieldDef* field, int mode, const char* delim)
{
   return bind(table->getValue(field), mode, delim);
}

int cDbStatement::bind(cDbTable* aTable, cDbFieldDef* field, int mode, const char* delim)
{
   return bind(aTable->getValue(field), mode, delim);
}

int cDbStatement::bind(cDbTable* aTable, const char* fname, int mode, const char* delim)
{
   return bind(aTable->getValue(fname), mode, delim);
}

int cDbStatement::bind(cDbValue* value, int mode, const char* delim)
{
   if (!value || !value->getField())
   {
      tell(eloAlways, "Error: Missing %s value", !value ? "bind" : "field of bind");
      buildErrors++;
      return fail;
   }

   if (delim)
      stmtTxt += delim;

   if (bindPrefix)
      stmtTxt += bindPrefix;

   if (mode & bndIn)
   {
      if (mode & bndSet)
         stmtTxt += value->getDbName() + std::string(" =");

      stmtTxt += " ?";
      appendBinding(value, bndIn);
   }
   else if (mode & bndOut)
   {
      stmtTxt += value->getDbName();
      appendBinding(value, bndOut);
   }

   return success;
}

int cDbStatement::bindAllOut(const char* delim)
{
   int n = 0;
   std::map<std::string, cDbFieldDef*>::iterator f;
   cDbTableDef* tableDef = table->getTableDef();

   if (delim)
      stmtTxt += delim;

   for (f = tableDef->dfields.begin(); f != tableDef->dfields.end(); f++)
   {
      if (f->second->getType() & ftMeta)
         continue;

      bind(f->second, bndOut, n++ ? ", " : "");
   }

   return success;
}

int cDbStatement::bindCmp(const char* ctable, cDbValue* value,
                          const char* comp, const char* delim)
{
   if (delim)  build("%s", delim);
   if (ctable) build("%s.", ctable);

   build("%s%s %s ?", bindPrefix ? bindPrefix : "", value->getDbName(), comp);

   appendBinding(value, bndIn);

   return success;
}

int cDbStatement::bindCmp(const char* ctable, cDbFieldDef* field, cDbValue* value,
                          const char* comp, const char* delim)
{
   cDbValue* vf = table->getRow()->getValue(field);
   cDbValue* vv = value ? value : vf;

   if (!vf || !vv)
   {
      buildErrors++;
      return fail;
   }

   if (delim)  build("%s", delim);
   if (ctable) build("%s.", ctable);

   build("%s%s %s ?", bindPrefix ? bindPrefix : "", vf->getDbName(), comp);

   appendBinding(vv, bndIn);

   return success;
}

int cDbStatement::bindCmp(const char* ctable, const char* fname, cDbValue* value,
                          const char* comp, const char* delim)
{
   cDbValue* vf = table->getRow()->getValue(fname);
   cDbValue* vv = value ? value : vf;

   if (!vf || !vv)
   {
      buildErrors++;
      return fail;
   }

   if (delim)  build("%s", delim);
   if (ctable) build("%s.", ctable);

   build("%s%s %s ?", bindPrefix ? bindPrefix : "", vf->getDbName(), comp);

   appendBinding(vv, bndIn);

   return success;
}

int cDbStatement::bindText(const char* text, cDbValue* value, const char* comp, const char* delim)
{
   if (!value)
   {
      buildErrors++;
      return fail;
   }

   if (delim) build("%s", delim);

   build("%s %s ?", text, comp);

   appendBinding(value, bndIn);

   return success;
}

int cDbStatement::bindTextFree(const char* text, cDbValue* value, const char* delim, int mode)
{
   if (!value)
   {
      buildErrors++;
      return fail;
   }

   if (delim) build("%s", delim);

   build("%s", text);

   if (mode & bndIn)
      appendBinding(value, bndIn);

   else if (mode & bndOut)
      appendBinding(value, bndOut);

   return success;
}

//***************************************************************************
// Bind In Char   - like <field> in ('A','B','C')
//
//   expected string in cDbValue is: "A,B,C"
//***************************************************************************

int cDbStatement::bindInChar(const char* ctable, const char* fname,
                             cDbValue* value, const char* delim)
{
   cDbValue* vf = table->getRow()->getValue(fname);
   cDbValue* vv = value ? value : vf;

   if (!vf || !vv)
   {
      buildErrors++;
      return fail;
   }

   build("%s find_in_set(cast(%s%s%s%s as char),?)",
         delim ? delim : "",
         bindPrefix ? bindPrefix : "",
         ctable ? ctable : "",
         ctable ? "." : "",
         vf->getDbName());

   appendBinding(vv, bndIn);

   return success;
}

//***************************************************************************
// Clear
//***************************************************************************

void cDbStatement::clear()
{
   stmtTxt = "";
   affected = 0;

   if (inCount)
   {
      free(inBind);
      inCount = 0;
      inBind = 0;
   }

   if (outCount)
   {
      free(outBind);
      outCount = 0;
      outBind = 0;
   }

   if (stmt)
   {
      mysql_stmt_free_result(stmt);
      mysql_stmt_close(stmt);
      stmt = 0;
   }
}

//***************************************************************************
// Append Binding
//***************************************************************************

int cDbStatement::appendBinding(cDbValue* value, BindType bt)
{
   int count = 0;
   MYSQL_BIND** bindings = 0;
   MYSQL_BIND* newBinding;

   if (bt & bndIn)
   {
      count = ++inCount;
      bindings = &inBind;
   }
   else if (bt & bndOut)
   {
      count = ++outCount;
      bindings = &outBind;
   }
   else
      return 0;

   if (!*bindings)
      *bindings = (MYSQL_BIND*)malloc(count * sizeof(MYSQL_BIND));
   else
      *bindings = (MYSQL_BIND*)srealloc(*bindings, count * sizeof(MYSQL_BIND));

   newBinding = &((*bindings)[count-1]);

   memset(newBinding, 0, sizeof(MYSQL_BIND));

   if (value->getField()->getFormat() == ffAscii || value->getField()->getFormat() == ffText || value->getField()->getFormat() == ffMText)
   {
      newBinding->buffer_type = MYSQL_TYPE_STRING;
      newBinding->buffer = value->getStrValueRef();
      newBinding->buffer_length = value->getField()->getSize();
      newBinding->length = value->getStrValueSizeRef();

      newBinding->is_null = value->getNullRef();
      newBinding->error = 0;            // #TODO
   }
   else if (value->getField()->getFormat() == ffMlob)
   {
      newBinding->buffer_type = MYSQL_TYPE_BLOB;
      newBinding->buffer = value->getStrValueRef();
      newBinding->buffer_length = value->getField()->getSize();
      newBinding->length = value->getStrValueSizeRef();

      newBinding->is_null = value->getNullRef();
      newBinding->error = 0;            // #TODO
   }
   else if (value->getField()->getFormat() == ffFloat)
   {
      newBinding->buffer_type = MYSQL_TYPE_FLOAT;
      newBinding->buffer = value->getFloatValueRef();

      newBinding->length = 0;            // #TODO
      newBinding->is_null =  value->getNullRef();
      newBinding->error = 0;             // #TODO
   }
   else if (value->getField()->getFormat() == ffDateTime)
   {
      newBinding->buffer_type = MYSQL_TYPE_DATETIME;
      newBinding->buffer = value->getTimeValueRef();

      newBinding->length = 0;            // #TODO
      newBinding->is_null =  value->getNullRef();
      newBinding->error = 0;             // #TODO
   }
   else if (value->getField()->getFormat() == ffBigInt || value->getField()->getFormat() == ffUBigInt)
   {
      newBinding->buffer_type = MYSQL_TYPE_LONGLONG;
      newBinding->buffer = value->getBigIntValueRef();
      newBinding->is_unsigned = (value->getField()->getFormat() == ffUBigInt);

      newBinding->length = 0;
      newBinding->is_null =  value->getNullRef();
      newBinding->error = 0;             // #TODO
   }
   else  // ffInt, ffUInt
   {
      newBinding->buffer_type = MYSQL_TYPE_LONG;
      newBinding->buffer = value->getIntValueRef();
      newBinding->is_unsigned = (value->getField()->getFormat() == ffUInt);

      newBinding->length = 0;
      newBinding->is_null =  value->getNullRef();
      newBinding->error = 0;             // #TODO
   }

   return success;
}

//***************************************************************************
// Prepare Statement
//***************************************************************************

int cDbStatement::prepare()
{
   if (!connection->getMySql())
   {
      tell(eloAlways, "Error: Lost connection, can't prepare statement");
      return fail;
   }

   if (!stmtTxt.length())
      return fail;

   if (buildErrors)
      return fail;

   stmt = mysql_stmt_init(connection->getMySql());

   // prepare statement

   if (mysql_stmt_prepare(stmt, stmtTxt.c_str(), stmtTxt.length()))
      return connection->errorSql(connection, "prepare(stmt_prepare)", stmt, stmtTxt.c_str());

   if (outBind)
   {
      if (mysql_stmt_bind_result(stmt, outBind))
         return connection->errorSql(connection, "execute(bind_result)", stmt);
   }

   if (inBind)
   {
      if (mysql_stmt_bind_param(stmt, inBind))
         return connection->errorSql(connection, "buildPrimarySelect(bind_param)", stmt);
   }

   tell(eloDebugDb, "Statement '%s' with (%ld) in parameters and (%d) out bindings prepared",
        stmtTxt.c_str(), mysql_stmt_param_count(stmt), outCount);

   return success;
}

//***************************************************************************
// Show Statistic
//***************************************************************************

void cDbStatement::showStat()
{
   if (callsPeriod)
   {
      tell(eloAlways, "calls %4ld in %6.2fms; total %4ld [%s]",
           callsPeriod, duration/1000, callsTotal, stmtTxt.c_str());

      callsPeriod = 0;
      duration = 0;
   }
}

//***************************************************************************
// cDbConnection statics
//***************************************************************************

char* cDbConnection::confPath = 0;
char* cDbConnection::encoding = 0;
char* cDbConnection::dbHost = strdup("localhost");
int   cDbConnection::dbPort = 3306;
char* cDbConnection::dbUser = 0;
char* cDbConnection::dbPass = 0;
char* cDbConnection::dbName = 0;
int   cDbConnection::initThreads = 0;
cMyMutex cDbConnection::initMutex;

//***************************************************************************
// Class cDbTable
//***************************************************************************

//***************************************************************************
// Object
//***************************************************************************

cDbTable::cDbTable(cDbConnection* aConnection, const char* name, bool readOnly)
{
   connection = aConnection;
   isReadOnly = readOnly;
   tableDef = dbDict.getTable(name);

   if (tableDef)
      row = new cDbRow(tableDef);
   else
      tell(eloAlways, "Fatal: Table '%s' missing in dictionary '%s'!", name, dbDict.getPath());
}

cDbTable::~cDbTable()
{
   close();

   delete row;
}

//***************************************************************************
// Open / Close
//***************************************************************************

int cDbTable::open(int allowAlter)
{
   if (!tableDef || !row)
      return abrt;

   if (attach() != success)
   {
      tell(eloAlways, "Could not access database '%s:%d' (tried to open %s)",
           connection->getHost(), connection->getPort(), TableName());

      return fail;
   }

   return init(allowAlter);
}

int cDbTable::close()
{
   if (stmtSelect) { delete stmtSelect; stmtSelect = nullptr; }
   if (stmtInsert) { delete stmtInsert; stmtInsert = nullptr; }
   if (stmtUpdate) { delete stmtUpdate; stmtUpdate = nullptr; }

   detach();

   return success;
}

//***************************************************************************
// Attach / Detach
//***************************************************************************

int cDbTable::attach()
{
   if (isAttached())
      return success;

   if (connection->attachConnection() != success)
   {
      tell(eloAlways, "Could not access database '%s:%d'",
           connection->getHost(), connection->getPort());

      return fail;
   }

   attached = yes;

   return success;
}

int cDbTable::detach()
{
   if (isAttached())
   {
      connection->detachConnection();
      attached = no;
   }

   return success;
}

//***************************************************************************
// Init
//***************************************************************************

int cDbTable::init(int allowAlter)
{
   std::string str;
   std::map<std::string, cDbFieldDef*>::iterator f;
   int n = 0;

   if (!isConnected())
      return fail;

   // check/create table ...

   if (allowAlter)
   {
      if (exist())
      validateStructure(allowAlter);

      if (!exist() && createTable() != success)
      return fail;

   // check/create indices

   createIndices();
   }

   // ------------------------------
   // prepare BASIC statements
   // ------------------------------

   // select by primary key ...

   stmtSelect = new cDbStatement(this);

   stmtSelect->build("select ");

   n = 0;

   for (f = tableDef->dfields.begin(); f != tableDef->dfields.end(); f++)
      stmtSelect->bind(f->second, bndOut, n++ ? ", " : "");

   stmtSelect->build(" from %s ", TableName());

   n = 0;

   for (f = tableDef->dfields.begin(); f != tableDef->dfields.end(); f++)
   {
      if (!(f->second->getType() & ftPrimary))
         continue;

      if (!n)
         stmtSelect->build("where ");

      stmtSelect->bind(f->second, bndIn | bndSet, n++ ? " and " : "");
   }

   stmtSelect->build(";");

   if (stmtSelect->prepare() != success)
      return fail;

   if (!isReadOnly)
   {
      // -----------------------------------------
      // insert

      stmtInsert = new cDbStatement(this);

      stmtInsert->build("insert into %s set ", TableName());

      n = 0;

      for (f = tableDef->dfields.begin(); f != tableDef->dfields.end(); f++)
      {
         // don't insert autoinc and calculated fields

         if (f->second->getType() & ftAutoinc)
            continue;

         stmtInsert->bind(f->second, bndIn | bndSet, n++ ? ", " : "");
      }

      stmtInsert->build(";");

      if (stmtInsert->prepare() != success)
         return fail;

      // -----------------------------------------
      // update via primary key ...

      stmtUpdate = new cDbStatement(this);

      stmtUpdate->build("update %s set ", TableName());

      n = 0;

      for (f = tableDef->dfields.begin(); f != tableDef->dfields.end(); f++)
      {
         // don't update PKey, autoinc and not used fields

         if (f->second->getType() & ftPrimary ||
             f->second->getType() & ftAutoinc)
            continue;

         if (strcasecmp(f->second->getName(), "inssp") == 0)  // don't update the insert stamp
            continue;

         stmtUpdate->bind(f->second, bndIn | bndSet, n++ ? ", " : "");
      }

      stmtUpdate->build(" where ");

      n = 0;

      for (f = tableDef->dfields.begin(); f != tableDef->dfields.end(); f++)
      {
         if (!(f->second->getType() & ftPrimary))
            continue;

         stmtUpdate->bind(f->second, bndIn | bndSet, n++ ? " and " : "");
      }

      stmtUpdate->build(";");

      if (stmtUpdate->prepare() != success)
         return fail;
   }

   return success;
}

//***************************************************************************
// Check Table
//***************************************************************************

int cDbTable::exist(const char* name)
{
   if (isEmpty(name))
      name = TableName();

   if (!connection || !connection->getMySql())
      return fail;

   MYSQL_RES* result = mysql_list_tables(connection->getMySql(), name);
   MYSQL_ROW tabRow = mysql_fetch_row(result);
   mysql_free_result(result);

   return tabRow ? yes : no;
}

//***************************************************************************
// Validate Structure
//***************************************************************************

struct FieldInfo
{
   std::string columnFormat;
   std::string description;
   std::string defaulValue;
};

int cDbTable::validateStructure(int allowAlter)
{
   std::map<std::string, FieldInfo, _casecmp_> fields;
   MYSQL_RES* result;
   MYSQL_ROW row;
   std::map<std::string, FieldInfo, _casecmp_>::iterator it;
   int needDetach = no;

   if (!allowAlter)
      return done;

   const char* select = "select column_name, column_type, column_comment, data_type, is_nullable, "
      " character_maximum_length, column_default, numeric_precision "
      " from information_schema.columns "
      " where table_name = '%s' and table_schema= '%s'";

   if (!isAttached())
   {
      needDetach = yes;

   if (attach() != success)
      return fail;
   }

   // ------------------------
   // execute query

   if (connection->query(select, TableName(), connection->getName()) != success)
   {
      connection->errorSql(getConnection(), "validateStructure()", 0);
      if (needDetach) detach();
      return fail;
   }

   // ------------------------
   // process the result

   if (!(result = mysql_store_result(connection->getMySql())))
   {
      connection->errorSql(getConnection(), "validateStructure()");
      if (needDetach) detach();
      return fail;
   }

   while ((row = mysql_fetch_row(result)))
   {
      fields[row[0]].columnFormat = row[1];
      fields[row[0]].description = row[2] ? row[2] : "";
      fields[row[0]].defaulValue = row[6] ? strcasecmp(row[6], "NULL") == 0 ? "" : row[6] : "";

      if (fields[row[0]].defaulValue.length() > 2 &&
          fields[row[0]].defaulValue.back() == '\'' &&
          fields[row[0]].defaulValue.front() == '\'')
      {
         fields[row[0]].defaulValue.pop_back();
         fields[row[0]].defaulValue.erase(0, 1);
      }
   }

   mysql_free_result(result);

   // --------------------------------------
   // validate if all fields of dict are in
   //   table and check their format, ...

   for (int i = 0; i < fieldCount(); i++)
   {
      char colType[100];

      tell(eloDebugDb, "Check field '%s'", getField(i)->getName());

      if (fields.find(getField(i)->getDbName()) == fields.end())
         alterAddField(getField(i));

      else
      {
         FieldInfo* fieldInfo = &fields[getField(i)->getDbName()];

         getField(i)->toColumnFormat(colType);

         if (strcasecmp(fieldInfo->columnFormat.c_str(), colType) != 0 ||
             strcasecmp(fieldInfo->description.c_str(), getField(i)->getDescription()) != 0 ||
             (strcasecmp(fieldInfo->defaulValue.c_str(), getField(i)->getDefault()) != 0 && !(getField(i)->getType() & ftPrimary)))
         {
            if (strcasecmp(fieldInfo->columnFormat.c_str(), colType) != 0)
               tell(eloDebugDb, "Debug: Format of '%s' changed from '%s' to '%s'", getField(i)->getDbName(),
                    fieldInfo->columnFormat.c_str(), colType);

            if (strcasecmp(fieldInfo->description.c_str(), getField(i)->getDescription()) != 0)
               tell(eloDebugDb, "Debug: Description of '%s' changed from '%s' to '%s'", getField(i)->getDbName(),
                    fieldInfo->description.c_str(), getField(i)->getDescription());

            if (strcasecmp(fieldInfo->defaulValue.c_str(), getField(i)->getDefault()) != 0 && !(getField(i)->getType() & ftPrimary))
               tell(eloDebugDb, "Debug: Default value of '%s' changed from from '%s' to '%s'", getField(i)->getDbName(),
                    fieldInfo->defaulValue.c_str(), getField(i)->getDefault());

            alterModifyField(getField(i));
         }
      }
   }

   // --------------------------------------
   // check if table contains unused fields
   //   and report them

   for (it = fields.begin(); it != fields.end(); it++)
   {
      if (!getRow()->getFieldByDbName(it->first.c_str()))
      {
         if (allowAlter == 2)
            alterDropField(it->first.c_str());
         else
            tell(eloAlways, "Info: Field '%s' not used anymore, "
                 "to remove it call 'ALTER TABLE %s DROP COLUMN %s;' manually",
                 it->first.c_str(), TableName(), it->first.c_str());
      }
   }

   if (needDetach) detach();

   return success;
}

//***************************************************************************
// Alter 'Modify Field'
//***************************************************************************

int cDbTable::alterModifyField(cDbFieldDef* def)
{
   char* statement;
   char colType[100];

   tell(eloAlways, "  Info: Definition of field '%s.%s' modified, try to alter table",
        TableName(), def->getName());

   // alter table events modify column guest varchar(50)

   asprintf(&statement, "alter table %s modify column %s %s comment '%s' %s %s%s%s",
            TableName(),
            def->getDbName(),
            def->toColumnFormat(colType),
            def->getDbDescription(),
            def->getType() & ftAutoinc ? " not null auto_increment " : "",
            !isEmpty(def->getDefault()) ? "default '" : "",
            !isEmpty(def->getDefault()) ? def->getDefault() : "",
            !isEmpty(def->getDefault()) ? "'" : ""
      );

   tell(eloDetail, "Execute [%s]", statement);

   if (connection->query("%s", statement))
      return connection->errorSql(getConnection(), "alterAddField()",
                                  0, statement);

   free(statement);

   return done;
}

//***************************************************************************
// Alter 'Add Field'
//***************************************************************************

int cDbTable::alterAddField(cDbFieldDef* def)
{
   std::string statement;
   char colType[100];

   tell(eloAlways, "Info: Missing field '%s.%s', try to alter table",
        TableName(), def->getName());

   // alter table channelmap add column ord int(11) [after source]

   statement = std::string("alter table ") + TableName() + std::string(" add column ")
      + def->getDbName() + std::string(" ") + def->toColumnFormat(colType);

   if (def->getFormat() != ffMlob)
   {
      if (def->getType() & ftAutoinc)
         statement += " not null auto_increment";
      else if (!isEmpty(def->getDefault()))
         statement += " default '" + std::string(def->getDefault()) + "'";
   }

   if (!isEmpty(def->getDbDescription()))
      statement += std::string(" comment '") + def->getDbDescription() + std::string("'");

   if (def->getIndex() > 0)
      statement += std::string(" after ") + getField(def->getIndex()-1)->getDbName();

   tell(eloDetail, "Execute [%s]", statement.c_str());

   if (connection->query("%s", statement.c_str()))
      return connection->errorSql(getConnection(), "alterAddField()", 0, statement.c_str());

   return done;
}

//***************************************************************************
// Alter 'Drop Field'
//***************************************************************************

int cDbTable::alterDropField(const char* name)
{
   char* statement;

   tell(eloAlways, "Info: Unused field '%s', try to drop it", name);

   // alter table channelmap add column ord int(11) [after source]

   asprintf(&statement, "alter table %s drop column %s", TableName(), name);

   tell(eloDetail, "Execute [%s]", statement);

   if (connection->query("%s", statement))
      return connection->errorSql(getConnection(), "alterDropField()",
                                  0, statement);

   free(statement);

   return done;
}

//***************************************************************************
// Create Table
//***************************************************************************

int cDbTable::createTable()
{
   std::string statement;
   std::string aKey;
   int needDetach = no;

   if (!tableDef || !row)
      return abrt;

   if (!isAttached())
   {
      needDetach = yes;

   if (attach() != success)
      return fail;
   }

   // table exists -> nothing to do

   if (exist())
   {
      if (needDetach) detach();
      return done;
   }

   tell(eloAlways, "Initialy creating table '%s'", TableName());

   // build 'create' statement ...

   statement = std::string("create table ") + TableName() + std::string("(");

   for (int i = 0; i < fieldCount(); i++)
   {
      char colType[100];

      if (i) statement += std::string(", ");

      statement += std::string(getField(i)->getDbName()) + " " + std::string(getField(i)->toColumnFormat(colType));

      if (getField(i)->getFormat() != ffMlob)
      {
         if (getField(i)->getType() & ftAutoinc)
            statement += " not null auto_increment";
         else if (!isEmpty(getField(i)->getDefault()))
            statement += " default '" + std::string(getField(i)->getDefault()) + "'";
      }

      if (!isEmpty(getField(i)->getDbDescription()))
         statement += std::string(" comment '") + getField(i)->getDbDescription() + std::string("'");
   }

   aKey = "";

   for (int i = 0, n = 0; i < fieldCount(); i++)
   {
      if (getField(i)->getType() & ftPrimary)
      {
         if (n++) aKey += std::string(", ");
         aKey += std::string(getField(i)->getDbName()) + " DESC";
      }
   }

   if (aKey.length())
   {
      statement += std::string(", PRIMARY KEY(");
      statement += aKey;
      statement += ")";
   }

   aKey = "";

   for (int i = 0, n = 0; i < fieldCount(); i++)
   {
      if (getField(i)->getType() & ftAutoinc && !(getField(i)->getType() & ftPrimary))
      {
         if (n++) aKey += std::string(", ");
         aKey += std::string(getField(i)->getDbName()) + " DESC";
      }
   }

   if (aKey.length())
   {
      statement += std::string(", KEY(");
      statement += aKey;
      statement += ")";
   }

   statement += std::string(") ENGINE=InnoDB ROW_FORMAT=DYNAMIC;");

   tell(eloDetail, "%s", statement.c_str());

   if (connection->query("%s", statement.c_str()))
   {
      if (needDetach) detach();
      return connection->errorSql(getConnection(), "createTable()",
                                  0, statement.c_str());
   }

   if (needDetach) detach();

   return success;
}

//***************************************************************************
// Create Indices
//***************************************************************************

int cDbTable::createIndices()
{
   std::string statement;

   tell(eloDebugDb, "Initialy checking indices for '%s'", TableName());

   // check/create indexes

   for (int i = 0; i < tableDef->indexCount(); i++)
   {
      cDbIndexDef* index = tableDef->getIndex(i);
      int fCount;
      std::string idxName;

      if (!index->fieldCount())
         continue;

      // check

      idxName = "idx" + std::string(index->getName());

      checkIndex(idxName.c_str(), fCount);

      if (fCount != index->fieldCount())
      {
         // create index

         statement = "create index " + idxName;
         statement += " on " + std::string(TableName()) + "(";

         int n = 0;

         for (int f = 0; f < index->fieldCount(); f++)
         {
            cDbFieldDef* fld = index->getField(f);

            if (fld)
            {
               if (n++) statement += std::string(", ");
               statement += fld->getDbName();
            }
         }

         if (!n) continue;

         statement += ");";
         tell(eloDetail, "%s", statement.c_str());

         if (connection->query("%s", statement.c_str()))
            return connection->errorSql(getConnection(), "createIndices()",
                                        0, statement.c_str());
      }
   }

   return success;
}

//***************************************************************************
// Check Index
//***************************************************************************

int cDbTable::checkIndex(const char* idxName, int& fieldCount)
{
   enum IndexQueryFields
   {
      idTable,
      idNonUnique,
      idKeyName,
      idSeqInIndex,
      idColumnName,
      idCollation,
      idCardinality,
      idSubPart,
      idPacked,
      idNull,
      idIndexType,
      idComment,
      idIndexComment,

      idCount
   };

   MYSQL_RES* result;
   MYSQL_ROW row;

   fieldCount = 0;

   if (connection->query("show index from %s", TableName()) != success)
   {
      connection->errorSql(getConnection(), "checkIndex()", 0);

      return fail;
   }

   if ((result = mysql_store_result(connection->getMySql())))
   {
      while ((row = mysql_fetch_row(result)))
      {
         tell(eloDebugDb, "%s:  %-20s %s %s",
              row[idTable], row[idKeyName],
              row[idSeqInIndex], row[idColumnName]);

         if (strcasecmp(row[idKeyName], idxName) == 0)
            fieldCount++;
      }

      mysql_free_result(result);

      return success;
   }

   connection->errorSql(getConnection(), "checkIndex()");

   return fail;
}

//***************************************************************************
// Copy Values
//***************************************************************************

void cDbTable::copyValues(cDbRow* r, int typesFilter)
{
   std::map<std::string, cDbFieldDef*>::iterator f;

   for (f = tableDef->dfields.begin(); f != tableDef->dfields.end(); f++)
   {
      cDbFieldDef* fld = f->second;

      if (r->isNull(fld))                   // skip where source field is NULL
         continue;

      if (!(typesFilter & fld->getType()))  // Filter
         continue;

      switch (fld->getFormat())
      {
         case ffAscii:
         case ffText:
         case ffMText:
         case ffMlob:
            row->setValue(fld, r->getStrValue(fld));
            break;

         case ffFloat:
            row->setValue(fld, r->getFloatValue(fld));
            break;

         case ffDateTime:
            row->setValue(fld, r->getTimeValue(fld));
            break;

         case ffBigInt:
         case ffUBigInt:
            row->setBigintValue(fld, r->getBigintValue(fld));
            break;

         case ffInt:
         case ffUInt:
            row->setValue(fld, r->getIntValue(fld));
            break;

         default:
            tell(eloAlways, "Fatal unhandled field type %d", fld->getFormat());
      }
   }
}

//***************************************************************************
// SQL Error
//***************************************************************************

int cDbConnection::errorSql(cDbConnection* connection, const char* prefix,
                            MYSQL_STMT* stmt, const char* stmtTxt)
{
   if (!connection || !connection->mysql)
   {
      tell(eloAlways, "SQL-Error in '%s'", prefix);
      return fail;
   }

   int error = mysql_errno(connection->mysql);
   char* conErr = 0;
   char* stmtErr = 0;

   if (error == CR_SERVER_LOST ||
       error == CR_SERVER_GONE_ERROR ||
// for compatibility with newer versions of MariaDB library
#ifdef CR_INVALID_CONN_HANDLE
       error == CR_INVALID_CONN_HANDLE ||
#endif
       error == CR_COMMANDS_OUT_OF_SYNC ||
       error == CR_SERVER_LOST_EXTENDED ||
#ifdef CR_STMT_CLOSED
       error == CR_STMT_CLOSED ||
#endif
// for compatibility with newer versions of MariaDB library
#ifdef CR_CONN_UNKNOW_PROTOCOL
       error == CR_CONN_UNKNOW_PROTOCOL ||
#else
# ifdef CR_CONN_UNKNOWN_PROTOCOL
       error == CR_CONN_UNKNOWN_PROTOCOL ||
# endif
#endif
       error == CR_UNSUPPORTED_PARAM_TYPE ||
       error == CR_NO_PREPARE_STMT ||
       error == CR_SERVER_HANDSHAKE_ERR ||
       error == CR_WRONG_HOST_INFO ||
       error == CR_OUT_OF_MEMORY ||
       error == CR_IPSOCK_ERROR ||
       error == CR_SOCKET_CREATE_ERROR ||
       error == CR_CONNECTION_ERROR ||
       error == CR_TCP_CONNECTION ||
       error == CR_PARAMS_NOT_BOUND ||
       error == CR_CONN_HOST_ERROR ||
       error == CR_SSL_CONNECTION_ERROR

       // to be continued - not all errors should result in a reconnect ...

      )
   {
      connectDropped = yes;
   }

   if (error)
      asprintf(&conErr, "%s (%d) ", mysql_error(connection->mysql), error);

   if (stmt || stmtTxt)
      asprintf(&stmtErr, "'%s' [%s]",
               stmt ? mysql_stmt_error(stmt) : "",
               stmtTxt ? stmtTxt : "");

   tell(eloAlways, "SQL-Error in '%s' - %s%s", prefix,
        conErr ? conErr : "", stmtErr ? stmtErr : "");

   free(conErr);
   free(stmtErr);

   if (connectDropped)
      tell(eloAlways, "Fatal, lost connection to mysql server, aborting pending actions");

   return fail;
}

//***************************************************************************
// Delete Where
//***************************************************************************

int cDbTable::deleteWhere(const char* where, ...)
{
   std::string stmt;
   char* tmp;
   va_list more;

   if (!connection || !connection->getMySql())
      return fail;

   va_start(more, where);
   vasprintf(&tmp, where, more);

   stmt = "delete from " + std::string(TableName()) + " where " + std::string(tmp);

   free(tmp);

   if (connection->query("%s", stmt.c_str()))
      return connection->errorSql(connection, "deleteWhere()", 0, stmt.c_str());

   return success;
}

//***************************************************************************
// Coiunt Where
//***************************************************************************

int cDbTable::countWhere(const char* where, int& count, const char* what)
{
   std::string tmp;
   MYSQL_RES* res;
   MYSQL_ROW data;

   count = 0;

   if (isEmpty(what))
      what = "count(1)";

   if (!isEmpty(where))
      tmp = "select " + std::string(what) + " from " + std::string(TableName()) + " where " + std::string(where);
   else
      tmp = "select " + std::string(what) + " from " + std::string(TableName());

   if (connection->query("%s", tmp.c_str()))
      return connection->errorSql(connection, "countWhere()", 0, tmp.c_str());

   if ((res = mysql_store_result(connection->getMySql())))
   {
      data = mysql_fetch_row(res);

      if (data)
         count = atoi(data[0]);

      mysql_free_result(res);
   }

   return success;
}

//***************************************************************************
// Truncate
//***************************************************************************

int cDbTable::truncate()
{
   std::string tmp;

   tmp = "delete from " + std::string(TableName());

   if (connection->query("%s", tmp.c_str()))
      return connection->errorSql(connection, "truncate() 'delete from'", 0, tmp.c_str());

   tmp = "truncate table " + std::string(TableName());

   if (connection->query("%s", tmp.c_str()))
      return connection->errorSql(connection, "truncate()", 0, tmp.c_str());

   return success;
}

//***************************************************************************
// Store
//***************************************************************************

int cDbTable::store()
{
   int found;

   // insert or just update ...

   if (stmtSelect->execute(/*noResult =*/ yes) != success)
   {
      connection->errorSql(connection, "store()");
      return no;
   }

   found = stmtSelect->getAffected() == 1;
   stmtSelect->freeResult();

   if (found)
      return update();
   else
      return insert();
}

//***************************************************************************
// Insert
//***************************************************************************

int cDbTable::insert(time_t inssp)
{
   std::map<std::string, cDbFieldDef*>::iterator f;
   lastInsertId = na;

   if (!stmtInsert)
   {
      tell(eloAlways, "Fatal missing insert statement\n");
      return fail;
   }

   for (f = tableDef->dfields.begin(); f != tableDef->dfields.end(); f++)
   {
      cDbFieldDef* fld = f->second;

      if (strcasecmp(fld->getName(), "updsp") == 0 || strcasecmp(fld->getName(), "inssp") == 0)
         setValue(fld, inssp ? inssp : time(0));

      else if (getValue(fld)->isNull() && !isEmpty(fld->getDefault()))
         setValue(fld, fld->getDefault());
   }

   if (stmtInsert->execute())
      return fail;

   lastInsertId = stmtInsert->getLastInsertId();

   return stmtInsert->getAffected() == 1 ? success : fail;
}

//***************************************************************************
// Update
//***************************************************************************

int cDbTable::update(time_t updsp)
{
   std::map<std::string, cDbFieldDef*>::iterator f;

   if (!stmtUpdate)
   {
      tell(eloAlways, "Fatal missing update statement\n");
      return fail;
   }

   for (f = tableDef->dfields.begin(); f != tableDef->dfields.end(); f++)
   {
      cDbFieldDef* fld = f->second;

      if (strcasecmp(fld->getName(), "updsp") == 0)
         setValue(fld, updsp ? updsp : time(0));

      else if (getValue(fld)->isNull() && !isEmpty(fld->getDefault()))
         setValue(fld, fld->getDefault());
   }

   if (stmtUpdate->execute())
      return fail;

   return stmtUpdate->getAffected() == 1 ? success : fail;
}

//***************************************************************************
// Find
//***************************************************************************

int cDbTable::find()
{
   if (!stmtSelect)
      return no;

   if (stmtSelect->execute() != success)
   {
      connection->errorSql(connection, "find()");
      return no;
   }

   return stmtSelect->getAffected() == 1 ? yes : no;
}

//***************************************************************************
// Find via Statement
//***************************************************************************

int cDbTable::find(cDbStatement* stmt)
{
   if (!stmt)
      return no;

   if (stmt->execute() != success)
   {
      connection->errorSql(connection, "find(stmt)");
      return no;
   }

   return stmt->getAffected() > 0 ? yes : no;
}

//***************************************************************************
// Fetch
//***************************************************************************

int cDbTable::fetch(cDbStatement* stmt)
{
   if (!stmt)
      return no;

   return stmt->fetch();
}

//***************************************************************************
// Reset Fetch
//***************************************************************************

void cDbTable::reset(cDbStatement* stmt)
{
   if (stmt)
      stmt->freeResult();
}
