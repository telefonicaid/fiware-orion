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
#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState, dbName
#include "orionld/db/dbCollectionPathGet.h"                      // Own interface
  


// ----------------------------------------------------------------------------
//
// dbCollectionPathGet -
//
int dbCollectionPathGet(char* path, int pathLen, const char* collection)
{
  int dbNameLen     = strlen(dbName);          // Can be faster - global variable can keep this constant
  int tenantLen     = ((multitenancy == true) && (orionldState.tenant != NULL) && (orionldState.tenant[0] != 0))? strlen(orionldState.tenant) + 1 : 0;
  int collectionLen = strlen(collection) + 1;  // +1: '.'

  if (dbNameLen + tenantLen + collectionLen >= pathLen)
  {
    LM_E(("Internal Error (database name is too long)"));
    return -1;
  }
    
  strcpy(path, dbName);

  LM_TMP(("DB: dbName:     '%s'", dbName));
  LM_TMP(("DB: tenant:     '%s'", (orionldState.tenant != NULL)? orionldState.tenant: "NULL"));
  LM_TMP(("DB: collection: '%s'", collection));

  if (tenantLen != 0)
  {
    path[dbNameLen] = '-';
    strcpy(&path[dbNameLen + 1], orionldState.tenant);
  }

  path[dbNameLen + tenantLen] = '.';
  strcpy(&path[dbNameLen + tenantLen + 1], collection);

  LM_TMP(("DB: collection path: %s", path));

  return 0;
}
