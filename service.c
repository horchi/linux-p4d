//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File service.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 19.11.2013  Jörg Wendel
//***************************************************************************

#include "service.h"

//***************************************************************************
// Const
//***************************************************************************

const char* FroelingService::nameChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789äöüÄÖÜ_-";

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

//***************************************************************************
// Didital Output Definitions
//***************************************************************************

FroelingService::DigitalOutDef FroelingService::diditalOutDefinitions[] =
{
   { doHeizungLambdasonde, "HeizungLambdasonde" },
   { doBrennerrelais,      "Brennerrelais" },
   { doStandbyRelais,      "StandbyRelais" },

   { doHeizkreispumpe1,    "Heizkreispumpe 1" },
   { doHeizkreispumpe2,    "Heizkreispumpe 2" },
   { doHeizkreispumpe3,    "Heizkreispumpe 3" },
   { doHeizkreispumpe4,    "Heizkreispumpe 4" },
   { doHeizkreispumpe5,    "Heizkreispumpe 5" },
   { doHeizkreispumpe6,    "Heizkreispumpe 6" },
   { doHeizkreispumpe7,    "Heizkreispumpe 7" },
   { doHeizkreispumpe8,    "Heizkreispumpe 8" },

   { doMischer1auf,        "Mischer 1 auf" },
   { doMischer1zu,         "Mischer 1 zu" },       
   { doMischer2auf,        "Mischer 2 auf" },       
   { doMischer2zu,         "Mischer 2 zu" },       
   { doMischer3auf,        "Mischer 3 auf" },       
   { doMischer3zu,         "Mischer 3 zu" },       
   { doMischer4auf,        "Mischer 4 auf" },       
   { doMischer4zu,         "Mischer 4 zu" },       
   { doMischer5auf,        "Mischer 5 auf" },       
   { doMischer5zu,         "Mischer 5 zu" },       
   { doMischer6auf,        "Mischer 6 auf" },       
   { doMischer6zu,         "Mischer 6 zu" },       
   { doMischer7auf,        "Mischer 7 auf" },       
   { doMischer7zu,         "Mischer 7 zu" },       
   { doMischer8auf,        "Mischer 8 auf" },       
   { doMischer8zu,         "Mischer 8 zu" },       

   { 0, 0 }
};
