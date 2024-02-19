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
#include "jexl/JexlResult.h"
#include "logMsg/logMsg.h"

#include "orionTypes/OrionValueType.h"



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
  pyjexlModule         = NULL;
  jsonModule           = NULL;
  jexlEngine           = NULL;

  if (sem_init(&sem, 0, 1) == -1)
  {
    LM_X(1, ("Fatal Error (error initializing 'jexl mgr' semaphore: %s)", strerror(errno)));
  }

  Py_Initialize();
  LM_T(LmtJexl, ("Python interpreter has been initialized"));

  pyjexlModule = PyImport_ImportModule("pyjexl");
  if (pyjexlModule == NULL)
  {
    const char* error = capturePythonError();
    LM_X(1, ("Fatal Error (error importing pyjexl module: %s)", error));
  }
  LM_T(LmtJexl, ("pyjexl module has been loaded"));

  jsonModule = PyImport_ImportModule("json");
  if (jsonModule == NULL)
  {
    const char* error = capturePythonError();
    LM_X(1, ("Fatal Error (error importing json module: %s)", error));
  }
  LM_T(LmtJexl, ("json module has been loaded"));

  jexlEngine = PyObject_CallMethod(pyjexlModule, "JEXL", NULL);
  if (jexlEngine == NULL)
  {
    const char* error = capturePythonError();
    LM_X(1, ("Fatal Error (error creating jexlEngine: %s)", error));
  }
  LM_T(LmtJexl, ("jexl engine has been created"));
}



/* ****************************************************************************
*
* JexlManager::evaluate -
*/
JexlResult JexlManager::evaluate(JexlContext* jexlContextP, const std::string& _expression)
{
  JexlResult r;
  r.valueType = orion::ValueTypeNull;

  LM_T(LmtJexl, ("evaluating JEXL expresion: <%s>", _expression.c_str()));

  PyObject* expression = Py_BuildValue("s", _expression.c_str());
  if (expression == NULL)
  {
    // FIXME PR: use LM_E/LM_W?
    LM_T(LmtJexl, ("error building expression: %s", capturePythonError()));
    return r;
  }

  PyObject* result = PyObject_CallMethod(jexlEngine, "evaluate", "OO", expression, jexlContextP->get());
  Py_XDECREF(expression);
  if (result == NULL)
  {
    // FIXME PR: use LM_E/LM_W?
    LM_T(LmtJexl, ("error evaluating expression: %s", capturePythonError()));
    return r;
  }

  r.fill(result);
  Py_XDECREF(result);

  return r;
}



/* ****************************************************************************
*
* JexlManager::release -
*/
void JexlManager::release(void)
{
  if (jexlEngine != NULL)
  {
    Py_XDECREF(jexlEngine);
    LM_T(LmtJexl, ("jexl engine has been freed"));
  }

  if (pyjexlModule != NULL)
  {
    Py_XDECREF(pyjexlModule);
    LM_T(LmtJexl, ("pyjexl module has been freed"));
  }

  if (jsonModule != NULL)
  {
    Py_XDECREF(jsonModule);
    LM_T(LmtJexl, ("json module has been freed"));
  }

  Py_Finalize();
  LM_T(LmtJexl, ("Python interpreter has been finalized"));
}