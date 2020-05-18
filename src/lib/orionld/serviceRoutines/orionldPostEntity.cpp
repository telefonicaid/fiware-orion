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
#include <string.h>                                              // strlen
#include <string>                                                // std::string
#include <vector>                                                // std::vector

extern "C"
{
#include "kbase/kMacros.h"                                       // K_VEC_SIZE, K_FT
#include "kjson/kjBuilder.h"                                     // kjChildRemove
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjClone.h"                                       // kjClone
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kalloc/kaStrdup.h"                                     // kaStrdup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "common/MimeType.h"                                     // MimeType
#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "ngsi/ContextAttribute.h"                               // ContextAttribute
#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                        // UpdateContextResponse
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext

#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/db/dbEntityLookup.h"                           // dbEntityLookup
#include "orionld/db/dbEntityUpdate.h"                           // dbEntityUpdate
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldContextValueExpand.h"           // orionldContextValueExpand
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/kjTree/kjTreeToContextAttribute.h"             // kjTreeToContextAttribute
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/serviceRoutines/orionldPostEntity.h"           // Own Interface



// ----------------------------------------------------------------------------
//
// attributeNotUpdated -
//
static void attributeNotUpdated(KjNode* notUpdatedP, const char* attrName, const char* reason)
{
  KjNode* notUpdatedDetailsP = kjObject(orionldState.kjsonP, NULL);
  KjNode* attrNameP          = kjString(orionldState.kjsonP, "attributeName", attrName);
  KjNode* reasonP            = kjString(orionldState.kjsonP, "reason", reason);

  kjChildAdd(notUpdatedDetailsP, attrNameP);
  kjChildAdd(notUpdatedDetailsP, reasonP);

  kjChildAdd(notUpdatedP, notUpdatedDetailsP);
}



// ----------------------------------------------------------------------------
//
// attributeUpdated -
//
static void attributeUpdated(KjNode* updatedP, const char* attrName)
{
  KjNode* attrNameP = kjString(orionldState.kjsonP, NULL, attrName);

  kjChildAdd(updatedP, attrNameP);
}



// ----------------------------------------------------------------------------
//
// orionldPostEntity -
//
// POST /ngsi-ld/v1/entities/*/attrs
//
// URI PARAMETERS
//   options=noOverwrite
//
// From ETSI spec:
//   Behaviour
//   * If the Entity Id is not present or it is not a valid URI then an error of type BadRequestData shall be raised.
//   * If the NGSI-LD endpoint does not know about this Entity, because there is no an existing Entity which id
//     (URI) is equivalent to the one passed as parameter, an error of type ResourceNotFound shall be raised.
//   * For each Attribute (Property or Relationship) included by the Entity Fragment at root level:
//     * If the target Entity does not include a matching Attribute then such Attribute shall be appended to the target Entity
//     * If the target Entity already includes a matching Attribute
//       - If no datasetId is present in the Attribute included by the Entity Fragment:
//         * If options==noOverwrite: the existing Attribute in the target Entity shall be left untouched
//         * If options!=noOverwrite: the existing Attribute in the target Entity shall be replaced by the new one supplied.
//
// The new idea on how to be able to use mongoBackend's mongoUpdateContext() for just about ANY DB Update:
// The attributes (or entire entities) that are to be updated are first selected and then REMOVED, using libdb.
// After that operation, we're free to use mongoBackend's mongoUpdateContext() to Update (Append) the attributes/entities.
//
// So:
// 01. Is the Entity ID in the URL a valid URI?
// 02. Is the payload not a JSON object?
// 03. Get the entity from mongo (using libdb)
// 04. Get the 'attrNames' array from the mongo entity
// 05. How many attributes are there (in the payload), and are they valid (if overwrite == false)?
//     Those that are invalid are removed from the tree "orionldState.requestTree"
// 06. If no valid attributes, then we're done
// 07. Create an array of the attributes "still left"
// 08. Save all attribute long names in attrNameV, also - replace all '.' for '='
// 09. Convert the remaining attributes in orionldState.requestTree to an UpdateContextRequest to prepare for call to mongoBackend
// 10. Call mongoBackend (mongoUpdateContext) to do the Update
// 11. Prepare the response
//
bool orionldPostEntity(ConnectionInfo* ciP)
{
  bool  overwrite  = (orionldState.uriParamOptions.noOverwrite == true)? false : true;
  char* entityId   = orionldState.wildcard[0];
  char* detail;

  // 1. Is the Entity ID in the URL a valid URI?
  if ((urlCheck(entityId, &detail) == false) && (urnCheck(entityId, &detail) == false))
  {
    orionldState.httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Entity ID must be a valid URI", entityId);
    return false;
  }

  // 2. Is the payload not a JSON object?
  OBJECT_CHECK(orionldState.requestTree, kjValueType(orionldState.requestTree->type));

  // 3. Get the entity from mongo
  KjNode* dbEntityP;
  if ((dbEntityP = dbEntityLookup(entityId)) == NULL)
  {
    orionldState.httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Entity does not exist", entityId);
    return false;
  }

  // 4. Get the 'attrNames' array from the mongo entity - to see whether an attribute already existed or not
  KjNode* inDbAttrNamesP = NULL;
  if (overwrite == false)
  {
    inDbAttrNamesP = kjLookup(dbEntityP, "attrNames");
    if (inDbAttrNamesP == NULL)
    {
      orionldState.httpStatusCode = SccReceiverInternalError;
      orionldErrorResponseCreate(OrionldInternalError, "Corrupt Database", "'attrNames' field of entity from DB not found");
      return false;
    }
  }

  //
  // 5. How many attributes are there (in the payload), and are they valid (if overwrite == false)?
  //    Those that are invalid are removed from the tree "orionldState.requestTree"
  int     attrsInPayload = 0;
  KjNode* attrP          = orionldState.requestTree->value.firstChildP;
  KjNode* next;
  KjNode* responseP      = kjObject(orionldState.kjsonP, NULL);
  KjNode* updatedP       = kjArray(orionldState.kjsonP, "updated");
  KjNode* notUpdatedP    = kjArray(orionldState.kjsonP, "notUpdated");
  KjNode* dbAttrsP       = kjLookup(dbEntityP, "attrs");

  if (dbAttrsP == NULL)
  {
    orionldState.httpStatusCode = SccReceiverInternalError;
    orionldErrorResponseCreate(OrionldInternalError, "Corrupt Database", "'attrs' field of entity from DB not found");
    return false;
  }

  kjChildAdd(responseP, updatedP);
  kjChildAdd(responseP, notUpdatedP);

  while (attrP != NULL)
  {
    next = attrP->next;

    if ((strcmp(attrP->name, "createdAt") == 0) || (strcmp(attrP->name, "modifiedAt") == 0))
    {
      // Remove attr from tree
      kjChildRemove(orionldState.requestTree, attrP);
      attrP = next;
      continue;
    }

    //
    // The attribute name must be expanded before any comparisons can take place
    //
    bool valueMayBeExpanded = false;
    attrP->name = orionldContextItemExpand(orionldState.contextP, attrP->name, &valueMayBeExpanded, true, NULL);

    // If overwrite is NOT allowed, attrs already in the the entity must be left alone and an error item added to the response
    if (overwrite == false)
    {
      if (kjStringValueLookupInArray(inDbAttrNamesP, attrP->name) != NULL)
      {
        attributeNotUpdated(notUpdatedP, attrP->name, "attribute already exists and overwrite is not allowed");

        // Remove attr from tree
        kjChildRemove(orionldState.requestTree, attrP);
        attrP = next;
        continue;
      }
    }

    if (valueMayBeExpanded)
      orionldContextValueExpand(attrP);

    ++attrsInPayload;
    attrP = next;
  }

  // 6. If no valid attributes, then we're done
  if (attrsInPayload == 0)
  {
    //
    // This is a copy of code from the end of this function ... fix it !
    //
    if (notUpdatedP->value.firstChildP == NULL)  // Empty array of "not updated attributes" - All OK
    {
      orionldState.responseTree   = NULL;
      orionldState.httpStatusCode = SccNoContent;
    }
    else
    {
      orionldState.responseTree   = responseP;
      orionldState.httpStatusCode = SccMultiStatus;
    }

    return true;
  }

  // 7. Create an array of the attributes "still left"
  char** attrNameV  = (char**) kaAlloc(&orionldState.kalloc, attrsInPayload * sizeof(char*));
  int    attrNameIx = 0;

  // 8. Save all attribute long names in attrNameV, also - replace all '.' for '='
  for (KjNode* attrP = orionldState.requestTree->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    attrNameV[attrNameIx] = attrP->name;
    dotForEq(attrNameV[attrNameIx]);
    ++attrNameIx;
  }

  // 8. Remove those attributes that need to be removed before adding attributes to the entity
  dbEntityAttributesDelete(entityId, attrNameV, attrsInPayload);

  //
  // 9. Convert the remaining attributes in orionldState.requestTree to an UpdateContextRequest to prepare for call to mongoBackend
  //
  UpdateContextRequest ucr;
  ContextElement*      ceP = new ContextElement();

  ceP->entityId.id = entityId;

  ucr.contextElementVector.push_back(ceP);

  for (KjNode* attrP = orionldState.requestTree->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    ContextAttribute* caP     = new ContextAttribute();
    char*             detail;

    if (kjTreeToContextAttribute(orionldState.contextP, attrP, caP, NULL, &detail) == false)
    {
      LM_E(("kjTreeToContextAttribute(%s): %s", attrP->name, detail));
      attributeNotUpdated(notUpdatedP, attrP->name, detail);
      caP->release();
      free(caP);
      continue;
    }

    attributeUpdated(updatedP, attrP->name);
    ceP->contextAttributeVector.push_back(caP);
  }

  // 10. Call mongoBackend to do the Update
  HttpStatusCode            status;
  UpdateContextResponse     ucResponse;

  ucr.updateActionType = ActionTypeAppend;
  status = mongoUpdateContext(&ucr,
                              &ucResponse,
                              orionldState.tenant,
                              ciP->servicePathV,
                              ciP->uriParam,
                              ciP->httpHeaders.xauthToken,
                              ciP->httpHeaders.correlator,
                              ciP->httpHeaders.ngsiv2AttrsFormat,
                              ciP->apiVersion,
                              NGSIV2_NO_FLAVOUR);

  if (status != SccOk)
    LM_E(("mongoUpdateContext: %d", status));

  //
  // 11. Prepare the response
  //
  if (notUpdatedP->value.firstChildP == NULL)  // Empty array of "not updated attributes" - All OK
  {
    orionldState.responseTree   = NULL;
    orionldState.httpStatusCode = SccNoContent;
  }
  else
  {
    orionldState.responseTree   = responseP;
    orionldState.httpStatusCode = SccMultiStatus;
  }

  ucr.release();
  return true;
}
