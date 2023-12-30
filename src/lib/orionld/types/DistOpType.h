#ifndef SRC_LIB_ORIONLD_TYPES_DISTOPTYPE_H_
#define SRC_LIB_ORIONLD_TYPES_DISTOPTYPE_H_

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
// DistOpType -
//
typedef enum DistOpType
{
  DoNone,                           // 0
  DoCreateEntity,
  DoUpdateEntity,
  DoAppendAttrs,
  DoUpdateAttrs,
  DoDeleteAttrs,                    // 5
  DoDeleteEntity,
  DoCreateBatch,
  DoUpsertBatch,
  DoUpdateBatch,
  DoDeleteBatch,                    // 10
  DoUpsertTemporal,
  DoAppendAttrsTemporal,
  DoDeleteAttrsTemporal,
  DoUpdateAttrsTemporal,
  DoDeleteAttrInstanceTemporal,     // 15
  DoDeleteTemporal,
  DoMergeEntity,
  DoReplaceEntity,
  DoReplaceAttr,
  DoMergeBatch,                     // 20

  DoRetrieveEntity,
  DoQueryEntity,
  DoQueryBatch,
  DoRetrieveTemporal,
  DoQueryTemporal,                  // 25
  DoRetrieveEntityTypes,
  DoRetrieveEntityTypeDetails,
  DoRetrieveEntityTypeInfo,
  DoRetrieveAttrTypes,
  DoRetrieveAttrTypeDetails,        // 30
  DoRetrieveAttrTypeInfo,

  DoCreateSubscription,
  DoUpdateSubscription,
  DoRetrieveSubscription,
  DoQuerySubscription,              // 35
  DoDeleteSubscription
} DistOpType;



// -----------------------------------------------------------------------------
//
// distOpTypes -
//
extern const char* distOpTypes[37];



// -----------------------------------------------------------------------------
//
// distOpTypeUrlLen -
//
extern const int distOpTypeUrlLen[37];



// -----------------------------------------------------------------------------
//
// distOpTypeToString -
//
extern const char* distOpTypeToString(DistOpType op);



// -----------------------------------------------------------------------------
//
// distOpTypeFromString -
//
extern DistOpType distOpTypeFromString(const char* fwdOp);



// -----------------------------------------------------------------------------
//
// distOpTypeAliasFromString -
//
extern DistOpType distOpTypeAliasFromString(const char* fwdOp);



// -----------------------------------------------------------------------------
//
// distOpTypeMask -
//
extern uint64_t distOpTypeMask(KjNode* operationsP);

#endif  // SRC_LIB_ORIONLD_TYPES_DISTOPTYPE_H_
