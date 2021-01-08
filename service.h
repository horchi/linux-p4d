//***************************************************************************
// p4d / Linux - Heizungs Manager
// File service.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.10 - 27.02.15  Jörg Wendel
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

      enum LogicalOperator
      {
         loAnd,
         loOr,
         loAndNot,
         loOrNot
      };

      struct StateInfo
      {
         int code;
         const char* title;
      };

      // statics

      static StateInfo stateInfos[];
      static const char* toTitle(int code);
      static int isError(int code);

      // -------------------------
      // COM1 (service interface)

      enum Error
      {
         wrnNonUpdate   = -1000,
         wrnOutOfRange,           // -999
         wrnLast,                 // -998
         wrnEmpty,                // -997
         wrnTimeout,              // -996
         wrnSkip,                 // -995

         errRequestFailed,        // -994
         errWrongAddress,         // -993
         errTransmissionFailed    // -992
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

      // menu struct types (parameter types)

      enum MenuStructType
      {
         // getValue:

         mstMesswert    = 0x03,  //  3 value
         mstMesswert1   = 0x46,  // 70 value

         // setParameter/getParameter:

         mstPar         = 0x07,  //  7 number parameter
         mstParDig      = 0x08,  //  8 boolean parameter
         mstParSet      = 0x32,  // 50
         mstParSet1     = 0x39,  // 57
         mstParSet2     = 0x40,  // 64
         mstParWeekday  = 0x0b,  // 11 -> TODO: check what means value 8
         mstParZeit     = 0x0a,  // 10 hh:mm -> TODO: check in service menu if it works

         mstWorkMode    = 0x2f,  // 47  -> TODO can set? check

         // getDigitalIn/getDigitalOut/getAnalogOut:

         mstDigOut      = 0x11,  // 17
         mstAnlOut      = 0x12,  // 18
         mstDigIn       = 0x13,  // 19

         // ... ??

         mstEmpty       = 0x22,  // 34
         mstReset       = 0x23,  // 35
         mstZeiten      = 0x26,  // 38
         mstAnzeigen    = 0x3a,  // 58
         mstFirmware    = 0x16,  // 22
         mstBusValues   = 0x31,  // 49
         mstFuelKind    = 0x3c,  // 60

         mstUnknown0    = 0x09,  //  9
         mstCurrentDate = 0x0c,  // 12
         mstCurrentTime = 0x0d,  // 13
         mstUnknown3    = 0x0f,  // 15
         mstMenuLevel   = 0x10,  // 16
         mstErrors      = 0x14,  // 20
         mstSetDefaults = 0x15,  // 21
         mstRost        = 0x18,  // 24
         mstUnknown8    = 0x19,  // 25
         mstLanguage    = 0x1d,  // 29
         mstErrorMem    = 0x1e,  // 30
         mstErrReset    = 0x1f,  // 31
         mstUnknown12   = 0x2c,  // 44
         mstXxxExist    = 0x38,  // 56
         mstSaugzug     = 0x3b,  // 59
         mstIgnitonStart= 0x3d,  // 61
         mstUnknown17   = 0x3e,  // 62
         mstDisplay     = 0x41,  // 65
         mstKesselChoice= 0x43,  // 67
         mstKesselType  = 0x44,  // 68
         mstWarningMsg  = 0x45,  // 69

         // menu groups

         mstMenuMain        = 0x01,  //  1
         mstMenuSysChoice   = 0x0e,  // 14
         mstMenu1           = 0x21,  // 33
         mstMenuDaySel      = 0x27,  // 39
         mstMenuRoot        = 0x28,  // 40
         mstMenuChoice      = 0x2a,  // 42
         mstMenu2           = 0x2d,  // 45
         mstMenu3           = 0x2e,  // 46
         mstMenuDisplay     = 0x30,  // 48
         mstGroup9          = 0x35,  // 53
         mstGroup10         = 0x37,  // 55
         mstMenuControlUnit = 0x3f   // 63
      };

      static bool isGroup(int type)
      { return
            type == mstMenuMain        ||
            type == mstMenuSysChoice   ||
            type == mstMenu1           ||
            type == mstMenuDaySel      ||
            type == mstMenuRoot        ||
            type == mstMenuChoice      ||
            type == mstMenu2           ||
            type == mstMenu3           ||
            type == mstMenuDisplay     ||
            type == mstGroup9          ||
            type == mstGroup10         ||
            type == mstMenuControlUnit; }

      // error states

      static const char* errState2Text(int state)
      {
         switch (state)
         {
            case 1:  return "gekommen";
            case 4:  return "gegangen";
            case 2:  return "quittiert";
            default: return "unknown";
         }
      }

      struct ConfigParameter
      {
         friend class P4Request;

         public:

            ConfigParameter(word addr = addrUnknown)  { unit = 0; address = addr; }
            ~ConfigParameter() { free(unit); }

            int getFactor() { return factor; }
            void setUnit(const char* u) { free(unit); unit = strdup(u); }

            void show()
            {
               tell(eloAlways,
                    " Address: 0x%4.4x\n"       \
                    " Unit: %s\n"               \
                    " Digits: %d\n"             \
                    escBlue " Value: %.*f (%d)\n" escReset   \
                    " Min: %.*f (%d)\n"              \
                    " Max: %.*f (%d)\n"              \
                    " Default: %.*f (%d)\n"          \
                    " Factor: %d\n",
                    address, unit, digits, digits, rValue, value, digits, rMin, min, digits, rMax, max, digits, rDefault, def, getFactor());
               tell(eloInfo,
                    " UB1 %d \n"                \
                    " UB2 %d \n"                \
                    " UB3 %d \n"                \
                    " UW1 %d \n",               \
                    ub1, ub2, ub3, uw1);
            }

            cRetBuf toNice(byte type)
            {
               char nice[100+TB];

               if (type == mstParDig)
                  sprintf(nice, "%s", value ? "ja" : "nein");
               else if (type == mstParZeit)
                  sprintf(nice, "%02d:%02d", value/60, value%60);
               else
                  sprintf(nice, "%.*f", digits, rValue);

               return cRetBuf(nice);
            }

            int setValueDirect(const char* strValue, int dig, int factor)
            {
               if (factor <= 0)
                  factor = 1;

               if (dig == 0)
               {
                  if (!isNum(strValue))
                     return fail;

                  rValue = atoi(strValue);
                  value = rValue * factor;
               }

               else
               {
                  if (!isFloat(strValue))
                     return fail;

                  rValue = strtod(strValue, nullptr);
                  value = rValue * factor;
               }

               return success;
            }

            int setValue(byte type, const char* strValue)
            {
               if (type == mstParDig)
               {
                  if (strcmp(strValue, "ja") != 0 && strcmp(strValue, "nein") != 0)
                     return fail;

                  value = strcmp(strValue, "ja") == 0 ? 1 : 0;
                  rValue = value;
               }
               else if (type == mstParZeit)
               {
                  char* h = strdup(strValue);
                  char* m;

                  if ((m = strchr(h, ':')))
                  {
                     *m++ = 0;

                     if (!isNum(h) || !isNum(m))
                     {
                        free(h);
                        return fail;
                     }

                     value = atoi(h)*60 + atoi(m);
                     rValue = value;
                  }
                  else
                  {
                     free(h);
                     return fail;
                  }

                  free(h);
               }
               else if (type == mstPar || type == mstParSet || type == mstParSet1  || type == mstParSet2)
               {
                  tell(0, "Set value %d with %d digits and factor %d", atoi(strValue), digits, factor);
                  if (digits == 0)
                  {
                     if (!isNum(strValue))
                        return fail;

                     rValue = atoi(strValue);
                     value = rValue * factor;
                  }

                  else
                  {
                     if (!isFloat(strValue))
                        return fail;

                     rValue = strtod(strValue, nullptr);
                     value = rValue * factor;
                  }
               }
               else
               {
                  return fail;
               }

               return success;
            }

            word address;
            char* unit;
            byte digits;

            double rValue;  // werte mit factor applied
            double rMin;
            double rMax;
            double rDefault;

            byte ub1;
            byte ub2;
            byte ub3;
            sword uw1;

         private:

            byte factor;
            sword value;
            sword min;
            sword max;
            sword def;
      };

      struct Status
      {
         Status()  { modeinfo = 0; stateinfo = 0; clear(); }
         ~Status() { clear(); }

         void clear()
         {
            free(modeinfo);  modeinfo = 0;
            free(stateinfo); stateinfo = 0;
            *version = 0;
            time = 0;
            mode = 0;
            state = 0;
         }

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
         ValueSpec()  {}
         ~ValueSpec() { free(unit); free(description); free(name); }

         word address;
         char* unit {nullptr};
         word factor;
         char* name {nullptr};
         char* description {nullptr};
         word unknown;
      };

      struct MenuItem
      {
         MenuItem()  { description = 0; unit = 0; clear(); }
         ~MenuItem() { free(unit); free(description); }

         void clear()
         {
            type = 0;
            unknown1 = 0;
            parent = 0;
            child = 0;
            address = 0;
            unknown2 = 0;
            free(description); description = 0;
            free(unit); unit = 0;
         }

         byte type;
         byte unknown1;
         word parent;
         word child;
         word address;
         word unknown2;
         char* description;
         char* unit;
      };

      struct TimeRanges
      {
         TimeRanges(byte addr = 0xff)  { address = addr; }
         ~TimeRanges() { }

         int isSet(int n)
         {
            if (n >= 4 || (timesFrom[n] == 0xff && timesTo[n] == 0xff))
               return no;

            return yes;
         }

         const char* getTimeRangeFrom(int n)
         {
            static char nice[10+TB];

            if (!isSet(n))
               return "nn:nn";

            sprintf(nice, "%02d:%02d", timesFrom[n]/10, timesFrom[n]%10*10);

            return nice;
         }

         const char* getTimeRangeTo(int n)
         {
            static char nice[10+TB];

            if (!isSet(n))
               return "nn:nn";

            sprintf(nice, "%02d:%02d", timesTo[n]/10, timesTo[n]%10*10);

            return nice;
         }

         const char* getTimeRange(int n)
         {
            static char nice[20+TB];
            sprintf(nice, "%s - %s", getTimeRangeFrom(n), getTimeRangeTo(n));
            return nice;
         }

         int setTimeRange(int n, const char* from, const char* to)
         {
            if (!strchr(from, ':') || !strchr(to, ':'))
               return fail;

            if (strcasecmp(from, "nn:nn") != 0 && strcasecmp(to, "nn:nn") != 0)
            {
               uint hF, mF, hT, mT;

               hF = atoi(from);
               mF = atoi(strchr(from, ':') + 1);
               hT = atoi(to);
               mT = atoi(strchr(to, ':') + 1);

               if (hF > 23 || hT > 23 || mF > 59 || mT > 59)
                  return fail;

               timesFrom[n] = hF*10 + mF/10;
               timesTo[n] = hT*10 + mT/10;
            }
            else
            {
               timesFrom[n] = 0xff;
               timesTo[n] = 0xff;
            }

            return success;
         }

         byte address;
         byte timesFrom[4];
         byte timesTo[4];
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
