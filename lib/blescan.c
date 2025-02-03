
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "common.h"

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char **argv)
{
   char addr[19] {};
   char name[248] {};

   int dev_id = hci_get_route(nullptr);
   int sock = hci_open_dev( dev_id );

   if (dev_id < 0 || sock < 0)
   {
      perror("opening socket");
      exit(1);
   }

   int len {8};
   int max_rsp = 255;
   int flags = IREQ_CACHE_FLUSH;
   inquiry_info* ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
   int numRsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);

   if (numRsp < 0)
      perror("hci_inquiry");

   tell(eloAlways, "Found %d devices", numRsp);

   for (uint i = 0; i < numRsp; i++)
   {
      ba2str(&(ii+i)->bdaddr, addr);
      memset(name, 0, sizeof(name));

      if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), name, 0) < 0)
         strcpy(name, "[unknown]");

      printf("%s  %s\n", addr, name);
   }

   free(ii);
   close(sock);

   return 0;
}
