//***************************************************************************
// p4d / Linux - Heizungs Manager
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
// Read Request
//***************************************************************************

int P4Request::request(byte command)
{
   header.id = htons(commId);
   header.command = command;

   prepareRequest();

   show("-> ");

   if (!s || !s->isOpen())
      return fail;

   return s->write(buffer, sizeBufferContent);
}

//***************************************************************************
// Read Header
//***************************************************************************

int P4Request::readHeader(int tms)
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
   tm.tm_isdst = -1;               // force DST auto detect

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
   tm.tm_isdst = -1;               // force DST auto detect

   t = mktime(&tm);
   // printf("got %d %d %d  (%d) '%s'\n", d, m, y, tm.tm_year, l2pTime(t).c_str());

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

#define SOH 0x01
#define XX1 0x02
#define XX2 0x03

int P4Request::readText(char*& s, int size)
{
   byte b;
   int i;
   int p {0};

   if (size <= 0)
   {
      s = strdup("");
      return done;
   }

   char* tmp = (char*)malloc(size+1);
   s = 0;

   for (i = 0; i < size; i++)
   {
      if (readByte(b) != success)
      {
         free(tmp);
         return fail;
      }

      if (b && b != SOH && b != XX1 && b != XX2)
         tmp[p++] = b;
   }

   tmp[p] = 0;
   s = (char*)malloc(2*size);

   if (toUTF8(s, 2*size, tmp) != success)
   {
      tell(eloDetail, "Warning: Converting of charset failed [%s]", tmp);

      if (s)
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

   cMyMutexLock lock(&mutex);

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

   int status {success};
   byte b;

   cMyMutexLock lock(&mutex);

   // cmdGetState

   clear();
   request(cmdGetState);
   s->clear();

   if ((status += readHeader()) == success)
   {
      char* text {};
      char* p {};
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

      int max = 0;

      while (size-- > 0 && max++ < 10)
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

   cMyMutexLock lock(&mutex);
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

   cMyMutexLock lock(&mutex);
   clear();
   addAddress(p->address);
   request(cmdGetParameter);

   if (readHeader() == success)
   {
      byte b;

      // we need at least the address the unknown byte and crc back

      if (header.size < 4)
      {
         tell(eloAlways, "Read header done, got %d bytes instead of 4", header.size);
         for (int i = 0; i < header.size; i++)
            readByte(b);
         return errWrongAddress;
      }

      status = readByte(p->ub1)       // unknown byte 1 -> always 0
         + readWord(p->address)       // address
         + readText(p->unit, 1)       // unit
         + readByte(p->digits)        // decimal digits

         + readByte(p->ub2)           // unknown byte 2

         + readByte(p->factor)        // factor
         + readWord(p->value)         // current value
         + readWord(p->min)           // min value
         + readWord(p->max)           // max value
         + readWord(p->def)           // default value

         + readWord(p->uw1)           // unknown word
         + readByte(p->ub3)           // unknown byte 3

         + readByte(b);               // crc

      if (status == success)
      {
         if (p->factor <= 0)
            p->factor = 1;

         p->rValue = double(p->value) / p->factor;
         // p->rMin = double(p->min) / p->factor;
         // p->rMax = double(p->max) / p->factor;
         // p->rDefault = double(p->def) / p->factor;

         p->rMin = double(p->min);
         p->rMax = double(p->max);
         p->rDefault = double(p->def);

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

   if (p->rValue == pActual.rValue)
   {
      tell(eloAlways, "Value %.*f not changed (%.*f)", p->digits, p->rValue, pActual.digits, pActual.rValue);
      return wrnNonUpdate;
   }

   if (p->rValue < pActual.min || p->rValue > pActual.max)
   {
      tell(eloAlways, "Value %d out of range %d-%d, ignoringe store request", p->value, pActual.min, pActual.max);
      return wrnOutOfRange;
   }

   cMyMutexLock lock(&mutex);
   clear();
   addAddress(p->address);
   addAddress(p->value);

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
// Get Time Ranges
//***************************************************************************

int P4Request::getTimeRanges(TimeRanges* t, int first)
{
   static bool last {false};

   RequestClean clean(this);
   int status {fail};

   if (!t)
      return fail;

   if (first)
      last = false;

   if (!first && last)
      return wrnLast;

   cMyMutexLock lock(&mutex);
   clear();
   int cmd = first ? cmdGetTimesFirst : cmdGetTimesNext;
   request(cmd);

   if ((status = readHeader()) == success)
   {
      byte crc;
      sword w;

      status = readWord(w)            // what ever (always 01 00 ?)
         + readByte(t->address);      // address (here only a byte!!)

      if (status != success)
      {
         tell(eloAlways, "Error: Reading address failed");
         return fail;
      }

      for (int n = 0; n < 4; n++)
      {
         // read time range 'n'

         status = readByte(t->timesFrom[n])
            + readByte(t->timesTo[n]);
      }

      if ((status = readByte(crc)) != success)
      {
         tell(eloAlways, "Error: Reading CRC failed;");
         return fail;
      }

      show("<- ");
   }

   last = t->address == 0xdf;

   return status == success ? success : fail;
}

//***************************************************************************
// Set Time Ranges
//***************************************************************************

int P4Request::setTimeRanges(TimeRanges* t)
{
   RequestClean clean(this);
   int status;
   byte crc;
   byte tmp;
   sword addr;

   if (!t || t->address == 0xFF)
      return errWrongAddress;

   cMyMutexLock lock(&mutex);
   clear();
   addAddress(t->address);

   for (int n = 0; n < 4; n++)
   {
      sword value;

      if ((t->timesFrom[n] > 0xeb && t->timesFrom[n] != 0xff) ||
          (t->timesTo[n] > 0xeb && t->timesTo[n] != 0xff))            // 0xeb => 235 => 23:50
      {
         tell(eloAlways, "Value '%s' isn't a valid time range!", t->getTimeRange(n));
         return wrnOutOfRange;
      }

      value = t->timesFrom[n] << 8 | t->timesTo[n];
      addAddress(value);
   }

   if (request(cmdSetTimes) != success)
      return errRequestFailed;

   tell(eloAlways, "Stored successfully, try to reading response");

   // reas response

   status = readHeader();
   status += readByte(tmp);   // always 00 ?
   status += readWord(addr);

   for (int n = 0; n < 4; n++)
   {
      // read time range 'n'

      status += readByte(t->timesFrom[n]);
      status += readByte(t->timesTo[n]);
   }

   status += readByte(crc);

   if (status != success)
     return errTransmissionFailed;

   show("<- ");

   return success;
}

//***************************************************************************
// Get Value
//***************************************************************************

int P4Request::getValue(Value* v)
{
   RequestClean clean(this);
   int status = fail;
   byte rCrc;

   if (!v || v->address == addrUnknown)
      return errWrongAddress;

   cMyMutexLock lock(&mutex);
   clear();
   addAddress(v->address);
   request(cmdGetValue);

   if (readHeader() == success)
   {
      status = readWord(v->value)
         + readByte(rCrc);

      // CRC check
      {
         int sizeNetto {0};
         byte tmp[sizeof(Header)+100+TB];

         memcpy(tmp+sizeNetto, &header, sizeof(Header));
         sizeNetto += sizeof(Header);
         memcpy(tmp+sizeNetto, &v->value, 2);
         sizeNetto += 2;

         byte bCrc = crc(tmp, sizeNetto);

         if (bCrc != rCrc)
         {
            show("<- ");
            tell(eloAlways, "Error: CRC check failed, got %c, expected %c", bCrc, rCrc);
            return fail;
         }
      }

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

   cMyMutexLock lock(&mutex);
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

   cMyMutexLock lock(&mutex);
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

   cMyMutexLock lock(&mutex);
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

   cMyMutexLock lock(&mutex);
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

#include <locale>

int P4Request::getValueSpec(ValueSpec* v, int first)
{
   RequestClean clean(this);
   int status = success;
   int size = 0;
   byte crc, tb, b;
   byte more;

   cMyMutexLock lock(&mutex);
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
   status += readWord(v->type);
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

   std::string name = v->description;

   removeCharsExcept(name, nameChars);
   asprintf(&v->name, "%s_0x%x", name.c_str(), v->address);

   // patch unit

   /*
   tell(eloDebug, "unit is '%s'", v->unit);

   std::locale l("de_DE.UTF-8");

   for (int i = 0; v->unit[i]; i++)
      if (!std::isprint(v->unit[i], l))
         v->unit[i] = ' ';
   */

   allTrim(v->unit);

   if (isEmpty(v->unit))
   {
      // try to prse unit from description

      const char* s = strchr(v->description, '[');
      const char* e = strchr(v->description, ']');

      if (s && e && e > s)
      {
         free(v->unit);
         asprintf(&v->unit, "%.*s", (int)(e-s)-1, s+1);
         tell(eloDetail, "Using unit '%s' from description", v->unit);
      }
   }

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

   cMyMutexLock lock(&mutex);
   m->clear();
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

   cMyMutexLock lock(&mutex);
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

//***************************************************************************
// Get User Command (to test/analyze any command)
//***************************************************************************

int P4Request::getUser(byte cmd)
{
   RequestClean clean(this);

   cMyMutexLock lock(&mutex);
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
