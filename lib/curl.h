/*
 * curlfuncs.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __LIB_CURL__
#define __LIB_CURL__

#include <curl/curl.h>
#include <curl/easy.h>

#include <string>

#include "common.h"
//#include "config.h"
//#include "configuration.h"

#define CURL_USERAGENT "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; Mayukh's libcurl wrapper http://www.mayukhbose.com/)"

//***************************************************************************
// CURL
//***************************************************************************

class cCurl
{
   public:

      cCurl();
      ~cCurl();

      int init(const char* httpproxy = "");
      int exit();

      static int create();
      static int destroy();

      int GetUrl(const char *url, std::string *sOutput, const std::string &sReferer="");
      int GetUrlFile(const char *url, const char *filename, const std::string &sReferer="");
      int SetCookieFile(char *filename);
      int postUrl(const char *url, const std::string &sPost, std::string *sOutput, const std::string &sReferer = "");
      int postRaw(const char *url, const std::string &sPost, std::string *sOutput, const std::string &sReferer = "");
      int doPost(const char* url, std::string* sOutput, const std::string& sReferer,
                 curl_mime* multipart, struct curl_slist* headerlist);

      int post(const char* url, const std::string& sPost, std::string* sOutput);
      int put(const char* url, const std::string& sPost, std::string* sOutput);
      char* EscapeUrl(const char *url);
      void Free(char* str);

      int downloadFile(const char* url, int& size, MemoryStruct* data, int timeout = 30,
                       const char* userAgent = CURL_USERAGENT, struct curl_slist* headerlist = 0);

      // static stuff

      // static void setSystemNotification(cSystemNotification* s) { sysNotification = s; }
      static std::string sBuf;   // dirty

   protected:

      // data

      CURL* handle;

   // static cSystemNotification* sysNotification;

      // statics

      static size_t WriteMemoryCallback(void* ptr, size_t size, size_t nmemb, void* data);
      static size_t WriteHeaderCallback(char* ptr, size_t size, size_t nmemb, void* data);

      static int curlInitialized;
};

extern cCurl curl;

//***************************************************************************
#endif // __LIB_CURL__
