#ifndef SRC_LIB_JEXL_JEXLCONTEXT_H_
#define SRC_LIB_JEXL_JEXLCONTEXT_H_

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

// FIXME PR: rename this to a better name (ExprContextDict?)

#include <Python.h>

#include <string>

//#include "jexl/JexlContextList.h"

class JexlContextList;   // forward declaration

/* ****************************************************************************
*
* JexlContext -
*/
class JexlContext
{
private:
  PyObject*  jexl_context;

public:
  ~JexlContext();

  JexlContext();

  PyObject* get(void);
  void      add(const std::string& key, const std::string& value);
  void      add(const std::string& key, double value);
  void      add(const std::string& key, bool value);
  void      add(const std::string& key);
  void      add(const std::string& key, JexlContext jexlContext);
  void      add(const std::string& key, JexlContextList jexlContextList);
  bool      hasKey(const std::string& key);
};

class JexlContextList
{
private:
  PyObject*  jexl_context;

public:
  ~JexlContextList();

  JexlContextList();

  PyObject* get(void);
  void      add(const std::string& value);
  void      add(double value);
  void      add(bool value);
  void      add(void);
  void      add(JexlContext jexlContext);
  void      add(JexlContextList jexlContextList);
};


#endif  // #define SRC_LIB_JEXL_JEXLCONTEXT_H_
