/*
*
* Copyright 2024 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Fermin Galan
*/

#include <Python.h>

#include "jexl/JexlManager.h"
#include "logMsg/logMsg.h"

/* ****************************************************************************
*
* JexlManager::init -
*/
void JexlManager::init(void)
{
  if (sem_init(&sem, 0, 1) == -1)
  {
    LM_X(1, ("Fatal Error (error initializing 'jexl mgr' semaphore: %s)", strerror(errno)));
  }

  // FIXME PR: Py_Finalize() has to be be called in the proper place
  LM_T(LmtJexl, ("Python interpreter is about to be initialized"));
  Py_Initialize();
  LM_T(LmtJexl, ("Python interpreter has been initialized"));

  // FIXME PR: proper Py_XDECREF() has to be called in the proper place
  // FIXME PR: capture error and move it to char* to print in LM_X
  PyObject* jexl_module = PyImport_ImportModule("pyjexl");
  LM_T(LmtJexl, ("jexl module has been loaded"));

  // Create JEXL engine
  PyObject* jexl_engine = PyObject_CallMethod(jexl_module, "JEXL", NULL);
  LM_T(LmtJexl, ("jexl engine has been created"));

  // Create expression and context
  PyObject* expression = Py_BuildValue("s", "x + y");
  LM_T(LmtJexl, ("jexl expression has been built"));
  PyObject* context = Py_BuildValue("{s:i, s:i}", "x", 4, "y", 11);
  LM_T(LmtJexl, ("jexl context has been built"));

  // Call evaluate method
  PyObject* result = PyObject_CallMethod(jexl_engine, "evaluate", "OO", expression, context);
  LM_T(LmtJexl, ("jexl evaluation is done"));

  // Print result
  PyObject* repr = PyObject_Repr(result);
  const char* result_str = PyUnicode_AsUTF8(repr);
  LM_T(LmtJexl, ("jexl evaluation result is obtainted"));
  LM_T(LmtJexl, ("jexl result: %s", result_str));

  // Free resources
  Py_XDECREF(repr);
  Py_XDECREF(result);
  Py_XDECREF(context);
  Py_XDECREF(expression);
  Py_XDECREF(jexl_engine);
  Py_XDECREF(jexl_module);
  LM_T(LmtJexl, ("all has been freed"));

  // Finalize the Python interpreter
  Py_Finalize();
}