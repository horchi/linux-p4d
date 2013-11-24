//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File service.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
// Date 04.11.2010 - 19.11.2013  Jörg Wendel
//***************************************************************************

#include "service.h"

//***************************************************************************
// Working States
//***************************************************************************

FroelingService::StateInfo FroelingService::stateInfos[] =
{
   {  wsError,  "Störung"        }, 
   {  1,        "Brenner aus"    }, 
   {  2,        "Anheizen"       },
   {  3,        "Heizen"         },
   {  4,        "Feuerhaltung"   },
   {  5,        "Feuer aus"      },
   {  6,        "Tür offen"      },
   {  7,        "Vorbereitung"   },
   {  8,        "Vorwärmphase"   },
   {  9,        "Zünden"         },
   { 10,        "Abstellen Warten" },
   { 11,        "Abstellen Warten 1"  },
   { 12,        "Abstellen Einsch."   },
   { 13,        "Abstellen Warten 2"  },
   { 14,        "Abstellen Einsch. 2"  },
   { 15,        "Abreinigen"     },

   { 19,        "Betriebsbereit" },

   { 35,        "Zünden warten"  },
   { 56,        "Vorbelüften"    },

   { wsUnknown, "" }
};

//***************************************************************************
// To Title
//***************************************************************************

const char* FroelingService::toTitle(int code)
{
   for (int i = 0; stateInfos[i].code != na; i++)
      if (stateInfos[i].code == code)
         return stateInfos[i].title;

   return "unknown";
}
