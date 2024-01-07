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
#include <string>                                                // std::string
#include <vector>                                                // std::vector

extern "C"
{
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove
#include "kjson/kjRender.h"                                      // kjFastRender
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext

#include "orionld/types/OrionldContextItem.h"                    // OrionldContextItem
#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/common/attributeUpdated.h"                     // attributeUpdated
#include "orionld/common/attributeNotUpdated.h"                  // attributeNotUpdated
#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_OBJECT
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/payloadCheck/pCheckAttribute.h"                // pCheckAttribute
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/legacyDriver/kjTreeToContextAttribute.h"       // kjTreeToContextAttribute
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityLookup.h"   // mongoCppLegacyEntityLookup
#include "orionld/legacyDriver/legacyPatchEntity.h"              // Own Interface



// ----------------------------------------------------------------------------
//
// legacyPatchEntity -
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
bool legacyPatchEntity(void)
{
  char* entityId   = orionldState.wildcard[0];
  char* detail;

  // 1. Is the Entity ID in the URL a valid URI?
  if (pCheckUri(entityId, "Entity ID from URL PATH", true) == false)
    return false;

  // 2. Is the payload not a JSON object?
  PCHECK_OBJECT(orionldState.requestTree, 0, NULL, kjValueType(orionldState.requestTree->type), 400);

  // 3. Get the entity from mongo
  KjNode* dbEntityP;
  if ((dbEntityP = mongoCppLegacyEntityLookup(entityId)) == NULL)
  {
    orionldError(OrionldResourceNotFound, "Entity does not exist", entityId, 404);
    return false;
  }

  // 3. Get the Entity Type, needed later in the call to the constructor of ContextElement
  KjNode* idNodeP = kjLookup(dbEntityP, "_id");

  if (idNodeP == NULL)
  {
    orionldError(OrionldInternalError, "Corrupt Database", "'_id' field of entity from DB not found", 500);
    return false;
  }

  KjNode*     entityTypeNodeP = kjLookup(idNodeP, "type");
  const char* entityType      = (entityTypeNodeP != NULL)? entityTypeNodeP->value.s : NULL;

  if (entityTypeNodeP == NULL)
  {
    orionldError(OrionldInternalError, "Corrupt Database", "'_id::type' field of entity from DB not found", 500);
    return false;
  }

  // 4. Get the 'attrNames' array from the mongo entity - to see whether an attribute already existed or not
  KjNode* inDbAttrNamesP = kjLookup(dbEntityP, "attrNames");
  if (inDbAttrNamesP == NULL)
  {
    orionldError(OrionldInternalError, "Corrupt Database", "'attrNames' field of entity from DB not found", 500);
    return false;
  }

  // 4. Also, get the "attrs" object for insertion of modified attributes
  KjNode* inDbAttrsP = kjLookup(dbEntityP, "attrs");
  if (inDbAttrsP == NULL)
  {
    orionldError(OrionldInternalError, "Corrupt Database", "'attrs' field of entity from DB not found", 500);
    return false;
  }

  //
  // 5. Loop over the incoming payload data
  //    Those attrs that don't exist in the DB (dbEntityP) are discarded and added to the 'notUpdated' array
  //    Those that do exist in dbEntityP are removed from dbEntityP and replaced with the corresponding attribute from the incoming payload data.
  //    (finally the modified dbEntityP will REPLACE what is currently in the database)
  //
  KjNode*              newAttrP     = orionldState.requestTree->value.firstChildP;
  KjNode*              next;
  KjNode*              updatedP     = kjArray(orionldState.kjsonP, "updated");
  KjNode*              notUpdatedP  = kjArray(orionldState.kjsonP, "notUpdated");
  int                  newAttrs     = 0;
  OrionldContextItem*  contextItemP = NULL;

  while (newAttrP != NULL)
  {
    char* shortName = newAttrP->name;

    next = newAttrP->next;

    if ((strcmp(newAttrP->name, "createdAt") == 0) || (strcmp(newAttrP->name, "modifiedAt") == 0))
    {
      attributeNotUpdated(notUpdatedP, shortName, "built-in timestamps are ignored", NULL);
      newAttrP = next;
      continue;
    }
    else if ((strcmp(newAttrP->name, "id") == 0) || (strcmp(newAttrP->name, "@id") == 0))
    {
      attributeNotUpdated(notUpdatedP, shortName, "the ID of an entity cannot be altered", NULL);
      newAttrP = next;
      continue;
    }
    else if ((strcmp(newAttrP->name, "type") == 0) || (strcmp(newAttrP->name, "@type") == 0))
    {
      attributeNotUpdated(notUpdatedP, shortName, "the TYPE of an entity cannot be altered", NULL);
      newAttrP = next;
      continue;
    }
    else
    {
      if (pCheckUri(newAttrP->name, newAttrP->name, false) == false)
      {
        attributeNotUpdated(notUpdatedP, shortName, "invalid attribute name", NULL);
        newAttrP = next;
        continue;
      }

      newAttrP->name = orionldAttributeExpand(orionldState.contextP, newAttrP->name, true, NULL);  // Expansion before pCheckAttribute
    }

    //
    // Check and Normalize
    //
    KjNode*               dbAttributeP;
    OrionldAttributeType  attributeType     = NoAttributeType;
    char*                 eqName            = kaStrdup(&orionldState.kalloc, newAttrP->name);

    dotForEq(eqName);
    dbAttributeP = kjLookup(inDbAttrsP, eqName);
    if (dbAttributeP == NULL)  // Doesn't already exist - must be discarded
    {
      attributeNotUpdated(notUpdatedP, shortName, "attribute doesn't exist", newAttrP->name);
      newAttrP = next;
      continue;
    }

    //
    // Get the type of the attribute (from DB)
    //
    KjNode* dbTypeP = kjLookup(dbAttributeP, "type");

    if (dbTypeP != NULL)
      attributeType = orionldAttributeType(dbTypeP->value.s);

    if (pCheckAttribute(entityId, newAttrP, true, attributeType, true, contextItemP) == false)
    {
      //
      // A failure will set a 400 (probably) in orionldState.pd.status
      // Here it's OK to fail, as this piece of info is put in a 207 response.
      // So, need to set it back to 200
      //
      // pCheckAttribute should perhaps have a parameter about "ok-to-fail"
      //
      LM_E(("attributeCheck: title: '%s', detail: '%s', code: %d", orionldState.pd.title, orionldState.pd.detail, orionldState.pd.status));
      orionldState.pd.status = 200;
      attributeNotUpdated(notUpdatedP, shortName, orionldState.pd.title, orionldState.pd.detail);
      newAttrP = next;
      continue;
    }

    // Steal createdAt from dbAttributeP?

    // Remove the attribute to be updated (from dbEntityP::inDbAttrsP) and insert the attribute from the payload data
    kjChildRemove(inDbAttrsP, dbAttributeP);
    kjChildAdd(inDbAttrsP, newAttrP);
    attributeUpdated(updatedP, shortName);

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

      if (kjTreeToContextAttribute(orionldState.contextP, attrP, caP, NULL, &detail) == false)
      {
        LM_E(("kjTreeToContextAttribute: %s", detail));
        attributeNotUpdated(notUpdatedP, attrP->name, "Error", detail);
        delete caP;
      }
      else
        ceP->contextAttributeVector.push_back(caP);
    }
    ucRequest.updateActionType = ActionTypeReplace;


    // 8. Call mongoBackend to do the REPLACE of the entity
    UpdateContextResponse    ucResponse;
    std::vector<std::string> servicePathV;
    servicePathV.push_back("/");

    orionldState.httpStatusCode = mongoUpdateContext(&ucRequest,
                                                     &ucResponse,
                                                     orionldState.tenantP,
                                                     servicePathV,
                                                     orionldState.in.xAuthToken,
                                                     orionldState.correlator,
                                                     orionldState.attrsFormat,
                                                     orionldState.apiVersion,
                                                     NGSIV2_NO_FLAVOUR);

    ucRequest.release();
  }

  // 9. Postprocess output from mongoBackend
  if ((orionldState.pd.status == 200) || (orionldState.pd.status == 0))
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

      orionldState.httpStatusCode = 207;
    }
    else
      orionldState.httpStatusCode = 204;
  }
  else
  {
    LM_E(("Error: %s: %s (%d)", orionldState.pd.title, orionldState.pd.detail, orionldState.pd.status));
    return false;
  }

  return true;
}
