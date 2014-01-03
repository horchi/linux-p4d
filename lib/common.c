/*
 * common.c: EPG2VDR plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <sys/stat.h>
#include <sys/time.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <zlib.h>

#include <algorithm>

#ifdef VDR_PLUGIN
# include <vdr/thread.h>
#endif

#include "common.h"

#ifdef VDR_PLUGIN
  cMutex logMutex;
#endif

int loglevel = 1;
int logstdout = no;
int logstamp = no;

//***************************************************************************
// Debug
//***************************************************************************

void tell(int eloquence, const char* format, ...)
{
   if (loglevel < eloquence)
      return ;

   const int sizeBuffer = 100000;
   char t[sizeBuffer+100]; *t = 0;
   va_list ap;

#ifdef VDR_PLUGIN
   cMutexLock lock(&logMutex);
#endif

   va_start(ap, format);

#ifdef VDR_PLUGIN
   snprintf(t, sizeBuffer, "EPG2VDR: ");
#endif

   vsnprintf(t+strlen(t), sizeBuffer-strlen(t), format, ap);
   
   if (logstdout)
   {
      char buf[50+TB];
      *buf = 0;

      if (logstamp)
      {
         timeval tp;
         tm* tm;
         
         gettimeofday(&tp, 0);
         tm = localtime(&tp.tv_sec);
         
         sprintf(buf, "%2.2d:%2.2d:%2.2d,%3.3ld ",
                 tm->tm_hour, tm->tm_min, tm->tm_sec, 
                 tp.tv_usec / 1000);
      }

      printf("%s%s\n", buf, t);
   }
   else
      syslog(LOG_ERR, "%s", t);

   va_end(ap);
}

//***************************************************************************
// Host ID
//***************************************************************************

unsigned int getHostId()
{
   static unsigned int id = gethostid() & 0xFFFFFFFF;
   return id;
}

//***************************************************************************
// Create MD5
//***************************************************************************

#ifdef USEMD5

int createMd5(const char* buf, md5* md5)
{
   MD5_CTX c;
   unsigned char out[MD5_DIGEST_LENGTH];

   MD5_Init(&c);
   MD5_Update(&c, buf, strlen(buf));
   MD5_Final(out, &c);

   for (int n = 0; n < MD5_DIGEST_LENGTH; n++)
      sprintf(md5+2*n, "%02x", out[n]);

   md5[sizeMd5] = 0;

   return done;
}

int createMd5OfFile(const char* path, const char* name, md5* md5)
{
   FILE* f;
   char buffer[1000];
   int nread = 0;
   MD5_CTX c;
   unsigned char out[MD5_DIGEST_LENGTH];
   char* file = 0;

   asprintf(&file, "%s/%s", path, name);
   
   if (!(f = fopen(file, "r")))
   {
      tell(0, "Fatal: Can't access '%s'; %m", file);
      free(file);
      return fail;
   }

   free(file);

   MD5_Init(&c);   
   
   while ((nread = fread(buffer, 1, 1000, f)) > 0)
      MD5_Update(&c, buffer, nread);
   
   fclose(f);

   MD5_Final(out, &c);
   
   for (int n = 0; n < MD5_DIGEST_LENGTH; n++)
      sprintf(md5+2*n, "%02x", out[n]);

   md5[sizeMd5] = 0;

   return success;
}

#endif // USEMD5

//***************************************************************************
// String Operations
//***************************************************************************

void toUpper(std::string& str)
{
   const char* s = str.c_str();
   int lenSrc = str.length();

   char* dest = (char*)malloc(lenSrc+TB); *dest = 0;
   char* d = dest;

   int csSrc;  // size of character

   for (int ps = 0; ps < lenSrc; ps += csSrc)
   {
      csSrc = max(mblen(&s[ps], lenSrc-ps), 1);
      
      if (csSrc == 1)
         *d++ = toupper(s[ps]);
      else if (csSrc == 2 && s[ps] == (char)0xc3 && s[ps+1] >= (char)0xa0)
      {
         *d++ = s[ps];
         *d++ = s[ps+1] - 32;
      }
      else
      {
         for (int i = 0; i < csSrc; i++)
            *d++ = s[ps+i];
      }
   }

   *d = 0;

   str = dest;
   free(dest);
}

//***************************************************************************
// String Operations
//***************************************************************************

void removeChars(std::string& str, const char* ignore)
{
   const char* s = str.c_str();
   int lenSrc = str.length();
   int lenIgn = strlen(ignore);

   char* dest = (char*)malloc(lenSrc+TB); *dest = 0;
   char* d = dest;

   int csSrc;  // size of character
   int csIgn;  // 

   for (int ps = 0; ps < lenSrc; ps += csSrc)
   {
      int skip = no;

      csSrc = max(mblen(&s[ps], lenSrc-ps), 1);

      for (int pi = 0; pi < lenIgn; pi += csIgn)
      {
         csIgn = max(mblen(&ignore[pi], lenIgn-pi), 1);

         if (csSrc == csIgn && strncmp(&s[ps], &ignore[pi], csSrc) == 0)
         {
            skip = yes;
            break;
         }
      }

      if (!skip)
      {
         for (int i = 0; i < csSrc; i++)
            *d++ = s[ps+i];
      }
   }

   *d = 0;

   str = dest;
   free(dest);
}

void removeCharsExcept(std::string& str, const char* except)
{
   const char* s = str.c_str();
   int lenSrc = str.length();
   int lenIgn = strlen(except);

   char* dest = (char*)malloc(lenSrc+TB); *dest = 0;
   char* d = dest;

   int csSrc;  // size of character
   int csIgn;  // 

   for (int ps = 0; ps < lenSrc; ps += csSrc)
   {
      int skip = yes;

      csSrc = max(mblen(&s[ps], lenSrc-ps), 1);

      for (int pi = 0; pi < lenIgn; pi += csIgn)
      {
         csIgn = max(mblen(&except[pi], lenIgn-pi), 1);

         if (csSrc == csIgn && strncmp(&s[ps], &except[pi], csSrc) == 0)
         {
            skip = no;
            break;
         }
      }

      if (!skip)
      {
         for (int i = 0; i < csSrc; i++)
            *d++ = s[ps+i];
      }
   }

   *d = 0;

   str = dest;
   free(dest);
}

void removeWord(string& pattern, string word)
{
   size_t  pos;

   if ((pos = pattern.find(word)) != string::npos)
      pattern.swap(pattern.erase(pos, word.length()));
}

//***************************************************************************
// String Manipulation
//***************************************************************************

void prepareCompressed(std::string& pattern)
{
   // const char* ignore = " (),.;:-_+*!#?=&%$<>§/'`´@~\"[]{}"; 
   const char* notignore = "ABCDEFGHIJKLMNOPQRSTUVWXYZßÖÄÜöäü0123456789"; 

   toUpper(pattern);
   removeWord(pattern, " TEIL ");
   removeWord(pattern, " FOLGE ");
   removeCharsExcept(pattern, notignore);
}

const char* plural(int n)
{ 
   return n > 0 ? "s" : ""; 
}

//***************************************************************************
// Left Trim
//***************************************************************************

char* lTrim(char* buf)
{
   if (buf)
   {
      char *tp = buf;

      while (*tp && strchr("\n\r\t ",*tp)) 
         tp++;

      memmove(buf, tp, strlen(tp) +1);
   }
   
   return buf;
}

//*************************************************************************
// Right Trim
//*************************************************************************

char* rTrim(char* buf)
{
   if (buf)
   {
      char *tp = buf + strlen(buf);

      while (tp >= buf && strchr("\n\r\t ",*tp)) 
         tp--;

      *(tp+1) = 0;
   }
   
   return buf;
}

//*************************************************************************
// All Trim
//*************************************************************************

char* allTrim(char* buf)
{
   return lTrim(rTrim(buf));
}

//***************************************************************************
// Is Number
//***************************************************************************

int isNum(const char* value)
{
   char* p = (char*)value;

   while(*p)
      if (!isdigit(*p++))
         return no;

   return yes;
}

//***************************************************************************
// Number to String
//***************************************************************************

string num2Str(int num)
{
   char txt[16];

   snprintf(txt, sizeof(txt), "%d", num);

   return string(txt);
}

string num2Str(double num)
{
   char txt[16];

   snprintf(txt, sizeof(txt), "%.2f", num);

   return string(txt);
}

//***************************************************************************
// Long to Pretty Time
//***************************************************************************

string l2pTime(time_t t)
{
   char txt[30];
   tm* tmp = localtime(&t);
   
   strftime(txt, sizeof(txt), "%d.%m.%Y %T", tmp);
   
   return string(txt);
}

const char* toElapsed(int seconds, char* buf)
{   
   char* p = buf;
   int parts = 0;

   int days = seconds / tmeSecondsPerDay;
   seconds %= tmeSecondsPerDay;
   int hours = seconds / tmeSecondsPerHour;
   seconds %= tmeSecondsPerHour;
   int minutes = seconds / tmeSecondsPerMinute;
   seconds %= tmeSecondsPerMinute;

   if (days)
   {
      p += sprintf(p, " %d day%s", days, plural(days));
      parts++;
   }
   
   if (parts < 2 && hours)
   {
      p += sprintf(p, " %d hour%s", hours, plural(hours));
      parts++;
   }
   
   if (parts < 2 && minutes)
   {
      p += sprintf(p, " %d minute%s", minutes, plural(minutes));
      parts++;
   }
   
   if (!parts)
   {
      p += sprintf(p, " %d second%s", seconds, plural(seconds));
      parts++;
   }

   return lTrim(buf);
}

//***************************************************************************
// TOOLS
//***************************************************************************

int isEmpty(const char* str)
{
   return !str || !*str;
}

char* sstrcpy(char* dest, const char* src, int max)
{
   if (!dest || !src)
      return 0;

   strncpy(dest, src, max);
   dest[max-1] = 0;
   
   return dest;
}

int isLink(const char* path)
{
   struct stat sb;

   if (lstat(path, &sb) == 0)
      return S_ISLNK(sb.st_mode);

   tell(0, "Error: Detecting state for '%s' failed, error was '%m'", path);

   return false;
}

int fileExists(const char* path)
{
   return access(path, F_OK) == 0; 
}

int createLink(const char* link, const char* dest, int force)
{
   if (!fileExists(link) || force)
   {
      // may be the link exists and point to a wrong or already deleted destination ...
      //   .. therefore we delete the link at first
      
      unlink(link);
      
      if (symlink(dest, link) != 0)
      {
         tell(0, "Failed to create symlink '%s', error was '%m'", link);
         return fail;
      }
   }

   return success;
}


//***************************************************************************
// Remove File
//***************************************************************************

int removeFile(const char* filename)
{
   if (unlink(filename) != 0)
   {
      tell(0, "Can't remove file '%s', '%m'", filename);
      
      return 1;
   }

   tell(2, "Removed file '%s'", filename);
   
   return 0;
}

#ifdef WITH_GUNZIP

//***************************************************************************
// Gnu Unzip
//***************************************************************************

int gunzip(MemoryStruct* zippedData, MemoryStruct* unzippedData)
{
   const int growthStep = 1024;

   z_stream stream = {0,0,0,0,0,0,0,0,0,0,0,Z_NULL,Z_NULL,Z_NULL};
   unsigned int resultSize = 0;
   int res = 0;

   unzippedData->clear();

   // determining the size in this way is taken from the sources of the gzip utility.

   memcpy(&unzippedData->size, zippedData->memory + zippedData->size -4, 4); 
   unzippedData->memory = (char*)malloc(unzippedData->size);

   // zlib initialisation

   stream.avail_in  = zippedData->size;
   stream.next_in   = (Bytef*)zippedData->memory;
   stream.avail_out = unzippedData->size;
   stream.next_out  = (Bytef*)unzippedData->memory;

   // The '+ 32' tells zlib to process zlib&gzlib headers

   res = inflateInit2(&stream, MAX_WBITS + 32);

   if (res != Z_OK)
   {
      tellZipError(res, " during zlib initialisation", stream.msg);
      inflateEnd(&stream);
      return fail;
   }

   // skip the header

   res = inflate(&stream, Z_BLOCK);

   if (res != Z_OK)
   {
      tellZipError(res, " while skipping the header", stream.msg);
      inflateEnd(&stream);
      return fail;
   }

   while (res == Z_OK)
   {
      if (stream.avail_out == 0)
      {
         unzippedData->size += growthStep;
         unzippedData->memory = (char*)realloc(unzippedData->memory, unzippedData->size);

         // Set the stream pointers to the potentially changed buffer!

         stream.avail_out = resultSize - stream.total_out;
         stream.next_out  = (Bytef*)(unzippedData + stream.total_out);
      }

      res = inflate(&stream, Z_SYNC_FLUSH);
      resultSize = stream.total_out;
   }

   if (res != Z_STREAM_END)
   {
      tellZipError(res, " during inflating", stream.msg);
      inflateEnd(&stream);
      return fail;
   }

   unzippedData->size = resultSize;
   inflateEnd(&stream);

   return success;
}

//*************************************************************************
// tellZipError
//*************************************************************************

void tellZipError(int errorCode, const char* op, const char* msg)
{
   if (!op)  op  = "";
   if (!msg) msg = "None";

   switch (errorCode)
   {
      case Z_OK:           return;
      case Z_STREAM_END:   return;
      case Z_MEM_ERROR:    tell(0, "Error: Not enough memory to unzip file%s!\n", op); return;
      case Z_BUF_ERROR:    tell(0, "Error: Couldn't unzip data due to output buffer size problem%s!\n", op); return;
      case Z_DATA_ERROR:   tell(0, "Error: Zipped input data corrupted%s! Details: %s\n", op, msg); return;
      case Z_STREAM_ERROR: tell(0, "Error: Invalid stream structure%s. Details: %s\n", op, msg); return;
      default:             tell(0, "Error: Couldn't unzip data for unknown reason (%6d)%s!\n", errorCode, op); return;
   }
}

#endif  // WITH_GUNZIP

//*************************************************************************
// Host Data
//*************************************************************************

#include <sys/utsname.h>
#include <netdb.h>
#include <ifaddrs.h>

static struct utsname info;

const char* getHostName()
{
   // get info from kernel

   if (uname(&info) == -1)
      return "";

   return info.nodename;
}

const char* getFirstIp()
{
   struct ifaddrs *ifaddr, *ifa;
   static char host[NI_MAXHOST] = "";

   if (getifaddrs(&ifaddr) == -1) 
   {
      tell(0, "getifaddrs() failed");
      return "";
   }

   // walk through linked interface list

   for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) 
   {
      if (!ifa->ifa_addr)
         continue;
      
      // For an AF_INET interfaces

      if (ifa->ifa_addr->sa_family == AF_INET) //  || ifa->ifa_addr->sa_family == AF_INET6) 
      {
         int res = getnameinfo(ifa->ifa_addr, 
                               (ifa->ifa_addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) :
                               sizeof(struct sockaddr_in6),
                               host, NI_MAXHOST, 0, 0, NI_NUMERICHOST);

         if (res)
         {
            tell(0, "getnameinfo() failed: %s", gai_strerror(res));
            return "";
         }

         // skip loopback interface

         if (strcmp(host, "127.0.0.1") == 0)
            continue;

         tell(5, "%-8s %-15s %s", ifa->ifa_name, host,
              ifa->ifa_addr->sa_family == AF_INET   ? " (AF_INET)" :
              ifa->ifa_addr->sa_family == AF_INET6  ? " (AF_INET6)" : "");
      }
   }

   freeifaddrs(ifaddr);

   return host;
}

//***************************************************************************
// To UTF8
//***************************************************************************

int toUTF8(char* out, int outMax, const char* in, const char* from_code)
{
   iconv_t cd;
   size_t ret;
   char* toPtr;
   char* fromPtr;
   size_t fromLen, outlen;

   const char* to_code = "UTF-8";

   if (isEmpty(from_code))
      from_code = "ISO8859-1";

   if (!out || !in || !outMax)
      return fail;

   *out = 0;
   fromLen = strlen(in);
   
   if (!fromLen)
      return fail;
   
	cd = iconv_open(to_code, from_code);

   if (cd == (iconv_t)-1) 
      return fail;

   fromPtr = (char*)in;
   toPtr = out;
   outlen = outMax;

   ret = iconv(cd, &fromPtr, &fromLen, &toPtr, &outlen);

   *toPtr = 0;   
   iconv_close(cd);

   if (ret == (size_t)-1)
   {
      tell(0, "Converting [%s] from '%s' to '%s' failed", 
           fromPtr, from_code, to_code);

		return fail;
   }

   return success;
}

//***************************************************************************
// CRC
//***************************************************************************

byte crc(const byte* data, int size)
{
   byte dummy;
   byte crc = 0;

   for (int count = 0; count < size; count++)
   {
      dummy = data[count] * 2 & 0xFF;
      crc = crc ^ (byte)(data[count] ^ dummy);
   }

   return crc;
}

//***************************************************************************
// 
//***************************************************************************

cRetBuf::cRetBuf(const char* buf)
{
  s = buf ? strdup(buf) : 0;
}

cRetBuf::cRetBuf(const cRetBuf& buf)
{
  s = buf.s ? strdup(buf.s) : 0;
}

cRetBuf::~cRetBuf()
{
  free(s);
}

cRetBuf& cRetBuf::operator = (const cRetBuf& buf)
{
  if (this == &buf)
     return *this;

  free(s);
  s = buf.s ? strdup(buf.s) : 0;

  return *this;
}

cRetBuf& cRetBuf::operator=(const char* buf)
{
  if (s == buf)
    return *this;

  free(s);
  s = buf ? strdup(buf) : 0;

  return *this;
}

//***************************************************************************
// cTimeMs 
//***************************************************************************

cTimeMs::cTimeMs(int Ms)
{
  if (Ms >= 0)
     Set(Ms);
  else
     begin = 0;
}

uint64_t cTimeMs::Now(void)
{
  struct timeval t;

  if (gettimeofday(&t, 0) == 0)
     return ((uint64_t)t.tv_sec) * 1000 + t.tv_usec / 1000;

  return 0;
}

void cTimeMs::Set(int Ms)
{
  begin = Now() + Ms;
}

bool cTimeMs::TimedOut(void)
{
  return Now() >= begin;
}

uint64_t cTimeMs::Elapsed(void)
{
  return Now() - begin;
}
