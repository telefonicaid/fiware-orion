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
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjString, kjObject, ...
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "ngsi10/UpdateContextRequest.h"                       // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                      // UpdateContextResponse

#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/db/dbConfiguration.h"                        // dbEntitiesDelete, dbEntityListLookupWithIdTypeCreDate
#include "orionld/payloadCheck/pcheckUri.h"                    // pcheckUri
#include "orionld/serviceRoutines/orionldPostBatchDelete.h"    // Own interface



// ----------------------------------------------------------------------------
//
// orionldPostBatchDelete -
//
// This function receives an array of entity ids as parameter and performs the batch delete operation.
// It will remove a set of entities from the database.
//
bool orionldPostBatchDelete(ConnectionInfo* ciP)
{
  KjNode* success  = kjArray(orionldState.kjsonP, "success");
  KjNode* errors   = kjArray(orionldState.kjsonP, "errors");
  KjNode* errorObj;
  KjNode* nodeP;

  if (orionldState.requestTree->type != KjArray)
  {
    LM_W(("Bad Input (Payload must be a JSON Array)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid payload", "Must be a JSON Array");
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  //
  // Make sure all array items are strings and valid URIs
  //
  for (KjNode* idNodeP = orionldState.requestTree->value.firstChildP; idNodeP != NULL; idNodeP = idNodeP->next)
  {
    char* detail;

    if (idNodeP->type != KjString)
    {
      LM_W(("Bad Input (Invalid payload - Array items must be JSON Strings)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid payload", "Array items must be JSON Strings");
      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }

    if (pcheckUri(idNodeP->value.s, &detail) == false)
    {
      LM_W(("Bad Input (Invalid payload - Array items must be valid URIs)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid payload", "Array items must be valid URIs");  // FIXME: Include 'detail' and name ("id") and its value (idNodeP->value.s)
      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }
  }


  //
  // First get the entities from database to check if they exist
  //
  KjNode* dbEntities = dbEntityListLookupWithIdTypeCreDate(orionldState.requestTree);
  if (dbEntities == NULL)
  {
    LM_E(("mongoCppLegacyEntityListLookupWithIdTypeCreDate returned NULL"));
    orionldState.httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Entities not found", "Entities were not found in database.");
    return false;
  }

  //
  // Now loop in the array of entities from database and compare each id with the id from requestTree
  //
  KjNode* reqEntityId = orionldState.requestTree->value.firstChildP;
  while (reqEntityId != NULL)
  {
    KjNode*  next     = reqEntityId->next;
    bool     idExists = false;

    for (KjNode* dbEntity = dbEntities->value.firstChildP; dbEntity != NULL; dbEntity = dbEntity->next)
    {
      KjNode* dbEntityId  = kjLookup(dbEntity, "id");  // Coming from DB - '@id' not necessary

      if (dbEntityId == NULL)
      {
        // This can't happen ... however, let's make sure the broker never crashes ...
      }
      else if (strcmp(reqEntityId->value.s, dbEntityId->value.s) == 0)
      {
        idExists = true;
        break;  // Found - no need to keep searching.
      }
    }

    if (idExists == false)
    {
      // Entity not found. Reporting error.

      // entityId field
      errorObj = kjObject(orionldState.kjsonP, NULL);
      nodeP    = kjString(orionldState.kjsonP, "entityId", reqEntityId->value.s);
      kjChildAdd(errorObj, nodeP);

      // error field
      nodeP    = kjString(orionldState.kjsonP, "error", "Entity not found in database.");
      kjChildAdd(errorObj, nodeP);
      kjChildAdd(errors, errorObj);

      // Remove id not found from payload
      kjChildRemove(orionldState.requestTree, reqEntityId);
    }
    else
      kjChildAdd(success, reqEntityId);

    reqEntityId = next;
  }


  //
  // Eliminate any copies
  //
  KjNode* eidP = orionldState.requestTree->value.firstChildP;
  KjNode* next;
  while (eidP != NULL)
  {
    next = eidP->next;

    //
    // Compare current (eidP) string value with all nextcoming EIDs is the array
    // If match, remove the nextcoming
    //
    KjNode* copyP = eidP->next;
    KjNode* copyNext;

    while (copyP != NULL)
    {
      copyNext = copyP->next;
      if (strcmp(eidP->value.s, copyP->value.s) == 0)
        kjChildRemove(orionldState.requestTree, copyP);
      copyP = copyNext;
    }

    eidP = next;
  }

  //
  // Call batch delete function
  //
  if (dbEntitiesDelete(orionldState.requestTree) == false)
  {
    LM_E(("dbEntitiesDelete returned false"));
    orionldState.httpStatusCode = SccBadRequest;
    if (orionldState.responseTree == NULL)
      orionldErrorResponseCreate(OrionldBadRequestData, "Database Error", "dbEntitiesDelete");
    return false;
  }
  else
  {
    orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

    kjChildAdd(orionldState.responseTree, success);
    kjChildAdd(orionldState.responseTree, errors);
    orionldState.httpStatusCode = SccOk;
  }

  return true;
}
