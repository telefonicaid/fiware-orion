#ifndef SRC_LIB_EXPRESSIONS_EXPRCONTEXT_H_
#define SRC_LIB_EXPRESSIONS_EXPRCONTEXT_H_

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
#include <string>
#include <map>

class ExprContextList;   // forward declaration

/* ****************************************************************************
*
* ExprContext -
*/
class ExprContextObject
{
private:
  bool                                legacy;
  PyObject*                           jexlContext; // used in regular (i.e. not legacy) mode
  std::map<std::string, std::string>  repl;        // used in legacy mode

public:
  ExprContextObject(bool legacy = false);

  PyObject*                            getJexlContext(void);
  std::map<std::string, std::string>*  getMap(void);

  void      add(const std::string& key, const std::string& value);
  void      add(const std::string& key, double value);
  void      add(const std::string& key, bool value);
  void      add(const std::string& key);
  void      add(const std::string& key, ExprContextObject exprContextObject);
  void      add(const std::string& key, ExprContextList exprContextList);

  bool      isLegacy(void);

  std::string toString(void);

  void      release(void);
};

class ExprContextList
{
private:
  PyObject*  jexlContext;

public:
  ExprContextList();

  PyObject* get(void);
  void      add(const std::string& value);
  void      add(double value);
  void      add(bool value);
  void      add(void);
  void      add(ExprContextObject exprContextObject);
  void      add(ExprContextList exprContextList);

  std::string toString(void);

  void      release(void);
};


#endif  // SRC_LIB_EXPRESSIONS_EXPRCONTEXT_H_
