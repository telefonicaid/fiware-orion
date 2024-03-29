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
#include <unistd.h>                                            // NULL

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjArray, ...
#include "kjson/kjClone.h"                                     // kjClone
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/dbModel/dbModelFromApiEntity.h"              // dbModelFromApiEntity
#include "orionld/common/batchReplaceEntity.h"                 // Own interface



// ----------------------------------------------------------------------------
//
// batchReplaceEntity -
//
// About the current state in mongo -
//   The only difference between BATCH Create and BATCH Replace is that
//   the Entity creDate needs to stay intact in the latter
//
// About Notifications
//   We're REPLACING an entire entity, so, some attributes that existed may disappear
//   This should be notified.
//   Having the "Original DB Entity", it's easy to figure out which attributes are removed in the REPLACE operation
//
KjNode* batchReplaceEntity(KjNode* inEntityP, char* entityId, char* entityType, double entityCreDate)
{
  KjNode* dbFinalEntityP = kjClone(orionldState.kjsonP, inEntityP);  // Starts out as API-Entity but dbModelFromApiEntity makes it a DB-Entity

  if (dbModelFromApiEntity(dbFinalEntityP, NULL, true, entityId, entityType) == false)
    return NULL;

  LM_T(LmtSR, ("entityType: '%s'", entityType));
  kjTreeLog(dbFinalEntityP, "dbFinalEntity", LmtSR);

  //
  // Fix the entity's creDate (from the version of the entity that was fouind in the database)
  //
  KjNode* creDateNodeP = kjLookup(dbFinalEntityP, "creDate");
  if (creDateNodeP != NULL)
    creDateNodeP->value.f = entityCreDate;
  else
  {
    creDateNodeP = kjFloat(orionldState.kjsonP, "creDate", entityCreDate);
    kjChildAdd(dbFinalEntityP, creDateNodeP);
  }

  //
  // Add the field for TRoE inside the original entity: ".troe": "Create"/"Replace"/"Update"/"Ignore"
  // This field tells TRoE processing how to treat the entity
  // It needs to be inside the tree (TRoE processing will remove it) as this merchanism
  // cannot be based on solely the Entity ID - there may be more that one Entity-instance with the same Entity ID
  //
  if (troe)
  {
    KjNode* troeNodeP = kjString(orionldState.kjsonP, ".troe", "Replace");
    kjChildAdd(inEntityP, troeNodeP);
  }

  return dbFinalEntityP;
}
