
//***************************************************************************
// Group VDR/GraphTFT
// File tcpchannel.h
// Date 31.10.06
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
// (c) 2006-2008 Jörg Wendel
//--------------------------------------------------------------------------
// Class TcpChannel
//***************************************************************************

#pragma once

// #include <thread.h>

#include "common.h"

//***************************************************************************
// Class TcpChannel
//***************************************************************************

class TcpChannel
{
   public:

     enum Errors
      {
         errChannel = -100,

         errUnknownHostname,      // 99
         errBindAddressFailed,    // 98
         errAcceptFailed,
         errListenFailed,
         errConnectFailed,        // 95
         errIOError,              // 94
         errConnectionClosed,     // 93
         errInvalidEndpoint,
         errOpenEndpointFailed,   // 91

         // Warnungen

         wrnNoEventPending,       // 90
         errUnexpectedEvent,      // 89
         wrnChannelBlocked,       // 88
         wrnNoConnectIndication,  // 87
         wrnNoResponseFromServer, // 86
         wrnNoDataAvaileble,      // 85
         wrnSysInterrupt,         // 84
         wrnTimeout               // 83
      };

#pragma pack(1)
      struct Header
      {
         int command;
         int size;
      };
#pragma pack()

		TcpChannel(int aTimeout = 2, int aHandle = 0);
		~TcpChannel();

      int openLstn(unsigned short aPort, const char* aLocalHost = 0);
      int open(unsigned short aPort, const char* aHost);
      int isOpen() { return handle > 0; }
      int flush();
      int close();
		int listen(TcpChannel*& child);
      int look(uint64_t aTimeout = 0);
      int read(char* buf, int bufLen, int ln = no);
      char* readln();

      int writeCmd(int command, const char* buf = 0, int bufLen = 0);
      int write(const char* buf, int bufLen = 0);

      int isConnected()    { return handle != 0; }
      int getHandle()      { return handle; }

   private:

      int checkErrno();

      // data

      int handle {0};
      unsigned short port {0};
      char localHost[100] {};
      char remoteHost[100] {};
      long localAddr {0};
      long remoteAddr {0};
      long timeout {0};
      int lookAheadChar {0};
      int lookAhead {0};
      int nTtlReceived {0};
      int nTtlSent {0};

      char* readBuffer {};
      int readBufferSize {0};
      int readBufferPending {0};

#ifdef VDR_PLUGIN
      cMutex _mutex;
#endif
};
