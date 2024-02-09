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
* capturePythonError -
*/
static const char* capturePythonError()
{
  if (PyErr_Occurred())
  {
    PyObject* ptype;
    PyObject* pvalue;
    PyObject* ptraceback;

    // Fetch the exception type, value, and traceback
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);

    if (pvalue != NULL)
    {
      PyObject* str_obj = PyObject_Str(pvalue);
      const char* error_message = PyUnicode_AsUTF8(str_obj);

      // Release the Python objects
      Py_XDECREF(str_obj);
      Py_XDECREF(ptype);
      Py_XDECREF(pvalue);
      Py_XDECREF(ptraceback);

      return error_message;
    }
  }

  PyErr_Clear();
  return "<no captured error>";
}



/* ****************************************************************************
*
* JexlManager::init -
*/
void JexlManager::init(void)
{
  jexl_module = NULL;
  jexl_engine = NULL;

  if (sem_init(&sem, 0, 1) == -1)
  {
    LM_X(1, ("Fatal Error (error initializing 'jexl mgr' semaphore: %s)", strerror(errno)));
  }

  Py_Initialize();
  LM_T(LmtJexl, ("Python interpreter has been initialized"));

  jexl_module = PyImport_ImportModule("pyjexl");
  if (jexl_module == NULL)
  {
    const char* error = capturePythonError();
    LM_X(1, ("Fatal Error (error importing jexl_module: %s)", error));
  }
  LM_T(LmtJexl, ("jexl module has been loaded"));

  jexl_engine = PyObject_CallMethod(jexl_module, "JEXL", NULL);
  if (jexl_engine == NULL)
  {
    const char* error = capturePythonError();
    LM_X(1, ("Fatal Error (error creating jexl_engine: %s)", error));
  }
  LM_T(LmtJexl, ("jexl engine has been created"));
}



/* ****************************************************************************
*
* JexlManager::evaluate -
*/
std::string JexlManager::evaluate(JexlContext* jexlContextP, const std::string& _expression)
{
  LM_T(LmtJexl, ("evaluating JEXL expresion: %s", _expression.c_str()));

  PyObject* expression = Py_BuildValue("s", _expression.c_str());
  if (expression == NULL)
  {
    // FIXME PR: grab error message from Python stack
    LM_T(LmtJexl, ("error evaluating expression, result is null"));
    return "null";
  }

  PyObject* result = PyObject_CallMethod(jexl_engine, "evaluate", "OO", expression, jexlContextP->get());
  Py_XDECREF(expression);
  if (result == NULL)
  {
    // FIXME PR: grab error message from Python stack
    LM_T(LmtJexl, ("error evaluating expression, result is null"));
    return "null";
  }

  PyObject* repr = PyObject_Repr(result);
  Py_XDECREF(result);
  if (repr == NULL)
  {
    // FIXME PR: grab error message from Python stack
    LM_T(LmtJexl, ("error evaluating expression, result is null"));
    return "null";
  }

  std::string result_str = std::string(PyUnicode_AsUTF8(repr));
  Py_XDECREF(repr);

  LM_T(LmtJexl, ("JEXL expression evaluation result: %s", result_str.c_str()));
  return result_str;
}



/* ****************************************************************************
*
* JexlManager::release -
*/
void JexlManager::release(void)
{
  if (jexl_engine != NULL)
  {
    Py_XDECREF(jexl_engine);
    LM_T(LmtJexl, ("jexl engine has been freed"));
  }

  if (jexl_module != NULL)
  {
    Py_XDECREF(jexl_module);
    LM_T(LmtJexl, ("jexl module has been freed"));
  }

  Py_Finalize();
  LM_T(LmtJexl, ("Python interpreter has been finalized"));
}