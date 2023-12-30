/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
#include "logMsg/logMsg.h"                                          // LM_*

#include "orionld/types/DistOp.h"                                   // DistOpListItem
#include "orionld/distOp/distOpListItemCreate.h"                    // distOpListItemCreate
#include "orionld/distOp/distOpListItemAdd.h"                       // Own interface



// -----------------------------------------------------------------------------
//
// distOpListItemAdd -
//
DistOpListItem* distOpListItemAdd(DistOpListItem* distOpList, const char* distOpId, char* idString)
{
  LM_T(LmtEntityMap, ("Creating DistOpListItem for DistOp '%s', entities '%s'", distOpId, idString));

  DistOpListItem* doliP = distOpListItemCreate(distOpId, idString);

  if (doliP == NULL)
    return distOpList;

  if (distOpList != NULL)
    doliP->next = distOpList;

  return doliP;
}
