#ifndef SRC_LIB_ORIONLD_FORWARDING_FWDOPERATION_H_
#define SRC_LIB_ORIONLD_FORWARDING_FWDOPERATION_H_

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
#include <stdint.h>                                              // types: uint64_t, ...

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}


// -----------------------------------------------------------------------------
//
// FwdOperation -
//
typedef enum FwdOperation
{
  FwdNone,                           // 0
  FwdCreateEntity,
  FwdUpdateEntity,
  FwdAppendAttrs,
  FwdUpdateAttrs,
  FwdDeleteAttrs,                    // 5
  FwdDeleteEntity,
  FwdCreateBatch,
  FwdUpsertBatch,
  FwdUpdateBatch,
  FwdDeleteBatch,                    // 10
  FwdUpsertTemporal,
  FwdAppendAttrsTemporal,
  FwdDeleteAttrsTemporal,
  FwdUpdateAttrsTemporal,
  FwdDeleteAttrInstanceTemporal,     // 15
  FwdDeleteTemporal,
  FwdMergeEntity,
  FwdReplaceEntity,
  FwdReplaceAttrs,
  FwdMergeBatch,                     // 20

  FwdRetrieveEntity,
  FwdQueryEntity,
  FwdQueryBatch,
  FwdRetrieveTemporal,
  FwdQueryTemporal,                  // 25
  FwdRetrieveEntityTypes,
  FwdRetrieveEntityTypeDetails,
  FwdRetrieveEntityTypeInfo,
  FwdRetrieveAttrTypes,
  FwdRetrieveAttrTypeDetails,        // 30
  FwdRetrieveAttrTypeInfo,

  FwdCreateSubscription,
  FwdUpdateSubscription,
  FwdRetrieveSubscription,
  FwdQuerySubscription,              // 35
  FwdDeleteSubscription
} FwdOperation;



// -----------------------------------------------------------------------------
//
// fwdOperations -
//
extern const char* fwdOperations[37];



// -----------------------------------------------------------------------------
//
// fwdOperationUrlLen -
//
extern const int fwdOperationUrlLen[37];



// -----------------------------------------------------------------------------
//
// fwdOperationToString -
//
extern const char* fwdOperationToString(FwdOperation op);



// -----------------------------------------------------------------------------
//
// fwdOperationFromString -
//
extern FwdOperation fwdOperationFromString(const char* fwdOp);



// -----------------------------------------------------------------------------
//
// fwdOperationAliasFromString -
//
extern FwdOperation fwdOperationAliasFromString(const char* fwdOp);



// -----------------------------------------------------------------------------
//
// fwdOperationMask -
//
extern uint64_t fwdOperationMask(KjNode* operationsP);

#endif  // SRC_LIB_ORIONLD_FORWARDING_FWDOPERATION_H_
