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
#include <string>                                                 // std::string

#include "logMsg/logMsg.h"                                        // LM_*
#include "logMsg/traceLevels.h"                                   // Lmt*

#include "orionld/types/OrionldTenant.h"                          // OrionldTenant
#include "mongoBackend/connectionOperations.h"                    // collectionCreateIndex

#include "orionld/mongoCppLegacy/mongoCppLegacyIdIndexCreate.h"   // Own interface



// -----------------------------------------------------------------------------
//
// mongoCppLegacyIdIndexCreate -
//
bool mongoCppLegacyIdIndexCreate(OrionldTenant* tenantP)
{
  std::string err;

  if (collectionCreateIndex(tenantP->entities, BSON("_id.id" << 1), false, &err) == false)
  {
    LM_E(("Database Error (error creating entity id (_id.id) index for tenant '%s')", tenantP->tenant));
    return false;
  }

  return true;
}
