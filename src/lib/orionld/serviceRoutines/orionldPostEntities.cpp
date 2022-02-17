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
#include <unistd.h>                                              // NULL

#include <string>                                                // std::string
#include <vector>                                                // std::vector

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
#include "rest/httpHeaderAdd.h"                                  // httpHeaderLocationAdd
#include "orionTypes/OrionValueType.h"                           // orion::ValueType
#include "orionTypes/UpdateActionType.h"                         // ActionType
#include "parse/CompoundValueNode.h"                             // CompoundValueNode
#include "ngsi/ContextAttribute.h"                               // ContextAttribute
#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                        // UpdateContextResponse
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext

#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType, NoAttributeType
#include "orionld/rest/orionldServiceInit.h"                     // orionldHostName, orionldHostNameLen
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/common/performance.h"                          // PERFORMANCE
#include "orionld/common/CHECK.h"                                // CHECK_*
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_*
#include "orionld/payloadCheck/pCheckEntityId.h"                 // pCheckEntityId
#include "orionld/payloadCheck/pCheckEntityType.h"               // pCheckEntityType
#include "orionld/payloadCheck/pCheckEntity.h"                   // pCheckEntity
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/kjTree/kjTreeToContextAttribute.h"             // kjTreeToContextAttribute
#include "orionld/mongoBackend/mongoEntityExists.h"              // mongoEntityExists
#include "orionld/serviceRoutines/orionldPostEntities.h"         // Own interface



// -----------------------------------------------------------------------------
//
// datasetInstances -
//
static KjNode* datasetInstances(KjNode* datasets, KjNode* attrV, char* attributeName)
{
  char* longName;

  longName      = orionldAttributeExpand(orionldState.contextP, attributeName, true, NULL);
  attributeName = kaStrdup(&orionldState.kalloc, longName);
  dotForEq(attributeName);

  //
  // Loop over all instances and remove all lacking datasetId (or with default datasetId - which is DEPRECATED !!!)
  //
  KjNode*  defaultInstanceP  = NULL;
  KjNode*  instanceP         = attrV->value.firstChildP;
  KjNode*  next;

  while (instanceP != NULL)
  {
    next = instanceP->next;

    KjNode* datasetIdP = kjLookup(instanceP, "datasetId");

    if (datasetIdP != NULL)
    {
      if (datasetIdP->type != KjString)
      {
        orionldError(OrionldBadRequestData, "Not a JSON String", "datasetId", 400);
        return NULL;
      }

      if (pCheckUri(datasetIdP->value.s, true) == false)
      {
        orionldError(OrionldBadRequestData, "Not a URI", "datasetId", 400);
        return NULL;
      }
    }

    if (datasetIdP == NULL)  // No datasetId
    {
      if (defaultInstanceP != NULL)  // Already found an instance without datasetId ?
      {
        orionldError(OrionldBadRequestData, "Invalid payload data", "more that one instance without datasetId", 400);
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
      KjNode*  createdAt   = kjFloat(orionldState.kjsonP, "createdAt",  orionldState.requestTime);
      KjNode*  modifiedAt  = kjFloat(orionldState.kjsonP, "modifiedAt", orionldState.requestTime);

      kjChildAdd(instanceP, createdAt);
      kjChildAdd(instanceP, modifiedAt);
    }

    instanceP = next;
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



// ----------------------------------------------------------------------------
//
// orionldPostEntities -
//
bool orionldPostEntities(void)
{
  char*    entityId;
  char*    entityType;

  PCHECK_OBJECT(orionldState.requestTree, 0, NULL, "To create an Entity, a JSON OBJECT describing the entity must be provided", 400);

  if (pCheckEntityId(orionldState.payloadIdNode,     true, &entityId)   == false)   return false;
  if (pCheckEntityType(orionldState.payloadTypeNode, true, &entityType) == false)   return false;


  //
  // If the entity already exists, an error is returned
  //
  if (mongoEntityExists(entityId, orionldState.tenantP) == true)
  {
    orionldError(OrionldAlreadyExists, "Entity already exists", entityId, SccConflict);
    return false;
  }

  //
  // payloadParseAndExtractSpecialFields() from orionldMhdConnectionTreat() decouples the entity id and type
  // from the payload body, so, the entity type is not expanded by pCheckEntity()
  // It's done here instead
  //
  entityType = orionldContextItemExpand(orionldState.contextP, entityType, true, NULL);  // entity::type removed from payload body - needs expansion

  //
  // Check and fix the incoming payload (entity)
  //
  if (pCheckEntity(orionldState.requestTree, false) == false)
    return false;

  //
  // The function 'kjTreeToContextAttribute' destroys the incoming tree - must save it for TRoE
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
  entityIdP->type          = entityType;


  //
  // Attributes
  //
  KjNode*          datasets = kjObject(orionldState.kjsonP, "@datasets");
  KjNode*          kNodeP   = orionldState.requestTree->value.firstChildP;
  KjNode*          next;

  while (kNodeP != NULL)
  {
    next = kNodeP->next;

    //
    // If the attribute is an array ( "attr": [] ), then special treatment is necessary
    // One of the instances in the array may be without datasetId, or with the default datasetId ...
    // If so, that instance must be treated as a "normal" attribute.
    // It must be removed from the array and returned as kNodeP;
    //
    if (kNodeP->type == KjArray)
    {
      kNodeP = datasetInstances(datasets, kNodeP, kNodeP->name);

      if (kNodeP == NULL)
      {
        if (orionldState.pd.status != 0)  // Error in datasetInstances
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
      URI_CHECK(datasetIdP->value.s, "datasetId", true);

      kjChildRemove(orionldState.requestTree, kNodeP);
      kjChildAdd(datasets, kNodeP);

      // Add createdAt and modifiedAt to the instance
      KjNode* createdAt  = kjFloat(orionldState.kjsonP, "createdAt",  orionldState.requestTime);
      KjNode* modifiedAt = kjFloat(orionldState.kjsonP, "modifiedAt", orionldState.requestTime);
      kjChildAdd(kNodeP, createdAt);
      kjChildAdd(kNodeP, modifiedAt);

      // Change to longName
      char*  longName = orionldAttributeExpand(orionldState.contextP, kNodeP->name, true, NULL);

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

  PERFORMANCE(dbStart);

  std::vector<std::string> servicePathV;
  servicePathV.push_back("/");

  orionldState.httpStatusCode = mongoUpdateContext(&mongoRequest,
                                                   &mongoResponse,
                                                   orionldState.tenantP,
                                                   servicePathV,
                                                   orionldState.xAuthToken,
                                                   orionldState.correlator,
                                                   orionldState.attrsFormat,
                                                   orionldState.apiVersion,
                                                   NGSIV2_NO_FLAVOUR);

  PERFORMANCE(dbEnd);

  mongoRequest.release();
  mongoResponse.release();

  if (orionldState.httpStatusCode != 200)
  {
    LM_E(("mongoUpdateContext: HTTP Status Code: %d", orionldState.httpStatusCode));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Error from Mongo-DB backend");
    return false;
  }
  else if ((mongoResponse.oe.code != 200) && (mongoResponse.oe.code != 0))
  {
    LM_E(("mongoUpdateContext: mongo responds with error %d: '%s'", mongoResponse.oe.code, mongoResponse.oe.details));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Error from Mongo-DB backend");
    return false;
  }

  orionldState.httpStatusCode = 201;

  httpHeaderLocationAdd("/ngsi-ld/v1/entities/", entityId);

  if (cloneForTroeP != NULL)
    orionldState.requestTree = cloneForTroeP;

  return true;
}
