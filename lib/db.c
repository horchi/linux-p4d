/*
 * db.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <stdio.h>

#include <mysql/errmsg.h>

#include "db.h"

// #define DEB_HANDLER

//***************************************************************************
// DB Statement
//***************************************************************************

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
}

cDbStatement::cDbStatement(cDbConnection* aConnection, const char* stmt)
{
   table = 0;
   connection = aConnection;
   stmtTxt = stmt;
   stmt = 0;
   inCount = 0;
   outCount = 0;
   inBind = 0;
   outBind = 0;
   affected = 0;
   metaResult = 0;
   bindPrefix = 0;
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

   // tell(0, "execute %d [%s]", stmt, stmtTxt.c_str());

   if (mysql_stmt_execute(stmt))
      return connection->errorSql(connection, "execute(stmt_execute)", stmt, stmtTxt.c_str());

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

int cDbStatement::getResultCount()
{
   mysql_stmt_store_result(stmt);

   return mysql_stmt_affected_rows(stmt);
}

int cDbStatement::find()
{
   if (execute() != success)
      return fail;

   return getAffected() > 0 ? yes : no;
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

int cDbStatement::bind(int field, int mode, const char* delim)
{
   return bind(table->getRow()->getValue(field), mode, delim);
}

int cDbStatement::bind(cDbValue* value, int mode, const char* delim)
{
   if (!value || !value->getField())
      return fail;

   if (delim)
      stmtTxt += delim;

   if (bindPrefix)
      stmtTxt += bindPrefix;

   if (mode & bndIn)
   {
      if (mode & bndSet)
         stmtTxt += value->getName() + string(" =");
      
      stmtTxt += " ?";
      appendBinding(value, bndIn);
   }
   else if (mode & bndOut)
   {
      stmtTxt += value->getName();
      appendBinding(value, bndOut);
   }

   return success;
}

int cDbStatement::bindCmp(const char* ctable, cDbValue* value, 
                          const char* comp, const char* delim)
{
   if (ctable)
      build("%s.", ctable);

   build("%s%s %s ?", delim ? delim : "", value->getName(), comp);

   appendBinding(value, bndIn);

   return success;
}

int cDbStatement::bindCmp(const char* ctable, int field, cDbValue* value, 
                          const char* comp, const char* delim)
{
   cDbValue* vf = table->getRow()->getValue(field);
   cDbValue* vv = value ? value : vf;

   if (ctable)
      build("%s.", ctable);

   build("%s%s %s ?", delim ? delim : "", vf->getName(), comp);

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

   if (!bindings)
      *bindings = (MYSQL_BIND*)malloc(count * sizeof(MYSQL_BIND));
   else
      *bindings = (MYSQL_BIND*)realloc(*bindings, count * sizeof(MYSQL_BIND));

   newBinding = &((*bindings)[count-1]);

   if (value->getField()->format == ffAscii || value->getField()->format == ffText)
   {
      newBinding->buffer_type = MYSQL_TYPE_STRING;
      newBinding->buffer = value->getStrValueRef();
      newBinding->buffer_length = value->getField()->size;
      newBinding->length = value->getStrValueSizeRef();
      
      newBinding->is_null = value->getNullRef();
      newBinding->error = 0;            // #TODO
   }
   else if (value->getField()->format == ffMlob)
   {
      newBinding->buffer_type = MYSQL_TYPE_BLOB;
      newBinding->buffer = value->getStrValueRef();
      newBinding->buffer_length = value->getField()->size;
      newBinding->length = value->getStrValueSizeRef();
      
      newBinding->is_null = value->getNullRef();
      newBinding->error = 0;            // #TODO
   }
   else if (value->getField()->format == ffFloat)
   {
      newBinding->buffer_type = MYSQL_TYPE_FLOAT;
      newBinding->buffer = value->getFloatValueRef();
      
      newBinding->length = 0;            // #TODO
      newBinding->is_null =  value->getNullRef();
      newBinding->error = 0;             // #TODO
   }
   else if (value->getField()->format == ffDateTime)
   {
      newBinding->buffer_type = MYSQL_TYPE_DATETIME;
      newBinding->buffer = value->getTimeValueRef();
      
      newBinding->length = 0;            // #TODO
      newBinding->is_null =  value->getNullRef();
      newBinding->error = 0;             // #TODO
   }
   else
   {
      newBinding->buffer_type = MYSQL_TYPE_LONG;
      newBinding->buffer = value->getIntValueRef();
      newBinding->is_unsigned = (value->getField()->format == ffUInt);

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
   if (!stmtTxt.length() || !connection->getMySql())
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

   tell(2, "Statement '%s' with (%d) in parameters and (%d) out bindings prepared", 
        stmtTxt.c_str(), mysql_stmt_param_count(stmt), outCount);
   
   return success;
}

//***************************************************************************
// cDbService
//***************************************************************************

const char* cDbService::formats[] =
{
   "INT",
   "INT",
   "VARCHAR",
   "TEXT",
   "MEDIUMBLOB",
   "FLOAT",
   "DATETIME",

   0
};

const char* cDbService::toString(FieldFormat t)
{
   return formats[t];
}

//***************************************************************************
// Class cDbTable
//***************************************************************************

char* cDbTable::confPath = 0;

char* cDbConnection::encoding = 0;
char* cDbConnection::dbHost = strdup("localhost");
int   cDbConnection::dbPort = 3306;
char* cDbConnection::dbUser = 0;
char* cDbConnection::dbPass = 0;
char* cDbConnection::dbName = 0;

//***************************************************************************
// Object
//***************************************************************************

cDbTable::cDbTable(cDbConnection* aConnection, const char* aName, FieldDef* f, IndexDef* i)
{
   tableName = strdup(aName);
   connection = aConnection;
   row = new cDbRow(f);
   holdInMemory = no;

   stmtSelect = 0;
   stmtInsert = 0;
   stmtUpdate = 0;

   indices = i;
}

cDbTable::~cDbTable()
{
   close();
   free(tableName);

   delete row;
}

//***************************************************************************
// Open / Close
//***************************************************************************

int cDbTable::open()
{
   if (connection->attachConnection() != success)
   {
      tell(0, "Could not access database '%s:%d' (tried to open %s)", 
           connection->getHost(), connection->getPort(), TableName());

      return fail;
   }
   
   return init();
}

int cDbTable::close()
{
   if (stmtSelect) { delete stmtSelect; stmtSelect = 0; }
   if (stmtInsert) { delete stmtInsert; stmtInsert = 0; }
   if (stmtUpdate) { delete stmtUpdate; stmtUpdate = 0; }

   connection->detachConnection();

   return success;
}

//***************************************************************************
// Init
//***************************************************************************

int cDbTable::init()
{
   string str;

   if (!isConnected()) return fail;

   // check/create table ...

   if (createTable() != success)
      return fail;

   // ------------------------------
   // prepare BASIC statements
   // ------------------------------

   // select by primary key ...

   stmtSelect = new cDbStatement(this);
   
   stmtSelect->build("select ");
   
   for (int i = 0, n = 0; row->getField(i)->name; i++)
   {
      if (row->getField(i)->type & ftCalc)
         continue;
      
      stmtSelect->bind(i, bndOut, n++ ? ", " : "");
   }
   
   stmtSelect->build(" from %s where ", TableName());
   
   for (int i = 0, n = 0; row->getField(i)->name; i++)
   {
      if (!(row->getField(i)->type & ftPrimary))
         continue;
      
      stmtSelect->bind(i, bndIn | bndSet, n++ ? " and " : "");
   }
   
   stmtSelect->build(";");
 
   if (stmtSelect->prepare() != success)
      return fail;

   // -----------------------------------------
   // insert 

   stmtInsert = new cDbStatement(this);

   stmtInsert->build("insert into %s set ", TableName());

   for (int i = 0, n = 0; row->getField(i)->name; i++)
   {
      // don't insert autoinc and calculated fields

      if (row->getField(i)->type & ftCalc || row->getField(i)->type & ftAutoinc)
         continue;

      stmtInsert->bind(i, bndIn | bndSet, n++ ? ", " : "");
   }

   stmtInsert->build(";");

   if (stmtInsert->prepare() != success)
      return fail;

   // -----------------------------------------
   // update via primary key ...

   stmtUpdate = new cDbStatement(this);

   stmtUpdate->build("update %s set ", TableName());
         
   for (int i = 0, n = 0; row->getField(i)->name; i++)
   {
      // don't update PKey, autoinc and calculated fields

      if (row->getField(i)->type & ftPrimary || 
          row->getField(i)->type & ftCalc || 
          row->getField(i)->type & ftAutoinc)
         continue;
      
      if (strcmp(row->getField(i)->name, "inssp") == 0)  // don't update the insert stamp
         continue;
      
      stmtUpdate->bind(i, bndIn | bndSet, n++ ? ", " : "");
   }

   stmtUpdate->build(" where ");

   for (int i = 0, n = 0; row->getField(i)->name; i++)
   {
      if (!(row->getField(i)->type & ftPrimary))
         continue;

      stmtUpdate->bind(i, bndIn | bndSet, n++ ? " and " : "");
   }

   stmtUpdate->build(";");

   if (stmtUpdate->prepare() != success)
      return fail;

   return success;
}

//***************************************************************************
// Check Table 
//***************************************************************************

int cDbTable::exist(const char* name)
{
   if (!name)
      name = TableName();

   MYSQL_RES* result = mysql_list_tables(connection->getMySql(), name);
   MYSQL_ROW tabRow = mysql_fetch_row(result);
   mysql_free_result(result);

   return tabRow ? yes : no;
}

//***************************************************************************
// Create Table
//***************************************************************************

int cDbTable::createTable()
{
   string statement;
   string aKey;

   if (!isConnected())
   {
      if (connection->attachConnection() != success)
      {
         tell(0, "Could not access database '%s:%d' (tried to create %s)", 
              connection->getHost(), connection->getPort(), TableName());
         
         return fail;
      }
   }

   // table exists -> nothing to do

   if (exist())
      return done;

   tell(0, "Initialy creating table '%s'", TableName());

   // build 'create' statement ...

   statement = string("create table ") + TableName() + string("(");

   for (int i = 0; getField(i)->name; i++)
   {
      int size = getField(i)->size;
      char num[10];

      if (getField(i)->type & ftCalc)
         continue;

      if (i) statement += string(", ");
      statement += string(getField(i)->name) + " " + string(toString(getField(i)->format));

      if (getField(i)->format != ffMlob)
      {
         if (!size) size = getField(i)->format == ffAscii || getField(i)->format == ffText ? 100 : 11;

         if (getField(i)->format != ffFloat)
            sprintf(num, "%d", size);
         else
            sprintf(num, "%d,%d", size/10, size%10);

         statement += "(" + string(num) + ")";

         if (getField(i)->format == ffUInt)
            statement += " unsigned";

         if (getField(i)->type & ftAutoinc)
            statement += " not null auto_increment";
         else if (getField(i)->type & ftDef0)
            statement += " default '0'";
      }
   }

   aKey = "";

   for (int i = 0, n = 0; getField(i)->name; i++)
   {
      if (getField(i)->type & ftPrimary)
      {
         if (n++) aKey += string(", ");
         aKey += string(getField(i)->name) + " DESC";
      }
   }

   if (aKey.length())
   {
      statement += string(", PRIMARY KEY(");
      statement += aKey;
      statement += ")";
   }

   aKey = "";

   for (int i = 0, n = 0; getField(i)->name; i++)
   {
      if (getField(i)->type & ftAutoinc && !(getField(i)->type & ftPrimary))
      {
         if (n++) aKey += string(", ");
         aKey += string(getField(i)->name) + " DESC";
      }
   }

   if (aKey.length())
   {
      statement += string(", KEY(");
      statement += aKey;
      statement += ")";
   }

   // statement += string(") ENGINE MYISAM;");
   statement += string(") ENGINE InnoDB;");

   tell(1, "%s", statement.c_str());

   if (connection->query(statement.c_str()))
      return connection->errorSql(getConnection(), "createTable()", 
                                  0, statement.c_str());

   // create indices

   createIndices();  

   return success;
}

//***************************************************************************
// Create Indices
//***************************************************************************

int cDbTable::createIndices()
{
   string statement;

   tell(5, "Initialy checking indices for '%s'", TableName());

   // check/create indexes

   if (!indices)
      return done;

   for (int i = 0; getIndex(i)->name; i++)
   {
      IndexDef* index = getIndex(i);
      int fCount;
      string idxName;
      int expectCount = 0;

      for (; index->fields[expectCount] != na; expectCount++) ;

      if (!expectCount)
         continue;
         
      // check

      idxName = "idx" + string(index->name);

      checkIndex(idxName.c_str(), fCount);

      if (fCount != expectCount)
      {
         // create index
            
         statement = "create index " + idxName;
         statement += " on " + string(TableName()) + "(";
            
         int n = 0;
            
         for (int f = 0; index->fields[f] != na; f++)
         {              
            FieldDef* fld = getField(index->fields[f]);
               
            if (fld && !(fld->type & ftCalc))
            {
               if (n++) statement += string(", ");
               statement += fld->name;
            }
         }
            
         if (!n) continue;
            
         statement += ");";
         tell(1, "%s", statement.c_str());
            
         if (connection->query(statement.c_str()))
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
         tell(5, "%s:  %-20s %s %s", 
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

void cDbTable::copyValues(cDbRow* r)
{
   for (int i = 0; i < fieldCount(); i++)
   {
      if (getField(i)->format == ffAscii || getField(i)->format == ffText)
         row->setValue(i, r->getStrValue(i));
      else
         row->setValue(i, r->getIntValue(i));
   }
}

//***************************************************************************
// SQL Error 
//***************************************************************************

int cDbConnection::errorSql(cDbConnection* connection, const char* prefix, MYSQL_STMT* stmt, const char* stmtTxt)
{
   if (!connection || !connection->mysql)
   {
      tell(0, "SQL-Error in '%s'", prefix);
      return fail;
   }

   int error = mysql_errno(connection->mysql);
   char* conErr = 0;
   char* stmtErr = 0;

   if (error == CR_SERVER_LOST ||
       error == CR_SERVER_GONE_ERROR ||
       error == CR_INVALID_CONN_HANDLE ||
       error == CR_SERVER_LOST_EXTENDED)
      connectDropped = yes;

   if (error)
      asprintf(&conErr, "%s (%d) ", mysql_error(connection->mysql), error);

   if (stmt || stmtTxt)
      asprintf(&stmtErr, "'%s' [%s]",
               stmt ? mysql_stmt_error(stmt) : "",
               stmtTxt ? stmtTxt : "");

   tell(0, "SQL-Error in '%s' - %s%s", prefix, 
        conErr ? conErr : "", stmtErr ? stmtErr : "");

   free(conErr);
   free(stmtErr);

   if (connectDropped)
      tell(0, "Fatal, lost connection to mysql server, aborting pending actions");

   return fail;
}

//***************************************************************************
// Delete Where
//***************************************************************************

int cDbTable::deleteWhere(const char* where)
{
   string tmp;

   if (!connection || !connection->getMySql())
      return fail;

   tmp = "delete from " + string(TableName()) + " where " + string(where);
   
   if (connection->query(tmp.c_str()))
      return connection->errorSql(connection, "deleteWhere()", 0, tmp.c_str());

   return success;
}

//***************************************************************************
// Coiunt Where
//***************************************************************************

int cDbTable::countWhere(const char* where, int& count, const char* what)
{
   string tmp;
   MYSQL_RES* res;
   MYSQL_ROW data;

   count = 0;
   
   if (isEmpty(what))
      what = "count(1)";

   if (!isEmpty(where))
      tmp = "select " + string(what) + " from " + string(TableName()) + " where " + string(where);
   else
      tmp = "select " + string(what) + " from " + string(TableName());
   
   if (connection->query(tmp.c_str()))
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
   string tmp;

   tmp = "delete from " + string(TableName());

   if (connection->query(tmp.c_str()))
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

int cDbTable::insert()
{
   if (!stmtInsert)
   {
      tell(0, "Fatal missing insert statement\n");
      return fail;
   }

   for (int i = 0; getField(i)->name; i++)
   {
      if (strcmp(getField(i)->name, "updsp") == 0 || strcmp(getField(i)->name, "inssp") == 0)
         setValue(getField(i)->index, time(0));
   }

#ifdef DEB_HANDLER

   if (strcmp(TableName(), "events") == 0)
      tell(1, "inserting vdr event %d for '%s', starttime = %ld, updflg = '%s'", 
           getIntValue(0), getStrValue(1), getIntValue(15), getStrValue(6));
#endif

   if (stmtInsert->execute())
      return fail;

   return stmtInsert->getAffected() == 1 ? success : fail;
}

//***************************************************************************
// Update
//***************************************************************************

int cDbTable::update()
{
   if (!stmtUpdate)
   {
      tell(0, "Fatal missing update statement\n");
      return fail;
   }

   for (int i = 0; getField(i)->name; i++)
   {
      if (strcmp(getField(i)->name, "updsp") == 0)
      {
         setValue(getField(i)->index, time(0));
         break;
      }
   }

#ifdef DEB_HANDLER
   if (strcmp(TableName(), "events") == 0)
      tell(1, "updating vdr event %d for '%s', starttime = %ld, updflg = '%s'", 
           getIntValue(0), getStrValue(1), getIntValue(15), getStrValue(6));
#endif

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
