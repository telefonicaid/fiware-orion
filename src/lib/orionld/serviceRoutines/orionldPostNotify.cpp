/*
*
 Copyright 2021 FIWARE Foundation e.V.
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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjClone.h"                                     // kjClone
}

#include "common/globals.h"                                    // parse8601Time
#include "rest/ConnectionInfo.h"                               // ConnectionInfo

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/serviceRoutines/orionldPostBatchUpsert.h"    // orionldPostBatchUpsert
#include "orionld/serviceRoutines/orionldPostNotify.h"         // Own interface



// ----------------------------------------------------------------------------
//
// orionldPostNotify -
//
bool orionldPostNotify(ConnectionInfo* ciP)
{
  KjNode* dataArray = kjLookup(orionldState.requestTree, "data");

  if (dataArray == NULL)
  {
    LM_W(("Invalid Notification (the 'data' member is missing)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Notification", "The 'data' member is missing");
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  if (dataArray->type != KjArray)
  {
    LM_W(("Invalid Notification (the 'data' member is present but it is not a GSON Array - it is a JSON %s)", kjValueType(dataArray->type)));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Notification", "The 'data' member is present but it is not an array");
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  //
  // All good - letting BATCH UPSERT (update, not replace) take care of the request
  // For this to work, I simply have to call the service routine, after:
  // * Setting the "incoming tree" to point to the value of the "data" array
  // * Setting the URI param "option" to "update"
  //
  orionldState.requestTree            = dataArray;
  orionldState.uriParamOptions.update = true;

  return orionldPostBatchUpsert(ciP);
}
