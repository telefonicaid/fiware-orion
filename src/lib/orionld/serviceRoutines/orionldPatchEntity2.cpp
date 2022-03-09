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
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "common/globals.h"                                      // parse8601Time
#include "orionld/mongoc/mongocEntityUpdate.h"                   // mongocEntityUpdate
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/types/OrionldHeader.h"                         // orionldHeaderAdd
#include "orionld/kjTree/kjJsonldNullObject.h"                   // kjJsonldNullObject
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/kjTree/kjTimestampAdd.h"                       // kjTimestampAdd
#include "orionld/kjTree/kjArrayAdd.h"                           // kjArrayAdd
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/payloadCheck/pCheckEntity.h"                   // pCheckEntity
#include "orionld/db/dbModelFromApiEntity.h"                     // dbModelFromApiEntity
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
//   The PATCH-function (not DB dependent) pinpoints every node in the tree where there is s difference
//   The PATH is the pointer to the differing node.
//   The TREE is the new value for the differing node.
//
//   TREE.type == KjNull means removal
//



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



// -----------------------------------------------------------------------------
//
// namesArrayMergeToPatchTree -
//
static void namesArrayMergeToPatchTree(KjNode* patchTree, const char* path, KjNode* namesP, KjNode* addedP, KjNode* removedP)
{
  for (KjNode* rmP = removedP->value.firstChildP; rmP != NULL; rmP = rmP->next)
  {
    KjNode* itemP = kjStringValueLookupInArray(namesP, rmP->value.s);
    if (itemP)
      kjChildRemove(namesP, itemP);
  }

  // Now add the strings in addedP:
  if (namesP->value.firstChildP == NULL)  // Array is empty
    namesP->value.firstChildP = addedP->value.firstChildP;
  else
    namesP->lastChild->next = addedP->value.firstChildP;

  namesP->lastChild = addedP->lastChild;

  patchTreeItemAdd(patchTree, path, namesP, NULL);
}



// -----------------------------------------------------------------------------
//
// namesToPatchTree -
//
static void namesToPatchTree(KjNode* patchTree, const char* path, KjNode* namesP, KjNode* addedP, KjNode* removedP)
{
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

  bool added   = false;
  bool removed = false;

  if ((addedP   != NULL) && (addedP->value.firstChildP   != NULL))  added   = true;
  if ((removedP != NULL) && (removedP->value.firstChildP != NULL))  removed = true;

  if      ((added == true)   && (removed == false))   patchTreeItemAdd(patchTree, namesPath, addedP,   "PUSH");
  else if ((added == false)  && (removed == true))    patchTreeItemAdd(patchTree, namesPath, removedP, "PULL");
  else if ((added == true)   && (removed == true))    namesArrayMergeToPatchTree(patchTree, namesPath, namesP, addedP, removedP);
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
  if (newP == NULL)  // Not sure this is ever a possibility ...
    return;

  if (oldP == NULL)  // It's NEW - didn't exist - simply ADD
  {
    patchTreeItemAdd(patchTree, path, newP, NULL);
    return;
  }

  //
  // In JSON-LD, null is expressed in a different way ...
  //
  if (newP->type == KjObject)
  {
    KjNode* typeP = kjLookup(newP, "@type");  // If it's an object, and it has a @type member - might be a JSON-LD null
    if (typeP)
      kjJsonldNullObject(newP, typeP);
  }


  //
  // NULL as RHS means REMOVAL
  //
  if (newP->type == KjNull)
  {
    LM_TMP(("KZ: newP->type == KjNull"));
    patchTreeItemAdd(patchTree, path, newP, "DELETE");
    return;
  }

  if (newP->type != oldP->type)  // Different type?  - overwrite the old
  {
    patchTreeItemAdd(patchTree, path, newP, NULL);
    return;
  }

  bool change = false;
  bool leave  = false;  // We're done if it's a SIMPLE TYPE (String, Number, Bool)

  if      (newP->type == KjString)   { if (strcmp(newP->value.s, oldP->value.s) != 0)  change = true; leave = true; }
  else if (newP->type == KjInt)      { if (newP->value.i != oldP->value.i)             change = true; leave = true; }
  else if (newP->type == KjFloat)    { if (newP->value.f != oldP->value.f)             change = true; leave = true; }
  else if (newP->type == KjBoolean)  { if (newP->value.b != oldP->value.b)             change = true; leave = true; }

  if (change == true)
    patchTreeItemAdd(patchTree, path, newP, NULL);

  if (leave == true)
    return;

  if (newP->type == KjArray)  // If it's an Array, we replace the old array (by definition)
  {
    patchTreeItemAdd(patchTree, path, newP, NULL);
    return;
  }

  // Both are JSON Object - entering to find differences

  //
  // Careful with the linked lists of newP - the recursive calls may invoke patchTreeItemAdd and that REMOVES ITEMS FROM A LIST
  // => Can't use the normal "for" - must be a "while and a next pointer"
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

    orionldEntityPatchTree(oldItemP, newItemP, newPath, patchTree);

    newItemP = next;
  }

  if (namesP != NULL)
    namesToPatchTree(patchTree, path, namesP, addedP, removedP);
}



void dbModelToApiSubAttribute(KjNode* subP)
{
  //
  // Remove unwanted parts of the sub-attribute from DB
  //
  const char* unwanted[] = { "createdAt", "modifiedAt" };

  for (unsigned int ix = 0; ix < K_VEC_SIZE(unwanted); ix++)
  {
    KjNode* nodeP = kjLookup(subP, unwanted[ix]);

    if (nodeP != NULL)
      kjChildRemove(subP, nodeP);
  }
}



void dbModelToApiAttribute(KjNode* attrP)
{
  //
  // Remove unwanted parts of the attribute from DB
  //
  const char* unwanted[] = { "mdNames", "creDate", "modDate" };

  for (unsigned int ix = 0; ix < K_VEC_SIZE(unwanted); ix++)
  {
    KjNode* nodeP = kjLookup(attrP, unwanted[ix]);

    if (nodeP != NULL)
      kjChildRemove(attrP, nodeP);
  }

  KjNode* observedAtP = kjLookup(attrP, "observedAt");
  KjNode* unitCodeP   = kjLookup(attrP, "unitCode");

  if (observedAtP != NULL)
  {
    char* dateTimeBuf = kaAlloc(&orionldState.kalloc, 32);
    numberToDate(observedAtP->value.firstChildP->value.f, dateTimeBuf, 32);
    observedAtP->type      = KjString;
    observedAtP->value.s   = dateTimeBuf;
    observedAtP->lastChild = NULL;
  }

  if (unitCodeP != NULL)
  {
    unitCodeP->type      = KjString;
    unitCodeP->value.s   = unitCodeP->value.firstChildP->value.s;
    unitCodeP->lastChild = NULL;
  }

  //
  // Sub-Attributes
  //
  KjNode* mdP = kjLookup(attrP, "md");
  if (mdP != NULL)
  {
    kjChildRemove(attrP, mdP);  // The content of mdP is added to attrP at the end of the if

    // Special Sub-Attrs: unitCode + observedAt
    for (KjNode* subP = mdP->value.firstChildP; subP != NULL; subP = subP->next)
    {
      if (strcmp(subP->name, "unitCode") == 0)
      {
        subP->type  = subP->value.firstChildP->type;
        subP->value = subP->value.firstChildP->value;
      }
      else if (strcmp(subP->name, "observedAt") == 0)  // Part of Sub-Attribute
      {
        subP->type  = subP->value.firstChildP->type;
        subP->value = subP->value.firstChildP->value;  // + convert to iso8601 string
      }
      else
        dbModelToApiSubAttribute(subP);
    }

    //
    // Move all metadata (sub-attrs) up one level
    //
    attrP->lastChild->next = mdP->value.firstChildP;
    attrP->lastChild       = mdP->lastChild;
  }
}



// ----------------------------------------------------------------------------
//
// orionldPatchEntity2 -
//
bool orionldPatchEntity2(void)
{
  if (experimental == false)
  {
    orionldError(OrionldResourceNotFound, "Service Not Found", orionldState.urlPath, 404);
    return false;
  }

  char*    entityId    = orionldState.wildcard[0];
  KjNode*  dbEntityP;

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

  //
  // FIXME: change pCheckEntity param no 3 to be not only the attributes, but the entire entity (dbEntityP)
  //
  if (pCheckEntity(orionldState.requestTree, false, dbAttrsP) == false)
  {
    LM_W(("Invalid payload body. %s: %s", orionldState.pd.title, orionldState.pd.detail));
    return false;
  }

  //
  // For TRoE we need a tree with all those attributes that have been patched (part of incoming tree)
  // but, with their current value in the database.
  // That tree needs to be trimmed
  //
  if (troe)
  {
    KjNode* troeTree = kjObject(orionldState.kjsonP, NULL);

    for (KjNode* patchedAttrP = orionldState.requestTree->value.firstChildP; patchedAttrP != NULL; patchedAttrP = patchedAttrP->next)
    {
      KjNode* dbAttrP;
      dotForEq(patchedAttrP->name);

      if ((dbAttrP = kjLookup(dbAttrsP, patchedAttrP->name)) != NULL)
      {
        KjNode* troeAttrP = kjClone(orionldState.kjsonP, dbAttrP);

        dbModelToApiAttribute(troeAttrP);
        kjChildAdd(troeTree, troeAttrP);
      }
      eqForDot(patchedAttrP->name);
    }

    orionldState.patchBase = troeTree;
  }

  //
  // FIXME: If I destroy the tree, how do I handle forwarding later ... ?
  //        I extract the part that is to be forwarded from the tree before I destroy
  //        If non-exclusive, might be I both forward and do it locally - kjClone(attrP)
  //
  KjNode* dbAttrNamesP = kjLookup(dbEntityP, "attrNames");
  dbModelFromApiEntity(orionldState.requestTree, dbAttrsP, dbAttrNamesP);

  //
  // Due to the questionable database model of Orion (that Orion-LD has inherited - backwards compatibility),
  // the attributes that have been added/removed must be added/removed to a redundant database field called "attrNames".
  //
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
  orionldState.requestTree->name = NULL;
  orionldEntityPatchTree(dbAttrsObject, orionldState.requestTree, NULL, patchTree);

  bool b = mongocEntityUpdate(entityId, patchTree);  // Added/Removed (sub-)attrs are found as arrays named ".added" and ".removed"
  if (b == false)
  {
    bson_error_t* errP = &orionldState.mongoc.error;  // Can't be in orionldState - DB Dependant!!!

    LM_E(("mongocEntityUpdate(%s): [%d.%d]: %s", entityId, errP->domain, errP->code, errP->message));

    if (errP->code == 12)  orionldError(OrionldResourceNotFound, "Entity not found", entityId, 404);
    else                   orionldError(OrionldInternalError, "Internal Error", errP->message, 500);

    return false;
  }

  orionldState.httpStatusCode = 204;

  if (troe)
    orionldState.requestTree = patchTree;

  return true;
}
