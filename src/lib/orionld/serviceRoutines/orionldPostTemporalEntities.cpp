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
* Author: Ken Zangelin
*/
extern "C"
{
#include "kjson/kjRender.h"                                         // kjFastRender
}

#include "logMsg/logMsg.h"                                          // LM_*
#include "logMsg/traceLevels.h"                                     // Lmt*

#include "orionld/common/orionldState.h"                            // orionldState
#include "orionld/common/numberToDate.h"                            // numberToDate
#include "orionld/http/httpHeaderLocationAdd.h"                     // httpHeaderLocationAdd
#include "orionld/payloadCheck/PCHECK.h"                            // PCHECK_*
#include "orionld/payloadCheck/pCheckEntityId.h"                    // pCheckEntityId
#include "orionld/payloadCheck/pCheckEntityType.h"                  // pCheckEntityType
#include "orionld/payloadCheck/pCheckAttribute.h"                   // pCheckAttribute
#include "orionld/mongoBackend/mongoEntityExists.h"                 // mongoEntityExists
#include "orionld/troe/troePostEntities.h"                          // troePostEntities
#include "orionld/serviceRoutines/orionldPostTemporalEntities.h"    // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPostTemporalEntities -
//
// The Payload Body for "POST /temporal/entities" looks like this:
//
// {
//   "@context": "https://...",
//   "id": "urn:ngsi-ld:entities:E1",
//   "type": "T",
//   "attr1": [
//     {
//       "type": "Property",
//       "value": 14,
//       "observedAt": "2021-04-20T08:31:00",
//       "sub-R1": { "type": "Relationship", "object": "urn:xxx" },
//       "sub-P1": { "type": "Property", "value": 14 },
//       ...
//     },
//     {
//       ...
//     }
//   ],
//   "attr2": [
//     {
//       "type": "Relationship",
//       "object": "urn:xxx",
//       "observedAt": "2021-04-20T08:31:00",
//       "sub-R1": { "type": "Relationship", "object": "urn:xxx" },
//       "sub-P1": { "type": "Property", "value": 14 },
//        ...
//     },
//     {
//       ...
//     }
//   ]
// }
//
// This request adds entries in the TRoE database for the entity urn:ngsi-ld:entities:E1 and all of its attrs and their sub-attrs
//
bool orionldPostTemporalEntities(void)
{
  char*    entityId;
  char*    entityType;

  // Is TRoE on?
  if (troe == false)
  {
    orionldError(OrionldResourceNotFound, "TRoE Not Enabled", orionldState.urlPath, 503);
    return false;
  }

  //
  // Check the entity id and type
  //
  PCHECK_OBJECT(orionldState.requestTree, 0, NULL, "To create an Entity, a JSON OBJECT describing the entity must be provided", 400);

  if (pCheckEntityId(orionldState.payloadIdNode,     true, &entityId)   == false)   return false;
  if (pCheckEntityType(orionldState.payloadTypeNode, true, &entityType) == false)   return false;

  //
  // NOTE
  //   payloadParseAndExtractSpecialFields() from mhdConnectionTreat() decouples the entity id and type
  //   from the payload body, so, the entity type is not expanded by pCheckEntity()
  //   The expansion is instead done by payloadTypeNodeFix, called by mhdConnectionTreat
  //


  //
  // Check/Expand all attributes (RHS can be Array or Object), using pCheckAttribute
  //
  // FIXME: location, observationSpace, operationSpace
  //
  for (KjNode* attrP = orionldState.requestTree->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (attrP->type == KjArray)
    {
      for (KjNode* aInstanceP = attrP->value.firstChildP; aInstanceP != NULL; aInstanceP = aInstanceP->next)
      {
        aInstanceP->name = attrP->name;  // Need to "inherit" the name for the Array

        if (pCheckAttribute(entityId, aInstanceP, true, NoAttributeType, false, NULL) == false)
          return false;

        attrP->name = aInstanceP->name;  // It's been expanded ionside pCheckAttribute
      }
    }
    else  // KjObject
    {
      if (pCheckAttribute(entityId, attrP, true, NoAttributeType, false, NULL) == false)
        return false;
    }
  }

  // Does the entity already exist?
  // If so, it's a 204, not a 201
  //
  // FIXME: This check should really be made in the TRoE database but, seems valid enough to do the
  //        search in mongo instead
  //
  int httpStatusCode = 201;

  if (mongoEntityExists(entityId, orionldState.tenantP) == true)
    httpStatusCode = 204;

  //
  // Nothing is sent to mongo, only TRoE is updated
  // And, the TRoE function is invoked by mhdConnectionTreat
  //
  numberToDate(orionldState.requestTime, orionldState.requestTimeString, sizeof(orionldState.requestTimeString));
  orionldState.troeOpMode = TROE_ENTITY_REPLACE;
  bool troeOk = troePostEntities();

  if (troeOk == true)
  {
    if (httpStatusCode == 201)
      httpHeaderLocationAdd("/ngsi-ld/v1/temporal/entities/", entityId, orionldState.tenantP->tenant);

    orionldState.httpStatusCode = httpStatusCode;
    return true;
  }

  LM_E(("troePostEntities failed (%s: %s)", orionldState.pd.title, orionldState.pd.detail));

  return false;
}
