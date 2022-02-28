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

#include "common/globals.h"                                      // parse8601Time
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
// All db functions (all new ones anyway) should return an entity from DB AS IS, i.e. include the database model
// Then we need functions to adapt to and from DB-Model:
//
// dbModelToApiEntity           - Strip Entity of extra DB model stuff and produce an NGSI-LD API Entity (attrNames is "hidden" as .names)
// dbModelToApiAttribute        - ... produce an NGSI-LD API Attribute (mdNames is "hidden as .names)
// dbModelToApiSubAttribute     - ... produce an NGSI-LD API Sub-Attribute
// dbModelFromApiEntity         - convert an NGSI-LD API Entity into an "Orion DB" Entity
// dbModelFromApiAttribute      -
// dbModelFromApiSubAttribute   -
//
// To PATCH an Entity:
// 1. GET the entity from DB - in DB-Model style
// 2. Convert the request payload to a DB-Model style Entity (includes modifiedAt but not createdAt) - dbModelFromApiXXX()
// 3. Compare the two trees and come up with an array of changes (orionldEntityPatchTree - output from orionldPatchXXX functions):
//    [
//      { PATH, TREE },
//      { PATH, TREE },
//      ...
//    ]
//
//    E.g. (modification of a compound value of a sub-attr   +  addition of an attribute + removal of a sub-attr):
//    [
//      {
//        "path": "attrs.P1.md.Sub-R.value.F",
//        "tree": <KjNode tree of the new value>
//      },
//      {
//        "path": "attrs.P2",
//        "tree": <KjNode tree of the attribute - DB Model style>,
//      },
//      {
//        "path": [ "attrs", "P3", "md", "SP1" ]
//        "tree": null,
//        "op": "delete"
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



// -----------------------------------------------------------------------------
//
// dbModelFromApiSubAttribute -
//
// PARAMETERS
//   - saP            pointer to the sub-attribute KjNode tree
//   - 
//   - mdAddedV       array of names of new metadata (sub-attributes)
//   - mdRemovedV     array of names of sub-attributes that are to be removed (RHS == null)
//   - newAttribute   if true, the attribute is new and mdAddedV/mdRemovedV need not be used
//
bool dbModelFromApiSubAttribute(KjNode* saP, KjNode* dbMdP, KjNode* mdAddedV, KjNode* mdRemovedV, bool newAttribute)
{
  dotForEq(saP->name);  // Orion DB-Model states that dots are replaced for equal signs in sub-attribute names

  KjNode* dbSubAttributeP = (dbMdP ==  NULL)? NULL : kjLookup(dbMdP, saP->name);    
  if (saP->type == KjNull)
  {
    if (dbSubAttributeP == NULL)
    {
      LM_W(("Attempt to DELETE a sub-attribute that doesn't exist (%s)", saP->name));
      orionldError(OrionldResourceNotFound, "Cannot delete a sub-attribute that does not exist", saP->name, 404);
      // Add to 207
      return false;
    }

    LM_TMP(("KZ: '%s' has a  null RHS - to be removed", saP->name));
    KjNode* saNameP = kjString(orionldState.kjsonP, NULL, saP->name);
    kjChildAdd(mdRemovedV, saNameP);
    return true;
  }

  if (strcmp(saP->name, "value") == 0)
  {
    return true;
  }
  else if ((strcmp(saP->name, "object") == 0) || (strcmp(saP->name, "languageMap") == 0))
  {
    saP->name = (char*) "value";  // Orion-LD's database model states that all attributes have a "value"
  }
  else if (strcmp(saP->name, "observedAt") == 0)
  {
    //
    // observedAt is stored as a JSON object, as any other sub-attribute (my mistake, bad idea but too late now)
    // Also, the string representation of the ISO8601 is turned into a floating point representation
    // So, those two problems are fixed here:
    //
    double timestamp = parse8601Time(saP->value.s);
    KjNode* valueP = kjFloat(orionldState.kjsonP, "value", timestamp);

    saP->type = KjObject;
    saP->value.firstChildP = valueP;
    saP->lastChild         = valueP;

    // Also, these two special sub-attrs have no creDate/modDate but they're still sub-attrs, need to be id "mdNames"
    if (dbSubAttributeP == NULL)
      kjChildAdd(mdAddedV, kjString(orionldState.kjsonP, NULL, saP->name));
  }
  else if (strcmp(saP->name, "unitCode") == 0)
  {
    //
    // unitCode is stored as a JSON object, as any other sub-attribute (my mistake, bad idea but too late now)
    // So, that problem is fixed here:
    //
    KjNode* valueP = kjString(orionldState.kjsonP, "value", saP->value.s);

    saP->type = KjObject;
    saP->value.firstChildP = valueP;
    saP->lastChild         = valueP;

    // Also, these two special sub-attrs have no creDate/modDate but they're still sub-attrs, need to be id "mdNames"
    if (dbSubAttributeP == NULL)
      kjChildAdd(mdAddedV, kjString(orionldState.kjsonP, NULL, saP->name));
  }
  else
  {
    if (dbSubAttributeP == NULL)
    {
      timestampAdd(saP, "creDate");
      // The "mdNames" array stores the sub-attribute names in their original names, with dots - undo
      char* saName = kaStrdup(&orionldState.kalloc, saP->name);
      eqForDot(saName);
      kjChildAdd(mdAddedV, kjString(orionldState.kjsonP, NULL, saName));
    }

    // All sub-attrs get a "modifiedAt" (called "modDate" in Orion's database model
    timestampAdd(saP, "modDate");
  }

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
    LM_TMP(("KZ: '%s' has a null RHS - to be removed", attrP->name));
    KjNode* attrNameP = kjString(orionldState.kjsonP, NULL, attrP->name);
    kjChildAdd(attrRemovedV, attrNameP);
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
    const char* specialV[] = { "type", "value", "object", "languageMap", "datasetId" };  // observedAt+unitCode are mds (db-model)
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

  KjNode* dbAttrP = kjLookup(dbAttrsP, attrP->name);
  if (dbAttrP == NULL)  // Attribute does not exist already
  {
    if (attrP->type == KjNull)
    {
      LM_W(("Attempt to DELETE an attribute that doesn't exist (%s)", attrP->name));
      orionldError(OrionldResourceNotFound, "Cannot delete an attribute that does not exist", attrP->name, 404);
      // Add to 207
      return false;
    }

    timestampAdd(attrP, "creDate");

    // The "attrNames" array stores the sub-attribute names in their original names, with dots - undo
    char* attrName = kaStrdup(&orionldState.kalloc, attrP->name);
    eqForDot(attrName);
    kjChildAdd(attrAddedV, kjString(orionldState.kjsonP, NULL, attrName));

    newAttribute = true;  // For new attributes, no need to populate .added" and ".removed" - all sub.attr are "added"
  }
  else
  {
    KjNode* mdNamesP = kjLookup(dbAttrP, "mdNames");

    if (mdNamesP == NULL)
      LM_X(1, ("Fatal Error (database currupt? No 'mdNames' field in DB Attribute"));

    mdNamesP->name = (char*) ".names";
    kjChildAdd(attrP, mdNamesP);

    mdAddedP   = arrayAdd(attrP, ".added");
    mdRemovedP = arrayAdd(attrP, ".removed");
  }

  treePresent(attrP, "KZ: Attribute with unprocessed sub-attributes");
  if (attrP->type == KjNull)
    return true;

  // Sub-Attributes
  KjNode* mdV = (newAttribute == true)? NULL : kjLookup(dbAttrP, "md");
  LM_TMP(("KZ: Loop over sub-attributes (contents of 'mdP')"));
  for (KjNode* subP = mdP->value.firstChildP; subP != NULL; subP = subP->next)
  {
    LM_TMP(("KZ: sub-attribute '%s'", subP->name));
    dbModelFromApiSubAttribute(subP, mdV, mdAddedP, mdRemovedP, newAttribute);
  }

  kjChildAdd(attrP, mdP);
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
bool dbModelFromApiEntity(KjNode* entityP, KjNode* dbAttrsP, KjNode* dbAttrNamesP)
{
  KjNode*      nodeP;
  const char*  mustGo[] = { "_id", "id", "@id", "type", "@type", "scope", "createdAt", "modifiedAt", "creDate", "modDate" };

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

  // Rename "attrNames" to ".names" and add it to the entity
  if (dbAttrNamesP == NULL)
    LM_X(1, ("Fatal Error (database currupt? No 'attrNames' field in DB Entity"));

  dbAttrNamesP->name = (char*) ".names";
  // Now we can add "attrNames"
  kjChildAdd(entityP, dbAttrNamesP);

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
// patchTreeItemAdd -
//
void patchTreeItemAdd(KjNode* patchTree, const char* path, KjNode* valueP, const char* op)
{
  KjNode* arrayItemP = kjObject(orionldState.kjsonP, NULL);
  KjNode* pathP      = kjString(orionldState.kjsonP, "PATH", path);

  // valueP is STOLEN here - careful with its previous linked list ...
  valueP->name = (char*) "TREE";
  kjChildAdd(arrayItemP, pathP);
  kjChildAdd(arrayItemP, valueP);

  if (op != NULL)
  {
    KjNode* opP = kjString(orionldState.kjsonP, "op", op);
    kjChildAdd(arrayItemP, opP);
  }

  kjChildAdd(patchTree, arrayItemP);
}



static void namesArrayAddedToPatchTree(KjNode* patchTree, const char* path, KjNode* addedP)
{
  LM_TMP(("KZ2: Something added, nothing removed - $push to array"));
  patchTreeItemAdd(patchTree, path, addedP, "PUSH");
}

static void namesArrayRemovedToPatchTree(KjNode* patchTree, const char* path, KjNode* removedP)
{
  LM_TMP(("KZ2: Nothing added, somehing removed - $pull from array"));

  patchTreeItemAdd(patchTree, path, removedP, "PULL");
}

static void namesArrayMergeToPatchTree(KjNode* patchTree, const char* path, KjNode* namesP, KjNode* addedP, KjNode* removedP)
{
  LM_TMP(("KZ2: Something added, something removed - merge and $set array"));
}



static void namesToPatchTree(KjNode* patchTree, const char* path, KjNode* namesP, KjNode* addedP, KjNode* removedP)
{
  LM_TMP(("KZ2 -------------- Found a names array. path: '%s'", path));

  treePresent(namesP,   "KZ2: names");
  treePresent(addedP,   "KZ2: added");
  treePresent(removedP, "KZ2: removed");

  // Fix the attrNames/mdNames $set/$pull/$pop
  char* namesPath;

  if (path == NULL)
    namesPath = (char*) "attrNames";
  else
  {
    int namesPathLen = ((path != NULL)? strlen(path) + 1 : 0) + 8;  // 8: strlen("mdNames") + 1

    namesPath = kaAlloc(&orionldState.kalloc, namesPathLen);
    snprintf(namesPath, namesPathLen, "%s.mdNames", path);
  }

  LM_TMP(("KZ2: Fix %s", namesPath));

  bool added   = false;
  bool removed = false;

  if ((addedP   != NULL) && (addedP->value.firstChildP   != NULL))  added   = true;
  if ((removedP != NULL) && (removedP->value.firstChildP != NULL))  removed = true;

  if      ((added == false)  && (removed == false))   LM_TMP(("KZ2: Nothing added, nothing removed - nothing needs to be done!"));
  else if ((added == true)   && (removed == false))   namesArrayAddedToPatchTree(patchTree,   namesPath, addedP);
  else if ((added == false)  && (removed == true))    namesArrayRemovedToPatchTree(patchTree, namesPath, removedP);
  else if ((added == true)   && (removed == true))    namesArrayMergeToPatchTree(patchTree,   namesPath, namesP, addedP, removedP);
}



// -----------------------------------------------------------------------------
//
// orionldEntityPatchTree -
//
//   - if item exists in old but not in new it is kept (nothing added to 'patchTree')
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
void orionldEntityPatchTree(KjNode* oldP, KjNode* newP, char* path, KjNode* patchTree)
{
  // <DEBUG>
  LM_TMP(("KZ: path: %s", path));
  LM_TMP(("KZ: old:  %s", (oldP == NULL)? "NULL" : oldP->name));
  LM_TMP(("KZ: new:  %s", (newP == NULL)? "NULL" : newP->name));
  // </DEBUG>

  if (newP == NULL)  // Not sure this is ever a possibility ...
    return;

  if (oldP == NULL)
  {
    char buf[1024];
    kjFastRender(newP, buf);
    LM_TMP(("KZ: patchTreeItemAdd(PATH: %s, TREE: %s)", path, buf));
    patchTreeItemAdd(patchTree, path, newP, NULL);
    return;
  }

  // newP != NULL && oldP != NULL
  if (newP->type == KjNull)
  {
    LM_TMP(("KZ: patchTreeItemAdd(PATH: %s, TREE: null - REMOVAL)", path));
    patchTreeItemAdd(patchTree, path, newP, NULL);
    return;
  }

  if (newP->type != oldP->type)
  {
    LM_TMP(("KZ: patchTreeItemAdd(PATH: %s, TREE: <JSON %s>)", path, kjValueType(newP->type)));
    patchTreeItemAdd(patchTree, path, newP, NULL);
    return;
  }

  bool change = false;
  bool leave  = false;
  if      (newP->type == KjString)   { if (strcmp(newP->value.s, oldP->value.s) != 0)  change = true; leave = true; }
  else if (newP->type == KjInt)      { if (newP->value.i != oldP->value.i)             change = true; leave = true; }
  else if (newP->type == KjFloat)    { if (newP->value.f != oldP->value.f)             change = true; leave = true; }
  else if (newP->type == KjBoolean)  { if (newP->value.b != oldP->value.b)             change = true; leave = true; }

  if (change == true)
  {
    LM_TMP(("KZ: patchTreeItemAdd(PATH: %s, TREE: <JSON %s>)", path, kjValueType(newP->type)));
    patchTreeItemAdd(patchTree, path, newP, NULL);
  }

  if (leave == true)
    return;

  if (newP->type == KjArray)
  {
    LM_TMP(("KZ: patchTreeItemAdd(PATH: %s, TREE: <JSON %s>)", path, kjValueType(newP->type)));
    patchTreeItemAdd(patchTree, path, newP, NULL);
    return;
  }

  // Both are JSON Object - entering
  // Careful about the newP linked lists - the recursive calls may call patchTreeItemAdd and that removes items from the list
  //
  // Can't use the normal "for" - must be a "while and a next pointer"
  //
  KjNode* newItemP   = newP->value.firstChildP;
  KjNode* next;
  KjNode* namesP     = NULL;
  KjNode* addedP     = NULL;
  KjNode* removedP   = NULL;

  while (newItemP != NULL)
  {
    // Skip "hidden" fields
    if (newItemP->name[0] == '.')
    {
      LM_TMP(("KZ2: Skipping '%s' of '%s'", newItemP->name, path));

      if      (strcmp(newItemP->name, ".names")   == 0) namesP   = newItemP;
      else if (strcmp(newItemP->name, ".added")   == 0) addedP   = newItemP;
      else if (strcmp(newItemP->name, ".removed") == 0) removedP = newItemP;

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

    LM_TMP(("KZ: both objects - recursive call to orionldEntityPatchTree('%s', path:'%s')", newItemP->name, newPath));
    orionldEntityPatchTree(oldItemP, newItemP, newPath, patchTree);

    newItemP = next;
  }

  if (namesP != NULL)
    namesToPatchTree(patchTree, path, namesP, addedP, removedP);
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
  KjNode* dbAttrNamesP = kjLookup(dbEntityP, "attrNames");
  dbModelFromApiEntity(orionldState.requestTree, dbAttrsP, dbAttrNamesP);


  
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
  KjNode* patchTree     = kjArray(orionldState.kjsonP, NULL);
  KjNode* dbAttrsObject = kjObject(orionldState.kjsonP, NULL);
  kjChildAdd(dbAttrsObject, dbAttrsP);

  LM_TMP(("KZ: --------------------------- Before orionldEntityPatchTree --------------------------"));
  treePresent(dbAttrsObject, "KZ: DB-Attrs");
  treePresent(orionldState.requestTree, "KZ: Patch");
  LM_TMP(("KZ: --------------------------- Calling orionldEntityPatchTree -------------------------"));
  orionldState.requestTree->name = NULL;
  orionldEntityPatchTree(dbAttrsObject, orionldState.requestTree, NULL, patchTree);
  LM_TMP(("KZ2: --------------------------- After orionldEntityPatchTree ---------------------------"));
  treePresent(patchTree, "KZ2: PATCH TREE");

  bool b = mongocEntityUpdate(entityId, patchTree);  // Added/Removed (sub-)attrs are found as arrays named ".added" and ".removed"
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
