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

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/forwarding/DistOp.h"                           // DistOp
#include "orionld/forwarding/DistOpType.h"                       // DistOpType



// -----------------------------------------------------------------------------
//
// attrsParam -
//
static void attrsParam(DistOp* distOpP, StringArray* attrList, bool permanent)
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

  char* attrs = (char*) ((permanent == true)? malloc(attrsLen) : kaAlloc(&orionldState.kalloc, attrsLen));

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
  StringArray*  attrList,
  bool          permanent
)
{
  DistOp* distOpP = (DistOp*) ((permanent == true)? malloc(sizeof(DistOp)) : kaAlloc(&orionldState.kalloc, sizeof(DistOp)));

  if (distOpP == NULL)
    LM_X(1, ("Out of memory"));

  bzero(distOpP, sizeof(DistOp));

  distOpP->regP       = regP;
  distOpP->operation  = operation;
  distOpP->attrList   = attrList;
  distOpP->idList     = idList;
  distOpP->typeList   = typeList;

  // Fix the comma-separated attribute list
  if ((attrList != NULL) && (attrList->items > 0))
    attrsParam(distOpP, attrList, permanent);

  // Assign an ID to this DistOp
  if (regP != NULL)
  {
    snprintf(distOpP->id, sizeof(distOpP->id), "DO-%04d", orionldState.distOpNo);
    ++orionldState.distOpNo;
  }
  else
    strncpy(distOpP->id, "local", sizeof(distOpP->id));
  
  if (distOpP->regP != NULL)
    LM_T(LmtDistOpList, ("Created distOp '%s', for reg '%s'", distOpP->id, distOpP->regP->regId));
  else
    LM_T(LmtDistOpList, ("Created distOp '%s', for 'local DB'", distOpP->id));

  return distOpP;
}
