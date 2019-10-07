/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <bson/bson.h>                                         // BSON

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjParse.h"                                     // kjParse
}

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/mongoc/mongocKjTreeFromBson.h"               // Own interface



// -----------------------------------------------------------------------------
//
// mongocKjTreeFromBson -
//
KjNode* mongocKjTreeFromBson(const void* dataP, char** titleP, char** detailsP)
{
  const bson_t*  bsonP  = (const bson_t*) dataP;
  char*          json;
  KjNode*        treeP = NULL;

  if ((json = bson_as_json(bsonP, NULL)) == NULL)
  {
    *titleP   = (char*) "Internal Error";
    *detailsP = (char*) "Error creating JSON from BSON";
  }
  else
  {
    treeP = kjParse(orionldState.kjsonP, json);
    if (treeP == NULL)
    {
      *titleP   = (char*) "Internal Error";
      *detailsP = (char*) "Error parsing JSON output from bson_as_json";
    }
  }

  bson_free(json);

  return treeP;
}
