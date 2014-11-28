//***************************************************************************
// Group p4d / Linux - Heizungs Manager
// File w1.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 07.02.2014  Jörg Wendel
//***************************************************************************

#include <stdio.h>
#include <map>

#include "lib/common.h"

//***************************************************************************
// Class W1
//***************************************************************************

class W1
{
   public:
      
      typedef std::map<std::string, double> SensorList;

      W1()  { w1Path = strdup("/sys/bus/w1/devices"); }
      ~W1() { free(w1Path); }

      int scan();
      int show();
      int update();

      SensorList* getList() { return &sensors; }

      double valueOf(const char* id) { return sensors[id]; }

      static unsigned int toId(const char* name);

   protected:
      
      char* w1Path;
      SensorList sensors;
};
