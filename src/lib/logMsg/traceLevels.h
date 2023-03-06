#ifndef SRC_LIB_LOGMSG_TRACELEVELS_H_
#define SRC_LIB_LOGMSG_TRACELEVELS_H_

/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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



/* ****************************************************************************
*
* TraceLevels -
*/
typedef enum TraceLevels
{
  //
  // Requests and responses
  //
  LmtRequest = 30,                     // Incoming requests
  LmtResponse,                         // HTTP Response
  LmtHeaders,                          // HTTP Headers
  LmtUriParams,                        // HTTP URI Parameters
  LmtUriParamOptions,                  // HTTP URI Parameter 'options'

  //
  // Alterations and Notifications
  //
  LmtAlt = 40,                         // Notifications: Alterations
  LmtRegMatch,                         // Distributed Operations: registration matching
  LmtWatchedAttributes,                // Watched attributes in subscriptions
  LmtNotificationMsg,                  // Notifications: Messages
  LmtNotificationStats,                // Errors and timestamps for subscriptions
  LmtNotificationSend,                 // Sending of notifications
  LmtNotificationHeaders,              // notification request/response headers
  LmtNotificationBody,                 // notification request/response body

  //
  // Subscription Cache
  //
  LmtSubCache = 50,                    // Subscription Cache
  LmtSubCacheMatch,                    // Subscription Cache Matches

  //
  // Registration Cache
  //
  LmtRegCache = 60,                    // Registration Cache

  //
  // Distributed Operations
  //
  LmtDistOpRequest = 70,               // ONLY the verb, path, and body of a distributed request
  LmtDistOpResponse,                   // ONLY the body and status code of the response to a distributed request
  LmtDistOpResponseBuf,                // Specific debugging of the incoming response of a distributed message
  LmtDistOpResponseDetail,             // Details on responses to distributed requests
  LmtDistOpResponseHeaders,            // HTTP headers of responses to distributed requests
  LmtDistOp207,                        // Merging of the final 207 response
  LmtDistOpList,                       // Matching registrations subject to forwarded requests
  LmtDistOpMsgs,                       // Deprecated, still in use though :(
  LmtMqtt,                             // MQTT notifications

  //
  // Pagination
  //
  LmtEntityMap = 80,                   // The arrays of registrations per entity - distributed GET /entities
  LmtEntityMapDetail,                  // Details of the entity-registration maps

  //
  // Misc
  //
  LmtMongoc = 90,                      // Entire mongoc library
  LmtSR,                               // Service Routine (whatever it is doing)
  LmtSemaphore,                        // Semaphores
  LmtKjlParse,                         // Trace level start for K libs
  LmtLegacy                            // Old code (mongoBackend, json parsers, etc)
} TraceLevels;



/* ****************************************************************************
*
* traceLevelName -
*/
extern char* traceLevelName(TraceLevels level);

#endif  // SRC_LIB_LOGMSG_TRACELEVELS_H_
