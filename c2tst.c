
#include "unistd.h"
#include <vector>

#include "service.h"
#include "p4io.h"

//***************************************************************************
// main
//***************************************************************************

int main()
{
   const char* ttyDevice = "/dev/ttyUSB1";

   int status;
   std::vector<Fs::Parameter>::iterator it;
   P4Packet* tlg = new P4Packet();

   logstdout = yes;

   if (tlg->open(ttyDevice) != success)
   {
      tell(0, "Opening serial line '%s' failed, aborting", ttyDevice);
      return 1;
   }

   tell(0, "Opening serial line '%s' succeeded", ttyDevice);

   while (true)
   {
      // try serial read

      if ((status = tlg->read()) != success)
      {
         tell(0, "Reading serial line failed with %d, try reopen in 5 seconds");
         sleep(5);
         tlg->reopen(ttyDevice);

         continue;
      }

      // process parameters ..

      for (it = tlg->getParameters()->begin(); it != tlg->getParameters()->end(); it++)
      {
         Fs::Parameter p = *it;

         if (*p.name)
            tell(eloAlways, "(%2d) %-17s: %.2f %s", p.index, p.name, p.value, p.unit);
      }
   }

   tlg->close();

   return success;
}
