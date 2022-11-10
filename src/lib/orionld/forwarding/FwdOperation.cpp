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
#include <string.h>                                             // strcmp

extern "C"
{
#include "kbase/kMacros.h"                                      // K_VEC_SIZE
}

#include "orionld/forwarding/FwdOperation.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// fwdOperations -
//
const char* fwdOperations[] = {
  "none",
  "createEntity",
  "updateEntity",
  "appendAttrs",
  "updateAttrs",
  "deleteAttrs",
  "deleteEntity",
  "createBatch",
  "upsertBatch",
  "updateBatch",
  "deleteBatch",
  "upsertTemporal",
  "appendAttrsTemporal",
  "deleteAttrsTemporal",
  "updateAttrsTemporal",
  "deleteAttrInstanceTemporal",
  "deleteTemporal",
  "mergeEntity",
  "replaceEntity",
  "replaceAttrs",
  "mergeBatch",

  "retrieveEntity",
  "queryEntity",
  "queryBatch",
  "retrieveTemporal",
  "queryTemporal",
  "retrieveEntityTypes",
  "retrieveEntityTypeDetails",
  "retrieveEntityTypeInfo",
  "retrieveAttrTypes",
  "retrieveAttrTypeDetails",
  "retrieveAttrTypeInfo",

  "createSubscription",
  "updateSubscription",
  "retrieveSubscription",
  "querySubscription",
  "deleteSubscription",

  // Aliases for pre-defined groups of operations
  "federationOps",
  "updateOps",
  "retrieveOps",
  "redirectionOps"
};



// -----------------------------------------------------------------------------
//
// fwdOperationToString -
//
const char* fwdOperationToString(FwdOperation op)
{
  if ((op < 1) || (op >= K_VEC_SIZE(fwdOperations)))
    return "nop";

  return fwdOperations[op];
}



// -----------------------------------------------------------------------------
//
// fwdOperationFromString -
//
FwdOperation fwdOperationFromString(const char* fwdOp)
{
  for (long unsigned int ix = 1; ix < K_VEC_SIZE(fwdOperations); ix++)
  {
    if (strcmp(fwdOp, fwdOperations[ix]) == 0)
      return (FwdOperation) ix;
  }

  return FwdNone;
}
