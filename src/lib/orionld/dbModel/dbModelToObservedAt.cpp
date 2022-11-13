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
#include "logMsg/logMsg.h"                                       // LM*

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove. kjString, ...
}

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/dbModel/dbModelToObservedAt.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// dbModelToObservedAt -
//
KjNode* dbModelToObservedAt(KjNode* dbObservedAtP)
{
  char   buf[80];
  char*  bufP = NULL;

  if (dbObservedAtP->type == KjObject)
  {
    KjNode* valueP = kjLookup(dbObservedAtP, "value");

    if ((valueP != NULL) && (valueP->type == KjFloat))
    {
      numberToDate(valueP->value.f, buf, sizeof(buf) - 1);
      bufP = buf;
    }
  }
  else if (dbObservedAtP->type == KjString)
    bufP = dbObservedAtP->value.s;
  else if (dbObservedAtP->type == KjFloat)
  {
    numberToDate(dbObservedAtP->value.f, buf, sizeof(buf) - 1);
    bufP = buf;
  }
  else
    LM_E(("dbObservedAt is of type '%s'", kjValueType(dbObservedAtP->type)));

  if (bufP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error", "Invalid observedAt in DB", 500);
    return kjString(orionldState.kjsonP, "observedAt", "ERROR");
  }

  KjNode* observedAtP = kjString(orionldState.kjsonP, "observedAt", bufP);
  return observedAtP;
}
