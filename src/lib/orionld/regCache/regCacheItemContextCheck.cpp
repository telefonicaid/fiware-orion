/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldContext.h"                        // OrionldContext
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/context/orionldContextFromUrl.h"               // orionldContextFromUrl
#include "orionld/regCache/regCacheItemContextCheck.h"           // Own interface



// -----------------------------------------------------------------------------
//
// kjValueFromKvList - FIXME: Own module in kjTree library
//
static char* kjValueFromKvList(KjNode* kvArray, const char* key)
{
  for (KjNode* kvPair = kvArray->value.firstChildP; kvPair != NULL; kvPair = kvPair->next)
  {
    KjNode* keyNodeP = kjLookup(kvPair, "key");

    if (strcmp(keyNodeP->value.s, key) == 0)
    {
      KjNode* valueNodeP = kjLookup(kvPair, "value");

      return valueNodeP->value.s;
    }
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// regCacheItemContextCheck -
//
// PARAMETERS
// * apiRegP            - KjNode tree of the entire API formatted Registration (to lookup the "jsonldContext" from "contextSourceInfo"
// * jsonldContextValue - Direct value of "jsonldContext" - NULL if not known
// * fwdContextPP       - To give back the context returned from orionldContextFromUrl (to be inserted ionto the reg-cache item)
//
bool regCacheItemContextCheck(KjNode* apiRegP, char* jsonldContextValue, OrionldContext** fwdContextPP)
{
  *fwdContextPP = NULL;

  char* jsonldContext = jsonldContextValue;
  if (jsonldContext == NULL)
  {
    KjNode* cSourceInfoP = kjLookup(apiRegP, "contextSourceInfo");

    if (cSourceInfoP == NULL)
      return true;

    jsonldContext = kjValueFromKvList(cSourceInfoP, "jsonldContext");
    if (jsonldContext == NULL)
      return true;
  }

  *fwdContextPP = orionldContextFromUrl(jsonldContext, NULL);
  if (*fwdContextPP == NULL)
    return false;

  return true;
}
