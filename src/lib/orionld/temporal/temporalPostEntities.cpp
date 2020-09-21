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
* Author: Ken Zangelin, Chandra Challagonda
*/
extern "C"
{
#include "kbase/kMacros.h"                                     // K_FT
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/HttpStatusCode.h"                               // SccNotImplemented
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/rest/OrionLdRestService.h"                   // OrionLdRestService
#include "orionld/temporal/temporalPostEntities.h"             // Own interface



// ----------------------------------------------------------------------------
//
// temporalPostEntities -
//
bool temporalPostEntities(ConnectionInfo* ciP)
{
  char*  entityId   = orionldState.payloadIdNode->value.s;
  char*  entityType = orionldState.payloadTypeNode->value.s;


  //
  // Some traces just to see how the KjNode tree works
  //
  LM_TMP(("TMPF: Entity Id:   '%s'", entityId));
  LM_TMP(("TMPF: Entity Type: '%s'", entityType));
  LM_TMP(("TMPF: Context:     '%s'", orionldState.contextP->url));
  LM_TMP(("TMPF:"));

  for (KjNode* attrP = orionldState.requestTree->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    KjNode* attrValueP = kjLookup(attrP, "value");
    KjNode* attrTypeP  = kjLookup(attrP, "type");

    if (attrValueP == NULL)
      attrValueP = kjLookup(attrP, "object");  // Relationships have no "value" but an "object"

    LM_TMP(("TMPF: Attribute:   '%s':", attrP->name));
    LM_TMP(("TMPF:   Type:      '%s'", attrTypeP->value.s));

    if (attrValueP->type == KjString)
      LM_TMP(("TMPF:   Value:     '%s'", attrValueP->value.s));
    else if (attrValueP->type == KjInt)
      LM_TMP(("TMPF:   Value:     %d", attrValueP->value.i));
    else if (attrValueP->type == KjFloat)
      LM_TMP(("TMPF:   Value:     %f", attrValueP->value.f));
    else if (attrValueP->type == KjBoolean)
      LM_TMP(("TMPF:   Value:     '%s'", K_FT(attrValueP->value.b)));
    else if (attrValueP->type == KjNull)
      LM_TMP(("TMPF:   Value:     NULL"));
    else if (attrValueP->type == KjArray)
      LM_TMP(("TMPF:   Value:     ARRAY"));
    else if (attrValueP->type == KjObject)
      LM_TMP(("TMPF:   Value:     OBJECT"));
    else
      LM_TMP(("TMPF:   Value:     UNKNOWN: %d", attrValueP->type));
    LM_TMP(("TMPF:"));
  }

  return false;
}
