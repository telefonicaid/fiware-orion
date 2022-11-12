#ifndef SRC_LIB_ORIONLD_COMMON_ORIONLDSTATE_H_
#define SRC_LIB_ORIONLD_COMMON_ORIONLDSTATE_H_

/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include <time.h>                                                // struct timespec, struct timeval
#include <semaphore.h>                                           // sem_t
#include <mongoc/mongoc.h>                                       // MongoDB C Client Driver
#include <microhttpd.h>                                          // MHD_Connection
#include <bson/bson.h>                                           // bson_error_t
#include <curl/curl.h>                                           // CURLM

#include "orionld/db/dbDriver.h"                                 // database driver header
#include "orionld/db/dbConfiguration.h"                          // DB_DRIVER_MONGOC

extern "C"
{
#include "prometheus-client-c/prom/include/prom.h"               // prom_counter_t
#include "kjson/kjson.h"                                         // Kjson
#include "kjson/KjNode.h"                                        // KjNode
}

#include "common/globals.h"                                      // ApiVersion
#include "common/limits.h"                                       // IP_LENGTH_MAX, STATIC_BUFFER_SIZE
#include "common/MimeType.h"                                     // MimeType
#include "common/RenderFormat.h"                                 // RenderFormat
#include "rest/HttpStatusCode.h"                                 // HttpStatusCode
#include "rest/Verb.h"                                           // Verb
#include "parse/CompoundValueNode.h"                             // orion::CompoundValueNode

#include "orionld/common/performance.h"                          // REQUEST_PERFORMANCE
#include "orionld/common/OrionldResponseBuffer.h"                // OrionldResponseBuffer
#include "orionld/kjTree/kjTreeLog.h"                            // Because it is so often used but then removed again ...
#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/types/OrionldGeoIndex.h"                       // OrionldGeoIndex
#include "orionld/types/OrionldPrefixCache.h"                    // OrionldPrefixCache
#include "orionld/types/OrionldTenant.h"                         // OrionldTenant
#include "orionld/types/OrionldHeader.h"                         // OrionldHeaderSet
#include "orionld/types/OrionldAlteration.h"                     // OrionldAlteration
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/troe/troe.h"                                   // TroeMode
#include "orionld/context/OrionldContext.h"                      // OrionldContext



// -----------------------------------------------------------------------------
//
// ORIONLD_VERSION -
//
#define ORIONLD_VERSION "post-v1.1.0"



// -----------------------------------------------------------------------------
//
// ORIONLD_DEFAULT_DATASET_ID -
//
#define ORIONLD_DEFAULT_DATASET_ID "urn:ngsi-ld:default:datasetId"



// -----------------------------------------------------------------------------
//
// Forward declarations -
//
struct OrionLdRestService;



// -----------------------------------------------------------------------------
//
// OrionldUriParamOptions - flags for all possible members in URI Param ?options=x,y,z
//
typedef struct OrionldUriParamOptions
{
  bool update;
  bool replace;
  bool noOverwrite;
  bool keyValues;
  bool concise;
  bool sysAttrs;
  bool normalized;
  bool fromDb;
  bool append;         // Only NGSIv2
  bool values;         // Only NGSIv2
  bool uniqueValues;   // Only NGSIv2
  bool dateCreated;    // Only NGSIv2
  bool dateModified;   // Only NGSIv2
  bool noAttrDetail;   // Only NGSIv2
  bool upsert;         // Only NGSIv2
} OrionldUriParamOptions;



// -----------------------------------------------------------------------------
//
// OrionldUriParams -
//
typedef struct OrionldUriParams
{
  char*     id;
  char*     type;
  char*     typePattern;
  char*     idPattern;
  char*     attrs;
  char*     options;
  int       offset;
  int       limit;
  bool      count;
  char*     q;
  char*     mq;
  char*     geometry;
  char*     coordinates;
  char*     georel;
  char*     geoloc;
  char*     geoproperty;
  char*     geometryProperty;
  char*     datasetId;
  bool      deleteAll;
  char*     timeproperty;
  char*     timerel;
  char*     timeAt;
  char*     endTimeAt;
  bool      details;
  bool      prettyPrint;
  int       spaces;
  char*     subscriptionId;
  bool      location;
  char*     url;
  bool      reload;
  char*     exists;
  char*     notExists;
  char*     metadata;
  char*     orderBy;
  bool      collapse;
  bool      reset;
  char*     attributeFormat;
  char*     level;
  char*     relationships;
  char*     geoproperties;
  char*     languageproperties;
  char*     observedAt;
  char*     lang;
  bool      local;

  double    observedAtAsDouble;
  uint64_t  mask;
} OrionldUriParams;



// -----------------------------------------------------------------------------
//
// OrionldNotificationInfo -
//
typedef struct OrionldNotificationInfo
{
  char*     subscriptionId;
  MimeType  mimeType;
  KjNode*   attrsForNotification;
  char*     reference;
  int       fd;
  bool      connected;
  bool      allOK;
} OrionldNotificationInfo;



// -----------------------------------------------------------------------------
//
// OrionldPhase -
//
typedef enum OrionldPhase
{
  OrionldPhaseStartup = 1,
  OrionldPhaseServing
} OrionldPhase;



// -----------------------------------------------------------------------------
//
// OrionldStateOut - data for the response
//
typedef struct OrionldStateOut
{
  // Outgoing HTTP headers
  OrionldHeaderSet headers;
  MimeType         contentType;  // Content-Type is "special" - not part of OrionldHeaderSet

  // Rendering info
  RenderFormat    format;

  // Errors
  char*     acceptErrorDetail;  // FIXME: Use OrionldProblemDetails for this
} OrionldStateOut;



// -----------------------------------------------------------------------------
//
// OrionldStateIn - data of the request
//
typedef struct OrionldStateIn
{
  // Incoming HTTP headers
  MimeType  contentType;
  char*     contentTypeString;
  int       contentLength;
  char*     origin;
  char*     host;
  char*     xRealIp;
  char*     xForwardedFor;
  char*     connection;
  char*     servicePath;
  char*     xAuthToken;
  char*     authorization;
  char*     tenant;
  char*     legacy;          // Use legacy mongodb driver / mongoBackend
  bool      performance;

  // Incoming payload
  char*     payload;
  int       payloadSize;
  char*     payloadCopy;

  // Meta info for URL parameters
  bool      attributeFormatAsObject;
  bool      entityTypeDoesNotExist;
  bool      entityTypeExists;
  char*     geometryPropertyExpanded;

  // Processed URI params
  StringArray  idList;
  StringArray  typeList;
  StringArray  attrList;

  // Processed wildcards
  char*         pathAttrExpanded;
} OrionldStateIn;



// -----------------------------------------------------------------------------
//
// OrionldMongoC -
//
typedef struct OrionldMongoC
{
  mongoc_client_t*      client;
  mongoc_collection_t*  contextsP;
  mongoc_collection_t*  entitiesP;
  mongoc_collection_t*  subscriptionsP;
  mongoc_collection_t*  registrationsP;
  bson_error_t          error;
} OrionldMongoC;



// -----------------------------------------------------------------------------
//
// OrionldConnectionState - the state of the connection
//
// This struct contains all the state of a connection, like the Kjson pointer, the pointer to
// the RestService of the request or the urlPath of the request or ...
// Basically EVERYTHING that is a 'characteristics' for the connection.
// These fields/variables will be set once, initially, when the request arrived and after that will only be read.
// It makes very little sense to send these variables to each and every function where they are to be used.
// Much easier and faster to simply store them in a thread global struct.
//
typedef struct OrionldConnectionState
{
  OrionldPhase            phase;
  MHD_Connection*         mhdConnection;
  struct timeval          transactionStart;       // For metrics
  struct timespec         timestamp;              // The time when the request entered
  double                  requestTime;            // Same same (timestamp), but at a floating point
  char                    requestTimeString[64];  // ISO8601 representation of 'requestTime'
  int                     httpStatusCode;
  Kjson                   kjson;
  Kjson*                  kjsonP;
  KAlloc                  kalloc;
  char                    kallocBuffer[8 * 1024];
  KjNode*                 requestTree;
  KjNode*                 responseTree;
  char*                   responsePayload;
  bool                    responsePayloadAllocated;
  char*                   tenantName;
  OrionldTenant*          tenantP;
  bool                    linkHttpHeaderPresent;
  char*                   link;
  bool                    linkHeaderAdded;
  bool                    noLinkHeader;
  char*                   preferHeader;
  char*                   authorizationHeader;
  OrionldContext*         contextP;
  ApiVersion              apiVersion;
  int                     requestNo;

  KjNode*                 geoAttr[10];                 // Preallocated array of GeoProperties
  KjNode**                geoAttrV;                    // Array of GeoProperty attributes
  int                     geoAttrs;
  int                     geoAttrMax;
  char*                   geoType;
  KjNode*                 geoCoordsP;

  char*                   entityId;
  OrionldUriParamOptions  uriParamOptions;
  OrionldUriParams        uriParams;
  char*                   errorAttributeArrayP;
  char                    errorAttributeArray[512];
  int                     errorAttributeArrayUsed;
  int                     errorAttributeArraySize;
  OrionLdRestService*     serviceP;
  char*                   wildcard[2];
  char*                   urlPath;
  char*                   httpVersion;
  Verb                    verb;
  bool                    badVerb;             // ToDo: verb == NOVERB should cover this
  char*                   verbString;          // For error handling the incorrect verb is needed for the error response
  KjNode*                 payloadContextNode;
  KjNode*                 payloadIdNode;
  KjNode*                 payloadTypeNode;
  char                    contextId[256];
  bool                    useMalloc;         // allocate using kalloc or malloc?
  mongo::BSONObj*         qMongoFilterP;
  char*                   jsonBuf;           // Used by kjTreeFromBsonObj
  OrionldPrefixCache      prefixCache;
  OrionldResponseBuffer   httpResponse;


#if 0
  //
  // Array of KjNode trees that are to freed when the request thread ends
  //
  KjNode*                 delayedKjFreeVec[50];
  int                     delayedKjFreeVecIndex;
  int                     delayedKjFreeVecSize;
#endif

  //
  // Array of allocated buffers that are to be freed when the request thread ends
  //
  void*                   delayedFreeVec[1001];  // FIXME: try to make this number smaller ...
  int                     delayedFreeVecIndex;
  int                     delayedFreeVecSize;

  //
  // Special "delayed free" field for orionldRequestSend that does reallocs and it's simpler this way
  //
  void*                   delayedFreePointer;

  //
  // Notifications
  //
  OrionldAlteration*      alterations;
  OrionldAlteration*      alterationsTail;

  //
  // CURL Handles + Headers Lists
  //
  CURLM*                  multiP;                // curl multi api
  CURL**                  easyV;
  CURLM*                  curlFwdMultiP;
  int                     easySize;
  int                     easyIx;
  struct curl_slist**     curlHeadersV;
  int                     curlHeadersSize;
  int                     curlHeadersIx;


  //
  // MongoDB stuff - Context Cache uses mongoc regardless of which mongo client lib is in use
  //
  // mongoc_uri_t*           mongoUri;
  // mongoc_client_t*        mongoClient;
  // mongoc_database_t*      mongoDatabase;

  OrionldMongoC           mongoc;

  //
  // Instructions for mongoBackend
  //
  KjNode*                 creDatesP;
  bool                    onlyCount;
  KjNode*                 datasets;  // Also used w/o mongoBackend, (dbModelFromApiAttribute)

  //
  // General Behavior
  //
  bool                    forwardAttrsCompacted;
  uint32_t                acceptMask;            // "1 << MimeType" mask for all accepted Mime Types, regardless of which is chosen and of weight

  //
  // TRoE
  //
  bool                    noDbUpdate;        // If nothing changed in DB - troe is not invoked
  bool                    troeError;
  KjNode*                 duplicateArray;
  KjNode*                 troeIgnoreV[20];
  unsigned int            troeIgnoreIx;
  KjNode*                 batchEntities;
  KjNode*                 dbAttrWithDatasetsP;  // Used in TRoE for DELETE Attribute with ?deleteAll=true
  TroeMode                troeOpMode;           // Used in troePostEntities as both POST /entities and POST /temporal/entities use troePostEntities
  KjNode*                 patchBase;            // Used in troePatchEntity2 as base to where apply the patchTree and then REPLACE those attrs in postgres
  KjNode*                 patchTree;            // Used in troePatchEntity (set by troePatchEntity2) for inclusion of deleted attrs/sub-attrs

  //
  // GeoJSON - help vars for the case:
  // - Accept: application/geo+json
  // - URI param 'attrs' used
  // - The geometryproperty is not part of 'attrs' URI param
  //
  KjNode*                 geoPropertyNode;            // Must point to the "value" of the GeoProperty (for Retrieve Entity only)
  bool                    geoPropertyMissing;         // The gro-property is really not present in the DB - must be NULL is the response (for Retrieve Entity only)
  KjNode*                 geoPropertyNodes;           // object with "entityId": { <GeoProperty value> }, one per entity (for Query Entities
  bool                    geoPropertyFromProjection;  // It was added to the projection but needs to be removed

  OrionldStateOut out;
  OrionldStateIn  in;

  // NGSI-LD Scope (or NGSIv2 ServicePath)
  char* scopeV[10];
  int   scopes;

  // Attribute Format
  char* attrsFormat;

  // X-Auth-Token
  char* xAuthToken;

  // FIWARE Correlator
  char* correlator;

  //
  // Compounds
  //
  bool                                      inCompoundValue;
  orion::CompoundValueNode*                 compoundValueP;       // Points to current node in the tree
  orion::CompoundValueNode*                 compoundValueRoot;    // Points to the root of the tree

  //
  // Error Handling
  //
  OrionldProblemDetails   pd;
} OrionldConnectionState;



#ifdef REQUEST_PERFORMANCE
// -----------------------------------------------------------------------------
//
// Timestamps - timestamps for performance tests
//
typedef struct Timestamps
{
  struct timespec reqStart;               // Start of          Request
  struct timespec reqEnd;                 // End of            Request
  struct timespec parseStart;             // Start of          Request Payload body JSON parse
  struct timespec parseEnd;               // End of            Request Payload body JSON parse
  struct timespec serviceRoutineStart;    // Start of          Service Routine
  struct timespec serviceRoutineEnd;      // End of            Service Routine
  struct timespec mongoBackendStart;      // Start of          Mongo Backend Command
  struct timespec mongoBackendEnd;        // End of            Mongo Backend Command
  struct timespec dbStart;                // Start of    Main  DB query/update
  struct timespec dbEnd;                  // End of      Main  DB query/update
  struct timespec extraDbStart;           // Start of "extra"  DB query, e.g. query before an update
  struct timespec extraDbEnd;             // End of   "extra"  DB query, e.g. query before an update
  struct timespec notifStart;             // Start of          Sending of Notifications
  struct timespec notifEnd;               // End of            Sending of Notifications
  struct timespec notifDbStart;           // Start of          DB query for Notifications
  struct timespec notifDbEnd;             // End of            DB query for Notifications
  struct timespec forwardStart;           // Start of          Sending of Forwarded messages
  struct timespec forwardEnd;             // End of            Sending of Forwarded messages
  struct timespec forwardDbStart;         // Start of          DB query for Forwaring
  struct timespec forwardDbEnd;           // End of            DB query for Forwaring
  struct timespec renderStart;            // Start of          Resonse Payload body render JSON
  struct timespec renderEnd;              // End of            Resonse Payload body render JSON
  struct timespec restReplyStart;         // Start of          REST Reply
  struct timespec restReplyEnd;           // End of            REST Reply
  struct timespec troeStart;              // Start of          TRoE processing
  struct timespec troeEnd;                // End of            TRoE processing
  struct timespec requestPartEnd;         // End of            MHD-1-2-3 processing
  struct timespec requestCompletedStart;  // Start of          Request Completed
  struct timespec srStart[50];            // Start of          Service Routine Sample
  struct timespec srEnd[50];              // End of            Service Routine Sample
  char*           srDesc[50];             // Description for   Service Routine Sample
  double          mongoConnectAccumulated;
  int             getMongoConnectionCalls;
} Timestamps;

extern __thread Timestamps performanceTimestamps;
#endif


// -----------------------------------------------------------------------------
//
// orionldState -
//
extern __thread OrionldConnectionState orionldState;



// -----------------------------------------------------------------------------
//
// Thread variables from rest/rest.cpp
//
extern __thread char  clientIp[IP_LENGTH_MAX + 1];
extern __thread char  static_buffer[STATIC_BUFFER_SIZE + 1];



// -----------------------------------------------------------------------------
//
// Global state
//
extern char*             coreContextUrl;
extern char              orionldHostName[128];
extern int               orionldHostNameLen;
extern char              kallocBuffer[32 * 1024];
extern int               requestNo;                // Never mind protecting with semaphore. Just a debugging help
extern KAlloc            kalloc;
extern Kjson             kjson;
extern Kjson*            kjsonP;
extern uint16_t          portNo;
extern char              dbHost[];                 // From orionld.cpp
extern char              dbName[];                 // From orionld.cpp
extern int               dbNameLen;
extern char              dbUser[];                 // From orionld.cpp
extern char              dbPwd[];                  // From orionld.cpp
extern char              dbAuthDb[];               // From orionld.cpp
extern char              dbAuthMechanism[];        // From orionld.cpp
extern char              rplSet[];                 // From orionld.cpp
extern bool              dbSSL;                    // From orionld.cpp
extern char              dbCertFile[];             // From orionld.cpp
extern char              dbURI[];                  // From orionld.cpp
extern bool              multitenancy;             // From orionld.cpp
extern int               contextDownloadAttempts;  // From orionld.cpp
extern int               contextDownloadTimeout;   // From orionld.cpp
extern bool              troe;                     // From orionld.cpp
extern char              troeHost[256];            // From orionld.cpp
extern unsigned short    troePort;                 // From orionld.cpp
extern char              troeUser[256];            // From orionld.cpp
extern char              troePwd[256];             // From orionld.cpp
extern int               troePoolSize;             // From orionld.cpp
extern char              pgPortString[16];
extern bool              forwarding;               // From orionld.cpp
extern const char*       orionldVersion;
extern OrionldGeoIndex*  geoIndexList;
extern OrionldPhase      orionldPhase;
extern bool              orionldStartup;           // For now, only used inside sub-cache routines
extern bool              idIndex;                  // From orionld.cpp
extern bool              noNotifyFalseUpdate;      // From orionld.cpp
extern char              mongoServerVersion[32];
extern bool              experimental;             // From orionld.cpp
extern bool              mongocOnly;               // From orionld.cpp
extern char              allowedOrigin[64];        // From orionld.cpp (CORS)
extern int               maxAge;                   // From orionld.cpp (CORS)
extern char              userAgentHeader[64];      // From notificationSend.cpp - move to orionld.cpp
extern size_t            userAgentHeaderLen;       // From notificationSend.cpp - move to orionld.cpp
extern char              userAgentHeaderNoLF[64];  // move to orionld.cpp (from orionldPostEntities.cpp)
extern char              hostHeader[256];          // move to orionld.cpp (from orionldPostEntities.cpp)
extern bool              debugCurl;                // From orionld.cpp
extern bool              noCache;                  // From orionld.cpp
extern int               cSubCounters;             // Number of subscription counter updates before flush from sub-cache to DB



// -----------------------------------------------------------------------------
//
// Global variables for Prometheus
//
extern prom_counter_t*     promNgsildRequests;
extern prom_counter_t*     promNgsildRequestsFailed;
extern prom_counter_t*     promNotifications;
extern prom_counter_t*     promNotificationsFailed;



// -----------------------------------------------------------------------------
//
// Global variables for Mongo C Driver
//
extern mongoc_collection_t*  mongoEntitiesCollectionP;       // Deprecated
extern mongoc_collection_t*  mongoRegistrationsCollectionP;  // Deprecated

extern mongoc_uri_t*          mongocUri;
extern mongoc_client_pool_t*  mongocPool;
extern sem_t                  mongocContextsSem;
extern char                   mongocServerVersion[128];
extern char                   postgresServerVersion[128];



// -----------------------------------------------------------------------------
//
// orionldStateInit - initialize the thread-local variable orionldState
//
extern void orionldStateInit(MHD_Connection* connection);



// -----------------------------------------------------------------------------
//
// orionldStateRelease - release the thread-local variable orionldState
//
extern void orionldStateRelease(void);



// ----------------------------------------------------------------------------
//
// orionldStateErrorAttributeAdd -
//
extern void orionldStateErrorAttributeAdd(const char* attributeName);



// -----------------------------------------------------------------------------
//
// orionldStateDelayedKjFreeEnqueue -
//
extern void orionldStateDelayedKjFreeEnqueue(KjNode* tree);



// -----------------------------------------------------------------------------
//
// orionldStateDelayedFreeEnqueue -
//
extern void orionldStateDelayedFreeEnqueue(void* allocatedBuffer);



// -----------------------------------------------------------------------------
//
// orionldStateDelayedFreeCancel -
//
extern void orionldStateDelayedFreeCancel(void* allocatedBuffer);

#endif  // SRC_LIB_ORIONLD_COMMON_ORIONLDSTATE_H_
