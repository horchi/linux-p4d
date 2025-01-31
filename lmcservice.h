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
      uint64_t updatedAt {0};

      std::string mode;
      std::string version;
      int volume {0};
      int muted {0};
      int trackTime {0};             // playtime of the current song

      // playlist state

      std::string plName;
      int plIndex {0};
      int plCount {0};
      int plShuffle {0};
      int plRepeat {0};
};

struct TrackInfo
{
      uint64_t updatedAt {0};

      std::string genre;
      std::string year;
      std::string album;
      std::string artist;
      std::string title;
      std::string artworkTrackId;
      std::string artworkUrl;
      std::string remoteTitle;
      std::string contentType;
      std::string lyrics;
      std::string file;

      unsigned int bitrate {0};
      unsigned short remote {0};

      int index {0};             // index in current playlist
      int id {0};                // track ID
      int duration {0};
};

#define SQUEEZEBOX_CURRENT_TRACK    "squeezebox_current_track-v1.0"
