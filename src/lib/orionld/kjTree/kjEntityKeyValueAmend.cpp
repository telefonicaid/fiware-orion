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
#include "kbase/kMacros.h"                                       // K_VEC_SIZE
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, kjChildAdd, kjChildRemove
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType, orionldAttributeTypeName
#include "orionld/common/orionldState.h"                         // orionldState, coreContextUrl
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/attributeTypeFromUriParam.h"            // attributeTypeFromUriParam
#include "orionld/kjTree/kjEntityKeyValueAmend.h"                // Own interface



// -----------------------------------------------------------------------------
//
// kjEntityKeyValueAmend -
//
KjNode* kjEntityKeyValueAmend(KjNode* entityP)
{
  KjNode*      outP        = kjObject(orionldState.kjsonP, NULL);
  const char*  keepAsIs[5] = { "id", "@id", "type", "@type", "scope" };

  for (unsigned int ix = 0; ix < K_VEC_SIZE(keepAsIs); ix++)
  {
    KjNode* nodeP = kjLookup(entityP, keepAsIs[ix]);

    if (nodeP != NULL)
    {
      kjChildRemove(entityP, nodeP);
      kjChildAdd(outP, nodeP);
    }
  }

  KjNode* attrNodeP    = entityP->value.firstChildP;
  KjNode* next;

  while (attrNodeP != NULL)
  {
    next = attrNodeP->next;

    kjChildRemove(entityP, attrNodeP);

    KjNode*               objectP = kjObject(orionldState.kjsonP, attrNodeP->name);
    OrionldAttributeType  attributeType;

    if      (strcmp(attrNodeP->name, "location")         == 0)  attributeType = GeoProperty;
    else if (strcmp(attrNodeP->name, "observationSpace") == 0)  attributeType = GeoProperty;
    else if (strcmp(attrNodeP->name, "operationSpace")   == 0)  attributeType = GeoProperty;
    else                                                        attributeType = attributeTypeFromUriParam(attrNodeP->name);

    attrNodeP->name = (attributeType == Relationship)? (char*) "object" : (char*) "value";
    kjChildAdd(objectP, attrNodeP);

    KjNode* typeP = kjString(orionldState.kjsonP, "type", orionldAttributeTypeName(attributeType));
    kjChildAdd(objectP, typeP);

    kjChildAdd(outP, objectP);

    attrNodeP = next;
  }

  return outP;
}
