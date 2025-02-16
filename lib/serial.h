//***************************************************************************
// Serial Interface
// File serial.h
// Date 04.11.12 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
//***************************************************************************

#pragma once

#define _NEW_TERMIOS

#ifndef _NEW_TERMIOS
#  include <termios.h>
#else
#  include <sys/ioctl.h>
#  include <asm/termbits.h>
#endif

#include "common.h"

//***************************************************************************
// IO Interface
//***************************************************************************

class Serial
{
   public:

      enum Misc
      {
         sizeCmdMax = 100,

         errReadFailed = -200,
         errCountMissmatch,
         wrnTimeout
      };

      Serial(int aBaud, int aCflags = 0);
      virtual ~Serial();

      // interface

      virtual int open(const char* dev = nullptr);
      virtual int close();
      virtual int reopen(const char* dev = nullptr);
      virtual int isOpen()              { return fdDevice != 0 && opened; }
      virtual int flush();

      virtual int look(byte& b, int timeoutMs = 0);
      virtual int read(void* buf, size_t count, uint timeoutMs = 0);
      virtual int readByte(byte& v, int timeoutMs = 100);
      virtual int readWord(word& v, int timeoutMs = 100);
      virtual int readSWord(sword& v, int timeoutMs = 100);
      virtual int write(void* line, int size = 0);
      virtual int write(byte b);

      // settings

      virtual void setSpecialSpeed(int s)       { specialSpeed = s; }
      // virtual void setBaud(int b)               { baud = b; }
      virtual void setTimeout(int timeout)      { readTimeout = timeout; }
      virtual void setWriteTimeout(int timeout) { writeTimeout = timeout; }

   protected:

      // data

      bool opened {false};
      int readTimeout {10};
      int writeTimeout {10};
      int baud {0};
      int specialSpeed {0};
      int cflags {0};
      char deviceName[100] {};

      int fdDevice {0};
      // struct termios oldtio;
};
