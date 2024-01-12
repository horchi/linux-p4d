/*
 * lmccom.c
 *
 * See the README file for copyright information
 *
 */

#include "lib/common.h"

#include "lmccom.h"
#include "lmctag.h"

//***************************************************************************
// Object
//***************************************************************************

LmcCom::LmcCom(const char* aMac)
   : TcpChannel()
{
   // init curl, used only for (de)escape the url encoding

   curl_global_init(CURL_GLOBAL_NOTHING);
   curl = curl_easy_init();

   if (!isEmpty(aMac))
   {
      mac = strdup(aMac);
      escId = escape(mac);
   }
}

LmcCom::~LmcCom()
{
   if (notify)
      stopNotify();

   curl_easy_cleanup(curl);
   close();
   free(host);
   free(mac);
   free(escId);
   free(queryTitle);
}

//***************************************************************************
// Open
//***************************************************************************

int LmcCom::open(const char* aHost, unsigned short aPort)
{
   LmcLock;

   free(host);
   port = aPort;
   host = strdup(aHost);

   tell(eloAlways, "[LMC] Try connecting LMC server at '%s:%d", host, port);
   int res = TcpChannel::open(port, host);

   if (res == success)
      tell(eloAlways, "[LMC] Connection to LMC server established");

   return res;
}

//***************************************************************************
// Update Current Playlist
//***************************************************************************

int LmcCom::update(int stateOnly)
{
   LmcLock;

   const int maxValue = 10000;
   char* value = 0;
   int tag;

   LmcTag lt(this);
   char cmd[100] {};
   int status {success};
   int count {0};
   TrackInfo t;
   bool track {false};
   char* buf {};

   playerState = {};
   tracks.clear();

   status += queryInt("playlist tracks", count);
   query("version", playerState.version,  sizeof(playerState.version));
   queryInt("mixer muting", playerState.muted);

   if (status != success)
      return status;

   if (!stateOnly)
      count = std::max(count, 100);

   char* param = escape("tags:agdluyKJNxrow");
   sprintf(cmd, "status 0 %d %s", count, param);
   free(param);

   // perform LMC request ..

   LmcDoLock;
   status = request(cmd);
   status += write("\n");
   status += responseP(buf);
   LmcUnLock;

   if (status != success)
   {
      tell(eloAlways, "[LMC] Error: Request of '%s' failed", cmd);
      free(buf);
      return fail;
   }

   lt.set(buf);
   free(buf);

   t.index = na;
   value = (char*)malloc(maxValue+TB);

   while (lt.getNext(tag, value, maxValue, track) != LmcTag::wrnEndOfPacket)
   {
      switch (tag)
      {
         case LmcTag::tTime:             playerState.trackTime = atoi(value);  playerState.updatedAt = cTimeMs::Now(); break;
         case LmcTag::tMixerVolume:      playerState.volume = atoi(value);     break;
         case LmcTag::tPlaylistCurIndex: playerState.plIndex  = atoi(value);   break;
         case LmcTag::tPlaylistShuffle:  playerState.plShuffle = atoi(value);  break;
         case LmcTag::tPlaylistRepeat:   playerState.plRepeat = atoi(value);   break;
         case LmcTag::tMode:             snprintf(playerState.mode, sizeof(playerState.mode), "%s", value);     break;
         case LmcTag::tPlaylistName:     snprintf(playerState.plName, sizeof(playerState.plName), "%s", value); break;

         // playlist tags ...

         case LmcTag::tPlaylistIndex:
         {
            if (t.index != na)
            {
               t.updatedAt = cTimeMs::Now();
               tracks.push_back(t);
               // memset(&t, 0, sizeof(t));
               t = {};
            }

            track = true;
            t.index = atoi(value);
            break;
         }

         case LmcTag::tId:             t.id = atoi(value);                                          break;
         case LmcTag::tYear:           t.year = atoi(value);                                        break;
         case LmcTag::tTitle:          snprintf(t.title, sizeof(t.title), "%s", value);             break;
         case LmcTag::tArtist:         snprintf(t.artist, sizeof(t.artist), "%s", value);           break;
         case LmcTag::tGenre:          snprintf(t.genre, sizeof(t.genre), "%s", value);             break;
         case LmcTag::tTrackDuration:  t.duration = atoi(value);                                    break;
         case LmcTag::tArtworkTrackId: snprintf(t.artworkTrackId, sizeof(t.artworkTrackId), "%s", value); break;
         case LmcTag::tArtworkUrl:     snprintf(t.artworkurl, sizeof(t.artworkurl), "%s", value);   break;
         case LmcTag::tAlbum:          snprintf(t.album, sizeof(t.album), "%s", value);             break;
         case LmcTag::tRemoteTitle:    snprintf(t.remoteTitle, sizeof(t.remoteTitle), "%s", value); break;
         case LmcTag::tContentType:    snprintf(t.contentType, sizeof(t.contentType), "%s", value); break;
         case LmcTag::tLyrics:         snprintf(t.lyrics, sizeof(t.lyrics), "%s", value);           break;
         case LmcTag::tRemote:         t.remote = atoi(value);                                      break;
         case LmcTag::tBitrate:        t.bitrate = atoi(value);                                     break;

         // case LmcTag::tUrl:         snprintf(t.url, sizeof(t.url), "%s", url);                   break;
      }
   }

   free(value);

   if (t.index != na)
   {
      t.updatedAt = cTimeMs::Now();
      tracks.push_back(t);
   }

   playerState.plCount = tracks.size();
   tell(eloLmc, "[LMC] Playlist updated, got %d track", playerState.plCount);

   return success;
}

//***************************************************************************
// Query
//***************************************************************************

int LmcCom::query(const char* command, char* result, int max)
{
   LmcLock;

   int status;

   setQueryTitle(command);
   status = request(command);
   status += write(" ?\n");

   if ((status += response(result, max)) != success)
       tell(eloAlways, "[LMC] Error: Request of '%s' failed", command);

   unescape(result);

   return status;
}

int LmcCom::queryInt(const char* command, int& value)
{
   LmcLock;

   int status {success};
   char result[100+TB] {};

   status = request(command);
   status += write(" ?\n");
   status += response(result, 100);

   unescape(result);

   if (status == success)
      value = atol(result);

   return status;
}

//***************************************************************************
// Query Range
//***************************************************************************

int LmcCom::queryRange(RangeQueryType queryType, int from, int count,
                       RangeList* list, int& total, const char* special, Parameters* pars)
{
   LmcLock;

   char query[200] {};
   char cmd[250] {};

   const int maxValue {100};
   char value[maxValue+TB] {};
   int tag {0};
   char* result {};
   LmcTag lt(this);
   int status {success};
   ListItem item;
   int firstTag {LmcTag::tId};

   total = 0;
   list->clear();

   switch (queryType)
   {
      case rqtGenres:    sprintf(query, "genres");            break;
      case rqtArtists:   sprintf(query, "artists");           break;
      case rqtAlbums:    sprintf(query, "albums");            break;
      case rqtNewMusic:  sprintf(query, "albums");            break;
      case rqtTracks:    sprintf(query, "tracks");            break;
      case rqtPlaylists: sprintf(query, "playlists");         break;
      case rqtFavorites: sprintf(query, "favorites items");   break;
      case rqtRadioApps: sprintf(query, "%s items", special); break;

      case rqtYears:
         sprintf(query, "years");
         firstTag = LmcTag::tYear;
         break;

      case rqtRadios:
         sprintf(query, "radios");
         firstTag = LmcTag::tIcon;
         break;

      case rqtPlayers:
         sprintf(query, "players");
         firstTag = LmcTag::tPlayerindex;
         break;

      default: break;
   }

   if (isEmpty(query))
      return fail;

   setQueryTitle(query);

   snprintf(cmd, 250, "%s %d %d", query, from, count);
   status = request(cmd, pars);
   status += write("\n");

   if ((status += responseP(result)) != success || isEmpty(result))
   {
      free(result);
      if (status != success)
         tell(eloAlways, "[LMC] Error: Request of '%s' failed", cmd);
      return status;
   }

   lt.set(result);
   tell(eloDebugLmc, "[LMC] Got [%s]", unescape(result));
   free(result); result = 0;

   while (lt.getNext(tag, value, maxValue) != LmcTag::wrnEndOfPacket)
   {
      if (tag == LmcTag::tItemCount)
      {
         total = atoi(value);
         continue;
      }

      if (tag == firstTag && !item.isEmpty())
      {
         list->push_back(item);
         item.clear();
      }

      if (tag == LmcTag::tId)
      {
         item.id = value;
         continue;
      }

      switch (queryType)
      {
         case rqtGenres:
         {
            item.isAudio = yes;
            if (tag == LmcTag::tGenre)   item.content = value;
            break;
         }
         case rqtArtists:
         {
            item.isAudio = yes;
            if (tag == LmcTag::tArtist)  item.content = value;
            break;
         }
         case rqtNewMusic:
         case rqtAlbums:
         {
            item.isAudio = yes;
            if (tag == LmcTag::tAlbum)   item.content = value;
            break;
         }
         case rqtTracks:
         {
            item.isAudio = yes;
            if (tag == LmcTag::tTitle)   item.content = value;
            break;
         }
         case rqtFavorites:
            item.command = "favorites";
         case rqtRadioApps:
         {
            if      (tag == LmcTag::tName)     item.content = value;
            else if (tag == LmcTag::tTitle)    setQueryTitle(value);
            else if (tag == LmcTag::tHasItems) item.hasItems = atoi(value);
            else if (tag == LmcTag::tIsAudio)  item.isAudio = atoi(value);

            break;
         }
         case rqtYears:
         {
            item.isAudio = yes;
            if (tag == LmcTag::tYear)  item.content = value;

            break;
         }
         case rqtPlaylists:
         {
            item.isAudio = yes;
            if (tag == LmcTag::tPlaylist && strcmp(value, escId) != 0)
               item.content = value;
            break;
         }
         case rqtRadios:
         {
            if (tag == LmcTag::tCmd)          item.command = value;
            else if (tag == LmcTag::tTitle)   setQueryTitle(value);
            else if (tag == LmcTag::tName)    item.content = value;
            break;
         }
         case rqtPlayers:
         {
            // tell(eloDebugLmc, "Got '%s : %s'", LmcTag::toName(tag), value);
            if      (tag == LmcTag::tPlayerindex) item.id = value;
            else if (tag == LmcTag::tPlayerid)    item.content = value;
            else if (tag == LmcTag::tName)        item.command = value;
            else if (tag == LmcTag::tConnected)   item.isConnected = atoi(value);

            break;
         }

         default: break;
      };
   }

   if (!item.isEmpty())
   {
      list->push_back(item);
      item.clear();
   }

   // #TODO list->sort();

   return success;
}

//***************************************************************************
// Execute
//***************************************************************************

int LmcCom::execute(const char* command, Parameters* pars)
{
   LmcLock;

   tell(eloLmc, "[LMC] Exectuting '%s' with %zu parameters", command, pars ? pars->size() : 0);
   request(command, pars);
   write("\n");

   return response();
}

int LmcCom::execute(const char* command, const char* par)
{
   LmcLock;

   tell(eloLmc, "[LMC] Exectuting '%s' with '%s'", command, par);
   request(command, par);
   write("\n");

   return response();
}

int LmcCom::execute(const char* command, int par)
{
   LmcLock;
   char str[10] {};

   sprintf(str, "%d", par);

   return execute(command, str);
}

//***************************************************************************
// Request
//***************************************************************************

int LmcCom::request(const char* command, const char* par)
{
   Parameters pars;

   pars.push_back(par);

   return request(command, &pars);
}

//***************************************************************************
// Request
//***************************************************************************

int LmcCom::request(const char* command, Parameters* pars)
{
   int status {success};

   snprintf(lastCommand, sizeMaxCommand, "%s", command);
   lastPar = "";

   if (pars)
   {
      LmcCom::Parameters::iterator it;

      for (it = pars->begin(); it != pars->end(); ++it)
      {
         char* p = escape((*it).c_str());
         lastPar += std::string(p) + " ";
         free(p);
      }
   }

   tell(eloDebugLmc, "[LMC] Requesting '%s' with '%s'", lastCommand, lastPar.c_str());
   flush();

   status = write(escId)
      + write(" ")
      + write(lastCommand);

   if (lastPar != "")
      status += write(" ")
         + write(lastPar.c_str());

   return status;
}

//***************************************************************************
// Response
//***************************************************************************

int LmcCom::responseP(char*& result)
{
   int status {fail};
   char* buf {};

   result = 0;

   // wait op to 30 seconds to receive answer ..

   if (look(30000) == success && (buf = readln()))
   {
      char* p = buf + strlen(escId) +1;

      if ((p = strstr(p, lastCommand)) && strlen(p) >= strlen(lastCommand))
      {
         p += strlen(lastCommand);

         while (*p && *p == ' ')         // skip leading blanks
            p++;

         if (strcmp(p, "?") != 0)
            result = strdup(p);

         tell(eloDebugLmc, "[LMC] <- (response %zu bytes) [%s]", strlen(buf), unescape(p));
         status = success;
      }
      else
      {
         tell(eloAlways, "[LMC] Got unexpected answer for '%s' [%s]", lastCommand, buf);
         status = fail;
      }
   }

   free(buf);

   return status;
}

//***************************************************************************
// Response
//***************************************************************************

int LmcCom::response(char* result, int max)
{
   // LogDuration ld("response", 0);

   int status {success};
   char* buf {};

   if (result)
      *result = 0;

   // wait op to 30 seconds to receive answer ..

   if (look(30000) == success && (buf = readln()))
   {
      char* p = buf + strlen(escId) +1;

      if ((p = strstr(p, lastCommand)) && strlen(p) >= strlen(lastCommand))
      {
         p += strlen(lastCommand);

         while (*p && *p == ' ')         // skip leading blanks
            p++;

         if (result && max && strcmp(p,"?") != 0)
            sprintf(result, "%.*s", max, p);

         tell(eloDebugLmc, "[LMC] <- (response %zu bytes) [%s]", strlen(buf), unescape(p));
      }
      else
      {
         tell(eloAlways, "[LMC] Got unexpected answer for '%s' [%s]", lastCommand, buf);
         status = fail;
      }
   }

   free(buf);

   return status;
}

//***************************************************************************
// (Un)escape URL Encoding
//***************************************************************************

char* LmcCom::unescape(char* buf)
{
   int len {0};
   char* res {};

   res = curl_easy_unescape(curl , buf, strlen(buf), &len);

   if (res)
      strcpy(buf, res);

   free(res);

   return buf;
}

char* LmcCom::escape(const char* buf)
{
   char* res {};

   res = curl_easy_escape(curl , buf, strlen(buf));

   return res;
}

//***************************************************************************
// Notification
//***************************************************************************

int LmcCom::startNotify()
{
   notify = new LmcCom(mac);

   if (notify->open(host, port) != success)
   {
      delete notify;
      notify = 0;
      return fail;
   }

   tell(eloAlways, "[LMC] Starting notification channel");

   return notify->execute("listen 1");
}

int LmcCom::stopNotify()
{
   if (notify)
   {
      notify->execute("listen 0");
      notify->close();
      delete notify;
      notify = 0;

      tell(eloAlways, "[LMC] Stopped notification channel");
   }

   return success;
}

//***************************************************************************
// Check for Notification
//***************************************************************************

int LmcCom::checkNotify(uint64_t timeout)
{
   char buf[1000+TB] {};
   int status {wrnNoEventPending};
   static time_t checkPlayersNext {time(0)};

   // check if 'my' player is connected

   if (time(0) >= checkPlayersNext)
   {
      checkPlayersNext = time(0) + 10;

      bool myPlayerConnected {false};
      LmcCom::Parameters filters;
      LmcCom::RangeList list;
      int total {0};

      if (queryRange(LmcCom::rqtPlayers, 0, 20, &list, total, "", &filters) == success)
      {
         tell(eloDebugLmc, "[LMC] Query players succeeded (%d)", total);

         LmcCom::RangeList::iterator it;

         for (it = list.begin(); it != list.end(); ++it)
         {
            tell(eloLmc, "[LMC] Player %s) %s '%s'", (*it).id.c_str(), (*it).command.c_str(), (*it).content.c_str());

            if ((*it).content == mac && (*it).isConnected)
               myPlayerConnected = true;
         }

         if (notify && !myPlayerConnected)
            stopNotify();
         else if (!notify && myPlayerConnected)
            startNotify();
      }
   }

   metaDataChanged = false;

   if (!notify)
   {
      // tell(eloLmc, "[LMC] Cant check for notifications until startNotify is called");
      return fail;
   }

   while (notify->look(timeout) == success)
   {
      if (notify->read(buf, 1000, yes) == success)
      {
         buf[strlen(buf)-1] = 0;   // cut LF
         unescape(buf);
         tell(eloDebugLmc, "[LMC] <- [%s]", buf);

         if (strstr(buf, "playlist "))
            status = success;
         else if (strstr(buf, "pause ") || strstr(buf, "server"))
            status = success;
         else if (strstr(buf, "newmetadata"))
         {
            metaDataChanged = true;
            status = success;
         }
      }
   }

   if (status == success)
      update();

   return status;
}

//***************************************************************************
// Get Current Cover
//***************************************************************************

int LmcCom::getCurrentCover(MemoryStruct* cover, TrackInfo* track)
{
   char* url {};
   int status {fail};

   if (track && !isEmpty(track->artworkurl))
   {
      asprintf(&url, "http://%s:%d/%s", host, 9000, track->artworkurl);
      status = downloadFile(url, cover);
      free(url);
   }

   if (status != success)
   {
      // http://localhost:9000/music/current/cover.jpg?player=f0:4d:a2:33:b7:ed

      asprintf(&url, "http://%s:%d/music/current/cover.jpg?player=%s", host, 9000, escId);
      status = downloadFile(url, cover);
      free(url);
   }

   return status;
}

int LmcCom::getCurrentCoverUrl(TrackInfo* track, std::string& coverUrl)
{
   char* url {};

   if (track && !isEmpty(track->artworkurl))
   {
      asprintf(&url, "http://%s:%d/%s", host, 9000, track->artworkurl);
   }
   else
   {
      // http://localhost:9000/music/current/cover.jpg?player=f0:4d:a2:33:b7:ed
      asprintf(&url, "http://%s:%d/music/current/cover.jpg?player=%s&nocache=%ld", host, 9000, escId, time(0));
   }

   coverUrl = url;
   free(url);

   return done;
}

//***************************************************************************
// Get Cover
//***************************************************************************

int LmcCom::getCover(MemoryStruct* cover, TrackInfo* track)
{
   char* url {};
   int status {fail};

   if (track && !isEmpty(track->artworkurl))
   {
      asprintf(&url, "http://%s:%d/%s", host, 9000, track->artworkurl);
      status = downloadFile(url, cover);
      free(url);
   }

   if (status != success)
   {
      // http://<server>:<port>/music/<track_id>/cover.jpg

      if (isEmpty(track->artworkTrackId))
         asprintf(&url, "http://%s:%d/music/%d/cover.jpg", host, 9000, track->id);
      else
         asprintf(&url, "http://%s:%d/music/%s/cover.jpg", host, 9000, track->artworkTrackId);

      status = downloadFile(url, cover);
      free(url);
   }

   return status;
}

int LmcCom::getCoverUrl(TrackInfo* track, std::string& coverUrl)
{
   char* url {};

   if (track && !isEmpty(track->artworkurl))
   {
      asprintf(&url, "http://%s:%d/%s", host, 9000, track->artworkurl);
   }
   else
   {
      // http://<server>:<port>/music/<track_id>/cover.jpg

      if (isEmpty(track->artworkTrackId))
         asprintf(&url, "http://%s:%d/music/%d/cover.jpg", host, 9000, track->id);
      else
         asprintf(&url, "http://%s:%d/music/%s/cover.jpg", host, 9000, track->artworkTrackId);
   }

   coverUrl = url;
   free(url);

   return coverUrl.length() ? success : fail;
}
