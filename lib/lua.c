/*
 * lua.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "lua.h"

//**************************************************************************
// Init / Exit
//**************************************************************************

int Lua::init()
{
   handle = luaL_newstate();
   luaL_openlibs(handle);

/*   const char* file = "foobar.lua";  // #TODO

   tell(eloDebug, "Debug: Loading LUA file '%s'", file);

   if (luaL_loadfile(handle, file) != LUA_OK)
   {
      tell(eloAlways, "Error: '%s'\n", lua_tostring(handle, lua_gettop(handle)));
      lua_pop(handle, lua_gettop(handle));
      return fail;
      } */

   return success;
}

int Lua::exit()
{
   if (handle)
      lua_close(handle);
   handle = nullptr;
   return success;
}

//**************************************************************************
// Execute Script
//**************************************************************************

int Lua::executeFunction(const char* function, const std::vector<std::string>& arguments, Result& res)
{
   res = {};

   if (lua_pcall(handle, 0, 1, 0) == LUA_OK)
      lua_pop(handle, lua_gettop(handle));         //  executed successfuly, remove code from stack

   lua_getglobal(handle, function);

   if (!lua_isfunction(handle, -1))
   {
      tell(eloAlways, "Error: Function '%s' not found", function);
      lua_pop(handle, lua_gettop(handle));
      return fail;
   }

   for (const auto& arg : arguments)
   {
      tell(eloLua, "LUA: Add parameter '%s'", arg.c_str());
      lua_pushstring(handle, arg.c_str());
   }

   if (lua_pcall(handle, arguments.size(), 1, 0) == LUA_OK)
   {
      if (lua_isboolean(handle, -1))
      {
         res.type = tBoolean;
         res.bValue = lua_toboolean(handle, -1);
      }
      else if (lua_isinteger(handle, -1))
      {
         res.type = tInteger;
         res.iValue = lua_tointeger(handle, -1);
      }
      else if (lua_isnumber(handle, -1))
      {
         res.type = tDouble;
         res.dValue = lua_tonumber(handle, -1);
      }
      else if (lua_isstring(handle, -1))
      {
         res.type = tString;
         res.sValue = lua_tostring(handle, -1);
      }
      else
      {
         res.type = tNil;
         res.isNil = true;
      }

      lua_pop(handle, 1);                      // pop return value from stack
      lua_pop(handle, lua_gettop(handle));     // pop function from stack
   }
   else
   {
      tell(eloAlways, "Error: '%s'\n", lua_tostring(handle, lua_gettop(handle)));
      lua_pop(handle, lua_gettop(handle));
      return fail;
   }

   return success;
}


//**************************************************************************
// Execute LUA String
//**************************************************************************

int Lua::executeExpression(const char* expression, const std::vector<std::string>& arguments, Result& res)
{
   res = {};

   char* code {};

   asprintf(&code, "function execute()"          \
            "  %s\n"                             \
            "end", expression);

   tell(eloLua, "LUA: Calling '%s'", code);

   if (luaL_loadstring(handle, code) == LUA_OK)
   {
      free(code);

      if (lua_pcall(handle, 0, 1, 0) == LUA_OK)
         lua_pop(handle, lua_gettop(handle));         //  executed successfuly, remove code from stack

      lua_getglobal(handle, "execute");

      if (!lua_isfunction(handle, -1))
      {
         tell(eloAlways, "Error: '%s'", lua_tostring(handle, lua_gettop(handle)));
         lua_pop(handle, lua_gettop(handle));
         return fail;
      }

      if (lua_pcall(handle, 0, 1, 0) == LUA_OK)
      {
         if (lua_isboolean(handle, -1))
         {
            res.type = tBoolean;
            res.bValue = lua_toboolean(handle, -1);
         }
         else if (lua_isinteger(handle, -1))
         {
            res.type = tInteger;
            res.iValue = lua_tointeger(handle, -1);
         }
         else if (lua_isnumber(handle, -1))
         {
            res.type = tDouble;
            res.dValue = lua_tonumber(handle, -1);
         }
         else if (lua_isstring(handle, -1))
         {
            res.type = tString;
            res.sValue = lua_tostring(handle, -1);
         }
         else
         {
            res.type = tNil;
            res.isNil = true;
         }

         lua_pop(handle, 1);                      // pop return value from stack
         lua_pop(handle, lua_gettop(handle));     // pop unction from stack
      }
      else
      {
         tell(eloAlways, "Error: '%s'", lua_tostring(handle, lua_gettop(handle)));
         lua_pop(handle, lua_gettop(handle));
         return fail;
      }
   }
   else
   {
      free(code);

      tell(eloAlways, "Error: '%s'", lua_tostring(handle, lua_gettop(handle)));
      lua_pop(handle, lua_gettop(handle));
      return fail;
   }

   return success;
}
