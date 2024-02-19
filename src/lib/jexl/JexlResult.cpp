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

#include "jexl/JexlResult.h"

#include "common/string.h"
#include "common/JsonHelper.h"
#include "logMsg/logMsg.h"



/* ****************************************************************************
*
* capturePythonError -
*
* FIXME PR: duplicate code. Unify
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
  // PyBool_Check() has to be done before than PyLong_Check(). Alternatively, PyLong_CheckExact() could be
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



/* ****************************************************************************
*
* fill -
*
*/
void JexlResult::fill(PyObject* result)
{
  // If nothing changes, the returned value would be null (failsafe)
  valueType = orion::ValueTypeNull;

  // Special case: expresion evalutes to None
  if (result == Py_None)
  {
    LM_T(LmtJexl, ("JexlResult is null"));
    valueType = orion::ValueTypeNull;
    return;
  }

  // Other not null types
  valueType = getPyObjectType(result);
  if (valueType == orion::ValueTypeNumber)
  {
    numberValue = PyFloat_AsDouble(result);
    LM_T(LmtJexl, ("JexlResult (double): %f", numberValue));
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    boolValue = PyObject_IsTrue(result);
    LM_T(LmtJexl, ("JexlResult (bool): %s", boolValue ? "true": "false"));
  }
  else if (valueType == orion::ValueTypeObject)
  {
    compoundValueP = new orion::CompoundValueNode(orion::ValueTypeObject);
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(result, &pos, &key, &value))
    {
      processDictItem(compoundValueP, key, value);
    }
    // Using Python json.dumps() for this may seem overkill, but noe that PyObject_Repr(result) cannot
    // be used, as it produce string with ' instead of " in the resulting JSON string.
    // FIXME PR: is the customJsonSerializer going to be used at the end?
    /*PyObject* repr = PyObject_CallMethod(jsonModule, "dumps", "O", result);
    if (repr == NULL)
    {
        // FIXME PR: use LM_E/LM_W?
        LM_T(LmtJexl, ("error obtaining dict/list representation: %s", capturePythonError()));
        valueType = orion::ValueTypeNull;
    }
    else
    {
      const char* str = PyUnicode_AsUTF8(repr);
      Py_XDECREF(repr);
      if (str == NULL)
      {
        // FIXME PR: use LM_E/LM_W?
        LM_T(LmtJexl, ("error obtaining str representation (object or vector): %s", capturePythonError()));
        valueType = orion::ValueTypeNull;
      }
      else
      {
        LM_T(LmtJexl, ("JexlResult (object or vector): %s", str));
        stringValue = std::string(str);
      }
    }*/
  }
  else if (valueType == orion::ValueTypeVector)
  {
    compoundValueP = new orion::CompoundValueNode(orion::ValueTypeVector);
    Py_ssize_t size = PyList_Size(result);
    for (Py_ssize_t ix = 0; ix < size; ++ix)
    {
       processListItem(compoundValueP, PyList_GetItem(result, ix));
    }
    // Using Python json.dumps() for this may seem overkill, but noe that PyObject_Repr(result) cannot
    // be used, as it produce string with ' instead of " in the resulting JSON string.
    // FIXME PR: is the customJsonSerializer going to be used at the end?
    /*PyObject* repr = PyObject_CallMethod(jsonModule, "dumps", "O", result);
    if (repr == NULL)
    {
        // FIXME PR: use LM_E/LM_W?
        LM_T(LmtJexl, ("error obtaining dict/list representation: %s", capturePythonError()));
        valueType = orion::ValueTypeNull;
    }
    else
    {
      const char* str = PyUnicode_AsUTF8(repr);
      Py_XDECREF(repr);
      if (str == NULL)
      {
        // FIXME PR: use LM_E/LM_W?
        LM_T(LmtJexl, ("error obtaining str representation (object or vector): %s", capturePythonError()));
        valueType = orion::ValueTypeNull;
      }
      else
      {
        LM_T(LmtJexl, ("JexlResult (object or vector): %s", str));
        stringValue = std::string(str);
      }
    }*/
  }  
  else if (valueType == orion::ValueTypeString)
  {
    const char* str = PyUnicode_AsUTF8(result);
    if (str == NULL)
    {
      // FIXME PR: use LM_E/LM_W?
      LM_T(LmtJexl, ("error obtaning str representation (string): %s", capturePythonError()));
      valueType = orion::ValueTypeNull;
    }
    else
    {
      LM_T(LmtJexl, ("JexlResult (string): %s", str));
      stringValue = std::string(str);
    }
  }
}



/* ****************************************************************************
*
* processListItem -
*
* FIXME PR: maybe this should be static function out of the class?
*/
void JexlResult::processListItem(orion::CompoundValueNode* parentP, PyObject* item)
{
  orion::CompoundValueNode* nodeP;
  const char* str;
  double d;
  bool b;
  switch (getPyObjectType(item))
  {
  case orion::ValueTypeString:
    str = PyUnicode_AsUTF8(item);
    if (str == NULL)
    {
      // FIXME PR: use LM_E/LM_W?
      LM_T(LmtJexl, ("error obtaning str representation (string): %s", capturePythonError()));
    }
    else
    {
      LM_T(LmtJexl, ("processListITem (string): %s", str));
      nodeP = new orion::CompoundValueNode("", str, orion::ValueTypeString);
      parentP->add(nodeP);
    }
    break;

  case orion::ValueTypeNumber:
    d = PyFloat_AsDouble(item);
    LM_T(LmtJexl, ("processList (double): %f", d));
    nodeP = new orion::CompoundValueNode("", d, orion::ValueTypeNumber);
    parentP->add(nodeP);
    break;

  case orion::ValueTypeBoolean:
    b = PyObject_IsTrue(item);
    LM_T(LmtJexl, ("JexlResult (bool): %s", b ? "true": "false"));
    nodeP = new orion::CompoundValueNode("", b, orion::ValueTypeBoolean);
    parentP->add(nodeP);
    break;

  case orion::ValueTypeNull:
    nodeP = new orion::CompoundValueNode("", "", orion::ValueTypeNull);
    parentP->add(nodeP);
    break;

  case orion::ValueTypeVector:
    // TBD
    break;

  case orion::ValueTypeObject:
    // TBD
    break;

  case orion::ValueTypeNotGiven:
    LM_E(("Runtime Error (value type not given))"));
    break;

  default:
    LM_E(("Runtime Error (value type unknown))"));
  }
}



/* ****************************************************************************
*
* processDictItem -
*
* FIXME PR: maybe this should be static function out of the class?
*/
void JexlResult::processDictItem(orion::CompoundValueNode* parentP, PyObject* key, PyObject* value)
{
  std::string keyStr = "";
  orion::CompoundValueNode* nodeP;
  const char* str;
  double d;
  bool b;
  switch (getPyObjectType(value))
  {
  case orion::ValueTypeString:
    str = PyUnicode_AsUTF8(value);
    if (str == NULL)
    {
      // FIXME PR: use LM_E/LM_W?
      LM_T(LmtJexl, ("error obtaning str representation (string): %s", capturePythonError()));
    }
    else
    {
      LM_T(LmtJexl, ("processListITem (string): %s", str));
      nodeP = new orion::CompoundValueNode(keyStr, str, orion::ValueTypeString);
      parentP->add(nodeP);
    }
    break;

  case orion::ValueTypeNumber:
    d = PyFloat_AsDouble(value);
    LM_T(LmtJexl, ("processList (double): %f", d));
    nodeP = new orion::CompoundValueNode(keyStr, d, orion::ValueTypeNumber);
    parentP->add(nodeP);
    break;

  case orion::ValueTypeBoolean:
    b = PyObject_IsTrue(value);
    LM_T(LmtJexl, ("JexlResult (bool): %s", b ? "true": "false"));
    nodeP = new orion::CompoundValueNode(keyStr, b, orion::ValueTypeBoolean);
    parentP->add(nodeP);
    break;

  case orion::ValueTypeNull:
    nodeP = new orion::CompoundValueNode(keyStr, "", orion::ValueTypeNull);
    parentP->add(nodeP);
    break;

  case orion::ValueTypeVector:
    // TBD
    break;

  case orion::ValueTypeObject:
    // TBD
    break;

  case orion::ValueTypeNotGiven:
    LM_E(("Runtime Error (value type not given))"));
    break;

  default:
    LM_E(("Runtime Error (value type unknown))"));
  }
}



/* ****************************************************************************
*
* toString -
*
* Pretty similar to ContextAttribute::toJsonValue()
*
*/
std::string JexlResult::toString(void)
{
  if (valueType == orion::ValueTypeNumber)
  {
    return double2string(numberValue);
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    return boolValue ? "true" : "false";
  }
  else if (valueType == orion::ValueTypeString)
  {
    return "\"" + toJsonString(stringValue) + "\"";
  }
  else if ((valueType == orion::ValueTypeObject)||(valueType == orion::ValueTypeVector))
  {
    if (compoundValueP != NULL)
    {
      return compoundValueP->toJson();
    }
    else
    {
      LM_E(("Runtime Error (result is vector/object but compountValue is NULL)"));
      return "";
    }
  }
  else if (valueType == orion::ValueTypeNull)
  {
    return "null";
  }
  else
  {
    LM_E(("Runtime Error (not allowed type in JexlResult: %s)", valueTypeName(valueType)));
    return "";
  }
}



/* ****************************************************************************
*
* JexlResult::release -
*/
void JexlResult::release(void)
{
  if (compoundValueP != NULL)
  {
    delete compoundValueP;
    compoundValueP = NULL;
  }
}