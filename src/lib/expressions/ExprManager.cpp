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

#include "expressions/ExprManager.h"
#include "expressions/ExprResult.h"
#include "logMsg/logMsg.h"

#include "orionTypes/OrionValueType.h"

// Interface to use libcjexl.a
extern "C" {
    void* new_engine();
}

extern "C" {
   void free_engine(void* ptr);
}

extern "C" {
   const char* eval(void* ptr, const char* script_ptr, const char* context_ptr);
}


/* ****************************************************************************
*
* ExprManager::init -
*/
void ExprManager::init(void)
{
  // FIXME PR: this is probably not needed
  //if (sem_init(&sem, 0, 1) == -1)
  //{
  //  LM_X(1, ("Fatal Error (error initializing 'jexl mgr' semaphore: %s)", strerror(errno)));
  //}

  jexlEngine = new_engine();
}



/* ****************************************************************************
*
* ExprManager::evaluate -
*/
ExprResult ExprManager::evaluate(ExprContextObject* exprContextObjectP, const std::string& _expression)
{
  ExprResult r;
  r.valueType = orion::ValueTypeNull;

  if (exprContextObjectP->isLegacy())
  {
    // std::map based evaluation. Only pure replacement is supported
    LM_T(LmtExpr, ("evaluating legacy expresion: <%s>", _expression.c_str()));

    std::map<std::string, std::string>* replacementsP = exprContextObjectP->getMap();

    std::map<std::string, std::string>::iterator iter = replacementsP->find(_expression);
    if (iter != replacementsP->end())
    {
      r.valueType   = orion::ValueTypeString;
      r.stringValue = iter->second;
    }
  }
  else
  {
    // JEXL based evaluation
    LM_T(LmtExpr, ("evaluating JEXL expresion: <%s>", _expression.c_str()));
    const char* result = eval(jexlEngine, _expression.c_str(), exprContextObjectP->getJexlContext().c_str());
    r.fill(result);
  }

  return r;
}



/* ****************************************************************************
*
* ExprManager::release -
*/
void ExprManager::release(void)
{
  free_engine(jexlEngine);
}