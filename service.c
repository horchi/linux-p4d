//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File service.h
// Date 04.11.10 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

#include <service.h>

//***************************************************************************
// Working States
//***************************************************************************

FroelingService::WorkingState FroelingService::workingStates[] =
{
   {  0,  "Störung"        }, 
   {  1,  "Brenner aus"    }, 
   {  2,  "Anheizen"       },
   {  3,  "Heizen"         },
   {  4,  "Feuerhaltung"   },
   {  5,  "Feuer aus"      },
   {  6,  "Tür offen"      },
   {  7,  "Vorbereitung"   },
   {  8,  "Vorwärmphase"   },
   {  9,  "Zünden"         },
   { 10,  "Abstellen Warten" },
   { 11,  "Abstellen Warten 1"  },
   { 12,  "Abstellen Einsch."   },
   { 13,  "Abstellen Warten 2"  },
   { 14,  "Abstellen Einsch. 2"  },
   { 15,  "Abreinigen"     },

   { 19,  "Betriebsbereit" },

   { 35,  "Zünden warten"  },
   { 56,  "Vorbelüften"    },

   { na,  "" }
};

//***************************************************************************
// To Title
//***************************************************************************

const char* FroelingService::toTitle(int code)
{
   for (int i = 0; workingStates[i].code != na; i++)
      if (workingStates[i].code == code)
         return workingStates[i].title;

   return "unknown";
}
