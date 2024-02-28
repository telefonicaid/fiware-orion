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

#include <string>

#include "common/string.h"
#include "logMsg/logMsg.h"
#include "expressions/ExprContext.h"
#include "expressions/exprCommon.h"



/* ****************************************************************************
*
* ExprContextObject::ExprContextObject -
*/
ExprContextObject::ExprContextObject(bool _legacy)
{
  legacy = _legacy;

  if (!legacy)
  {
    jexlContext = PyDict_New();

    if (jexlContext == NULL)
    {
      // Note that this ruins the object. We log this situation just one time here, but we include a NULL check
      // in every other method to be safer
      LM_E(("Runtime Error (fail creating new dict: %s)", capturePythonError()));
    }
  }
}



/* ****************************************************************************
*
* ExprContextObject::getJexlContext -
*/
PyObject* ExprContextObject::getJexlContext(void)
{
  return jexlContext;
}



/* ****************************************************************************
*
* ExprContextObject::getJexlContext -
*/
std::map<std::string,std::string>* ExprContextObject::getMap(void)
{
  return &repl;
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string &key, const std::string &_value, bool raw)
{
  if (legacy)
  {
    std::string value = _value;
    if (!raw)
    {
      // This is the case of regular string. The raw case is for JSON generated from compound values
      value = '"' + _value + '"';
    }
    LM_T(LmtExpr, ("adding to legacy expression context object (string): %s=%s", key.c_str(), value.c_str()));
    repl.insert(std::pair<std::string, std::string>(key, value));
  }
  else
  {
    if (jexlContext == NULL)
    {
      return;
    }
    LM_T(LmtExpr, ("adding to JEXL expression context object (string): %s=%s", key.c_str(), _value.c_str()));
    PyObject* value = Py_BuildValue("s", _value.c_str());
    if (value == NULL)
    {
      LM_E(("Runtime Error (fail creating string value: %s)", capturePythonError()));
      return;
    }
    PyDict_SetItemString(jexlContext, key.c_str(), value);
    Py_DECREF(value);
  }
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string &key, double _value)
{
  if (legacy)
  {
    LM_T(LmtExpr, ("adding to legacy expression context object (double): %s=%f", key.c_str(), _value));
    repl.insert(std::pair<std::string, std::string>(key, double2string(_value)));
  }
  else
  {
    if (jexlContext == NULL)
    {
      return;
    }
    LM_T(LmtExpr, ("adding to JEXL expression context object (double): %s=%f", key.c_str(), _value));
    PyObject* value = Py_BuildValue("d", _value);
    if (value == NULL)
    {
      LM_E(("Runtime Error (fail creating double value: %s)", capturePythonError()));
      return;
    }
    PyDict_SetItemString(jexlContext, key.c_str(), value);
    Py_DECREF(value);
  }
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string &key, bool _value)
{
  if (legacy)
  {
    LM_T(LmtExpr, ("adding to legacy expression context object (bool): %s=%s", key.c_str(), _value ? "true" : "false"));
    repl.insert(std::pair<std::string, std::string>(key, _value? "true": "false"));
  }
  else
  {
    if (jexlContext == NULL)
    {
      return;
    }
    LM_T(LmtExpr, ("adding to JEXL expression context object (bool): %s=%s", key.c_str(), _value ? "true" : "false"));
    if (_value)
    {
      PyDict_SetItemString(jexlContext, key.c_str(), Py_True);
    }
    else
    {
      PyDict_SetItemString(jexlContext, key.c_str(), Py_False);
    }
  }
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string &key)
{
  if (legacy)
  {
    LM_T(LmtExpr, ("adding to legacy expression context object (none): %s", key.c_str()));
    repl.insert(std::pair<std::string, std::string>(key, "null"));
  }
  else
  {
    if (jexlContext == NULL)
    {
      return;
    }
    LM_T(LmtExpr, ("adding to JEXL expression context object (none): %s", key.c_str()));
    PyDict_SetItemString(jexlContext, key.c_str(), Py_None);
  }
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string &key, ExprContextObject exprContextObject)
{
  if (legacy)
  {
    LM_E(("Runtime Error (this method must not be invoked in legacy mode)"));
  }
  else
  {
    if (jexlContext == NULL)
    {
      return;
    }
    // FIXME P3: improve this implemeting a toString() method for ExprContextObject
    LM_T(LmtExpr, ("adding to JEXL expression context object (object): %s=[object]", key.c_str());
    PyDict_SetItemString(jexlContext, key.c_str(), exprContextObject.getJexlContext());
  }
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string &key, ExprContextList exprContextList)
{
  if (legacy)
  {
    LM_E(("Runtime Error (this method must not be invoked in legacy mode)"));
  }
  else
  {
    if (jexlContext == NULL)
    {
      return;
    }
    // FIXME P3: improve this implemeting a toString() method for ExprContextList
    LM_T(LmtExpr, ("adding to JEXL expression context object (list): %s=[list]", key.c_str()));
    PyDict_SetItemString(jexlContext, key.c_str(), exprContextList.get());
  }
}



/* ****************************************************************************
*
* ExprContextObject::isLegacy -
*/
bool ExprContextObject::isLegacy(void)
{
  return legacy;
}



/* ****************************************************************************
*
* ExprContextObject::release -
*/
void ExprContextObject::release(void)
{
  if (jexlContext == NULL)
  {
    return;
  }
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
    // Note that this ruins the object. We log this situation just one time here, but we include a NULL check
    // in every other method to be safer
    LM_E(("Runtime Error (fail creating new dict: %s)", capturePythonError()));
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
void ExprContextList::add(const std::string &_value)
{
  if (jexlContext == NULL)
  {
    return;
  }
  LM_T(LmtExpr, ("adding to JEXL expression context list (string): %s=", _value.c_str()));
  PyObject* value = Py_BuildValue("s", _value.c_str());
  if (value == NULL)
  {
    LM_E(("Runtime Error (fail creating string value: %s)", capturePythonError()));
    return;
  }
  PyList_Append(jexlContext, value);
  Py_DECREF(value);
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(double _value)
{
  if (jexlContext == NULL)
  {
    return;
  }
  LM_T(LmtExpr, ("adding to JEXL expression context list (double): %f", _value));
  PyObject* value = Py_BuildValue("d", _value);
  if (value == NULL)
  {
    LM_E(("Runtime Error (fail creating double value: %s)", capturePythonError()));
    return;
  }
  PyList_Append(jexlContext, value);
  Py_DECREF(value);
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(bool _value)
{
  if (jexlContext == NULL)
  {
    return;
  }
  LM_T(LmtExpr, ("adding to JEXL expression context list (bool): %s", _value ? "true" : "false"));
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
  if (jexlContext == NULL)
  {
    return;
  }
  LM_T(LmtExpr, ("adding to JEXL expression context list (none)"));
  PyList_Append(jexlContext, Py_None);
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(ExprContextObject exprContextObject)
{
  if (jexlContext == NULL)
  {
    return;
  }
  // FIXME P3: improve this implemeting a toString() method for ExprContextObject
  LM_T(LmtExpr, ("adding to JEXL expression context list (object): [object]")));
  PyList_Append(jexlContext, exprContextObject.getJexlContext());
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(ExprContextList exprContextList)
{
  if (jexlContext == NULL)
  {
    return;
  }
  // FIXME P3: improve this implemeting a toString() method for ExprContextList
  LM_T(LmtExpr, ("adding to JEXL expression context list (list): [list]"));
  PyList_Append(jexlContext, exprContextList.get());
}



/* ****************************************************************************
*
* ExprContextList::relesase -
*/
void ExprContextList::release(void)
{
  if (jexlContext == NULL)
  {
    return;
  }
  // FIXME PR: this is not correct. Recursively release of the list object
  //Py_XDECREF(jexlContext);
}