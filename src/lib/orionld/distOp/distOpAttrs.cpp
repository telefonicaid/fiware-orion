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
#include <string.h>                                              // strlen, strcpy
#include <strings.h>                                             // bzero

extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
}

#include "logMsg/logMsg.h"                                       // LM_T

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/common/orionldState.h"                         // orionldState, kjTreeLog
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/distOp/distOpAttrs.h"                          // Own interface



// -----------------------------------------------------------------------------
//
// distOpAttrs -
//
void distOpAttrs(DistOp* distOpP, StringArray* attrList)
{
  //
  // The attributes are in longnames but ... should probably compact them.
  // A registration can have its own @context, in cSourceInfo - for now, we use the @context of the original request.
  // The attrList is always cloned, so, no problem modifying it.
  //
  int attrsLen = 0;
  for (int ix = 0; ix < attrList->items; ix++)
  {
    attrList->array[ix]  = orionldContextItemAliasLookup(orionldState.contextP, attrList->array[ix], NULL, NULL);
    attrsLen            += strlen(attrList->array[ix]) + 1;
  }

  // Make room for "attrs=" and the string-end zero
  attrsLen += 7;

  char* attrs = kaAlloc(&orionldState.kalloc, attrsLen);

  if (attrs == NULL)
    LM_X(1, ("Out of memory"));

  bzero(attrs, attrsLen);

  strcpy(attrs, "attrs=");

  int   pos = 6;
  for (int ix = 0; ix < attrList->items; ix++)
  {
    int len = strlen(attrList->array[ix]);
    strcpy(&attrs[pos], attrList->array[ix]);

    // Add comma unless it's the last attr (in which case we add a zero, just in case)
    pos += len;

    if (ix != attrList->items - 1)  // Not the last attr
    {
      attrs[pos] = ',';
      pos += 1;
    }
    else
      attrs[pos] = 0;
  }

  distOpP->attrsParam    = attrs;
  distOpP->attrsParamLen = pos;
}
