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
#include <stdint.h>                                             // types: uint64_t, ...

extern "C"
{
#include "kbase/kMacros.h"                                      // K_VEC_SIZE
#include "kjson/KjNode.h"                                       // KjNode
#include "kjson/kjLookup.h"                                     // kjLookup
}

#include "orionld/forwarding/FwdOperation.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// fwdOperations -
//
const char* fwdOperations[37] = {
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
  "deleteSubscription"
};



// -----------------------------------------------------------------------------
//
// fwdOperationUrlLen -
//
const int fwdOperationUrlLen[37] = {
  0,
  20,   // strlen("/ngsi-ld/v1/entities")
  21,   // strlen("/ngsi-ld/v1/entities/")       + strlen(entityId)
  27,   // strlen("/ngsi-ld/v1/entities//attrs") + strlen(entityId)
  27,   // strlen("/ngsi-ld/v1/entities//attrs") + strlen(entityId)
  28,   // strlen("/ngsi-ld/v1/entities//attrs") + strlen(entityId) + strlen(attrName)
  21,   // strlen("/ngsi-ld/v1/entities/")       + strlen(entityId)
  35,   // strlen("/ngsi-ld/v1/entityOperations/create")
  35,   // strlen("/ngsi-ld/v1/entityOperations/upsert")
  35,   // strlen("/ngsi-ld/v1/entityOperations/update")
  35,   // strlen("/ngsi-ld/v1/entityOperations/delete")
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  21,   // strlen("/ngsi-ld/v1/entities/")       + strlen(entityId)
  20,   // strlen("/ngsi-ld/v1/entities")
  34,   // strlen("/ngsi-ld/v1/entityOperations/query")
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  0,    // Not Implemented ...
  25,   // strlen("/ngsi-ld/v1/subscriptions")
  26,   // strlen("/ngsi-ld/v1/subscriptions/")  + strlen(subscriptionId)
  26,   // strlen("/ngsi-ld/v1/subscriptions/")  + strlen(subscriptionId)
  26    // strlen("/ngsi-ld/v1/subscriptions/")  + strlen(subscriptionId)
};



// -----------------------------------------------------------------------------
//
// fwdOperationAlias -
//
// Aliases for pre-defined groups of operations
//
const char* fwdOperationAlias[5] = {
  "none",
  "federationOps",
  "updateOps",
  "retrieveOps",
  "redirectionOps"
};



#define M(x) (1LL << x)
uint64_t federationOpsMask   = M(FwdRetrieveEntity)       | M(FwdQueryEntity)       | M(FwdCreateSubscription) | M(FwdUpdateSubscription) |
                               M(FwdRetrieveSubscription) | M(FwdQuerySubscription) | M(FwdDeleteSubscription);
uint64_t updateOpsMask       = M(FwdUpdateEntity)         | M(FwdUpdateAttrs)       | M(FwdReplaceEntity)      | M(FwdReplaceAttrs);
uint64_t retrieveOpsMask     = M(FwdRetrieveEntity)       | M(FwdQueryEntity);
uint64_t redirectionOpsMask  = M(FwdCreateEntity)         | M(FwdUpdateEntity)      | M(FwdAppendAttrs)        | M(FwdUpdateAttrs)        |
                               M(FwdDeleteAttrs)          | M(FwdDeleteEntity)      | M(FwdMergeEntity)        | M(FwdReplaceEntity)      |
                               M(FwdReplaceAttrs)         | M(FwdRetrieveEntity)    | M(FwdQueryEntity);



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



// -----------------------------------------------------------------------------
//
// fwdOperationAliasFromString -
//
FwdOperation fwdOperationAliasFromString(const char* fwdOp)
{
  for (long unsigned int ix = 1; ix < K_VEC_SIZE(fwdOperationAlias); ix++)
  {
    if (strcmp(fwdOp, fwdOperationAlias[ix]) == 0)
      return (FwdOperation) ix;
  }

  return FwdNone;
}



// -----------------------------------------------------------------------------
//
// fwdOperationMask -
//
uint64_t fwdOperationMask(KjNode* operationsP)
{
  if (operationsP == NULL)
    return federationOpsMask;

  uint64_t mask = 0;
  for (KjNode* operationP = operationsP->value.firstChildP; operationP != NULL; operationP = operationP->next)
  {
    char* op = operationP->value.s;

    if      (strcmp(op, "federationOps")  == 0)      mask |= federationOpsMask;
    else if (strcmp(op, "updateOps")      == 0)      mask |= updateOpsMask;
    else if (strcmp(op, "retrieveOps")    == 0)      mask |= retrieveOpsMask;
    else if (strcmp(op, "redirectionOps") == 0)      mask |= redirectionOpsMask;
    else
    {
      for (unsigned int ix = 1; ix < K_VEC_SIZE(fwdOperations); ix++)
      {
        if (strcmp(op, fwdOperations[ix]) == 0)
        {
          mask |= (1L << ix);
          break;
        }
      }
    }
  }

  return mask;
}
