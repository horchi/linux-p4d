//***************************************************************************
// Home Systemd Interface / Test
// File sdclient.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 24.02.2024 - Jörg Wendel
//***************************************************************************

#include "systemdctl.h"

#include "common.h"

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char* argv[])
{
   int res {0};

   logstdout = true;

   if (argc < 2 || (argc < 3 && strcmp(argv[1], "List") != 0))
   {
      tell(eloAlways, "Usage: %s {List|State|Restart|Start|Stop|Enable|Disable} <service>", argv[0]);
      exit(1);
   }

   std::string cmd {argv[1]};
   std::string unit {argc > 2 ? argv[2] : ""};

   SysCtl ctl;

   // Connect to the system bus

   if (!ctl.conncected())
      return EXIT_FAILURE;

   if (strstr("Restart|Start|Stop|Enable|Disable", cmd.c_str()))
   {
      ctl.unitAction(cmd.c_str(), unit.c_str());
   }
   else if (cmd == "List")
   {
      SysCtl::Services services;
      ctl.unitList(services);

      tell(eloAlways, "----------------------------------");
      for (const auto& s : services)
         tell(eloAlways, "%s/%s: %-40s  %s", s.second.activeState.c_str(), s.second.unitFileState.c_str(),s.second.primaryName.c_str(), s.second.humanName.c_str());
      tell(eloAlways, "----------------------------------");
   }
   else if (cmd == "State")
   {
      std::string result;
      std::string unitPath;

      if (ctl.loadUnit(unit.c_str(), unitPath) == 0)
      {
         // tell(eloAlways, "Unit path is '%s'", unitPath.c_str());

         tell(eloAlways, "State of unit '%s' [%s]", unit.c_str(), unitPath.c_str());
         ctl.getProperty(unitPath.c_str(), "ActiveState", result);
         tell(eloAlways, "   ActiveState: %s", result.c_str());
         ctl.getProperty(unitPath.c_str(), "SubState", result);
         tell(eloAlways, "   SubState: %s", result.c_str());
         ctl.getProperty(unitPath.c_str(), "LoadState", result);
         tell(eloAlways, "   LoadState: %s", result.c_str());
         ctl.getProperty(unitPath.c_str(), "UnitFileState", result);
         tell(eloAlways, "   UnitFileState: %s", result.c_str());
      }
   }
   else
   {
      tell(eloAlways, "'%s' to be implemented", cmd.c_str());
   }

   return EXIT_SUCCESS;
}
