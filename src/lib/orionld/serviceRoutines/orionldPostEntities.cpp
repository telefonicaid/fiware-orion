/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

extern "C"
{
#include "kbase/kTime.h"                                         // kTimeGet
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjRender.h"                                      // kjRender - TMP
#include "kjson/kjClone.h"                                       // kjClone
}

#include "common/globals.h"                                      // parse8601Time
#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                  // httpHeaderLocationAdd
#include "orionTypes/OrionValueType.h"                           // orion::ValueType
#include "orionTypes/UpdateActionType.h"                         // ActionType
#include "parse/CompoundValueNode.h"                             // CompoundValueNode
#include "ngsi/ContextAttribute.h"                               // ContextAttribute
#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                        // UpdateContextResponse
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext

#include "orionld/rest/orionldServiceInit.h"                     // orionldHostName, orionldHostNameLen
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/common/performance.h"                          // REQUEST_PERFORMANCE
#include "orionld/payloadCheck/pcheckEntity.h"                   // pcheckEntity
#include "orionld/payloadCheck/pcheckUri.h"                      // pcheckUri
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/kjTree/kjTreeToContextAttribute.h"             // kjTreeToContextAttribute
#include "orionld/mongoBackend/mongoEntityExists.h"              // mongoEntityExists
#include "orionld/serviceRoutines/orionldPostEntities.h"         // Own interface



// -----------------------------------------------------------------------------
//
// pcheckAttribute -
//
// FIXME: Use instead the one from src/lib/payloadCheck/pcheckAttribute.cpp
//
static bool pcheckAttribute(KjNode* attributeP, OrionldProblemDetails* pdP)
{
  return true;
}



// -----------------------------------------------------------------------------
//
// datasetInstances -
//
KjNode* datasetInstances(KjNode* datasets, KjNode* attrV, char* attributeName, double timestamp, OrionldProblemDetails* pdP)
{
  // Loop over all instances and remove all without datasetId and with default datasetId
  KjNode*          next;
  KjNode*          instanceP         = attrV->value.firstChildP;
  KjNode*          defaultInstanceP  = NULL;
  KjNode*          modifiedAt;
  KjNode*          createdAt;
  char*            longName = NULL;

  longName      = orionldContextItemExpand(orionldState.contextP, attributeName, true, NULL);
  attributeName = kaStrdup(&orionldState.kalloc, longName);
  dotForEq(attributeName);

  int instanceIx = 0;
  while (instanceP != NULL)
  {
    if (pcheckAttribute(instanceP, pdP) != true)
      return NULL;  // pdP->status has been set by pcheckAttribute. Also, orionldErrorResponseCreate has been called

    next = instanceP->next;

    KjNode* datasetIdP = kjLookup(instanceP, "datasetId");

    if (datasetIdP != NULL)
    {
      if (datasetIdP->type != KjString)
      {
        orionldErrorResponseCreate(OrionldBadRequestData, "Not a JSON String", "datasetId");
        orionldState.httpStatusCode = SccBadRequest;
        pdP->status = 400;
        return NULL;
      }

      if (pcheckUri(datasetIdP->value.s, &pdP->detail) == false)
      {
        orionldErrorResponseCreate(OrionldBadRequestData, "Not a URI", "datasetId");  // FIXME: Include 'detail' and value (datasetIdP->value.s)
        orionldState.httpStatusCode = SccBadRequest;
        pdP->status = 400;
        return NULL;
      }
    }

    if (datasetIdP == NULL)  // No datasetId
    {
      if (defaultInstanceP != NULL)  // Already found an instance without datasetId ?
      {
        LM_W(("Bad Input (more that one instance without datasetId"));
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid payload data", "more that one instance without datasetId");
        pdP->status = 400;
        return NULL;
      }

      defaultInstanceP = instanceP;

      //
      // Remove the instance from 'attrV', where we only want datasetId attr
      //
      kjChildRemove(attrV, defaultInstanceP);
    }
    else
    {
      // Add createdAt and modifiedAt to the instance
      createdAt  = kjFloat(orionldState.kjsonP, "createdAt",  timestamp);
      modifiedAt = kjFloat(orionldState.kjsonP, "modifiedAt", timestamp);
      kjChildAdd(instanceP, createdAt);
      kjChildAdd(instanceP, modifiedAt);
    }

    instanceP = next;
    ++instanceIx;
  }

  // Create an array for the attribute and put all remaining instances in the array
  KjNode* attrArray = kjArray(orionldState.kjsonP, attributeName);

  attrArray->value.firstChildP = attrV->value.firstChildP;
  attrArray->lastChild         = attrV->lastChild;

  // Then, add the array to 'datasets'
  kjChildAdd(datasets, attrArray);

  //
  // The object returned was part of an array and thus has no name.
  // But, the array had a name, and that's the name we need to give the object before returning it.
  //
  if (defaultInstanceP != NULL)
    defaultInstanceP->name = (char*) attributeName;

  return defaultInstanceP;
}



// -----------------------------------------------------------------------------
//
// pcheckAttributeType - move to payloadCheck library
//
bool pcheckAttributeType(KjNode* attrTypeP, const char* attrName)
{
  if (attrTypeP == NULL)
  {
    LM_W(("Bad Input (attribute without type)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Attribute without type", attrName);
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  if (attrTypeP->type != KjString)
  {
    LM_W(("Bad Input (attribute type must be a JSON string)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Attribute type must be a JSON string", attrName);
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// orionldPostEntities -
//
bool orionldPostEntities(ConnectionInfo* ciP)
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
  // Entity ID
  //
  if (pcheckUri(entityId, &detail) == false)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Entity id", "The id specified cannot be resolved to a URL or URN");  // FIXME: Include 'detail' and name (entityId)
    return false;
  }


  //
  // If the entity already exists, an error should be returned
  //
  if (mongoEntityExists(entityId, orionldState.tenant) == true)
  {
    orionldErrorResponseCreate(OrionldAlreadyExists, "Entity already exists", entityId);
    orionldState.httpStatusCode = SccConflict;
    return false;
  }

  //
  // This function destroys the incoming tree - must save it for TRoE
  //
  KjNode* cloneForTroeP = NULL;
  if (troe)
    cloneForTroeP = kjClone(orionldState.kjsonP, orionldState.requestTree);

  orionldState.entityId = entityId;

  UpdateContextRequest   mongoRequest;
  UpdateContextResponse  mongoResponse;
  ContextElement*        ceP       = new ContextElement();  // FIXME: Any way I can avoid to allocate ?
  EntityId*              entityIdP;

  mongoRequest.contextElementVector.push_back(ceP);
  entityIdP = &mongoRequest.contextElementVector[0]->entityId;
  mongoRequest.updateActionType = ActionTypeAppend;

  entityIdP->id            = entityId;
  entityIdP->isPattern     = "false";
  entityIdP->creDate       = orionldState.requestTime;
  entityIdP->modDate       = orionldState.requestTime;
  entityIdP->isTypePattern = false;
  entityIdP->type          = orionldContextItemExpand(orionldState.contextP, entityType, true, NULL);


  //
  // Attributes
  //
  KjNode*          datasets = kjObject(orionldState.kjsonP, "@datasets");
  KjNode*          kNodeP   = orionldState.requestTree->value.firstChildP;
  KjNode*          next;
  struct timespec  now;
  double           timestamp;

  kTimeGet(&now);
  timestamp = now.tv_sec + ((double) now.tv_nsec / 1000000000);

  while (kNodeP != NULL)
  {
    next = kNodeP->next;

    //
    // If the attribute is an array ( "attr": [] ), then special treatment is necessary
    // One of the instances in the array may be without datasetId, or with the default datasetId ...
    // If so, that instance must be treated as a "normal" attribute.
    // It must be remved from the array and returned as kNodeP;
    if (kNodeP->type == KjArray)
    {
      OrionldProblemDetails pd = { OrionldOk, NULL, NULL, 0 };

      kNodeP = datasetInstances(datasets, kNodeP, kNodeP->name, timestamp, &pd);

      if (kNodeP == NULL)
      {
        if (pd.status != 0)  // Error in datasetInstances
        {
          mongoRequest.release();
          return false;
        }

        kNodeP = next;
        continue;
      }

      //
      // NOTE
      //   kNode != NULL means that datasetInstances has identified an instance that has no datasetId OR
      //   the default datasetId (which has been removed) and the "new" kNodeP is treated as if there was no JSON Array
      //   as RHS for the attribute, just a normal JSON object ...
      //
    }

    KjNode* attrType = kjLookup(kNodeP, "type");

    if ((kNodeP == createdAtP) || (kNodeP == modifiedAtP))
    {
      kNodeP = next;
      continue;
    }

    if (pcheckAttributeType(attrType, kNodeP->name) == false)
    {
      mongoRequest.release();
      return false;
    }

    //
    // If a datasetId member is present, and it's not the default datasetId, then the
    // attribute is removed from the entity and added to the 'datasets' object
    //
    // If the datasetId is the default datasetId, then the field is simply removed
    //
    KjNode* datasetIdP = kjLookup(kNodeP, "datasetId");
    if (datasetIdP != NULL)
    {
      STRING_CHECK(datasetIdP, "datasetId");
      URI_CHECK(datasetIdP, "datasetId");

      kjChildRemove(orionldState.requestTree, kNodeP);
      kjChildAdd(datasets, kNodeP);

      // Add createdAt and modifiedAt to the instance
      KjNode* createdAt  = kjFloat(orionldState.kjsonP, "createdAt",  timestamp);
      KjNode* modifiedAt = kjFloat(orionldState.kjsonP, "modifiedAt", timestamp);
      kjChildAdd(kNodeP, createdAt);
      kjChildAdd(kNodeP, modifiedAt);

      // Change to longName
      char*  longName = orionldContextItemExpand(orionldState.contextP, kNodeP->name, true, NULL);

      longName = kaStrdup(&orionldState.kalloc, longName);
      dotForEq(longName);
      kNodeP->name = longName;

      kNodeP = next;

      continue;
    }

    ContextAttribute* caP            = new ContextAttribute();
    KjNode*           attrTypeNodeP  = NULL;
    char*             detail         = (char*) "none";

    eqForDot(kNodeP->name);
    if (kjTreeToContextAttribute(orionldState.contextP, kNodeP, caP, &attrTypeNodeP, &detail) == false)
    {
      // kjTreeToContextAttribute calls orionldErrorResponseCreate
      LM_E(("kjTreeToContextAttribute failed: %s", detail));
      caP->release();
      delete caP;
      mongoRequest.release();
      return false;
    }

    if (attrTypeNodeP != NULL)
      ceP->contextAttributeVector.push_back(caP);
    else
      delete caP;

    kNodeP = next;
  }

  //
  // Mongo
  //
  if (datasets->value.firstChildP != NULL)  // Not Empty
    orionldState.datasets = datasets;

#ifdef REQUEST_PERFORMANCE
  kTimeGet(&timestamps.dbStart);
#endif

  orionldState.httpStatusCode = mongoUpdateContext(&mongoRequest,
                                                   &mongoResponse,
                                                   orionldState.tenant,
                                                   ciP->servicePathV,
                                                   ciP->uriParam,
                                                   ciP->httpHeaders.xauthToken,
                                                   ciP->httpHeaders.correlator,
                                                   ciP->httpHeaders.ngsiv2AttrsFormat,
                                                   ciP->apiVersion,
                                                   NGSIV2_NO_FLAVOUR);

#ifdef REQUEST_PERFORMANCE
  kTimeGet(&timestamps.dbEnd);
#endif
  mongoRequest.release();
  mongoResponse.release();

  if (orionldState.httpStatusCode != SccOk)
  {
    LM_E(("mongoUpdateContext: HTTP Status Code: %d", orionldState.httpStatusCode));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Error from Mongo-DB backend");
    return false;
  }

  orionldState.httpStatusCode = SccCreated;
  orionldState.entityCreated  = true;

  httpHeaderLocationAdd(ciP, "/ngsi-ld/v1/entities/", entityId);

  if (cloneForTroeP != NULL)
    orionldState.requestTree = cloneForTroeP;

  return true;
}
