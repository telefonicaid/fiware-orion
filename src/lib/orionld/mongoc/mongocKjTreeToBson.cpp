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
#include "logMsg/traceLevels.h"                                  // Lmt*



// -----------------------------------------------------------------------------
//
// kjTreeToBson -
//
static void kjTreeToBson(KjNode* nodeP, bson_t* parentP, bool inArray)
{
  int         slen = (inArray == true)? 0 : -1;
  const char* name = (inArray == true)? "" : nodeP->name;

  if      (nodeP->type == KjString)    bson_append_symbol(parentP, name, slen, nodeP->value.s, -1);
  else if (nodeP->type == KjInt)       bson_append_int32(parentP, name, slen, nodeP->value.i);
  else if (nodeP->type == KjFloat)     bson_append_double(parentP, name, slen, nodeP->value.f);
  else if (nodeP->type == KjBoolean)   bson_append_bool(parentP, name, slen, nodeP->value.b);
  else if (nodeP->type == KjNull)      bson_append_null(parentP, name, slen);
  else if (nodeP->type == KjObject)
  {
    bson_t bObject;

    bson_append_document_begin(parentP, name, slen, &bObject);
    for (KjNode* itemP = nodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
    {
      kjTreeToBson(itemP, &bObject, false);
    }
    bson_append_document_end(parentP, &bObject);
  }
  else if (nodeP->type == KjArray)
  {
    bson_t bArray;

    bson_append_array_begin(parentP, name, slen, &bArray);
    for (KjNode* itemP = nodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
    {
      kjTreeToBson(itemP, &bArray, true);
    }
    bson_append_array_end(parentP, &bArray);
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

  for (KjNode* itemP = treeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    kjTreeToBson(itemP, bsonP, inArray);
  }
}
