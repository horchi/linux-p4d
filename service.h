//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File service.h
// Date 04.11.10-24.11.13 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
//***************************************************************************

#ifndef _FSERVICE_H_
#define _FSERVICE_H_

#include <stdio.h>

#include "lib/common.h"

//***************************************************************************
// Class Froeling Service
//***************************************************************************

class FroelingService
{
   public:

      static const char* nameChars;

      enum WorkingState
      {
         wsUnknown = na,

         wsError
      };

      struct StateInfo
      {
         int code;
         const char* title;
      };

      // statics

      static StateInfo stateInfos[];
      static const char* toTitle(int code);

      // -------------------------
      // COM1 (service interface)

      enum Error
      {
         wrnNonUpdate   = -1000,
         wrnOutOfRange,
         wrnLast,
         wrnEmpty,
         wrnTimeout,
         wrnSkip,

         errRequestFailed,
         errWrongAddress,
         errTransmissionFailed
      };

      enum InterfaceDef1
      {
         commId = 0x02FD,     // communication identifier
         
         sizeId = 2,
         posSize = 2,
         sizeSize = 2,
         sizeCommand = 1,
         sizeDataMax = 254,
         sizeCrc  = 1,        // CRC (over all bytes incl. complete header)
         
         sizeAddress = 2,
         sizeByte    = 1,

         sizeMaxRequest = sizeId + sizeSize + sizeCommand + sizeDataMax + sizeCrc,
         sizeMaxReply   = sizeMaxRequest,

         maxAddresses   = sizeDataMax / sizeAddress,
         maxBytes       = sizeDataMax / sizeByte,

         //

         addrUnknown = 0xFFFF
      };

#pragma pack(1)
      struct Header
      {
         word id;        // 2 bytes frame start kennung stets 0x02 0xFD
         word size;      // 2 bytes size of data packet -> payload incl. CRC without header
         byte command;   // 1 byte  command
      };
#pragma pack()

      enum Command
      {
         cmdUnknown = 0,

         cmdCheck             = 0x22,   // Verbindung zur Steuerung überprüfen

         cmdGetValue          = 0x30,   // Aktuelle Werte der Heizung
         cmdGetValueListFirst = 0x31,   // Liste der verfügbaren Aktuellen Werte
         cmdGetValueListNext  = 0x32,   //   "

         cmdGetUnknownFirst   = 0x35,   // What ever :o
         cmdGetUnknownNext    = 0x36,   // 

         cmdGetMenuListFirst  = 0x37,   // Menuestruktur auslesen
         cmdGetMenuListNext   = 0x38,   //   "
         cmdSetParameter      = 0x39,   // Einstellung ändern

         cmdGetBaseSetup      = 0x40,   // Konfiguration des Kessels
         cmdGetVersion        = 0x41,   // Software Version, aktuelle Zeit und Datum

         cmdGetTimesFirst     = 0x42,   // Zeiten abfragen
         cmdGetTimesNext      = 0x43,   // Zeiten abfragen

         cmdGetDigOut         = 0x44,   // Digitale Ausgänge abfragen
         cmdGetAnlOut         = 0x45,   // Analoge Ausgänge abfragen
         cmdGetDigIn          = 0x46,   // Digitale Eingänge abfragen

         cmdGetErrorFirst     = 0x47,   // Fehlerpuffer abfragen
         cmdGetErrorNext      = 0x48,   //   "

         cmdSetTimes          = 0x50,   // Zeiten ändern
         cmdGetState          = 0x51,   // Kesselzustand abfragen

         cmdSetDateTime       = 0x54,   // Datum und Uhrzeit einstellen
         cmdGetParameter      = 0x55,   // Einstellung abfragen

         cmdSetDigOut         = 0x58,   // Digitale Ausgänge manipulieren
         cmdSetAnlOut         = 0x59,   // Analoge Ausgänge manipulieren
         cmdSetDigIn          = 0x5A,   // Digitale Eingänge manipulieren

         cmdGetForce          = 0x5E,   // Überprüfen ob forcen aktiv
         cmdSetForce          = 0x7E    // Force ein/aus
      };

      enum UserDefined
      {
         udState = 1,
         udMode,
         udTime
      };

      // menu struct types
      
      enum MenuStructType
      {
         mstMesswert  = 0x03,
         mstMesswert1 = 0x46,

         mstPar      = 0x07,
         mstParDig   = 0x08,
         mstParSet   = 0x32,
         mstParSet1  = 0x39,
         mstParSet2  = 0x40,
         mstParZeit  = 0x0a,

         mstDigOut   = 0x11,
         mstAnlOut   = 0x12, 
         mstDigIn    = 0x13, 
         mstEmpty    = 0x22,
         mstReset    = 0x23,
         mstZeiten   = 0x26,
         mstAnzeigen = 0x3a,
         mstFirmware = 0x16
      };

      // error states

      static const char* errState2Text(int state)
      {
         switch (state)
         {
            case 1:  return "gekommen"; 
            case 2:  return "quittiert";
            case 4:  return "gegangen"; 
            default: return "unknown"; 
         }
      }

      struct ConfigParameter
      {
         public:

            ConfigParameter(word addr = addrUnknown)  { unit = 0; address = addr; }
            ~ConfigParameter() { free(unit); }
            
            static cRetBuf toNice(sword value, byte type)
            {
               char nice[100+TB];
               
               if (type == mstParDig)
                  sprintf(nice, "%s", value ? "ja" : "nein");
               else if (type == mstParZeit)
                  sprintf(nice, "%02d:%02d", value/60, value%60);
               else
                  sprintf(nice, "%d", value);

               return nice;
            }

            static int toValue(const char* nice, byte type, sword& value)
            {
               if (type == mstParDig)
               {
                  if (strcmp(nice, "ja") != 0 && strcmp(nice, "nein") != 0)
                     return fail;

                  value = strcmp(nice, "ja") == 0 ? 1 : 0;
               }
               else if (type == mstParZeit)
               {
                  char* h = strdup(nice);
                  char* m;

                  if ((m = strchr(h, ':')))
                  {
                     *m++ = 0;

                     if (!isNum(h) || !isNum(m))
                        return fail;

                     value = atoi(h)*60 + atoi(m);
                  }
                  else 
                     return fail;

                  free(h);
               }
               else if (type == mstPar || type == mstParSet || 
                        type == mstParSet1  || type == mstParSet2)
               {
                  if (!isNum(nice))
                     return fail;
                  
                  value = atoi(nice);
               }
               else
                  return fail;

               return success;
            }

            void setUnit(const char* u) { free(unit); unit = strdup(u); }
            
            word address;
            char* unit;
            byte digits;
            word factor;
            sword value;
            sword min;
            sword max;
            sword def;
      };

      struct Status
      {
         Status()  { modeinfo = 0; stateinfo = 0; clear(); }
         ~Status() { clear(); }
 
         void clear() { free(modeinfo); free(stateinfo); modeinfo = 0; stateinfo = 0; *version = 0; }
         time_t time; 
         byte mode;           // Betriebsart
         byte state;          // Zustand
         char* modeinfo;      // Betriebsart Text
         char* stateinfo;     // Zustand Text
         char version[12+TB];
      };

      struct Value
      {
         Value(word addr = addrUnknown)  { address = addr; }
         ~Value() { }

         word address;
         sword value;
      };

      struct IoValue
      {
         IoValue(word addr = addrUnknown)  { address = addr; }
         ~IoValue() { }

         word address;
         byte mode;
         byte state;
      };

      struct ValueSpec
      {
         ValueSpec()  { unit = 0; description = 0; name = 0; }
         ~ValueSpec() { free(unit); free(description); free(name); }

         word address;
         char* unit;
         word factor;
         char* name;
         char* description;
         word unknown;
      };

      struct MenuItem
      {
         MenuItem()  { unit = 0; description = 0; }
         ~MenuItem() { free(unit); free(description); }

         byte type;
         byte unknown1;
         word parent;
         word child;
         word address;
         word unknown2;
         char* description;
         char* unit;
      };

      struct ErrorInfo
      {
         ErrorInfo()  { text = 0; }
         ~ErrorInfo() { free(text); }

         byte info;     // info byte
         word number;   // error number
         byte state;    // state
         time_t time;   // time
         char* text;    // text
      };

      // -------------------------
      // -------------------------
      // COM2
      // -------------------------

      enum InterfaceDef2
      {
         seqStart = 0x24,           // $
         seqEnd   = 0x1A,           // ^Z <SUB>

         delimiter = ';',

         sizeName = 29,
         sizeUnit = 4,
         sizeText = 19,
         sizeMaxPacket = 2048
      };

      enum ParamPart
      {
         partUnknown = na,

         partName,
         partValue,
         partIndex,
         partDiv,
         partUnit,

         partCount
      };

      enum ParameterIndex
      {
         parState = 1,
         parCopperTemp = 2,
         parExhaustTemp = 3,

         parPufferOben = 20,
         parPufferUnten = 21,
         parPufferPump = 22,

         parHk1Temp = 24,
         parHk2Temp = 25,
         parHk1Pump = 26,
         parHk2Pump = 27,
         parOutsideTemp = 28,

         parError = 99
      };

      struct Parameter
      {
         int index;                     // 17
         char name[sizeName+TB];        // "Kesseltemp."
         double value;                  // 80,5
         char text[sizeText+TB];        // "Kein Fehler"
         char unit[sizeUnit+TB];        // °C"
      };
};

typedef FroelingService Fs;

//***************************************************************************

#endif //  _FSERVICE_H_
