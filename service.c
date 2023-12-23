//***************************************************************************
// p4d / Linux - Heizungs Manager
// File service.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 19.11.2013  Jörg Wendel
//***************************************************************************

#include "service.h"

//***************************************************************************
// Const
//***************************************************************************

const char* FroelingService::nameChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789äöüÄÖÜß_-";

//***************************************************************************
// Working States
//***************************************************************************

FroelingService::StateInfo FroelingService::stateInfos[] =
{
   {  0,  { "Störung" } },
   {  1,  { "Brenner aus", "Kessel aus" } },
   {  2,  { "Anheizen"     } },
   {  3,  { "Heizen"       } },
   {  4,  { "Feuerhaltung", "Feuererhaltung" } },
   {  5,  { "Feuer aus"    } },
   {  6,  { "Tür offen"    } },
   {  7,  { "Vorbereitung" } },
   {  8,  { "Vorwärmphase", "Vorwärmen" } },
   {  9,  { "Zünden"       } },
   { 10,  { "Abstellen Warten"    } },
   { 11,  { "Abstellen Warten 1", "Abstellen Warten1" } },
   { 12,  { "Abstellen Einsch."   } },
   { 13,  { "Abstellen Warten 2", "Abstellen Warten2" } },
   { 14,  { "Abstellen Einsch. 2" } },
   { 15,  { "Abreinigen" } },
   { 16,  { "Std warten" } },
   { 17,  { "Saugheizen" } },
   { 18,  { "Fehlzündung" } },
   { 19,  { "Betriebsbereit" } },
   { 20,  { "Rost schliessen" } },
   { 21,  { "Stokerleeren" } },
   { 22,  { "Vorheizen" } },
   { 23,  { "Saugen" } },
   { 32,  { "Gebläsenachlauf 2" } },
   { 33,  { "Abgestellt" } },
   { 34,  { "Nachzünden" } },
   { 35,  { "Zünden warten"  } },
   { 36,  { "Fehlerbehebung" } },
   { 37,  { "Fehlerbehebung 1" } },
   { 38,  { "Fehlerbehebung 2" } },
   { 39,  { "Fehlerbehebung 3" } },
   { 40,  { "Abstellen RSE" } },
   { 41,  { "Störung STB" } },
   { 42,  { "Störung Kipprost" } },
   { 43,  { "Störung RGTW" } },
   { 44,  { "Störung Tür" } },
   { 45,  { "Störung Saugzug" } },
   { 46,  { "Störung HYDR" } },
   { 47,  { "Fehler STB" } },
   { 48,  { "Fehler Kipprost" } },
   { 49,  { "Fehler RGTW" } },
   { 50,  { "Fehler Tür" } },
   { 51,  { "Fehler Saugzug" } },
   { 52,  { "Fehler Hydraulik" } },
   { 53,  { "Fehler Stoker" } },
   { 54,  { "Störung Stoker" } },
   { 55,  { "Fehlerbehebung 4" } },
   { 56,  { "Vorbelüften" } },
   { 57,  { "Störung Modul" } },
   { 58,  { "Fehler Modul" } },
   { 59,  { "SHNB Tür offen" } },
   { 60,  { "SHNB ANHEIZEN" } },
   { 61,  { "SHNB HEIZEN" } },
   { 62,  { "SHNB STB Notaus" } },
   { 63,  { "SHNB Fehler Allgemein" } },
   { 64,  { "SHNB Feuer Aus" } },
   { 65,  { "Selbsttest" } },
   { 66,  { "Fehlerb P2" } },
   { 67,  { "Fehler Fallschluft" } },
   { 68,  { "Störung Fallschluft" } },
   { 69,  { "Reinigen TMC" } },
   { 70,  { "Onlinereinigen" } },
   { 71,  { "SH Anheizen" } },
   { 72,  { "SH Heizen" } },

   { na,  { "" } }
};

//***************************************************************************
// To Title
//***************************************************************************

const char* FroelingService::toTitle(int code)
{
   for (int i = 0; stateInfos[i].code != na; i++)
      if (stateInfos[i].code == code)
         return stateInfos[i].titles[0];

   return "unknown";
}

int FroelingService::toState(const char* title)
{
   for (uint i = 0; stateInfos[i].code != na; i++)
   {
      for (const auto& t : stateInfos[i].titles)
      {
         if (strcmp(t, title) == 0)
            return stateInfos[i].code;
      }
   }

   return na;
}

int FroelingService::isError(int code)
{
   const char* title = toTitle(code);

   if (!isEmpty(title))
   {
      if (strcasecmp(title, "Fehler") == 0 || strcasecmp(title, "Störung") == 0)
         return yes;
   }

   return no;
}
