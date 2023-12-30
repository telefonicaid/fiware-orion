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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldTenant.h"                         // OrionldTenant
#include "orionld/types/RegCache.h"                              // RegCache
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/mongoc/mongocRegistrationsIter.h"              // mongocRegistrationsIter
#include "orionld/dbModel/dbModelToApiRegistration.h"            // dbModelToApiRegistration
#include "orionld/regCache/regCacheItemAdd.h"                    // regCacheItemAdd
#include "orionld/regCache/regCacheItemContextCheck.h"           // regCacheItemContextCheck
#include "orionld/regCache/regCacheCreate.h"                     // Own interface



extern void apiModelToCacheRegistration(KjNode* apiRegistrationP);
// -----------------------------------------------------------------------------
//
// regIterFunc -
//
int regIterFunc(RegCache* rcP, KjNode* dbRegP)
{
  // Convert DB Reg to API Reg
  if (dbModelToApiRegistration(dbRegP, true, true) == false)
  {
    LM_E(("dbModelToApiRegistration failed"));
    return 1;
  }

  // The DB Registration 'dbRegP' is now in API Registration format (after calling dbModelToApiRegistration)
  KjNode* apiRegP = dbRegP;

  // If an @context is given for the registration, make sure it's valid
  OrionldContext* fwdContextP = NULL;
  if (regCacheItemContextCheck(apiRegP, NULL, &fwdContextP) == false)
  {
    LM_W(("Unable to resolve a Registration @context for a reg-cache item"));
    return 0;
  }

  // Registration Id
  KjNode* regIdNodeP = kjLookup(apiRegP, "id");
  char*   regId      = (regIdNodeP != NULL)? regIdNodeP->value.s : (char*) "no:reg:id";

  // Convert API Reg to Cache Reg
  apiModelToCacheRegistration(apiRegP);

  // Insert cacheRegP in tenantP->regCache (rcP)
  regCacheItemAdd(rcP, regId, apiRegP, true, fwdContextP);

  return 0;
}



// -----------------------------------------------------------------------------
//
// regCacheCreate -
//
RegCache* regCacheCreate(OrionldTenant* tenantP, bool scanRegs)
{
  RegCache* rcP = (RegCache*) malloc(sizeof(RegCache));

  if (rcP == NULL)
    LM_RE(NULL, ("Out of memory (attempt to create a registration cache)"));

  rcP->tenantP  = tenantP;
  rcP->regList  = NULL;
  rcP->last     = NULL;

  if (scanRegs)
  {
    if (mongocRegistrationsIter(rcP, regIterFunc) != 0)
      LM_E(("mongocRegistrationsIter failed"));
  }

  return rcP;
}
