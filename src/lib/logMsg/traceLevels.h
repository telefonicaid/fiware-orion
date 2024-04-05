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
  LmtWatchedAttributes,                // Watched attributes in subscriptions
  LmtNotificationMsg,                  // Notifications: Messages
  LmtNotificationStats,                // Errors and timestamps for subscriptions
  LmtNotificationSend,                 // Sending of notifications
  LmtNotificationHeaders,              // notification request/response headers
  LmtNotificationBody,                 // notification request/response body
  LmtShowChanges,                      // Add the field 'previousX' to attributes in notifications

  //
  // Subscription Cache
  //
  LmtSubCache = 50,                    // Subscription Cache
  LmtSubCacheMatch,                    // Subscription Cache Matches
  LmtSubCacheDebug,                    // Subscription Cache Debug
  LmtSubCacheStats,                    // Subscription Cache Counters and Timestamps
  LmtSubCacheSync,                     // Subscription Cache Refresh
  LmtSubCacheFlush,                    // Subscription Cache Flush

  //
  // Registration Cache
  //
  LmtRegCache = 60,                    // Registration Cache

  //
  // Distributed Operations - requests
  //
  LmtDistOpRequest = 70,               // ONLY the verb, path, and body of a distributed request
  LmtDistOpRequestHeaders,             // HTTP headers of distributed requests
  LmtDistOpRequestParams,              // URL parameters of distributed requests

  //
  // Distributed Operations - responses
  //
  LmtDistOpResponse = 80,              // ONLY the body and status code of the response to a distributed request
  LmtDistOpResponseBuf,                // Specific debugging of the incoming response of a distributed message
  LmtDistOpResponseDetail,             // Details on responses to distributed requests
  LmtDistOpResponseHeaders,            // HTTP headers of responses to distributed requests
  LmtRegMatch,                         // Distributed Operations: registration matching

  //
  // Distributed Operations - misc
  //
  LmtDistOpList = 90,                  // Linked list of DistOps
  LmtDistOpAttributes,                 // The union of attributes URL-Param / Registered Attributes
  LmtDistOpMerge,                      // Merge of responses from forwsrded requests (GET /entities)
  LmtDistOpLoop,                       // Loop detection in forwarded messages
  LmtDistOp207,                        // Merging of the final 207 response

  //
  // Context
  //
  LmtContexts = 100,                   // Contexts
  LmtContextTree,                      // Context Tree
  LmtContextCache,                     // Context Cache
  LmtContextCacheStats,                // Context Cache Statistics
  LmtContextDownload,                  // Context Download
  LmtCoreContext,                      // Core Context

  // GeoJSON
  LmtGeoJSON = 110,                    // GeoJSON ... everything (for now)

  //
  // Pernot sub-cache
  //
  LmtPernot = 120,                     // Periodic Notification Subscription cache
  LmtPernotLoop,                       // Pernot loop, when each sub is triggered in time
  LmtPernotLoopTimes,                  // Pernot loop, details on timestamps
  LmtPernotFlush,                      // Pernot flush to DB
  LmtPernotQuery,                      // Pernot query

  //
  // Pagination
  //
  LmtEntityMap = 130,                  // The arrays of registrations per entity - distributed GET /entities
  LmtEntityMapRetrieve,                // Retrieval of an entity map
  LmtEntityMapDetail,                  // Details of the entity-registration maps

  //
  // Misc
  //
  LmtMongoc = 200,                     // Entire mongoc library
  LmtSR,                               // Service Routine (whatever it is it's doing)
  LmtCount,                            // NGSILD-Results-Count header, details for distops
  LmtSemaphore,                        // Semaphores
  LmtKjlParse,                         // Trace level start for K libs
  LmtMqtt = 205,                       // MQTT notifications
  LmtQ,                                // Query Language
  LmtPostgres,                         // Postgres (TRoE)
  LmtSql,                              // SQL command for TRoE
  LmtPgPool,                           // Postgres Connection Pool
  LmtTenants = 210,                    // Well, tenants :)
  LmtSocketService,                    // Socket Service
  LmtRegex,                            // Regular expressions - all of them
  LmtDateTime,                         // DateTime (ISO8601) conversion
  LmtMimeType,                         // MimeType
  LmtArrayReduction = 215,             // Arrays of only one item are reduced to the item
  LmtFormat,                           // Normalized, Concise, Simplified
  LmtUriEncode,                        // YRL encode/decode results
  LmtBug,                              // Current bug being debugged

  //
  // Legacy
  //
  LmtLegacy  = 220,                    // Old code (mongoBackend, json parsers, etc)
  LmtLegacySubMatch,                   // Old code - update/subscription match for subs/notifs
  LmtLegacySubCacheRefresh,            // Old code - sub-cache-refresh

  LmtCurl    = 250,                    // CURL library
  LmtToDo,                             // To Do list
  LmtPatchEntity,                      // Real merge+patch
  LmtPatchEntity2,                     // Real merge+patch: merging for final API Entity, for notifications
  LmtSysAttrs,                         // System Attributes
  LmtLeak                              // Used when debugging leaks and valgrind errors
} TraceLevels;



/* ****************************************************************************
*
* traceLevelName -
*/
extern char* traceLevelName(TraceLevels level);

#endif  // SRC_LIB_LOGMSG_TRACELEVELS_H_
