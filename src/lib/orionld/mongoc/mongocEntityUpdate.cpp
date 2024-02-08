/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include <bson/bson.h>                                           // bson_t, ...
#include <mongoc/mongoc.h>                                       // MongoDB C Client Driver

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjRender.h"                                      // kjFastRender
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/mongoc/mongocWriteLog.h"                       // MONGOC_WLOG
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeToBson.h"                   // mongocKjTreeToBson
#include "orionld/mongoc/mongocIndexString.h"                    // mongocIndexString
#include "orionld/mongoc/mongocEntityUpdate.h"                   // Own interface



// -----------------------------------------------------------------------------
//
// mongocKjTreeToUpdateBson -
//
// { $set: { "a.2": <new value>, "a.10": <new value> }, $push: {}, ... }
//
//
static void mongocKjTreeToUpdateBson
(
  KjNode*      tree,
  bson_t*      setP,
  bson_t*      unsetP,
  const char*  bsonPath,
  KjNode*      attrsAdded,
  KjNode*      attrsRemoved,
  bool*        nullsP,
  int          level
)
{
  char path[1024];

  for (KjNode* attrP = tree->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if      (strcmp(attrP->name, "scope")      == 0)  {}
    else if (strcmp(attrP->name, "location")   == 0)  {}
    else if (strcmp(attrP->name, ".attrNames") == 0)  { continue; }
    else
      dotForEq(attrP->name);

    if (attrP->type == KjObject)
    {
      int jx = 0;

      for (KjNode* subAttrP = attrP->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
      {
        bool isValue = false;

        if      (strcmp(subAttrP->name, "value")     == 0)     { isValue = true; }
//      else if (strcmp(subAttrP->name, "type")      == 0)     { continue; }  - needed only if the attribute did not previously exist
        else if (strcmp(subAttrP->name, "object")    == 0)     { subAttrP->name = (char*) "value"; }
        else if (strcmp(subAttrP->name, "datasetId") == 0)     { }
        else if (strcmp(subAttrP->name, "unitCode")  == 0)     { }
        else if (strcmp(subAttrP->name, "mdNames")   == 0)     { isValue = true; }
        else
          dotForEq(subAttrP->name);

        int pathLen = snprintf(path, sizeof(path), "%s.%s.%s", bsonPath, attrP->name, subAttrP->name);

        if      (subAttrP->type == KjString)   bson_append_utf8(setP,   path, pathLen, subAttrP->value.s, -1);
        else if (subAttrP->type == KjInt)      bson_append_int32(setP,  path, pathLen, subAttrP->value.i);
        else if (subAttrP->type == KjFloat)    bson_append_double(setP, path, pathLen, subAttrP->value.f);
        else if (subAttrP->type == KjBoolean)  bson_append_bool(setP,   path, pathLen, subAttrP->value.b);
        else if (subAttrP->type == KjObject)
        {
          if (isValue)
          {
            bson_t compound;
            LM_T(LmtMongoc, ("Member '%s' has a value that is a compound object - calling mongocKjTreeToBson", subAttrP->name));
            mongocKjTreeToBson(subAttrP, &compound);
            bson_append_document(setP, path, pathLen, &compound);
          }
          else
          {
            int    bsonPathLen = strlen(bsonPath) + 1 + strlen(attrP->name) + 1;
            char*  bsonPath2   = kaAlloc(&orionldState.kalloc, bsonPathLen);

            LM_T(LmtMongoc, ("Member '%s' has a value that is a simple value - calling mongocKjTreeToUpdateBson", subAttrP->name));
            snprintf(bsonPath2, bsonPathLen, "%s.%s", bsonPath, attrP->name);
            mongocKjTreeToUpdateBson(subAttrP, setP, unsetP, bsonPath2, NULL, NULL, NULL, level + 1);  // Perhaps another function for sub-attributes ...?
          }
        }
        else if (subAttrP->type == KjArray)
        {
          if (isValue)
          {
            bson_t compound;

            LM_T(LmtMongoc, ("Array member '%s' is a compound object - calling mongocKjTreeToBson", subAttrP->name));
            mongocKjTreeToBson(subAttrP, &compound);
            bson_append_array(setP, path, pathLen, &compound);
          }
          else
            LM_T(LmtMongoc, ("Wait, what???"));
        }

        ++jx;
      }

      // Set the modifiedAt timestamp for the Attribute
      int pathLen = snprintf(path, sizeof(path), "attrs.%s.modDate", attrP->name);
      bson_append_double(setP, path, pathLen, orionldState.requestTime);
    }
    else if (attrP->type == KjNull)
    {
      int pathLen = snprintf(path, sizeof(path), "attrs.%s", attrP->name);
      bson_append_utf8(unsetP, path, pathLen, "", 0);
      *nullsP = true;
    }
  }

  // Set the modifiedAt timestamp for the Entity
  bson_append_double(setP, "modDate", 7, orionldState.requestTime);
}



// -----------------------------------------------------------------------------
//
// patchApply -
//
static bool patchApply
(
  KjNode*  patchTree,
  bson_t*  setP,
  bson_t*  unsetP,
  int*     unsetsP,
  bson_t*  pullP,
  int*     pullsP,
  bson_t*  pushP,
  int*     pushesP
)
{
  for (KjNode* patchObject = patchTree->value.firstChildP; patchObject != NULL; patchObject = patchObject->next)
  {
    KjNode* pathNode  = kjLookup(patchObject, "PATH");
    KjNode* tree      = kjLookup(patchObject, "TREE");
    KjNode* op        = kjLookup(patchObject, "op");
    char*   path      = pathNode->value.s;
    bson_t  compound;

    if (op != NULL)
    {
      if (strcmp(op->value.s, "PULL") == 0)
      {
        // if "op" == "PULL", then "tree" is an array of strings
        // { $pull: { fruits: { $in: [ "apples", "oranges" ] }}
        bson_t inBson;
        bson_t commaArrayBson;

        bson_append_document_begin(pullP, path, -1,  &inBson);
        bson_append_array_begin(&inBson, "$in", -1, &commaArrayBson);

        int ix = 0;
        for (KjNode* itemNameP = tree->value.firstChildP; itemNameP != NULL; itemNameP = itemNameP->next)
        {
          char buf[16];
          int  bufLen = mongocIndexString(ix, buf);

          bson_append_utf8(&commaArrayBson, buf, bufLen, itemNameP->value.s, -1);
          ++ix;
        }
        *pullsP += ix;

        bson_append_array_end(&inBson, &commaArrayBson);
        bson_append_document_end(pullP, &inBson);
      }
      else if (strcmp(op->value.s, "PUSH") == 0)
      {
        // if "op" == "PUSH", then "tree" is an array of strings
        // { $push: { fruits: { $in: [ "apples", "oranges" ] }}
        bson_t eachBson;
        bson_t commaArrayBson;

        bson_append_document_begin(pushP, path, -1,  &eachBson);
        bson_append_array_begin(&eachBson, "$each", -1, &commaArrayBson);

        int ix = 0;
        for (KjNode* itemNameP = tree->value.firstChildP; itemNameP != NULL; itemNameP = itemNameP->next)
        {
          char buf[16];
          int  bufLen = mongocIndexString(ix, buf);

          LM_T(LmtMongoc, ("Array Index: '%s'", buf));
          bson_append_utf8(&commaArrayBson, buf, bufLen, itemNameP->value.s, -1);
          ++ix;
        }
        *pushesP += ix;

        bson_append_array_end(&eachBson, &commaArrayBson);
        bson_append_document_end(pushP, &eachBson);
      }
      else if (strcmp(op->value.s, "DELETE") == 0)
      {
        bson_append_utf8(unsetP, path, -1, "", 0);
        *unsetsP += 1;
      }
    }
    else if (tree->type == KjNull)
    {
      bson_append_utf8(unsetP, path, -1, "", 0);
      *unsetsP += 1;
    }
    else if (tree->type == KjString)   bson_append_utf8(setP,   path, -1, tree->value.s, -1);
    else if (tree->type == KjInt)      bson_append_int32(setP,  path, -1, tree->value.i);
    else if (tree->type == KjFloat)    bson_append_double(setP, path, -1, tree->value.f);
    else if (tree->type == KjBoolean)  bson_append_bool(setP,   path, -1, tree->value.b);
    else if (tree->type == KjArray)
    {
      LM_T(LmtMongoc, ("'%s' is an Array - calling mongocKjTreeToBson", tree->name));
      kjTreeLog(tree, "TREE", LmtMongoc);
      mongocKjTreeToBson(tree, &compound);
      bson_append_array(setP, path, -1, &compound);
      bson_destroy(&compound);
    }
    else if (tree->type == KjObject)
    {
      LM_T(LmtMongoc, ("'%s' is an Object - calling mongocKjTreeToBson", tree->name));
      mongocKjTreeToBson(tree, &compound);
      bson_append_document(setP, path, -1, &compound);
      bson_destroy(&compound);
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// mongocEntityUpdate -
//
// For now, only PATCH /entities/{entityId} uses this function
//
bool mongocEntityUpdate(const char* entityId, KjNode* patchTree)
{
  mongocConnectionGet(orionldState.tenantP, DbEntities);

  bson_t selector;
  bson_init(&selector);
  bson_append_utf8(&selector, "_id.id", 6, entityId, -1);

  bson_t request;
  bson_t reply;
  bson_t set;    // No counter needed - there's always at least one 'set' - modDate on the Entity
  bson_t unset;
  bson_t pull;
  bson_t push;
  int    unsets  = 0;
  int    pulls   = 0;
  int    pushes  = 0;

  bson_init(&request);
  bson_init(&reply);
  bson_init(&set);
  bson_init(&unset);
  bson_init(&pull);
  bson_init(&push);

  patchApply(patchTree, &set, &unset, &unsets, &pull, &pulls, &push, &pushes);

  bson_append_document(&request, "$set", 4, &set);

  if (unsets > 0)    bson_append_document(&request, "$unset", 6, &unset);
  if (pulls  > 0)    bson_append_document(&request, "$pull",  5, &pull);
  if (pushes > 0)    bson_append_document(&request, "$push",  5, &push);

  MONGOC_WLOG("PATCH Entity", orionldState.tenantP->mongoDbName, "entities", &selector, &request, LmtMongoc);
  bool b = mongoc_collection_update_one(orionldState.mongoc.entitiesP, &selector, &request, NULL, &reply, &orionldState.mongoc.error);
  if (b == false)
  {
    bson_error_t* errP = &orionldState.mongoc.error;
    LM_E(("mongoc error updating entity '%s': [%d.%d]: %s", entityId, errP->domain, errP->code, errP->message));
  }

  // bson_error_t* errP = &orionldState.mongoc.error;
  //
  // FIXME:  MUST go inside 'reply' and make sure it worked
  //
  // Should look something like this:
  //   { "modifiedCount" : { "$numberInt" : "1" }, "matchedCount" : { "$numberInt" : "1" }, "upsertedCount" : { "$numberInt" : "0" } }
  //
  // char* s = bson_as_canonical_extended_json(&reply, NULL);

  // mongocConnectionRelease(); - done at the end of the request

  bson_destroy(&request);
  bson_destroy(&reply);
  bson_destroy(&set);

  if (unsets > 0) bson_destroy(&unset);
  if (pulls  > 0) bson_destroy(&pull);
  if (pushes > 0) bson_destroy(&push);

  return b;
}
