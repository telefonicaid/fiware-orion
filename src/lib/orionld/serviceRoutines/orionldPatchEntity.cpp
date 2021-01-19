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
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove
#include "kjson/kjRender.h"                                      // kjRender
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/HttpStatusCode.h"                                 // SccNotFound
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext

#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // DUPLICATE_CHECK, STRING_CHECK, ...
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/common/attributeUpdated.h"                     // attributeUpdated
#include "orionld/common/attributeNotUpdated.h"                  // attributeNotUpdated
#include "orionld/payloadCheck/pcheckUri.h"                      // pcheckUri
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
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
      *detailP = (char*) "Mandatory field missing: Relationship object";

      return false;
    }
  }
  else
  {
    // Properties MUST have a "value"
    if (valueP == NULL)
    {
      *titleP  = (char*) "Mandatory field missing";
      *detailP = (char*) "Mandatory field missing: Property value";

      return false;
    }
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// orionldPatchEntity -
//
// The input payload is a collection of attributes.
// Those attributes that don't exist already in the entity are not added, but reported in the response payload data as "notUpdated".
// The rest of attributes replace the old attribute.
//
// Extract from ETSI NGSI-LD spec:
//   For each of the Attributes included in the Fragment, if the target Entity includes a matching one (considering
//   term expansion rules as mandated by clause 5.5.7), then replace it by the one included by the Fragment. If the
//   Attribute includes a datasetId, only an Attribute instance with the same datasetId is replaced.
//   In all other cases, the Attribute shall be ignored.
//
bool orionldPatchEntity(ConnectionInfo* ciP)
{
  char* entityId   = orionldState.wildcard[0];
  char* detail;

  LM_TMP(("PATCH: entity id: '%s'", entityId));
  // 1. Is the Entity ID in the URL a valid URI?
  if (pcheckUri(entityId, &detail) == false)
  {
    orionldState.httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Entity ID must be a valid URI", entityId);  // FIXME: Include 'detail' and name (entityId)
    return false;
  }

  // 2. Is the payload not a JSON object?
  OBJECT_CHECK(orionldState.requestTree, kjValueType(orionldState.requestTree->type));

  char buf[1024];
  kjRender(orionldState.kjsonP, orionldState.requestTree, buf, sizeof(buf));
  LM_TMP(("PATCH: incoming payload body: %s", buf));

  // 3. Get the entity from mongo
  KjNode* dbEntityP;
  if ((dbEntityP = dbEntityLookup(entityId)) == NULL)
  {
    orionldState.httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Entity does not exist", entityId);
    return false;
  }

  kjRender(orionldState.kjsonP, dbEntityP, buf, sizeof(buf));
  LM_TMP(("PATCH: entity in DB: %s", buf));

  // 3. Get the Entity Type, needed later in the call to the constructor of ContextElement
  KjNode* idNodeP = kjLookup(dbEntityP, "_id");

  if (idNodeP == NULL)
  {
    orionldState.httpStatusCode = SccReceiverInternalError;
    orionldErrorResponseCreate(OrionldInternalError, "Corrupt Database", "'_id' field of entity from DB not found");
    return false;
  }

  KjNode*     entityTypeNodeP = kjLookup(idNodeP, "type");
  const char* entityType      = (entityTypeNodeP != NULL)? entityTypeNodeP->value.s : NULL;

  if (entityTypeNodeP == NULL)
  {
    orionldState.httpStatusCode = SccReceiverInternalError;
    orionldErrorResponseCreate(OrionldInternalError, "Corrupt Database", "'_id::type' field of entity from DB not found");
    return false;
  }

  // 4. Get the 'attrNames' array from the mongo entity - to see whether an attribute already existed or not
  KjNode* inDbAttrNamesP = kjLookup(dbEntityP, "attrNames");
  if (inDbAttrNamesP == NULL)
  {
    orionldState.httpStatusCode = SccReceiverInternalError;
    orionldErrorResponseCreate(OrionldInternalError, "Corrupt Database", "'attrNames' field of entity from DB not found");
    return false;
  }

  // 4. Also, get the "attrs" object for insertion of modified attributes
  KjNode* inDbAttrsP = kjLookup(dbEntityP, "attrs");
  if (inDbAttrsP == NULL)
  {
    orionldState.httpStatusCode = SccReceiverInternalError;
    orionldErrorResponseCreate(OrionldInternalError, "Corrupt Database", "'attrs' field of entity from DB not found");
    return false;
  }
  kjRender(orionldState.kjsonP, inDbAttrsP, buf, sizeof(buf));
  LM_TMP(("PATCH: inDbAttrsP: %s", buf));

  //
  // 5. Loop over the incoming payload data
  //    Those attrs that don't exist in the DB (dbEntityP) are discarded and added to the 'notUpdated' array
  //    Those that do exist in dbEntityP are removed from dbEntityP and replaced with the corresponding attribute from the incoming payload data.
  //    (finally the modified dbEntityP will REPLACE what is currently in the database)
  //
  KjNode* newAttrP     = orionldState.requestTree->value.firstChildP;
  KjNode* next;
  KjNode* updatedP     = kjArray(orionldState.kjsonP, "updated");
  KjNode* notUpdatedP  = kjArray(orionldState.kjsonP, "notUpdated");
  int     newAttrs     = 0;

  while (newAttrP != NULL)
  {
    KjNode*  dbAttrP;
    char*    title;
    char*    detail;

    LM_TMP(("PATCH: attribute name: '%s'", newAttrP->name));
    next = newAttrP->next;

    if ((strcmp(newAttrP->name, "location")         != 0) &&
        (strcmp(newAttrP->name, "observationSpace") != 0) &&
        (strcmp(newAttrP->name, "operationSpace")   != 0) &&
        (strcmp(newAttrP->name, "observedAt")       != 0))
    {
      newAttrP->name = orionldContextItemExpand(orionldState.contextP, newAttrP->name, true, NULL);
    }
    LM_TMP(("PATCH: attribute name expanded: '%s'", newAttrP->name));

    // Is the attribute in the incoming payload a valid attribute?
    if (attributeCheck(ciP, newAttrP, &title, &detail) == false)
    {
      LM_E(("attributeCheck: %s: %s", title, detail));
      attributeNotUpdated(notUpdatedP, newAttrP->name, detail);
      newAttrP = next;
      continue;
    }

    char* eqName = kaStrdup(&orionldState.kalloc, newAttrP->name);
    dotForEq(eqName);
    LM_TMP(("PATCH: looking up the attribute '%s' in the db attrs array (where '.' is replaced with '=')", eqName));
    dbAttrP = kjLookup(inDbAttrsP, eqName);
    if (dbAttrP == NULL)  // Doesn't already exist - must be discarded
    {
      LM_TMP(("PATCH: attribute '%s' doesn't exist - it's discarded", eqName));
      attributeNotUpdated(notUpdatedP, newAttrP->name, "attribute doesn't exist");
      newAttrP = next;
      continue;
    }
    LM_TMP(("PATCH: attribute '%s' exists and will be modified (real attr name: '%s')", eqName, newAttrP->name));

    // Steal createdAt from dbAttrP?

    // Remove the attribute to be updated (from dbEntityP::inDbAttrsP) and insert the attribute from the payload data
    kjChildRemove(inDbAttrsP, dbAttrP);
    kjChildAdd(inDbAttrsP, newAttrP);
    attributeUpdated(updatedP, newAttrP->name);

    ++newAttrs;
    newAttrP = next;
  }


  if (newAttrs > 0)
  {
    // 6. Convert the resulting tree (dbEntityP) to a ContextElement
    UpdateContextRequest ucRequest;
    ContextElement*      ceP = new ContextElement(entityId, entityType, "false");

    ucRequest.contextElementVector.push_back(ceP);

    for (KjNode* attrP = inDbAttrsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
    {
      ContextAttribute* caP = new ContextAttribute();

      eqForDot(attrP->name);
      LM_TMP(("PATCH: converting attribute '%s' to a ContextAttribute", attrP->name));

      if (kjTreeToContextAttribute(orionldState.contextP, attrP, caP, NULL, &detail) == false)
      {
        LM_E(("kjTreeToContextAttribute: %s", detail));
        attributeNotUpdated(notUpdatedP, attrP->name, "Error");
        delete caP;
      }
      else
        ceP->contextAttributeVector.push_back(caP);
    }
    ucRequest.updateActionType = ActionTypeReplace;


    // 8. Call mongoBackend to do the REPLACE of the entity
    UpdateContextResponse  ucResponse;

    orionldState.httpStatusCode = mongoUpdateContext(&ucRequest,
                                                     &ucResponse,
                                                     orionldState.tenant,
                                                     ciP->servicePathV,
                                                     ciP->uriParam,
                                                     ciP->httpHeaders.xauthToken,
                                                     ciP->httpHeaders.correlator,
                                                     ciP->httpHeaders.ngsiv2AttrsFormat,
                                                     ciP->apiVersion,
                                                     NGSIV2_NO_FLAVOUR);

    ucRequest.release();
  }

  // 9. Postprocess output from mongoBackend
  if (orionldState.httpStatusCode == SccOk)
  {
    //
    // 204 or 207?
    //
    // 204 if all went ok (== empty list of 'not updated')
    // 207 if something went wrong (== non-empty list of 'not updated')
    //
    // If 207 - prepare the response payload data
    //
    if (notUpdatedP->value.firstChildP != NULL)  // non-empty list of 'not updated'
    {
      orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

      kjChildAdd(orionldState.responseTree, updatedP);
      kjChildAdd(orionldState.responseTree, notUpdatedP);

      orionldState.httpStatusCode = SccMultiStatus;
    }
    else
      orionldState.httpStatusCode = SccNoContent;
  }
  else
  {
    LM_E(("mongoUpdateContext: HTTP Status Code: %d", orionldState.httpStatusCode));
    orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Error from Mongo-DB backend");
    return false;
  }

  return true;
}
