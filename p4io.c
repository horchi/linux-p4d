//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File p4io.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 19.11.2013  Jörg Wendel
//***************************************************************************

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "p4io.h"

// #define __TEST

//***************************************************************************
// Class P4 Packet
//***************************************************************************

P4Packet::P4Packet()
{
   *buffer = 0;
   pBuf = 0;
}

//***************************************************************************
// Set
//***************************************************************************

int P4Packet::set(const char* data)
{
   if (!data || strlen(data) > sizeMaxPacket)
      return fail;

   // convert charset ...

   if (toUTF8(buffer, sizeMaxPacket, data) != success)
   {
      tell(eloAlways, "Warning: Converting of charset failed");
      strcpy(buffer, data);
   }

   pBuf = buffer;

   return evaluate();
}

//***************************************************************************
// Evaluate
//***************************************************************************

int P4Packet::evaluate()
{
   Parameter p;
   int status;

   parameters.clear();

   // 

   while ((status = getItem(&p)) == success)
   {
      if (p.index < 0)
      {
         tell(eloAlways, "Warning: Unexpected parameter index (%d), ignoring",
              p.index);

         continue;
      }

      parameters.push_back(p);
   }

   return status == wrnEndOfPacket ? success : status;
}

//***************************************************************************
// Get Item
// Format Example: 'Kesseltemp.;0161;2;2;°C;'
//***************************************************************************

int P4Packet::getItem(Parameter* p)
{
   char lastToken[sizeMaxPacket+TB];

   if (!p)
      return fail;

   memset(p, sizeof(Parameter), 0);

   // get parameter parts

   for (int part = 0; part < partCount; part++)
   {
      int status;

      if ((status = getToken(lastToken)) != success)
         return status;

      // tell(eloAlways, "got token '%s'", lastToken);

      switch (part)
      {
         case partName:  sprintf(p->name, "%.*s", sizeName, lastToken);  break;
         case partIndex: p->index = atoi(lastToken);                     break;
         case partDiv:   p->value /= atoi(lastToken);                    break;
         case partUnit:  sprintf(p->unit, "%.*s", sizeUnit, lastToken);  break;

         case partValue:
            p->value = atof(lastToken);
            sprintf(p->text, "%.*s", sizeText, lastToken);

            break;
            
      }
   }

   // for parameter state we patch name and text

   if (p->index == parState)
   {
      sprintf(p->name, "%.*s", sizeName, "Status");
      sprintf(p->text, "%.*s", sizeText, toTitle((int)p->value));
   }

   // drop text for all except parameters 'state' and 'error'

   if (p->index != parState && p->index != parError)
      *p->text = 0;

   return success;
}

//***************************************************************************
// Get Value
//***************************************************************************

int P4Packet::getToken(char* token)
{
   char* t = token;

   if (isEmpty(pBuf))
      return wrnEndOfPacket;

   // skip leading blanks ..

   while (*pBuf == ' ')
      pBuf++;

   // get next token ..

   while (*pBuf && *pBuf != delimiter)
      *t++ = *pBuf++;

   *t = 0;

   if (*pBuf != delimiter)
   {
      tell(eloAlways, "Error: Missing delimiter at (%d/%d) [%s] [%s]", 
           pBuf, buffer, pBuf, buffer);
      return fail;
   }

   pBuf++;

   return success;
}

//***************************************************************************
// Read Packet
//***************************************************************************

int P4Packet::readPacket(const char*& packet)
{
   byte b = 0;
   int pos = 0;
   long startAt = time(0);
   int foundStart = no;
   int skipped = 0;
   int looks = 0;

   // read one data packet ended by 'seqEnd'

   while (pos < sizeMaxPacket && time(0) < startAt + readTimeout)
   {
      if (look(b) != success)
      {
         looks++;
         usleep(100000);         // take a breath
         continue;
      }

      if (!foundStart)
      {
         foundStart = (b == seqStart);
         
         if (!foundStart)
            skipped++;

         continue;
      }

      if (b == seqEnd)
         break;

      rbuffer[pos++] = b;
   }

   // check ..

   if (b != seqEnd)
   {
      tell(eloAlways, "Timeout of %d seconds after %d reads and %d bytes while reading packet",
           time(0)-startAt, looks, pos);

      return fail;
   }

   tell(eloDebug, "Debug: Detected eop, read (%d) bytes", pos);

   if (skipped)
      tell(eloDetail, "Info: Skipped (%d) bytes waste in front", pos);

   // terminate buffer

   rbuffer[pos] = 0;
   packet = rbuffer;

   return success;
}

//***************************************************************************
// Read
//***************************************************************************

int P4Packet::read()
{
#ifndef __TEST

   if (isOpen())
   {
      int status;
      const char* line;

      if ((status = readPacket(line)) != success)
         return status;

      tell(eloDebug, "Read line [%s]", line);

      return set(line);
   }

#else

   const char* dummy = " Betriebsbereit;0019;1;1;zst;"
      "Kesseltemp.;0089;2;2;°C;"
      "Abgastemp.;0037;3;1;°C;"
      "Abgastemp S;0047;11;1;°C;"
      "Kesselstrg ;0099;4;1;%;"
      "Primärluft ;0000;5;1;%;"
      "Rest O2 ist;0019;6;10;%;"
      "O2 Regler  ;0100;7;1;%;"
      "Sekundärluft;0000;8;1;%;"
      "Saugzug Soll;0000;9;1;%;"
      "Saugzug Ist;0000;10;1;U;"
      "Einschub Ist;0000;12;1;%;"
      "O2 ReglerPell ;0114;13;1;%;"
      "Füllstand: ;0000;14;207;%;"
      "Ansauggeschw.;0454;15;100;m/s;"
      "Strom Austrags;0606;16;1000;A;"
      "Fühler 1;0096;17;2;'C;"
      "Kesselsoll ;0144;18;2;°C;"
      "Pufferoben ;0115;20;2;°C;"
      "Pufferunten ;0060;21;2;°C;"
      "Pufferpumpe ;0000;22;1;%;"
      "Boiler 1;0000;23;2;°C;"
      "Vorlauf 1;0076;24;2;°C;"
      "Vorlauf 2;0052;25;2;°C;"
      "HK Pumpe 1;0001;26;1; ;"
      "HK Pumpe 2;0001;27;1; ;"
      "Aussentemp;0013;28;2;°C;"
      "Kollektortemp;0000;29;2;°C;"
      "Betriebsstunden;0066;30;1;h;"
      "Fehler;Kein Fehler ;99;1; ;";

   return set(dummy);

#endif

   return fail;
}


//***************************************************************************
// Class P4 Request
//***************************************************************************
//***************************************************************************
// Prepare Request
//***************************************************************************

int P4Request::prepareRequest()
{
   byte tmp[sizeMaxRequest+TB];
   int sizeNetto = 0;
   byte* p;

   sizeBufferContent = 0;
   sizeDecodedContent = 0;

   if (addressCount)
      header.size = htons((addressCount * sizeAddress) + sizeCrc);
   else if (byteCount)
      header.size = htons(byteCount + sizeCrc);
   else if (!isEmpty(text))
      header.size = htons(strlen(text) + sizeCrc);
   else
      header.size = htons(sizeCrc);

   // add header

   memcpy(tmp+sizeNetto, &header, sizeof(Header));
   sizeNetto += sizeof(Header);

   // add text, bytes or addresses, a combination is not implemented (needed?)

   if (addressCount)
   {
      // add addresses
      
      memcpy(tmp+sizeNetto, addresses, addressCount*sizeAddress);
      sizeNetto += addressCount*sizeAddress;
   }
   else if (byteCount)
   {
      // add bytes
      
      memcpy(tmp+sizeNetto, bytes, byteCount);
      sizeNetto += byteCount;
   }
   else if (!isEmpty(text))
   {
      // add text without TB
      
      memcpy(tmp+sizeNetto, text, strlen(text));
      sizeNetto += strlen(text);
   }

   // add crc

   tmp[sizeNetto] = crc(tmp, sizeNetto);
   sizeNetto++;

   // convert (mask) all bytes behind id 

   sizeBufferContent = sizeNetto;

   memcpy(buffer, &header.id, sizeId);
   p = buffer + sizeId;

   for (int i = posSize; i < sizeNetto; i++)
   {
      switch (tmp[i])
      {
         case 0x02: *p++ = tmp[i]; *p++ = 0;    sizeBufferContent++; break; // 02 -> 02 00
         case 0x2b: *p++ = tmp[i]; *p++ = 0;    sizeBufferContent++; break; // 2B -> 2B 00
         case 0xfe: *p++ = tmp[i]; *p++ = 0;    sizeBufferContent++; break; // FE -> FE 00

         case 0x11: *p++ = 0xfe;   *p++ = 0x12; sizeBufferContent++; break; // 11 -> FE 12
         case 0x13: *p++ = 0xfe;   *p++ = 0x14; sizeBufferContent++; break; // 13 -> FE 14

         default: *p++ = tmp[i];
      }
   }

   return sizeBufferContent;
}

//***************************************************************************
// Read Byte
//***************************************************************************

int P4Request::readByte(byte& v, int decode, int tms)
{
   byte b;
   byte b1;
   int status;

   if ((status = s->look(b, tms)) != success)
      return status == Serial::wrnTimeout ? (int)wrnTimeout : fail;
   
   buffer[sizeBufferContent++] = b;

   if (!decode)
   {
      v = b;
      decoded[sizeDecodedContent++] = v;

      return success;
   }
   
   // decode ...

   if (b != 0x02 && b != 0x2b && b != 0x0fe)
   {
      v = b;
   }
   else if (b == 0x02 || b == 0x2b)
   {
      if ((status = s->look(b1, tms)) != success || b1 != 0x00)
         return status == Serial::wrnTimeout ? (int)wrnTimeout : fail;

      buffer[sizeBufferContent++] = b1;
      
      v = b;
   }
   else if (b == 0x0fe)
   {
      if ((status = s->look(b1, tms)) != success)
         return status == Serial::wrnTimeout ? (int)wrnTimeout : fail;
      
      buffer[sizeBufferContent++] = b1;

      if (b1 == 0x12)
         v = 0x11;
      else if (b1 == 0x14)
         v = 0x13;
      else if (b1 == 0x00)
         v = 0x0fe;
      else
         return fail;
   }
   else
      return fail;

   decoded[sizeDecodedContent++] = v;

   return success;
}

//***************************************************************************
// Read Word
//***************************************************************************

int P4Request::readWord(word& v, int decode, int tms)
{
   byte b1;
   byte b2;
   int status;

   if ((status = readByte(b1, decode, tms)) != success)
      return status;
   
   if ((status = readByte(b2, decode, tms)) != success)
      return status;
   
   v = (b1 << 8) | b2;

   return success;
}

int P4Request::readWord(sword& v, int decode, int tms)
{
   byte b1;
   byte b2;
   int status;

   if ((status = readByte(b1, decode, tms)) != success)
      return status;
   
   if ((status = readByte(b2, decode, tms)) != success)
      return status;
   
   v = (signed short)(b1 << 8) | b2;

   return success;
}
//***************************************************************************
// Read Time
//***************************************************************************

int P4Request::readTime(time_t& t)
{
   byte s, m, h;
   
   if (readByte(s) != success)
      return fail;
   
   if (readByte(m) != success)
      return fail;

   if (readByte(h) != success)
      return fail;
   
   t = s + tmeSecondsPerMinute*m + tmeSecondsPerHour*h;

   return success;
}

//***************************************************************************
// Read Date
//***************************************************************************

int P4Request::readDate(time_t& t)
{
   byte d, m, y;
   struct tm tm;

   if (readByte(d) != success)
      return fail;
   
   if (readByte(m) != success)
      return fail;

   if (readByte(y) != success)
      return fail;

   memset(&tm, 0, sizeof(tm));

   tm.tm_mday = d;
   tm.tm_mon = m - 1;
   tm.tm_year = y + 100;

   t = mktime(&tm);

   return success;
}

//***************************************************************************
// Read Date
//***************************************************************************

int P4Request::readDateExt(time_t& t)
{
   byte d, m, y, dow;
   struct tm tm;

   if (readByte(d) != success)
      return fail;
   
   if (readByte(m) != success)
      return fail;

   if (readByte(dow) != success)  // day of week
      return fail;

   if (readByte(y) != success)
      return fail;
   
   memset(&tm, 0, sizeof(tm));
   
   tm.tm_mday = d;
   tm.tm_mon = m - 1;
   tm.tm_year = y + 100;

   t = mktime(&tm);

   return success;
}

//***************************************************************************
// Read Time-Date
//***************************************************************************

int P4Request::readTimeDate(time_t& t)
{
   time_t ti, dt;

   if (readTime(ti) != success)    // first the time !
      return fail;

   if (readDate(dt) != success)    // second the date!
      return fail;

   t = dt + ti;

   return success;
}

//***************************************************************************
// Read Time-Date Ext
//***************************************************************************

int P4Request::readTimeDateExt(time_t& t)
{
   time_t ti, dt;

   if (readTime(ti) != success)       // first the time !
      return fail;

   if (readDateExt(dt) != success)    // second the date!
      return fail;

   t = dt + ti;

   return success;
}

//***************************************************************************
// Read Text
//***************************************************************************

int P4Request::readText(char*& s, int size)
{
   byte b;
   int i;
   int p = 0;
   char* tmp = (char*)malloc(size+1);

   s = 0;
   
   for (i = 0; i < size; i++)
   {
      if (readByte(b) != success)
      {
         free(tmp);
            
         return fail;
      }
     
      if (b)
         tmp[p++] = b;
   }

   tmp[p] = 0;
   s = (char*)malloc(2*size);

   if (toUTF8(s, 2*size, tmp) != success)
   {
      tell(eloDetail, "Warning: Converting of charset failed [%s]", tmp);
      strcpy(s, tmp);
   }

   free(tmp);

   return success;
}

//***************************************************************************
// Read Text
//***************************************************************************

int P4Request::check()
{
   RequestClean clean(this);

   int status = fail;
   char* s = 0;
   byte b;

   clear();
   addText("Tescht ;-)");
   request(cmdCheck);

   if (readHeader() == success)
   {
      if (readText(s, getHeader()->size - sizeCrc) == success)
         if (readByte(b) == success)   // read crc ..
            status = success;

      free(s);

      show("<- ");
   }

   return status;
}

//***************************************************************************
// Get State
//***************************************************************************

int P4Request::getStatus(Status* s)
{
   RequestClean clean(this);

   int status = success;
   byte b;

   // cmdGetState

   clear();
   request(cmdGetState);
   s->clear();

   if ((status += readHeader()) == success)
   {
      char* text = 0;
      char* p;
      int size = getHeader()->size;
      
      status += readByte(s->mode);
      size--;
      status += readByte(s->state);
      size--;
      
      status += readText(text, size-sizeCrc);
      status += readByte(b);
 
      if ((p = strchr(text, ';')))
      {
         *p = 0;
      
         s->modeinfo = strcasecmp(text, "Übergangsbetr") != 0 ? strdup(text) : strdup("Übergangsbetrieb");
         s->stateinfo = strdup(p+1);
   
         free(text);
         show("<- ");
      }
      else
      {  
         s->stateinfo = strdup("Communication error");
         tell(eloAlways, "Communication error while reading state, got size %d, status was %d", size, status);

         status = fail;
         show("<- ", eloAlways);
      }
   }

   if (status != success)
      return status;

   // cmdGetVersion

   clear();
   request(cmdGetVersion);
   
   if ((status += readHeader()) == success)
   {
      int size = getHeader()->size;
      byte v1, v2, v3, v4;

      status += readByte(v1);
      size--;
      status += readByte(v2);
      size--;
      status += readByte(v3);
      size--;
      status += readByte(v4);
      size--;

      if (status == success)
         sprintf(s->version, "%2.2x.%2.2x.%2.2x.%2.2x", v1, v2, v3, v4);

      status += readTimeDateExt(s->time);
      size -= 7;

      while (size-- > 0)
         status += readByte(b);

      show("<- ");
   }

   return status;
}

//***************************************************************************
// Sync Time
//***************************************************************************

int P4Request::syncTime(int offset)
{
   RequestClean clean(this);

   struct tm tim = {0};
   int status = success;
   byte b;
   time_t now = time(0);

   if (offset)
   {
      now += offset;
      tell(eloAlways, "Syncing time with offset of %d seconds", offset);
   }

   clear();

   localtime_r(&now, &tim);

   addByte(tim.tm_sec+1);  // s-3200 is NOT the fastest ;-)
   addByte(tim.tm_min);
   addByte(tim.tm_hour);

   addByte(tim.tm_mday);
   addByte(tim.tm_mon + 1);
   addByte(tim.tm_wday);
   addByte(tim.tm_year - 100);

   request(cmdSetDateTime);

   if ((status += readHeader()) == success)
   {
      status += readByte(b);
      
      if (b != 0)
         status = fail;

      status += readByte(b); // crc
   }

   show("<- ");

   return status;
}

//***************************************************************************
// Get Parameter
//***************************************************************************

int P4Request::getParameter(ConfigParameter* p)
{
   RequestClean clean(this);

   int status = fail;

   if (!p || p->address == addrUnknown)
      return errWrongAddress;
   
   clear();
   addAddress(p->address);
   request(cmdGetParameter);
   
   if (readHeader() == success)
   {
      byte b;
      word w;
      
      status = readByte(b)            // what ever

         + readWord(p->address)       // address
         + readText(p->unit, 1)       // unit
         + readByte(p->digits)        // decimal digits
         + readWord(p->factor)        // factor
         + readWord(p->value)         // current value
         + readWord(p->min)           // min value
         + readWord(p->max)           // max value
         + readWord(p->def)           // default value
         
         + readWord(w)                // what ever
         + readByte(b)                // what ever
         
         + readByte(b);               // crc

      if (status == success)
      {
         if (p->factor)
            p->value /= p->factor;

         if (strcmp(p->unit, "°") == 0)
            p->setUnit("°C");
      }

      show("<- ");
   }

   return status == success ? success : fail;
}

//***************************************************************************
// Set Parameter
//***************************************************************************

int P4Request::setParameter(ConfigParameter* p)
{
   RequestClean clean(this);

   byte crc;

   if (!p || p->address == addrUnknown)
      return errWrongAddress;

   ConfigParameter pActual(p->address);

   if (getParameter(&pActual) != success)
      return fail;
   
   if (p->value == pActual.value)
      return wrnNonUpdate;

   if (p->value < pActual.min || p->value > pActual.max)
   {
      tell(eloAlways, "value %d out of range %d-%d", 
           p->value, pActual.min, pActual.max);

      return wrnOutOfRange;
   }

   clear();
   addAddress(p->address);
   addAddress(p->value * pActual.factor);

   if (request(cmdSetParameter) != success)
      return errRequestFailed;
   
   if (readHeader() + readWord(p->address) + readWord(p->value) + readByte(crc) != success)
      return fail;

   show("<- ");

   if (p->value == pActual.value)
      return errTransmissionFailed;

   if (readHeader() + readWord(p->address) + readWord(p->value) + readByte(crc) != success)
      return fail;

   show("<- ");

   return getParameter(p);
}

//***************************************************************************
// Get Value
//***************************************************************************

int P4Request::getValue(Value* v)
{
   RequestClean clean(this);
   int status = fail;
   byte crc;

   if (!v || v->address == addrUnknown)
      return errWrongAddress;
   
   clear();
   addAddress(v->address);
   request(cmdGetValue);
   
   if (readHeader() == success)
   {
      status = readWord(v->value)
         + readByte(crc);

      show("<- ");
   }

   return status;
}

//***************************************************************************
// Get Digital Out
//***************************************************************************

int P4Request::getDigitalOut(IoValue* v)
{
   RequestClean clean(this);
   int status = fail;
   byte crc;

   if (!v || v->address == addrUnknown)
      return errWrongAddress;
   
   clear();
   addAddress(v->address);
   request(cmdGetDigOut);
   
   if (readHeader() == success)
   {
      status = readByte(v->mode)
         + readByte(v->state)
         + readByte(crc);

      show("<- ");
   }

   return status;
}

//***************************************************************************
// Get Digital In
//***************************************************************************

int P4Request::getDigitalIn(IoValue* v)
{
   RequestClean clean(this);
   int status = fail;
   byte crc;

   if (!v || v->address == addrUnknown)
      return errWrongAddress;
   
   clear();
   addAddress(v->address);
   request(cmdGetDigIn);
   
   if (readHeader() == success)
   {
      status = readByte(v->mode)
         + readByte(v->state)
         + readByte(crc);

      show("<- ");
   }

   return status;
}

//***************************************************************************
// Get Analog Out
//***************************************************************************

int P4Request::getAnalogOut(IoValue* v)
{
   RequestClean clean(this);
   int status = fail;
   byte crc;

   if (!v || v->address == addrUnknown)
      return errWrongAddress;
   
   clear();
   addAddress(v->address);
   request(cmdGetAnlOut);
   
   if (readHeader() == success)
   {
      status = readByte(v->mode)
         + readByte(v->state)
         + readByte(crc);

      show("<- ");
   }

   return status;
}

//***************************************************************************
// Get Error
//***************************************************************************

int P4Request::getError(ErrorInfo* e, int first)
{
   RequestClean clean(this);
   int status = success;
   int size = 0;
   byte crc;
   byte more;

   if (!e)
      return fail;

   clear();
   request(first ? cmdGetErrorFirst : cmdGetErrorNext);
   
   if (readHeader() != success)
      return fail;

   size = getHeader()->size;
   
   status += readByte(more);
   size--;
   
   if (!more)
   {
      readByte(crc);
      show("<- ");
      tell(eloDebug, "Got 'end of list'");

      return wrnLast;
   }
   
   status += readWord(e->number);
   size -= 2;
   status += readByte(e->info);
   size--;
   status += readByte(e->state);
   size--;
   status += readTimeDate(e->time);
   size -= 6;
   status += readText(e->text, size-sizeCrc);
   size -= size-sizeCrc;
   
   status += readByte(crc);
   show("<- ");

   return status;
}

//***************************************************************************
// Get Value Spec
//***************************************************************************

int P4Request::getValueSpec(ValueSpec* v, int first)
{
   RequestClean clean(this);
   int status = success;
   int size = 0;
   byte crc, tb, b;
   byte more;

   clear();
   request(first ? cmdGetValueListFirst : cmdGetValueListNext);
   
   if (readHeader() != success)
      return fail;

   size = getHeader()->size;

   status += readByte(more);
   size--;

   if (!more)
   {
      readByte(crc);
      show("<- ");
      tell(eloDebug, "Got 'end of list'");

      return wrnLast;
   }

   if (size < 11)
   {
      while (size--)
         readByte(b);

      show("<- ");
      return wrnEmpty;
   }

   status += readWord(v->factor);
   size -= 2;
   status += readWord(v->unknown);
   size -= 2;
   status += readText(v->unit, 2);
   size -= 2;
   status += readWord(v->address);
   size -= 2;
   status += readText(v->description, size-sizeCrc-1);
   size -= size-sizeCrc-1;
   status += readByte(tb);  // termination byte
   size--;
   
   status += readByte(crc);
   show("<- ");

   // create sensor name

   string name = v->description;

   removeCharsExcept(name, nameChars);
   asprintf(&v->name, "%s_0x%x", name.c_str(), v->address);

   return status;   
}

//***************************************************************************
// Get Menu Item
//***************************************************************************

int P4Request::getMenuItem(MenuItem* m, int first)
{
   RequestClean clean(this);
   int status;
   int size = 0;
   byte crc, tb, b;
   byte more;

   memset(m, 0, sizeof(MenuItem));

   clear();
   request(first ? cmdGetMenuListFirst : cmdGetMenuListNext);
   
   if ((status = readHeader()) != success)
      return status;

   size = getHeader()->size;

   if ((status = readByte(more)) != success)
      return status;

   size--;

   if (!more)
   {
      readByte(crc);
      show("<- ");

      tell(eloDebug, "Got 'end of list'");
      return wrnLast;
   }

   if (size < 30)
   {
      tell(eloDebug, "At least 30 byte more expected but only %d pending, skipping item", size);

      while (size-- > 0 && status == success)
         status = readByte(b);

      show("<- ");

      return wrnSkip;
   }

   if ((status = readByte(m->type)) != success)
   {
      tell(eloAlways, "Missing at least %d bytes at reading type (byte)", size);
      show("<- ");
      return status;
   }

   size -= 1;

   if ((status = readByte(m->unknown1)) != success)
   {
      tell(eloAlways, "Missing at least %d bytes at reading unknown1 (byte)", size);
      show("<- ");
      return status;
   }

   size -= 1;

   if ((status = readWord(m->parent)) != success)
   {
      tell(eloAlways, "Missing at least %d bytes at reading parent (word)", size);
      show("<- ");
      return status;
   }

   size -= 2;

   if ((status = readWord(m->child)) != success)
   {
      tell(eloAlways, "Missing at least %d bytes at reading child (word)", size);
      show("<- ");
      return status;
   }

   size -= 2;

   // 18 noch unbekannte byte lesen ...

   for (int i = 0; i < 18; i++)
   {
      if ((status = readByte(b)) != success)
      {
         tell(eloAlways, "Missing at least %d of %d bytes at reading spare bytes", size, getHeader()->size);
         show("<- ");
         return status;
      }
      
      size--;
   }

   if ((status = readWord(m->address)) != success)
   {
      tell(eloAlways, "Missing at least %d bytes at reading address", size);
      show("<- ");
      return status;
   }

   size -= 2;

   if ((status = readWord(m->unknown2)) != success)
   {
      tell(eloAlways, "Missing at least %d bytes at reading unknown2 (word)", size);
      show("<- ");
      return status;
   }

   size -= 2;

   // Rest als text lesen

   if ((status = readText(m->description, size-sizeCrc-1)) != success)
   {
      tell(eloAlways, "Missing at least %d bytes at reading text", size);
      show("<- ");
      return status;
   }

   size -= size-sizeCrc-1;

   if ((status = readByte(tb)) != success)
   {
      tell(eloAlways, "Missing at least %d bytes at reading tb", size);
      show("<- ");
      return status;
   }

   size--;
   
   if ((status = readByte(crc)) != success)
   {
      tell(eloAlways, "Missing at least %d bytes at reading crc", size);
      show("<- ");
      return status;
   }

   show("<- ");
   showDecoded("DEBUG: ");

   return status;   
}

int P4Request::getItem(int first)
{
   RequestClean clean(this);
   byte more;
   clear();
   
   request(first ? cmdGetUnknownFirst : cmdGetUnknownNext);

   if (readHeader() != success)
   {
      tell(eloAlways, "request failed");
      return fail;
   }

   int size = getHeader()->size;
   byte b, crc;

   readByte(more);
   size--;

   if (!more)
   {
      readByte(crc);
      show("<- ");
      tell(eloDebug, "Got 'end of list'");

      return wrnLast;
   }

   while (size > 0 && readByte(b) == success)
      size--;

   show("<- ");

   return done;
}

int P4Request::getUser(byte cmd)
{
   RequestClean clean(this); 
   clear();
   request(cmd);
   
   if (readHeader() != success)
   {
      tell(eloAlways, "request of %d failed", cmd);
      return fail;
   }

   int size = getHeader()->size;
   byte b;

   while (size > 0 && readByte(b) == success)
      size--;

   show("<- ");

   return done;
}
