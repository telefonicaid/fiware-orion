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

#include "logMsg/logMsg.h"
#include "jexl/JexlContext.h"



/* ****************************************************************************
*
* JexlContext::JexlContext -
*/
JexlContext::JexlContext
(
  const std::string& id,
  const std::string& type,
  const std::string& service,
  const std::string& servicePath,
  const std::string& token
)
{
  jexl_context = PyDict_New();

  if (jexl_context == NULL)
  {
    // FIXME PR: error control
  }

  add("id", id);
  add("type", type);
  add("service", service);
  add("servicePath", servicePath);
  add("authToken", token);
}



/* ****************************************************************************
*
* JexlContext::get -
*/
PyObject* JexlContext::get(void)
{
  return jexl_context;
}



/* ****************************************************************************
*
* JexlContext::add -
*/
void JexlContext::add(const std::string& key, const std::string& _value)
{
  LM_T(LmtJexl, ("adding to JEXL context (string): %s=%s", key.c_str(), _value.c_str()));
  PyObject* value = Py_BuildValue("s", _value.c_str());
  PyDict_SetItemString(jexl_context, key.c_str(), value);
  Py_DECREF(value);
}



/* ****************************************************************************
*
* JexlContext::add -
*/
void JexlContext::add(const std::string& key, double _value)
{
  LM_T(LmtJexl, ("adding to JEXL context (double): %s=%f", key.c_str(), _value));
  PyObject* value = Py_BuildValue("d", _value);
  PyDict_SetItemString(jexl_context, key.c_str(), value);
  Py_DECREF(value);
}




/* ****************************************************************************
*
* JexlContext::add -
*/
void JexlContext::add(const std::string& key, bool _value)
{
  LM_T(LmtJexl, ("adding to JEXL context (bool): %s=%s", key.c_str(), _value? "true" : "false"));
  if (_value)
  {
    PyDict_SetItemString(jexl_context, key.c_str(), Py_True);
  }
  else
  {
    PyDict_SetItemString(jexl_context, key.c_str(), Py_False);
  }
}



/* ****************************************************************************
*
* JexlContext::add -
*/
void JexlContext::add(const std::string& key)
{
  LM_T(LmtJexl, ("adding to JEXL context (none): %s", key.c_str()));
  PyDict_SetItemString(jexl_context, key.c_str(), Py_None);
}



/* ****************************************************************************
*
* JexlContext::hasKey -
*/
bool JexlContext::hasKey(const std::string& key)
{
  // Check if the key exists in the jexl_context dictionary
  PyObject* keyObject = PyUnicode_FromString(key.c_str());
  int result = PyDict_Contains(jexl_context, keyObject);
  Py_DECREF(keyObject);

  return result == 1; // Return true if key exists, false otherwise
}



/* ****************************************************************************
*
* JexlContext::~JexlContext -
*/
JexlContext::~JexlContext()
{
  // FIXME PR: this is not correct. Recursively release of the dict object
  Py_XDECREF(jexl_context);
}