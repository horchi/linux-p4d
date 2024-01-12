/*
 * common.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <sys/stat.h>
#include <sys/time.h>
#include <math.h>

#ifdef USEUUID
#  include <uuid/uuid.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <zlib.h>
#include <dirent.h>
#include <regex.h>
#include <fcntl.h>

#include <algorithm>
#include <regex>

#ifdef VDR_PLUGIN
# include <vdr/thread.h>
#endif

#include "common.h"

#ifdef VDR_PLUGIN
  cMutex logMutex;
#endif

Eloquence eloquence {eloAlways};
Eloquence argEloquence {eloAlways};
bool logstdout {false};
bool logstamp {false};

//***************************************************************************
// Logging
//***************************************************************************

const char* Elo::eloquences[] =
{
   "Info",
   "Detail",
   "Debug",
   "Debug2",
   "WebSock",
   "DebugWebSock",
   "Mqtt",
   "Db",
   "DebugDb",

   "NodeRed",
   "Deconz",
   "DebugDeconz",
   "Weather",

   "HomeMatic",
   "DebugHomeMatic",
   "Lua",
   "GroWatt",

   "Lmc",
   "DebugLmc",

   nullptr
};

Eloquence Elo::stringToEloquence(const std::string string)
{
   int elo {0};
   int n {0};

   auto elos = split(string, ',');

   for (const auto& e : elos)
   {
      if ((n = toEloquence(e.c_str())) != na)
         elo |= n;
   }

   return (Eloquence)elo;
}

int Elo::toEloquence(const char* str)
{
   for (int i = 0; eloquences[i]; i++)
      if (strcasecmp(eloquences[i], str) == 0)
         return (Eloquence)pow(2, i);

   return na;
}

void tell(Eloquence elo, const char* format, ...)
{
   if (elo && !(eloquence & elo))
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
   {
      bool error = strncmp(t, "Err", 3) != 0;
      bool warning = strncmp(t, "Warn", 4) != 0;

      if (error)                  syslog(LOG_ERR, "%s", t);
      else if (warning)           syslog(LOG_WARNING, "%s", t);
      else if (elo & eloInfo)     syslog(LOG_NOTICE, "%s", t);
      else if (elo & eloDetail)   syslog(LOG_INFO, "%s", t);
      else                        syslog(LOG_DEBUG, "%s", t);

      // int prio = elo == eloAlways ? LOG_ERR : LOG_NOTICE;
      // syslog(prio, "%s", t);
   }

   va_end(ap);
}

//***************************************************************************
// Execute Command
//***************************************************************************

std::string executeCommand(const char* cmd)
{
   char buffer[128] {'\0'};
   std::string result {""};

   FILE* pipe = popen(cmd, "r");

   while (fgets(buffer, sizeof(buffer), pipe))
      result += buffer;

   pclose(pipe);

   return result;
}

//***************************************************************************
// Save Realloc
//***************************************************************************

char* srealloc(void* ptr, size_t size)
{
   void* n = realloc(ptr, size);

   if (!n)
   {
      free(ptr);
      ptr = 0;
   }

   return (char*)n;
}

//***************************************************************************
// us now
//***************************************************************************

double usNow()
{
   struct timeval tp;

   gettimeofday(&tp, 0);

   return tp.tv_sec * 1000000.0 + tp.tv_usec;
}


//***************************************************************************
// bin 2 string
//***************************************************************************

const char* bin2string(word n)
{
   static char bin[17];

   for (int x = 0; x < 16; x++)
   {
      bin[x] = n & 0x8000 ? '1' : '0';
      n <<= 1;
   }

   bin[16] = '\0';

   return bin;
}

const char* bin2string(byte n)
{
   static char bin[9];

   for (int x = 0; x < 8; x++)
   {
      bin[x] = n & 0x80 ? '1' : '0';
      n <<= 1;
   }

   bin[8] = '\0';

   return bin;
}

const char* bin2string(unsigned long n)
{
   const int len {4*8};
   static char bin[len+1];

   for (int x = 0; x < len; x++)
   {
      bin[x] = n & 0x80000000 ? '1' : '0';
      n <<= 1;
   }

   bin[len] = '\0';

   return bin;
}

//***************************************************************************
// Bytes Pretty
//***************************************************************************

const char* bytesPretty(double bytes, int precision)
{
   const char* unit= "B";

   if (bytes > 1024)
   {
      bytes /= 1024;
      unit= "kB";
   }

   if (bytes > 1024)
   {
      bytes /= 1024;
      unit= "MB";
   }

   if (bytes > 1024)
   {
      bytes /= 1024;
      unit= "GB";
   }

   if (bytes > 1024)
   {
      bytes /= 1024;
      unit= "TB";
   }

   static char res[100];
   sprintf(res, "%.*f %s", precision, bytes, unit);

   return res;
}

//***************************************************************************
// time_t to hhmm like '2015'
//***************************************************************************

int l2hhmm(time_t t)
{
   struct tm tm;

   localtime_r(&t, &tm);

   return  tm.tm_hour * 100 + tm.tm_min;
}

time_t midnightOf(time_t t)
{
   struct tm tm;

   localtime_r(&t, &tm);

   tm.tm_hour = 0;
   tm.tm_min = 0;
   tm.tm_sec = 0;
   tm.tm_isdst = -1;  // force DST auto detect

   return mktime(&tm);
}

const char* toWeekdayName(uint day)
{
   const char* dayNames[] =
   {
      "Montag",
      "Dienstag",
      "Mittwoch",
      "Donnerstag",
      "Freitag",
      "Samstag",
      "Sonntag",
      0
   };

   if (day > 6)
      return "<unknown>";

   return dayNames[day];
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

#include <openssl/evp.h>

#if OPENSSL_VERSION_MAJOR >= 3

int createMd5(const char* buf, md5* md5)
{
   EVP_MD_CTX* mdctx;
   unsigned char* md5_digest {};
   unsigned int md5_digest_len = EVP_MD_size(EVP_md5());

   mdctx = EVP_MD_CTX_new();
   EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);

   EVP_DigestUpdate(mdctx, buf, strlen(buf));

   md5_digest = (unsigned char*)OPENSSL_malloc(md5_digest_len);
   EVP_DigestFinal_ex(mdctx, md5_digest, &md5_digest_len);
   EVP_MD_CTX_free(mdctx);

   for (uint n = 0; n < md5_digest_len; n++)
      sprintf(md5+2*n, "%02x", md5_digest[n]);

   md5[sizeMd5] = 0;

   return done;
}

int createMd5OfFile(const char* path, const char* name, md5* md5)
{
   EVP_MD_CTX* mdctx;
   unsigned char* md5_digest;
   unsigned int md5_digest_len = EVP_MD_size(EVP_md5());

   char* file {};
   asprintf(&file, "%s/%s", path, name);

   FILE* f {};

   if (!(f = fopen(file, "r")))
   {
      tell(eloAlways, "Fatal: Cannot build MD5 of '%s'; Error was '%s'", file, strerror(errno));
      free(file);
      return fail;
   }

   free(file);

   char buffer[1000];
   int nread = 0;

   mdctx = EVP_MD_CTX_new();
   EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);

   while ((nread = fread(buffer, 1, 1000, f)) > 0)
      EVP_DigestUpdate(mdctx, buffer, nread);

   fclose(f);

   md5_digest = (unsigned char*)OPENSSL_malloc(md5_digest_len);
   EVP_DigestFinal_ex(mdctx, md5_digest, &md5_digest_len);
   EVP_MD_CTX_free(mdctx);

   for (uint n = 0; n < md5_digest_len; n++)
      sprintf(md5+2*n, "%02x", md5_digest[n]);

   md5[sizeMd5] = 0;

   return success;
}

#else

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
      tell(eloAlways, "Fatal: Cannot build MD5 of '%s'; Error was '%s'", file, strerror(errno));
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
#endif // OPENSSL_NO_DEPRECATED_3_0

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
      csSrc = std::max(mblen(&s[ps], lenSrc-ps), 1);

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
// To Case (UTF-8 save)
//***************************************************************************

const char* toCase(Case cs, char* str)
{
   char* s = str;
   int lenSrc = strlen(str);

   int csSrc;  // size of character

   for (int ps = 0; ps < lenSrc; ps += csSrc)
   {
      csSrc = std::max(mblen(&s[ps], lenSrc-ps), 1);

      if (csSrc == 1)
         s[ps] = cs == cUpper ? toupper(s[ps]) : tolower(s[ps]);
      else if (csSrc == 2 && s[ps] == (char)0xc3 && s[ps+1] >= (char)0xa0)
      {
         s[ps] = s[ps];
         s[ps+1] = cs == cUpper ? toupper(s[ps+1]) : tolower(s[ps+1]);
      }
      else
      {
         for (int i = 0; i < csSrc; i++)
            s[ps+i] = s[ps+i];
      }
   }

   return str;
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

      mblen(0,0);
      csSrc = std::max(mblen(&s[ps], lenSrc-ps), 1);

      for (int pi = 0; pi < lenIgn; pi += csIgn)
      {
         csIgn = std::max(mblen(&ignore[pi], lenIgn-pi), 1);

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

      mblen(0,0);
      csSrc = std::max(mblen(&s[ps], lenSrc-ps), 1);

      for (int pi = 0; pi < lenIgn; pi += csIgn)
      {
         csIgn = std::max(mblen(&except[pi], lenIgn-pi), 1);

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

void removeWord(std::string& pattern, std::string word)
{
   size_t pos;

   if ((pos = pattern.find(word)) != std::string::npos)
      pattern.swap(pattern.erase(pos, word.length()));
}

std::string strReplace(const std::string& what, const std::string& with, const std::string& subject)
{
   std::string str = subject;
   size_t pos {0};

   while ((pos = str.find(what, pos)) != std::string::npos)
   {
      str.replace(pos, what.length(), with);
      pos += with.length();
   }

   return str;
}

std::string strReplace(const std::string& what, long with, const std::string& subject)
{
   char swith[100];

   sprintf(swith, "%ld", with);

   return strReplace(what, swith, subject);
}

std::string strReplace(const std::string& what, double with, const std::string& subject, const char* decPoint)
{
   char swith[100];

   sprintf(swith, "%f", with);

   if (decPoint)
   {
      std::string sWith = strReplace(",", decPoint, swith);
      return strReplace(what, sWith.c_str(), subject);
   }

   return strReplace(what, swith, subject);
}

//***************************************************************************
// isNan
//***************************************************************************

bool isNan(double value)
{
   return isnan(value);
}

//***************************************************************************
// plural
//***************************************************************************

const char* plural(int n, const char* s)
{
   return n == 1 ? "" : s;
}

//***************************************************************************
// Left Trim
//***************************************************************************

char* lTrim(char* buf)
{
   if (buf)
   {
      char* tp = buf;

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

bool isNum(const char* value)
{
   const char* p = value;

   while (*p)
   {
      if (p == value && *p == '-')  // '-' is allowed at first pos
      {
         p++;
         continue;
      }

      if (!isdigit(*p))
         return false;

      p++;
   }

   return true;
}

bool isFloat(const char* value)
{
   uint digits {0};
   const char* p {value};

   while (*p)
   {
      if (p == value && *p == '-')  // '-' is allowed at first pos
      {
         p++;
         continue;
      }

      if (*p == ',')
      {
         if (++digits > 1)
            return false;

         p++;
         continue;
      }

      if (!isdigit(*p))
         return false;

      p++;
   }

   return true;
}

//***************************************************************************
// Number to String
//***************************************************************************

std::string num2Str(int num)
{
   char txt[16];

   snprintf(txt, sizeof(txt), "%d", num);

   return std::string(txt);
}

std::string num2Str(double num)
{
   char txt[16];

   snprintf(txt, sizeof(txt), "%.2f", num);

   return std::string(txt);
}


//***************************************************************************
// real round to 2 decimals
//***************************************************************************

double round2(double d)
{
   return (int)(d*100+0.5)/100.0;
}

//***************************************************************************
// Split String to Vector
//***************************************************************************

std::vector<std::string> split(const std::string& str, char delim, std::vector<std::string>* strings)
{
   std::vector<std::string> _strings;

   if (!strings)
      strings = &_strings;

   size_t start;
   size_t end {0};

   while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
   {
      end = str.find(delim, start);
      std::string s = std::regex_replace(str.substr(start, end - start), std::regex("^\\s+"), std::string(""));
      strings->push_back(std::regex_replace(s, std::regex("\\s+$"), std::string("")));
   }

   return *strings;
}

//***************************************************************************
// Get String Before
//***************************************************************************

std::string getStringBefore(std::string str, const char* begin)
{
   size_t pos = str.find(begin);
   return str.substr(0, pos);
}

//***************************************************************************
// Get String Between
//***************************************************************************

std::string getStringBetween(std::string str, const char* begin, const char* end)
{
   size_t first = str.find(begin);
   size_t last = str.find(end, first);

   if (first == str.npos || last == str.npos)
      return "";

   return str.substr(first+1, last-(first+1));
}

//***************************************************************************
// End Of String
//***************************************************************************

char* eos(char* s)
{
   if (!s)
      return 0;

   return s + strlen(s);
}

//***************************************************************************
// Long to Pretty Time
//***************************************************************************

std::string l2pTime(time_t t, const char* fmt)
{
   char txt[300];
   tm* tmp = localtime(&t);

   strftime(txt, sizeof(txt), fmt, tmp);

   return std::string(txt);
}

const char* toElapsed(int seconds, char* buf)
{
   char* p = buf;
   int parts {0};

   int days = seconds / tmeSecondsPerDay;
   seconds %= tmeSecondsPerDay;
   int hours = seconds / tmeSecondsPerHour;
   seconds %= tmeSecondsPerHour;
   int minutes = seconds / tmeSecondsPerMinute;
   seconds %= tmeSecondsPerMinute;

   if (days)
   {
      p += sprintf(p, " %d Tag%s", days, plural(days, "e"));
      parts++;
   }

   if (parts < 2 && hours)
   {
      p += sprintf(p, " %d Stunde%s", hours, plural(hours, "n"));
      parts++;
   }

   if (parts < 2 && minutes)
   {
      p += sprintf(p, " %d Minute%s", minutes, plural(minutes, "n"));
      parts++;
   }

   if (!parts)
   {
      p += sprintf(p, " %d Sekunde%s", seconds, plural(seconds, "n"));
      parts++;
   }

   return lTrim(buf);
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

   tell(eloAlways, "Error: Detecting state for '%s' failed, error was '%s'", path, strerror(errno));

   return false;
}

const char* suffixOf(const char* path)
{
   const char* p;

   if (path && (p = strrchr(path, '.')))
      return p+1;

   return "";
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
         tell(eloAlways, "Failed to create symlink '%s', error was '%s'", link, strerror(errno));
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
      tell(eloAlways, "Can't remove file '%s', '%s'", filename, strerror(errno));

      return 1;
   }

   tell(eloDebug, "Removed file '%s'", filename);

   return 0;
}

//***************************************************************************
// Check Dir
//***************************************************************************

int chkDir(const char* path)
{
   struct stat fs;

   if (stat(path, &fs) != 0 || !S_ISDIR(fs.st_mode))
   {
      tell(eloAlways, "Creating directory '%s'", path);

      if (mkdir(path, ACCESSPERMS) == -1)
      {
         tell(eloAlways, "Can't create directory '%s'", strerror(errno));
         return fail;
      }
   }

   return success;
}

//***************************************************************************
// Load From File
//***************************************************************************

int loadFromFile(const char* infile, MemoryStruct* data)
{
   FILE* fin;
   struct stat sb;

   data->clear();

   if (!fileExists(infile))
   {
      tell(eloAlways, "File '%s' not found'", infile);
      return fail;
   }

   if (stat(infile, &sb) < 0)
   {
      tell(eloAlways, "Can't get info of '%s', error was '%s'", infile, strerror(errno));
      return fail;
   }

   if ((fin = fopen(infile, "r")))
   {
      const char* sfx = suffixOf(infile);

      data->size = sb.st_size;
      data->modTime = sb.st_mtime;
      data->memory = (char*)malloc(data->size);
      fread(data->memory, sizeof(char), data->size, fin);
      fclose(fin);
      sprintf(data->tag, "%ld", (long int)data->size);

      if (strcmp(sfx, "gz") == 0)
         sprintf(data->contentEncoding, "gzip");

      if (strcmp(sfx, "js") == 0)
         sprintf(data->contentType, "application/javascript");

      else if (strcmp(sfx, "png") == 0 || strcmp(sfx, "jpg") == 0 || strcmp(sfx, "gif") == 0)
         sprintf(data->contentType, "image/%s", sfx);
      else if (strcmp(sfx, "svg") == 0)
         sprintf(data->contentType, "image/%s+xml", sfx);
      else if (strcmp(sfx, "ico") == 0)
         strcpy(data->contentType, "image/x-icon");

      else
         sprintf(data->contentType, "text/%s", sfx);
   }
   else
   {
      tell(eloAlways, "Error, can't open '%s' for reading, error was '%s'", infile, strerror(errno));
      return fail;
   }

   return success;
}

//***************************************************************************
// Store To File
//***************************************************************************

int storeToFile(const char* filename, const char* data, int size)
{
   FILE* fout;

   if (!(fout = fopen(filename, "w+")))
      return fail;

   size = size > 0 ? size : strlen(data);
   fwrite(data, sizeof(char), size, fout);
   fclose(fout);

   return success;
}

//***************************************************************************
// Filesystem Stat
//***************************************************************************

#include <sys/statvfs.h>

int fsStat(const char* mount, FsStat* stat)
{
   struct statvfs statVFS;

   if (statvfs(mount, &statVFS) != 0)
      return fail;

   stat->total = (double)(statVFS.f_blocks * statVFS.f_frsize);
   stat->available = (double)(statVFS.f_bfree * statVFS.f_frsize);
   stat->used = stat->total - stat->available;
   stat->usedP = (stat->used / stat->total) * 100.0;

   return success;
}

//***************************************************************************
// TOOLS
//***************************************************************************

int isEmpty(const char* str)
{
   return !str || !*str;
}

const char* notNull(const char* str, const char* def)
{
   if (!str)
      return def;

   return str;
}

int isZero(const char* str)
{
   const char* p = str;

   while (p && *p)
   {
      if (*p != '0')
         return no;

      p++;
   }

   return yes;
}

//***************************************************************************
// Load Lines
//***************************************************************************

int loadLinesFromFile(const char* infile, std::vector<std::string>& lines, bool removeLF)
{
   FILE* fp;

   if (!fileExists(infile))
   {
      tell(eloAlways, "File '%s' not found'", infile);
      return fail;
   }

   if (!(fp = fopen(infile, "r")))
   {
      tell(eloAlways, "Error, can't open '%s' for reading, error was '%s'", infile, strerror(errno));
      return fail;
   }

   char* line {nullptr};
   size_t len {0};

   while (getline(&line, &len, fp) != -1)
   {
      if (removeLF)
         line[strlen(line)-1] = 0;

      lines.push_back(line);
      line[0] = '\0';
   }

   fclose(fp);
   free(line);

   return success;
}

//*************************************************************************
// Read Remaining from ds
//*************************************************************************

uintmax_t readRemaining(int fd, uintmax_t nBytes, std::vector<std::string>& lines)
{
   uintmax_t nDone {0};
   uintmax_t nRemaining {nBytes};

   while (nRemaining)
   {
      char buffer[BUFSIZ];
      ssize_t nRead = read(fd, buffer, std::min((int)nRemaining, BUFSIZ));

      if (!nRead || nRead == -1)
         return nDone;

      // printf("%.*s", (int)nRead, buffer);
      split({buffer, (size_t)nRead-1}, '\n', &lines);
      // lines.insert(lines.begin(), {buffer, (size_t)nRead-1});
      nDone += nRead;
      nRemaining -= nRead;
   }

   return nDone;
}

//*************************************************************************
// get the last n lines from fd
//*************************************************************************

int nTailFileLines(int fd, char const* filename, uintmax_t nLines, std::vector<std::string>& lines)
{
   if (!nLines)
      return success;

   off_t startPos {0};
   char buffer[BUFSIZ];
   ssize_t nRead {0};
   off_t endPos = lseek(fd, 0, SEEK_END);
   off_t pos = endPos;

   // Set 'nRead' to the size of the last, probably partial, buffer;
   //   0 < 'nRead' <= 'BUFSIZ'

   nRead = (pos - startPos) % BUFSIZ;

   if (nRead == 0)
      nRead = BUFSIZ;

   // Make 'pos' a multiple of 'BUFSIZ' (0 if the file is short), so that all
   //   reads will be on block boundaries, which might increase efficiency

   pos -= nRead;
   lseek(fd, pos, SEEK_SET);
   nRead = read(fd, buffer, nRead);

   if (nRead == -1)
   {
      printf("Error reading %s\n", filename);
      return fail;
   }

   // Count the incomplete line on files that don't end with a newline

   if (nRead && buffer[nRead-1] != '\n')
      --nLines;

   do
   {
      // Scan backward, counting the newlines in this bufferfull

      ssize_t n {nRead};

      while (n)
      {
         const char* nl = (const char*)memrchr(buffer, '\n', n);

         if (!nl)
            break;

         n = nl - buffer;

         if (nLines-- == 0)
         {
            // this newline isn't the last character in the buffer, output the part after

            // printf("%.*s\n", (int)(nRead - (n+2)), nl + 1);
            // lines.insert(lines.begin(), {nl + 1, (size_t)nRead - (n+2)});
            split({nl + 1, (size_t)(nRead-(n+2))}, '\n', &lines);
            return success;
         }
      }

      // Not enough newlines in that buffer

      if (pos == startPos)
      {
         // Not enough lines in the file, print everything from startPos to the end

         lseek(fd, startPos, SEEK_SET);
         readRemaining(fd, endPos, lines);
         return success;
      }

      pos -= BUFSIZ;
      lseek(fd, pos, SEEK_SET);
      nRead = read(fd, buffer, BUFSIZ);

      if (nRead == -1)
      {
         printf("Error reading %s\n", filename);
         return fail;
      }
   }
   while (nRead > 0);

   return success;
}

//*************************************************************************
// Load 'N' Tail Lines From File
//*************************************************************************

int loadTailLinesFromFile(char const* filename, int count, std::vector<std::string>& lines)
{
   int fd = open(filename, O_RDONLY);

   if (!fd)
   {
      printf("Opening %s failed", filename);
      return fail;
   }

   int res = nTailFileLines(fd, filename, count, lines);

   close(fd);

   return res;
}

//*************************************************************************
// Load 'N' Tail Lines From File
//*************************************************************************

bool fctImageSort(FileInfo& v1, FileInfo& v2)
{
   return v1.name < v2.name;
}

void sortFileList(FileList& list)
{
   std::sort(list.begin(), list.end(), fctImageSort);
}

int getFileList(const char* path, int type, const char* extensions, int recursion, FileList* dirs, int& count)
{
   DIR* dir {nullptr};

   tell(eloDebug2, "Scanning directory '%s' with extensions '%s'", path, extensions);

   if (!(dir = opendir(path)))
   {
      tell(eloDetail, "Can't open directory '%s', '%s'", path, strerror(errno));
      return fail;
   }

#ifndef HAVE_READDIR_R
   dirent* pEntry {nullptr};

   while ((pEntry = readdir(dir)))
#else
   dirent entry;
   dirent* pEntry = &entry;
   dirent* res {nullptr};

   // deprecated but the only reentrant with old libc!

   while (readdir_r(dir, pEntry, &res) == 0 && res)
#endif
   {
      // if 'recursion' is set scan subfolders also

      if (recursion && pEntry->d_type == DT_DIR && pEntry->d_name[0] != '.')
      {
         char* buf {nullptr};
         asprintf(&buf, "%s/%s", path, pEntry->d_name);
         getFileList(buf, type, extensions, recursion, dirs, count);
         free(buf);
      }

      // filter type and ignore '.', '..' and hidden files

      if (pEntry->d_type != type || pEntry->d_name[0] == '.')
         continue;

      // filter file extensions

      if (extensions)
      {
         const char* ext {nullptr};

         if ((ext = strrchr(pEntry->d_name, '.')))
            ext++;

         if (isEmpty(ext) || !strcasestr(extensions, ext))
         {
            tell(eloDebug2, "Skipping file '%s' with extension '%s'", pEntry->d_name, ext);
            continue;
         }
      }

      count++;

      if (dirs)
         dirs->push_back({ path, pEntry->d_name, pEntry->d_type });
   }

   closedir(dir);

   return success;
}

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

//***************************************************************************
// gzip
//***************************************************************************

int gzip(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen)
{
    z_stream stream;
    int res;

    stream.next_in = (Bytef *)source;
    stream.avail_in = (uInt)sourceLen;
    stream.next_out = dest;
    stream.avail_out = (uInt)*destLen;
    if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = (voidpf)0;

    res = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);

    if (res == Z_OK)
    {
       res = deflate(&stream, Z_FINISH);

       if (res != Z_STREAM_END)
       {
          deflateEnd(&stream);
          res = res == Z_OK ? Z_BUF_ERROR : res;
       }
    }

    if (res == Z_STREAM_END)
    {
       *destLen = stream.total_out;
       res = deflateEnd(&stream);
    }

    if (res !=  Z_OK)
       tellZipError(res, " during compression", "");

    return res == Z_OK ? success : fail;
}

ulong gzipBound(ulong size)
{
   return compressBound(size);
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
      case Z_MEM_ERROR:    tell(eloAlways, "Error: Not enough memory to unzip file%s!\n", op); return;
      case Z_BUF_ERROR:    tell(eloAlways, "Error: Couldn't unzip data due to output buffer size problem%s!\n", op); return;
      case Z_DATA_ERROR:   tell(eloAlways, "Error: Zipped input data corrupted%s! Details: %s\n", op, msg); return;
      case Z_STREAM_ERROR: tell(eloAlways, "Error: Invalid stream structure%s. Details: %s\n", op, msg); return;
      default:             tell(eloAlways, "Error: Couldn't unzip data for unknown reason (%6d)%s!\n", errorCode, op); return;
   }
}

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
      tell(eloAlways, "getifaddrs() failed");
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
            tell(eloAlways, "getnameinfo() failed: %s", gai_strerror(res));
            return "";
         }

         // skip loopback interface

         if (strcmp(host, "127.0.0.1") == 0)
            continue;

         tell(eloDebug2, "%-8s %-15s %s", ifa->ifa_name, host,
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
      tell(eloAlways, "Converting [%s] from '%s' to '%s' failed", fromPtr, from_code, to_code);
		return fail;
   }

   // printf("------------- converting (%d) '%s' [0x%x0x%x] to '%s'\n", fromLen, in, in[0], in [1], out);

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

//**************************************************************************
//  Regular Expression Searching
//**************************************************************************

int rep(const char* string, const char* expression, Option options)
{
  const char* tmpA;
  const char* tmpB;

  return rep(string, expression, tmpA, tmpB, options);
}

int rep(const char* string, const char* expression, const char*& s_location,
        Option options)
{
  const char* tmpA;

  return rep(string, expression, s_location, tmpA, options);
}


int rep(const char* string, const char* expression, const char*& s_location,
        const char*& e_location, Option options)
{
   regex_t reg;
   regmatch_t rm;
   int status;
   int opt = 0;

   // Vorbereiten von reg fuer die Expressionsuche mit regexec
   // Flags:  REG_EXTENDED = Use Extended Regular Expressions
   //         REG_ICASE    = Ignore case in match.

   reg.re_nsub = 0;

   // Options umwandeln
   if (options & repUseRegularExpression)
     opt = opt | REG_EXTENDED;
   if (options & repIgnoreCase)
     opt = opt | REG_ICASE;

   if (regcomp(&reg, expression, opt) != 0)
      return fail;

   // Suchen des ersten Vorkommens von reg in string

   status = regexec(&reg, string, 1, &rm, 0);
   regfree(&reg);

   if (status != 0)
      return fail;

   // Suche erfolgreich =>
   // Setzen der ermittelten Start- und Endpositionen

   s_location = (char*)(string + rm.rm_so);
   e_location = (char*)(string + rm.rm_eo);

   return success;
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
// cMyMutex
//***************************************************************************

cMyMutex::cMyMutex (void)
{
  locked = 0;
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
  pthread_mutex_init(&mutex, &attr);
}

cMyMutex::~cMyMutex()
{
  pthread_mutex_destroy(&mutex);
}

void cMyMutex::Lock(void)
{
  pthread_mutex_lock(&mutex);
  locked++;
}

void cMyMutex::Unlock(void)
{
 if (!--locked)
    pthread_mutex_unlock(&mutex);
}

//***************************************************************************
// cMyMutexLock
//***************************************************************************

cMyMutexLock::cMyMutexLock(cMyMutex* Mutex)
{
   mutex = NULL;
   locked = false;
   Lock(Mutex);
}

cMyMutexLock::~cMyMutexLock()
{
   if (mutex && locked)
      mutex->Unlock();
}

bool cMyMutexLock::Lock(cMyMutex* Mutex)
{
   if (Mutex && !mutex)
   {
      mutex = Mutex;
      Mutex->Lock();
      locked = true;
      return true;
   }

   return false;
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

//***************************************************************************
// Class LogDuration
//***************************************************************************

LogDuration::LogDuration(const char* aMessage, Eloquence aLogLevel)
{
   logLevel = aLogLevel;
   strcpy(message, aMessage);

   // at last !

   durationStart = cMyTimeMs::Now();
}

LogDuration::~LogDuration()
{
   tell(logLevel, "duration '%s' was (%ldms)",
        message, (long)(cMyTimeMs::Now() - durationStart));
}

void LogDuration::show(const char* label)
{
   tell(logLevel, "elapsed '%s' at '%s' was (%ldms)",
        message, label, (long)(cMyTimeMs::Now() - durationStart));
}

//***************************************************************************
// Get Unique ID
//***************************************************************************

#ifdef USEUUID
const char* getUniqueId()
{
   static char uuid[sizeUuid+TB] = "";

   uuid_t id;
   uuid_generate(id);
   uuid_unparse_upper(id, uuid);

   return uuid;
}
#endif // USEUUID
