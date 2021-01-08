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
extern "C"
{
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kjson/KjNode.h"                                      // KjNode
}

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/numberToDate.h"                       // numberToDate
#include "orionld/troe/pgObservedAtExtract.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// pgObservedAtExtract - extract an observedAt as either a string or a float
//
char* pgObservedAtExtract(KjNode* observedAtNodeP)
{
  if (observedAtNodeP->type == KjString)
    return observedAtNodeP->value.s;

  if (observedAtNodeP->type == KjFloat)  // Transformed to double before sent to TEMP layer - convert back to string!
  {
    char* observedAt = kaAlloc(&orionldState.kalloc, 64);
    numberToDate(observedAtNodeP->value.f, observedAt, 64);
    return observedAt;
  }

  return (char*) "1970:01:01T00:00:00.000Z";
}
