//***************************************************************************
// Serial Interface
// File serial.h
// Date 04.11.12 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
//***************************************************************************

#pragma once

//***************************************************************************
// Include
//***************************************************************************

#include <termios.h>

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

      // object

      Serial();
      virtual ~Serial();

      // interface

      virtual int open(const char* dev = 0);
      virtual int close();
      virtual int reopen(const char* dev = 0);
      virtual int isOpen()              { return fdDevice != 0 && opened; }
      virtual int flush();

      virtual int look(byte& b, int timeoutMs = 0);
      virtual int read(void* buf, size_t count, uint timeoutMs = 0);
      virtual int write(void* line, int size = 0);

      // settings

      virtual int setTimeout(int timeout);
      virtual int setWriteTimeout(int timeout);

   protected:

      // data

      int opened;
      int readTimeout;
      int writeTimeout;
      char deviceName[100];

      int fdDevice;
      struct termios oldtio;
};
