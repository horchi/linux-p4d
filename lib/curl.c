/*
 * curlfuncs.cpp
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "curl.h"

#define MY_USERAGENT "libcurl-agent/1.0"

//***************************************************************************
// Singelton
//***************************************************************************

cCurl curl;

std::string cCurl::sBuf = "";
int cCurl::curlInitialized = no;
//cSystemNotification* cCurl::sysNotification = 0;

//***************************************************************************
// Callbacks
//***************************************************************************

size_t collect_data(void *ptr, size_t size, size_t nmemb, void* stream)
{
   std::string sTmp;
   size_t actualsize = size * nmemb;

   if (!stream)
   {
      sTmp.assign((char *)ptr, actualsize);
      cCurl::sBuf += sTmp;
   }
   else
   {
      fwrite(ptr, size, nmemb, (FILE *)stream);
   }

   return actualsize;
}

//***************************************************************************
// Object
//***************************************************************************

cCurl::cCurl()
{
   handle = 0;
}

cCurl::~cCurl()
{
   exit();
}

//***************************************************************************
// Create / Destroy
//***************************************************************************

int cCurl::create()
{
   if (!curlInitialized)
   {
      // call only once per process and *before* any thread is started!

      if (curl_global_init(CURL_GLOBAL_NOTHING /*CURL_GLOBAL_ALL*/) != 0)
      {
         tell(eloAlways, "Error, something went wrong with curl_global_init()");
         return fail;
      }

      curlInitialized = yes;
   }

   return done;
}

int cCurl::destroy()
{
   if (curlInitialized)
      curl_global_cleanup();

   curlInitialized = no;

   return done;
}

//***************************************************************************
// Init / Exit
//***************************************************************************

int cCurl::init(const char* httpproxy)
{
   if (!handle)
   {
      if (!(handle = curl_easy_init()))
      {
         tell(eloAlways, "Could not create new curl instance");
         return fail;
      }
   }
   else
   {
      curl_easy_reset(handle);
   }

   // Reset Options

   if (!isEmpty(httpproxy))
   {
      curl_easy_setopt(handle, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
      curl_easy_setopt(handle, CURLOPT_PROXY, httpproxy);   // Specify HTTP proxy
   }

   curl_easy_setopt(handle, CURLOPT_HTTPHEADER, 0);
   curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, collect_data);
   curl_easy_setopt(handle, CURLOPT_WRITEDATA, 0);                        // Set option to write to string
   curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, yes);
   curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, 0);                   // Send header to this function
   curl_easy_setopt(handle, CURLOPT_WRITEHEADER, 0);                      // Pass some header details to this struct
   curl_easy_setopt(handle, CURLOPT_MAXFILESIZE, 100*1024*1024);          // Set maximum file size to get (bytes)
   curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1);                       // No progress meter
   curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);                         // No signaling
   curl_easy_setopt(handle, CURLOPT_TIMEOUT, 30);                         // Set timeout
   curl_easy_setopt(handle, CURLOPT_NOBODY, 0);                           //
   curl_easy_setopt(handle, CURLOPT_USERAGENT, CURL_USERAGENT);           // Some servers don't like requests
   curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "");

   return success;
}

int cCurl::exit()
{
   if (handle)
      curl_easy_cleanup(handle);

   handle = 0;

   return done;
}

//***************************************************************************
// Get Url
//***************************************************************************

int cCurl::GetUrl(const char *url, std::string *sOutput, const std::string &sReferer)
{
  CURLcode res;

  init();

  curl_easy_setopt(handle, CURLOPT_URL, url);            // Set the URL to get

  if (sReferer != "")
    curl_easy_setopt(handle, CURLOPT_REFERER, sReferer.c_str());

  curl_easy_setopt(handle, CURLOPT_HTTPGET, yes);
  curl_easy_setopt(handle, CURLOPT_FAILONERROR, yes);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, 0);       // Set option to write to string
  sBuf = "";

   if ((res = curl_easy_perform(handle)) != CURLE_OK)
  {
      long httpCode = 0;

      curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &httpCode);
      tell(eloAlways, "Error: Getting URL failed; %s (%d); http code was (%ld) [%s]",
           curl_easy_strerror(res), res, httpCode, url);

     *sOutput = "";

     return 0;
  }

  *sOutput = sBuf;

  return 1;
}

int cCurl::GetUrlFile(const char *url, const char *filename, const std::string &sReferer)
{
   int nRet = 0;

   init();

   // Point the output to a file

   FILE *fp;

   if ((fp = fopen(filename, "w")) == NULL)
      return 0;

   curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);       // Set option to write to file
   curl_easy_setopt(handle, CURLOPT_URL, url);            // Set the URL to get

   if (sReferer != "")
      curl_easy_setopt(handle, CURLOPT_REFERER, sReferer.c_str());

   curl_easy_setopt(handle, CURLOPT_HTTPGET, yes);

   if (curl_easy_perform(handle) == 0)
      nRet = 1;
   else
      nRet = 0;

   curl_easy_setopt(handle, CURLOPT_WRITEDATA, NULL);     // Set option back to default (string)
   fclose(fp);

   return nRet;
}

int cCurl::postUrl(const char* url, const std::string &sPost, std::string* sOutput, const std::string &sReferer)
{
   init();

   int retval {1};
   std::string::size_type nStart {0}, nEnd {0}, nPos {0};
   std::string sTmp, sName, sValue;
   struct curl_slist* headerlist {};

   curl_mime* multipart {curl_mime_init(handle)};
   curl_mimepart* part {};

   // Add the POST variables here

   while ((nEnd = sPost.find("##", nStart)) != std::string::npos)
   {
      sTmp = sPost.substr(nStart, nEnd - nStart);

      if ((nPos = sTmp.find("=")) == std::string::npos)
         return 0;

      sName = sTmp.substr(0, nPos);
      sValue = sTmp.substr(nPos+1);

      part = curl_mime_addpart(multipart);
      curl_mime_name(part, sName.c_str());
      curl_mime_data(part, sValue.c_str(), CURL_ZERO_TERMINATED);

      nStart = nEnd + 2;
   }
   sTmp = sPost.substr(nStart);

   if ((nPos = sTmp.find("=")) == std::string::npos)
      return 0;

   sName = sTmp.substr(0, nPos);
   sValue = sTmp.substr(nPos+1);

   part = curl_mime_addpart(multipart);
   curl_mime_name(part, sName.c_str());
   curl_mime_data(part, sValue.c_str(), CURL_ZERO_TERMINATED);

   retval = curl.doPost(url, sOutput, sReferer, multipart, headerlist);
   curl_mime_free(multipart);
   curl_slist_free_all(headerlist);

   return retval;
}

int cCurl::postRaw(const char* url, const std::string &sPost, std::string* sOutput, const std::string &sReferer)
{
  init();

  int retval {0};
  curl_mime* multipart = curl_mime_init(handle);
  struct curl_slist* headerlist {};

  curl_easy_setopt(handle, CURLOPT_POSTFIELDS, sPost.c_str());
  curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, 0); //FIXME: Should this be the size instead, in case this is binary string?

  retval = curl.doPost(url, sOutput, sReferer, multipart, headerlist);

  curl_mime_free(multipart);
  curl_slist_free_all(headerlist);
  return retval;
}

int cCurl::doPost(const char* url, std::string* sOutput, const std::string& sReferer,
                  curl_mime* multipart, struct curl_slist* headerlist)
{
   headerlist = curl_slist_append(headerlist, "Expect:");

   // Now do the form post

   curl_easy_setopt(handle, CURLOPT_URL, url);

   if (sReferer != "")
      curl_easy_setopt(handle, CURLOPT_REFERER, sReferer.c_str());

   curl_easy_setopt(handle, CURLOPT_MIMEPOST, multipart);
   curl_easy_setopt(handle, CURLOPT_WRITEDATA, sOutput); // Set option to write to string

   if (curl_easy_perform(handle) == 0)
      return 1;

   // We have an error here mate!

   *sOutput = "";

   return 0;
}

int cCurl::SetCookieFile(char *filename)
{
   init();

   if (curl_easy_setopt(handle, CURLOPT_COOKIEFILE, filename) != 0)
      return 0;
   if (curl_easy_setopt(handle, CURLOPT_COOKIEJAR, filename) != 0)
      return 0;

   return 1;
}

char* cCurl::EscapeUrl(const char *url)
{
   init();
   return curl_easy_escape(handle, url , strlen(url));
}

void cCurl::Free(char* str)
{
   curl_free(str);
}

//***************************************************************************
// Callbacks
//***************************************************************************

size_t cCurl::WriteMemoryCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
   size_t realsize = size * nmemb;
   MemoryStruct* mem = (MemoryStruct*)data;

   // if (sysNotification)
   //    sysNotification->check();

   if (mem->memory)
      mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
   else
      mem->memory = (char*)malloc(mem->size + realsize + 1);

   if (mem->memory)
   {
      memcpy (&(mem->memory[mem->size]), ptr, realsize);
      mem->size += realsize;
      mem->memory[mem->size] = 0;
   }

   return realsize;
}

size_t cCurl::WriteHeaderCallback(char* ptr, size_t size, size_t nmemb, void* data)
{
   size_t realsize = size * nmemb;
   MemoryStruct* mem = (MemoryStruct*)data;
   char* p;

   // if (sysNotification)
   //    sysNotification->check();

   if (ptr)
   {
      // add to Header to map

      std::string header(ptr);
      std::size_t pos = header.find(": ");

      if(pos != std::string::npos)
      {
         std::string name = header.substr(0, pos);
         std::string value = header.substr(pos+2, std::string::npos);
         mem->headers[name] = value;
      }

      // get filename
      {
         // Content-Disposition: attachment; filename="20140103_20140103_de_qy.zip"

         const char* attribute = "Content-disposition: ";

         if ((p = strcasestr((char*)ptr, attribute)))
         {
            if ((p = strcasestr(p, "filename=")))
            {
               p += strlen("filename=");

               tell(eloDebug, "found filename at [%s]", p);

               if (*p == '"')
                  p++;

               sprintf(mem->name, "%s", p);

               if ((p = strchr(mem->name, '\n')))
                  *p = 0;

               if ((p = strchr(mem->name, '\r')))
                  *p = 0;

               if ((p = strchr(mem->name, '"')))
                  *p = 0;

               tell(eloDebug, "set name to '%s'", mem->name);
            }
         }
      }

      // since some sources update "ETag" an "Last-Modified:" without changing the contents
      //  we have to use "Content-Length:" to check for updates :(
      {
         const char* attribute = "Content-Length: ";

         if ((p = strcasestr((char*)ptr, attribute)))
         {
            sprintf(mem->tag, "%s", p+strlen(attribute));

            if ((p = strchr(mem->tag, '\n')))
               *p = 0;

            if ((p = strchr(mem->tag, '\r')))
               *p = 0;

            if ((p = strchr(mem->tag, '"')))
               *p = 0;
         }
      }
   }

   return realsize;
}

//***************************************************************************
// Download File
//***************************************************************************

int cCurl::downloadFile(const char* url, int& size, MemoryStruct* data, int timeout,
                        const char* userAgent, struct curl_slist* headerlist)
{
   long code {0};
   CURLcode res {CURLE_OK};

   size = 0;

   init();

   curl_easy_setopt(handle, CURLOPT_URL, url);                                // Specify URL to get
   curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, yes);
   curl_easy_setopt(handle, CURLOPT_UNRESTRICTED_AUTH, yes);                  // continue to send authentication (user+password) when following locations
   curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);      // Send all data to this function
   curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)data);                  // Pass our 'data' struct to the callback function
   curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);     // Send header to this function
   curl_easy_setopt(handle, CURLOPT_WRITEHEADER, (void*)data);                // Pass some header details to this struct
   curl_easy_setopt(handle, CURLOPT_MAXFILESIZE, 100*1024*1024);              // Set maximum file size to get (bytes)
   curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1);                           // No progress meter
   curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);                             // No signaling
   curl_easy_setopt(handle, CURLOPT_TIMEOUT, timeout);                        // Set timeout
   curl_easy_setopt(handle, CURLOPT_NOBODY, data->headerOnly ? 1 : 0);        //
   curl_easy_setopt(handle, CURLOPT_USERAGENT, userAgent);                    // Some servers don't like requests without a user-agent field
   curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "gzip");                 //

   if (headerlist)
       curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headerlist);

   // perform http-get

   if ((res = curl_easy_perform(handle)) != 0)
   {
      long httpCode = 0;

      curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &httpCode);
      tell(eloAlways, "Error: Download failed, got %ld bytes; %s (%d); http code was (%ld) [%s]",
           data->size, curl_easy_strerror(res), res, httpCode, url);
      tell(eloAlways, "Error, download failed; %s (%d)", curl_easy_strerror(res), res);
      data->clear();
      exit();
      return fail;
   }

   curl_easy_getinfo(handle, CURLINFO_HTTP_CODE, &code);
   data->statusCode = code;

   if (code == 400 || code == 404 || code == 500)
   {
      data->clear();
      exit();
      return fail;
   }

   size = data->size;

   return success;
}

//***************************************************************************
// Post
//***************************************************************************

int cCurl::post(const char* url, const std::string& sPost, std::string* sOutput)
{
   init();

   curl_slist* headers {nullptr};
   headers = curl_slist_append(headers, "Accept: application/json");
   headers = curl_slist_append(headers, "Content-Type: application/json");
   headers = curl_slist_append(headers, "charset: utf-8");

   curl_easy_setopt(handle, CURLOPT_URL, url);
   curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "POST");
   curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
   curl_easy_setopt(handle, CURLOPT_POSTFIELDS, sPost.c_str());
   curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcrp/0.1");

   // CURLcode res = curl_easy_perform(handle);

   sBuf = "";

   if (curl_easy_perform(handle) == 0)
   {
      *sOutput = sBuf;
      curl_slist_free_all(headers);
      return success;
   }

   *sOutput = "";
   curl_slist_free_all(headers);

   return fail;
}

//***************************************************************************
// Put
//***************************************************************************

int cCurl::put(const char* url, const std::string& sPost, std::string* sOutput)
{
   init();

   curl_slist* headers {nullptr};

   headers = curl_slist_append(headers, "Accept: application/json");
   headers = curl_slist_append(headers, "Content-Type: application/json");
   headers = curl_slist_append(headers, "charset: utf-8");

   curl_easy_setopt(handle, CURLOPT_URL, url);
   curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "PUT");
   curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
   curl_easy_setopt(handle, CURLOPT_POSTFIELDS, sPost.c_str());
   curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcrp/0.1");

   // CURLcode res = curl_easy_perform(handle);

   sBuf = "";

   if (curl_easy_perform(handle) == 0)
   {
      *sOutput = sBuf;
      curl_slist_free_all(headers);
      return success;
   }

   *sOutput = "";
   curl_slist_free_all(headers);

   return fail;
}

//***************************************************************************
// Callbacks
//***************************************************************************

static size_t WriteMemoryCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
   size_t realsize = size * nmemb;
   struct MemoryStruct* mem = (struct MemoryStruct*)data;

   if (mem->memory)
      mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
   else
      mem->memory = (char*)malloc(mem->size + realsize + 1);

   if (mem->memory)
   {
      memcpy (&(mem->memory[mem->size]), ptr, realsize);
      mem->size += realsize;
      mem->memory[mem->size] = 0;
   }

   return realsize;
}

static size_t WriteHeaderCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
   size_t realsize = size * nmemb;
   struct MemoryStruct* mem = (struct MemoryStruct*)data;
   char* p;

   if (ptr)
   {
      // get filename
      {
         const char* attribute = "Content-disposition: ";

         if ((p = strcasestr((char*)ptr, attribute)))
         {
            if ((p = strcasestr(p, "filename=")))
            {
               sprintf(mem->name, "%s", p+strlen("filename="));

               if ((p = strchr(mem->name, '\n')))
                  *p = 0;

               if ((p = strchr(mem->name, '\r')))
                  *p = 0;

               if ((p = strchr(mem->name, '"')))
                  *p = 0;
            }
         }
      }

      // since some sources update "ETag" an "Last-Modified:" without changing the contents
      //  we have to use "Content-Length:" to check for updates :(
      {
         const char* attribute = "Content-Length: ";

         if ((p = strcasestr((char*)ptr, attribute)))
         {
            sprintf(mem->tag, "%s", p+strlen(attribute));

            if ((p = strchr(mem->tag, '\n')))
               *p = 0;

            if ((p = strchr(mem->tag, '\r')))
               *p = 0;

            if ((p = strchr(mem->tag, '"')))
               *p = 0;
         }
      }
   }

   return realsize;
}

//***************************************************************************
// Download File
//***************************************************************************

int downloadFile(const char* url, MemoryStruct* data, int timeout, const char* httpproxy)
{
   CURL* curl_handle;
   long code;
   CURLcode res = CURLE_OK;

   // init curl

   if (curl_global_init(CURL_GLOBAL_ALL) != 0)
   {
      tell(eloAlways, "Error, something went wrong with curl_global_init()");
      return fail;
   }

   curl_handle = curl_easy_init();

   if (!curl_handle)
   {
      tell(eloAlways, "Error, unable to get handle from curl_easy_init()");
      return fail;
   }

   if (!isEmpty(httpproxy))
   {
      curl_easy_setopt(curl_handle, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
      curl_easy_setopt(curl_handle, CURLOPT_PROXY, httpproxy);                 // Specify HTTP proxy
   }

   curl_easy_setopt(curl_handle, CURLOPT_URL, url);                            // Specify URL to get
   curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 0);                   // don't follow redirects
   curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);  // Send all data to this function
   curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)data);              // Pass our 'data' struct to the callback function
   curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback); // Send header to this function
   curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, (void*)data);            // Pass some header details to this struct
   curl_easy_setopt(curl_handle, CURLOPT_MAXFILESIZE, 100*1024*1024);          // Set maximum file size to get (bytes)
   curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1);                       // No progress meter
   curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);                         // No signaling
   curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);                    // Set timeout
   curl_easy_setopt(curl_handle, CURLOPT_NOBODY, data->headerOnly ? 1 : 0);    //
   curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, MY_USERAGENT);            // Some servers don't like requests
                                                                               // that are made without a user-agent field
   // perform http-get

   if ((res = curl_easy_perform(curl_handle)) != 0)
   {
      data->clear();

      tell(eloDetail, "Error, download failed; %s (%d)", curl_easy_strerror(res), res);
      curl_easy_cleanup(curl_handle);  // Cleanup curl stuff

      return fail;
   }

   curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &code);
   curl_easy_cleanup(curl_handle);     // cleanup curl stuff

   if (code == 404)
   {
      data->clear();
      return fail;
   }

   return success;
}
