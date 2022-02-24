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



// -----------------------------------------------------------------------------
//
// dbModelEntity -
//
// Perhaps the "attrs" member should be added in here after all - make the PATCH op easier
//
bool dbModelEntity(KjNode* tree, KjNode* dbAttrs)
{
  KjNode*      nodeP;
  const char*  go[] = { "id", "@id", "type", "@type", "scope", "createdAt", "modifiedAt" };

  // Remove any non-attribute nodes
  for (unsigned int ix = 0; ix < sizeof(go) / sizeof(go[0]); ix++)
  {
    nodeP = kjLookup(tree, go[ix]);
    if (nodeP != NULL)
      kjChildRemove(tree, nodeP);
  }

  // "modDate" is given to entity + all attributes in dbEntityUpdate (mongocEntityUpdate)

  // "creDate" is given here (only for attrs that did not previously exist)
  for (KjNode* attrP = tree->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    char*   eqName = kaStrdup(&orionldState.kalloc, attrP->name);
    dotForEq(eqName);
    KjNode* dbAttrP = kjLookup(dbAttrs, eqName);

    if (dbAttrP != NULL)  // Attribute already exists
      continue;

    KjNode* createdAtP   = kjFloat(orionldState.kjsonP, "creDate", orionldState.requestTime);
    KjNode* mdNamesP     = kjArray(orionldState.kjsonP, "mdNames");

    kjChildAdd(attrP, createdAtP);
    kjChildAdd(attrP, mdNamesP);
    LM_TMP(("KZ: Added 'creDate' and 'mdNames' for '%s' (%p %p)", attrP->name, createdAtP, mdNamesP));
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// attrNamePush -
//
static void attrNamePush(KjNode* attrNamesV, const char* name)
{
  if (attrNamesV == NULL)
    return;

  char dotName[256];

  strncpy(dotName, name, sizeof(dotName) - 1);
  eqForDot(dotName);

  KjNode* stringP = kjString(orionldState.kjsonP, NULL, dotName);

  kjChildAdd(attrNamesV, stringP);
}



// -----------------------------------------------------------------------------
//
// patchEntity -
//
// Compare node per node:
// - Foreach item in old:
//   - Lookup in new:
//     - if item exists in old but not in new it is kept
//     - if item exists in new but not in old it is added
//     - it item exists in both old and new:
//       - if new's value is null             - REMOVE item
//       - If JSON type of new item is Simple - new REPLACES old item
//       - if items JSON type differ          - new REPLACES old item
//       - if Array                           - new REPLACES old item
//       - If Object (both of them)           - recursive call
//
// Might be a good idea to sort both trees before starting with the processing ...
//
// "attrNames" (level 0) and "mdNames" (level 2) are special arrays - what's in new shall be added to old
//
// PARAMETERS
//   level              0: attributes, 1: sub-attrs
//
KjNode* patchEntity(KjNode* oldP, KjNode* newP, int level, KjNode* attrNamesV, int* attrNamesChangesP)
{
  KjNode* oldItemP = oldP->value.firstChildP;
  KjNode* next;
  int     added   = 0;
  int     removed = 0;

  while (oldItemP != NULL)
  {
    next = oldItemP->next;

    //
    // Looking up the old item in the new.
    //
    // The name in the database (old item) has been "dotForEq"'ed and for the lookup to work, we need to reverse that.
    //
    char oldItemName[256];  // Hope 256 is enough ...
    strncpy(oldItemName, oldItemP->name, sizeof(oldItemName) - 1);
    eqForDot(oldItemName);
    KjNode* newItemP = kjLookup(newP, oldItemName);

    if (newItemP == NULL)    // We keep the old
    {
      LM_TMP(("KZ: Keeping old item '%s' (no new item)", oldItemP->name));

      //
      // However, keeping the old in the DB does NOT mean we need it in the tree.
      // If not in the tree, it will not be touched by mongocEntityUpdate.
      // No need to overwrite what we already have, especially, we'd modify modifiedAt if we did.
      // So, the old item needs to be removed from the tree
      //
      kjChildRemove(oldP, oldItemP);

      attrNamePush(attrNamesV, oldItemP->name);
    }
    else if (newItemP->type == KjNull)
    {
      LM_TMP(("KZ: Removing old item '%s' (new item is KjNull)", oldItemP->name));

      // An item is removed from the DB (by mongocEntityUpdate) by setting the value of the item to null
      oldItemP->type = KjNull;
      ++removed;
    }
    else if ((newItemP->type != KjObject) || (oldItemP->type != KjObject))
    {
      LM_TMP(("KZ: Replacing old item '%s' with new item", oldItemP->name));
      oldItemP->type       = newItemP->type;
      oldItemP->value      = newItemP->value;
      oldItemP->lastChild  = newItemP->lastChild;  // If Array
      attrNamePush(attrNamesV, oldItemP->name);
    }
    else
    {
      attrNamePush(attrNamesV, oldItemP->name);
      patchEntity(oldItemP, newItemP, level + 1, NULL, NULL);
    }

    if (newItemP != NULL)
      kjChildRemove(newP, newItemP);

    oldItemP = next;
  }

  // Any remaining items in newP are now added to oldP
  if (newP->value.firstChildP != NULL)
  {
    oldP->lastChild->next = newP->value.firstChildP;
    oldP->lastChild       = newP->lastChild;

    for (KjNode* newItemP = newP->value.firstChildP; newItemP != NULL; newItemP = newItemP->next)
    {
      attrNamePush(attrNamesV, newItemP->name);
      ++added;
    }
  }

  if (attrNamesChangesP != NULL)
    *attrNamesChangesP = removed + added;

  return oldP;
}



// ----------------------------------------------------------------------------
//
// orionldPatchEntity2 -
//
// For all this Real PATCH thingy to work for any API operation, we will need the following
//
//   * mongocEntitiesGet() - returning entities as they are in the DB - with "attrs", "attrNames", and "$datasets" children as objects of the output array
//   * patchEntity(KjNode* dbEntity, KjNode* patchEntity, bson_t* toSet, bson_t* toUnset)
//   * patchAttribute(KjNode* dbAttribute, KjNode* patchAttribute, KjNode* dbAttrNamesArray, bson_t* toSet, bson_t* toUnset)
//   * patchSubAttribute(KjNode* dbSubAttribute, KjNode* patchSubAttribute, KjNode* dbMdNamesArray, char* dbPath, bson_t* toSet, bson_t* toUnset)
//
//
//
//
bool orionldPatchEntity2(void)
{
  char*   entityId   = orionldState.wildcard[0];
  KjNode* dbEntity   = mongocEntityLookup(entityId);

  // <DEBUG>
  LM_TMP(("KZ: First level members of the Entity '%s'", entityId));
  // <DEBUG>

  if (dbEntity == NULL)
  {
    orionldError(OrionldResourceNotFound, "Entity does not exist", entityId, 404);
    return false;
  }

  // FIXME: Add some header to warn about this request not being finished?
  KjNode* dbAttrsP = kjLookup(dbEntity, "attrs");
  LM_TMP(("KZ: Calling pCheckEntity with dbAttrsP at %p", dbAttrsP));
  if (pCheckEntity(orionldState.requestTree, false, dbAttrsP) == false)
  {
    LM_W(("Invalid payload body. %s: %s", orionldState.pd.title, orionldState.pd.detail));
    return false;
  }

  KjNode* dbAttrNames = kjArray(orionldState.kjsonP, NULL);
  int     changes;

  dbModelEntity(orionldState.requestTree, dbAttrsP);

  char buf[4096];
  kjFastRender(dbAttrsP, buf);
  LM_TMP(("old: %s", buf));
  kjFastRender(orionldState.requestTree, buf);
  LM_TMP(("new: %s", buf));

  KjNode* patchTree = patchEntity(dbAttrsP, orionldState.requestTree, 0, dbAttrNames, &changes);

  kjFastRender(patchTree, buf);
  LM_TMP(("PATCHED Entity: %s", buf));

  bool b = mongocEntityUpdate(entityId, patchTree, dbAttrNames, changes);
  if (b == false)
  {
    bson_error_t* errP = &orionldState.mongoc.error;

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
