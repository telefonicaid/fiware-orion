#ifndef SRC_LIB_ORIONLD_MONGOC_MONGOCENTITYLOOKUP_H_
#define SRC_LIB_ORIONLD_MONGOC_MONGOCENTITYLOOKUP_H_

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

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "orionld/types/StringArray.h"                           // StringArray



// -----------------------------------------------------------------------------
//
// mongocEntityLookup -
//
// This function, that seems quite similar to mongocEntityRetrieve, is used by:
//   * orionldPutEntity    - Uses creDate + attrs to make sure no attr types are modified
//   * orionldPatchEntity  - Like PUT but also using entity type, entire attributes, etc.
//   * orionldPostEntities - Only to make sure the entity does not already exists (mongocEntityExists should be implemented and used instead)
//   * orionldPostEntity   - The entire DB Entity is needed as it is later used as base for the "merge" with the payload body
//
// So, this function is QUITE NEEDED, just as it is.
// The other one, mongocEntityRetrieve, does much more than just DB. It should be REMOVED.
// mongocEntityRetrieve is only used by legacyGetEntity() which is being deprecated anyway.
//
extern KjNode* mongocEntityLookup(const char* entityId, const char* entityType, StringArray* attrsV, const char* geojsonGeometry);

#endif  // SRC_LIB_ORIONLD_MONGOC_MONGOCENTITYLOOKUP_H_
