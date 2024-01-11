/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include <string.h>                                              // strncpy

extern "C"
{
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildAdd, kjChildRemove
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/responseFix.h"                          // responseFix
#include "orionld/legacyDriver/legacyPatchAttribute.h"           // legacyPatchAttribute
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/payloadCheck/pCheckAttribute.h"                // pCheckAttribute
#include "orionld/dbModel/dbModelFromApiSubAttribute.h"          // dbModelFromApiSubAttribute
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/kjTree/kjChildCount.h"                         // kjChildCount
#include "orionld/kjTree/kjSort.h"                               // kjStringArraySort
#include "orionld/mongoc/mongocEntityGet.h"                      // mongocEntityGet
#include "orionld/mongoc/mongocAttributesAdd.h"                  // mongocAttributesAdd
#include "orionld/distOp/distOpRequests.h"                       // distOpRequests
#include "orionld/distOp/distOpResponses.h"                      // distOpResponses
#include "orionld/distOp/distOpListRelease.h"                    // distOpListRelease
#include "orionld/notifications/alteration.h"                    // alteration
#include "orionld/notifications/sysAttrsStrip.h"                 // sysAttrsStrip
#include "orionld/notifications/previousValuePopulate.h"         // previousValuePopulate
#include "orionld/serviceRoutines/orionldPatchAttribute.h"       // Own interface



// -----------------------------------------------------------------------------
//
// mdItemRemove -
//
static void mdItemRemove(KjNode* mdArray, const char* subAttrName)
{
  KjNode* mdItemP = kjStringValueLookupInArray(mdArray, subAttrName);

  if (mdItemP != NULL)
  {
    kjChildRemove(mdArray, mdItemP);
    LM_T(LmtSR, ("Removed '%s' from '%s'", subAttrName, mdArray->name));
  }
  else
    LM_T(LmtSR, ("Can't find subAttr '%s' in 'md'", subAttrName));
}



// -----------------------------------------------------------------------------
//
// mdItemAdd -
//
static void mdItemAdd(KjNode* mdArray, const char* subAttrName)
{
  KjNode* mdItemP = kjString(orionldState.kjsonP, NULL, subAttrName);

  kjChildAdd(mdArray, mdItemP);
  LM_T(LmtSR, ("Added '%s' to '%s'", subAttrName, mdArray->name));
}



// -----------------------------------------------------------------------------
//
// attributeMerge -
//
// Already existing Sub-Attributes are REPLACED, if present in the incoming payload
// New Sub-Attributes are appended.
//
static void attributeMerge(KjNode* dbAttrP, KjNode* incomingP, KjNode* addedV, KjNode* removedV)
{
  KjNode* mdP      = kjLookup(dbAttrP, "md");
  KjNode* mdNamesP = kjLookup(dbAttrP, "mdNames");
  bool    mustAdd  = false;

  if (mdP == NULL)
  {
    mdP = kjObject(orionldState.kjsonP, "md");
    mustAdd = true;
  }

  // "mdNames" is always present, this is just a "just in case" ...
  if (mdNamesP == NULL)
  {
    mdNamesP = kjArray(orionldState.kjsonP, "mdNames");
    kjChildAdd(dbAttrP, mdNamesP);
  }

  //
  // Loop subAttrP over incomingP
  // subAttrP is added to mdP if all is OK (and it's actually a sub-attribute)
  // So, we can't use a for-loop, must be a while with a next-pointer
  //
  KjNode* subAttrP = incomingP->value.firstChildP;
  KjNode* next;

  while (subAttrP != NULL)
  {
    next = subAttrP->next;

    if (strcmp(subAttrP->name, "type") == 0)
    {
      subAttrP = next;
      continue;
    }

    bool isValue = false;

    if      (strcmp(subAttrP->name, "languageMap") == 0)  { isValue = true; }
    else if (strcmp(subAttrP->name, "object")      == 0)  { isValue = true; }
    else if (strcmp(subAttrP->name, "value")       == 0)  { isValue = true; }

    //
    // Just a change in the "value"
    //
    if (isValue == true)
    {
      previousValuePopulate(NULL, dbAttrP, orionldState.in.pathAttrExpanded);
      // Can't be null - must have been captured earlier (pCheckAttribute?)
      KjNode* dbValueP = kjLookup(dbAttrP, "value");
      if (dbValueP != NULL)
      {
        dbValueP->type  = subAttrP->type;
        dbValueP->value = subAttrP->value;
      }
      else
        LM_E(("Databse Error (attribute '%s' without a 'value' field in the database)", dbAttrP->name));

      subAttrP = next;
      continue;
    }

    char eqName[512];
    strncpy(eqName, subAttrP->name, sizeof(eqName) - 1);
    dotForEq(eqName);
    LM_T(LmtSR, ("Looking up '%s' in dbAttr's md", eqName));
    KjNode* dbSubAttrP = kjLookup(mdP, eqName);

    if (dbSubAttrP != NULL)
      kjChildRemove(mdP, dbSubAttrP);


    //
    // sub-attr value can be NULL - if so, just "continue" (that removes it)
    // Might need this for Alterations though - store it somewhere? .removed array?
    // Also, who converts the JSON Null Literal to a real 'null' ?
    //   pCheckAttribute?
    //
    if (subAttrP->type == KjNull)
    {
      mdItemRemove(mdNamesP, subAttrP->name);
      subAttrP = next;
      continue;
    }

    if (dbSubAttrP == NULL)
    {
      LM_T(LmtSR, ("dbSubAttrP == NULL, so, adding '%s' to mdNames", subAttrP->name));
      mdItemAdd(mdNamesP, subAttrP->name);
    }

    // dbModelFromApiSubAttribute only sets 'ignore' if RHS is NULL - already taken care of
    bool ignore = false;
    dbModelFromApiSubAttribute(subAttrP, dbSubAttrP, addedV, removedV, &ignore);

    if (strcmp(subAttrP->name, "observedAt") == 0)
      LM_T(LmtSR, ("observedAt ... special treatment (not String - object with Float)"));
    else if (strcmp(subAttrP->name, "unitCode") == 0)
      LM_T(LmtSR, ("observedAt ... special treatment (not String - object with String)"));

    LM_T(LmtSR, ("Adding sub-attr '%s' to 'md'", subAttrP->name));
    kjChildRemove(incomingP, subAttrP);
    kjChildAdd(mdP, subAttrP);

    if (mustAdd == true)
    {
      LM_T(LmtSR, ("Adding 'md' to dbAttr"));
      kjChildAdd(dbAttrP, mdP);
      mustAdd = false;
    }

    subAttrP = next;
  }
}



// ----------------------------------------------------------------------------
//
// dbModelEntityTypeExtract -
//
char* dbModelEntityTypeExtract(KjNode* dbEntityP)
{
  KjNode* _idP = kjLookup(dbEntityP, "_id");

  if (_idP != NULL)
  {
    KjNode* typeP = kjLookup(_idP, "type");
    if (typeP != NULL)
      return typeP->value.s;
  }

  return NULL;
}



// ----------------------------------------------------------------------------
//
// orionldPatchAttribute -
//
// From the API Spec:
//   This operation allows performing a partial update on an NGSI-LD Entity's Attribute (Property or Relationship).
//   A partial update only changes the elements provided in an Entity Fragment, leaving the rest as they are.
//   This operation supports the deletion of sub-Attributes but not the deletion of the whole Attribute itself.
//
//   If present in the provided NGSI-LD Entity Fragment, the type of the Attribute has to be the same as the
//   type of the targeted Attribute fragment, i.e. it is not allowed to change the type of an Attribute.
//
//   Perform a partial update patch operation on the target Attribute following the algorithm mandated by clause 5.5.8:
//     The Partial Update Patch procedure modifies an existing NGSI-LD element by overwriting the data at the Attribute
//     level, replacing it with the data provided in the NGSI-LD Fragment.
//
//  --------------------------------------------------------------------------------------------------------------------------
//
// In other words:
//   * The Attribute Type cannot be modified.
//   * Sub-Attributes are either REPLACED or ADDED.
//
//  --------------------------------------------------------------------------------------------------------------------------
//
// The DB update will touch the following fields:
//   * Entity 'modDate'
//   * Attribute 'modDate'
//   * The Attributes 'mdNames' (push and/or pull)
//   * For REPLACED sub-attrs:
//     - Keep createdAt from older sub-attribute
//   * For DELETED sub-attrs:
//     - $pull from mdNames
//   * For ADDED sub-attrs:
//     - $push from mdNames
//
// If sub-attrs are both added and removed, mongo doesn't allow to do both $push and $pull in a single operation,
// so we'll need the initial mdNames merged with the new+deleted sub-attrs names.
//
// So, from the DB we only need (for the update):
//   Entity.attrs.$attrName.mdNames     to differentiate between ADDED and REPLACED sub-attributes
//   Entity.attrs.$attrName.type        to make sure thje attribute type isn't modified)
//
// BUT, for notifications, we may need the entity entity ...
// So, as always, we'll GET the entire entity, PATCH it, and hand it over to Alterations.
//
bool orionldPatchAttribute(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyPatchAttribute();

  char* entityId   = orionldState.wildcard[0];
  char* entityType = NULL;
  char* attrName   = orionldState.wildcard[1];

  //
  // Make sure the Entity ID is a valid URI
  //
  if (pCheckUri(entityId, "Entity ID from URL PATH", true) == false)
    return false;

  //
  // Make sure the Attribute Name is valid
  //
  if (pCheckUri(attrName, "Attribute Name from URL PATH", false) == false)
    return false;


  //
  // mhdConnectionTreat() expands the attribute name for us.
  // Here we save it in the orionldState.wildcard array, so that TRoE won't have to expand it
  //
  orionldState.wildcard[1] = orionldState.in.pathAttrExpanded;
  char longAttrNameEq[512];
  strncpy(longAttrNameEq, orionldState.in.pathAttrExpanded, sizeof(longAttrNameEq) - 1);
  dotForEq(longAttrNameEq);

  //
  // Now that the attribute name is expanded, let's name the payload -
  // it is an attribute, and we have the name of it, but the json parser didn't know that !
  //
  orionldState.requestTree->name = orionldState.in.pathAttrExpanded;

  //
  // GET the entire entity
  // - we need all of it (in API format) for notifications
  // - to update mongo, we only need the entity type and the attribute in question
  // - for TRoE, just the attribute
  //
  //
  KjNode* dbEntityP = mongocEntityGet(entityId, NULL, true);

  if (dbEntityP != NULL)
    entityType = dbModelEntityTypeExtract(dbEntityP);
  else if (orionldState.distributed == false)
  {
    orionldError(OrionldResourceNotFound, "Entity Not Found", entityId, 404);
    return false;
  }

  KjNode* responseBody = kjObject(orionldState.kjsonP, NULL);

  if (orionldState.distributed == true)
  {
    KjNode* body = kjObject(orionldState.kjsonP, NULL);
    kjChildAdd(body, orionldState.requestTree);
    DistOp* distOpList = distOpRequests(entityId, entityType, DoUpdateAttrs, body);  // DoUpdateAttrs should be Singular !!!
    if (distOpList != NULL)
    {
      distOpResponses(distOpList, responseBody);
      distOpListRelease(distOpList);
    }
  }

  KjNode* incomingP = NULL;
  //
  // If the entity is not found locally and all distributed requests give 404
  // then it's a 404
  //
  bool notFound = ((dbEntityP == NULL) && (orionldState.distOp.requests == orionldState.distOp.e404));

  if ((notFound == false) && (orionldState.requestTree != NULL) && (orionldState.requestTree->value.firstChildP != NULL))  // Attribute left for local request
  {
    //
    // First of all, make sure the attribute exists
    //
    KjNode* dbAttrsP = (dbEntityP != NULL)? kjLookup(dbEntityP, "attrs") : NULL;
    if ((dbAttrsP == NULL) && (orionldState.distributed == false))  // Entity without attributes
    {
      orionldError(OrionldResourceNotFound, "Entity/Attribute Not Found", entityId, 404);
      return false;
    }

    KjNode* dbAttrP = (dbAttrsP != NULL)? kjLookup(dbAttrsP, longAttrNameEq) : NULL;
    if ((dbAttrP == NULL) && (orionldState.distributed == false))
    {
      orionldError(OrionldResourceNotFound, "Entity/Attribute Not Found", entityId, 404);
      return false;
    }

    if (dbAttrP != NULL)
    {
      KjNode* dbAttrTypeP = kjLookup(dbAttrP, "type");

      if (dbAttrTypeP == NULL)
      {
        orionldError(OrionldInternalError, "Database Error (attribute without a type in the database)", longAttrNameEq, 500);
        return false;
      }
      const char* attrTypeInDb = dbAttrTypeP->value.s;

      //
      // Make sure the incoming attribute is valid
      //
      OrionldAttributeType attrType = orionldAttributeType(attrTypeInDb);
      if (pCheckAttribute(entityId, orionldState.requestTree, true, attrType, true, NULL) == false)
        return false;

      // Keep untouched initial state of the entity in the database - for alterations (to check for false updates)
      KjNode* initialDbEntityP = kjClone(orionldState.kjsonP, dbEntityP);

      // Merge orionldState.requestTree into dbEntityP, then convert to API Entity, for Alterations
      KjNode* addedV    = kjArray(orionldState.kjsonP, ".added");
      KjNode* removedV  = kjArray(orionldState.kjsonP, ".removed");

      //
      // attributeMerge DESTROYs orionldState.requestTree
      // We need it intact for ALTERATION and TROE as well (if on)
      //
      incomingP = kjClone(orionldState.kjsonP, orionldState.requestTree);

      attributeMerge(dbAttrP, orionldState.requestTree, addedV, removedV);

      //
      // To call mongocAttributesAdd we need the attribute in an array
      //
      int r = mongocAttributesAdd(entityId, NULL, dbAttrP, true);
      if (r == false)
        LM_E(("Database Error ()"));

      //
      // For alteration, we need:
      //   * the Entity Type (must take it from the DB - expanded
      //   * The Final API entity - converted from the final DB Entity
      //
      KjNode*     _idNodeP         = kjLookup(dbEntityP, "_id");

      if (_idNodeP == NULL)
        LM_E(("Database Error (no _id in the DB for entity '%s')", entityId));
      else
      {
        KjNode*      eTypeNodeP = kjLookup(_idNodeP, "type");
        const char*  entityType = (eTypeNodeP != NULL)? eTypeNodeP->value.s : NULL;

        if (entityType != NULL)
        {
          KjNode* finalApiEntityWithSysAttrsP = dbModelToApiEntity2(dbEntityP, true, RF_NORMALIZED, NULL, false, &orionldState.pd);

          // We need the "Incoming Entity", we have just an attribute ...
          KjNode* attributeNodeP = incomingP;
          KjNode* inEntityP      = kjObject(orionldState.kjsonP, NULL);

          //
          // For 'alterations, the attribute name needs to be like in the DB (dotForEq)
          //
          orionldState.requestTree->name = kaStrdup(&orionldState.kalloc, longAttrNameEq);

          kjChildAdd(inEntityP, attributeNodeP);

          KjNode* finalApiEntityWithoutSysAttrsP = kjClone(orionldState.kjsonP, finalApiEntityWithSysAttrsP);
          sysAttrsStrip(finalApiEntityWithoutSysAttrsP);

          OrionldAlteration* alterationP = alteration(entityId, entityType, finalApiEntityWithoutSysAttrsP, inEntityP, initialDbEntityP);
          alterationP->finalApiEntityWithSysAttrsP = finalApiEntityWithSysAttrsP;
        }
        else
          LM_E(("Database Error (no _id::type in the DB for entity '%s')", entityId));
      }
    }
  }

  if (notFound == true)
  {
    orionldError(OrionldResourceNotFound, "Entity/Attribute Not Found", entityId, 404);
    return false;
  }

  responseFix(responseBody, DoUpdateAttrs, 204, entityId);

  if ((troe == true) && (incomingP != NULL))
    orionldState.requestTree = incomingP;

  return true;
}
