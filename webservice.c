//***************************************************************************
// Web Service
// File webservice.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 28.12.2021 - Jörg Wendel
//***************************************************************************

#include <strings.h>

#include "webservice.h"

//***************************************************************************
// Web Service
//***************************************************************************

const char* cWebService::events[] =
{
   "unknown",
   "login",
   "logout",
   "pagechange",
   "data",
   "init",
   "toggleio",
   "toggleionext",
   "togglemode",
   "storeconfig",
   "gettoken",
   "setup",
   "storeiosetup",
   "chartdata",
   "logmessage",

   "userdetails",
   "storeuserconfig",
   "changepasswd",

   "command",
   "groups",
   "groupconfig",
   "chartbookmarks",
   "storechartbookmarks",
   "syslog",
   "system",
   "forcerefresh",
   "storedashboards",
   "alerts",
   "storealerts",
   "imageconfig",
   "schema",
   "storeschema",

   "errors",
   "menu",
   "pareditrequest",
   "parstore",
   "pellets",

   0
};

const char* cWebService::toName(Event event)
{
   if (event >= evUnknown && event < evCount)
      return events[event];

   return events[evUnknown];
}

cWebService::Event cWebService::toEvent(const char* name)
{
   if (!name)
      return evUnknown;

   for (int e = evUnknown; e < evCount; e++)
      if (strcasecmp(name, events[e]) == 0)
         return (Event)e;

   return evUnknown;
}
