/*
 * lmccom.c
 *
 * See the README file for copyright information
 *
 */

#include "lib/common.h"
#include "lib/json.h"

#include "lmccom.h"

//***************************************************************************
// Object
//***************************************************************************

const char* LmcCom::rangeQueryTypes[]
{
   "genres",
   "artists",
   "albums",
   "newmusic",
   "tracks",
   "years",
   "playlists",
   "radios",
   "radioapps",
   "favorites",
   "players",
   "music",
    nullptr
};

const char* LmcCom::toName(LmcCom::RangeQueryType rqt)
{
   if (rqt > rqtUnknown && rqt < rqtCount)
      return rangeQueryTypes[rqt];

   return "";
}

//***************************************************************************
// Object
//***************************************************************************

LmcCom::LmcCom(const char* aMac)
   : TcpChannel()
{
   curl = new cCurl;

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

   close();
   free(host);
   free(mac);
   free(escId);
   free(curlUrl);

   delete curl;
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

   // Endpoint for new LMC REST API
   //   actually only used for players query

   free(curlUrl);
   asprintf(&curlUrl, "http://%s:%d/jsonrpc.js", host, 9000);

   return res;
}

//***************************************************************************
// Update Current Playlist
//***************************************************************************

int LmcCom::update(int stateOnly)
{
   playerState = {};
   tracks.clear();

   if (restQuery("version", {"?"}) == success)
   {
      json_t* jResult {getRestResult()};
      playerState.version = getStringByPath(jResult, "result/_version", "");
      json_decref(jResult);
   }

   if (restQuery("mixer", {"muting", "?"}) == success)
   {
      json_t* jResult {getRestResult()};
      playerState.muted = atoi(getStringByPath(jResult, "result/_muting", "0"));
      json_decref(jResult);
   }

   if (restQuery("playlist", {"tracks", "?"}) != success)
      return fail;

   json_t* jResult {getRestResult()};
   int count = getIntByPath(jResult, "result/_tracks", 0);
   json_decref(jResult);

   if (!stateOnly)
      count = std::max(count, 100);

   std::string title;
   LmcCom::RangeList list;
   LmcCom::Parameters filters;
   filters.push_back("tags:agdluyKJNxrow");
   int total {0};

   if (restQueryRange("status", 0, count, "", &list, total, title, &filters) != success)
   {
      tell(eloLmc, "[LMC] Status request failed!");
      return fail;
   }

   // {
   //  "result": {
   //   "power": 1,
   //   "sync_slaves": "b8:27:eb:c4:c3:4e",
   //   "playlist_tracks": 1042,
   //   "player_ip": "192.168.200.198:43968",
   //   "playlist_cur_index": "7",
   //   "seq_no": 0,
   //   "can_seek": 1,
   //   "playlist_timestamp": 1737389243.55132,
   //   "playlist shuffle": 1,
   //   "duration": 153.333,
   //   "player_name": "vdrtft",
   //   "randomplay": 0,
   //   "mixer volume": 35,
   //   "digital_volume_control": 1,
   //   "signalstrength": 0,
   //   "time": 5.89090189170837,
   //   "playlist mode": "off",
   //   "rate": 1,
   //   "playlist repeat": 2,
   //   "sync_master": "e8:40:f2:3e:0c:f0",
   //   "mode": "play",
   //   "player_connected": 1
   //   "playlist_loop": [
   //    {
   //       "url": "file:///tank/mediaroot/music/flac/John%20Denver/John%20Denver%20-%20Seine%20gro%C3%9Fen%20Erfolge%20CD%202/03%20Fly%20Away.flac",
   //       "playlist index": 0,
   //       "title": "Fly Away",
   //       "year": "0",
   //       "type": "flc",
   //       "album": "Seine großen Erfolge CD 2",
   //       "id": 1986,
   //       "artwork_track_id": "9b36d76f",
   //       "artwork_url": "/imageproxy/http%3A%2F%2Fcdn-radiotime-logos.tunein.com%2Fs261313q.png/image.png",
   //       "genre": "Country",
   //       "duration": 247.893,
   //       "artist": "John Denver",
   //       "bitrate": "782kb/s VBR",
   //       "remote": "0"
   //    },
   //    ...
   //    ...

   // old ...

   json_t* jData {};

   if (!(jData = getRestResult()))
       return fail;

   jResult = getObjectFromJson(jData, "result");

   playerState.updatedAt = cTimeMs::Now();
   playerState.trackTime = getDoubleFromJson(jResult, "time");
   playerState.volume = getIntFromJson(jResult, "mixer volume");
   playerState.plIndex  = atoi(getStringFromJson(jResult, "playlist_cur_index", "0"));
   playerState.plShuffle = getIntFromJson(jResult, "playlist shuffle");
   playerState.plRepeat = getIntFromJson(jResult, "playlist repeat");

   playerState.mode =  getStringFromJson(jResult, "mode", "");
   playerState.plName = getStringFromJson(jResult, "player_name", "");

   json_t* jPlaylist = getObjectFromJson(jResult, "playlist_loop");

   if (!jPlaylist)
   {
      json_decref(jData);
      return fail;
   }

   TrackInfo t {};

   size_t index {0};
   json_t* jItem {};

   json_array_foreach(jPlaylist, index, jItem)
   {
      t = {};

      t.updatedAt = cTimeMs::Now();
      t.index = getIntFromJson(jItem, "playlist index");
      t.id = getIntFromJson(jItem, "id");
      t.year = getStringFromJson(jItem, "year", "");
      t.duration = getDoubleFromJson(jItem, "duration");
      t.remote = atoi(getStringFromJson(jItem, "remote", "0"));
      t.bitrate = atoi(getStringFromJson(jItem, "bitrate", "0"));

      t.title = getStringFromJson(jItem, "title", "");
      t.artist = getStringFromJson(jItem, "artist", "");
      t.genre = getStringFromJson(jItem, "genre", "");
      t.artworkTrackId = getStringFromJson(jItem, "artwork_track_id", "");
      t.artworkUrl = getStringFromJson(jItem, "artwork_url", "");
      t.album = getStringFromJson(jItem, "album", "");
      t.remoteTitle = getStringFromJson(jItem, "remote", "");
      t.contentType = getStringFromJson(jItem, "type", "");
      t.file = getStringFromJson(jItem, "url", "");

      // ?? t.lyrics = getStringFromJson(jItem, "", "");
      // ?? t.artworkurl = getStringFromJson(jItem, "", "");

      tracks.push_back(t);
   }

   json_decref(jData);

   playerState.plCount = tracks.size();
   tell(eloDebugLmc, "[LMC] Playlist updated, got %d track", playerState.plCount);

   return success;
}

//***************************************************************************
// Query Players
//***************************************************************************

int LmcCom::queryPlayers(RangeList* list)
{
   std::string sOutput;
   const char* request {"{ \"method\": \"slim.request\", \"params\":[\"00:00:00:00:00:00\", [\"players\", \"0\", \"20\"]]}"};

   if (curl->post(curlUrl, request, &sOutput) != success)
      return fail;

   tell(eloDebugLmc, "[LMC/REST] <- (players) [%s]", sOutput.c_str());

   json_t* oData = jsonLoad(sOutput.c_str());

   if (!oData)
      return fail;

   json_t* oResult = getObjectByPath(oData, "result/players_loop");

   if (!oResult)
      return fail;

   size_t index {0};
   json_t* jObj {};

   json_array_foreach(oResult, index, jObj)
   {
      ListItem item;
      item.clear();

      const char* playerindex = getStringFromJson(jObj, "playerindex");

      if (!isEmpty(playerindex))
         item.id = atoi(playerindex);
      else
         item.id = getIntFromJson(jObj, "playerindex");

      item.content = getStringFromJson(jObj, "playerid");
      item.command = getStringFromJson(jObj, "name");
      item.isConnected = getIntFromJson(jObj, "connected");

      list->push_back(item);
   }

   return done;
}

//***************************************************************************
// Query Action via REST
//***************************************************************************

int LmcCom::restQuery(std::string what)
{
   LmcCom::Parameters parameters;

   return restQuery(what,  parameters);
}

int LmcCom::restQuery(std::string what, Parameters pars)
{
   json_t* jRequest {json_object()};
   json_object_set_new(jRequest, "method", json_string("slim.request"));

   json_t* jParameters {json_array()};
   json_object_set_new(jRequest, "params", jParameters);
   json_array_append_new(jParameters, json_string(mac));

   json_t* jParameter {json_array()};
   json_array_append_new(jParameters, jParameter);
   json_array_append_new(jParameter, json_string(what.c_str()));

   for (const auto& par : pars)
      json_array_append_new(jParameter, json_string(par.c_str()));

   char* request = json_dumps(jRequest, JSON_REAL_PRECISION(4));
   json_decref(jRequest);
   tell(eloDebugLmc, "[LMC/REST] -> (%s) [%s]", what.c_str(), request);
   int res = curl->post(curlUrl, request, &lastRestResult);
   free(request);

   tell(eloDebugLmc, "[LMC/REST] <- (%s) [%s]", what.c_str(), lastRestResult.c_str());
   return res;
}

//***************************************************************************
// Get REST Result
//***************************************************************************

json_t* LmcCom::getRestResult()
{
   if (lastRestResult.empty())
      return nullptr;

   return jsonLoad(lastRestResult.c_str());
}

//***************************************************************************
// Query Range via REST
//***************************************************************************

int LmcCom::restQueryRange(std::string what, int from, int count, const char* special,
                           RangeList* list, int& total, std::string& title, Parameters* pars)
{
   json_t* jRequest {json_object()};
   json_object_set_new(jRequest, "method", json_string("slim.request"));

   json_t* jParameters {json_array()};
   json_object_set_new(jRequest, "params", jParameters);
   json_array_append_new(jParameters, json_string(mac));

   json_t* jParameter {json_array()};
   json_array_append_new(jParameters, jParameter);
   json_array_append_new(jParameter, json_string(what.c_str()));

   if (!isEmpty(special))
      json_array_append_new(jParameter, json_string(special));

   json_array_append_new(jParameter, json_integer(from));
   json_array_append_new(jParameter, json_integer(count));

   if (pars)
   {
      for (const auto& par : *pars)
         json_array_append_new(jParameter, json_string(par.c_str()));
   }

   char* request = json_dumps(jRequest, JSON_REAL_PRECISION(4));
   json_decref(jRequest);
   tell(eloAlways, "[LMC/REST] -> (%s) [%s]", what.c_str(), request);
   int res = curl->post(curlUrl, request, &lastRestResult);
   free(request);

   if (res != success)
      return fail;

   tell(eloAlways, "[LMC/REST] <- [%s]", lastRestResult.c_str());
   json_t* jData {jsonLoad(lastRestResult.c_str())};

   if (!jData)
      return fail;

   json_t* jResult = getObjectFromJson(jData, "result");
   total = getIntFromJson(jResult, "count");
   title = getStringFromJson(jResult, "title", "");

   if (title.empty())
   {
      json_t* jParams = getObjectByPath(jData, "params[1]");

      if (jParams && json_is_array(jParams))
      {
         size_t index {0};
         json_t* jObj {};

         json_array_foreach(jParams, index, jObj)
         {
            if (json_is_string(jObj))
            {
               const char* p {json_string_value(jObj)};
               if (strstr(p, "menu_title:"))
               {
                  title = what + " - " + (strstr(p, "menu_title:") + strlen("menu_title:"));
                  title[0] = toupper(title[0]);
                  break;
               }
            }
         }
      }
   }

   if (title.empty())
   {
      title = what;
      title[0] = toupper(title[0]);
   }

   json_t* jList {};
   const char* key {};
   json_t* jValue {};

   json_object_foreach(jResult, key, jValue)
   {
      if (strstr(key, "_loop"))
      {
         jList = jValue;
         break;
      }
   }

   if (!jList)
   {
      json_decref(jResult);
      return fail;
   }

   size_t index {0};
   json_t* jObj {};

   json_array_foreach(jList, index, jObj)
   {
      int res {fail};
      ListItem item;

      tell(eloLmc, "[LMC] Parsing '%s'", what.c_str());

      if (what == "genres")
         res = parseGenres(jObj, item);
      else if (what == "artists")
         res = parseArtists(jObj, item);
      else if (what == "albums")
         res = parseAlbums(jObj, item);
      else if (what == "newmusic")
         res = parseNewmusic(jObj, item);
      else if (what == "tracks")
         res = parseTracks(jObj, item);
      else if (what == "years")
         res = parseYears(jObj, item);
      else if (what == "playlists")
         res = parsePlaylists(jObj, item);
      else if (what == "radios")
         res = parseRadios(jObj, item);
      else if (what == "radioapps")
         res = parseRadioapps(jObj, item);
      else if (what == "favorites")
         res = parseFavorites(jObj, item);
      else if (what == "players")
         res = parsePlayers(jObj, item);
      else if (what == "music")
         res = parseMusic(jObj, item);
      else if (what == "local")
         res = parseLocalRadio(jObj, item);
      else
      {
         tell(eloDebugLmc, "[LMC] No parsing of '%s'", what.c_str());
         break;
      }

      if (res == success)
         list->push_back(item);
   }

   json_decref(jResult);

   return success;
}

//***************************************************************************
// Parse ..
//***************************************************************************

//***************************************************************************
// {
//   "text": "Sender",
//   "addAction": "go",
//   "actions": {
//     "go": {
//       "params": {
//         "menu": "local",
//         "item_id": "c2a1c02a.0"
//       },
//       "cmd": [
//         "local",
//         "items"
//       ]
//     }
//   }
// },
// {
//   "actions": {
//     "go": {
//       "params": {
//         "item_id": "c2a1c02a.1",
//         "menu": "local"
//       },
//       "cmd": [
//         "local",
//         "items"
//       ]
//     }
//   },
//   "addAction": "go",
//   "type": "link",
//   "text": "Alle Deutschland"
// }
//***************************************************************************

int LmcCom::parseLocalRadio(json_t* jObj, ListItem& item)
{
   item.id = getStringByPath(jObj, "actions/go/params/item_id", "");

   if (!item.id.length())
      item.id = getStringByPath(jObj, "params/item_id", "");

   item.content = getStringFromJson(jObj, "text", "");
   json_t* jCmdArray = getObjectByPath(jObj, "actions/go/cmd");

   if (jCmdArray)
   {
      json_t* j = json_array_get(jCmdArray, 0);
      item.command = json_string_value(j);
      j = json_array_get(jCmdArray, 1);
      item.commandSpecial = json_string_value(j);
   }

   item.contextMenu = getIntByPath(jObj, "params/isContextMenu", 0);
   item.icon = getStringFromJson(jObj, "icon", "");
   return success;
}

int LmcCom::parseGenres(json_t* jObj, ListItem& item)
{
   item.id = std::to_string(getIntFromJson(jObj, "id"));
   item.content = getStringFromJson(jObj, "genre", "");
   return success;
}

int LmcCom::parseArtists(json_t* jObj, ListItem& item)
{
   item.id = std::to_string(getIntFromJson(jObj, "id"));
   item.content = getStringFromJson(jObj, "artist", "");
   return success;
}

int LmcCom::parseAlbums(json_t* jObj, ListItem& item)
{
   item.id = std::to_string(getIntFromJson(jObj, "id"));
   item.content = getStringFromJson(jObj, "album", "");
   return success;
}

int LmcCom::parseNewmusic(json_t* jObj, ListItem& item)
{
   item.id = std::to_string(getIntFromJson(jObj, "id"));
   item.content = getStringFromJson(jObj, "album", "");
   return success;
}

//***************************************************************************
// {
//   "id": 1003,
//   "title": "Daddy Cool + The Girl Can.t Help It",
//   "duration": 150.8,
//   "artist_id": "25",
//   "tracknum": "1",
//   "year": "1987",
//   "url": "file:///tank/mediaroot/music/flac/Darts/20%20Of%20The%20Best/01%20-%20Daddy%20Cool%20+%20The%20Girl%20Can%27t%20Help%20It.flac",
//   "artist": "Darts",
//   "compilation": "0",
//   "genres": "Revival",
//   "genre_ids": "11",
//   "artist_ids": "25"
// }
//***************************************************************************

int LmcCom::parseTracks(json_t* jObj, ListItem& item)
{
   item.id = std::to_string(getIntFromJson(jObj, "id"));
   item.content = getStringFromJson(jObj, "title", "");
   item.tracknum = atoi(getStringFromJson(jObj, "tracknum", "-1"));
   item.contextMenu = true;


   return success;
}

int LmcCom::parseYears(json_t* jObj, ListItem& item)
{
   item.id = std::to_string(getIntFromJson(jObj, "id"));
   item.content = std::to_string(getIntFromJson(jObj, "year", 0));
   return success;
}

int LmcCom::parsePlaylists(json_t* jObj, ListItem& item)
{
   item.id = std::to_string(getIntFromJson(jObj, "id"));
   item.content = getStringFromJson(jObj, "playlist", "");
   item.contextMenu = true;
   return success;
}

//***************************************************************************
// Parse Radios
// {
//   "type": "xmlbrowser",
//   "name": "RadioNet ",
//   "icon": "plugins/RadioNet/html/images/radionet.png",
//   "cmd": "radionet",
//   "weight": 1
// },
// {
//   "type": "xmlbrowser",
//   "name": "Eigene Voreinstellungen",
//   "icon": "/plugins/TuneIn/html/images/radiopresets.png",
//   "cmd": "presets",
//   "weight": 5
// },
// {
//   "cmd": "local",
//   "icon": "/plugins/TuneIn/html/images/radiolocal.png",
//   "weight": 20,
//   "type": "xmlbrowser",
//   "name": "Lokales Radio"
// },
// {
//   "type": "xmlbrowser",
//   "name": "Musik",
//   "icon": "/plugins/TuneIn/html/images/radiomusic.png",
//   "cmd": "music",
//   "weight": 30
// },
// {
//   "cmd": "sports",
//   "icon": "/plugins/TuneIn/html/images/radiosports.png",
//   "weight": 40,
//   "type": "xmlbrowser",
//   "name": "Sport"
// },
// {
//   "name": "Nachrichten",
//   "type": "xmlbrowser",
//   "weight": 45,
//   "cmd": "news",
//   "icon": "/plugins/TuneIn/html/images/radionews.png"
// },
// {
//   "type": "xmlbrowser",
//   "name": "Talksendungen",
//   "icon": "/plugins/TuneIn/html/images/radiotalk.png",
//   "cmd": "talk",
//   "weight": 50
// },
// {
//   "weight": 55,
//   "icon": "/plugins/TuneIn/html/images/radioworld.png",
//   "cmd": "location",
//   "name": "Nach Standort",
//   "type": "xmlbrowser"
// },
// {
//   "icon": "/plugins/TuneIn/html/images/radioworld.png",
//   "cmd": "language",
//   "weight": 56,
//   "type": "xmlbrowser",
//   "name": "Nach Sprache"
// },
// {
//   "type": "xmlbrowser",
//   "name": "Podcasts",
//   "cmd": "podcast",
//   "icon": "/plugins/TuneIn/html/images/podcasts.png",
//   "weight": 70
// },
// {
//   "type": "xmlbrowser_search",
//   "name": "TuneIn durchsuchen",
//   "cmd": "search",
//   "icon": "/plugins/TuneIn/html/images/radiosearch.png",
//   "weight": 110
// }
//***************************************************************************

int LmcCom::parseRadios(json_t* jObj, ListItem& item)
{
   item.command = getStringFromJson(jObj, "cmd", "");
   item.content = getStringFromJson(jObj, "name", "");
   item.icon = getStringFromJson(jObj, "icon", "");
   return success;
}

int LmcCom::parseRadioapps(json_t* jObj, ListItem& item)
{
   item.id = std::to_string(getIntFromJson(jObj, "id"));
   item.content = getStringFromJson(jObj, "name", "");
   item.command = getStringFromJson(jObj, "favorites", "");
   item.hasItems = getIntFromJson(jObj, "hasitems");
   item.isAudio = getIntFromJson(jObj, "isaudio");
   return success;
}

//***************************************************************************
// {
//   "id": "cbd52104.0",
//   "name": "Australian Country Radio (Australien)",
//   "type": "audio",
//   "image": "/imageproxy/http%3A%2F%2Fcdn-radiotime-logos.tunein.com%2Fs261313q.png/image.png",
//   "isaudio": 1,
//   "hasitems": 0
// }
//***************************************************************************

int LmcCom::parseFavorites(json_t* jObj, ListItem& item)
{
   item.id = getStringFromJson(jObj, "id", "");
   item.content = getStringFromJson(jObj, "name", "");
   item.command = getStringFromJson(jObj, "favorites", "");
   item.contextMenu = true;
   item.icon = getStringFromJson(jObj, "image", "");
   return success;
}

int LmcCom::parsePlayers(json_t* jObj, ListItem& item)
{
   const char* playerindex = getStringFromJson(jObj, "playerindex");

   if (!isEmpty(playerindex))
      item.id = playerindex;
   else
      item.id = std::to_string(getIntFromJson(jObj, "playerindex"));

   item.content = getStringFromJson(jObj, "playerid", "");
   item.command = getStringFromJson(jObj, "name", "");
   item.isConnected = getIntFromJson(jObj, "connected");

   return success;
}

//***************************************************************************
// {
//    "text": "Weltmusik",
//    "type": "link",
//    "addAction": "go",
//    "actions": {
//      "go": {
//        "cmd": [
//          "music",
//          "items"
//        ],
//        "params": {
//          "item_id": "04412624.0",
//          "menu": "music"
//        }
//      }
//    }
//  }
//***************************************************************************

int LmcCom::parseMusic(json_t* jObj, ListItem& item)
{
   item.id = getStringByPath(jObj, "actions/go/params/item_id", "");

   if (item.id.empty())
      item.id = getStringByPath(jObj, "params/item_id", "");

   item.contextMenu = getIntByPath(jObj, "params/isContextMenu", 0);
   item.content = getStringFromJson(jObj, "text", "");
   item.icon = getStringFromJson(jObj, "icon", "");

   json_t* jCmdArray = getObjectByPath(jObj, "actions/go/cmd");

   if (jCmdArray)
   {
      json_t* j = json_array_get(jCmdArray, 0);
      item.command = json_string_value(j);
      j = json_array_get(jCmdArray, 1);
      item.commandSpecial = json_string_value(j);
   }

   return success;
}

//***************************************************************************
// Request
//***************************************************************************

int LmcCom::request(const char* command)
{
   snprintf(lastCommand, sizeMaxCommand, "%s", command);
   lastPar = "";

   tell(eloDebugLmc, "[LMC] Requesting '%s' with '%s', escId '%s'", lastCommand, lastPar.c_str(), escId);
   flush();

   int status {success};

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

int LmcCom::execute(const char* command)
{
   LmcLock;
   request(command);
   write("\n");

   return response();
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
      LmcCom::RangeList players;

      if (queryPlayers(&players) == success)
      {
         tell(eloDebugLmc, "[LMC] Query players succeeded (%zu)", players.size());

         for (const auto& player : players)
         {
            if (player.content == mac && player.isConnected)
               myPlayerConnected = true;
            tell(eloDebugLmc, "[LMC] Player %s) %s '%s', connected %d", player.id.c_str(), player.command.c_str(), player.content.c_str(), myPlayerConnected);
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
//
//   /imageproxy/http%3A%2F%2Fcdn-profiles.tunein.com%2Fs312162%2Fimages%2Flogoq.png%3Ft%3D484/image_300x300_f.png
//   /imageproxy/http%3A%2F%2Fcdn-profiles.tunein.com%2Fs312162%2Fimages%2Flogoq.png%3Ft%3D484/image.png
//   /imageproxy/http%3A%2F%2Fcdn-profiles.tunein.com%2Fs312162%2Fimages%2Flogoq.png%3Ft%3D484/image_1024x1024_f.png
//***************************************************************************

int LmcCom::getCurrentCover(MemoryStruct* cover, TrackInfo* track, bool big)
{
   char* tmp {};
   int status {fail};

   if (track && !track->artworkUrl.empty())
   {
      asprintf(&tmp, "http://%s:%d%s", host, 9000, track->artworkUrl.c_str());
      std::string url {tmp};
      free(tmp);

      if (big)
         url = strReplace("image.png", "image_1024x1024_f.png", url);

      status = downloadFile(url.c_str(), cover);

   }

   if (status != success)
   {
      // http://localhost:9000/music/current/cover.jpg?player=f0:4d:a2:33:b7:ed

      asprintf(&tmp, "http://%s:%d/music/current/cover.jpg?player=%s", host, 9000, escId);
      status = downloadFile(tmp, cover);
      free(tmp);
   }

   return status;
}

int LmcCom::getCurrentCoverUrl(TrackInfo* track, std::string& coverUrl, bool big)
{
   char* tmp {};

   if (track && !track->artworkUrl.empty())
   {
      asprintf(&tmp, "http://%s:%d%s", host, 9000, track->artworkUrl.c_str());
      coverUrl = tmp;

      if (big)
         coverUrl = strReplace("image.png", "image_1024x1024_f.png", coverUrl);

      // asprintf(&tmp, "http://%s:%d%s%s", host, 9000, track->artworkurl[0] == '/' ? "" : "/", track->artworkurl);
   }
   else
   {
      // http://localhost:9000/music/current/cover.jpg?player=f0:4d:a2:33:b7:ed
      asprintf(&tmp, "http://%s:%d/music/current/cover.jpg?player=%s&nocache=%ld", host, 9000, escId, time(0));
      coverUrl = tmp;
   }

   free(tmp);

   return done;
}

//***************************************************************************
// Get Cover
//***************************************************************************

int LmcCom::getCover(MemoryStruct* cover, TrackInfo* track)
{
   char* url {};
   int status {fail};

   if (track && !track->artworkUrl.empty())
   {
      asprintf(&url, "http://%s:%d%s", host, 9000, track->artworkUrl.c_str());
      status = downloadFile(url, cover);
      free(url);
   }

   if (status != success)
   {
      // http://<server>:<port>/music/<track_id>/cover.jpg

      if (track->artworkTrackId.empty())
         asprintf(&url, "http://%s:%d/music/%d/cover.jpg", host, 9000, track->id);
      else
         asprintf(&url, "http://%s:%d/music/%s/cover.jpg", host, 9000, track->artworkTrackId.c_str());

      status = downloadFile(url, cover);
      free(url);
   }

   return status;
}

int LmcCom::getCoverUrl(TrackInfo* track, std::string& coverUrl)
{
   char* url {};

   if (track && !track->artworkUrl.empty())
   {
      asprintf(&url, "http://%s:%d%s", host, 9000, track->artworkUrl.c_str());
   }
   else
   {
      // http://<server>:<port>/music/<track_id>/cover.jpg

      if (track->artworkTrackId.empty())
         asprintf(&url, "http://%s:%d/music/%d/cover.jpg", host, 9000, track->id);
      else
         asprintf(&url, "http://%s:%d/music/%s/cover.jpg", host, 9000, track->artworkTrackId.c_str());
   }

   coverUrl = url;
   free(url);

   return coverUrl.length() ? success : fail;
}
