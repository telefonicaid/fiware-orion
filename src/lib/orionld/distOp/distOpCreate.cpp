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
#include <string.h>                                              // strlen

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/StringArray.h"                           // StringArray, stringArrayClone
#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/types/DistOpType.h"                            // DistOpType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/distOp/distOpAttrs.h"                          // distOpAttrs
#include "orionld/distOp/distOpCreate.h"                         // Own interface



// -----------------------------------------------------------------------------
//
// distOpCreate -
//
DistOp* distOpCreate
(
  DistOpType    operation,
  RegCacheItem* regP,
  StringArray*  idList,
  StringArray*  typeList,
  StringArray*  attrList    // As it arrives in the GET request (URL param 'attrs')
)
{
  DistOp* distOpP = (DistOp*) kaAlloc(&orionldState.kalloc, sizeof(DistOp));

  if (distOpP == NULL)
    LM_X(1, ("Out of memory"));

  bzero(distOpP, sizeof(DistOp));

  distOpP->regP       = regP;
  distOpP->operation  = operation;
  distOpP->idList     = idList;
  distOpP->typeList   = typeList;

  attrList = (attrList != NULL)? stringArrayClone(attrList) : NULL;
  if ((attrList != NULL) && (attrList->items > 0))
    distOpAttrs(distOpP, attrList);
  else
    distOpP->attrsParam = NULL;

  // Assign an ID to this DistOp
  if (regP != NULL)
  {
    snprintf(distOpP->id, sizeof(distOpP->id), "DO-%04d", orionldState.distOpNo);
    ++orionldState.distOpNo;
  }
  else
    strncpy(distOpP->id, "@none", sizeof(distOpP->id));

  if (distOpP->regP != NULL)
    LM_T(LmtDistOpList, ("Created distOp '%s', for reg '%s'", distOpP->id, distOpP->regP->regId));
  else
    LM_T(LmtDistOpList, ("Created distOp '%s', for 'local DB'", distOpP->id));

  return distOpP;
}
