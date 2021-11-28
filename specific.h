//***************************************************************************
// Automation Control
// File specific.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 16.04.2020 - Jörg Wendel
//***************************************************************************

#pragma once

#include "daemon.h"

//***************************************************************************
// Class Pool
//***************************************************************************

class P4d : public Daemon
{
   public:

      P4d();
      virtual ~P4d();

      int init() override;

      enum PinMappings
      {
      };

      enum AnalogInputs
      {
      };

      enum SpecialValues  // 'SP'
      {
      };

   protected:

      int initDb() override;
      int exitDb() override;

      int readConfiguration(bool initial) override;
      int applyConfigurationSpecials() override;
      int loadStates() override;
      int atMeanwhile() override;

      int process() override;
      int performJobs() override;
      void logReport() override;

      std::list<ConfigItemDef>* getConfiguration() override { return &configuration; }

      // config

      // statics

      static std::list<ConfigItemDef> configuration;
};
