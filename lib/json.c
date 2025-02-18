/*
 * json.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifdef USEJSON

#include "json.h"

const char* charset = "utf-8";  // #TODO, move to configuration?

//***************************************************************************
// Copy JSON Object to Data Buffer
//***************************************************************************

int json2Data(json_t* obj, MemoryStruct* data, const char* encoding)
{
   int status = success;

   // will be freed by data's dtor

   data->memory = json_dumps(obj, JSON_PRESERVE_ORDER);
   data->size = strlen(data->memory);
   sprintf(data->contentType, "application/json; charset=%s", charset);

   // gzip content if supported by browser ...

   if (data->size && encoding && strstr(encoding, "gzip"))
      status = data->toGzip();

   return status;
}

//***************************************************************************
// Load JSON Object from String
//***************************************************************************

json_t* jsonLoad(const char* data)
{
   json_error_t error;
   json_t* jData = json_loads(data, 0, &error);

   if (!jData)
   {
      tell(eloAlways, "Error: Ignoring invalid json in '%s' [%s]", data, getBacktrace(5).c_str());
      tell(eloAlways, "Error decoding json: %s (%s, line %d column %d, position %d)",
           error.text, error.source, error.line, error.column, error.position);
      return nullptr;
   }

   return jData;

}

#ifdef USEDB
//***************************************************************************
// Add field to json object
//***************************************************************************

int addFieldToJson(json_t* obj, cDbValue* value, int ignoreEmpty, const char* extName)
{
   char* name;

   if (!value || !value->getField())
      return fail;

   name = strdup(extName ? extName : value->getField()->getName());
   toCase(cLower, name);   // use always lower case names in json

   switch (value->getField()->getFormat())
   {
      case cDBS::ffAscii:
      case cDBS::ffText:
      case cDBS::ffMText:
         if (!ignoreEmpty || !isEmpty(value->getStrValue()))
            json_object_set_new(obj, name, json_string(value->getStrValue()));
         break;

      case cDBS::ffInt:
      case cDBS::ffUInt:
         json_object_set_new(obj, name, json_integer(value->getIntValue()));
         break;

      case cDBS::ffFloat:
         json_object_set_new(obj, name, json_real(value->getFloatValue()));
         break;

      default:
         break;
   }

   free(name);

   return success;
}

//***************************************************************************
// Add field to json object
//***************************************************************************

int addFieldToJson(json_t* obj, cDbTable* table, const char* fname,
                   int ignoreEmpty, const char* extName)
{
   return addFieldToJson(obj, table->getValue(fname), ignoreEmpty, extName);
}

//***************************************************************************
// Get Field From Json
//   - if a default is required put it into the row
//     before calling this function
//***************************************************************************

int getFieldFromJson(json_t* obj, cDbRow* row, const char* fname, const char* extName)
{
   cDbValue* value = row->getValue(fname);
   char* jname;

   if (!value)
      return fail;

   jname = strdup(!isEmpty(extName) ? extName : value->getField()->getName());
   toCase(cLower, jname);   // use always lower case names in json

   switch (value->getField()->getFormat())
   {
      case cDBS::ffAscii:
      case cDBS::ffText:
      case cDBS::ffMText:
      {
         const char* v = getStringFromJson(obj, jname, "");
         if (!isEmpty(v) || !value->isEmpty())
            value->setValue(v);
         break;
      }

      case cDBS::ffInt:
      case cDBS::ffUInt:
      {
         int v = getIntFromJson(obj, jname, 0);
         const char* s = getStringFromJson(obj, jname, "_not_set_");

         if (s && strcmp(s, "_not_set_") == 0)
            break;

         if (s && strcmp(s, "null") == 0)
            value->setNull();
         else
            value->setValue(v);

         break;
      }

//    case cDBS::ffFloat:    #TODO to be implemented
//       {
//          double v = getFloatFromJson(obj, jname, na);
//          if (v != na) value->setValue(v);
//          break;
//       }

      default:
         break;
   }

   return success;
}
#endif

//***************************************************************************
// Check Element
//***************************************************************************

bool isElementSet(json_t* obj, const char* name)
{
   return json_object_get(obj, name);
}

//***************************************************************************
// Get Elements
//***************************************************************************

const char* getStringFromJson(json_t* obj, const char* name, const char* def)
{
   json_t* o = json_object_get(obj, name);

   if (!o)
      return def;

   const char* s = json_string_value(o);

   if (!s)
      return def;

   return s;
}

int getBoolFromJson(json_t* obj, const char* name, bool def)
{
   json_t* o = json_object_get(obj, name);

   if (!o)
      return def;

   return json_boolean_value(o);
}

int getIntFromJson(json_t* obj, const char* name, int def)
{
   json_t* o = json_object_get(obj, name);

   if (!o)
      return def;

   return json_integer_value(o);
}

long getLongFromJson(json_t* obj, const char* name, long def)
{
   json_t* o = json_object_get(obj, name);

   if (!o)
      return def;

   return json_integer_value(o);
}

double getDoubleFromJson(json_t* obj, const char* name, double def)
{
   json_t* o = json_object_get(obj, name);

   if (!o)
      return def;

   if (json_is_integer(o))
      return json_integer_value(o);

   return json_real_value(o);
}

json_t* getObjectFromJson(json_t* obj, const char* name, json_t* def)
{
   json_t* o = json_object_get(obj, name);

   if (!o)
      return def;

   return o;
}

json_t* getObjectByPath(json_t* jData, const char* aPath, json_t* def)
{
   json_t* jElement {jData};
   const auto vPath = split(aPath, '/');

   for (const auto& tag : vPath)
   {
      // chek if array like weather[0]

      std::string sPos = getStringBetween(tag, "[", "]");

      if (sPos != "")
      {
         size_t nElement = atoi(sPos.c_str());
         std::string t = getStringBefore(tag, "[");

         jElement = getObjectFromJson(jElement, t.c_str());

         if (!jElement || !json_is_array(jElement))
            return def;

         jElement = json_array_get(jElement, nElement);
      }
      else
      {
         jElement = getObjectFromJson(jElement, tag.c_str());
      }

      if (!jElement)
         return def;
   }

   return jElement;
}

bool getBoolByPath(json_t* jData, const char* aPath, bool def)
{
   json_t* jObj = getObjectByPath(jData, aPath);

   if (!jObj)
      return def;

   return json_boolean_value(jObj);
}

int getIntByPath(json_t* jData, const char* aPath, int def)
{
   json_t* jObj = getObjectByPath(jData, aPath);

   if (!jObj || !json_is_integer(jObj))
      return def;

   return json_integer_value(jObj);
}

double getDoubleByPath(json_t* jData, const char* aPath, double def)
{
   json_t* jObj = getObjectByPath(jData, aPath);

   if (!jObj)
      return def;

   if (json_is_integer(jObj))
      return json_integer_value(jObj);

   return json_real_value(jObj);
}

const char* getStringByPath(json_t* jData, const char* aPath, const char* def)
{
   json_t* jObj = getObjectByPath(jData, aPath);

   if (!jObj || !json_is_string(jObj))
      return def;

   return json_string_value(jObj);
}

int jStringValid(const char* s)
{
   json_t* obj = json_string(s);

   if (!obj)
      return no;

   json_decref(obj);

   return yes;
}

//***************************************************************************
// Add Element
//***************************************************************************

int addToJson(json_t* obj, const char* name, const char* value, const char* def)
{
   json_t* j = json_string(value ? value : def);

   if (!j)
      j = json_string("");

   return json_object_set_new(obj, name, j);
}

int addToJson(json_t* obj, const char* name, long value)
{
   return json_object_set_new(obj, name, json_integer(value));
}

int addToJson(json_t* obj, const char* name, int value)
{
   return json_object_set_new(obj, name, json_integer(value));
}

int addToJson(json_t* obj, const char* name, double value)
{
   return json_object_set_new(obj, name, json_real(value));
}

int addToJson(json_t* obj, const char* name, json_t* o)
{
   return json_object_set_new(obj, name, o);
}

#endif
