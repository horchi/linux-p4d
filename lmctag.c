/*
 * lmctag.c
 *
 * See the README file for copyright information
 *
 */

#include "lmctag.h"

//***************************************************************************
// LMC response Tags
//***************************************************************************

const char* LmcTag::tags[] =
{
   "player_name",
   "player_connected",
   "player_ip",
   "player_needs_upgrade", // Connected player needs a firmware upgrade.
   "player_is_upgrading",  // Connected player is in the process of performing a firmware update.

   "rescan",               // Returned with value 1 if the Squeezebox Server is still scanning the database. The results may therefore be incomplete. Not returned if no scan is in progress.
   "error",                // Returned with value "invalid player" if the player this subscription query referred to does no longer exist.In non subscription mode, the query simply echoes itself (i.e. produces no result) if <playerid> is wrong.

   "current_title",        // Returns the current title for remote streams. Only if remote stream is playing
   "sleep",                // If set to sleep, the amount (in seconds) it was set to.
   "will_sleep_in",        // Seconds left until sleeping. Only if set to sleep
   "sync_master",          // ID of the master player in the sync group this player belongs to. Only if synced.
   "sync_slaves",          // Comma-separated list of player IDs, slaves to sync_master in the sync group this player belongs to. Only if synced.

   "power",
   "signalstrength",
   "mode",
   "time",
   "rate",
   "duration",
   "can_seek",

   "mixer volume",
   "mixer treble",	      // Only for SliMP3 and Squeezebox1 players.
   "mixer bass",	         // Only for SliMP3 and Squeezebox1 players.
   "mixer pitch",	         // Only for Squeezebox1 players.

   "digital_volume_control",

   "playlist repeat",
   "playlist shuffle",
   "playlist mode",
   "seq_no",

   "playlist_id",	         // Playlist id, if the current playlist is a stored playlist.
   "playlist_name",        // Playlist name, if the current playlist is a stored playlist. Equivalent to "playlist name ?".
   "playlist_cur_index",
   "playlist_timestamp",
   "playlist_modified",    // Modification state of the saved playlist (if the current playlist is one). Equivalent to "playlist modified ?".
   "playlist_tracks",

   // tags of status querys track list

   "start of track tags",  // dummy entry !!

   "playlist index",
   "id",
   "title",
   "artist",
   "genre",
   "duration",
   "coverid",
   "artwork_track_id",
   "bitrate",
   "album",
   "year",
   "url",

   // tags of list range queries like 'genre 0 10'

   "count",
   "playlist",
   "genre_id",
   "artist_id",
   "album_id",
   "track",
   "track_id",
   "artwork_url",
   "remote",               // 1 if a remote stream is currently playing.
   "remote_title",         // Returns the current title for remote streams. Only if remote stream is playing
   "remoteMeta",
   "type",                 // tContentType
   "lyrics",
   "radio",

   "name",
   "type",
   "icon",
   "cmd",
   "weight",
   "sort",
   "isaudio",
   "hasitems",
   "item_id",
   "image",
   "waitingToPlay",

   "canpoweroff",
   "connected",
   "displaytype",
   "firmware",
   "ip",
   "isplayer",
   "isplaying",
   "model",
   "modelname",
   "playerid",
   "playerindex",
   "uuid",

   0
};

int LmcTag::toTag(const char* name, int track)
{
   if (isEmpty(name))
      return tUnknown;

   for (int i = track ? tStartOfTrackTags+1 : 0; i < tCount; i++)
   {
      if (strcmp(tags[i], name) == 0)
         return i;
   }

   tell(eloAlways, "Info: Ignoring unexpected tag '%s'", name);

   return tUnknown;
}

const char* LmcTag::toName(int tag, int track)
{
   if (!isValid(tag))
      return "<unknown>";

   return tags[tag];
}

//***************************************************************************
// Set
//***************************************************************************

int LmcTag::set(const char* data)
{
   if (!data)
      return fail;

   free(buffer);
   buffer = strdup(data);
   pos = buffer;

   return success;
}

//***************************************************************************
// Get Next
//***************************************************************************

int LmcTag::getNext(int& tag, char* value, unsigned short max, int track)
{
   int status;
   char name[100+TB];

   if ((status = getNext(name, value, max)) != success)
      return status;

   tag = toTag(name, track);

   return tag != tUnknown ? success : (int)wrnUnknownTag;
}

int LmcTag::getNext(char* name, char* value, unsigned short max)
{
   int status;
   char* v;

   *name = 0;
   *value = 0;

   if ((status = getNextToken()) == success)
   {
      lmc->unescape(token);

      if ((v = strchr(token, ':')))
      {
         if (strlen(v+1) > max)
            tell(eloDetail, "Info: Value '%s' exceed max size, truncated", v+1);

         strncpy(value, v+1, max);
         value[max] = 0;
         *v = 0;
      }

      strcpy(name, token);
   }

   return status;
}

//***************************************************************************
// Get Next Token
//***************************************************************************

int LmcTag::getNextToken()
{
   if (!pos || !*pos)
      return wrnEndOfPacket;

   int len = 0;
   char* end = strchr(pos, ' ');

   len = end ? end-pos : strlen(pos);
   snprintf(token, sizeof(token), "%.*s", len, pos);

   pos = end ? end+1 : 0;

   return success;
}
