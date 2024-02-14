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
* getPyObjectType -
*/
static orion::ValueType getPyObjectType(PyObject* obj)
{
  // PyBool_Check() has to be donde before than PyLong_Check(). Alternatively, PyLong_CheckExact() could be
  // used (not tested). See: https://stackoverflow.com/q/77990353/1485926
  if (PyBool_Check(obj))
  {
    return orion::ValueTypeBoolean;
  }
  else if (PyLong_Check(obj))
  {
    return orion::ValueTypeNumber;
  }
  else if (PyFloat_Check(obj))
  {
    return orion::ValueTypeNumber;
  }
  else if (PyDict_Check(obj))
  {
    return orion::ValueTypeObject;
  }
  else if (PyList_Check(obj))
  {
    return orion::ValueTypeVector;
  }
  else
  {
    // For other types we use string (this is also a failsafe for types not being strings)
    return orion::ValueTypeString;
  }
}



#if 0
/* ****************************************************************************
*
* customJsonSerializerFunction -
*
* To be used in the json.dumps() step in JexlManager::evaluate() in order to get
* integer numbers when possible (i.e. "2" instead of "2.0"), this way behaving as
* the pre-JEXL macro replacement logic
*/
static PyObject* customJsonSerializerFunction(PyObject* obj)
{
  // Check if the object is a float
  if (PyFloat_Check(obj))
  {
    // Convert float to integer if it represents a whole number
    double value = PyFloat_AsDouble(obj);
    if (value == (int)value)
    {
      return PyLong_FromDouble(value);
    }
  }

  // Return the original object
  Py_INCREF(obj);
  return obj;
}
#endif



/* ****************************************************************************
*
* JexlManager::init -
*/
void JexlManager::init(void)
{
  pyjexlModule         = NULL;
  jsonModule           = NULL;
  jexlEngine           = NULL;
  //customJsonSerializer = NULL;

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

#if 0
  customJsonSerializer = PyCapsule_New((void*)customJsonSerializerFunction, NULL, NULL);
  if (customJsonSerializer == NULL)
  {
    const char* error = capturePythonError();
    LM_X(1, ("Fatal Error (error creating json custom serializer function: %s)", error));
  }
  LM_T(LmtJexl, ("json custom serializer has been createdmodule has been loaded"));
#endif

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
  LM_T(LmtJexl, ("evaluating JEXL expresion: <%s>", _expression.c_str()));

  JexlResult r;

  // If nothing changes, the returned value would be ValueTypeNotGiven
  r.valueType = orion::ValueTypeNotGiven;

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

  // Special case: expresion evalutes to None
  if (result == Py_None)
  {
    LM_T(LmtJexl, ("JEXL evaluation result is null"));
    r.valueType = orion::ValueTypeNull;
    Py_XDECREF(result);
    return r;
  }

  // Other not null types
  r.valueType = getPyObjectType(result);
  if (r.valueType == orion::ValueTypeNumber)
  {
    r.numberValue = PyFloat_AsDouble(result);
    LM_T(LmtJexl, ("JEXL evaluation result (double): %f", r.numberValue));
    Py_XDECREF(result);    
  }
  else if (r.valueType == orion::ValueTypeBoolean)
  {
    r.boolValue = PyObject_IsTrue(result);
    LM_T(LmtJexl, ("JEXL evaluation result (bool): %s", r.boolValue ? "true": "false"));
    Py_XDECREF(result);
  }
  else if ((r.valueType == orion::ValueTypeObject) || (r.valueType == orion::ValueTypeVector))
  {
    // Using Python json.dumps() for this may seem overkill, but noe that PyObject_Repr(result) cannot
    // be used, as it produce string with ' instead of " in the resulting JSON string.
    // FIXME PR: is the customJsonSerializer going to be used at the end?
    PyObject* repr = PyObject_CallMethod(jsonModule, "dumps", "O", result);
    Py_XDECREF(result);
    if (repr == NULL)
    {
        // FIXME PR: use LM_E/LM_W?
        LM_T(LmtJexl, ("error obtaining dict/list representation: %s", capturePythonError()));
        r.valueType = orion::ValueTypeNull;
    }
    else
    {
      const char* str = PyUnicode_AsUTF8(repr);
      Py_XDECREF(repr);
      if (str == NULL)
      {
        // FIXME PR: use LM_E/LM_W?
        LM_T(LmtJexl, ("error obtaining str representation (object or vector): %s", capturePythonError()));
        r.valueType = orion::ValueTypeNull;
      }
      else
      {
        LM_T(LmtJexl, ("JEXL evaluation result (object or vector): %s", str));
        r.stringValue = std::string(str);
      }
    }
  }
  else if (r.valueType == orion::ValueTypeString)
  {
    const char* str = PyUnicode_AsUTF8(result);
    Py_XDECREF(result);
    if (str == NULL)
    {
      // FIXME PR: use LM_E/LM_W?
      LM_T(LmtJexl, ("error obtaning str representation (string): %s", capturePythonError()));
      r.valueType = orion::ValueTypeNull;
    }
    else
    {
      LM_T(LmtJexl, ("JEXL evaluation result (string): %s", str));
      r.stringValue = std::string(str);
    }
  }

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

#if 0
  if (customJsonSerializer != NULL)
  {
    Py_XDECREF(customJsonSerializer);
    LM_T(LmtJexl, ("json custom serializer module has been freed"));
  }
#endif

  if (jsonModule != NULL)
  {
    Py_XDECREF(jsonModule);
    LM_T(LmtJexl, ("json module has been freed"));
  }

  Py_Finalize();
  LM_T(LmtJexl, ("Python interpreter has been finalized"));
}