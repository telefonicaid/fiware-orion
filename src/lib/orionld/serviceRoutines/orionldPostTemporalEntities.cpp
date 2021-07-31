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

#include "rest/ConnectionInfo.h"                                    // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                     // httpHeaderLocationAdd

#include "orionld/common/orionldState.h"                            // orionldState
#include "orionld/common/orionldErrorResponse.h"                    // orionldErrorResponseCreate
#include "orionld/common/CHECK.h"                                   // OBJECT_CHECK
#include "orionld/common/numberToDate.h"                            // numberToDate
#include "orionld/rest/OrionLdRestService.h"                        // OrionLdRestService
#include "orionld/payloadCheck/pcheckEntity.h"                      // pcheckEntity
#include "orionld/payloadCheck/pcheckUri.h"                         // pcheckUri
#include "orionld/troe/troePostEntities.h"                          // troePostEntities
#include "orionld/mongoBackend/mongoEntityExists.h"                  // mongoEntityExists
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
bool orionldPostTemporalEntities(ConnectionInfo* ciP)
{
  OBJECT_CHECK(orionldState.requestTree, "toplevel");

  char*    detail;
  KjNode*  locationP          = NULL;
  KjNode*  observationSpaceP  = NULL;
  KjNode*  operationSpaceP    = NULL;
  KjNode*  createdAtP         = NULL;
  KjNode*  modifiedAtP        = NULL;

  if (pcheckEntity(orionldState.requestTree->value.firstChildP, &locationP, &observationSpaceP, &operationSpaceP, &createdAtP, &modifiedAtP, false) == false)
    return false;

  char*    entityId           = orionldState.payloadIdNode->value.s;
  char*    entityType         = orionldState.payloadTypeNode->value.s;


  //
  // Entity ID and TYPE
  //
  if (pcheckUri(entityId, true, &detail) == false)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity id", "The id specified cannot be resolved to a URL or URN");  // FIXME: Include 'detail' and name (entityId)
    return false;
  }

  if (pcheckUri(entityType, false, &detail) == false)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity Type", detail);  // FIXME: Include 'detail' and name (entityId)
    return false;
  }


  // Does the entity already exist?
  // FIXME: This check should really be made in the TRoE database but, seems valid enough to do the
  //        search in mongo instead
  //
  int httpStatusCode = 201;

  if (mongoEntityExists(entityId, orionldState.tenantP) == true)
    httpStatusCode = 204;

  //
  // Nothing is sent to mongo, only TRoE is updated
  // And, the TRoE function is invoked by orionldMhdConnectionTreat
  //
  numberToDate(orionldState.requestTime, orionldState.requestTimeString, sizeof(orionldState.requestTimeString));
  orionldState.troeOpMode = TROE_ENTITY_REPLACE;
  bool troeOk = troePostEntities(ciP);

  if (troeOk == true)
  {
    if (httpStatusCode == 201)
      httpHeaderLocationAdd(ciP, "/ngsi-ld/v1/temporal/entities/", entityId);

    orionldState.httpStatusCode = httpStatusCode;
    return true;
  }

  LM_E(("troePostEntities failed (%s: %s)", orionldState.pd.title, orionldState.pd.detail));
  orionldErrorResponseCreate(orionldState.pd.type, orionldState.pd.title, orionldState.pd.detail);
  orionldState.httpStatusCode = orionldState.pd.type;

  return false;
}
