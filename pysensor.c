//***************************************************************************
// Automation Control
// File pysensor.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 22.08.2021 - Jörg Wendel
//***************************************************************************

#include "lib/common.h"

#include "pysensor.h"
#include "daemon.h"

Daemon* PySensor::singletonDaemon {nullptr};
int PySensor::usages {0};

//***************************************************************************
// Static Interface Methods
//***************************************************************************

PyObject* PySensor::getData(PyObject* self, PyObject* args)
{
   const char* type {nullptr};
   int address {0};

   if (!PyArg_ParseTuple(args, "si", &type, &address))
      return nullptr;

   // tell(0, "python PySensor::getData: %s:%d  [%s]\n", type, address, singletonDaemon->sensorJsonStringOf(type, address).c_str());

   return Py_BuildValue("s", singletonDaemon->sensorJsonStringOf(type, address).c_str());
}

//***************************************************************************
// The Methods
//***************************************************************************

PyMethodDef PySensor::sensorMethods[] =
{
   { "getData",  PySensor::getData,  METH_VARARGS,  "Return sensor data ..." },
   {        0,                   0,             0,  0 }
};

PyModuleDef PySensor::sensorModuleDef =
{
   PyModuleDef_HEAD_INIT,    // m_base
   "sensor",                 // m_name     - name of module
   0,                        // m_doc      - module documentation, may be NULL
   -1,                       // m_size     - size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
   sensorMethods,            // m_methods
   0,                        // m_slots    - array of slot definitions for multi-phase initialization, terminated by a {0, NULL} entry.
                             //              when using single-phase initialization, m_slots must be NULL.
   0,                        // traverseproc m_traverse - traversal function to call during GC traversal of the module object, or NULL if not needed.
   0,                        // inquiry m_clear         - clear function to call during GC clearing of the module object, or NULL if not needed.
   0                         // freefunc m_free         - function to call during deallocation of the module object or NULL if not needed.
};

//***************************************************************************
// Object
//***************************************************************************

PySensor::PySensor(Daemon* aDaemon, const char* aPath, const char* aFile)
{
   singletonDaemon = aDaemon;

   char* tmp = strdup(aFile);
   moduleName = strdup(basename(tmp));
   free(tmp);

   *(strchr(moduleName, '.')) = '\0';

   path = strdup(aPath);
   function = strdup("sensorCall");
}

PySensor::~PySensor()
{
   exit();

   free(result);
   free(path);
   free(moduleName);
   free(function);
}

//***************************************************************************
// Init / Exit
//***************************************************************************

int PySensor::init()
{
   tell(1, "Initialize python script '%s/%s.py'", path, moduleName);

   // register sensor method

   PyImport_AppendInittab("sensor", &PyInitSensorModule);

   if (!usages)
      Py_Initialize();     // initialize the Python interpreter only once

   usages++;

   // add search path for Python modules

   char* p {nullptr};
   asprintf(&p, "sys.path.append(\"%s\")", path);
   PyRun_SimpleString("import sys");
   PyRun_SimpleString(p);
   free(p);

   // import the module

   PyObject* pName = PyUnicode_FromString(moduleName);
   pModule = PyImport_Import(pName);
   Py_DECREF(pName);

   if (!pModule)
   {
      showError("pModule");
      tell(0, "Failed to load python module '%s.py'", moduleName);
      return fail;
   }

   pFunc = PyObject_GetAttrString(pModule, function);

   // pFunc is a new reference

   if (!pFunc || !PyCallable_Check(pFunc))
   {
      tell(0, "python pFunc = %p", (void*)pFunc);

      if (PyErr_Occurred())
         showError("PyCallable_Check");

      tell(0, "Cannot init python function '%s'", function);
      return fail;
   }

   return success;
}

int PySensor::exit()
{
   usages--;

   if (pFunc)
      Py_XDECREF(pFunc);

   if (pModule)
      Py_DECREF(pModule);

   if (!usages)
      Py_Finalize();

   return success;
}

//***************************************************************************
// Execute
//***************************************************************************

int PySensor::execute(const char* command)
{
   PyObject* pValue {nullptr};

   free(result);
   result = nullptr;

   PyObject* pyArgs = PyTuple_New(3);
   PyTuple_SetItem(pyArgs, 0, PyUnicode_FromString(command));
   PyTuple_SetItem(pyArgs, 1, PyUnicode_FromString("SC"));
   PyTuple_SetItem(pyArgs, 2, PyLong_FromLong(99));

   pValue = PyObject_CallObject(pFunc, pyArgs);

   if (!pValue)
   {
      showError("execute");
      tell(0, "Python: Call of function '%s()' failed", function);
      return fail;
   }

   PyObject* pyStr = PyUnicode_AsEncodedString(pValue, "utf-8", "replace");

   if (!pyStr)
   {
      Py_DECREF(pValue);
      tell(0, "Error: Unexpected result of call to %s()", function);
      return fail;
   }

   tell(3, "Result of call to %s(): %s", function, result);
   result = strdup(PyBytes_AsString(pyStr));
   Py_DECREF(pValue);

   return success;
}

//***************************************************************************
// Dup Py String
//***************************************************************************

char* dupPyString(PyObject* pyObj)
{
   char* s;
   PyObject* pyString;

   if (!pyObj || !(pyString=PyObject_Str(pyObj)))
   {
      s = strdup("unknown error");
      return s;
   }

   PyObject* pyUni = PyObject_Repr(pyString);    // Now a unicode object
   PyObject* pyStr = PyUnicode_AsEncodedString(pyUni, "utf-8", "Error ~");
   s = strdup(PyBytes_AsString(pyStr));
   Py_XDECREF(pyUni);
   Py_XDECREF(pyStr);

   Py_DECREF(pyString);

   return s;
}

//***************************************************************************
// Show Error
//***************************************************************************

void PySensor::showError(const char* info)
{
   char* error;
   PyObject *ptype = 0, *pError = 0, *ptraceback = 0;
   PyObject *moduleName, *pythModule;

   PyErr_Fetch(&ptype, &pError, &ptraceback);

   error = dupPyString(pError);
   moduleName = PyUnicode_FromString("traceback");

   tell(0, "Python error at '%s' was %s", info, error);

   pythModule = PyImport_Import(moduleName);
   Py_DECREF(moduleName);

   // traceback

   if (pythModule)
   {
      PyObject* pyFunc = PyObject_GetAttrString(pythModule, "format_exception");

      if (pyFunc && PyCallable_Check(pyFunc))
      {
         PyObject *pythVal, *pystr;
         char* s;

         pythVal = PyObject_CallFunctionObjArgs(pyFunc, ptype, pError, ptraceback, 0);

         if (pythVal)
         {
            s = dupPyString(pystr = PyObject_Str(pythVal));
            tell(0, "   %s", s);

            free(s);
            Py_DECREF(pystr);
            Py_DECREF(pythVal);
         }
      }
   }

   free(error);
   Py_DECREF(pError);
   Py_DECREF(ptype);
   Py_XDECREF(ptraceback);
}
