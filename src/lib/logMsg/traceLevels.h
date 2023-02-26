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
  // All the old trace levels are to be removed and new invented.
  //

  // New
  LmtDistOpMsgs = 20,                  // Distributed Operations: messages
  LmtRegMatch,                         // Distributed Operations: registration matching
  LmtAlt,                              // Notifications: Alterations
  LmtHeaders = 24,                     // HTTP Headers
  LmtUriParams,                        // HTTP URI Parameters
  LmtResponse,                         // HTTP Response
  LmtMqtt,                             // MQTT (notifications)
  LmtMongoc,                           // Entire mongoc library
  LmtEntityType     = 30,              // Entity Types
  LmtDistOpRequest,                    // ONLY the verb, path, and body of a distributed request
  LmtDistOpResponse,                   // ONLY the body and status code of the response to a distributed request
  LmtDistOp207,                        // Merging of the final 207 response
  LmtDistOpResponseBuf,                // Specific debugging of the incoming response of a distributed message
  LmtDistOpResponseDetail,             // Details on responses to distributed requests
  LmtDistOpResponseHeaders,            // HTTP headers of responses to distributed requests
  LmtSR = 40,                          // Service Routine (whatever it is doing)
  LmtRegCreate,                        // Creation of registrations
  LmtRegCache,                         // Registration Cache
  LmtWatchedAttributes,                // Watched attributes in subscriptions
  LmtUriParamOptions,                  // HTTP URI Parameter 'options'
  LmtNotificationMsg     = 200,        // Notifications: Messages
  LmtNotificationStats   = 201,        // Errors and timestamps for subscriptions
  LmtNotificationSend    = 202,        // Sending of notifications
  LmtNotificationHeaders = 203,        // notification request/response headers
  LmtNotificationBody    = 204,        // notification request/response body

  // Both,
  LmtRequest = 38,                     // Incoming requests
  LmtSubCache = 205,                   // Subscription Cache

  // Old
  LmtRest = 20,
  LmtRestCompare,
  LmtUrlParse,
  LmtHttpRequest,
  LmtHttpHeaders,
  LmtHttpDaemon = 25,
  LmtHttpUnsupportedHeader,
  LmtMhd,
  LmtSavedResponse,
  LmtIncompletePayload,
  LmtTenant = 30,
  LmtServicePath,
  LmtPagination,
  LmtCoap,
  LmtHttps = 35,
  LmtIpVersion,
  LmtCtxProviders,
  LmtPayload = 39,

  /* Parser (40-59) */
  LmtParse    = 40,
  LmtParsedPayload,
  LmtParseCheck,
  LmtNew,
  LmtTreat = 45,
  LmtDump,
  LmtNullNode,
  LmtCompoundValue,
  LmtCompoundValueAdd,
  LmtCompoundValueLookup = 50,
  LmtCompoundValueRender,
  LmtCompoundValueRaw,
  LmtCompoundValueContainer,
  LmtCompoundValueStep,
  LmtCompoundValueShow = 55,
  LmtJsonAttributes,

  /* RestService and Service (60-79) */
  LmtService     = 60,
  LmtConvenience,

  /* ConvenienceMap (80-99) */
  LmtClone = 80,

  /* MongoBackend (100-119) */
  LmtMongo = 100,

  /* Cleanup (120-139) */
  LmtDestructor = 120,
  LmtRelease,

  /* Types (140-159) */
  LmtEntityId = 140,
  LmtRestriction,
  LmtScope,

  /* Notifications (160-179) */
  LmtNotifier = 160,

  /* Input/Output payloads (180-199) */
  LmtServiceInputPayload = 180,
  LmtServiceOutPayload,
  LmtClientInputPayload,
  LmtClientOutputPayload = 183,  // Very important for harness test notification_different_sizes
  LmtPartialPayload,
  LmtClientOutputPayloadDump,
  LmtCPrForwardRequestPayload,
  LmtCPrForwardResponsePayload,

  /* Semaphores (200-204) */
  LmtReqSem = 200,
  LmtMongoSem,
  LmtTransSem,
  LmtCacheSem,
  LmtTimeStatSem,

  /* Cache (205 - 207) */
  LmtSubCacheMatch,
  LmtCacheSync,

  /* Others (207-211) */
  LmtCm = 207,
  LmtRush,
  LmtSoftError,
  LmtNotImplemented,
  LmtCurlContext,

#ifdef ORIONLD
  LmtKjlParse     = 212,
  LmtKjlParseValue,
  LmtKjlParseObject,
  LmtKjlParseArray,
  LmtKjlAllocBuffer,
  LmtKjlMalloc,
  LmtKjlAllocBytesLeft,
  LmtKjlRender,
  LmtKjlRenderNode,
  LmtKjlRenderArray,
  LmtKjlRenderObject,
  LmtKjlRenderValue,
  LmtKjlShortArray,
  LmtKjlNewline,
  LmtKjlBuilder,

  LmtWriteCallback = 228,
  LmtRequestSend,
  LmtPayloadCheck,
  LmtUriExpansion,
  LmtCompoundCreation,
  LmtErrorResponse,
  LmtUriPath,
  LmtVerb,
  LmtServiceRoutine,
  LmtServiceLookup,
  LmtPayloadParse,
  LmtJsonResponse,
  LmtFree,
  LmtMetadata,

  LmtContext = 242,
  LmtContextList,
  LmtContextTreat,
  LmtContextDownload,
  LmtContextLookup,
  LmtContextItemLookup,
  LmtContextValueLookup,
  LmtContextPresent,
  LmtPreloadedContexts,
  LmtAlias,
  LmtGeoJson,
  LmtAccept,
  LmtBadVerb,
  LmtStringFilter = 254,

#endif

  LmtBug = 255
} TraceLevels;



/* ****************************************************************************
*
* traceLevelName - 
*/
extern char* traceLevelName(TraceLevels level);

#endif  // SRC_LIB_LOGMSG_TRACELEVELS_H_
