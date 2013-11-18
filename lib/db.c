/*
 * db.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <stdio.h>
#include <stdarg.h>

#include <mysql/errmsg.h>

#include "db.h"

//***************************************************************************
// DB Statement
//***************************************************************************

cDbStatement::cDbStatement(Table* aTable)
{
   table = aTable;
   row = table->getRow();
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

//***************************************************************************
// Execute
//***************************************************************************

int cDbStatement::execute(int noResult)
{
   affected = 0;

   if (!stmt)
      return cDbConnection::errorSql(table->getConnection(), "execute(missing statement)");

   // tell(0, "execute %d [%s]", stmt, stmtTxt.c_str());

   if (mysql_stmt_execute(stmt))
      return cDbConnection::errorSql(table->getConnection(), "execute(stmt_execute)", stmt, stmtTxt.c_str());

   // out binding - if needed

   if (outCount && !noResult)
   {
      if (mysql_stmt_store_result(stmt))
         return cDbConnection::errorSql(table->getConnection(), "execute(store_result)", stmt, stmtTxt.c_str());

      // fetch the first result - if any

      if (mysql_stmt_affected_rows(stmt) > 0)
         mysql_stmt_fetch(stmt);
   }
   else if (outCount)
   {
      mysql_stmt_store_result(stmt);
   }

   // result was stored (above) only if output (outCound) is expected, 
   // therefor we dont need to call freeResult() after insert() or update()
   
   affected = mysql_stmt_affected_rows(stmt);

   return success;
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
   return bind(row->getValue(field), mode, delim);
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

int cDbStatement::bindCmp(const char* table, cDbValue* value, 
                          const char* comp, const char* delim)
{
   if (table)
      build("%s.", table);

   build("%s%s %s ?", delim ? delim : "", value->getName(), comp);

   appendBinding(value, bndIn);

   return success;
}

int cDbStatement::bindCmp(const char* table, int field, cDbValue* value, 
                          const char* comp, const char* delim)
{
   cDbValue* vf = row->getValue(field);
   cDbValue* vv = value ? value : vf;

   if (table)
      build("%s.", table);

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
   memset(newBinding, 0, sizeof(MYSQL_BIND));

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
      
      newBinding->length = 0;            // #TODO
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
   if (!stmtTxt.length())
      return fail;
   
   stmt = mysql_stmt_init(table->getMySql());
   
   // prepare statement
   
   if (mysql_stmt_prepare(stmt, stmtTxt.c_str(), stmtTxt.length()))
      return cDbConnection::errorSql(table->getConnection(), "prepare(stmt_prepare)", stmt, stmtTxt.c_str());

   if (outBind)
   {
      if (mysql_stmt_bind_result(stmt, outBind))
         return cDbConnection::errorSql(table->getConnection(), "execute(bind_result)", stmt);
   }

   if (inBind)
   {
      if (mysql_stmt_bind_param(stmt, inBind))
         return cDbConnection::errorSql(table->getConnection(), "buildPrimarySelect(bind_param)", stmt);
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
// Class Table
//***************************************************************************

char* Table::confPath = 0;

char* cDbConnection::encoding = 0;
char* cDbConnection::dbHost = strdup("localhost");
int   cDbConnection::dbPort = 3306;
int   cDbConnection::connectDropped = yes;
char* cDbConnection::dbUser = 0;
char* cDbConnection::dbPass = 0;
char* cDbConnection::dbName = 0;

//***************************************************************************
// Object
//***************************************************************************

Table::Table(cDbConnection* aConnection, const char* name, FieldDef* f, ViewDef* v)
{
   connection = aConnection;
   tableName = name;
   row = new cDbRow(f);
   holdInMemory = no;

   stmtSelect = 0;
   stmtInsert = 0;
   stmtUpdate = 0;

   viewDef = 0;
   viewStatements = 0;
   viewCount = 0;

   useView(v);
}

Table::~Table()
{
   close();

   delete row;
}

//***************************************************************************
// 
//***************************************************************************

void Table::useView(ViewDef* v)
{ 
   freeViewStatements();
   viewDef = v;
   
   if (viewDef)
      for (viewCount = 0; getView(viewCount)->name; viewCount++) ;
} 

//***************************************************************************
// Open / Close
//***************************************************************************

int Table::open()
{
   if (connection->attachConnection() != success)
      return fail;
   
   return init();
}

int Table::close()
{
   if (stmtSelect) { delete stmtSelect; stmtSelect = 0; }
   if (stmtInsert) { delete stmtInsert; stmtInsert = 0; }
   if (stmtUpdate) { delete stmtUpdate; stmtUpdate = 0; }

   freeViewStatements();

   connection->detachConnection();

   return success;
}

//***************************************************************************
// Init
//***************************************************************************

int Table::init()
{
   string str;

   if (!isConnected()) return fail;

   // ------------------------------
   // check/create table ...

   MYSQL_RES* result = mysql_list_tables(connection->getMySql(), tableName);
   MYSQL_ROW tabRow = mysql_fetch_row(result);

   mysql_free_result(result);

   if (!tabRow)  // no row selected -> table don't exists -> create table
   {
      if (createTable() != success)
         return fail;
   }

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
      if (row->getField(i)->type & ftCalc)
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
      if (row->getField(i)->type & ftPrimary || row->getField(i)->type & ftCalc)
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

   // -----------------------------------------
   // prepare selects for all defined views ..
   
   createViewStatements();

   return success;
}

//***************************************************************************
// Create Table
//***************************************************************************

int Table::createTable()
{
   string statement;

   tell(0, "Initialy creating table '%s'", tableName);

   // build 'create' statement ...

   statement = string("create table ") + tableName + string("(");

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
         if (!size)
         {
            // set default sizes

            if (getField(i)->format == ffAscii || getField(i)->format == ffText)
               size = 100;
            else if (getField(i)->format == ffInt)
               size = 11;
            else if (getField(i)->format == ffFloat)
               size = 10;
         }
         
         if (getField(i)->format != ffDateTime)
         {
            if (getField(i)->format == ffFloat)
               size -= 2;             // we use a default 2 digits after the decimal point
            
            sprintf(num, "%d", size);
            statement += "(" + string(num);
            
            if (getField(i)->format == ffFloat)
               statement += ",2";     // we use a default 2 digits after the decimal point
            
            statement += ")";
         }
      }
   }

   statement += string(", PRIMARY KEY(");

   for (int i = 0, n = 0; getField(i)->name; i++)
   {
      if (getField(i)->type & ftPrimary)
      {
         if (n++) statement += string(", ");
         statement += string(getField(i)->name) + " DESC";
      }
   }

   // statement += string(")) ENGINE MYISAM;");
   statement += string(")) ENGINE InnoDB;");

   tell(1, "%s", statement.c_str());

   if (mysql_query(connection->getMySql(), statement.c_str()))
      return cDbConnection::errorSql(getConnection(), "createTable()", 0, statement.c_str());

   // create indexes - loop over logical and 'real' views ...

   if (getView(0))
   {
      for (int i = 0; getView(i)->name; i++)
      {
         ViewDef* view = getView(i);

         // create indexes for logical (non database) views ...
         
         if (!view->critCount)
            continue;
         
         // index anlegen
         
         statement = "create index idx" + string(view->name);
         
         if (view->type != vtSelect)
            statement += num2Str(i);
         
         statement += " on " + string(tableName) + "(";
         
         int n = 0;
         
         for (int f = 0; view->crit[f].fieldIndex != na; f++)
         {              
            FieldDef* fld = getField(view->crit[f].fieldIndex);
            
            if (fld && !(fld->type & ftCalc))
            {
               if (n++) statement += string(", ");
               statement += fld->name;
            }
         }
         
         if (!n) continue;
         
         statement += ");";
         tell(1, "%s", statement.c_str());
         
         if (mysql_query(connection->getMySql(), statement.c_str()))
            return cDbConnection::errorSql(getConnection(), "createTable()", 0, statement.c_str());
         
         // view ..

         if (view->type == vtDbView)
         {
            // create 'real' database view 

            if (createView(getView(i)) != success)
               return fail;
         }
      }
   }

   return success;
}

//***************************************************************************
// Create View
//***************************************************************************

int Table::createView(ViewDef* view)
{
   char* tmp;
   FILE* f;
   char* buffer;
   int size = 1000;
   int nread = 0;
   int res;

   asprintf(&tmp, "%s/%s.sql", confPath, view->name);

   tell(0, "Initialy creating view '%s' for %s using definition in '%s'", 
        view->name, tableName, tmp);
   
   if (!(f = fopen(tmp, "r")))
   {
      free(tmp);
      tell(0, "Fatal: Can't access '%s'; %m", tmp);
      return fail;
   }

   free(tmp);
   buffer = (char*)malloc(size+1);
   
   while ((res = fread(buffer+nread, 1, 1000, f)))
   {
      nread += res;
      size += 1000;
      buffer = (char*)realloc(buffer, size+1);
   }

   fclose(f);
   buffer[nread] = 0;

   // execute statement

   tell(0, "Executing '%s'", buffer);

   if (mysql_query(connection->getMySql(), buffer))
   {
      free(buffer);
      return cDbConnection::errorSql(connection, "createView()");
   }

   free(buffer);
   
   return success;
}

void Table::freeViewStatements()
{
   if (viewStatements)
   {
      for (int i = 0; i < viewCount; i++)
         delete viewStatements[i];
      
      delete[] viewStatements;
      viewStatements = 0;
   }
}

int Table::createViewStatements()
{
   freeViewStatements();

   // select eventid, channelid, source, delflg, fileref, tableid, version, title, shorttext, starttime, duration, parentalrating, vps, description 
   //   from eventsview
   //      where updsp > ?(select count(1) from channelmap where channelname = channelid and eventsview.source = channelmap.source) > 0 order by channelid;

   if (viewCount)
   {
      viewStatements = new cDbStatement*[viewCount];
      
      for (int i = 0; i < viewCount; i++)
      {
         ViewDef* view = getView(i);
         cDbStatement* stmt = new cDbStatement(this);

         viewStatements[i] = stmt;
         
         // select by ...
         
         stmt->build("select ");
         
         if (view->fields[0] == all)
         {
            for (int i = 0, n = 0; row->getField(i)->name; i++)
            {
               if (row->getField(i)->type & ftCalc)
                  continue;
               
               stmt->bind(i, bndOut, n++ ? ", " : "");
            }
         }
         else
         {
            for (int f = 0, n = 0; view->fields[f] != na; f++)
               stmt->bind(view->fields[f], bndOut, n++ ? ", " : "");
         }
         
         stmt->build(" from %s", view->type == vtDbView || view->type == vtDbViewSelect ? 
                     view->name : TableName());
         
         if (view->critCount)
         {
            stmt->build(" where ");
            
            for (int f = 0, n = 0; view->crit[f].fieldIndex != na; f++)
            {
               if (view->crit[f].fieldIndex != fiNone)
                  stmt->bindCmp(0, view->crit[f].fieldIndex, 0, view->crit[f].op, n++ ? " and " : "");
               else
                  stmt->build("%s%s", n++ ? " and " : "", view->crit[f].op);
            }
         }
         
         if (view->order && row->getField(view->order))
         {
            stmt->build(" order by %s", row->getField(view->order)->name);
         }

         stmt->build(";");
         
         if (stmt->prepare() != success)
            return fail;
      }
   }
   
   return success;
}

//***************************************************************************
// 
//***************************************************************************

void Table::copyValues(cDbRow* r)
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
// Query
//***************************************************************************

int Table::query(const char* statement)
{
   if (mysql_query(connection->getMySql(), statement))
      return cDbConnection::errorSql(connection, "query()", 0, statement);

   return success;
}

//***************************************************************************
// Delete Where
//***************************************************************************

int Table::deleteWhere(const char* where)
{
   string tmp;

   if (!connection || !connection->getMySql())
      return fail;

   tmp = "delete from " + string(tableName) + " where " + string(where);
   
   if (mysql_query(connection->getMySql(), tmp.c_str()))
      return cDbConnection::errorSql(connection, "deleteWhere()", 0, tmp.c_str());

   return success;
}

//***************************************************************************
// Coiunt Where
//***************************************************************************

int Table::countWhere(const char* where, int& count)
{
   string tmp;
   MYSQL_RES* res;
   MYSQL_ROW data;

   count = 0;

   if (where && *where)
      tmp = "select count(1) from " + string(tableName) + " where " + string(where);
   else
      tmp = "select count(1) from " + string(tableName);
   
   if (mysql_query(connection->getMySql(), tmp.c_str()))
      return cDbConnection::errorSql(connection, "countWhere()", 0, tmp.c_str());

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

int Table::truncate()
{
   string tmp;

   tmp = "delete from " + string(tableName);

   if (mysql_query(connection->getMySql(), tmp.c_str()))
      return cDbConnection::errorSql(connection, "truncate()", 0, tmp.c_str());

   return success;
}


//***************************************************************************
// Store
//***************************************************************************

int Table::store()
{
   int found;

   // insert or just update ...

   if (stmtSelect->execute(/*noResult =*/ yes) != success)
   {
      cDbConnection::errorSql(connection, "store()");
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

int Table::insert()
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

   if (stmtInsert->execute())
      return fail;

   return stmtInsert->getAffected() == 1 ? success : fail;
}

//***************************************************************************
// Update
//***************************************************************************

int Table::update()
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

   if (stmtUpdate->execute())
      return fail;

   return stmtUpdate->getAffected() == 1 ? success : fail;
}

//***************************************************************************
// Find
//***************************************************************************

int Table::find()
{
   if (!stmtSelect)
      return no;

   if (stmtSelect->execute() != success)
   {
      cDbConnection::errorSql(connection, "find()");
      return no;
   }

   return stmtSelect->getAffected() == 1 ? yes : no;
}

//***************************************************************************
// Find via View
//***************************************************************************

int Table::find(int viewIndex)
{
   return find(getViewStmt(viewIndex));
}

//***************************************************************************
// Fetch
//***************************************************************************

int Table::fetch(int viewIndex)
{
   return fetch(getViewStmt(viewIndex));
}

//***************************************************************************
// Reset Fetch
//***************************************************************************

void Table::reset(int viewIndex)
{
   reset(viewIndex != na ? getViewStmt(viewIndex) : stmtSelect);
}

//***************************************************************************
// Find via Statement
//***************************************************************************

int Table::find(cDbStatement* stmt)
{
   if (!stmt)
      return no;

   if (stmt->execute() != success)
   {
      cDbConnection::errorSql(connection, "find(stmt)");
      return no;
   }

   return stmt->getAffected() > 0 ? yes : no;
}

//***************************************************************************
// Fetch
//***************************************************************************

int Table::fetch(cDbStatement* stmt)
{
   if (!stmt)
      return no;

   return stmt->fetch();
}

//***************************************************************************
// Reset Fetch
//***************************************************************************

void Table::reset(cDbStatement* stmt)
{
   if (stmt)
      stmt->freeResult();
}
