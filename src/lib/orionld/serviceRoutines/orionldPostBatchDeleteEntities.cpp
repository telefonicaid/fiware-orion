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
* Author: Larysse Savanna
*/
extern "C"
{
#include "kjson/KjNode.h"                                             // KjNode
#include "kjson/kjBuilder.h"                                          // kjString, kjObject, ...
#include "kjson/kjRender.h"                                           // kjRender
}

#include "logMsg/logMsg.h"                                            // LM_*
#include "logMsg/traceLevels.h"                                       // Lmt*

#include "rest/ConnectionInfo.h"                                      // ConnectionInfo
#include "ngsi10/UpdateContextRequest.h"                              // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                             // UpdateContextResponse
#include "orionld/common/urlCheck.h"                                  // urlCheck
#include "orionld/common/urnCheck.h"                                  // urnCheck
#include "orionld/common/orionldState.h"                              // orionldState
#include "orionld/common/orionldErrorResponse.h"                      // orionldErrorResponseCreate
#include "orionld/db/dbEntityBatchDelete.h"                           // dbEntityBatchDelete.h
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityBatchDelete.h"   // mongoCppLegacyEntityBatchDelete
#include "orionld/serviceRoutines/orionldPostBatchDeleteEntities.h"   // Own interface



// ----------------------------------------------------------------------------
//
// orionldPostBatchDeleteEntities -
//
bool orionldPostBatchDeleteEntities(ConnectionInfo* ciP)
{
  LM_TMP(("LARYSSE: Payload is a JSON %s", kjValueType(orionldState.requestTree->type)));

  if (orionldState.requestTree->type != KjArray)
  {
    LM_W(("Bad Input (Payload must be a JSON Array)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid payload", "Must be a JSON Array");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  //
  // Making sure all items of the array are stringa and valid URIs
  //
  for (KjNode* idNodeP = orionldState.requestTree->value.firstChildP; idNodeP != NULL; idNodeP = idNodeP->next)
  {
    char* detail;

    if (idNodeP->type != KjString)
    {
      LM_W(("Bad Input (Invalid payload - Array items must be JSON Strings)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid payload", "Array items must be JSON Strings");
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }

    if (!urlCheck(idNodeP->value.s, &detail) && !urnCheck(idNodeP->value.s, &detail))
    {
      LM_W(("Bad Input (Invalid payload - Array items must be valid URIs)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid payload", "Array items must be valid URIs");
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }
  }

  if (mongoCppLegacyEntityBatchDelete(orionldState.requestTree) == false)
  {
    LM_E(("mongoCppLegacyEntityBatchDelete returned false"));
    ciP->httpStatusCode = SccBadRequest;
    if (orionldState.responseTree == NULL)
      orionldErrorResponseCreate(OrionldBadRequestData, "Database Error", "mongoCppLegacyEntityBatchDelete");
    return false;
  }
  else
    ciP->httpStatusCode = SccOk;

  return true;
}
