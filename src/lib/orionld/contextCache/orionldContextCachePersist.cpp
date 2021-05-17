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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjFloat, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/uuidGenerate.h"                         // uuidGenerate
#include "orionld/context/orionldContextUrlGenerate.h"           // orionldContextUrlGenerate
#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/mongoc/mongocContextCachePersist.h"            // mongocContextCachePersist - FIXME: Use dbContextCachePersist
#include "orionld/contextCache/orionldContextCachePersist.h"     // Own interface



extern const char* originName(OrionldContextOrigin origin);  // FIXME: own module!
// -----------------------------------------------------------------------------
//
// orionldContextCachePersist -
//
// In  mongo, a context looks like this:
// {
//   "_id":       "uuid-invented-by-broker",
//   "url":       "URL to the context",
//   "parent":    "ID of parent context"  # If parent is deleted, so am I
//   "origin":    "Downloaded|..."
//   "createdAt": "ISO8601 timestamp"
//   "value":     JSON Array|Object representation of the VALUE of the @context (i.e. NOT containing the @context member)
// }
//
void orionldContextCachePersist(OrionldContext* contextP)
{
  KjNode*  contextObjP  = kjObject(orionldState.kjsonP, NULL);
  KjNode*  idP;
  KjNode*  urlP         = kjString(orionldState.kjsonP, "url",       contextP->url);
  KjNode*  parentP;
  KjNode*  originP      = kjString(orionldState.kjsonP, "origin",    originName(contextP->origin));
  KjNode*  createdAtP   = kjFloat(orionldState.kjsonP,  "createdAt", orionldState.requestTime);  // FIXME: make sure it's not overwritten if already existss
  KjNode*  valueP       = contextP->tree;

  if (contextP->tree == NULL)
    LM_X(1, ("FATAL ERROR: the context '%s' has no tree!", contextP->url));

  // Field: "_id"
  if (contextP->id == NULL)
    orionldContextUrlGenerate(&contextP->id);

  idP = kjString(orionldState.kjsonP, "_id", contextP->id);
  kjChildAdd(contextObjP, idP);

  // Field: "url"
  kjChildAdd(contextObjP, urlP);

  // Field: "parent"
  if (contextP->parent != NULL)
    parentP = kjString(orionldState.kjsonP, "parent", contextP->parent);
  else
    parentP = kjNull(orionldState.kjsonP, "parent");
  kjChildAdd(contextObjP, parentP);

  // Field: "origin"
  kjChildAdd(contextObjP, originP);

  // Field: "createdAt"
  kjChildAdd(contextObjP, createdAtP);

  // Field: "value"
  valueP->name = (char*) "value";
  kjChildAdd(contextObjP, valueP);

  mongocContextCachePersist(contextObjP);
}
