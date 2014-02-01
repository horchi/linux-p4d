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
   { 16,        "Std warten" },
   { 17,        "Saugheizen" },
   { 18,        "Fehlzuendung" },
   { 19,        "Betriebsbereit" },
   { 20,        "Rost schliessen" },
   { 21,        "Stokerleeren" },
   { 22,        "Vorheizen" },
   { 23,        "Saugen" },
   { 32,        "Geblaesenachlauf 2" },
   { 33,        "Abgestellt" },
   { 34,        "Nachzuenden" },
   { 35,        "Zünden warten"  },

   { 36,        "Fehlerbehebung" },
   { 37,        "Fehlerbehebung 1" },
   { 38,        "Fehlerbehebung 2" },
   { 39,        "Fehlerbehebung 3" },
   { 40,        "Abstellen RSE" },
   { 41,        "Störung STB" },
   { 42,        "Stöerung Kipprost" },
   { 43,        "Stöerung RGTW" },
   { 44,        "Stöerung Tür" },
   { 45,        "Stöerung Saugzug" },
   { 46,        "StöERung HYDR" },
   { 47,        "Fehler STB" },
   { 48,        "Fehler Kipprost" },
   { 49,        "Fehler RGTW" },
   { 50,        "Fehler Tür" },
   { 51,        "Fehler Saugzug" },
   { 52,        "Fehler Hydraulik" },
   { 53,        "Fehler Stoker" },
   { 54,        "Störung Stoker" },
   { 55,        "Fehlerbehebung 4" },
   { 56,        "Vorbelüften" },
   { 57,        "Störung Modul" },
   { 58,        "Fehler Modul" },
   { 59,        "SHNB Tür offen" },
   { 60,        "SHNB ANHEIZEN" },
   { 61,        "SHNB HEIZEN" },
   { 62,        "SHNB STB Notaus" },
   { 63,        "SHNB Fehler Allgemein" },
   { 64,        "SHNB Feuer Aus" },
   { 65,        "Selbsttest" },
   { 66,        "Fehlerb P2" },
   { 67,        "Fehler Fallschluft" },
   { 68,        "Störung Fallschluft" },
   { 69,        "Reinigen TMC" },
   { 70,        "Onlinereinigen" },
   
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
