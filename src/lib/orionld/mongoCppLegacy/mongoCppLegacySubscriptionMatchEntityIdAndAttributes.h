#ifndef SRC_LIB_ORIONLD_MONGOCPPLEGACY_MONGOCPPLEGACYSUBSCRIPTIONMATCHENTITYIDANDATTRIBUTES_H_
#define SRC_LIB_ORIONLD_MONGOCPPLEGACY_MONGOCPPLEGACYSUBSCRIPTIONMATCHENTITYIDANDATTRIBUTES_H_

/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "orionld/db/dbConfiguration.h"                          // DbSubscriptionMatchCallback



// -----------------------------------------------------------------------------
//
// mongoCppLegacySubscriptionMatchEntityIdAndAttributes - 
//
// PARAMETERS
//   * entityId             The ID of the entity as a string
//   * currentEntityTree    The entire Entity as it is in the database before being updated
//   * incomingRequestTree  The incoming request, supposed to modify the current Entity
//   * subMatchCallback     The callback function to be called for each matching subscription
//
extern void mongoCppLegacySubscriptionMatchEntityIdAndAttributes
(
  const char*                 entityId,
  KjNode*                     currentEntityTree,
  KjNode*                     incomingRequestTree,
  DbSubscriptionMatchCallback subMatchCallback
);


#endif  // SRC_LIB_ORIONLD_MONGOCPPLEGACY_MONGOCPPLEGACYSUBSCRIPTIONMATCHENTITYIDANDATTRIBUTES_H_
