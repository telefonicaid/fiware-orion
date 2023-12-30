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
#include "kjson/kjClone.h"                                       // kjClone
#include "kjson/kjBuilder.h"                                     // kjChildAdd
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/types/DistOpType.h"                            // DistOpType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/regMatch/regMatchAttributes.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// regMatchAttributes -
//
KjNode* regMatchAttributes(RegCacheItem* regP, DistOpType operation, KjNode* propertyNamesP, KjNode* relationshipNamesP, KjNode* payloadBody)
{
  //
  // Registration of entire entity?
  // 'exclusive' registrations can't do that
  //
  if ((propertyNamesP == NULL) && (relationshipNamesP == NULL))
  {
    if (operation == DoUpdateAttrs)
    {
      //
      // IMPORTANT !!!
      // To make "PATCH Attribute" able to use "regMatchForEntityCreation", the incoming body was "enhanced":
      //
      //   Incoming: { "value": 12 }
      // Was Changed to:
      //   Modified: { "attrName" { "value": 12 } }
      //
      // That way, the body of "PATCH Attribute" is on the same level as the body of "POST /entities", thus
      // regMatchForEntityCreation can be used as is.
      // Now, before cloning the payload body for distributed requests, this modification needs to be rolled back.
      //
      LM_T(LmtRegMatch, ("It's PATCH Attribute, so, the payload body is one level down"));
      payloadBody = payloadBody->value.firstChildP;
    }

    KjNode* body = kjClone(orionldState.kjsonP, payloadBody);

    // Add entity type and id
    if (orionldState.payloadIdNode != NULL)  // WAS: if ((operation != DoUpdateAttrs) && (operation != DoMergeEntity))
    {
      KjNode* idP   = kjClone(orionldState.kjsonP, orionldState.payloadIdNode);
      kjChildAdd(body, idP);
    }

    if (orionldState.payloadTypeNode != NULL)
    {
      KjNode* typeP = kjClone(orionldState.kjsonP, orionldState.payloadTypeNode);
      kjChildAdd(body, typeP);
    }

    return body;
  }


  KjNode* attrObject = NULL;
  KjNode* attrP      = payloadBody->value.firstChildP;
  KjNode* next;

  while (attrP != NULL)
  {
    next = attrP->next;

    KjNode* matchP = NULL;

    if (propertyNamesP != NULL)
      matchP = kjStringValueLookupInArray(propertyNamesP, attrP->name);
    if ((matchP == NULL) && (relationshipNamesP != NULL))
      matchP = kjStringValueLookupInArray(relationshipNamesP, attrP->name);

    if (matchP == NULL)
    {
      attrP = next;
      continue;
    }

    //
    // 'inclusive' and 'redirect' registrations must CLONE the attribute (as all DistOp items have their own body)
    // The third one (exclusive) STEALS the attribute.
    // However ... once all exclusive registrations are done chopping off attributes, the entity will stay intact.
    // That's interesting!!!
    // So:
    // 1. Treat only exclusive regs - chop off
    // 2. Treat all other regs - all of them can point to the very same payload body (what's left in orionldState.requestTree)
    //    - Only, what if registrations have different @context ... ?  The short-names can differ
    //      Might need all those clones anyway!    Pity ...
    //
    // Actually, as the regs can have different attributes registered, the payload may differ in that respect as well.
    //
    // FORGET ALL ABOUT THIS (I'm keeping the comment though, as it's important info), we MUST CLONE
    //
    if (regP->mode == RegModeExclusive)
    {
      matchP = attrP;
      kjChildRemove(payloadBody, attrP);
    }
    else
      matchP = kjClone(orionldState.kjsonP, attrP);

    if (attrObject == NULL)
      attrObject = kjObject(orionldState.kjsonP, NULL);

    kjChildAdd(attrObject, matchP);

    attrP = next;
  }

  if (attrObject == NULL)
    LM_T(LmtRegMatch, ("No match due to no matching attributes"));
  else if (operation == DoUpdateAttrs)
  {
    //
    // IMPORTANT !!!
    // To make "PATCH Attribute" able to use "regMatchForEntityCreation", the incoming body was "enhanced":
    //
    //   Incoming: { "value": 12 }
    // Was Changed to:
    //   Modified: { "attrName" { "value": 12 } }
    //
    // That way, the body of "PATCH Attribute" is on the same level as the body of "POST /entities" and regMatchForEntityCreation can be used as is.
    // Now, before cloning the payload body for distributed requests, this modification needs to be rolled back.
    //
    LM_T(LmtRegMatch, ("It's PATCH Attribute, so, the payload body is one level down"));
    return attrObject->value.firstChildP;
  }

  return attrObject;
}
