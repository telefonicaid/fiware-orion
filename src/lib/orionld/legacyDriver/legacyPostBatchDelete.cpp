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
* Author: Ken Zangelin and Larysse Savanna
*/
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjString, kjObject, ...
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/entitySuccessPush.h"                  // entitySuccessPush
#include "orionld/common/entityErrorPush.h"                    // entityErrorPush
#include "orionld/mongoCppLegacy/mongoCppLegacyEntitiesDelete.h"                      // mongoCppLegacyEntitiesDelete
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityListLookupWithIdTypeCreDate.h"   // mongoCppLegacyEntityListLookupWithIdTypeCreDate
#include "orionld/payloadCheck/PCHECK.h"                       // PCHECK_STRING, PCHECK_URI
#include "orionld/legacyDriver/legacyPostBatchDelete.h"        // Own interface



// ----------------------------------------------------------------------------
//
// legacyPostBatchDelete -
//
// This function receives an array of entity ids as parameter and performs the batch delete operation.
// It will remove a set of entities from the database.
//
bool legacyPostBatchDelete(void)
{
  KjNode* success  = kjArray(orionldState.kjsonP, "success");
  KjNode* errors   = kjArray(orionldState.kjsonP, "errors");

  // Error or not, the Link header should never be present in the reponse
  orionldState.noLinkHeader = true;

  // The response is never JSON-LD
  orionldState.out.contentType = MT_JSON;

  if (orionldState.requestTree->type != KjArray)
  {
    LM_W(("Bad Input (Payload must be a JSON Array)"));
    orionldError(OrionldBadRequestData, "Invalid payload", "Must be a JSON Array", 400);
    return false;
  }

  //
  // Make sure all array items are strings and valid URIs
  //
  for (KjNode* idNodeP = orionldState.requestTree->value.firstChildP; idNodeP != NULL; idNodeP = idNodeP->next)
  {
    PCHECK_STRING(idNodeP, 0, "Invalid payload", "Array items must be JSON Strings", 400);
    PCHECK_URI(idNodeP->value.s, true, 0, "Invalid payload", "Array items must be valid URIs", 400);
  }


  //
  // First get the entities from database to check which exist
  //
  KjNode* dbEntities = mongoCppLegacyEntityListLookupWithIdTypeCreDate(orionldState.requestTree, false);

  //
  // Now loop in the array of entities from database and compare each id with the id from requestTree
  //
  KjNode* reqEntityId = orionldState.requestTree->value.firstChildP;
  while (reqEntityId != NULL)
  {
    KjNode*  next     = reqEntityId->next;
    bool     idExists = false;

    if (dbEntities != NULL)
    {
      for (KjNode* dbEntity = dbEntities->value.firstChildP; dbEntity != NULL; dbEntity = dbEntity->next)
      {
        KjNode* dbEntityId  = kjLookup(dbEntity, "id");  // Coming from DB - '@id' not necessary

        if ((dbEntityId != NULL) && (strcmp(reqEntityId->value.s, dbEntityId->value.s) == 0))
        {
          idExists = true;
          break;  // Found - no need to keep searching.
        }
      }
    }

    if (idExists == false)
    {
      entityErrorPush(errors, reqEntityId->value.s, OrionldResourceNotFound, "Entity not found", NULL, 404);

      // Remove the not found entity from the incoming payload
      kjChildRemove(orionldState.requestTree, reqEntityId);
    }
    else
      entitySuccessPush(success, reqEntityId->value.s);

    reqEntityId = next;
  }


  //
  // Eliminate possibly duplicated entity ids
  //
  KjNode* eidP = orionldState.requestTree->value.firstChildP;
  KjNode* next;

  while (eidP != NULL)
  {
    next = eidP->next;

    //
    // Compare current (eidP) string value with all nextcoming EIDs is the array
    // If match, remove the latter
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
  if ((orionldState.requestTree->value.firstChildP != NULL) && (mongoCppLegacyEntitiesDelete(orionldState.requestTree) == false))
  {
    LM_E(("Database Error (mongoCppLegacyEntitiesDelete returned error)"));
    orionldState.httpStatusCode = 500;
    orionldError(OrionldBadRequestData, "Database Error", "mongoCppLegacyEntitiesDelete", 400);

    return false;
  }

  if (errors->value.firstChildP == NULL)
  {
    orionldState.responseTree   = NULL;
    orionldState.httpStatusCode = 204;
  }
  else
  {
    orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

    kjChildAdd(orionldState.responseTree, success);
    kjChildAdd(orionldState.responseTree, errors);

    orionldState.httpStatusCode  = 207;      // Multi-Status
    orionldState.out.contentType = MT_JSON;  // restReply already sets it to JSON is 207 ...
  }

  return true;
}
