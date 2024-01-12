/*
 * service.h
 *
 * See the README file for copyright information
 *
 */

#pragma once

#include <inttypes.h>

#include "lib/common.h"

struct PlayerState
{
   PlayerState() { memset(this, 0, sizeof(PlayerState)); }

      uint64_t updatedAt {0};

      char mode[20+TB] {};
      char version[50+TB] {};
      int volume {0};
      int muted {0};
      int trackTime {0};             // playtime of the current song

      // playlist state

      char plName[300+TB] {};
      int plIndex {0};
      int plCount {0};
      int plShuffle {0};
      int plRepeat {0};
};

struct TrackInfo
{
   TrackInfo() { memset(this, 0, sizeof(TrackInfo)); }

      uint64_t updatedAt {0};

      char genre[100+TB] {};
      char album[100+TB] {};
      char artist[100+TB] {};
      char title[200+TB] {};
      char artworkTrackId[500+TB] {};
      char artworkurl[500+TB] {};
      char remoteTitle[500+TB] {};
      char contentType[100+TB] {};
      char lyrics[10000+TB] {};

      unsigned int bitrate {0};
      unsigned short remote {0};
      unsigned int year {0};

      int index {0};             // index in current playlist
      int id {0};                // track ID
      int duration {0};
};

#define SQUEEZEBOX_CURRENT_TRACK    "squeezebox_current_track-v1.0"
