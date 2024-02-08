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
#include <bson/bson.h>                                           // bson_t, ...
#include <mongoc/mongoc.h>                                       // MongoDB C Client Driver

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeToBson.h"                   // mongocKjTreeToBson
#include "orionld/mongoc/mongocRegistrationReplace.h"            // Own interface



// -----------------------------------------------------------------------------
//
// mongocRegistrationReplace -
//
bool mongocRegistrationReplace(const char* registrationId, KjNode* dbRegistrationP)
{
  bson_t selector;
  bson_t replacement;
  bson_t reply;

  mongocConnectionGet(orionldState.tenantP, DbRegistrations);

  bson_init(&selector);
  bson_init(&replacement);
  bson_init(&reply);

  bson_append_utf8(&selector, "_id", 3, registrationId, -1);

  mongocKjTreeToBson(dbRegistrationP, &replacement);

  bool b = mongoc_collection_replace_one(orionldState.mongoc.registrationsP, &selector, &replacement, NULL, &reply, &orionldState.mongoc.error);
  if (b == false)
  {
    bson_error_t* errP = &orionldState.mongoc.error;
    LM_E(("mongoc error replacing registration '%s': [%d.%d]: %s", registrationId, errP->domain, errP->code, errP->message));
  }

  // mongocConnectionRelease(); - Not here - done at the end of the request

  bson_destroy(&selector);
  bson_destroy(&replacement);
  bson_destroy(&reply);

  return b;
}
