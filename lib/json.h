/*
 * json.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

//***************************************************************************
// Include
//***************************************************************************

#include <jansson.h>

#ifdef USEDB
#  include "db.h"
#endif

#include "common.h"

//***************************************************************************
// JSON Helper Functions
//***************************************************************************

int json2Data(json_t* obj, MemoryStruct* data, const char* encoding = 0);
json_t* jsonLoad(const char* data);

#ifdef USEDB
int addFieldToJson(json_t* obj, cDbTable* table, const char* fname, int ignoreEmpty = yes, const char* extName = 0);
int addFieldToJson(json_t* obj, cDbValue* value, int ignoreEmpty = yes, const char* extName = 0);
int getFieldFromJson(json_t* obj, cDbRow* row, const char* fname, const char* extName = 0);
#endif

int jStringValid(const char* s);

bool isElementSet(json_t* obj, const char* name);
const char* getStringFromJson(json_t* obj, const char* name, const char* def = nullptr);
int getIntFromJson(json_t* obj, const char* name, int def = na);
int getBoolFromJson(json_t* obj, const char* name, bool def = false);
long getLongFromJson(json_t* obj, const char* name, long def = na);
double getDoubleFromJson(json_t* obj, const char* name, double def = na);
json_t* getObjectFromJson(json_t* obj, const char* name, json_t* def = nullptr);

json_t* getObjectByPath(json_t* jData, const char* aPath, json_t* def = nullptr);
bool getBoolByPath(json_t* jData, const char* aPath, bool def = false);
int getIntByPath(json_t* jData, const char* aPath, int def = na);
double getDoubleByPath(json_t* jData, const char* aPath, double def = na);
const char* getStringByPath(json_t* jData, const char* aPath, const char* def = nullptr);

int addToJson(json_t* obj, const char* name, const char* value, const char* def = "");
int addToJson(json_t* obj, const char* name, long value);
int addToJson(json_t* obj, const char* name, double value);
int addToJson(json_t* obj, const char* name, json_t* o);
