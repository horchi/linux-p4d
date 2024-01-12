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

int Daemon::lmcInit()
{
   if (isEmpty(lmcHost) || isEmpty(lmcPlayerMac))
      return done;

   static time_t lastTryAt {0};

   if (lastTryAt > time(0) - 10)
      return done;

   lastTryAt = time(0);

   if (!lmc)
      lmc = new LmcCom(lmcPlayerMac);

   if (lmc->open(lmcHost, lmcPort) != success)
   {
      tell(eloAlways, "[LMC] Opening connection to server at '%s:%d' failed", lmcHost, lmcPort);
      return fail;
   }

   tell(eloAlways, "[LMC] Opening connection to server at '%s:%d' succeeded", lmcHost, lmcPort);

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
      tell(eloAlways, "[LMC] Changes pending");
      lmcUpdates();
   }

   return success;
}

int Daemon::lmcUpdates(long client)
{
   if (!lmc)
      return done;

   json_t* oJson = json_object();
   lmcTrack2Json(oJson, lmc->getCurrentTrack());
   lmcPlayerState2Json(oJson);
   lmcPlaylist2Json(oJson);
   lmcMainMenu2Json(oJson);

   return pushOutMessage(oJson, "lmcdata", client);
}

//***************************************************************************
// LMC Track To Json
//***************************************************************************

int Daemon::lmcTrack2Json(json_t* obj, TrackInfo* track)
{
   tell(eloDebug, "[LMC] Track: %s / %s / %s / %s ", track->title, track->artist, track->genre, track->album);

   // current track

   json_t* current = json_object();
   json_object_set_new(obj, "current", current);

   json_object_set_new(current, "title", json_string(track->title));
   json_object_set_new(current, "artist", json_string(track->artist));
   json_object_set_new(current, "genre", json_string(track->genre));
   json_object_set_new(current, "album", json_string(track->album));

   json_object_set_new(current, "duration", json_integer(track->duration));

   json_object_set_new(current, "bitrate", json_integer(track->bitrate));
   json_object_set_new(current, "contentType", json_string(track->contentType));
   json_object_set_new(current, "year", json_integer(track->year));
   json_object_set_new(current, "lyrics", json_string(track->lyrics));
   // json_object_set_new(current, "id", json_integer(track->id));

   std::string url;
   lmc->getCurrentCoverUrl(track, url);

   tell(eloAlways, "[LMC] Artworkurl: %s", track->artworkurl);
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
   json_object_set_new(state, "plName", json_string(currentState->plName));
   json_object_set_new(state, "plCount", json_integer(currentState->plCount));
   json_object_set_new(state, "plShuffle", json_integer(currentState->plShuffle));
   json_object_set_new(state, "plRepeat", json_integer(currentState->plRepeat));
   json_object_set_new(state, "muted", json_boolean(currentState->muted));
   json_object_set_new(state, "mode", json_string(currentState->mode));
   json_object_set_new(state, "trackTime", json_integer(currentState->trackTime));
   json_object_set_new(state, "volume", json_integer(currentState->volume / (100 / 100.0)));

   static int lastTime {0};

   tell(eloDebug, "[LMC] Mmode: %s", currentState->mode);

   if (strcmp(currentState->mode, "play") == 0)
      lastTime = currentState->trackTime + (cTimeMs::Now() - currentState->updatedAt) / 1000;

   json_object_set_new(state, "trackTime", json_integer(lastTime));

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

      json_object_set_new(oTrack, "title", json_string(track->title));
      json_object_set_new(oTrack, "artist", json_string(track->artist));
      json_object_set_new(oTrack, "album", json_string(track->album));

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
   { "Play random tracks", LmcCom::rqtUnknown },
   { "Playlists",          LmcCom::rqtPlaylists },
   { "Radios",             LmcCom::rqtRadios },
   { "Favorites",          LmcCom::rqtFavorites },
   { "Favorites",          LmcCom::rqtFavorites },

//   { "", LmcCom::rqtUnknown }
};

int Daemon::lmcMainMenu2Json(json_t* obj)
{
   json_t* oMenu = json_array();
   json_object_set_new(obj, "menu", oMenu);

   for (const auto& item : menuItems)
   {
      json_t* oItem = json_object();
      json_array_append_new(oMenu, oItem);

      json_object_set_new(oItem, "name", json_string(item.name));
      json_object_set_new(oItem, "id", json_integer(item.queryType));
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

   std::string action = getStringFromJson(oObject, "action", "");
   int index = getIntFromJson(oObject, "index", na);
   LmcCom::RangeQueryType queryType = (LmcCom::RangeQueryType)getIntFromJson(oObject, "query", na);

   tell(eloAlways, "[LMC] Action '%s' (%d)", action.c_str(), queryType);

   if (action == "menu" && queryType != LmcCom::rqtUnknown)
   {
      static int maxElements {50000};
      LmcCom::Parameters filters;
      LmcCom::RangeList list;
      int total {0};

      if (queryType == LmcCom::rqtNewMusic)
         filters.push_back("sort:new");

      json_t* oMenu = json_array();

      tell(eloAlways, "[LMC] Query (%d)", queryType);

      if (lmc->queryRange(queryType, 0, maxElements, &list, total, "", &filters) == success)
      {
         LmcCom::RangeList::iterator it;

         for (it = list.begin(); it != list.end(); ++it)
         {
            json_t* oItem = json_object();
            json_array_append_new(oMenu, oItem);

            json_object_set_new(oItem, "name", json_string((*it).content.c_str()));
            json_object_set_new(oItem, "id", json_string((*it).id.c_str()));
         }

         pushOutMessage(oMenu, "lmcmenu", client);

         if (total > maxElements)
            tell(eloAlways, "[LMC] Warning: %d more, only maxElements supported", total-maxElements);
      }
   }
   else if (action == "play" && index != na)
      lmc->track(index);
   else if (action == "play")
      lmc->play();
   else if (action == "pause")
      lmc->pause();
   else if (action == "stop")
      lmc->stop();

   else if (action == "prevTrack")
      lmc->prevTrack();
   else if (action == "nextTrack")
      lmc->nextTrack();

   else if (action == "repeat")
      lmc->repeat();
   else if (action == "shuffle")
      lmc->randomTracks();
   // lmc->shuffle();  //       lmc->randomTracks();

   else if (action == "volumeDown")
      lmc->volumeDown();
   else if (action == "volumeUp")
      lmc->volumeUp();
   else if (action == "muteToggle")
      lmc->muteToggle();

   return done;
}
