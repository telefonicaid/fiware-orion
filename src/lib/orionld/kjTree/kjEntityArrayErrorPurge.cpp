/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
* Author: Ken Zangelin, Gabriel Quaresma
*/
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjChildRemove
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/kjTree/kjEntityIdLookupInEntityArray.h"      // kjEntityIdLookupInEntityArray
#include "orionld/kjTree/kjStringValueLookupInArray.h"         // kjStringValueLookupInArray
#include "orionld/kjTree/kjEntityArrayErrorPurge.h"            // Own Interface



// -----------------------------------------------------------------------------
//
// kjEntityArrayErrorPurge -
//
// Any entities flagged as erroneous must be removed from the cloned array for TRoE
// - except if the entity id is present in the success array, i.e. present in both arrays
//
void kjEntityArrayErrorPurge(KjNode* incomingTree, KjNode* errorsArrayP, KjNode* successArrayP)
{
  for (KjNode* errorP = errorsArrayP->value.firstChildP; errorP != NULL; errorP = errorP->next)
  {
    KjNode* eId = kjLookup(errorP, "entityId");

    if (eId != NULL)  // Protection
    {
      KjNode* entityObjectP = kjEntityIdLookupInEntityArray(incomingTree, eId->value.s);

      if (entityObjectP != NULL)
      {
        // The entity from the 'incomingTree' IS PRESENT in the ERROR ARRAY - what about the SUCCESS ARRAY ?
        entityObjectP = kjStringValueLookupInArray(successArrayP, eId->value.s);

        // Remove only if NOT PRESENT in success array
        if (entityObjectP == NULL)
          kjChildRemove(incomingTree, entityObjectP);
      }
    }
  }
}
