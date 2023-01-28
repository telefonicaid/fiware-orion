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

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/legacyDriver/legacyPatchAttribute.h"           // legacyPatchAttribute
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/payloadCheck/pCheckAttribute.h"                // pCheckAttribute
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/mongoc/mongocEntityGet.h"                      // mongocEntityGet
#include "orionld/mongoc/mongocAttributesAdd.h"                  // mongocAttributesAdd
#include "orionld/dbModel/dbModelFromApiSubAttribute.h"          // dbModelFromApiSubAttribute
#include "orionld/notifications/alteration.h"                    // alteration
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
    LM(("PA: Removed '%s' from '%s'", subAttrName, mdArray->name));
  }
  else
    LM(("PA: Can't find subAttr '%s' in 'md'", subAttrName));
}



// -----------------------------------------------------------------------------
//
// mdItemAdd -
//
static void mdItemAdd(KjNode* mdArray, const char* subAttrName)
{
  KjNode* mdItemP = kjString(orionldState.kjsonP, NULL, subAttrName);

  kjChildAdd(mdArray, mdItemP);
  LM(("PA: Added '%s' to '%s'", subAttrName, mdArray->name));
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
  kjTreeLog(dbAttrP, "dbAttrP BEFORE attributeMerge");

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

    LM(("PA: Treating sub-attr '%s'", subAttrP->name));

    bool isValue = false;

    if      (strcmp(subAttrP->name, "languageMap") == 0)  { isValue = true; }
    else if (strcmp(subAttrP->name, "object")      == 0)  { isValue = true; }
    else if (strcmp(subAttrP->name, "value")       == 0)  { isValue = true; }

    //
    // Just a change in the "value"
    //
    if (isValue == true)
    {
      // Can't be null - must have been captured earlier (pCheckAttribute?)
      KjNode* dbValueP = kjLookup(dbAttrP, "value");
      if (dbValueP != NULL)
      {
        LM(("PA: Setting the value of the attribute"));
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
    LM(("PA: Looking up '%s' in dbAttr's md", eqName));
    KjNode* dbSubAttrP = kjLookup(mdP, eqName);

    if (dbSubAttrP != NULL)
    {
      LM(("PA: Found it"));
      LM(("PA: Removing already existing sub-attr '%s'", dbSubAttrP->name));
      kjChildRemove(mdP, dbSubAttrP);
      LM(("PA: Removed sub-attr '%s'", dbSubAttrP->name));
    }
    else
      LM(("PA: Did not find it"));


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
      LM(("PA: dbSubAttrP == NULL, so, adding '%s' to mdNames", subAttrP->name));
      mdItemAdd(mdNamesP, subAttrP->name);
    }

    // dbModelFromApiSubAttribute only sets 'ignore' if RHS is NULL - already taken care of
    bool ignore = false;
    dbModelFromApiSubAttribute(subAttrP, dbSubAttrP, addedV, removedV, &ignore);

    if (strcmp(subAttrP->name, "observedAt") == 0)
      LM(("PA: observedAt ... special treatment (not String - object with Float)"));
    else if (strcmp(subAttrP->name, "unitCode") == 0)
      LM(("PA: observedAt ... special treatment (not String - object with String)"));

    LM(("PA: Adding sub-attr '%s' to 'md'", subAttrP->name));
    kjChildRemove(incomingP, subAttrP);
    kjChildAdd(mdP, subAttrP);

    if (mustAdd == true)
    {
      LM(("PA: Adding 'md' to dbAttr"));
      kjChildAdd(dbAttrP, mdP);
      mustAdd = false;
    }

    subAttrP = next;
  }

  kjTreeLog(dbAttrP, "dbAttrP");
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

  char* entityId = orionldState.wildcard[0];
  char* attrName = orionldState.wildcard[1];

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
  // orionldMhdConnectionTreat() expands the attribute name for us.
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

  //
  // GET the entire entity
  // - we need all of it (in API format) for notifications
  // - to update mongo, we only need the entity type and the attribute in question
  // - for TRoE, just the attribute
  //
  //
  KjNode* dbEntityP = mongocEntityGet(entityId, NULL, true);

  if (dbEntityP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Entity Not Found", entityId, 404);
    return false;
  }

  //
  // First of all, make sure the attribute exists
  //
  KjNode* dbAttrsP = kjLookup(dbEntityP, "attrs");
  if (dbAttrsP == NULL)  // Entity without attributes
  {
    orionldError(OrionldResourceNotFound, "Entity/Attribute Not Found", entityId, 404);
    return false;
  }

  KjNode* dbAttrP = kjLookup(dbAttrsP, longAttrNameEq);
  if (dbAttrP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Entity/Attribute Not Found", entityId, 404);
    return false;
  }

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

  kjTreeLog(orionldState.requestTree, "TR: Right after pCheckAttribute");
  // Keep untouched initial state of the entity in the database - for alterations (to check for false updates)
  KjNode* initialDbEntityP = kjClone(orionldState.kjsonP, dbEntityP);
  kjTreeLog(dbEntityP, "ALT3: dbEntityP");
  // Merge orionldState.requestTree into dbEntityP, then convert to API Entity, for Alterations
  KjNode* addedV    = kjArray(orionldState.kjsonP, ".added");
  KjNode* removedV  = kjArray(orionldState.kjsonP, ".removed");

  //
  // attributeMerge DESTROYs orionldState.requestTree
  // We need it intact for ALTERATION and TROE as well (if on)
  //
  KjNode* incomingP = kjClone(orionldState.kjsonP, orionldState.requestTree);

  attributeMerge(dbAttrP, orionldState.requestTree, addedV, removedV);
  kjTreeLog(dbEntityP, "ALT3: dbEntityP after attributeMerge");

  //
  // To call mongocAttributesAdd we need the attribute in an array
  //
  kjTreeLog(dbAttrP, "PA: DB Attribute to mongo");
  int r = mongocAttributesAdd(entityId, NULL, dbAttrP, true);
  if (r == false)
    LM_E(("Database Error ()"));


  kjTreeLog(dbEntityP, "ALT3: After mongocAttributesAdd");

  //
  // For alteration, we need:
  //   * the Entity Type (must take it from the DB - expanded
  //   * The Final API entity - converted from the final DB Entity
  //
  kjTreeLog(dbEntityP, "dbEntityP");
  KjNode*     _idNodeP         = kjLookup(dbEntityP, "_id");

  if (_idNodeP == NULL)
    LM_E(("Database Error (no _id in the DB for entity '%s')", entityId));
  else
  {
    KjNode*      eTypeNodeP = kjLookup(_idNodeP, "type");
    const char*  entityType = (eTypeNodeP != NULL)? eTypeNodeP->value.s : NULL;

    if (entityType != NULL)
    {
      KjNode* finalApiEntityP = dbModelToApiEntity2(dbEntityP, false, RF_NORMALIZED, NULL, false, &orionldState.pd);

      // We need the "Incoming Entity", we have just an attribute ...
      KjNode* attributeNodeP = incomingP;
      KjNode* inEntityP      = kjObject(orionldState.kjsonP, NULL);

      //
      // For 'alterations, the attribute name needs to be like in the DB (dotForEq)
      //
      orionldState.requestTree->name = kaStrdup(&orionldState.kalloc, longAttrNameEq);

      kjChildAdd(inEntityP, attributeNodeP);

      kjTreeLog(inEntityP, "ALT2: inEntityP");
      kjTreeLog(incomingP, "ALT2: incomingP");
      kjTreeLog(finalApiEntityP, "ALT2: finalApiEntityP");
      alteration(entityId, entityType, finalApiEntityP, inEntityP, initialDbEntityP);
    }
    else
      LM_E(("Database Error (no _id::type in the DB for entity '%s')", entityId));
  }

  orionldState.httpStatusCode = 204;
  orionldState.responseTree   = NULL;

  if (troe == true)
  {
    orionldState.requestTree = incomingP;
    kjTreeLog(orionldState.requestTree, "TR: Tree for TRoE");
  }

  return true;
}
