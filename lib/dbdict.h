/*
 * dbdict.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __DBDICT_H
#define __DBDICT_H

#include <stdio.h>

#include <vector>
#include <map>
#include <string>

class cDbDict;

typedef int (*FilterFromName)(const char* name);

//***************************************************************************
// _casecmp_
//***************************************************************************

class _casecmp_
{
   public:

      bool operator() (const std::string& a, const std::string& b) const
      {
         return strcasecmp(a.c_str(), b.c_str()) < 0;
      }
};

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
         ffUnknown = na,

         ffInt,
         ffUInt,
         ffBigInt,
         ffUBigInt,
         ffAscii,      // -> VARCHAR
         ffText,
         ffMText,
         ffMlob,       // -> MEDIUMBLOB
         ffFloat,
         ffDateTime,

         ffCount
      };

      enum FieldType
      {
         ftUnknown    = na,
         ftNo         = 0,

         ftData       = 1,
         ftPrimary    = 2,
         ftMeta       = 4,
         ftAutoinc    = 8,
         ftDef0       = 16,

         ftAll = ftData | ftPrimary | ftMeta
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

      struct TypeDef
      {
         FieldType type;
         const char* name;
      };

      static const char* toString(FieldFormat t);
      static FieldFormat toDictFormat(const char* format);
      static const char* formats[];
      static const char* dictFormats[];

      static int toType(const char* type);
      static const char* toName(FieldType type, char* buf);
      static TypeDef types[];
};

typedef cDbService cDBS;

//***************************************************************************
// cDbFieldDef
//***************************************************************************

class cDbFieldDef : public cDbService
{
   public:

      friend class cDbDict;

      cDbFieldDef()
      {
         name = 0;
         dbname = 0,
         format = ffUnknown;
         size = na;
         type = ftUnknown;
         description = 0;
         dbdescription = 0;
         filter = 0xFFFF;
      }

      cDbFieldDef(const char* n, const char* dn, FieldFormat f, int s, int t, int flt = 0xFFFF)
      {
         name = strdup(n);
         dbname = strdup(dn);
         format = f;
         size = s;
         type = t;
         filter = flt;
         description = 0;
         dbdescription = 0;
      }

      ~cDbFieldDef()  { free(name); free(dbname); free(description); free(dbdescription); }

      int getIndex()                 { return index; }
      const char* getName()          { return name; }
      int hasName(const char* n)     { return strcasecmp(n, name) == 0; }
      int hasDbName(const char* n)   { return strcasecmp(n, dbname) == 0; }
      const char* getDescription()   { return description; }
      const char* getDbDescription() { return dbdescription; }
      const char* getDbName()        { return dbname; }
      int getSize()                  { return size; }
      FieldFormat getFormat()        { return format; }
      int getType()                  { return type; }
      int getFilter()                { return filter; }
      int filterMatch(int f)         { return !f || filter & f; }
      int hasType(int types)         { return types & type; }
      int hasFormat(int f)           { return format == f; }

      int isString()                 { return format == ffAscii || format == ffText ||
                                              format == ffMText || format == ffMlob; }
      int isInt()                    { return format == ffInt || format == ffUInt; }
      int isBigInt()                 { return format == ffBigInt || format == ffUBigInt; }
      int isFloat()                  { return format == ffFloat; }
      int isDateTime()               { return format == ffDateTime; }

      void setDescription(const char* desc)
      {
         description = strdup(desc);
         dbdescription = strdup(strReplace("'", "\\'", description).c_str());
      }

      const char* toColumnFormat(char* buf) // column type to be used for create/alter
      {
         if (!buf)
            return 0;

         sprintf(buf, "%s", toString(format));

         if (format != ffMlob)
         {
            if (!size)
            {
               if (format == ffAscii)
                  size = 100;
               else if (format == ffInt || format == ffUInt || format == ffBigInt || format == ffUBigInt)
                  size = 11;
               else if (format == ffFloat)
                  size = 10;
            }

            if (format == ffFloat)
               sprintf(eos(buf), "(%d,%d)", size/10 + size%10, size%10); // 62 -> 8,2
            else if (format == ffInt || format == ffUInt || format == ffAscii)
               sprintf(eos(buf), "(%d)", size);

            if (format == ffUInt || format == ffUBigInt)
               sprintf(eos(buf), " unsigned");
         }

         return buf;
      }

      int isValid()
      {
         if (!name)               { tell(0, "Missing field name");               return no; }
         if (!dbname)             { tell(0, "Missing fields database name");     return no; }
         if (size == na)          { tell(0, "Missing field size");               return no; }
         if (type == ftUnknown)   { tell(0, "Missing or invalid field type");    return no; }
         if (format == ffUnknown) { tell(0, "Missing or invalid field format");  return no; }

         return yes;
      }

      void show()
      {
         char colFmt[100];
         char fType[100];
         char tmp[100];

         sprintf(fType, "(%s)", toName((FieldType)type, tmp));

         tell(0, "%-20s %-25s %-17s %-20s (0x%04X) '%s'", name, dbname,
              toColumnFormat(colFmt), fType, filter, description);
      }

   protected:

      char* name;
      char* dbname;
      char* description;
      char* dbdescription;
      FieldFormat format;
      int size;
      int index;
      int type;
      int filter;     // bitmask (defaults to 0xFFFF)
};

//***************************************************************************
// cDbIndexDef
//***************************************************************************

class cDbIndexDef
{
   public:

      cDbIndexDef()  { name = 0; description = 0; }
      ~cDbIndexDef() { free(name); free(description); }

      void setName(const char* n) { free(name); name = strdup(n); }
      const char* getName()       { return name; }

      void setDescription(const char* d) { free(description); description = strdup(d); }
      const char* getDescription()       { return description; }

      int fieldCount()                    { return dfields.size(); }
      void addField(cDbFieldDef* f)       { dfields.push_back(f); }
      cDbFieldDef* getField(int i)        { return dfields[i]; }

      void show()
      {
         std::string s = "";

         for (uint i = 0; i < dfields.size(); i++)
            s += dfields[i]->getName() + std::string(" ");

         s.erase(s.find_last_not_of(' ')+1);

         tell(0, "Index %-25s (%s)", getName(), s.c_str());
      }

   protected:

      char* name;
      char* description;
      std::vector<cDbFieldDef*> dfields;  // index fields
};

//***************************************************************************
// cDbTableDef
//***************************************************************************

class cDbTableDef : public cDbService
{
   public:

      friend class cDbRow;
      friend class cDbTable;
      friend class cDbStatement;

      cDbTableDef(const char* n)       { name = strdup(n); }

      ~cDbTableDef()
      {
         for (uint i = 0; i < indices.size(); i++)
            delete indices[i];

         indices.clear();

         free(name);
         clear();
      }

      const char* getName()            { return name; }
      int fieldCount()                 { return dfields.size(); }
      cDbFieldDef* getField(int id)    { return _dfields[id]; }

      cDbFieldDef* getField(const char* fname, int silent = no)
      {
         std::map<std::string, cDbFieldDef*, _casecmp_>::iterator f;

         if ((f = dfields.find(fname)) != dfields.end())
            return f->second;

         if (!silent)
            tell(0, "Fatal: Missing definition of field '%s.%s' in dictionary!", name, fname);

         return 0;
      }

      cDbFieldDef* getFieldByDbName(const char* dbname)
      {
         std::map<std::string, cDbFieldDef*, _casecmp_>::iterator it;

         for (it = dfields.begin(); it != dfields.end(); it++)
         {
            if (it->second->hasDbName(dbname))
               return it->second;
         }

         tell(5, "Fatal: Missing definition of field '%s.%s' in dictionary!", name, dbname);

         return 0;
      }

      void addField(cDbFieldDef* f)
      {
         if (dfields.find(f->getName()) == dfields.end())
         {
            dfields[f->getName()] = f;   // add to named map
            _dfields.push_back(f);       // add to indexed list
         }
         else
            tell(0, "Fatal: Field '%s.%s' doubly defined", getName(), f->getName());
      }

      int indexCount()                    { return indices.size(); }
      cDbIndexDef* getIndex(int i)        { return indices[i]; }
      void addIndex(cDbIndexDef* i)       { indices.push_back(i); }

      void clear()
      {
         std::map<std::string, cDbFieldDef*>::iterator f;

         while ((f = dfields.begin()) != dfields.end())
         {
            if (f->second)
               delete f->second;

            dfields.erase(f);
         }
      }

      void show()
      {
         // show fields

         for (uint i = 0; i < _dfields.size(); i++)
            _dfields[i]->show();

         // show indices

         if (!indices.size())
         {
            tell(0, " ");
            return;
         }

         tell(0, "-----------------------------------------------------");
         tell(0, "Indices from '%s'", getName());
         tell(0, "-----------------------------------------------------");

         for (uint i = 0; i < indices.size(); i++)
            indices[i]->show();

         tell(0, " ");
      }

   private:

      char* name;
      std::vector<cDbIndexDef*> indices;

      // FiledDefs stored as list to have access via index
      std::vector<cDbFieldDef*> _dfields;

      // same FiledDef references stored as a map to have access via name
      std::map<std::string, cDbFieldDef*, _casecmp_> dfields;
};

//***************************************************************************
// cDbDict
//***************************************************************************

class cDbDict : public cDbService
{
   public:

      // declarations

      enum TableDictToken
      {
         dtName,
         dtDescription,
         dtDbName,
         dtFormat,
         dtSize,
         dtType,
         dtFilter,

         dtCount
      };

      enum IndexDictToken
      {
         idtName,
         idtDescription,
         idtFields
      };

      cDbDict();
      virtual ~cDbDict();

      int in(const char* file, int filter = 0);
      void setFilterFromNameFct(FilterFromName fct)  { fltFromNameFct = fct; }

      cDbTableDef* getTable(const char* name);
      void show();
      int init(cDbFieldDef*& field, const char* tname, const char* fname);
      const char* getPath() { return path ? path : ""; }
      void forget();

      std::map<std::string, cDbTableDef*>::iterator getFirstTableIterator() { return tables.begin(); }
      std::map<std::string, cDbTableDef*>::iterator getTableEndIterator()   { return tables.end(); }

   protected:

      int atLine(const char* line);
      int parseField(const char* line);
      int parseIndex(const char* line);
      int toFilter(char* token);

      // data

      int inside;
      cDbTableDef* curTable;
      std::map<std::string, cDbTableDef*, _casecmp_> tables;
      char* path;
      int fieldFilter;
      FilterFromName fltFromNameFct;
};

extern cDbDict dbDict;

//***************************************************************************
#endif //  __DBDICT_H
