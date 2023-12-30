
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
#include <string.h>                                              // strcmp
#include <regex.h>                                               // regexec

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/types/RegIdPattern.h"                          // RegIdPattern
#include "orionld/regMatch/regMatchEntityInfo.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// regIdPatternLookup - FIXME: Own module orionld/regMatch/regIdPatternLookup.cpp/h
//
RegIdPattern* regIdPatternLookup(RegCacheItem* regP, KjNode* idPatternP)
{
  RegIdPattern* ripP = regP->idPatternRegexList;

  while (ripP != NULL)
  {
    if (ripP->owner == idPatternP)
      return ripP;

    ripP = ripP->next;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// regMatchEntityInfo -
//
// Matching with "information::entities[X]"
//
// "information": [
//   {
//     "entities": [
//       {
//         "id": "urn:E1",
//         "idPattern": "xxx",   # Can't be present in an exclusive registration - BUT for others it can ...
//         "type": "T"
//       }
//     ],
//     "propertyNames": [ "P1", "P2" ]
//     "relationshipNames": [ "R1", "R2" ]
//   }
// ]
//
bool regMatchEntityInfo(RegCacheItem* regP, KjNode* entityInfoP, const char* entityId, const char* entityType)
{
  KjNode* idP         = kjLookup(entityInfoP, "id");
  KjNode* idPatternP  = kjLookup(entityInfoP, "idPattern");
  KjNode* typeP       = kjLookup(entityInfoP, "type");

  if (typeP == NULL)
  {
    LM_E(("%s: invalid registration (no type in information::entities)"));
    return false;
  }

  //
  // "id" is mandatory for 'exclusive' registrations
  // "type" is mandatory for all 'entityInfo' BUT, when matching
  // for GET /entities or /entities/{entityId} then there may be no entityType to compare with
  //
  if (entityType != NULL)
  {
    KjNode* typeP = kjLookup(entityInfoP, "type");

    if (typeP == NULL)
    {
      LM_T(LmtRegMatch, ("%s: No match due to invalid registration (no type in EntityInfo)", regP->regId));
      return false;
    }

    if (strcmp(typeP->value.s, entityType) != 0)
    {
      LM_T(LmtRegMatch, ("%s: No match due to entity type ('%s' in reg, '%s' in entity creation)", regP->regId, typeP->value.s, entityType));
      return false;
    }
  }

  if (idP != NULL)
  {
    if (strcmp(idP->value.s, entityId) != 0)
    {
      LM_T(LmtRegMatch, ("%s: No match due to entity id ('%s' in reg, '%s' in entity creation)", regP->regId, idP->value.s, entityId));
      return false;
    }
  }
  else if (idPatternP != NULL)
  {
    RegIdPattern* ripP = regIdPatternLookup(regP, idPatternP);  // I need the RegCacheItem here ...

    if (ripP == NULL)
    {
      LM_E(("Internal Error (%s: the regex corresponding to this idPattern could not be found)", regP->regId));
      return false;
    }
    else
    {
      if (regexec(&ripP->regex, entityId, 0, NULL, 0) != 0)
      {
        LM_T(LmtRegMatch, ("%s: No match due to entity idPattern", regP->regId));
        return false;
      }
    }
  }

  return true;
}
