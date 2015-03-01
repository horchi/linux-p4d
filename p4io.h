//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File p4io.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 19.11.2013  Jörg Wendel
//***************************************************************************

#ifndef _IO_P4_H_
#define _IO_P4_H_

#include <arpa/inet.h>

#include <time.h>
#include <string.h>
#include <stdio.h>

#include <vector>

#include "lib/serial.h"

#include "service.h"

//***************************************************************************
// Class P4 Packet
//***************************************************************************

class P4Packet : public FroelingService, public Serial
{
   public:

      // declarations

      enum Error
      {
         wrnEndOfPacket = -100
      };

      // object

      P4Packet();

		int read();
      int set(const char* data);
      int evaluate();
      std::vector<Parameter>* getParameters() { return &parameters; }

      Parameter* getParameter(int index)
      { 
         std::vector<Parameter>::iterator it;

         for (it = parameters.begin(); it != parameters.end(); it++)
            if ((*it).index == index)
               return &(*it);

         return 0; 
      }

      const char* all()                        { return buffer; }

   protected:

      // functions

      int readPacket(const char*& p);

      int getItem(Parameter* p);
      int getToken(char* token);

      // data

      char buffer[sizeMaxPacket+TB];
      const char* pBuf;
      std::vector<Parameter> parameters;
      char rbuffer[sizeMaxPacket+TB];
};

//***************************************************************************
// Request
//***************************************************************************

class P4Request : public FroelingService
{
   public:
      
      P4Request(Serial* aSerial)    { s = aSerial; text = 0; clear(); }
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
         text = 0;
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

      int request(byte command)
      {
         header.id = htons(commId);
         header.command = command;

         prepareRequest();

         show("-> ");

         if (!s || !s->isOpen())
            return fail;

         return s->write(buffer, sizeBufferContent);
      }

      void show(const char* prefix = "", int elo = eloDebug)
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

      Header* getHeader() { return &header; }

      int readHeader(int tms = 2000)
      {
         int status;

         clear();

         if (!s || !s->isOpen())
         {
            tell(eloAlways, "Line not open, aborting read");
            return fail;
         }

         if ((status = readWord(header.id, no, tms)) != success)
         {
            tell(eloAlways, "Read word failed, aborting");
            return status;
         }

         if (header.id != commId)
         {
            tell(eloAlways, "Got wrong communication id %4.4x "
                 "expected %4.4x", header.id, commId);
            return fail;
         }

         if ((status = readWord(header.size, yes, tms)) != success)
         {
            tell(eloAlways, "Read size failed, status was %d", status);
            return status;
         }

         if ((status = readByte(header.command, yes, tms)) != success)
         {
            tell(eloAlways, "Read command failed, status was %d", status);
            return status;
         }

         return success;
      }

      // interface

      int getStatus(Status* s);
      int syncTime(int offset = 0);

      int getParameter(ConfigParameter* p);
      int setParameter(ConfigParameter* p);

      int getValue(Value* v);
      int getDigitalOut(IoValue* v);
      int getDigitalIn(IoValue* v);
      int getAnalogOut(IoValue* v);

      int getFirstError(ErrorInfo* e)      { return getError(e, yes); }
      int getNextError(ErrorInfo* e)       { return getError(e, no); }
      int getFirstValueSpec(ValueSpec* v)  { return getValueSpec(v, yes); }
      int getNextValueSpec(ValueSpec* v)   { return getValueSpec(v, no); }

      int getFirstMenuItem(MenuItem* m)    { return getMenuItem(m, yes); }
      int getNextMenuItem(MenuItem* m)     { return getMenuItem(m, no); }

      int getUser(byte cmd);
      int getItem(int first);

      int check();

   protected:

      int prepareRequest();
      int getError(ErrorInfo* e, int first);
      int getValueSpec(ValueSpec* v, int first);
      int getMenuItem(MenuItem* m, int first);

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
      char* text;
      word addresses[maxAddresses];
      int addressCount;
      byte bytes[maxBytes];
      int byteCount;

      byte buffer[sizeMaxRequest*2+TB];
      int sizeBufferContent;

      byte decoded[sizeMaxRequest*2+TB];  // for debug
      int sizeDecodedContent;
      
      Serial* s;
};

//***************************************************************************
#endif // _IO_P4_H_
