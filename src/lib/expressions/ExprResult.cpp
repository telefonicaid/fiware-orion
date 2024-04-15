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

// The ExprResult class is used as return value in ExprManager::evaluate(). We could return std::string
// in that function and simplify (so avoiding the ExprResult class). But in that case float rounding is
// problematic (e.g. 1.999999 instead of 2), as they don't take advanage of the ad hoc logic implemented
// in ContextAttribute rendering

#include "expressions/ExprResult.h"

#include "common/string.h"
#include "common/JsonHelper.h"
#include "logMsg/logMsg.h"



#if 0
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
  else if (obj == Py_None)
  {
    return orion::ValueTypeNull;
  }
  else
  {
    // For other types we use string (this is also a failsafe for types not being strings)
    return orion::ValueTypeString;
  }
}


static void processDictItem(orion::CompoundValueNode* parentP, PyObject* key, PyObject* value);  // forward declaration


/* ****************************************************************************
*
* processListItem -
*
*/
void processListItem(orion::CompoundValueNode* parentP, PyObject* value)
{
  orion::CompoundValueNode* nodeP;

  const char* str;
  double d;
  bool b;
  PyObject *keyAux, *valueAux;
  Py_ssize_t pos, size;

  switch (getPyObjectType(value))
  {
  case orion::ValueTypeString:
    str = PyUnicode_AsUTF8(value);
    if (str == NULL)
    {
      LM_E(("Runtime Error (error obtaning str representation: %s)", capturePythonError()));
    }
    else
    {
      LM_T(LmtExpr, ("processListITem (string): %s", str));
      nodeP = new orion::CompoundValueNode("", str, orion::ValueTypeString);
      parentP->add(nodeP);
    }
    break;

  case orion::ValueTypeNumber:
    d = PyFloat_AsDouble(value);
    LM_T(LmtExpr, ("processList (double): %f", d));
    nodeP = new orion::CompoundValueNode("", d, orion::ValueTypeNumber);
    parentP->add(nodeP);
    break;

  case orion::ValueTypeBoolean:
    b = PyObject_IsTrue(value);
    LM_T(LmtExpr, ("ExprResult (bool): %s", b ? "true": "false"));
    nodeP = new orion::CompoundValueNode("", b, orion::ValueTypeBoolean);
    parentP->add(nodeP);
    break;

  case orion::ValueTypeNull:
    nodeP = new orion::CompoundValueNode("", "", orion::ValueTypeNull);
    parentP->add(nodeP);
    break;

  case orion::ValueTypeVector:
    nodeP = new orion::CompoundValueNode(orion::ValueTypeVector);
    size = PyList_Size(value);
    for (Py_ssize_t ix = 0; ix < size; ++ix)
    {
      // No need to free memory of each list item here, the whole result object is freed
      // in ExprManager::evaluate()
      processListItem(nodeP, PyList_GetItem(value, ix));
    }
    parentP->add(nodeP);
    break;

  case orion::ValueTypeObject:
    nodeP = new orion::CompoundValueNode(orion::ValueTypeObject);
    pos = 0;
    while (PyDict_Next(value, &pos, &keyAux, &valueAux))
    {
      // No need to free memory of each dict item here, the whole result object is freed
      // in ExprManager::evaluate()
      processDictItem(nodeP, keyAux, valueAux);
    }
    parentP->add(nodeP);
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
*/
void processDictItem(orion::CompoundValueNode* parentP, PyObject* key, PyObject* value)
{
  const char * keyStr = PyUnicode_AsUTF8(key);
  if (keyStr == NULL)
  {
    LM_E(("Runtime Error (error obtaning str representation: %s)", capturePythonError()));
    return;
  }

  orion::CompoundValueNode* nodeP;
  const char* str;
  double d;
  bool b;
  PyObject *keyAux, *valueAux;
  Py_ssize_t pos, size;

  switch (getPyObjectType(value))
  {
  case orion::ValueTypeString:
    str = PyUnicode_AsUTF8(value);
    if (str == NULL)
    {
      LM_E(("Runtime Error (error obtaning str representation: %s)", capturePythonError()));
    }
    else
    {
      LM_T(LmtExpr, ("processListITem (string): %s", str));
      nodeP = new orion::CompoundValueNode(keyStr, str, orion::ValueTypeString);
      parentP->add(nodeP);
    }
    break;

  case orion::ValueTypeNumber:
    d = PyFloat_AsDouble(value);
    LM_T(LmtExpr, ("processList (double): %f", d));
    nodeP = new orion::CompoundValueNode(keyStr, d, orion::ValueTypeNumber);
    parentP->add(nodeP);
    break;

  case orion::ValueTypeBoolean:
    b = PyObject_IsTrue(value);
    LM_T(LmtExpr, ("ExprResult (bool): %s", b ? "true": "false"));
    nodeP = new orion::CompoundValueNode(keyStr, b, orion::ValueTypeBoolean);
    parentP->add(nodeP);
    break;

  case orion::ValueTypeNull:
    nodeP = new orion::CompoundValueNode(keyStr, "", orion::ValueTypeNull);
    parentP->add(nodeP);
    break;

  case orion::ValueTypeVector:
    nodeP = new orion::CompoundValueNode(keyStr, "", orion::ValueTypeVector);
    size = PyList_Size(value);
    for (Py_ssize_t ix = 0; ix < size; ++ix)
    {
      // No need to free memory of each list item here, the whole result object is freed
      // in ExprManager::evaluate()
      processListItem(nodeP, PyList_GetItem(value, ix));
    }
    parentP->add(nodeP);
    break;

  case orion::ValueTypeObject:
    nodeP = new orion::CompoundValueNode(keyStr, "", orion::ValueTypeObject);
    pos = 0;
    while (PyDict_Next(value, &pos, &keyAux, &valueAux))
    {
      // No need to free memory of each dict item here, the whole result object is freed
      // in ExprManager::evaluate()
      processDictItem(nodeP, keyAux, valueAux);
    }
    parentP->add(nodeP);
    break;

  case orion::ValueTypeNotGiven:
    LM_E(("Runtime Error (value type not given))"));
    break;

  default:
    LM_E(("Runtime Error (value type unknown))"));
  }
}
#endif


/* ****************************************************************************
*
* fill -
*
*/
void ExprResult::fill(std::string result)
{
  /*
  // If nothing changes, the returned value would be null (failsafe)
  valueType = orion::ValueTypeNull;

  // Special case: expresion evalutes to None
  if (result == Py_None)
  {
    LM_T(LmtExpr, ("ExprResult is null"));
    valueType = orion::ValueTypeNull;
    return;
  }

  // Other not null types
  valueType = getPyObjectType(result);
  if (valueType == orion::ValueTypeNumber)
  {
    numberValue = PyFloat_AsDouble(result);
    LM_T(LmtExpr, ("ExprResult (double): %f", numberValue));
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    boolValue = PyObject_IsTrue(result);
    LM_T(LmtExpr, ("ExprResult (bool): %s", boolValue ? "true": "false"));
  }
  else if (valueType == orion::ValueTypeObject)
  {
    compoundValueP = new orion::CompoundValueNode(orion::ValueTypeObject);
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(result, &pos, &key, &value))
    {
      // No need to free memory of each dict item here, the whole result object is freed
      // in ExprManager::evaluate()
      processDictItem(compoundValueP, key, value);
    }
  }
  else if (valueType == orion::ValueTypeVector)
  {
    compoundValueP = new orion::CompoundValueNode(orion::ValueTypeVector);
    Py_ssize_t size = PyList_Size(result);
    for (Py_ssize_t ix = 0; ix < size; ++ix)
    {
      // No need to free memory of each list item here, the whole result object is freed
      // in ExprManager::evaluate()
      processListItem(compoundValueP, PyList_GetItem(result, ix));
    }
  }
  else if (valueType == orion::ValueTypeString)
  {
    const char* str = PyUnicode_AsUTF8(result);
    if (str == NULL)
    {
      LM_E(("Runtime Error (error obtaning str representation: %s)", capturePythonError()));
      valueType = orion::ValueTypeNull;
    }
    else
    {
      LM_T(LmtExpr, ("ExprResult (string): %s", str));
      stringValue = std::string(str);
    }
  }*/
}



/* ****************************************************************************
*
* toString -
*
* Pretty similar to ContextAttribute::toJsonValue()
*
*/
std::string ExprResult::toString(void)
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
    // FIXME PR: does this break the no legacy
    //return "\"" + toJsonString(stringValue) + "\"";
    return "\"" + stringValue + "\"";
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
    LM_E(("Runtime Error (not allowed type in ExprResult: %s)", valueTypeName(valueType)));
    return "";
  }
}



/* ****************************************************************************
*
* ExprResult::release -
*/
void ExprResult::release(void)
{
  if (compoundValueP != NULL)
  {
    delete compoundValueP;
    compoundValueP = NULL;
  }
}