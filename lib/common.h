/*
 * common.h: EPG2VDR plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>   // uint_64_t
#include <stdlib.h>
#include <iconv.h>

#include <string>

struct MemoryStruct;
extern int loglevel;
extern int logstdout;

using namespace std;

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;

//***************************************************************************
// MemoryStruct for curl callbacks
//***************************************************************************

struct MemoryStruct 
{
   MemoryStruct()   { memory = 0; clear(); }
   ~MemoryStruct()  { clear(); }

   // data

   char* memory;
   size_t size;

   // tag attribute

   int match;
   char tag[100];          // the tag to be compared 
   char ignoreTag[100];

   void clear() 
   {
      free(memory);
      memory = 0;
      size = 0;
      match = 0;
      *tag = 0;
      *ignoreTag = 0;
   }
};

//***************************************************************************
// 
//***************************************************************************

inline long min(long a, long b) { return a < b ? a : b; }

enum Misc
{
   success = 0,
   done    = success,
   fail    = -1,
   na      = -1,
   ignore  = -2,
   all     = -3,
   yes     = 1,
   on      = 1,
   off     = 0,
   no      = 0,
   TB      = 1,

   tmeSecondsPerMinute = 60,
   tmeSecondsPerHour = 60 * tmeSecondsPerMinute,
   tmeSecondsPerDay = 24 * tmeSecondsPerHour
};

//***************************************************************************
// Tell
//***************************************************************************

enum Eloquence
{
   eloOff = na,              // -1
   eloAlways,                // 0
   eloDetail,                // 1
   eloDebug,                 // 2
   eloDebug1 = eloDebug,     // 2
   eloDebug2,                // 3
   eloDebug3                 // 4
};

void tell(int eloquence, const char* format, ...);

//***************************************************************************
// Tools
//***************************************************************************

byte crc(const byte* data, int size);

int toUTF8(char* out, int outMax, const char* in, const char* from_code = 0);

void removeChars(string& str, string chars);
void removeWord(string& pattern, string word);
void prepareCompressed(string& pattern);

char* rTrim(char* buf);
char* lTrim(char* buf);
char* allTrim(char* buf);
char* sstrcpy(char* dest, const char* src, int max);
string num2Str(int num);
string num2Str(double num);
string l2pTime(time_t t);

int fileExists(const char* path);
int createLink(const char* link, const char* dest, int force);
int isLink(const char* path);
int isEmpty(const char* str);
int removeFile(const char* filename);

const char* getHostName();
const char* getFirstIp();

#ifdef WITH_GUNZIP

//***************************************************************************
// Zip
//***************************************************************************

int gunzip(MemoryStruct* zippedData, MemoryStruct* unzippedData);
void tellZipError(int errorCode, const char* op, const char* msg);

#endif // WITH_GUNZIP

//***************************************************************************
// cTimeMs 
//***************************************************************************

class cTimeMs 
{
   public:

      cTimeMs(int Ms = 0);
      static uint64_t Now();
      void Set(int Ms = 0);
      bool TimedOut();
      uint64_t Elapsed();

   private:

      uint64_t begin;
};

//***************************************************************************
#endif // ___COMMON_H
