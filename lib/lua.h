/*
 * lua.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

extern "C" {
  #include <lua5.3/lua.h>
  #include <lua5.3/lualib.h>
  #include <lua5.3/lauxlib.h>
}

#include "common.h"

//**************************************************************************
// Lua
//**************************************************************************

class Lua
{
   public:

      Lua()             { init(); }
      virtual ~Lua()    { exit(); }

      virtual int init();
      virtual int exit();

      enum Type
      {
         tBoolean,
         tDouble,
         tInteger,
         tString,
         tNil
      };

      struct Result
      {
         Type type;
         bool bValue {false};
         int iValue {0};
         double dValue {0.0};
         std::string sValue;
         bool isNil {false};
      };

      int executeFunction(const char* function, const std::vector<std::string>& arguments, Result& res);
      int executeExpression(const char* expression, const std::vector<std::string>& arguments, Result& res);

   protected:

      lua_State* handle {nullptr};
};
