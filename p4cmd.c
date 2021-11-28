//***************************************************************************
// p4d / Linux - Heizungs Manager
// File p4cmd.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 19.11.2013  Jörg Wendel
//***************************************************************************

#include <unistd.h>
#include <dirent.h>

#include "lib/common.h"
#include "p4io.h"
// #include "w1.h"

//***************************************************************************
// Ask Confirm
//***************************************************************************

bool askConfirm(const char* message)
{
   char buffer[1024+TB] = "";

   tell(eloAlways, "%s%sContinue? (YES|no) ", message, !isEmpty(message) ? "\n" : "");

   fgets(buffer, 1000, stdin);
   buffer[strlen(buffer)-1] = 0;

   if (!isEmpty(buffer) && strcmp(buffer, "yes") != 0)
   {
      tell(eloAlways, "Aborted!");
      return false;
   }

   return true;
}

//***************************************************************************
// Choice
//***************************************************************************

enum UserCommand
{
   ucUnknown = na,
   ucGetValue,
   ucGetParameter,
   ucSetParameter,
   ucGetTimeRanges,
   ucSetTimeRanges,
   ucParameterList,
   ucValueList,
   ucErrorList,
   ucMenuList,
   ucState,
   ucSyncTime,
   ucGetDo,
   ucGetAo,
   ucUser,
//   ucShowW1,
   ucUnkonownList
};

void showUsage(const char* bin)
{
   printf("Usage: %s <command> [-a <address> [-v <value>]] [-o <offset>] [-l <log-level>] [-d <device>]\n", bin);
   printf("\n");
   printf("  options:\n");
   printf("     -a <address>    address of parameter or value\n");
   printf("     -v <value>      new value\n");
   printf("     -l <log-level>  set log level\n");
   printf("     -d <device>     serial device file (defaults to /dev/ttyUSB0)\n");
   printf("     -o <offset>     optional offset for time sync in seconds\n");

   printf("\n");
   printf("  commands:\n");
   printf("     state    show state\n");
   printf("     tsync    set s-3200 time to systime\n");
   printf("     errors   show error buffer\n");
   printf("     values   list value addrsses\n");
   printf("     menu     list menu items (available parameters)\n");
   printf("     getv     show value at <addr>\n");
   printf("     getp     show parameter at <addr>\n");
   printf("     setp     set parameter at <addr> to <value>\n");
   printf("     times    get time ranges of <addr>\n");
   printf("     getdo    show digital output at <addr>\n");
   printf("     getao    show analog output at <addr>\n");
//   printf("     w1       show data of all connected one wire sensors\n");
}

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char** argv)
{
   Serial serial;
   int status;
   byte b;
   word addr = Fs::addrUnknown;
   int offset = 0;
   const char* value {nullptr};
   UserCommand cmd = ucUnknown;
   const char* device = "/dev/ttyUSB0";

//    {
//       md5Buf defaultPwd;

//       // init default user and password

//       createMd5("p4-3200", defaultPwd);
//       printf("'%s'\n", defaultPwd);
//       return done;
//    }

   Sem sem(0x3da00001);

   loglevel = 0;
   logstdout = yes;

   // usage ..

   if (argc <= 1 || (argv[1][0] == '?' || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)))
   {
      showUsage(argv[0]);
      return 0;
   }

   // get command

   if (strcasecmp(argv[1], "getv") == 0)
      cmd = ucGetValue;
   // else if (strcasecmp(argv[1], "w1") == 0)
   //    cmd = ucShowW1;
   else if (strcasecmp(argv[1], "getp") == 0)
      cmd = ucGetParameter;
   else if (strcasecmp(argv[1], "setp") == 0)
      cmd = ucSetParameter;
   else if (strcasecmp(argv[1], "times") == 0)
      cmd = ucGetTimeRanges;
   else if (strcasecmp(argv[1], "stimes") == 0)
      cmd = ucSetTimeRanges;
   else if (strcasecmp(argv[1], "values") == 0)
      cmd = ucValueList;
   else if (strcasecmp(argv[1], "errors") == 0)
      cmd = ucErrorList;
   else if (strcasecmp(argv[1], "menu") == 0)
      cmd = ucMenuList;
   else if (strcasecmp(argv[1], "state") == 0)
      cmd = ucState;
   else if (strcasecmp(argv[1], "tsync") == 0)
      cmd = ucSyncTime;
   else if (strcasecmp(argv[1], "getdo") == 0)
      cmd = ucGetDo;
   else if (strcasecmp(argv[1], "getao") == 0)
      cmd = ucGetAo;
   else if (strcasecmp(argv[1], "user") == 0)
      cmd = ucUser;
   else if (strcasecmp(argv[1], "list") == 0)
      cmd = ucUnkonownList;
   else
   {
      showUsage(argv[0]);
      return 0;
   }

   // if (cmd == ucShowW1)
   // {
   //    W1 w1;

   //    if (w1.scan() == success)
   //    {
   //       w1.update();
   //       w1.show();
   //    }

   //    return 0;
   // }

   // parse options

   for (int i = 1; argv[i]; i++)
   {
      if (argv[i][0] != '-' || strlen(argv[i]) != 2)
         continue;

      switch (argv[i][1])
      {
         case 'o': if (argv[i+1]) offset = strtol(argv[++i], 0, 0);  break;
         case 'a': if (argv[i+1]) addr = strtol(argv[++i], 0, 0);    break;
         case 'v': if (argv[i+1]) value = argv[++i];                 break;
         case 'l': if (argv[i+1]) loglevel = atoi(argv[++i]);        break;
         case 'd': if (argv[i+1]) device = argv[++i];                break;
      }
   }

   if (cmd == ucUnknown)
      return 1;

   if (loglevel > 0)
      logstamp = yes;

   int debugMode = strcmp(device, "-") == 0;

   P4Request request(&serial);

   if (!debugMode)
   {
      sem.p();

      if (serial.open(device) != success)
         return 1;

      while (serial.look(b, 100) == success)
         tell(eloDebug, "-> 0x%2.2x", b);

      // connection check

      if (request.check() != success)
      {
         serial.close();
         return 1;
      }
   }

   switch (cmd)
   {
      case ucUser:
      {
         request.getUser(addr);

         break;
      }

      case ucGetDo:
      {
         Fs::IoValue v(addr);

         if (request.getDigitalOut(&v) == success)
            tell(eloAlways, "mode %c; state %d", v.mode, v.state);

         break;
      }

      case ucGetAo:
      {
         Fs::IoValue v(addr);

         if (request.getAnalogOut(&v) == success)
         {
            if (v.mode == 0xff)
               tell(eloAlways, "mode A; value %d", v.state);
            else
               tell(eloAlways, "mode %d; value %d", v.mode, v.state);
         }

         break;
      }

      case ucState:
      {
         Fs::Status s;

         if (request.getStatus(&s) == success)
         {
            tell(eloAlways, "Version: %s", s.version);
            tell(eloAlways, "Time: %s", l2pTime(s.time, "%A, %d. %b. %Y %H:%M:%S").c_str());
            tell(eloAlways, "%d - %s", s.mode, s.modeinfo);
            tell(eloAlways, "%d - %s", s.state, s.stateinfo);
         }

         break;
      }

      case ucSyncTime:
      {
         if (request.syncTime(offset) == success)
            tell(eloAlways, "success");
         else
            tell(eloAlways, "failed");
      }

      case ucGetParameter:
      {
         Fs::ConfigParameter p(addr);

         if (request.getParameter(&p) == success)
         {
            p.show();
            tell(eloAlways, "=> %.*f%s", p.digits, p.rValue, p.unit);
         }

         break;
      }
      case ucSetParameter:
      {
         if (!value)
         {
            tell(eloAlways, "Missing value, aborting");
            break;
         }

         Fs::ConfigParameter p(addr);

         if (request.getParameter(&p) != success)
         {
            tell(eloAlways, "Set of parameter failed, query of current setting failed!");
            break;
         }

         p.setValueDirect(value, p.digits, p.getFactor());

         tell(eloAlways, "Set parameter to:");
         p.show();
         if (!askConfirm(""))
            break;

         if ((status = request.setParameter(&p)) == success)
         {
            tell(eloAlways, "Parameter 0x%4.4X changed successfully to:", p.address);
            p.show();
         }
         else
            tell(eloAlways, "Set of parameter failed, error was %d", status);

         break;
      }
      case ucGetTimeRanges:
      {
         Fs::TimeRanges t;

         for (status = request.getFirstTimeRanges(&t); status != Fs::wrnLast; status = request.getNextTimeRanges(&t))
         {
            for (int n = 0; n < 4; n++)
               tell(eloAlways, "  Range %d: %s [0x%02x]", n+1, t.getTimeRange(n), t.address);

            tell(eloAlways, "-------------------");
         }

         break;
      }
      case ucSetTimeRanges:
      {
         Fs::TimeRanges t(0x38);

         t.setTimeRange(0, "05:30", "22:00");
         tell(eloAlways, "DEBUG: '%s'", t.getTimeRange(0));
         tell(eloAlways, "%d %d", t.timesFrom[0], t.timesTo[0]);

         t.setTimeRange(1, "nn:nn", "nn:nn");
         tell(eloAlways, "DEBUG: '%s'", t.getTimeRange(1));
         tell(eloAlways, "%d %d", t.timesFrom[1], t.timesTo[1]);

         tell(eloAlways, "Set of time ranges not implementd yet!");

         // request.setTimeRanges(&t);

         break;
      }
      case ucGetValue:
      {
         Fs::Value v(addr);

         if ((status = request.getValue(&v)) == success)
            tell(eloAlways, "value 0x%x is %d / %d", v.address, v.value, (word)v.value);
         else
            tell(eloAlways, "Getting value '%d' failed, error %d", v.address, status);

         break;
      }
      case ucValueList:
      {
         int status;
         Fs::ValueSpec v;
         int n = 0;

         for (status = request.getFirstValueSpec(&v); status != Fs::wrnLast; status = request.getNextValueSpec(&v))
         {
            if (status == success)
               tell(eloAlways, "%3d) 0x%04x %4d '%s' (%04d) '%s'",
                    n, v.address, v.factor, v.unit, v.type, v.description);
            else
               tell(eloAlways, "%3d) <empty>", n);

            n++;
         }

         break;
      }
      case ucErrorList:
      {
         int status;
         Fs::ErrorInfo e;
         char* ct = 0;

         for (status = request.getFirstError(&e); status == success; status = request.getNextError(&e))
         {
            ct = strdup(ctime(&e.time));
            ct[strlen(ct)-1] = 0;   // remove linefeed of ctime()

            tell(eloAlways, "%s:  %03d/%03d  '%s' - %s",
                 ct, e.number, e.info, e.text, Fs::errState2Text(e.state));

            free(ct);
         }

         break;
      }
      case ucMenuList:
      {
         int status;
         Fs::MenuItem m;
         int n = 0;

         for (status = request.getFirstMenuItem(&m); status != Fs::wrnLast; status = request.getNextMenuItem(&m))
         {
            if (status == success)
               tell(eloAlways, "%3d) Address: 0x%04x, parent: 0x%04x, child: 0x%04x; '%s'",
                    n++, m.address, m.parent, m.child, m.description);
            else if (status != Fs::wrnSkip)
               break;
         }

         if (status == Fs::wrnTimeout)
            tell(eloAlways, "Aborting on timeout");
         else if (status != Fs::wrnLast && status != success)
            tell(eloAlways, "Aborting on error, status was %d", status);

         break;
      }
      case ucUnkonownList:
      {
         int status;

         for (status = request.getItem(yes); status != Fs::wrnLast; status = request.getItem(no))
            ;

         break;
      }


      default: break;
   }

   if (!debugMode)
   {
      serial.close();
      sem.v();
   }

   return 0;
}
