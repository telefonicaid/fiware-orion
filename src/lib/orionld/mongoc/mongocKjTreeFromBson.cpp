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
#include <bson/bson.h>                                         // BSON

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjParse.h"                                     // kjParse
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

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

#if 0
  // FIXME: Real implementation - avoiding the extra step going via JSON
#else
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

    // bson_free(json);
  }
#endif
  return treeP;
}
