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
      printf("%s\n", t);
   else
      syslog(LOG_ERR, "%s", t);

   va_end(ap);
}

//***************************************************************************
// String Operations
//***************************************************************************

void removeChars(string& str, string chars)
{
   size_t pos = 0;

   while ((pos = str.find_first_of(chars, pos)) != string::npos)
      str.erase(pos, 1);
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

void prepareCompressed(string& pattern)
{
   const char* ignChars = " (),.;:-_+*!#?=&%$<>§/'@~\"[]{}"; 

   transform(pattern.begin(), pattern.end(), pattern.begin(), ::toupper);
   removeWord(pattern, " TEIL ");
   removeWord(pattern, " FOLGE ");
   removeChars(pattern, ignChars);
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

   if (isEmpty)
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
