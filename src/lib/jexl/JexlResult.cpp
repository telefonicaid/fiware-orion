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

#include "jexl/JexlResult.h"

#include "common/string.h"
#include "common/JsonHelper.h"
#include "logMsg/logMsg.h"

/* ****************************************************************************
*
* toString -
*
* Pretty similar to ContextAttribute::toJsonValue()
*
* FIXME PR: ValueTypeVector and ValueTypeObject should be taken into account
*/
std::string JexlResult::toString(void)
{
  if (valueType == orion::ValueTypeNumber)
  {
    return double2string(numberValue);
  }
  else if (valueType == orion::ValueTypeBoolean)
  {
    return boolValue ? "true" : "false";
  }
  else if (valueType == orion::ValueTypeString)
  {
    return "\"" + toJsonString(stringValue) + "\"";
  }
  else if ((valueType == orion::ValueTypeObject)||(valueType == orion::ValueTypeVector))
  {
    return stringValue;
  }
  else if (valueType == orion::ValueTypeNull)
  {
    return "null";
  }
  else
  {
    LM_E(("Runtime Error (not allowed type in JexlResult: %s)", valueTypeName(valueType)));
    return "";
  }
}