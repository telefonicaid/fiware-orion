#ifndef SRC_LIB_ORIONLD_TYPES_PERNOTSUBSCRIPTION_H_
#define SRC_LIB_ORIONLD_TYPES_PERNOTSUBSCRIPTION_H_

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

#include "orionld/types/OrionldRenderFormat.h"                 // OrionldRenderFormat
#include "orionld/types/OrionldTenant.h"                       // OrionldTenant
#include "orionld/types/Protocol.h"                            // Protocol
#include "orionld/types/StringArray.h"                         // StringArray
#include "orionld/types/OrionldGeoInfo.h"                      // OrionldGeoInfo
#include "orionld/types/QNode.h"                               // QNode
#include "orionld/types/OrionldMimeType.h"                     // MimeType



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
  double                      timeInterval;             // In seconds
  KjNode*                     kjSubP;                   // OR, I split the entire tree into binary fields inside PernotSubscription ...
  OrionldTenant*              tenantP;

  // Cached from KjSubP
  char*                       lang;
  char*                       context;
  bool                        isActive;

  // Timestamps
  double                      lastNotificationTime;     // In seconds
  double                      lastSuccessTime;
  double                      lastFailureTime;
  double                      expiresAt;

  // For the Query
  KjNode*                     eSelector;
  KjNode*                     attrsSelector;
  QNode*                      qSelector;
  OrionldGeoInfo*             geoSelector;

  // Counters
  uint32_t                    notificationAttemptsDb;   // Total number of notification attempts, in DB
  uint32_t                    notificationAttempts;     // Total number of notification attempts, in cache (to be added to dbCount)
  uint32_t                    notificationErrorsDb;     // Total number of FAILED notification attempts, in DB
  uint32_t                    notificationErrors;       // Total number of FAILED notification attempts, in cache (to be added to dbNotificationErrors)
  uint32_t                    noMatchDb;                // Total number of attempts without any matching results of the entities query, in DB
  uint32_t                    noMatch;                  // Total number of attempts without any matching results of the entities query, in cache (added to dbNoMatch)
  uint32_t                    dirty;                    // Counter of number of cache counters/timestamps updates since last flush to DB

  // URL
  char                        url[512];                 // parsed and destroyed - can't be used (protocolString, ip, and rest points inside this buffer)
  char*                       protocolString;           // pointing to 'protocol' part of 'url'
  char*                       ip;                       // pointing to 'ip' part of 'url'
  unsigned short              port;                     // port, as parsed from 'url'
  char*                       rest;                     // pointing to 'rest' part of 'url'
  Protocol                    protocol;

  // HTTP headers for the notification
  StringArray                 headers;

  // Notification Format/Details
  bool                        ngsiv2;
  bool                        sysAttrs;
  OrionldRenderFormat         renderFormat;
  MimeType                    mimeType;

  // Errors
  uint32_t                    consecutiveErrors;
  uint32_t                    cooldown;
  char*                       lastErrorReason;

  CURL*                       curlHandle;

  struct PernotSubscription*  next;
} PernotSubscription;

#endif  // SRC_LIB_ORIONLD_TYPES_PERNOTSUBSCRIPTION_H_
