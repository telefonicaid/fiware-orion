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
extern "C"
{
#include "kbase/kMacros.h"                                       // K_VEC_SIZE
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove
#include "kjson/kjRender.h"                                      // kjFastRender
#include "kalloc/kaStrdup.h"                                     // kaStrdup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/mongoc/mongocEntityUpdate.h"                   // mongocEntityUpdate
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/types/OrionldHeader.h"                         // orionldHeaderAdd
#include "orionld/payloadCheck/pCheckEntity.h"                   // pCheckEntity
#include "orionld/serviceRoutines/orionldPatchEntity.h"          // Own Interface

//
// Just like the db pointers, we might need dbModel function pointers ...
//
// All db functions (all new ones anyway) should return an entity from DB AS IS, i.e. include the database model
// Then we need functions to adapt to and from DB-Model:
//
// dbModelToApiEntity           - Strip Entity of extra DB model stuff and produce an NGSI-LD API Entity (attrNames is "hidden" as .attrNames)
// dbModelToApiAttribute        - ... produce an NGSI-LD API Attribute (mdNames is "hidden as .mdNames)
// dbModelToApiSubAttribute     - ... produce an NGSI-LD API Sub-Attribute
// dbModelFromApiEntity         - convert an NGSI-LD API Entity into an "Orion DB" Entity
// dbModelFromApiAttribute      -
// dbModelFromApiSubAttribute   -
//
// To PATCH an Entity:
// 1. GET the entity from DB - in DB-Model style
// 2. Convert the request payload to a DB-Model style Entity (includes modifiedAt but not createdAt) - dbModelFromApiXXX()
// 3. Compare the two trees and come up with an array of changes (patchTree - output from orionldPatchXXX functions):
//    [
//      { [ PATH ], TREE },
//      { [ PATH ], TREE },
//      ...
//    ]
//
//    E.g. (modification of a compound value of a sub-attr   +  addition of an attribute + removal of a sub-attr):
//    [
//      {
//        "path": [ "attrs", "P1", "md", "Sub-R", "value", "F" ],
//        "tree": <KjNode tree of the new value>
//      },
//      {
//        "path": [ "attrs", "P2" ]
//        "tree": <KjNode tree of the attribute - DB Model style>,
//        "added": "attribute" || "sub-attribute"
//      },
//      {
//        "path": [ "attrs", "P3", "md", "SP1" ]
//        "tree": null,
//        "removed": "sub-attribute"
//      }
//    ]
//
//   The PATCH-function (not DB dependant) pinpoints every node in the tree where there is s difference
//   The PATH is the pointer to the differing node.
//   The TREE is the new value for the differing node.
//
//   TREE.type == KjNull means removal
//



// -----------------------------------------------------------------------------
//
// treePresent -
//
static void treePresent(KjNode* tree, const char* msg)
{
  char buf[2048];  // Make the buffer size a function parameter and use kaAlloc()
  kjFastRender(tree, buf);
  LM_TMP(("%s: %s", msg, buf));
}



// -----------------------------------------------------------------------------
//
// timestampAdd -
//
static void timestampAdd(KjNode* containerP, const char* name)
{
  KjNode* nodeP = kjFloat(orionldState.kjsonP, name, orionldState.requestTime);
  kjChildAdd(containerP, nodeP);
}



// -----------------------------------------------------------------------------
//
// arrayAdd -
//
static KjNode* arrayAdd(KjNode* containerP, const char* name)
{
  KjNode* nodeP = kjArray(orionldState.kjsonP, name);
  kjChildAdd(containerP, nodeP);
  return nodeP;
}



bool dbModelFromApiSubAttribute(KjNode* saP, KjNode* mdAddedV, KjNode* mdRemovedV, bool newAttribute)
{
  return true;
}


// -----------------------------------------------------------------------------
//
// dbModelFromApiAttribute -
//
// FOREACH attribute:
//   * First an object "md" is created, and all fields of the attribute, except the special ones are moved inside "md".
//     Special fields:
//     - type
//     - value/object/languageMap (must be renamed to "value" - that's part of the database model)
//     - observedAt
//     - datasetId
//     - unitCode
//
//   * Actually, all children of 'attrP' are moved to 'md'
//   * And then, all the special fields are moved back to 'attrP'
//   * ".added"   is added
//   * ".removed" is added
//   * "modDate"  is added
//   * "creDate"  is added iff the attribute did not previously exist
//
bool dbModelFromApiAttribute(KjNode* attrP, KjNode* dbAttrsP, KjNode* attrAddedV, KjNode* attrRemovedV)
{
  KjNode* mdP = NULL;

  dotForEq(attrP->name);  // Orion DB-Model states that dots are replaced for equal signs in attribute names
  LM_TMP(("KZ: Attribute: %s", attrP->name));
  if (attrP->type == KjNull)
  {
    LM_TMP(("KZ: '%s' has a  null RHS - to be removed", attrP->name));
    KjNode* attrNameP = kjString(orionldState.kjsonP, NULL, attrP->name);
    kjChildAdd(attrRemovedV, attrNameP);
    dotForEq(attrNameP->name);  // Already done?
  }
  else
  {
    // Move everything into "md", leaving attrP EMPTY
    mdP = kjObject(orionldState.kjsonP, "md");
    mdP->value.firstChildP   = attrP->value.firstChildP;
    mdP->lastChild           = attrP->lastChild;
    attrP->value.firstChildP = NULL;
    attrP->lastChild         = NULL;

    // Move special fields back to "attrP"
    const char* specialV[] = { "type", "value", "object", "languageMap", "datasetId", "observedAt", "unitCode" };
    for (unsigned int ix = 0; ix < K_VEC_SIZE(specialV); ix++)
    {
      KjNode* nodeP = kjLookup(mdP, specialV[ix]);
      if (nodeP != NULL)
      {
        if ((ix == 2) || (ix == 3))         // "object", "languageMap", change name to "value" (Orion's DB model)
          nodeP->name = (char*) "value";

        kjChildRemove(mdP, nodeP);
        kjChildAdd(attrP, nodeP);
      }
    }

    timestampAdd(attrP, "modDate");
  }

  KjNode*  mdAddedP     = NULL;
  KjNode*  mdRemovedP   = NULL;
  bool     newAttribute = false;
  char*    attrNameEq   = kaStrdup(&orionldState.kalloc, attrP->name);

  dotForEq(attrNameEq);

  if (kjLookup(dbAttrsP, attrNameEq) == NULL)  // Attribute does not exist already
  {
    if (attrP->type == KjNull)
    {
      LM_W(("Attempt to DELETE an attribute that doesn't exist (%s)", attrP->name));
      orionldError(OrionldResourceNotFound, "Cannot delete an attribute that does not exist", attrP->name, 404);
      // Add to 207
      return false;
    }

    timestampAdd(attrP, "creDate");
    kjChildAdd(attrAddedV, kjString(orionldState.kjsonP, NULL, attrNameEq));
    newAttribute = true;  // For new attributes, no need to populate .added" and ".removed" - all sub.attr are "added"
  }
  else
  {
    mdAddedP   = arrayAdd(attrP, ".added");
    mdRemovedP = arrayAdd(attrP, ".removed");
  }

  treePresent(attrP, "KZ: Attribute with unprocessed sub-attributes");
  if (attrP->type == KjNull)
    return true;

  LM_TMP(("KZ: Loop over sub-attributes (contents of 'mdP')"));
  for (KjNode* subP = mdP->value.firstChildP; subP != NULL; subP = subP->next)
  {
    LM_TMP(("KZ: sub-attribute '%s'", subP->name));
    dbModelFromApiSubAttribute(subP, mdAddedP, mdRemovedP, newAttribute);
  }
  treePresent(attrP, "KZ: Attribute with PROCESSED sub-attributes");

  return true;
}



// -----------------------------------------------------------------------------
//
// dbModelFromApiEntity - modify the request tree to match the db model
//
// Modifications:
//
//   * Entity Level (dbModelEntity)
//     * "id" can't be modified    (make sure it's removed and partial error reported by pCheckEntity)
//     * "type" can't be modified  (make sure it's removed and partial error reported by pCheckEntity)
//     * "scope" doesn't exist yet (make sure it's removed and partial error reported by pCheckEntity)
//     * "modDate" is set with orionldState.requestTime
//     * "attrs" member is created and added
//     * It's an attribute - move to "attrs" and call "attributeToDbModel" on it.
//     * if Array - datasetId - error for now (partial 501)
//     * if != Object - error (as pCheckAttribute makes sure the request tree is normalized)
//     * move the attribute there + call "Level 1 Function" (orionldDbModelAttribute)
//
//   * Attribute Level (dbModelAttribute)
//     * If Array, recursive call for each member (set the name to the sttribute name)
//     * "datasetId" present - call orionldDbModelAttributeDatasetId
//     * "type" can't be modified
//     * "value"/"object"/"languageMap" changes name to "value" and RHS stays as is
//     * "observedAt" is made an Object with single member "value"
//     * "unitCode" is made an Object with single member "value"
//     * "md" is created and added
//
//   * Sub-Attribute Level (dbModelSubAttribute)
//
bool dbModelFromApiEntity(KjNode* entityP, KjNode* dbAttrsP)
{
  KjNode*      nodeP;
  const char*  mustGo[] = { "_id", "id", "@id", "type", "@type", "scope", "createdAt", "modifiedAt", "creDate", "modDate", "attrNames" };

  //
  // Remove any non-attribute nodes
  // PATCH-able toplevel fields (type? scope?) would need to be removed and saved. Later reintroduced into entityP
  //
  for (unsigned int ix = 0; ix < sizeof(mustGo) / sizeof(mustGo[0]); ix++)
  {
    nodeP = kjLookup(entityP, mustGo[ix]);
    if (nodeP != NULL)
      kjChildRemove(entityP, nodeP);
  }

  //
  // Create the "attrs" field and move all attributes from entityP to there ("attrs")
  // Then move "attrs" inside entityP
  //
  KjNode* attrsP = kjObject(orionldState.kjsonP, "attrs");
  attrsP->value.firstChildP = entityP->value.firstChildP;
  attrsP->lastChild         = entityP->lastChild;

  // Make "attrs" the only child (thus far) of entityP
  entityP->value.firstChildP  = attrsP;
  entityP->lastChild          = attrsP;

  // Adding members necessary for the DB Model
  timestampAdd(entityP, "modDate");
  KjNode* attrAddedV   = arrayAdd(entityP, ".added");
  KjNode* attrRemovedV = arrayAdd(entityP, ".removed");  // Not really necessary ... the RHS == null already tells the next layer

  treePresent(entityP, "KZ: DB-prepped Entity with unprocessed attributes");

  //
  // Loop over the "attrs" member of entityP and call dbModelFromApiAttribute for every attribute
  //
  for (KjNode* attrP = attrsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    dbModelFromApiAttribute(attrP, dbAttrsP, attrAddedV, attrRemovedV);
  }

  treePresent(entityP, "KZ: DB-prepped Entity with PROCESSED attributes");

  return true;
}



// -----------------------------------------------------------------------------
//
// outTreeItemAdd -
//
void outTreeItemAdd(KjNode* outTree, const char* path, KjNode* valueP)
{
  KjNode* arrayItemP = kjObject(orionldState.kjsonP, NULL);
  KjNode* pathP      = kjString(orionldState.kjsonP, "PATH", path);

  // I steal valueP here - careful ...
  valueP->name = (char*) "TREE";
  kjChildAdd(arrayItemP, pathP);
  kjChildAdd(arrayItemP, valueP);
  kjChildAdd(outTree, arrayItemP);
}



// -----------------------------------------------------------------------------
//
// patchTree -
//
//   - if item exists in old but not in new it is kept (nothing added to 'outTree')
//   - if item exists in new but not in old it is added
//   - it item exists in both old and new:
//     - if new's value is null             - REMOVE item
//     - If JSON type of new item is Simple - new REPLACES old item
//     - if items JSON type differ          - new REPLACES old item
//     - if Array                           - new REPLACES old item
//     - If Object (both of them)           - recursive call
//
// Might be a good idea to sort both trees before starting with the processing ...
//
KjNode* patchTree(KjNode* oldP, KjNode* newP, char* path, KjNode* outTree)
{
  // <DEBUG>
  LM_TMP(("KZ: path: %s", path));
  LM_TMP(("KZ: old:  %s", (oldP == NULL)? "NULL" : oldP->name));
  LM_TMP(("KZ: new:  %s", (newP == NULL)? "NULL" : newP->name));
  // </DEBUG>

  if (newP == NULL)
    return NULL;

  if (oldP == NULL)
  {
    char buf[1024];
    kjFastRender(newP, buf);
    LM_TMP(("KZ: outTreeItemAdd(PATH: %s.%s, TREE: %s)", path, newP->name, buf));
    outTreeItemAdd(outTree, path, newP);
    return NULL;
  }

  // newP != NULL && oldP != NULL
  if (newP->type == KjNull)
  {
    LM_TMP(("KZ: outTreeItemAdd(PATH: %s.%s, TREE: null - REMOVAL)", path, newP->name));
    outTreeItemAdd(outTree, path, newP);
    return NULL;
  }

  if (newP->type != oldP->type)
  {
    LM_TMP(("KZ: outTreeItemAdd(PATH: %s.%s, TREE: <JSON %s>)", path, newP->name, kjValueType(newP->type)));
    outTreeItemAdd(outTree, path, newP);
    return NULL;
  }

  if ((newP->type == KjString) || (newP->type == KjInt) || (newP->type == KjFloat) || (newP->type == KjBoolean))
  {
    // Only patch if different
    if (((newP->type == KjString)   && (strcmp(newP->value.s, oldP->value.s) != 0))  ||
        ((newP->type == KjInt)      && (newP->value.i != oldP->value.i))             ||
        ((newP->type == KjFloat)    && (newP->value.f != oldP->value.f))             ||
        ((newP->type == KjBoolean)  && (newP->value.b != oldP->value.b)))
    {
      LM_TMP(("KZ: outTreeItemAdd(PATH: %s.%s, TREE: <JSON %s>)", path, newP->name, kjValueType(newP->type)));
      outTreeItemAdd(outTree, path, newP);
    }

    return NULL;
  }

  if (newP->type == KjArray)
  {
    LM_TMP(("KZ: outTreeItemAdd(PATH: %s.%s, TREE: <JSON %s>)", path, newP->name, kjValueType(newP->type)));
    outTreeItemAdd(outTree, path, newP);
    return NULL;
  }

  // Both are JSON Object - entering
  // Careful about the newP linked lists - the recursive calls may call outTreeItemAdd and that removes items from the list
  //
  // Can't use the normal "for" - must be a "while and a next pointer"
  //
  KjNode* newItemP = newP->value.firstChildP;
  KjNode* next;
  while (newItemP != NULL)
  {
    // Skip "hidden" fields
    if (newItemP->name[0] == '.')
    {
      newItemP = newItemP->next;
      continue;
    }

    next = newItemP->next;

    KjNode* oldItemP = kjLookup(oldP, newItemP->name);
    char*   newPath;

    if (path == NULL)
      newPath = newItemP->name;
    else
    {
      int     newPathLen  = strlen(path) + 1 + strlen(newItemP->name) + 1;
      char*   newPathV    = kaAlloc(&orionldState.kalloc, newPathLen);

      snprintf(newPathV, newPathLen, "%s.%s", path, newItemP->name);
      newPath = newPathV;
    }

    LM_TMP(("KZ: both objects - recursive call to patchTree('%s', path:'%s')", newItemP->name, newPath));
    patchTree(oldItemP, newItemP, newPath, outTree);

    newItemP = next;
  }

  return outTree;
}



// ----------------------------------------------------------------------------
//
// orionldPatchEntity2 -
//
// For all this Real PATCH thingy to work for any API operation, we will need the following
//
//   * mongocEntitiesGet() - returning entities as they are in the DB - with "attrs", "attrNames", and "$datasets" children as objects of the output array
//
//   * orionldEntityPatch(KjNode* dbEntity, KjNode* patchEntity)
//   * patchAttribute(KjNode* dbAttribute, KjNode* patchAttribute, KjNode* dbAttrNamesArray, bson_t* toSet, bson_t* toUnset)
//   * patchSubAttribute(KjNode* dbSubAttribute, KjNode* patchSubAttribute, KjNode* dbMdNamesArray, char* dbPath, bson_t* toSet, bson_t* toUnset)
//
//
//
//
bool orionldPatchEntity2(void)
{
  char*    entityId    = orionldState.wildcard[0];
  KjNode*  dbEntityP;
  char     buf[4096];  // DEBUG

  //
  // FIXME: would really like to only extract the attrs in the patch ...
  //        But, for that I'd need to:
  //        * check the attr name for forbidden chars
  //        * expand it
  //        * do dotForEq
  //        * create a KjNode array with all toplevel field names
  //        * sort out non-attribute names (id, type, scope, ...)
  //
  //        Perhaps later ...
  //
  dbEntityP = mongocEntityLookup(entityId);

  if (dbEntityP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Entity does not exist", entityId, 404);
    return false;
  }

  KjNode* dbAttrsP = kjLookup(dbEntityP, "attrs");
  LM_TMP(("KZ: Calling pCheckEntity with dbAttrsP at %p", dbAttrsP));

  //
  // FIXME: change pCheckEntity param no 3 to be not only the attributes, but the entire entity (dbEntityP)
  //
  if (pCheckEntity(orionldState.requestTree, false, dbAttrsP) == false)
  {
    LM_W(("Invalid payload body. %s: %s", orionldState.pd.title, orionldState.pd.detail));
    return false;
  }

  kjFastRender(dbEntityP, buf);
  LM_TMP(("DB Entity:  %s", buf));
  kjFastRender(orionldState.requestTree, buf);
  LM_TMP(("Patch Tree: %s", buf));

  // FIXME: If I destroy the tree, how do I handle forwarding later ... ?
  //        I extract the part that is to be forwarded from the tree before I destroy
  //        If non-exclusive, might be I both forward and do it locally - kjClone(attrP)
  //
  dbModelFromApiEntity(orionldState.requestTree, dbAttrsP);


  
  //
  // orionldEntityPatch - patching the entity
  //
  // Due to the questionable database model of Orion, that Orion-LD has inherited (backwards compatibility),
  // the attributes that have been added/removed must be added/removed to a redundant database field called "attrNames".
  // It's not possible to both remove and add items from a field in a single operation in mongo, so a list of all
  // added attributes and another list of all deleted attributes must be maintained for the purpose of updating "attrNames".
  //
  // The very same is true for the sub-attributes of an attribute - the database field "mdNames" is an array of all sub-attributes.
  //
  // The lists of added/deleted attributes are stored in arrays in the tree: ".added" + ".removed"
  // The lists of added/deleted sub-attributes, same same - ".added" + ".removed", under the attribute
  //
  // orionldEntityPatch returns a KjNode array of objects describing the modifications for the patch.
  // For details, see "To PATCH an Entity" above.
  //
  KjNode* outTree       = kjArray(orionldState.kjsonP, NULL);
  KjNode* dbAttrsObject = kjObject(orionldState.kjsonP, NULL);
  kjChildAdd(dbAttrsObject, dbAttrsP);

  LM_TMP(("KZ: --------------------------- Before patchTree --------------------------"));
  treePresent(dbAttrsObject, "KZ: DB-Attrs");
  treePresent(orionldState.requestTree, "KZ: Patch");
  LM_TMP(("KZ: --------------------------- Calling patchTree -------------------------"));
  orionldState.requestTree->name = NULL;
  KjNode* patchTreeP = patchTree(dbAttrsObject, orionldState.requestTree, NULL, outTree);
  LM_TMP(("KZ: --------------------------- After patchTree ---------------------------"));
  treePresent(patchTreeP, "KZ: PATCH TREE");

  bool b = mongocEntityUpdate(entityId, patchTreeP);  // Added/Removed (sub-)attrs are found as arrays named ".added" and ".removed"
  if (b == false)
  {
    bson_error_t* errP = &orionldState.mongoc.error;  // Can't be in orionldState - DB Dependant!!!

    LM_E(("mongocEntityUpdate(%s): [%d.%d]: %s", entityId, errP->domain, errP->code, errP->message));

    if (errP->code == 12)
      orionldError(OrionldResourceNotFound, "Entity not found", entityId, 404);
    else
      orionldError(OrionldInternalError, "Internal Error", errP->message, 500);
    return false;
  }

  orionldState.httpStatusCode = 204;
  return true;
}
