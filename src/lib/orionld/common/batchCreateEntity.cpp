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
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjString, kjChildAdd
#include "kjson/kjClone.h"                                     // kjClone
}

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/dbModel/dbModelFromApiEntity.h"              // dbModelFromApiEntity
#include "orionld/common/batchCreateEntity.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// batchCreateEntity -
//
// * An entity in DB-Model format is returned
// * 'inEntityP' is left untouched
//
KjNode* batchCreateEntity(KjNode* inEntityP, char* entityId, char* entityType, bool replaced)
{
  KjNode* dbFinalEntityP = kjClone(orionldState.kjsonP, inEntityP);  // Starts out as API-Entity but dbModelFromApiEntity makes it a DB-Entity

  if (dbModelFromApiEntity(dbFinalEntityP, NULL, true, entityId, entityType) == false)
    return NULL;

  // Inserting the ".troe" field AFTER the DB Entity has been created
  if (troe)
  {
    const char* troeOpMode = (replaced == false)? "Create" : "Replace";
    KjNode*     troeNodeP  = kjString(orionldState.kjsonP, ".troe", troeOpMode);

    kjChildAdd(inEntityP, troeNodeP);
  }

  return dbFinalEntityP;
}
