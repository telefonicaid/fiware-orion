/*
*
* Copyright 2021 FIWARE Foundation e.V.
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

extern "C"
{
#include "kbase/kMacros.h"                                       // K_FT
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*



// -----------------------------------------------------------------------------
//
// kjTreeToBson -
//
static void kjTreeToBson(KjNode* nodeP, bson_t* parentP, bool inArray, int arrayIndex = 0)
{
  char  nameBuf[16];
  int   slen;
  char* name;

  //
  // Fields whose names start with dot are "help fields" and are not to be taken into account
  //
  if ((nodeP->name != NULL) && (nodeP->name[0] == '.'))
    return;

  if (inArray == true)
  {
    slen = snprintf(nameBuf, sizeof(nameBuf), "%d", arrayIndex);
    name = nameBuf;
    // LM_T(LmtMongoc, ("Array Index: '%s'", name));
  }
  else
  {
    name = nodeP->name;
    slen = strlen(name);
    // LM_T(LmtMongoc, ("Name: '%s'", name));
  }

  if      (nodeP->type == KjString)    bson_append_utf8(parentP, name, slen, nodeP->value.s, -1);
  else if (nodeP->type == KjInt)       bson_append_int32(parentP, name, slen, nodeP->value.i);
  else if (nodeP->type == KjFloat)     bson_append_double(parentP, name, slen, nodeP->value.f);
  else if (nodeP->type == KjBoolean)   bson_append_bool(parentP, name, slen, nodeP->value.b);
  else if (nodeP->type == KjNull)      bson_append_null(parentP, name, slen);
  else if (nodeP->type == KjObject)
  {
    bson_t bObject;

    bson_init(&bObject);
    bson_append_document_begin(parentP, name, slen, &bObject);

    for (KjNode* itemP = nodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
    {
      // LM_T(LmtMongoc, ("Calling kjTreeToBson for item '%s' of container '%s'", itemP->name, nodeP->name));
      kjTreeToBson(itemP, &bObject, false);
    }
    bson_append_document_end(parentP, &bObject);
    bson_destroy(&bObject);
  }
  else if (nodeP->type == KjArray)
  {
    bson_t bArray;

    bson_init(&bArray);
    bson_append_array_begin(parentP, name, slen, &bArray);
    for (KjNode* itemP = nodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
    {
      // LM_T(LmtMongoc, ("Calling kjTreeToBson for array item %d of '%s'", arrayIndex, nodeP->name));
      kjTreeToBson(itemP, &bArray, true, arrayIndex);
      arrayIndex += 1;
    }
    bson_append_array_end(parentP, &bArray);
    bson_destroy(&bArray);
  }
}



// ----------------------------------------------------------------------------
//
// mongocKjTreeToBson -
//
void mongocKjTreeToBson(KjNode* treeP, bson_t* bsonP)
{
  bool inArray = (treeP->type == KjArray);

  bson_init(bsonP);

  int arrayIx = 0;
  for (KjNode* itemP = treeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    // LM_T(LmtMongoc, ("Calling kjTreeToBson for '%s' of '%s'", itemP->name, treeP->name));
    kjTreeToBson(itemP, bsonP, inArray, arrayIx);

    if (inArray == true)
      ++arrayIx;
  }
}
