//***************************************************************************
// Automation Control
// File lmc.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 2010-2023 Jörg Wendel
//***************************************************************************

#include "lib/json.h"
#include "daemon.h"

//***************************************************************************
// Init / Exit
//***************************************************************************

int Daemon::lmcInit(bool force)
{
   static time_t lastTryAt {0};

   if (isEmpty(lmcHost) || isEmpty(lmcPlayerMac))
      return done;


   if (!force && lastTryAt > time(0) - 10)
      return done;

   lastTryAt = time(0);

   if (!lmc)
      lmc = new LmcCom(lmcPlayerMac);

   tell(eloAlways, "[LMC] Try pening connection to server at '%s:%d' for player '%s'", lmcHost, lmcPort, lmcPlayerMac);

   if (lmc->open(lmcHost, lmcPort) != success)
   {
      tell(eloAlways, "[LMC] Opening connection to server at '%s:%d' failed", lmcHost, lmcPort);
      return fail;
   }

   tell(eloAlways, "[LMC] Opening connection to server at '%s:%d' for player '%s' succeeded", lmcHost, lmcPort, lmcPlayerMac);

   lmc->update();
   // lmc->startNotify();
   lmcUpdates();

   return success;
}

int Daemon::lmcExit()
{
   if (lmc)
   {
      lmc->stopNotify();
      lmc->close();
   }

   delete lmc;
   lmc = nullptr;

   return success;
}

//***************************************************************************
// Perform LMC Updates
//***************************************************************************

int Daemon::performLmcUpdates()
{
   if (!lmc)
      return done;

   if (!lmc->isOpen())
   {
      if (lmcInit() != success)
         return fail;
   }

   if (lmc->checkNotify(0) == success)
   {
      tell(eloDebugLmc, "[LMC] Changes pending");
      lmcUpdates();
   }

   return success;
}

int Daemon::lmcUpdates(long client)
{
   if (!lmc)
      return done;

   json_t* oJson {json_object()};
   lmcTrack2Json(oJson, lmc->getCurrentTrack());
   lmcPlayerState2Json(oJson);
   lmcPlaylist2Json(oJson);
   lmcMainMenu2Json(oJson);
   lmcPlayers2Json(oJson);

   return pushOutMessage(oJson, "lmcdata", client);
}

//***************************************************************************
// LMC Track To Json
//***************************************************************************

int Daemon::lmcTrack2Json(json_t* obj, TrackInfo* track)
{
   tell(eloDebugLmc, "[LMC] Track: %s / %s / %s / %s ", track->title.c_str(), track->artist.c_str(), track->genre.c_str(), track->album.c_str());

   // current track

   json_t* current {json_object()};
   json_object_set_new(obj, "current", current);

   json_object_set_new(current, "title", json_string(track->title.c_str()));
   json_object_set_new(current, "artist", json_string(track->artist.c_str()));
   json_object_set_new(current, "genre", json_string(track->genre.c_str()));
   json_object_set_new(current, "album", json_string(track->album.c_str()));

   json_object_set_new(current, "duration", json_integer(track->duration));

   json_object_set_new(current, "bitrate", json_integer(track->bitrate));
   json_object_set_new(current, "contentType", json_string(track->contentType.c_str()));
   json_object_set_new(current, "year", json_string(track->year.c_str()));
   json_object_set_new(current, "lyrics", json_string(track->lyrics.c_str()));
   // json_object_set_new(current, "id", json_integer(track->id));

   std::string url;
   lmc->getCurrentCoverUrl(track, url, true);

   tell(eloDebugLmc, "[LMC] Using artworkurl '%s'", track->artworkUrl.c_str());
   json_object_set_new(current, "cover", json_string(url.c_str()));

   return success;
}

int Daemon::lmcPlayerState2Json(json_t* obj)
{
   // player state

   json_t* state = json_object();
   json_object_set_new(obj, "state", state);

   PlayerState* currentState = lmc->getPlayerState();

   // json_object_set_new(state, "updatedAtMs", json_integer(currentState->updatedAt));

   json_object_set_new(state, "plIndex", json_integer(currentState->plIndex));
   json_object_set_new(state, "plName", json_string(currentState->plName.c_str()));
   json_object_set_new(state, "plCount", json_integer(currentState->plCount));
   json_object_set_new(state, "plShuffle", json_integer(currentState->plShuffle));
   json_object_set_new(state, "plRepeat", json_integer(currentState->plRepeat));
   json_object_set_new(state, "muted", json_boolean(currentState->muted));
   json_object_set_new(state, "mode", json_string(currentState->mode.c_str()));
   json_object_set_new(state, "trackTime", json_integer(currentState->trackTime));
   json_object_set_new(state, "volume", json_integer(currentState->volume / (100 / 100.0)));

   static int lastTime {0};

   tell(eloDebugLmc, "[LMC] Mode: %s", currentState->mode.c_str());

   if (currentState->mode == "play")
      lastTime = currentState->trackTime + (cTimeMs::Now() - currentState->updatedAt) / 1000;

   json_object_set_new(state, "trackTime", json_integer(lastTime));

   return success;
}

int Daemon::lmcPlayers2Json(json_t* obj)
{
   LmcCom::RangeList players;
   json_t* oPlayers = json_array();
   json_object_set_new(obj, "players", oPlayers);

   if (lmc->queryPlayers(&players) == success)
   {
      for (const auto& player : players)
      {
         json_t* oPlayer {json_object()};
         json_array_append_new(oPlayers, oPlayer);

         json_object_set_new(oPlayer, "id", json_integer(atoi(player.id.c_str())));
         json_object_set_new(oPlayer, "name", json_string(player.command.c_str()));
         json_object_set_new(oPlayer, "mac", json_string(player.content.c_str()));
         json_object_set_new(oPlayer, "connected", json_boolean(player.isConnected));
         json_object_set_new(oPlayer, "iscurrent", json_boolean(player.content == lmcPlayerMac));
      }
   }

   return success;
}

int Daemon::lmcPlaylist2Json(json_t* obj)
{
   // playlist

   json_t* oPlaylist = json_array();
   json_object_set_new(obj, "playlist", oPlaylist);

   for (int i = 0; i < lmc->getTrackCount(); i++)
   {
      TrackInfo* track = lmc->getTrack(i);

      json_t* oTrack = json_object();
      json_array_append_new(oPlaylist, oTrack);

      json_object_set_new(oTrack, "title", json_string(track->title.c_str()));
      json_object_set_new(oTrack, "artist", json_string(track->artist.c_str()));
      json_object_set_new(oTrack, "album", json_string(track->album.c_str()));

      std::string url;

      if (lmc->getCoverUrl(track, url) == success)
         json_object_set_new(oTrack, "cover", json_string(url.c_str()));
   }

   return success;
}

struct MenuItem
{
   const char* name;
   LmcCom::RangeQueryType queryType;
};

MenuItem menuItems[] =
{
   { "Artists",            LmcCom::rqtArtists },
   { "Albums",             LmcCom::rqtAlbums },
   { "Genres",             LmcCom::rqtGenres },
   { "Years",              LmcCom::rqtYears },
   // { "Play random tracks", LmcCom::rqtUnknown },
   { "Playlists",          LmcCom::rqtPlaylists },
   { "Radios",             LmcCom::rqtRadios },
   { "Favorites",          LmcCom::rqtFavorites },
// { "Players",            LmcCom::rqtPlayers },

//   { "", LmcCom::rqtUnknown }
};

int Daemon::lmcMainMenu2Json(json_t* obj)
{
   json_t* oMenu {json_object()};
   json_object_set_new(obj, "menu", oMenu);
   json_object_set_new(oMenu, "typeid", json_integer(na));
   json_object_set_new(oMenu, "type", json_string("main"));
   json_object_set_new(oMenu, "title", json_string("Home"));

   json_t* oItems {json_array()};
   json_object_set_new(oMenu, "items", oItems);

   for (const auto& item : menuItems)
   {
      json_t* oItem = json_object();
      json_array_append_new(oItems, oItem);

      json_object_set_new(oItem, "name", json_string(item.name));
      char tmp[20];
      sprintf(tmp, "%d", item.queryType);
      json_object_set_new(oItem, "id", json_string(tmp));
   }

   return success;
}

//***************************************************************************
// LMC Action
//***************************************************************************

int Daemon::performLmcAction(json_t* oObject, long client)
{
   if (!lmc)
      return done;

   LmcCom::Parameters filters;

   std::string action = getStringFromJson(oObject, "action", "");
   std::string id {getStringFromJson(oObject, "id", "")};
   std::string command {getStringFromJson(oObject, "cmd", "")};
   std::string commandsp {getStringFromJson(oObject, "cmdsp", "")};

   // #TODO
   // don't use LmcCom::RangeQueryType!
   LmcCom::RangeQueryType actualMenu {(LmcCom::RangeQueryType)getIntFromJson(oObject, "typeid")};
   std::string type {getStringFromJson(oObject, "type", "")};

   if (action == "menuaction")
   {
      std::string what {getStringFromJson(oObject, "what", "")};

      tell(eloLmc, "[LMC] Menu got action '%s', actualMenu '%s', what '%s', type '%s', id '%s'",
           action.c_str(), LmcCom::toName(actualMenu), what.c_str(), type.c_str(), id.c_str());

      if (what == "playnow")
      {
         if (!type.length())
            type = LmcCom::toName(actualMenu);

         if (type == "tracks")
         {
            lmc->restQuery("playlist", {"clear"});
            char par[50] {};
            sprintf(par, "track_id:%d", atoi(id.c_str()));
            lmc->restQuery("playlistcontrol", {"cmd:add", par});
            lmc->restQuery("play");
         }
         else if (type == "music" || type == "local" || type == "favorites")
         {
            // ["local", "playlist", "play", "menu:local", "item_id:e5b1dc1c.1.2.0.1", "isContextMenu:1"]
            char tmp[50] {};
            filters.push_back("playlist");
            filters.push_back("play");
            sprintf(tmp, "menu:%s", type.c_str());
            filters.push_back(tmp);
            sprintf(tmp, "item_id:%s", id.c_str());
            filters.push_back(tmp);
            lmc->restQuery(type.c_str(), filters);
         }
         else if (type == "playlists")
         {
            char tmp[50] {};
            filters.push_back("cmd:load");
            sprintf(tmp, "playlist_id:%s", id.c_str());
            filters.push_back(tmp);
            lmc->restQuery("playlistcontrol", filters);
         }
         else
            tell(eloAlways, "[LMC] Unexpected type '%s' for playnow", type.c_str());
      }
      else if (what == "append")
      {
         char par[50] {};
         sprintf(par, "track_id:%d", atoi(id.c_str()));
         lmc->restQuery("playlistcontrol", {"cmd:add", par});
      }

      return done;
   }
   else if (action == "menu")
   {
      constexpr int maxElements {50000};
      LmcCom::RangeList list;
      int total {0};
      std::string queryMenuString;
      std::string special;
      int status {success};

      std::string clickedText {getStringFromJson(oObject, "text", "")};
      LmcCom::RangeQueryType queryMenu = (LmcCom::RangeQueryType)getIntFromJson(oObject, "typeid", na);

      tell(eloLmc, "[LMC] Menu got action '%s', actualMenu '%s', queryMenu '%s', command '%s', commandsp '%s', id '%s', clickedText '%s'",
           action.c_str(), LmcCom::toName(actualMenu), LmcCom::toName(queryMenu), command.c_str(), commandsp.c_str(), id.c_str(), clickedText.c_str());

      if (queryMenu == LmcCom::rqtUnknown && !command.length())
         queryMenu = (LmcCom::RangeQueryType)atoi(id.c_str());

      else if (command.length())
      {
         queryMenu = LmcCom::rqtUnknown;
         queryMenuString = command;
         special = commandsp.length() ? commandsp : "items";
         if (id.length())
            filters.push_back("item_id:" + id);
         filters.push_back("menu:" + command);
      }

      else if (id.length())
      {
         filters.push_back("menu_title:" + clickedText);

         if (id == "local")
         {
            queryMenu = LmcCom::rqtUnknown;
            queryMenuString = id;
            special = "items";
            filters.push_back("menu:local");
         }

         else if (queryMenu == LmcCom::rqtAlbums)
         {
            queryMenu = LmcCom::rqtTracks;
            filters.push_back("tags:distbhz1kyuAACGPSE");
            filters.push_back("sort:tracknum");
            filters.push_back("album_id:" + id);
         }

         else if (queryMenu == LmcCom::rqtGenres)
         {
            queryMenu = LmcCom::rqtTracks;
            char* tmp {};
            asprintf(&tmp, "genre_id:%s", id.c_str());
            filters.push_back(tmp);
            free(tmp);
         }
         else if (queryMenu == LmcCom::rqtYears)
         {
            queryMenu = LmcCom::rqtTracks;
            char* tmp {};
            asprintf(&tmp, "year_id:%s", id.c_str());
            filters.push_back(tmp);
            free(tmp);
         }
         else if (queryMenu == LmcCom::rqtArtists)
         {
            queryMenu = LmcCom::rqtTracks;
            char* tmp {};
            asprintf(&tmp, "artist_id:%s", id.c_str());
            filters.push_back(tmp);
            free(tmp);
         }
         else if (queryMenu == LmcCom::rqtRadios)
         {
            queryMenu = LmcCom::rqtMusic;
            special = "items";
            filters.push_back("menu:music");
         }
         else if (queryMenu == LmcCom::rqtMusic)
         {
            queryMenu = LmcCom::rqtMusic;
            special = "items";
            filters.push_back("menu:music");
            char* tmp {};
            asprintf(&tmp, "item_id:%s", id.c_str());
            filters.push_back(tmp);
            free(tmp);
         }
      }

      if (queryMenu == LmcCom::rqtNewMusic)
         filters.push_back("sort:new");
      else if (queryMenu == LmcCom::rqtPlaylists)
         filters.push_back("tags:suxE");
      else if (queryMenu == LmcCom::rqtFavorites)
         special = "items";

      std::string title;

      if (queryMenu != LmcCom::rqtUnknown)
      {
         // old by hardcoded enum

         tell(eloLmc, "[LMC] Menu query: action '%s', queryMenu '%s' with special '%s'", action.c_str(), LmcCom::toName(queryMenu), special.c_str());
         status = lmc->restQueryRange(LmcCom::toName(queryMenu), 0, maxElements, special.c_str(), &list, total, title, &filters);
      }
      else
      {
         // new by string

         tell(eloLmc, "[LMC] Menu query: action '%s', queryMenuString '%s' with special '%s'", action.c_str(), queryMenuString.c_str(), special.c_str());
         status = lmc->restQueryRange(queryMenuString, 0, maxElements, special.c_str(), &list, total, title, &filters);
      }

      json_t* oMenu {json_object()};
      json_t* oItems {json_array()};
      json_object_set_new(oMenu, "items", oItems);
      json_object_set_new(oMenu, "typeid", json_integer(queryMenu));

      if (queryMenu != LmcCom::rqtUnknown)
         json_object_set_new(oMenu, "type", json_string(LmcCom::toName(queryMenu)));
      else
         json_object_set_new(oMenu, "type", json_string(queryMenuString.c_str()));

      if (status == success)
      {
         json_object_set_new(oMenu, "title", json_string(title.c_str()));

         LmcCom::RangeList::iterator it;

         for (it = list.begin(); it != list.end(); ++it)
         {
            json_t* oItem = json_object();
            json_array_append_new(oItems, oItem);

            json_object_set_new(oItem, "id", json_string((*it).id.c_str()));
            json_object_set_new(oItem, "cmd", json_string((*it).command.c_str()));
            json_object_set_new(oItem, "cmdsp", json_string((*it).commandSpecial.c_str()));
            json_object_set_new(oItem, "name", json_string((*it).content.c_str()));
            json_object_set_new(oItem, "contextMenu", json_boolean((*it).contextMenu));

            if ((*it).tracknum != na)
               json_object_set_new(oItem, "tracknum", json_integer((*it).tracknum));

            if (!(*it).icon.empty())
            {
               char* tmp {};
               if ((*it).icon.at(0) == '/')
                  asprintf(&tmp, "http://%s:%d%s", lmcHost, 9000, (*it).icon.c_str());
               else
                  asprintf(&tmp, "http://%s:%d/%s", lmcHost, 9000, (*it).icon.c_str());
               json_object_set_new(oItem, "icon", json_string(tmp));
               free(tmp);
            }
         }

         pushOutMessage(oMenu, "lmcmenu", client);

         if (total > maxElements)
            tell(eloAlways, "[LMC] Warning: %d more, only maxElements supported", total-maxElements);
      }
   }
   else if (action == "time")
   {
      int percent {getIntFromJson(oObject, "percent", na)};
      int seconds = lmc->getCurrentTrack()->duration / 100.0 * percent;
      char par[50] {};
      tell(eloAlways, "Jump to %d%% (%d seconds)", percent, seconds);
      sprintf(par, "%d", seconds);
      lmc->restQuery("time", {par});
   }
   else if (action == "play" && id.length())
      lmc->restQuery("playlist", {"index", id});   // lmc->track(atoi(id.c_str()));
   else if (action == "play")
      lmc->restQuery(action.c_str());
   else if (action == "pause")
      lmc->restQuery(action.c_str());
   else if (action == "stop")
      lmc->restQuery(action.c_str());
   else if (action == "prevTrack")
      lmc->restQuery("playlist", {"index", "+1"});
   else if (action == "nextTrack")
      lmc->restQuery("playlist", {"index", "-1"});
   else if (action == "repeat")
      lmc->restQuery("playlist", {"repeat"});
   else if (action == "shuffle")
      lmc->restQuery("playlist", {"shuffle"}); // modes; 0, 1, 2
   else if (action == "randomTracks")
      lmc->restQuery("randomplay", {"tracks"});
   else if (action == "volumeDown")
      lmc->restQuery("mixer", {"volume", "-5"});
   else if (action == "volumeUp")
      lmc->restQuery("mixer", {"volume", "+5"});
   else if (action == "muteToggle")
      lmc->restQuery("mixer", {"muting", "toggle"});
   // else if (action == "savePlaylist")
   //    lmc->restQuery("playlist", { "save", mac));
   else if (action == "changePlayer")
   {
      const char* mac = getStringFromJson(oObject, "mac");
      lmcExit();
      tell(eloLmc, "[LMC] Changing player to '%s'", mac);
      free(lmcPlayerMac);
      lmcPlayerMac = strdup(mac);
      lmcInit(true);
   }

   return done;
}
