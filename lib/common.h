/*
 * common.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <openssl/md5.h> // MD5_*

#include <stdint.h>   // uint_64_t
#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>
#include <errno.h>
#include <string.h>
#include <zlib.h>
#include <time.h>

#include <string>
#include <map>
#include <vector>
#include <list>

//***************************************************************************
// Colors
//***************************************************************************

#define escRed       "\033[0;31m"
#define escGreen     "\033[0;32m"
#define escYellow    "\033[0;33m"
#define escBlue      "\033[0;34m"
#define escMagenta   "\033[0;35m"
#define escCyan      "\033[0;36m"
#define escGray      "\033[0;37m"
#define escBlack     "\033[0;38m"


#define escBGray     "\033[1;30m"
#define escBRed      "\033[1;31m"
#define escBGreen    "\033[1;32m"
#define escBYellow   "\033[1;33m"
#define escBBlue     "\033[1;34m"
#define escBMagenta  "\033[1;35m"
#define escBCyan     "\033[1;36m"
#define escBWhite    "\033[1;37m"
#define escBBlack    "\033[1;38m"

#define escReset     "\033[m"

//***************************************************************************

class MemoryStruct;

typedef uint8_t byte;
typedef unsigned short word;
typedef short sword;
typedef unsigned int dword;

//***************************************************************************
//
//***************************************************************************

inline long min(long a, long b) { return a < b ? a : b; }

#define naf -1.0

enum Misc
{
   success = 0,
   done    = success,
   fail    = -1,
   na      = -1,
   ignore  = -2,
   all     = -3,
   abrt    = -4,
   cont    = -5,

   yes     = 1,
   on      = 1,
   off     = 0,
   no      = 0,
   TB      = 1,

   sizeMd5 = 2 * MD5_DIGEST_LENGTH,
   sizeUuid = 36,

   tmeSecondsPerMinute = 60,
   tmeSecondsPerHour = 60 * tmeSecondsPerMinute,
   tmeSecondsPerDay = 24 * tmeSecondsPerHour,
   tmeMinutesPerDay = 24 * 60
};

//***************************************************************************
// Tell
//***************************************************************************

enum Eloquence
{
   eloInfo           = 0x000001,
   eloDetail         = 0x000002,
   eloDebug          = 0x000004,
   eloDebug2         = 0x000008,
   eloWebSock        = 0x000010,
   eloDebugWebSock   = 0x000020,
   eloMqtt           = 0x000040,
   eloDb             = 0x000080,
   eloDebugDb        = 0x000100,

   eloNodeRed        = 0x000200,   // node-red MQTT topics
   eloDeconz         = 0x000400,
   eloDebugDeconz    = 0x000800,
   eloWeather        = 0x001000,

   eloHomeMatic      = 0x002000,
   eloDebugHomeMatic = 0x004000,
   eloLua            = 0x008000,
   eloGroWatt        = 0x010000,

   eloLmc            = 0x020000,
   eloDebugLmc       = 0x040000,

   eloDebugWiringPi  = 0x080000,
   eloScript         = 0x100000,
   eloLoopTimings    = 0x200000,

   eloAlways         = 0x000000
};

class Elo
{
   public:

      static const char* eloquences[];
      static Eloquence stringToEloquence(const std::string string);
      static int toEloquence(const char* str);
};

extern Eloquence eloquence;
extern Eloquence argEloquence;
extern bool logstdout;
extern bool logstamp;

void __attribute__ ((format(printf, 2, 3))) tell(Eloquence eloquence, const char* format, ...);

//***************************************************************************
//
//***************************************************************************

char* srealloc(void* ptr, size_t size);

enum Case
{
   cUpper,
   cLower
};

const char* toCase(Case cs, char* str);

//***************************************************************************
// Zip
//***************************************************************************

ulong gzipBound(ulong size);
int gzip(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen);
void tellZipError(int errorCode, const char* op, const char* msg);
int gunzip(MemoryStruct* zippedData, MemoryStruct* unzippedData);

//***************************************************************************
// MemoryStruct
//***************************************************************************

class MemoryStruct
{
   public:

      MemoryStruct()   { expireAt = 0; memory = 0; zmemory = 0; clear(); }
      explicit MemoryStruct(const MemoryStruct* o)
      {
         size = o->size;
         memory = (char*)malloc(size);
         memcpy(memory, o->memory, size);

         zsize = o->zsize;
         zmemory = (char*)malloc(zsize);
         memcpy(zmemory, o->zmemory, zsize);

         copyAttributes(o);
      }

      ~MemoryStruct()  { clear(); }

      int isEmpty()  { return memory == 0; }
      int isZipped() { return zmemory != 0 && zsize > 0; }

      int append(const char c)
      {
         size_t len = 1;
         memory = srealloc(memory, size+len);
         *(memory+size) = c;
         size += len;

         return success;
      }

      int append(const char* buf)
      {
         size_t len = strlen(buf);
         memory = srealloc(memory, size+len);
         memcpy(memory+size, buf, len);
         size += len;

         return success;
      }

      int append(const char* buf, int len)
      {
         if (len > 0)
         {
            memory = srealloc(memory, size+len);
            memcpy(memory+size, buf, len);
            size += len;
         }

         return success;
      }

      void copyAttributes(const MemoryStruct* o)
      {
         strcpy(tag, o->tag);
         strcpy(name, o->name);
         strcpy(contentType, o->contentType);
         strcpy(contentEncoding, o->contentEncoding);
         strcpy(mimeType, o->mimeType);
         headerOnly = o->headerOnly;
         modTime = o->modTime;
         expireAt = o->expireAt;
      }

      int toGzip()
      {
         free(zmemory);
         zsize = 0;

         if (isEmpty())
            return fail;

         zsize = gzipBound(size) + 512;  // the maximum calculated by the lib, will adusted at gzip() call
         zmemory = (char*)malloc(zsize);

         if (gzip((Bytef*)zmemory, &zsize, (Bytef*)memory, size) != success)
         {
            free(zmemory);
            zsize = 0;
            tell(eloAlways, "Error gzip failed!");

            return fail;
         }

         sprintf(contentEncoding, "gzip");

         return success;
      }

      void clear()
      {
         free(memory);
         memory = nullptr;
         size = 0;
         free(zmemory);
         zmemory = nullptr;
         zsize = 0;
         *tag = 0;
         *name = 0;
         *contentType = 0;
         *contentEncoding = 0;
         *mimeType = 0;
         modTime = time(0);
         headerOnly = no;
         headers.clear();
         statusCode = 0;
         // expireAt = time(0); -> don't reset 'expireAt' here !!!!
      }

      // data

      char* memory {};
      long unsigned int size {0};

      char* zmemory {};
      long unsigned int zsize {0};

      // tag attribute

      char tag[100+TB] {};              // the tag to be compared
      char name[100+TB] {};             // content name (filename)
      char contentType[100+TB] {};      // e.g. text/html
      char mimeType[100+TB] {};         //
      char contentEncoding[100+TB] {};  //
      int headerOnly {0};
      long statusCode {0};
      time_t modTime {0};
      time_t expireAt {0};
      std::map<std::string, std::string> headers;
};

//***************************************************************************
// Wrapper for Regual Expression Library
//***************************************************************************

enum Option
{
   repUseRegularExpression = 1,
   repIgnoreCase = 2
};

int rep(const char* string, const char* expression, Option options = repUseRegularExpression);

int rep(const char* string, const char* expression,
        const char*& s_location, Option options = repUseRegularExpression);

int rep(const char* string, const char* expression, const char*& s_location,
        const char*& e_location, Option options = repUseRegularExpression);

//***************************************************************************
// cMyMutex
//***************************************************************************

class cMyMutex
{
   friend class cCondVar;

   public:

      cMyMutex(void);
      ~cMyMutex();
      void Lock(void);
      void Unlock(void);

      bool isLocked() const { return locked; }

   private:

      pthread_mutex_t mutex;
      int locked;
};

class cMyMutexLock
{
   public:

      explicit cMyMutexLock(cMyMutex* Mutex = NULL);
      ~cMyMutexLock();
      bool Lock(cMyMutex* Mutex);

   private:

      cMyMutex* mutex;
      bool locked;
};

//***************************************************************************
// Strin Extensions
//***************************************************************************

namespace horchi
{
   std::string to_string(double d, size_t precision = 2, bool asHex = false);
}

class myString : public std::string
{
   public:

      myString() : std::string() { }
      myString(const std::string& s) : std::string(s) {}
      myString(const std::string& s, std::size_t n) : std::string(s, n) {}
      myString(const char *s, std::size_t n) : std::string(s, n) {}
      myString(const char *s) : std::string(s) {}
      myString(std::size_t n, char c) : std::string (n, c) {}
      template <class InputIterator>
      myString(InputIterator first, InputIterator last) : std::string(first,last) {}

#if __cplusplus < 202002L
      bool starts_with(const std::string& start) const
      {
         return substr(0, start.length()) == start;
      }
#endif
};

//***************************************************************************
// Tools
//***************************************************************************

std::string __attribute__ ((format(printf, 1, 2))) executeCommand(const char* format, ...);

#ifdef USEUUID
  const char* getUniqueId();
#endif

const char* bin2string(unsigned long n);
const char* bin2string(word n);
const char* bin2string(byte n);
const char* bytesPretty(double bytes, int precision = 0);
double usNow();
bool isDST();
int l2hhmm(time_t t);
time_t midnightOf(time_t t);
const char* toWeekdayName(uint day);
unsigned int getHostId();
byte crc(const byte* data, int size);
int toUTF8(char* out, int outMax, const char* in, const char* from_code = 0);

void removeChars(std::string& str, const char* ignore);
void removeCharsExcept(std::string& str, const char* except);
void removeWord(std::string& pattern, std::string word);
void prepareCompressed(std::string& pattern);
std::string strReplace(const std::string& what, const std::string& with, const std::string& subject);
std::string strReplace(const std::string& what, long with, const std::string& subject);
std::string strReplace(const std::string& what, double with, const std::string& subject, const char* decPoint = nullptr);

bool isNan(double value);
const char* plural(int n, const char* s = "s");
char* rTrim(char* buf);
char* lTrim(char* buf);
char* allTrim(char* buf);
bool isNum(const char* value);
bool isFloat(const char* value);
char* sstrcpy(char* dest, const char* src, int max);
std::string num2Str(int num);
std::string num2Str(double num);
double round2(double d);
std::string l2pTime(time_t t, const char* fmt = "%d.%m.%Y %T");
char* eos(char* s);
const char* toElapsed(int seconds, char* buf);
// #to-be-implemented: splitToInts(const char* string, char c, int& i1, int& i2);
std::vector<std::string> split(const std::string& str, char delim, std::vector<std::string>* strings = nullptr);
std::string getStringBetween(std::string str, const char* begin, const char* end);
std::string getStringBefore(std::string str, const char* begin);
bool fileExists(const char* path);
const char* suffixOf(const char* path);
int createLink(const char* link, const char* dest, int force);
int isLink(const char* path);
int isEmpty(const char* str);
const char* notNull(const char* str, const char* def = "<null>");
int isZero(const char* str);
int removeFile(const char* filename);
int chkDir(const char* path);
int loadFromFile(const char* infile, MemoryStruct* data);
int loadLinesFromFile(const char* infile, std::vector<std::string>& lines, bool removeLF = true, size_t maxLines = 0, const char* expression = nullptr, bool invert = false);
int loadTailLinesFromFile(char const* filename, int count, std::vector<std::string>& lines);
int downloadFile(const char* url, MemoryStruct* data, int timeout = 30, const char* httpproxy = 0);
int storeToFile(const char* filename, const char* data, int size = 0);

struct FsStat
{
   double total;
   double available;
   double used;
   unsigned int usedP;

};

int fsStat(const char* mount, FsStat* stat);

struct FileInfo
{
   std::string path;
   std::string name;
   uint type;
};

typedef std::vector<FileInfo> FileList;
int getFileList(const char* path, int type, const char* extensions, int recursion, FileList* dirs, int& count);
void sortFileList(FileList& list);

const char* getHostName();
const char* getFirstIp();

#ifdef USEMD5
  typedef char md5Buf[sizeMd5+TB];
  typedef char md5;
  int createMd5(const char* buf, md5* md5);
  int createMd5OfFile(const char* path, const char* name, md5* md5);
#endif

std::string getBacktrace(size_t steps);

//***************************************************************************
//
//***************************************************************************

class cRetBuf
{
   public:

      cRetBuf(const char* buf = 0);
      cRetBuf(const cRetBuf& buf);
      virtual ~cRetBuf();

      operator const void* () const { return s; } // to catch cases where operator*() should be used
      operator const char* () const { return s; } // for use in (const char *) context
      const char* operator*() const { return s; } // for use in (const void *) context (printf() etc.)
      cRetBuf& operator = (const cRetBuf& buf);
      cRetBuf& operator = (const char* buf);

      const char* string() { return s; }

   private:

      char* s;
};

//***************************************************************************
// cTimeMs
//***************************************************************************

class cTimeMs
{
   public:

      cTimeMs(int Ms = 0);
      static uint64_t Now();
      static uint64_t now() { return Now(); }
      void Set(int Ms = 0);
      bool TimedOut();
      uint64_t Elapsed();

   private:

      uint64_t begin;
};

typedef cTimeMs cMyTimeMs;

//***************************************************************************
// Smart char pointer
//  - freed at new assigment
//  - freed at destroy
//
// Usage:
//   char_smart p("init");
//
//   printf("char: %s\n", p.c_str());
//   printf("char: %s\n", (const char*)p);
//   asprintf(&p, "%s", "test");
//   printf("char: %s\n", p.c_str());
//***************************************************************************

class char_smart
{
   public:

      char_smart(const char* s = nullptr)  { set(s); }
      ~char_smart() { free(); }

      inline void operator = (const char* s) { set(s); }
      char** operator& () { return ref(); }
      operator const char* () const { return c_str(); }

      void free()
      {
         if (p)
            printf("free %s\n", p);
         ::free(p);
         p = nullptr;
      }

      void set(const char* s = nullptr)
      {
         free();
         if (s)
            p = strdup(s);
      };

      char** ref()
      {
         free();
         return &p;
      };

      char* c_str() const { return p; };

   private:

      char* p {};
};

//***************************************************************************
// Log Duration
//***************************************************************************

class LogDuration
{
   public:

      LogDuration(const char* aMessage, Eloquence aLogLevel = eloDetail);
      ~LogDuration();

      void show(const char* label = "");
      uint64_t getDuration() { return cMyTimeMs::Now() - durationStart; }

   protected:

      char message[1000];
      uint64_t durationStart;
      Eloquence logLevel;
};

//***************************************************************************
// Semaphore
//***************************************************************************

#include <sys/sem.h>

class Sem
{
   public:

      Sem(int aKey)
      {
         key = aKey;

         if ((id = semget(key, 1, 0666 | IPC_CREAT)) == -1)
            tell(eloAlways, "Error: Can't get semaphore, errno (%d) '%s'",
                 errno, strerror(errno));
      }

      ~Sem()
      {
         if (locked)
            v();
      }

      // ----------------------
      // get lock

      int p()
      {
         sembuf sops[2];

         sops[0].sem_num = 0;
         sops[0].sem_op = 0;                        // wait for lock
         sops[0].sem_flg = SEM_UNDO;

         sops[1].sem_num = 0;
         sops[1].sem_op = 1;                        // increment
         sops[1].sem_flg = SEM_UNDO | IPC_NOWAIT;

         if (semop(id, sops, 2) == -1)
         {
            tell(eloAlways, "Error: Can't lock semaphore '0x%x', errno (%d) '%s'", key, errno, strerror(errno));

            return fail;
         }

         locked = true;

         return success;
      }

      int inc()
      {
         sembuf sops[1];

         sops[0].sem_num = 0;
         sops[0].sem_op = 1;                        // increment
         sops[0].sem_flg = SEM_UNDO | IPC_NOWAIT;

         if (semop(id, sops, 1) == -1)
         {
            if (errno != EAGAIN)
               tell(eloAlways, "Error: Can't lock semaphore, errno was (%d) '%s'",
                    errno, strerror(errno));

            return fail;
         }

         locked = true;

         return success;
      }

      // ----------------------
      // decrement

      int dec()
      {
         return v();
      }

      // ----------------------
      // check

      int check()
      {
         sembuf sops[1];

         sops[0].sem_num = 0;
         sops[0].sem_op = 0;
         sops[0].sem_flg = SEM_UNDO | IPC_NOWAIT;

         if (semop(id, sops, 1) == -1)
         {
            if (errno != EAGAIN)
               tell(eloAlways, "Error: Can't lock semaphore, errno was (%d) '%s'",
                    errno, strerror(errno));

            return fail;
         }

         return success;
      }

      // ----------------------
      // release lock

      int v()
      {
         sembuf sops;

         sops.sem_num = 0;
         sops.sem_op = -1;                          // release control
         sops.sem_flg = SEM_UNDO | IPC_NOWAIT;

         if (semop(id, &sops, 1) == -1)
         {
            tell(eloAlways, "Error: Can't unlock semaphore, errno (%d) '%s'",
                 errno, strerror(errno));

            return fail;
         }

         locked = false;

         return success;
      }

   private:

      int key {0};
      int id {0};
      bool locked {false};
};

class SemLock
{
   public:

      SemLock(Sem* aSem) : sem(aSem) { sem->p(); }
      ~SemLock() { sem->v(); }

   private:

      Sem* sem {};
};
