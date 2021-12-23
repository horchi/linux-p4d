//***************************************************************************
// p4d / Linux - Heizungs Manager
// File p4io.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 04.09.2020  Jörg Wendel
//***************************************************************************

#pragma once

#include <arpa/inet.h>

#include <time.h>
#include <string.h>
#include <stdio.h>

#include <vector>

#include "lib/serial.h"

#include "service.h"

//***************************************************************************
// Request
//***************************************************************************

class P4Request : public FroelingService
{
   public:

      P4Request(Serial* aSerial)    { s = aSerial; clear(); }
      virtual ~P4Request()          { clear(); }

      class RequestClean
      {
         public:

            RequestClean(P4Request* aReq)
            {
               req = aReq;
            }

            ~RequestClean()
            {
               int count = 0;
               byte b;

               while (req->readByte(b, yes, 10) == success)
                  count++;

               if (count)
               {
                  tell(eloAlways, "Got %d unexpected bytes", count);
                  req->show("<- ");
               }
            }

         private:

            P4Request* req;
      };

      int clear()
      {
         free(text);
         text = nullptr;
         sizeBufferContent = 0;
         sizeDecodedContent = 0;
         memset(&header, 0, sizeof(Header));
         memset(buffer, 0, sizeof(buffer));
         memset(decoded, 0, sizeof(buffer));
         clearBytes();
         clearAddresses();
         return done;
      }

      void clearAddresses()
      {
         memset(addresses, 0, sizeof(addresses));
         addressCount = 0;
      }

      int addAddress(word address)
      {
         if (addressCount >= maxAddresses)
            return fail;

         addresses[addressCount++] = htons(address);
         return success;
      }

      void clearBytes()
      {
         memset(bytes, 0, sizeof(bytes));
         byteCount = 0;
      }

      int addByte(byte b)
      {
         if (byteCount >= maxBytes)
            return fail;

         bytes[byteCount++] = b;
         return success;
      }

      int addText(const char* t)
      {
         free(text);
         text = strdup(t);
         return success;
      }

      void show(const char* prefix = "", Eloquence elo = eloDebug2)
      {
         char tmp[1000];
         *tmp = 0;

         sprintf(tmp+strlen(tmp), "%s", prefix);

         for (int i = 0; i < sizeBufferContent; i++)
            sprintf(tmp+strlen(tmp), "%2.2X ", buffer[i]);

         sprintf(tmp+strlen(tmp), "   ");

         for (int i = 0; i < sizeBufferContent; i++)
            sprintf(tmp+strlen(tmp), "%c", isprint(buffer[i]) ? buffer[i] : '.');

         tell(elo, "%s", tmp);
      }

      void showDecoded(const char* prefix = "")
      {
         char tmp[1000];
         *tmp = 0;

         sprintf(tmp+strlen(tmp), "%s", prefix);

         for (int i = 0; i < sizeDecodedContent; i++)
            sprintf(tmp+strlen(tmp), "%2.2X ", decoded[i]);

         sprintf(tmp+strlen(tmp), "   ");

         for (int i = 0; i < sizeDecodedContent; i++)
            sprintf(tmp+strlen(tmp), "%c", isprint(decoded[i]) ? decoded[i] : '.');

         tell(eloDebug2, "%s", tmp);
      }

      int request(byte command);
      int readHeader(int tms = 2000);
      Header* getHeader() { return &header; }

      // interface

      int getStatus(Status* s);
      int syncTime(int offset = 0);

      int getParameter(ConfigParameter* p);
      int setParameter(ConfigParameter* p);


      int getFirstTimeRanges(TimeRanges* t)  { return getTimeRanges(t, yes); }
      int getNextTimeRanges(TimeRanges* t)   { return getTimeRanges(t, no); }
      int setTimeRanges(TimeRanges* t);

      int getValue(Value* v);
      int getDigitalOut(IoValue* v);
      int getDigitalIn(IoValue* v);
      int getAnalogOut(IoValue* v);

      int getFirstError(ErrorInfo* e)        { return getError(e, yes); }
      int getNextError(ErrorInfo* e)         { return getError(e, no); }
      int getFirstValueSpec(ValueSpec* v)    { return getValueSpec(v, yes); }
      int getNextValueSpec(ValueSpec* v)     { return getValueSpec(v, no); }

      int getFirstMenuItem(MenuItem* m)      { return getMenuItem(m, yes); }
      int getNextMenuItem(MenuItem* m)       { return getMenuItem(m, no); }

      int getUser(byte cmd);
      int getItem(int first);

      int check();

   protected:

      int prepareRequest();
      int getError(ErrorInfo* e, int first);
      int getValueSpec(ValueSpec* v, int first);
      int getMenuItem(MenuItem* m, int first);
      int getTimeRanges(TimeRanges* t, int first);

      int readByte(byte& v, int decode = yes, int tms = 1000);
      int readWord(word& v, int decode = yes, int tms = 1000);
      int readWord(sword& v, int decode = yes, int tms = 1000);
      int readTime(time_t& t);         // 3 byte
      int readDate(time_t& t);         // 3 byte
      int readDateExt(time_t& t);      // 4 byte
      int readTimeDate(time_t& t);     // 6 byte
      int readTimeDateExt(time_t& t);  // 7 byte
      int readText(char*& s, int size);

      // data

      Header header;
      cMyMutex mutex;
      char* text {nullptr};
      word addresses[maxAddresses];
      int addressCount;
      byte bytes[maxBytes];
      int byteCount;

      byte buffer[sizeMaxRequest*2+TB];
      int sizeBufferContent;

      byte decoded[sizeMaxRequest*2+TB];  // for debug
      int sizeDecodedContent;

      Serial* s {nullptr};
};
