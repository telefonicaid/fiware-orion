/*
*
* Copyright 2019 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode, kjValueType
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/payloadCheck/pcheckLanguagePropertyValue.h"    // Own interface



// ----------------------------------------------------------------------------
//
// pcheckLanguagePropertyValue -
//
bool pcheckLanguagePropertyValue(KjNode* valueP, const char* attrName)
{
  if (valueP->type != KjObject)
  {
    orionldError(OrionldBadRequestData, "The languageMap of a LanguageProperty attribute must be a JSON Object", attrName, 400);
    return false;
  }

  for (KjNode* itemP = valueP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (itemP->type != KjString)
    {
      orionldError(OrionldBadRequestData, "Items of the value of a LanguageProperty attribute must be JSON Strings", attrName, 400);
      return false;
    }

    if ((itemP->name[0] == 0) || (itemP->value.s[0] == 0))
    {
      orionldError(OrionldBadRequestData, "An item of the value (languageMap) of a LanguageProperty is an empty string", attrName, 400);
      return false;
    }
  }

  return true;
}
