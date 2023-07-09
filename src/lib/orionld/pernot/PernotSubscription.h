#ifndef SRC_LIB_ORIONLD_PERNOT_PERNOTSUBSCRIPTION_H_
#define SRC_LIB_ORIONLD_PERNOT_PERNOTSUBSCRIPTION_H_

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
#include <stdint.h>                                            // types: uint64_t, ...
#include <curl/curl.h>                                         // CURL

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}




// -----------------------------------------------------------------------------
//
// PernotState -
//
typedef enum PernotState
{
  SubActive    = 1,
  SubPaused    = 2,
  SubErroneous = 3,
  SubExpired   = 4
} PernotState;



// -----------------------------------------------------------------------------
//
// PernotSubscription - Periodic Notification Subscription
//
typedef struct PernotSubscription
{
  char*                       subscriptionId;
  PernotState                 state;
  char                        tenant[64];
  KjNode*                     kjSubP;                   // OR, I split the entire tree into binary fields inside PernotSubscription ...

  // Timestamps
  double                      lastNotificationAttempt;  // In seconds
  double                      lastSuccessTime;
  double                      lastFailureTime;
  double                      expiresAt;

  // Error handling
  uint32_t                    consecutiveErrors;
  uint32_t                    timeInterval;             // In seconds (Subscription::timeInterval is an integer in seconds)
  uint32_t                    cooldown;

  CURL*                       curlHandle;
  
  struct PernotSubscription*  next;
} PernotSubscription;

#endif  // SRC_LIB_ORIONLD_PERNOT_PERNOTSUBSCRIPTION_H_
