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

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*



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
// FIXME: Implement matching over idPattern as well
//
bool regMatchEntityInfo(KjNode* entityInfoP, const char* entityId, const char* entityType)
{
  KjNode* idP         = kjLookup(entityInfoP, "id");
  KjNode* idPatternP  = kjLookup(entityInfoP, "idPattern");
  KjNode* typeP       = kjLookup(entityInfoP, "type");

  //
  // "type" is mandatory for all 'entityInfo'
  // "id" is mandatory for 'exclusive' registrations
  //
  //
  if (typeP == NULL)
  {
    LM(("RM: No match due to invalid registration (no type in EntityInfo)"));
    return false;
  }

  if (strcmp(typeP->value.s, entityType) != 0)
  {
    LM(("RM: No match due to entity type ('%s' in reg, '%s' in entity creation)", typeP->value.s, entityType));
    return false;
  }

  if (idP != NULL)
  {
    if (strcmp(idP->value.s, entityId) != 0)
    {
      LM(("RM: No match due to entity id ('%s' in reg, '%s' in entity creation)", idP->value.s, entityId));
      return false;
    }
  }
  else if (idPatternP != NULL)
  {
    // FIXME: Implement idPattern matching
    LM_W(("Sorry, idPattern matching is not yet implemented ..."));
    return false;
  }

  return true;
}
