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
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeToBson.h"                   // mongocKjTreeToBson
#include "orionld/mongoc/mongocEntityUpdate.h"                   // Own interface



// -----------------------------------------------------------------------------
//
// mongocKjTreeToUpdateBson -
//
// { $set: { "a.2": <new value>, "a.10": <new value> }, $push: {}, ... }
//
//
static void mongocKjTreeToUpdateBson(KjNode* tree, bson_t* bsonP, const char* bsonPath, KjNode* attrNamesArray)
{
  bson_t set;
  bson_t unset;
  int    sets    = 0;
  int    unsets  = 0;

  bson_init(&set);    // Probably needs to be parameter to the function
  bson_init(&unset);  // Probably needs to be parameter to the function

  int     ix    = 0;
  char    path[1024];

  for (KjNode* attrP = tree->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    LM_TMP(("KZ: Child %d (%s) is a JSON %s", ix, attrP->name, kjValueType(attrP->type)));

    if      (strcmp(attrP->name, "scope")    == 0)  {}
    else if (strcmp(attrP->name, "location") == 0)  {}
    else
      dotForEq(attrP->name);

    ++sets;
    if (attrP->type == KjObject)
    {
      int jx = 0;

      for (KjNode* subAttrP = attrP->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
      {
        bool isValue = false;
        LM_TMP(("KZ: Sub-attr %d (%s) is a JSON %s", jx, subAttrP->name, kjValueType(subAttrP->type)));

        if      (strcmp(subAttrP->name, "value")     == 0)     { isValue = true; }
//      else if (strcmp(subAttrP->name, "type")      == 0)     { continue; }  - needed only if the attribute did not previously exist
        else if (strcmp(subAttrP->name, "object")    == 0)     { subAttrP->name = (char*) "value"; }
        else if (strcmp(subAttrP->name, "datasetId") == 0)     { }
        else if (strcmp(subAttrP->name, "unitCode")  == 0)     { }
        else if (strcmp(subAttrP->name, "mdNames")   == 0)     { isValue = true; }
        else
          dotForEq(subAttrP->name);

        int pathLen = snprintf(path, sizeof(path), "%s.%s.%s", bsonPath, attrP->name, subAttrP->name);

        if      (subAttrP->type == KjString)   bson_append_utf8(&set,   path, pathLen, subAttrP->value.s, -1);
        else if (subAttrP->type == KjInt)      bson_append_int32(&set,  path, pathLen, subAttrP->value.i);
        else if (subAttrP->type == KjFloat)    bson_append_double(&set, path, pathLen, subAttrP->value.f);
        else if (subAttrP->type == KjBoolean)  bson_append_bool(&set,   path, pathLen, subAttrP->value.b);
        else if (subAttrP->type == KjObject)
        {
          if (isValue)
          {
            bson_t compound;
            mongocKjTreeToBson(subAttrP, &compound);
            bson_append_document(&set, path, pathLen, &compound);
          }
          else
          {
            int    bsonPathLen = strlen(bsonPath) + 1 + strlen(attrP->name) + 1;
            char*  bsonPath2   = kaAlloc(&orionldState.kalloc, bsonPathLen);

            snprintf(bsonPath2, bsonPathLen, "%s.%s", bsonPath, attrP->name);
            mongocKjTreeToUpdateBson(subAttrP, bsonP, bsonPath2, NULL);
            --sets;
          }
        }
        else if (subAttrP->type == KjArray)
        {
          if (isValue)
          {
            bson_t compound;

            mongocKjTreeToBson(subAttrP, &compound);
            bson_append_array(&set, path, pathLen, &compound);
          }
          else
          {
            LM_TMP(("KZ: %s has an array as RHS ...", path));
            --sets;
          }
        }

        ++jx;
      }

      // Set the modifiedAt timestamp for the Attribute
      int pathLen = snprintf(path, sizeof(path), "attrs.%s.modDate", attrP->name);
      bson_append_double(&set, path, pathLen, orionldState.requestTime);
    }
    else if (attrP->type == KjNull)
    {
      int pathLen = snprintf(path, sizeof(path), "attrs.%s", attrP->name);
      bson_append_utf8(&unset, path, pathLen, "", 0);
      ++unsets;
    }

    ++ix;
  }

  // Set the modifiedAt timestamp for the Entity
  bson_append_double(&set, "modDate", 7, orionldState.requestTime);

  // Toplevel attrNames
  if (attrNamesArray != NULL)
  {
    bson_t namesArray;
    bson_init(&namesArray);

    for (KjNode* attrNameP = attrNamesArray->value.firstChildP; attrNameP != NULL; attrNameP = attrNameP->next)
    {
      LM_TMP(("KZ: Adding real (dots) attr name '%s' to the attrNames array", attrNameP->value.s));
      bson_append_utf8(&namesArray, "0", 1, attrNameP->value.s, -1);
    }
    bson_append_array(&set, "attrNames", 9, &namesArray);
  }

  if (sets   > 0) bson_append_document(bsonP, "$set",   4, &set);
  if (unsets > 0) bson_append_document(bsonP, "$unset", 6, &unset);
}



// -----------------------------------------------------------------------------
//
// mongocEntityUpdate -
//
// For now, only PATCH /entities/{entityId} uses this function
//
bool mongocEntityUpdate(const char* entityId, KjNode* requestTree, KjNode* attrNamesArray, int attrNamesChanges)
{
  mongocConnectionGet();

  if (orionldState.mongoc.entitiesP == NULL)
    orionldState.mongoc.entitiesP = mongoc_client_get_collection(orionldState.mongoc.client, orionldState.tenantP->mongoDbName, "entities");

  if (attrNamesChanges == 0)
    attrNamesArray = NULL;

  bson_t selector;
  bson_init(&selector);
  bson_append_utf8(&selector, "_id.id", 6, entityId, -1);

  bson_t request;
  bson_t reply;

  bson_init(&request);
  mongocKjTreeToUpdateBson(requestTree, &request, "attrs", attrNamesArray);

  bool b = mongoc_collection_update_one(orionldState.mongoc.entitiesP, &selector, &request, NULL, &reply, &orionldState.mongoc.error);

  if (b == false)
  {
    bson_error_t* errP = &orionldState.mongoc.error;
    LM_E(("KZ: mongoc error updating entity '%s': [%d.%d]: %s", entityId, errP->domain, errP->code, errP->message));
    return false;
  }

  bson_error_t* errP = &orionldState.mongoc.error;
  LM_TMP(("KZ: mongoc updating entity '%s': [%d.%d]: '%s'", entityId, errP->domain, errP->code, errP->message));

  //
  // FIXME:  MUST go inside 'reply' and make sure it worked
  //
  // Should look something like this:
  //   { "modifiedCount" : { "$numberInt" : "1" }, "matchedCount" : { "$numberInt" : "1" }, "upsertedCount" : { "$numberInt" : "0" } }
  //
  // char* s = bson_as_canonical_extended_json(&reply, NULL);
  // LM_TMP(("KZ: reply: %s", s));

  return true;
}
