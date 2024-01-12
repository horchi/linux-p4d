/*
 * lmccom.h
 *
 * See the README file for copyright information
 *
 */

// https://github.com/elParaguayo/LMS-CLI-Documentation/blob/master/LMS-CLI.md

/*
  or (better) use JSON interface like:
   curl -X POST -d '{ "method": "slim.request", "params":["b8:27:eb:91:96:1b", ["mixer", "volume", "100"]]}' gate:9000/jsonrpc.js
   curl -X POST -d '{ "method": "slim.request", "params":["b8:27:eb:91:96:1b", ["status"]]}' gate:9000/jsonrpc.js | json_pp
   curl -X POST -d '{ "method": "slim.request", "params":["", ["players", "0", "100"]]}' gate:9000/jsonrpc.js | json_pp
   curl -X POST -d '{ "method": "slim.request", "id":1, "params":[ "", ["albums", "0","100" ]]}' gate:9000/jsonrpc.js | json_pp
   curl -X POST -d '{ "method": "slim.request", "id":1, "params":[ "", ["serverstatus", "0","100" ]]}' gate:9000/jsonrpc.js | json_pp

   -> https://forums.slimdevices.com/showthread.php?82985-JSON-help
*/

#pragma once

#include <curl/curl.h>
#include <vector>
#include <list>
#include <string>

#include "lib/tcpchannel.h"
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

         rqtGenres,
         rqtArtists,
         rqtAlbums,
         rqtNewMusic,
         rqtTracks,
         rqtYears,
         rqtPlaylists,
         rqtRadios,
         rqtRadioApps,
         rqtFavorites,
         rqtPlayers
      };

      struct ListItem
      {
         ListItem()     { }
            void clear()   { id = ""; content = ""; command = ""; hasItems = false; isAudio = false, isConnected = false; }
         int isEmpty()  { return content == ""; }

         std::string id;
         std::string content;
         std::string command;
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

      int execute(const char* command, Parameters* pars = nullptr);
      int execute(const char* command, int par);
      int execute(const char* command, const char* par);

      int query(const char* command, char* response, int max);
      int queryInt(const char* command, int& value);

      int queryRange(RangeQueryType queryType, int from, int count,
                     RangeList* list, int& total, const char* special = "", Parameters* pars = nullptr);

      // cover

      int getCurrentCover(MemoryStruct* cover, TrackInfo* track = nullptr);
      int getCurrentCoverUrl(TrackInfo* track, std::string& coverUrl);
      int getCover(MemoryStruct* cover, TrackInfo* track);
      int getCoverUrl(TrackInfo* track, std::string& coverUrl);

      // notification channel

      int startNotify();
      int stopNotify();
      int checkNotify(uint64_t timeout = 0);

      // player steering

      int play()           { return execute("play"); }
      int pause()          { return execute("pause", "1"); }
      int pausePlay()      { return execute("pause"); }    // toggle pause/play
      int stop()           { return execute("stop"); }
      int volumeUp()       { return execute("mixer volume", "+5"); }
      int volumeDown()     { return execute("mixer volume", "-5"); }
      int mute()           { return execute("mixer muting", "1"); }
      int unmute()         { return execute("mixer muting", "0"); }
      int muteToggle()     { return execute("mixer muting toggle"); }
      int clear()          { return execute("playlist clear"); }
      int save()           { return execute("playlist save", escId); }
      int resume()         { return execute("playlist resume", escId); }
      int randomTracks()   { return execute("randomplay tracks"); }
      int shuffle()        { return execute("playlist shuffle"); }
      int repeat()         { return execute("playlist repeat"); }

      int scroll(short step)
      {
         char par[50] {};
         sprintf(par, "%c%d", step < 0 ? '-' : '+', (int)abs(step));
         return execute("time", par);
      }

      const char* getLastQueryTitle() { return queryTitle ? queryTitle : ""; }

      int nextTrack()      { return execute("playlist index", "+1"); }
      int prevTrack()      { return execute("playlist index", "-1"); }

      int track(unsigned short index)
      {
         char par[50] {};
         sprintf(par, "%d", index);
         return execute("playlist index", par);
      }

      int loadAlbum(const char* genre = "*", const char* artist = "*", const char* album = "*")
      {
         return ctrlAlbum("loadalbum", genre, artist, album);
      }

      int appendAlbum(const char* genre = "*", const char* artist = "*", const char* album = "*")
      {
         return ctrlAlbum("addalbum", genre, artist, album);
      }

      int ctrlAlbum(const char* command, const char* genre = "*", const char* artist = "*", const char* album = "*")
      {
         int status;
         char *a {}, *g {}, *l {};
         char* cmd {};

         asprintf(&cmd, "playlist %s %s %s %s", command,
                  *genre == '*' ? genre : g = escape(genre),
                  *artist == '*' ? artist : a = escape(artist),
                  *album == '*' ? album : l = escape(album));

         status = execute(cmd);
         free(cmd); free(a); free(g); free(l);
         return status;
      }

      int loadPlaylist(const char* playlist)
      {
         char par[500] {};
         Parameters pars;

         pars.push_back("cmd:load");
         sprintf(par, "playlist_name:%s", playlist);
         pars.push_back(par);

         return execute("playlistcontrol", &pars);
      }

      int appendPlaylist(const char* playlist)
      {
         char par[500] {};
         Parameters pars;

         pars.push_back("cmd:add");
         sprintf(par, "playlist_name:%s", playlist);
         pars.push_back(par);

         return execute("playlistcontrol", &pars);
      }

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

      int request(const char* command, Parameters* par = nullptr);
      int request(const char* command, const char* par);

      int responseP(char*& result);
      int response(char* response = nullptr, int max = 0);

   private:

      void setQueryTitle(const char* title) { free(queryTitle); queryTitle = strdup(title); }

      // data

      char* host {};
      unsigned short port {9090};
      char* mac {};
      char* escId {};                       // escaped player id (build from mac)

      CURL* curl {};
      char lastCommand[sizeMaxCommand+TB] {};
      std::string lastPar;

      TrackInfo dummyTrack;
      TrackInfo currentTrack;
      PlayerState playerState;
      LmcCom* notify {};
      std::vector<TrackInfo> tracks;
      char* queryTitle {};
      bool metaDataChanged {false};

#ifdef VDR_PLUGIN
      cMutex comMutex;
#endif
};
