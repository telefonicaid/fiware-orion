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

#include "expressions/ExprManager.h"
#include "expressions/ExprResult.h"
#include "expressions/exprCommon.h"
#include "logMsg/logMsg.h"

#include "orionTypes/OrionValueType.h"



/* ****************************************************************************
*
* ExprManager::init -
*/
void ExprManager::init(void)
{
  pyjexlModule         = NULL;
  jexlEngine           = NULL;

  if (sem_init(&sem, 0, 1) == -1)
  {
    LM_X(1, ("Fatal Error (error initializing 'jexl mgr' semaphore: %s)", strerror(errno)));
  }

  Py_Initialize();
  LM_T(LmtExpr, ("Python interpreter has been initialized"));

  pyjexlModule = PyImport_ImportModule("pyjexl");
  if (pyjexlModule == NULL)
  {
    const char* error = capturePythonError();
    LM_X(1, ("Fatal Error (error importing pyjexl module: %s)", error));
  }
  LM_T(LmtExpr, ("pyjexl module has been loaded"));

  jexlEngine = PyObject_CallMethod(pyjexlModule, "JEXL", NULL);
  if (jexlEngine == NULL)
  {
    const char* error = capturePythonError();
    LM_X(1, ("Fatal Error (error creating jexlEngine: %s)", error));
  }
  LM_T(LmtExpr, ("jexl engine has been created"));
}



/* ****************************************************************************
*
* ExprManager::evaluate -
*/
ExprResult ExprManager::evaluate(ExprContextObject* exprContextObjectP, const std::string& _expression)
{
  ExprResult r;
  r.valueType = orion::ValueTypeNull;

  LM_T(LmtExpr, ("evaluating JEXL expresion: <%s>", _expression.c_str()));

  PyObject* expression = Py_BuildValue("s", _expression.c_str());
  if (expression == NULL)
  {
    // FIXME PR: use LM_E/LM_W?
    LM_T(LmtExpr, ("error building expression: %s", capturePythonError()));
    return r;
  }

  PyObject* result = PyObject_CallMethod(jexlEngine, "evaluate", "OO", expression, exprContextObjectP->get());
  Py_XDECREF(expression);
  if (result == NULL)
  {
    // FIXME PR: use LM_E/LM_W?
    LM_T(LmtExpr, ("error evaluating expression: %s", capturePythonError()));
    return r;
  }

  r.fill(result);

  // FIXME PR: does this Py_XDECREF() recursively in the case of dicts or lists?
  Py_XDECREF(result);

  return r;
}



/* ****************************************************************************
*
* ExprManager::release -
*/
void ExprManager::release(void)
{
  if (jexlEngine != NULL)
  {
    Py_XDECREF(jexlEngine);
    LM_T(LmtExpr, ("jexl engine has been freed"));
  }

  if (pyjexlModule != NULL)
  {
    Py_XDECREF(pyjexlModule);
    LM_T(LmtExpr, ("pyjexl module has been freed"));
  }

  Py_Finalize();
  LM_T(LmtExpr, ("Python interpreter has been finalized"));
}