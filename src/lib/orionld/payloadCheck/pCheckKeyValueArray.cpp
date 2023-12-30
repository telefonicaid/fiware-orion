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
#include "kjson/KjNode.h"                                       // KjNode
}

#include "orionld/types/OrionldContext.h"                       // OrionldContext
#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/kjTree/kjLookupInKvList.h"                    // kjLookupInKvList
#include "orionld/regCache/regCacheItemContextCheck.h"          // regCacheItemContextCheck
#include "orionld/payloadCheck/PCHECK.h"                        // PCHECK_*
#include "orionld/payloadCheck/pCheckKeyValueArray.h"           // Own interface



// -----------------------------------------------------------------------------
//
// pCheckKeyValueArray - FIXME: Bad Name, use pCheckRegistrationCSourceInfo
//
// Keys with special treatment:
// * jsonldContext - needs to be downloaded, parsed etc, to be accepted.
// * Content-Type  - must be a valid mime type
// * Accept        - must be a valid Accept value (parse it and make sure)
//
bool pCheckKeyValueArray(KjNode* csiP, OrionldContext** fwdContextPP)
{
  for (KjNode* kvObjectP = csiP->value.firstChildP; kvObjectP != NULL; kvObjectP = kvObjectP->next)
  {
    KjNode* keyP   = NULL;
    KjNode* valueP = NULL;

    PCHECK_OBJECT(kvObjectP, 0, NULL, "Registration::contextSourceInfo[X]", 400);
    PCHECK_OBJECT_EMPTY(kvObjectP, 0, NULL, "Registration::contextSourceInfo[X]", 400);

    for (KjNode* kvItemP = kvObjectP->value.firstChildP; kvItemP != NULL; kvItemP = kvItemP->next)
    {
      if (strcmp(kvItemP->name, "key") == 0)
      {
        PCHECK_DUPLICATE(keyP, kvItemP, 0, "Duplicated field in Registration::contextSourceInfo[X]", kvItemP->name, 400);
        PCHECK_STRING(keyP, 0, "Non-String item in Registration::contextSourceInfo[X]", kvItemP->name, 400);
        PCHECK_STRING_EMPTY(keyP, 0, "Empty String item in Registration::contextSourceInfo[X]", kvItemP->name, 400);
      }
      else if (strcmp(kvItemP->name, "value") == 0)
      {
        PCHECK_DUPLICATE(valueP, kvItemP, 0, "Duplicated field in Registration::contextSourceInfo[X]", kvItemP->name, 400);
        PCHECK_STRING(valueP, 0, "Non-String item in Registration::contextSourceInfo[X]", kvItemP->name, 400);
        PCHECK_STRING_EMPTY(valueP, 0, "Empty String item in Registration::contextSourceInfo[X]", kvItemP->name, 400);
      }
      else
      {
        orionldError(OrionldBadRequestData, "Unrecognized field in Registration::contextSourceInfo[X]", kvItemP->name, 400);
        return false;
      }
    }

    if (keyP == NULL)
    {
      orionldError(OrionldBadRequestData, "Missing Mandatory Field in Registration::contextSourceInfo[X]", "key", 400);
      return false;
    }

    if (valueP == NULL)
    {
      orionldError(OrionldBadRequestData, "Missing Mandatory Field in Registration::contextSourceInfo[X]", "value", 400);
      return false;
    }

    //
    // Check for duplicates
    //
    if ((kvObjectP->next != NULL) && (kjLookupInKvList(kvObjectP, keyP->value.s) != NULL))
    {
      orionldError(OrionldBadRequestData, "Duplicated Key in Registration::contextSourceInfo", keyP->value.s, 400);
      return false;
    }

    // Special keys
    if (strcasecmp(keyP->value.s, "jsonldContext") == 0)
    {
      // If an @context is given for the registration, make sure it's valid
      if (regCacheItemContextCheck(NULL, valueP->value.s, fwdContextPP) == false)
      {
        LM_W(("Unable to add Registration @context '%s' for an item in the reg-cache", valueP->value.s));
        return 0;
      }
    }
    else if (strcasecmp(keyP->value.s, "Date") == 0)
    {
      orionldError(OrionldBadRequestData, "Invalid key in Registration::contextSourceInfo", keyP->value.s, 400);
      return false;
    }
    else if (strcasecmp(keyP->value.s, "Content-Length") == 0)
    {
      orionldError(OrionldBadRequestData, "Invalid key in Registration::contextSourceInfo", keyP->value.s, 400);
      return false;
    }
    else if (strcasecmp(keyP->value.s, "User-Agent") == 0)
    {
      orionldError(OrionldBadRequestData, "Invalid key in Registration::contextSourceInfo", keyP->value.s, 400);
      return false;
    }
    else if (strcasecmp(keyP->value.s, "NGSILD-Tenant") == 0)
    {
      orionldError(OrionldBadRequestData, "Invalid key in Registration::contextSourceInfo", keyP->value.s, 400);
      return false;
    }
    else if (strcasecmp(keyP->value.s, "Content-Type") == 0)
    {
      orionldError(OrionldBadRequestData, "Invalid key in Registration::contextSourceInfo", keyP->value.s, 400);
      return false;
    }
  }

  return true;
}
