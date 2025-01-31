//***************************************************************************
// Home Systemd Interface
// File systemdctl.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 24.02.2024 - Jörg Wendel
//***************************************************************************

//
// https://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html
//
// Query the Methods:
//    #> gdbus introspect --system --dest org.freedesktop.systemd1 --object-path /org/freedesktop/systemd1
//  with better overview:
//    #> busctl introspect org.freedesktop.systemd1 /org/freedesktop/systemd1/unit/docker_2eservice org.freedesktop.systemd1.Unit

// Details here:
//   https://www.freedesktop.org/wiki/Software/systemd/dbus/
//   https://stackoverflow.com/questions/61940461/how-to-get-the-state-of-a-service-with-sd-bus
//
// Query service state - for example of the 'dnsmasq.service' service:
// #> busctl call org.freedesktop.systemd1 /org/freedesktop/systemd1 org.freedesktop.systemd1.Manager LoadUnit s "dnsmasq.service"
//   o "/org/freedesktop/systemd1/unit/dnsmasq_2eservice"
// #> busctl get-property org.freedesktop.systemd1 /org/freedesktop/systemd1/unit/docker_2eservice org.freedesktop.systemd1.Unit ActiveState
//   s "active"
// #> busctl get-property org.freedesktop.systemd1 /org/freedesktop/systemd1/unit/docker_2eservice org.freedesktop.systemd1.Unit SubState
//   s "running"
//

#include <stdio.h>
#include <stdlib.h>

#include <systemd/sd-bus.h>

#include <string>
#include <map>

#include "common.h"

//***************************************************************************
// SysCtl
//***************************************************************************

class SysCtl
{
   public:

      struct Unit
      {
         std::string primaryName;
         std::string humanName;
         std::string loadState;
         std::string activeState;
         std::string subState;
         std::string unitFileState;
      };

      typedef std::map<std::string,SysCtl::Unit> Services;

      SysCtl() { init(); }
      ~SysCtl() { exit(); }

      bool conncected() { return bus != nullptr; }
      int init();
      int exit();

      int getProperty(const char* unit, const char* property, std::string& value);
      int unitList(Services& services, const char* filter = ".service");
      int unitAction(const char* command, const char* unit);
      int loadUnit(const char* unit, std::string& unitPath);
      int reload();

   private:

      sd_bus* bus {};
};
