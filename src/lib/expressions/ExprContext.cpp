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


/* ****************************************************************************
*
* ExprContextObject::ExprContextObject -
*/
ExprContextObject::ExprContextObject(bool _basic)
{
  basic = _basic;
}



/* ****************************************************************************
*
* ExprContextObject::getJexlContext -
*/
std::string ExprContextObject::getJexlContext(void)
{
  return jh.str(false) + '}';
}



/* ****************************************************************************
*
* ExprContextObject::getMap -
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
  if (basic)
  {
    std::string value = _value;
    if (!raw)
    {
      // This is the case of regular string. The raw case is for JSON generated from compound values
      value = '"' + _value + '"';
    }
    LM_T(LmtExpr, ("adding to basic expression context object (string): %s=%s", key.c_str(), value.c_str()));
    repl.insert(std::pair<std::string, std::string>(key, value));
  }
  else
  {
    LM_T(LmtExpr, ("adding to JEXL expression context object (string): %s=%s", key.c_str(), _value.c_str()));
    if (raw)
    {
      jh.addRaw(key, _value);
    }
    else
    {
      jh.addString(key, _value);
    }
  }
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string &key, double _value)
{
  if (basic)
  {
    LM_T(LmtExpr, ("adding to basic expression context object (double): %s=%f", key.c_str(), _value));
    repl.insert(std::pair<std::string, std::string>(key, double2string(_value)));
  }
  else
  {
    LM_T(LmtExpr, ("adding to JEXL expression context object (double): %s=%f", key.c_str(), _value));
    jh.addNumber(key, _value);
  }
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string &key, bool _value)
{
  if (basic)
  {
    LM_T(LmtExpr, ("adding to basic expression context object (bool): %s=%s", key.c_str(), _value ? "true" : "false"));
    repl.insert(std::pair<std::string, std::string>(key, _value? "true": "false"));
  }
  else
  {
    LM_T(LmtExpr, ("adding to JEXL expression context object (bool): %s=%s", key.c_str(), _value ? "true" : "false"));
    jh.addBool(key, _value);
  }
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string &key)
{
  if (basic)
  {
    LM_T(LmtExpr, ("adding to basic expression context object (none): %s", key.c_str()));
    repl.insert(std::pair<std::string, std::string>(key, "null"));
  }
  else
  {
    LM_T(LmtExpr, ("adding to JEXL expression context object (none): %s", key.c_str()));
    jh.addNull(key);
  }
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string &key, ExprContextObject exprContextObject)
{
  if (basic)
  {
    LM_E(("Runtime Error (this method must not be invoked in basic mode)"));
  }
  else
  {
    std::string s = exprContextObject.getJexlContext();
    LM_T(LmtExpr, ("adding to JEXL expression context object (object): %s=%s", key.c_str(), s.c_str()));
    jh.addRaw(key, s);
  }
}



/* ****************************************************************************
*
* ExprContextObject::add -
*/
void ExprContextObject::add(const std::string &key, ExprContextList exprContextList)
{
  if (basic)
  {
    LM_E(("Runtime Error (this method must not be invoked in basic mode)"));
  }
  else
  {
    std::string s = exprContextList.get();
    LM_T(LmtExpr, ("adding to JEXL expression context object (list): %s=%s", key.c_str(), s.c_str()));
    jh.addRaw(key, s);
  }
}



/* ****************************************************************************
*
* ExprContextObject::isBasic -
*/
bool ExprContextObject::isBasic(void)
{
  return basic;
}



/* ****************************************************************************
*
* ExprContextList::get -
*/
std::string ExprContextList::get(void)
{
  return jh.str();
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(const std::string &_value)
{
  LM_T(LmtExpr, ("adding to JEXL expression context list (string): %s", _value.c_str()));
  jh.addString(_value);
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(double _value)
{
  LM_T(LmtExpr, ("adding to JEXL expression context list (double): %f", _value));
  jh.addNumber(_value);
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(bool _value)
{
  LM_T(LmtExpr, ("adding to JEXL expression context list (bool): %s", _value ? "true" : "false"));
  jh.addBool(_value);
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(void)
{
  LM_T(LmtExpr, ("adding to JEXL expression context list (none)"));
  jh.addNull();
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(ExprContextObject exprContextObject)
{
  std::string s = exprContextObject.getJexlContext();
  LM_T(LmtExpr, ("adding to JEXL expression context list (object): %s", s.c_str()));
  jh.addRaw(s);
}



/* ****************************************************************************
*
* ExprContextList::add -
*/
void ExprContextList::add(ExprContextList exprContextList)
{
  std::string s = exprContextList.get();
  LM_T(LmtExpr, ("adding to JEXL expression context list (list): %s", s.c_str()));
  jh.addRaw(s);
}
