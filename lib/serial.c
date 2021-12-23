//***************************************************************************
// Serial Interface
// File serial.c
// Date 04.11.13 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "serial.h"

//***************************************************************************
// Object
//***************************************************************************

Serial::Serial()
{
   fdDevice = 0;
   opened = no;
   readTimeout = 10;
   writeTimeout = 10;

   bzero(&oldtio, sizeof(oldtio));
}

Serial::~Serial()
{
   close();
}

//***************************************************************************
// Settings
//***************************************************************************

int Serial::setTimeout(int timeout)
{
   readTimeout = timeout;
   return success;
}

int Serial::setWriteTimeout(int timeout)
{
   writeTimeout = timeout;
   return success;
}

//***************************************************************************
// Open Device
//***************************************************************************

int Serial::open(const char* dev)
{
   struct termios newtio;

   if (!dev)
      dev = deviceName;
   else
      strcpy(deviceName, dev);

   if (isEmpty(deviceName))
   {
      tell(eloAlways, "Error: Missing device name, can't open serial line");
      return fail;
   }

   if (isOpen())
      close();

   // open serial line

   if ((fdDevice = ::open(deviceName, O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
   {
      fdDevice = 0;

      tell(eloAlways, "Error: Opening '%s' failed, errno was (%d) '%s'",
           deviceName, errno, strerror(errno));

      return fail;
   }

   tell(eloDetail, "Opening '%s' succeeded!", deviceName);

   // configure serial line with 8 data bits, no parity, 1 stop bit

   tcgetattr(fdDevice, &oldtio);
   bzero(&newtio, sizeof(newtio));

   /* BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
      CRTSCTS : output hardware flow control (only used if the cable has
                all necessary lines. See sect. 7 of Serial-HOWTO)
      CS8     : 8n1 (8bit,no parity,1 stopbit)
      CLOCAL  : local connection, no modem control
      CREAD   : enable receiving characters  */

   newtio.c_cflag = B57600 | CS8 | CLOCAL | CREAD;
   newtio.c_iflag = IGNPAR;  // don't set ICRNL !!
   newtio.c_oflag = 0;
   newtio.c_lflag = 0;       // ICANON - 'disable echo functionality  and don't
                             //           send signals to the calling prozess'

   if (tcsetattr(fdDevice, TCSANOW, &newtio) < 0)
   {
      tell(eloAlways, "Setting inteface parameter failed, errno was (%d) '%s'!",
           errno, strerror(errno));

      ::close(fdDevice);
      fdDevice = 0;

      return fail;
   }

   flush();
   opened = yes;

   return success;
}

//***************************************************************************
// Close Device
//***************************************************************************

int Serial::close()
{
   // restore the old settings and close

   if (fdDevice)
   {
      tell(eloDetail, "Closing io device");

      flush();

      tcsetattr(fdDevice, TCSANOW, &oldtio);
      ::close(fdDevice);

      fdDevice = 0;
   }

   opened = no;

   return success;
}

int Serial::flush()
{
   tcflush(fdDevice, TCIFLUSH);

   return done;
}

//***************************************************************************
// Re Open
//***************************************************************************

int Serial::reopen(const char* dev)
{
   close();

   return open(dev);
}

//***************************************************************************
// Read Value
//***************************************************************************

int Serial::look(byte& b, int timeoutMs)
{
   int res;
   b = 0;

   if (!fdDevice)
   {
      tell(eloAlways, "Warning device not opened, can't read line");
      return fail;
   }

   while ((res = read(&b, 1, timeoutMs)) < 0)
   {
      if (res == wrnTimeout)
         return wrnTimeout;

      if (errno == EINTR)
         continue;

      tell(eloAlways, "Read failed, errno was %d '%s'", errno, strerror(errno));

      return fail;
   }

   return res != 1 ? fail : success;
}

//***************************************************************************
// Send Command
//***************************************************************************

int Serial::write(void* line, int size)
{
   if (!fdDevice)
   {
      tell(eloAlways, "Warning device not opened, can't write line");
      return fail;
   }

   if (!line)
      return fail;

   // #TODO: is a loop needed if write
   //        was interuppted by system or full buffer ??

   if (::write(fdDevice, line, size) != size)
      return fail;

   return success;
}

//***************************************************************************
// Read
//   returns
//     -  count of read bytes on success
//     -  or <0 on error or count missmatch
//***************************************************************************

int Serial::read(void* buf, size_t count, uint timeoutMs)
{
   size_t nRead {0};
   int res;
   uint64_t start = cTimeMs::Now();

   if (!fdDevice)
   {
      tell(eloAlways, "Warning device not opened, can't read line");
      return fail;
   }

   while (nRead < count)
   {
      if (cTimeMs::Now() > start + timeoutMs)
         return wrnTimeout;

      res = ::read(fdDevice, (char*)buf+nRead, count-nRead);

      if (res < 0)
      {
         tell(eloAlways, "Error read failed, '%s'", strerror(errno));
         return errReadFailed;
      }

      if (!res)
         usleep(2000);

      nRead += res;
   };

   if (nRead != count)
   {
      tell(eloAlways, "Error: Serial read failrd, got %zd bytes instead of %zd", nRead, count);
      return errCountMissmatch;
   }

   return nRead;
}

