/*
 * lmccom.h
 *
 * See the README file for copyright information
 *
 */

// https://github.com/elParaguayo/LMS-CLI-Documentation/blob/master/LMS-CLI.md

/*
  or (better) use JSON interface like:

  - the first parameter is the player id or "" for 'global' commands

   curl -X POST -d '{ "method": "slim.request", "params":["b8:27:eb:91:96:1b", ["mixer", "volume", "100"]]}' gate:9000/jsonrpc.js
   curl -X POST -d '{ "method": "slim.request", "params":["b8:27:eb:91:96:1b", ["status"]]}' gate:9000/jsonrpc.js | json_pp
   curl -X POST -d '{ "method": "slim.request", "params":["", ["players", "0", "100"]]}' gate:9000/jsonrpc.js | json_pp
   curl -X POST -d '{ "method": "slim.request", "id":1, "params":[ "", ["albums", "0","100" ]]}' gate:9000/jsonrpc.js | json_pp
   curl -X POST -d '{ "method": "slim.request", "id":1, "params":[ "", ["serverstatus", "0","100" ]]}' gate:9000/jsonrpc.js | json_pp

   // query tracks with genre_id 2:
   curl -X POST -d '{ "method": "slim.request", "id":4711, "params":[ 0, ["tracks", "0","100", "genre_id:2"]]}' gate:9000/jsonrpc.js | json_pp | less

   -> https://lyrion.org/reference/cli/using-the-cli/#jsonrpcjs
   -> https://forums.slimdevices.com/showthread.php?82985-JSON-help
*/

#pragma once

#include <curl/curl.h>
#include <vector>
#include <list>
#include <string>

#include "lib/tcpchannel.h"
#include "lib/curl.h"
#include "lib/json.h"
#include "lmcservice.h"

// #define LmcLock cMutexLock lock(&comMutex)
// #define LmcDoLock comMutex.Lock()
// #define LmcUnLock comMutex.Unlock()

#define LmcLock
#define LmcDoLock
#define LmcUnLock

//***************************************************************************
// LMC Communication
//***************************************************************************

class LmcCom : public TcpChannel
{
   public:

      enum RangeQueryType
      {
         rqtUnknown = na,

         rqtGenres,     //  0
         rqtArtists,    //  1
         rqtAlbums,     //  2
         rqtNewMusic,   //  3
         rqtTracks,     //  4
         rqtYears,      //  5
         rqtPlaylists,  //  6
         rqtRadios,     //  7
         rqtRadioApps,  //  8
         rqtFavorites,  //  9
         rqtPlayers,    // 10
         rqtMusic,      // 11

         rqtCount
      };

      static const char* rangeQueryTypes [];
      static const char* toName(LmcCom::RangeQueryType rqt);

      struct ListItem
      {
         ListItem() {};
         void clear() { id = ""; content = ""; command = ""; hasItems = false; isAudio = false, isConnected = false; }
         int isEmpty() { return content == ""; }

         std::string id;
         int tracknum {na};
         std::string content;
         std::string command;
         std::string commandSpecial;
         std::string icon;
         bool contextMenu {false};
         bool hasItems {false};
         bool isAudio {false};
         bool isConnected {false};
      };

      typedef std::list<ListItem> RangeList;
      typedef std::list<std::string> Parameters;

      enum Misc
      {
         sizeMaxCommand = 100
      };

      enum Results
      {
         wrnNoEventPending = -1000,
         ifoTrack,
         ifoPlayer
      };

      LmcCom(const char* aMac = nullptr);
      ~LmcCom();

      void setMac(const char* aMac)
      {
         free(mac);
         mac = strdup(aMac);
         free(escId);
         escId = escape(mac);
      }

      int open(const char* host = "localhost", unsigned short port = 9090);

      int restQuery(std::string what);
      int restQuery(std::string what,  Parameters pars);

      int restQueryRange(std::string what, int from, int count, const char* special,
                         RangeList* list, int& total, std::string& title, Parameters* pars = nullptr);

      json_t* getRestResult();

      int queryPlayers(RangeList* list);

      // cover

      int getCurrentCover(MemoryStruct* cover, TrackInfo* track = nullptr, bool big = false);
      int getCurrentCoverUrl(TrackInfo* track, std::string& coverUrl, bool big = false);
      int getCover(MemoryStruct* cover, TrackInfo* track);
      int getCoverUrl(TrackInfo* track, std::string& coverUrl);

      // notification channel

      int startNotify();
      int stopNotify();
      int checkNotify(uint64_t timeout = 0);
      int execute(const char* command);

      // const char* getLastQueryTitle() { return queryTitle ? queryTitle : ""; }

      // infos

      int update(int stateOnly = no);

      TrackInfo* getCurrentTrack()
      {
         if (playerState.plIndex >= 0 && playerState.plIndex < (int)tracks.size())
            return &tracks.at(playerState.plIndex);

         return &dummyTrack;
      }

      bool hasMetadataChanged() { return metaDataChanged; }

      PlayerState* getPlayerState() { return &playerState; }

      int getTrackCount()           { return tracks.size(); }
      TrackInfo* getTrack(int idx)  { return &tracks[idx]; }

      char* unescape(char* buf);       // url like encoding
      char* escape(const char* buf);
      int request(const char* command);
      int response(char* response = nullptr, int max = 0);

   private:

      int parseLocalRadio(json_t* jObj, ListItem& item);
      int parseArtists(json_t* jObj, ListItem& item);
      int parseAlbums(json_t* jObj, ListItem& item);
      int parseNewmusic(json_t* jObj, ListItem& item);
      int parseTracks(json_t* jObj, ListItem& item);
      int parseYears(json_t* jObj, ListItem& item);
      int parsePlaylists(json_t* jObj, ListItem& item);
      int parseRadios(json_t* jObj, ListItem& item);
      int parseRadioapps(json_t* jObj, ListItem& item);
      int parseFavorites(json_t* jObj, ListItem& item);
      int parsePlayers(json_t* jObj, ListItem& item);
      int parseMusic(json_t* jObj, ListItem& item);
      int parseGenres(json_t* jObj, ListItem& item);

      // void setQueryTitle(const char* title) { free(queryTitle); queryTitle = strdup(title); }
      // char* queryTitle {};

      char* host {};
      unsigned short port {9090};
      char* mac {};
      char* escId {};                       // escaped player id (build from mac)

      cCurl* curl {};
      char* curlUrl {};
      char lastCommand[sizeMaxCommand+TB] {};
      std::string lastPar;

      TrackInfo dummyTrack;
      TrackInfo currentTrack;
      PlayerState playerState;
      LmcCom* notify {};
      std::vector<TrackInfo> tracks;
      bool metaDataChanged {false};

      std::string lastRestResult;

#ifdef VDR_PLUGIN
      cMutex comMutex;
#endif
};
