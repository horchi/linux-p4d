//***************************************************************************
// Automation Control
// File pysensor.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 22.08.2021 - Jörg Wendel
//***************************************************************************

#pragma once

#include <Python.h>

class Daemon;

//***************************************************************************
// Class PySensor
//***************************************************************************

class PySensor
{
   public:

      PySensor(Daemon* aDaemon, const char* aPath, const char* aFile);
      ~PySensor();

      int init();
      int exit();

      int execute(const char* command);

      const char* getResult() { return result ? result : ""; }

   protected:

      static PyObject* getData(PyObject* self, PyObject* args);

      void showError(const char* info);

      // data

      PyObject* pModule {nullptr};
      PyObject* pFunc {nullptr};

      char* path {nullptr};
      char* moduleName {nullptr};
      char* function {nullptr};
      char* result {nullptr};

      // static stuff

      static Daemon* singletonDaemon;
      static int usages;
      static PyMethodDef sensorMethods[];

      static PyObject* PyInitSensorModule() { return PyModule_Create(&PySensor::sensorModuleDef); }
      static PyModuleDef sensorModuleDef;
};
