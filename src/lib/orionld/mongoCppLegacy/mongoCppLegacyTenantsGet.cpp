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
#include <string.h>                                            // strdup
#include <string>                                              // std::string
#include <vector>                                              // std::vector

#include "mongo/client/dbclient.h"                             // mongo legacy driver

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "mongoBackend/MongoGlobal.h"                          // getMongoConnection

#include "orionld/common/orionldState.h"                       // orionldState, dbName, dbNameLen
#include "orionld/common/orionldTenantCreate.h"                // orionldTenantCreate
#include "orionld/common/orionldTenantLookup.h"                // orionldTenantLookup
#include "orionld/mongoCppLegacy/mongoCppLegacyDbStringFieldGet.h"   // mongoCppLegacyDbStringFieldGet
#include "orionld/mongoCppLegacy/mongoCppLegacyDbFieldGet.h"   // mongoCppLegacyDbFieldGet
#include "orionld/mongoCppLegacy/mongoCppLegacyTenantsGet.h"   // Own interface



// -----------------------------------------------------------------------------
//
// mongoCppLegacyTenantsGet -
//
// Get all databases whose names start with '$dbPrefix'
//
bool mongoCppLegacyTenantsGet(void)
{
  mongo::DBClientBase* connection = getMongoConnection();

  try
  {
    mongo::BSONObj  result;
    mongo::BSONObj  command = BSON("listDatabases" << 1);

    connection->runCommand("admin", command, result);

    mongo::BSONElement              bsonElement;
    std::vector<mongo::BSONElement> dbV;

    if (mongoCppLegacyDbFieldGet(&result, "databases", &bsonElement) == false)
    {
      LM_E(("Database Error (mongoCppLegacyDbFieldGet('databases') failed)"));
      return false;
    }
    dbV = bsonElement.Array();

    for (unsigned int ix = 0; ix < dbV.size(); ix++)
    {
      mongo::BSONObj  db   = dbV[ix].Obj();
      char*           name = mongoCppLegacyDbStringFieldGet(&db, "name");

      LM_TMP(("TENANT: Got a mongo database named '%s'", name));
      if (strncmp(name, dbName, dbNameLen) == 0)
      {
        // The prefix matches, now EOS or '-'
        LM_TMP(("TENANT: first letter after prefix is: '%c' (0x%x)", name[dbNameLen], name[dbNameLen]));
        if (name[dbNameLen] == 0)  // the base
        {
          LM_TMP(("TENANT: it's the base: '%s'", name));
        }
        else if (name[dbNameLen] != '-')  // not a valid tenant (orionld, for example)
        {
          LM_TMP(("TENANT: it's not a DB for the broker (no hyphen after dbPrefix): '%s'", name));
        }
        else if (name[dbNameLen + 1] == 0)
        {
          LM_TMP(("TENANT: invalid database name (hyphen there but no tenant): '%s'", name));
        }
        else
        {
          char* tenantName = &name[dbNameLen + 1];

          if (orionldTenantLookup(tenantName) == NULL)
          {
            LM_TMP(("TENANT: calling orionldTenantCreate(%s)", tenantName));
            orionldTenantCreate(tenantName);
          }
        }
      }
      else
        LM_TMP(("TENANT: it's not a DB for the broker (doesn't start with dbPrefix): '%s'", name));
    }
  }
  catch (const std::exception &e)
  {
    LM_E(("Database Error (listDatabases: %s)", e.what()));
    return false;
  }
  catch (...)
  {
    LM_E(("Database Error (listDatabases: %s)", "generic exception"));
    return false;
  }

  return true;
}
