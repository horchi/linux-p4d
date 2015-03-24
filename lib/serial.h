//***************************************************************************
// Serial Interface
// File serial.h
// Date 04.11.12 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
//***************************************************************************

#ifndef _IO_SERIAL_H_
#define _IO_SERIAL_H_

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

         wrnTimeout = -10
      };

      // object

      Serial();
      virtual ~Serial();

      // interface

      virtual int open(const char* dev = 0);
      virtual int close();
      virtual int reopen(const char* dev = 0);
      virtual int isOpen()              { return fdDevice != 0 && opened; }
      virtual int look(byte& b, int timeout = 0);
      virtual int flush();
      virtual int write(void* line, int size = 0);

      // settings

      virtual int setTimeout(int timeout);
      virtual int setWriteTimeout(int timeout);

   protected:

      virtual int read(void* buf, unsigned int count, int timeout = 0);

      // data

      int opened;
      int readTimeout;
      int writeTimeout;
      char deviceName[100];

      int fdDevice;
      struct termios oldtio;
};

//***************************************************************************
#endif // _IO_SERIAL_H_
