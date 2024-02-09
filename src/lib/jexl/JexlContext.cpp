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

#include "jexl/JexlContext.h"

#include <string>

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

  PyObject* value;

  value = Py_BuildValue("s", id.c_str());
  PyDict_SetItemString(jexl_context, "id", value);
  Py_DECREF(value);

  value = Py_BuildValue("s", type.c_str());
  PyDict_SetItemString(jexl_context, "type", value);
  Py_DECREF(value);

  value = Py_BuildValue("s", service.c_str());
  PyDict_SetItemString(jexl_context, "service", value);
  Py_DECREF(value);

  value = Py_BuildValue("s", servicePath.c_str());
  PyDict_SetItemString(jexl_context, "servicePath", value);
  Py_DECREF(value);

  value = Py_BuildValue("s", token.c_str());
  PyDict_SetItemString(jexl_context, "authToken", value);
  Py_DECREF(value);
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
  PyObject* value = Py_BuildValue("s", _value.c_str());
  PyDict_SetItemString(jexl_context, key.c_str(), value);
  Py_DECREF(value);
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
  Py_XDECREF(jexl_context);
}