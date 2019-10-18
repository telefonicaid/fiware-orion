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
* Author: Gabriel Quaresma and Ken Zangelin
*/
#include "logMsg/logMsg.h"                                                 // LM_*
#include "logMsg/traceLevels.h"                                            // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                                  // KjNode
#include "kjson/kjBuilder.h"                                               // kjString, kjObject, ...
#include "kjson/kjRender.h"                                                // kjRender
}

#include "common/globals.h"                                                // parse8601Time
#include "rest/ConnectionInfo.h"                                           // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                            // httpHeaderLocationAdd
#include "orionTypes/OrionValueType.h"                                     // orion::ValueType
#include "orionTypes/UpdateActionType.h"                                   // ActionType
#include "parse/CompoundValueNode.h"                                       // CompoundValueNode
#include "ngsi/ContextAttribute.h"                                         // ContextAttribute
#include "ngsi10/UpdateContextRequest.h"                                   // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                                  // UpdateContextResponse
#include "mongoBackend/mongoEntityExists.h"                                // mongoEntityExists
#include "mongoBackend/mongoUpdateContext.h"                               // mongoUpdateContext
#include "rest/uriParamNames.h"                                            // URI_PARAM_PAGINATION_OFFSET, URI_PARAM_PAGINATION_LIMIT

#include "orionld/rest/orionldServiceInit.h"                               // orionldHostName, orionldHostNameLen
#include "orionld/common/orionldErrorResponse.h"                           // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                                       // SCOMPAREx
#include "orionld/common/CHECK.h"                                          // CHECK
#include "orionld/common/urlCheck.h"                                       // urlCheck
#include "orionld/common/urnCheck.h"                                       // urnCheck
#include "orionld/common/orionldState.h"                                   // orionldState
#include "orionld/common/orionldAttributeTreat.h"                          // orionldAttributeTreat
#include "orionld/context/orionldCoreContext.h"                            // orionldDefaultUrl, orionldCoreContext
#include "orionld/context/orionldContextAdd.h"                             // Add a context to the context list
#include "orionld/context/orionldContextLookup.h"                          // orionldContextLookup
#include "orionld/context/orionldContextItemLookup.h"                      // orionldContextItemLookup
#include "orionld/context/orionldContextList.h"                            // orionldContextHead, orionldContextTail
#include "orionld/context/orionldContextListInsert.h"                      // orionldContextListInsert
#include "orionld/context/orionldContextPresent.h"                         // orionldContextPresent
#include "orionld/context/orionldUserContextKeyValuesCheck.h"              // orionldUserContextKeyValuesCheck
#include "orionld/context/orionldUriExpand.h"                              // orionldUriExpand
#include "orionld/kjTree/kjStringValueLookupInArray.h"                     // kjStringValueLookupInArray
#include "orionld/serviceRoutines/orionldPostBatchUpsert.h"                // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPartialUpdateResponseCreateBatch -
//
void orionldPartialUpdateResponseCreateBatch(ConnectionInfo* ciP)
{
  //
  // Rob the incoming Request Tree - performance to be won!
  //
  orionldState.responseTree = orionldState.requestTree;
  orionldState.requestTree  = NULL;

  //
  // For all attrs in orionldState.responseTree, remove those that are found in orionldState.errorAttributeArrayP.
  // Remember, the format of orionldState.errorAttributeArrayP is:
  //
  //   |attrName|attrName|[attrName|]*
  //

  KjNode* attrNodeP = orionldState.responseTree->value.firstChildP;

  while (attrNodeP != NULL)
  {
    char*   match;
    KjNode* next   = attrNodeP->next;
    bool    moved  = false;

    if ((match = strstr(orionldState.errorAttributeArrayP, attrNodeP->name)) != NULL)
    {
      if ((match[-1] == '|') && (match[strlen(attrNodeP->name)] == '|'))
      {
        kjChildRemove(orionldState.responseTree, attrNodeP);
        attrNodeP = next;
        moved = true;
      }
    }

    if (moved == false)
      attrNodeP = attrNodeP->next;
  }
}



// -----------------------------------------------------------------------------
//
// kjTreeToContextElementBatch -
//
// NOTE: "id" and "type" of the entity must be removed from the tree before this function is called
//
bool kjTreeToContextElementAttributes
(
  ConnectionInfo*  ciP,
  KjNode*          entityNodeP,
  KjNode*          createdAtP,
  KjNode*          modifiedAtP,
  ContextElement*  ceP,
  char**           detailP
)
{
  // Iterate over the items of the entity
  for (KjNode* itemP = entityNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (itemP == createdAtP)
      continue;
    if (itemP == modifiedAtP)
      continue;

    // No key-values in batch ops
    if (itemP->type != KjObject)
    {
      *detailP = (char*) "attribute must be a JSON object";
      return false;
    }

    KjNode*            attrTypeNodeP  = NULL;
    ContextAttribute*  caP            = new ContextAttribute();

    // orionldAttributeTreat treats the attribute, including expanding the attribute name and values, if applicable
    if (orionldAttributeTreat(ciP, itemP, caP, &attrTypeNodeP, detailP) == false)
    {
      LM_E(("orionldAttributeTreat failed"));
      delete caP;
      return false;
    }

    ceP->contextAttributeVector.push_back(caP);
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// entitySuccessPush -
//
static void entitySuccessPush(KjNode* successArrayP, const char* entityId)
{
  KjNode* eIdP = kjString(orionldState.kjsonP, "id", entityId);

  kjChildAdd(successArrayP, eIdP);
}



// ----------------------------------------------------------------------------
//
// entityErrorPush -
//
// The array "errors" in BatchOperationResult is an array of BatchEntityError.
// BatchEntityError contains a string (the entity id) and an instance of ProblemDetails.
//
// ProblemDetails is described in https://www.etsi.org/deliver/etsi_gs/CIM/001_099/009/01.01.01_60/gs_CIM009v010101p.pdf
// and contains:
//
// * type      (string) A URI reference that identifies the problem type
// * title     (string) A short, human-readable summary of the problem
// * detail    (string) A human-readable explanation specific to this occurrence of the problem
// * status    (number) The HTTP status code
// * instance  (string) A URI reference that identifies the specific occurrence of the problem
//
// Of these five items, only "type" seems to be mandatory.
//
// This implementation will treat "type", "title", and "status" as MANDATORY, and "detail" as OPTIONAL
//
static void entityErrorPush(KjNode* errorsArrayP, const char* entityId, OrionldResponseErrorType type, const char* title, const char* detail, int status)
{
  KjNode* objP            = kjObject(orionldState.kjsonP, NULL);
  KjNode* eIdP            = kjString(orionldState.kjsonP, "entityId", entityId);
  KjNode* problemDetailsP = kjObject(orionldState.kjsonP, "error");
  KjNode* typeP           = kjString(orionldState.kjsonP, "type", orionldErrorTypeToString(type));
  KjNode* titleP          = kjString(orionldState.kjsonP, "title", title);
  KjNode* statusP         = kjInteger(orionldState.kjsonP, "status", status);

  kjChildAdd(problemDetailsP, typeP);
  kjChildAdd(problemDetailsP, titleP);

  if (detail != NULL)
  {
    KjNode* detailP = kjString(orionldState.kjsonP, "detail", detail);
    kjChildAdd(problemDetailsP, detailP);
  }

  kjChildAdd(problemDetailsP, statusP);

  kjChildAdd(objP, eIdP);
  kjChildAdd(objP, problemDetailsP);

  kjChildAdd(errorsArrayP, objP);
}



// ----------------------------------------------------------------------------
//
// orionldPostEntityOperationsUpsert -
//
// POST /ngsu-ld/v1/entityOperations/upsert
//
// From the spec:
//   This operation allows creating a batch of NGSI-LD Entities, updating each of them if they already exist.
//
//   An optional flag indicating the update mode (only applies in case the Entity already exists):
//     - ?options=replace  (default)
//     - ?options=update
//
//   Replace:  All the existing Entity content shall be replaced  - like PUT
//   Update:   Existing Entity content shall be updated           - like PATCH
//
//
bool orionldPostBatchUpsert(ConnectionInfo* ciP)
{
  //
  // Prerequisites 1: payload must be an array, and it cannot be empty
  //
  ARRAY_CHECK(orionldState.requestTree,       "toplevel");
  EMPTY_ARRAY_CHECK(orionldState.requestTree, "toplevel");

  if ((orionldState.uriParamOptions.update == true) && (orionldState.uriParamOptions.replace == true))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "URI Param Error", "options: both /update/ and /replace/ present");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (orionldState.uriParamOptions.update == false)
  {
    orionldErrorResponseCreate(OrionldOperationNotSupported, "Not Implemented", "Batch Upsert without URI param update");
    ciP->httpStatusCode = SccNotImplemented;
    return false;
  }

  //
  // Here we treat the "?options=update" mode
  //

  UpdateContextRequest   mongoRequest;
  UpdateContextResponse  mongoResponse;
  KjNode*                createdAtP       = NULL;
  KjNode*                modifiedAtP      = NULL;
  KjNode*                successArrayP    = kjArray(orionldState.kjsonP, "success");
  KjNode*                errorsArrayP     = kjArray(orionldState.kjsonP, "errors");
  char*                  detail;

  ciP->httpStatusCode = SccOk;

  mongoRequest.updateActionType = ActionTypeAppend;

  for (KjNode* entityNodeP = orionldState.requestTree->value.firstChildP; entityNodeP != NULL; entityNodeP = entityNodeP->next)
  {
    OBJECT_CHECK(entityNodeP, kjValueType(entityNodeP->type));

    //
    // First, extract Entity::id and Entity::type
    //
    // As we will remove items from the tree, we need to save the 'next-pointer' a priori
    // If not, after removing an item, its next pointer point to NULL and the for-loop (if used) is ended
    //
    KjNode*   itemP           = entityNodeP->value.firstChildP;
    KjNode*   entityIdNodeP   = NULL;
    KjNode*   entityTypeNodeP = NULL;
    bool      duplicatedType  = false;
    bool      duplicatedId    = false;

    //
    // We only check for duplicated entries in this loop.
    // The rest is taken care of after we've looked up entity::id
    //
    while (itemP != NULL)
    {
      if (SCOMPARE3(itemP->name, 'i', 'd', 0))
      {
        if (entityIdNodeP != NULL)
          duplicatedId = true;
        else
          entityIdNodeP = itemP;

        itemP = itemP->next;  // Point to the next item BEFORE the current one is removed
        kjChildRemove(entityNodeP, entityIdNodeP);
      }
      else if (SCOMPARE5(itemP->name, 't', 'y', 'p', 'e', 0))
      {
        if (entityTypeNodeP != NULL)  // Duplicated 'type' in payload?
          duplicatedType = true;
        else
          entityTypeNodeP = itemP;

        itemP = itemP->next;  // Point to the next item BEFORE the current one is removed
        kjChildRemove(entityNodeP, entityTypeNodeP);
      }
      else
        itemP = itemP->next;
    }


    // Entity ID is mandatory
    if (entityIdNodeP == NULL)
    {
      LM_W(("Bad Input (mandatory field missing: entity::id)"));
      entityErrorPush(errorsArrayP, "no entity::id", OrionldBadRequestData, "mandatory field missing", "entity::id", 400);
      continue;
    }

    // Entity ID must be a string
    if (entityIdNodeP->type != KjString)
    {
      LM_W(("Bad Input (entity::id not a string)"));
      entityErrorPush(errorsArrayP, "invalid entity::id", OrionldBadRequestData, "field with invalid type", "entity::id", 400);
      continue;
    }

    // Entity ID must be a valid URI
    if (!urlCheck(entityIdNodeP->value.s, &detail) && !urnCheck(entityIdNodeP->value.s, &detail))
    {
      LM_W(("Bad Input (entity::id is a string but not a valid URI)"));
      entityErrorPush(errorsArrayP, entityIdNodeP->value.s, OrionldBadRequestData, "Not a URI", entityIdNodeP->value.s, 400);
      continue;
    }

    // Entity ID must not be duplicated
    if (duplicatedId == true)
    {
      LM_W(("Bad Input (Duplicated entity::id)"));
      entityErrorPush(errorsArrayP, entityIdNodeP->value.s, OrionldBadRequestData, "Duplicated field", "entity::id", 400);
      continue;
    }


    // Entity TYPE is mandatory
    if (entityTypeNodeP == NULL)
    {
      LM_W(("Bad Input (mandatory field missing: entity::type)"));
      entityErrorPush(errorsArrayP, entityIdNodeP->value.s, OrionldBadRequestData, "mandatory field missing", "entity::type", 400);
      continue;
    }

    // Entity TYPE must not be duplicated
    if (duplicatedType == true)
    {
      LM_W(("Bad Input (Duplicated entity::type)"));
      entityErrorPush(errorsArrayP, entityIdNodeP->value.s, OrionldBadRequestData, "Duplicated field", "entity::type", 400);
      continue;
    }

    // Entity TYPE must be a string
    if (entityTypeNodeP->type != KjString)
    {
      LM_W(("Bad Input (entity::type not a string)"));
      entityErrorPush(errorsArrayP, entityIdNodeP->value.s, OrionldBadRequestData, "field with invalid type", "entity::type", 400);
      continue;
    }


    //
    // Both Entity::id and Entity::type are OK
    //
    char*            entityId    = entityIdNodeP->value.s;
    char*            entityType  = entityTypeNodeP->value.s;
    ContextElement*  ceP         = new ContextElement();  // FIXME: Any way I can avoid to allocate ?
    EntityId*        entityIdP   = &ceP->entityId;
    char             typeExpanded[256];

    mongoRequest.updateActionType = ActionTypeAppendStrict;
    entityIdP->id                 = entityId;

    if (orionldUriExpand(orionldState.contextP, entityType, typeExpanded, sizeof(typeExpanded), NULL, &detail) == false)
    {
      LM_E(("orionldUriExpand failed: %s", detail));
      entityErrorPush(errorsArrayP, entityIdNodeP->value.s, OrionldBadRequestData, "unable to expand entity::type", detail, 400);
      delete ceP;
      continue;
    }

    entityIdP->type      = typeExpanded;
    entityIdP->isPattern = "false";

#if 0
    entityIdP->creDate   = getCurrentTime();  // FIXME: Only if newly created. I think mongoBackend takes care of this - so, outdeffed
    entityIdP->modDate   = getCurrentTime();
#endif

    if (kjTreeToContextElementAttributes(ciP, entityNodeP, createdAtP, modifiedAtP, ceP, &detail) == false)
    {
      LM_W(("kjTreeToContextElementAttributes flags error '%s' for entity '%s'", detail, entityId));
      entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "", detail, 400);
      delete ceP;
      continue;
    }

    mongoRequest.contextElementVector.push_back(ceP);

    orionldState.payloadIdNode   = NULL;
    orionldState.payloadTypeNode = NULL;
  }


  //
  // Call mongoBackend
  //
  ciP->httpStatusCode = mongoUpdateContext(&mongoRequest,
                                           &mongoResponse,
                                           orionldState.tenant,
                                           ciP->servicePathV,
                                           ciP->uriParam,
                                           ciP->httpHeaders.xauthToken,
                                           ciP->httpHeaders.correlator,
                                           ciP->httpHeaders.ngsiv2AttrsFormat,
                                           ciP->apiVersion,
                                           NGSIV2_NO_FLAVOUR);

  //
  // Now check orionldState.errorAttributeArray to see whether any attribute failed to be updated
  //
  // bool partialUpdate = (orionldState.errorAttributeArrayP[0] == 0)? false : true;
  // bool retValue      = true;
  //

  if (ciP->httpStatusCode == SccOk)
  {
    orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

    for (unsigned int ix = 0; ix < mongoResponse.contextElementResponseVector.vec.size(); ix++)
    {
      const char* entityId = mongoResponse.contextElementResponseVector.vec[ix]->contextElement.entityId.id.c_str();

      if (mongoResponse.contextElementResponseVector.vec[ix]->statusCode.code == SccOk)
        entitySuccessPush(successArrayP, entityId);
      else
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "", mongoResponse.contextElementResponseVector.vec[ix]->statusCode.reasonPhrase.c_str(), 400);
    }

    for (unsigned int ix = 0; ix < mongoRequest.contextElementVector.vec.size(); ix++)
    {
      const char* entityId = mongoRequest.contextElementVector.vec[ix]->entityId.id.c_str();

      if (kjStringValueLookupInArray(successArrayP, entityId) == NULL)
        entitySuccessPush(successArrayP, entityId);
    }

    //
    // Add the success/error arrays to the response-tree
    //
    kjChildAdd(orionldState.responseTree, successArrayP);
    kjChildAdd(orionldState.responseTree, errorsArrayP);

    ciP->httpStatusCode = SccOk;
  }

  mongoRequest.release();
  mongoResponse.release();

  if (ciP->httpStatusCode != SccOk)
  {
    LM_E(("mongoUpdateContext flagged an error"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Database Error");
    ciP->httpStatusCode = SccReceiverInternalError;
    return false;
  }

  return true;

  //
  // TO-DO (Operation with "new way")
  // if (mongoCppLegacyEntityOperationsUpsert(orionldState.requestTree) == false)
  // {
  //   LM_E(("mongoCppLegacyEntityOperationsUpsert"));
  //   ciP->httpStatusCode = SccBadRequest;
  //   orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Error from Mongo-DB backend");
  //   return false;
  // }
  //
}
