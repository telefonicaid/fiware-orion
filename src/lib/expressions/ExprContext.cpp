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

// FIXME PR: add methods should check error in Py_BuildValue() for NULL

#include <string>

#include "logMsg/logMsg.h"
#include "expressions/ExprContext.h"



/* ****************************************************************************
*
* ExprContextObject::ExprContextObject -
*/
ExprContextObject::ExprContextObject
()
{
  jexlContext = PyDict_New();

  if (jexlContext == NULL)
  {
    // FIXME PR: error control
    // Note that this ruins the object... eg. add methods are not calleable (maybe should inclue a NULL check in all them)
  }
}



/* ****************************************************************************
*
* ExprContextObject::get -
*/
PyObject* ExprContextObject::get(void)
{
  return jexlContext;
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string& key, const std::string& _value)
{
  LM_T(LmtExpr, ("adding to expression context object (string): %s=%s", key.c_str(), _value.c_str()));
  PyObject* value = Py_BuildValue("s", _value.c_str());
  PyDict_SetItemString(jexlContext, key.c_str(), value);
  Py_DECREF(value);
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string& key, double _value)
{
  LM_T(LmtExpr, ("adding to expression context object (double): %s=%f", key.c_str(), _value));
  PyObject* value = Py_BuildValue("d", _value);
  PyDict_SetItemString(jexlContext, key.c_str(), value);
  Py_DECREF(value);
}




/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string& key, bool _value)
{
  LM_T(LmtExpr, ("adding to expression context object (bool): %s=%s", key.c_str(), _value? "true" : "false"));
  if (_value)
  {
    PyDict_SetItemString(jexlContext, key.c_str(), Py_True);
  }
  else
  {
    PyDict_SetItemString(jexlContext, key.c_str(), Py_False);
  }
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string& key)
{
  LM_T(LmtExpr, ("adding to expression context object (none): %s", key.c_str()));
  PyDict_SetItemString(jexlContext, key.c_str(), Py_None);
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string& key, ExprContextObject exprContextObject)
{
  // FIXME PR: implement a toString() method in ExprContextObject to be used here
  //LM_T(LmtExpr, ("adding to expression context (none): %s", key.c_str()));
  PyDict_SetItemString(jexlContext, key.c_str(), exprContextObject.get());
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string& key, ExprContextList exprContextList)
{
  // FIXME PR: implement a toString() method in ExprContextList to be used here
  //LM_T(LmtExpr, ("adding to expression context (none): %s", key.c_str()));
  PyDict_SetItemString(jexlContext, key.c_str(), exprContextList.get());
}



/* ****************************************************************************
*
* ExprContextObject::hasKey -
*/
bool ExprContextObject::hasKey(const std::string& key)
{
  // Check if the key exists in the jexlContext dictionary
  PyObject* keyObject = PyUnicode_FromString(key.c_str());
  int result = PyDict_Contains(jexlContext, keyObject);
  Py_DECREF(keyObject);

  return result == 1; // Return true if key exists, false otherwise
}



/* ****************************************************************************
*
* ExprContextObject::release -
*/
void ExprContextObject::release(void)
{
  // FIXME PR: this is not correct. Recursively release of the dict object
  //Py_XDECREF(jexlContext);
}


/* ****************************************************************************
*
* ExprContextList::ExprContextList -
*/
ExprContextList::ExprContextList()
{
  jexlContext = PyList_New(0);

  if (jexlContext == NULL)
  {
    // FIXME PR: error control
    // Note that this ruins the object... eg. add methods are not calleable (maybe should inclue a NULL check in all them)
  }
}



/* ****************************************************************************
*
* ExprContextList::get -
*/
PyObject* ExprContextList::get(void)
{
  return jexlContext;
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(const std::string& _value)
{
  LM_T(LmtExpr, ("adding to expression context list (string): %s=", _value.c_str()));
  PyObject* value = Py_BuildValue("s", _value.c_str());
  PyList_Append(jexlContext, value);
  Py_DECREF(value);
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(double _value)
{
  LM_T(LmtExpr, ("adding to expression context list (double): %f", _value));
  PyObject* value = Py_BuildValue("d", _value);
  PyList_Append(jexlContext, value);
  Py_DECREF(value);
}




/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(bool _value)
{
  LM_T(LmtExpr, ("adding to expression context list (bool): %s", _value? "true" : "false"));
  if (_value)
  {
    PyList_Append(jexlContext, Py_True);
  }
  else
  {
    PyList_Append(jexlContext, Py_False);
  }
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(void)
{
  LM_T(LmtExpr, ("adding to expression context list (none)"));
  PyList_Append(jexlContext, Py_None);
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(ExprContextObject exprContextObject)
{
  // FIXME PR: implement a toString() method in ExprContext to be used here
  //LM_T(LmtExpr, ("adding to expression context (none): %s", key.c_str()));
  PyList_Append(jexlContext, exprContextObject.get());
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(ExprContextList exprContextList)
{
  // FIXME PR: implement a toString() method in ExprContextList to be used here
  //LM_T(LmtExpr, ("adding to expression context (none): %s", key.c_str()));
  PyList_Append(jexlContext, exprContextList.get());
}



/* ****************************************************************************
*
* ExprContextList::relesase -
*/
void ExprContextList::release(void)
{
  // FIXME PR: this is not correct. Recursively release of the list object
  Py_XDECREF(jexlContext);
}