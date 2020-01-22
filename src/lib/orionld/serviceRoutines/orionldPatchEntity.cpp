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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove
#include "kalloc/kaStrdup.h"                                     // kaStrdup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/HttpStatusCode.h"                                 // SccNotFound
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext

#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/OrionldConnection.h"                    // orionldState
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // DUPLICATE_CHECK, STRING_CHECK, ...
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldContextValueExpand.h"           // orionldContextValueExpand
#include "orionld/kjTree/kjTreeToContextAttribute.h"             // kjTreeToContextAttribute
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/mongoBackend/mongoAttributeExists.h"           // mongoAttributeExists
#include "orionld/serviceRoutines/orionldPatchEntity.h"          // Own Interface



// ----------------------------------------------------------------------------
//
// attributeCheck -
//
// FIXME - move to separate module - should be used also for:
//   * POST /entities/*/attrs
//   * PATCH /entities/*/attrs
//   * etc
//
bool attributeCheck(ConnectionInfo* ciP, KjNode* attrNodeP, char** titleP, char** detailP)
{
  if (attrNodeP->type != KjObject)
  {
    *titleP  = (char*) "Invalid JSON Type";
    *detailP = (char*) "Attribute must be an object";

    return false;
  }

  KjNode* typeP    = NULL;
  KjNode* valueP   = NULL;
  KjNode* objectP  = NULL;
  int     attrType = 0;   // 1: Property, 2: Relationship, 3: GeoProperty, 4: TemporalProperty

  for (KjNode* nodeP = attrNodeP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (strcmp(nodeP->name, "type") == 0)
    {
      DUPLICATE_CHECK(typeP, "type", nodeP);
      STRING_CHECK(typeP, "type");

      if      (strcmp(typeP->value.s, "Property")         == 0)  attrType = 1;
      else if (strcmp(typeP->value.s, "Relationship")     == 0)  attrType = 2;
      else if (strcmp(typeP->value.s, "GeoProperty")      == 0)  attrType = 3;
      else if (strcmp(typeP->value.s, "TemporalProperty") == 0)  attrType = 4;
      else
      {
        *titleP  = (char*) "Invalid Value of Attribute Type";
        *detailP = typeP->value.s;

        return false;
      }
    }
    else if (strcmp(nodeP->name, "value") == 0)
    {
      DUPLICATE_CHECK(valueP, "value", nodeP);
    }
    else if (strcmp(nodeP->name, "object") == 0)
    {
      DUPLICATE_CHECK(objectP, "object", nodeP);
    }
  }

  if (typeP == NULL)
  {
    *titleP  = (char*) "Mandatory field missing";
    *detailP = (char*) "attribute type";

    return false;
  }

  if (attrType == 2)  // 2 == Relationship
  {
    // Relationships MUST have an "object"
    if (objectP == NULL)
    {
      *titleP  = (char*) "Mandatory field missing";
      *detailP = (char*) "Relationship object";

      return false;
    }
  }
  else
  {
    // Properties MUST have a "value"
    if (valueP == NULL)
    {
      *titleP  = (char*) "Mandatory field missing";
      *detailP = (char*) "Property value";

      return false;
    }
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// orionldPatchEntity -
//
// The input payload in an array of attributes.
// Those attributes that don't exist already in the entity are ignored,
// the rest of attributes replace the old attribute.
//
// Extract from ETSI NGSI-LD spec:
//   For each of the Attributes included in the Fragment, if the target Entity includes a matching one (considering
//   term expansion rules as mandated by clause 5.5.7), then replace it by the one included by the Fragment. If the
//   Attribute includes a datasetId, only an Attribute instance with the same datasetId is replaced.
//   In all other cases, the Attribute shall be ignored.
//
bool orionldPatchEntity(ConnectionInfo* ciP)
{
  char*   entityId          = orionldState.wildcard[0];
  KjNode* currentEntityTree;

  LM_T(LmtServiceRoutine, ("In orionldPatchEntity"));

  // Is the payload empty?
  if (orionldState.requestTree == NULL)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Payload is missing", NULL);
    return false;
  }

  // Is the payload not a JSON object?
  if  (orionldState.requestTree->type != KjObject)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Payload is not a JSON object", kjValueType(orionldState.requestTree->type));
    return false;
  }

  // Is the payload an empty object?
  if  (orionldState.requestTree->value.firstChildP == NULL)
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Payload is an empty JSON object", NULL);
    return false;
  }

  // Get the Entity from the database
  if ((currentEntityTree = dbEntityLookup(entityId)) == NULL)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Entity does not exist", entityId);
    return false;
  }


  //
  // o Make sure the attributes to be patched exist. If not - ignore!
  // o Expand attribute values if the @context says they should be expanded
  //
  KjNode* attrNamesArrayP = kjLookup(currentEntityTree, "attrNames");

  if (attrNamesArrayP == NULL)
    LM_X(1, ("Something has really gone wrong - the node 'attrNames' of the entity '%s' isn't present!", entityId));

  KjNode* attrNodeP = orionldState.requestTree->value.firstChildP;
  KjNode* next;
  while (attrNodeP != NULL)
  {
    char* attrName;

    next = attrNodeP->next;

    //
    // Is the incoming payload correct?
    // 1. All attributes must have a type
    // 2. All attributes must have a value/object
    //
    char* title;
    char* detail;

    if (attributeCheck(ciP, attrNodeP, &title, &detail) == false)
    {
      LM_E(("attributeCheck: %s: %s", title, detail));
      orionldErrorResponseCreate(OrionldBadRequestData, title, detail);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }

    if ((strncmp(attrNodeP->name, "http://", 7) == 0) || (strncmp(attrNodeP->name, "https://", 8) == 0))
      attrName = attrNodeP->name;
    else
    {
      bool valueToBeExpanded = false;

      attrName = orionldContextItemExpand(orionldState.contextP, attrNodeP->name, &valueToBeExpanded, true, NULL);

      if (valueToBeExpanded == true)
        orionldContextValueExpand(attrNodeP);
    }

    if (kjStringValueLookupInArray(attrNamesArrayP, attrName) == NULL)
    {
      //
      // Non-found attributes are ignored - see ETSI NGSi-LD spec, chapter 5.6.2.4:
      //    "In all other cases, the Attribute shall be ignored"
      //
      kjChildRemove(orionldState.requestTree, attrNodeP);
    }

    attrNodeP = next;
  }


  //
  // mongo ...
  // - fill in the UpdateContextRequest from the KjTree
  // - call the mongo routine 'mongoUpdateContext'
  //
  UpdateContextRequest   mongoRequest;
  UpdateContextResponse  mongoResponse;
  ContextElement         ce;
  EntityId*              entityIdP;
  ContextAttribute*      delayedDelete[20];  // If more attributes, then ... :(

  mongoRequest.contextElementVector.push_back(&ce);

  entityIdP     = &mongoRequest.contextElementVector[0]->entityId;
  entityIdP->id = entityId;
  mongoRequest.updateActionType = ActionTypeAppendStrict;

  // Iterate over the object, to get all attributes
  unsigned int attrs = 0;

  for (KjNode* kNodeP = orionldState.requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    if (strcmp(kNodeP->name, "createdAt") == 0)
      continue;

    if (strcmp(kNodeP->name, "modifiedAt") == 0)
      continue;

    KjNode*            attrTypeNodeP = NULL;
    ContextAttribute*  caP           = new ContextAttribute();  // I have tried to use a stack variable "ContextAttribute ca" instead of allocating
                                                                // but I get problems with it. Need a delayed delete for this.
    delayedDelete[attrs]     = caP;
    delayedDelete[attrs + 1] = NULL;
    ++attrs;

    if (attrs >= sizeof(delayedDelete) / sizeof(delayedDelete[0]))
      LM_X(1, ("Need a bigger vector for delayed deletion of attributes"));

    char* detail;
    if (kjTreeToContextAttribute(ciP, kNodeP, caP, &attrTypeNodeP, &detail) == false)
    {
      // kjTreeToContextAttribute calls orionldErrorResponseCreate
      mongoRequest.release();
      LM_E(("kjTreeToContextAttribute failed: %s", detail));
      delete caP;
      return false;
    }

    if (attrTypeNodeP != NULL)
      ce.contextAttributeVector.push_back(caP);
    else
      delete caP;
  }

  //
  // Call mongoBackend - FIXME: call postUpdateContext, not mongoUpdateContext
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
  // Delay-Delete allocated attrs
  //
  for (unsigned int ix = 0; ix < attrs; ix++)
    delete delayedDelete[ix];

  if (ciP->httpStatusCode == SccOk)
    ciP->httpStatusCode = SccNoContent;
  else
  {
    LM_E(("mongoUpdateContext: HTTP Status Code: %d", ciP->httpStatusCode));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Error from Mongo-DB backend");
    return false;
  }

  return true;
}
