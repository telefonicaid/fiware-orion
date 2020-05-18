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
#include <string>                                                 // std::string

extern "C"
{
#include "kalloc/kaAlloc.h"                                       // kaAlloc
#include "kalloc/kaStrdup.h"                                      // kaStrdup
}

#include "logMsg/logMsg.h"                                        // LM_*
#include "logMsg/traceLevels.h"                                   // Lmt*

#include "mongoBackend/connectionOperations.h"                    // collectionCreateIndex

#include "orionld/db/dbCollectionPathGet.h"                       // dbCollectionPathGet
#include "orionld/db/dbGeoIndexAdd.h"                             // dbGeoIndexAdd
#include "orionld/common/dotForEq.h"                              // dotForEq
#include "orionld/mongoCppLegacy/mongoCppLegacyGeoIndexCreate.h"  // Own interface



// -----------------------------------------------------------------------------
//
// mongoCppLegacyGeoIndexCreate -
//
bool mongoCppLegacyGeoIndexCreate(const char* tenant, const char* attrLongName)
{
  int         len          = 6 + strlen(attrLongName) + 6 + 1;              // "attrs." == 6, ".value" == 6, 1 for string-termination
  char*       index        = kaAlloc(&orionldState.kalloc, len);
  char*       attrNameCopy = kaStrdup(&orionldState.kalloc, attrLongName);  // To not destroy the original attrName
  std::string err;

  dotForEq(attrNameCopy);
  snprintf(index, len, "attrs.%s.value", attrNameCopy);

  char collectionPath[256];
  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities");

  if (collectionCreateIndex(collectionPath, BSON(index << "2dsphere"), false, &err) == false)
  {
    LM_E(("Database Error (error creating 2dsphere index for attribute '%s' for tenant '%s')", attrNameCopy, tenant));
    return false;
  }

  dbGeoIndexAdd(tenant, attrNameCopy);

  return true;
}
