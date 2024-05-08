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
#include "common/statistics.h"

// Interface to use libcjexl.a
extern "C" {
    void* cjexl_new_engine();
}

extern "C" {
   void cjexl_free_engine(void* ptr);
}

extern "C" {
   const char* cjexl_eval(void* ptr, const char* script_ptr, const char* context_ptr);
}


/* ****************************************************************************
*
* ExprManager::init -
*/
void ExprManager::init(void)
{
  jexlEngine = cjexl_new_engine();
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

    TIME_EXPR_LEGACY_EVAL_START();
    LM_T(LmtExpr, ("evaluating legacy expression: <%s>", _expression.c_str()));

    std::map<std::string, std::string>* replacementsP = exprContextObjectP->getMap();

    std::map<std::string, std::string>::iterator iter = replacementsP->find(_expression);
    if (iter != replacementsP->end())
    {
      r.valueType   = orion::ValueTypeString;
      r.stringValue = iter->second;
      LM_T(LmtExpr, ("legacy evaluation result: <%s>", r.stringValue.c_str()));
    }
    TIME_EXPR_LEGACY_EVAL_STOP();
  }
  else
  {
    // JEXL based evaluation

    TIME_EXPR_JEXL_EVAL_START();
    std::string context = exprContextObjectP->getJexlContext();
    LM_T(LmtExpr, ("evaluating JEXL expression <%s> with context <%s>", _expression.c_str(), context.c_str()));
    const char* result = cjexl_eval(jexlEngine, _expression.c_str(), context.c_str());
    LM_T(LmtExpr, ("JEXL evaluation result: <%s>", result));

    // The ExprResult::fill() method allocates dynamic memory. So, the callers to evaluate() are supposed to invoke ExprResult::release()
    // method to free it
    r.fill(result);
    TIME_EXPR_JEXL_EVAL_STOP();
  }

  return r;
}



/* ****************************************************************************
*
* ExprManager::release -
*/
void ExprManager::release(void)
{
  cjexl_free_engine(jexlEngine);
}