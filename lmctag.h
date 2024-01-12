/*
 * lmctag.h
 *
 * See the README file for copyright information
 *
 */

#pragma once

#include "lib/common.h"

#include "lmccom.h"

//***************************************************************************
// LmcTag
//***************************************************************************

class LmcTag
{
   public:

      enum Errors
      {
         errorFist = -20000,

         wrnUnknownTag,
         wrnEndOfPacket
      };

      enum Tag
      {
         tUnknown = na,

         // tags of status query

         tPlayerName,
         tPlayerConnected,
         tPlayerIp,
         tPlayerNeedsUpgrade,
         tPlayerIsUpgrading,

         rRescan,
         tError,

         tCurrentTitle,
         tSleep,
         tSleepIn,
         tSyncMaster,
         tSyncSlaves,

         tPower,
         tSignalstrength,
         tMode,
         tTime,
         tRate,
         tDuration,
         tCanSeek,

         tMixerVolume,
         tMixerTreble,
         tMixerBass,
         tMixerPitch,

         tDigitalVolumeControl,

         tPlaylistRepeat,
         tPlaylistShuffle,
         tPlaylistMode,
         tSeqNo,

         tPlaylistId,
         tPlaylistName,
         tPlaylistCurIndex,
         tPlaylistTimestamp,
         tPlaylistModified,
         tPlaylistTracks,

         // tags of status querys track list

         tStartOfTrackTags,

         tPlaylistIndex,
         tId,
         tTitle,
         tArtist,
         tGenre,
         tTrackDuration,
         tCoverid,
         tArtworkTrackId,
         tBitrate,
         tAlbum,
         tYear,
         tUrl,

         // tags of list range queries like 'genre 0 10'

         tItemCount,
         tPlaylist,
         tGenreId,
         tArtistId,
         tAlbumId,
         tTrack,
         tTrackId,
         tArtworkUrl,
         tRemote,
         tRemoteTitle,
         tRemoteMeta,
         tContentType,
         tLyrics,

         // are this tags :o, at leased used for the menu struct

         tRadio,

         // radio adon tags

         tName,
         tType,
         tIcon,
         tCmd,
         tWeight,
         tSort,
         tIsAudio,
         tHasItems,
         tItemId,
         tImage,
         tWaitingToPlay,

         // Players

         tCanpoweroff,
         tConnected,
         tDisplaytype,
         tIirmware,
         tIp,
         tIsplayer,
         tIsplaying,
         tModel,
         tMdelname,
         tPlayerid,
         tPlayerindex,
         tUuid,

         // technical stuff ..

         tCount
      };

      static const char* tags[];
      static int toTag(const char* name, int track = no);
      static const char* toName(int tag, int track = no);
      static int isValid(int tag) { return (tag > tUnknown && tag < tCount); }

      LmcTag(LmcCom* l) { lmc = l; }
      ~LmcTag()         { free(buffer); }

      int set(const char* data);
      int getNext(int& tag, char* value, unsigned short max, int track = no);
      int getNext(char* name, char* value, unsigned short max);

   protected:

      int getNextToken();

      char token[2000+TB] {};
      char* buffer {};
      char* pos {};
      LmcCom* lmc {};   // only to call unescape() -> #TODO redisign later?
};
