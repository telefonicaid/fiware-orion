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
#include <string.h>                                              // strlen
#include <semaphore.h>                                           // sem_t

extern "C"
{
#include "kbase/kTime.h"                                         // kTimeGet
#include "kbase/kMacros.h"                                       // K_VEC_SIZE
#include "kalloc/kaBufferInit.h"                                 // kaBufferInit
#include "kjson/kjBufferCreate.h"                                // kjBufferCreate
#include "kjson/kjFree.h"                                        // kjFree
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldGeoIndex.h"                       // OrionldGeoIndex
#include "orionld/types/OrionldTenant.h"                         // OrionldTenant
#include "orionld/types/QNode.h"                                 // QNode
#include "orionld/types/PernotSubCache.h"                        // PernotSubCache
#include "orionld/db/dbConfiguration.h"                          // DB_DRIVER_MONGOC
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContext, ORIONLD_CORE_CONTEXT_URL_V*
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/performance.h"                          // REQUEST_PERFORMANCE
#include "orionld/common/orionldState.h"                         // Own interface



// -----------------------------------------------------------------------------
//
// orionldVersion -
//
const char* orionldVersion = ORIONLD_VERSION;



// -----------------------------------------------------------------------------
//
// orionldState - the state of the connection
//
__thread OrionldConnectionState orionldState;



// -----------------------------------------------------------------------------
//
// timestamps -
//
#ifdef REQUEST_PERFORMANCE
__thread Timestamps performanceTimestamps;
#endif



// -----------------------------------------------------------------------------
//
// Global state - move all this to another file
//
// Actually, create two new files to replace this one:
// orionldThreadState.cpp/h
// orionldGlobalState.cpp/h
//
// Some of the global variables are defined in orionld.cpp:
// - dbName
// - dbUser
// - dbPwd
//
char              kallocBuffer[32 * 1024];
int               requestNo                = 0;             // Never mind protecting with semaphore. Just a debugging help
KAlloc            kalloc;
Kjson             kjson;
Kjson*            kjsonP;
uint16_t          portNo                   = 0;
int               dbNameLen;
char*             coreContextUrl           = (char*) ORIONLD_CORE_CONTEXT_URL_DEFAULT;  // v1.6, see orionld/context/orionldCoreContext.h
char              orionldHostName[128];
int               orionldHostNameLen       = -1;
OrionldGeoIndex*  geoIndexList             = NULL;
OrionldPhase      orionldPhase             = OrionldPhaseStartup;
bool              orionldStartup           = true;
char              pgPortString[16];
char              mongoServerVersion[32];
char              userAgentHeaderNoLF[64];     // "User-Agent: orionld/" + ORIONLD_VERSION - initialized in orionldServiceInit()
char              hostHeaderNoLF[128];
char              hostHeader[256];             // Host: xxx
size_t            hostHeaderLen;
PernotSubCache    pernotSubCache;
EntityMap*        entityMaps        = NULL;    // Used by GET /entities in the distributed case, for pagination
bool              entityMapsEnabled = false;



//
// Variables for Mongo C Driver
//
mongoc_uri_t*          mongocUri;
mongoc_client_pool_t*  mongocPool;
sem_t                  mongocContextsSem;
char                   mongocServerVersion[128];
char                   postgresServerVersion[128];



// -----------------------------------------------------------------------------
//
// orionldStateInit - initialize the thread-local variable orionldState
//
void orionldStateInit(MHD_Connection* connection)
{
  //
  // NOTE
  //   About 'bzero(&orionldState, sizeof(orionldState))'
  //   This is NOT DONE by the operating system, so, it needs to be done here 'manually'
  //
  bzero(&orionldState, sizeof(orionldState));   // Performance: ~5 microseconds

  orionldState.distributed = distributed;

  //
  // Creating kjson environment for KJson parse and render
  //
  kaBufferInit(&orionldState.kalloc, orionldState.kallocBuffer, sizeof(orionldState.kallocBuffer), 64 * 1024, NULL, "Thread KAlloc buffer");

  kTimeGet(&orionldState.timestamp);
  orionldState.mhdConnection           = connection;
  orionldState.requestTime             = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;

  numberToDate(orionldState.requestTime, orionldState.requestTimeString, sizeof(orionldState.requestTimeString));

  orionldState.kjsonP                  = kjBufferCreate(&orionldState.kjson, &orionldState.kalloc);
  orionldState.requestNo               = requestNo;
  orionldState.errorAttributeArrayP    = orionldState.errorAttributeArray;
  orionldState.errorAttributeArraySize = sizeof(orionldState.errorAttributeArray);
  orionldState.contextP                = orionldCoreContextP;
  orionldState.distOpAttrsCompacted    = true;
  orionldState.delayedFreeVecSize      = sizeof(orionldState.delayedFreeVec) / sizeof(orionldState.delayedFreeVec[0]);

  orionldState.uriParams.spaces        = 2;

  // Pagination
  orionldState.uriParams.offset        = 0;
  orionldState.uriParams.limit         = 20;

  // orionldState.delayedKjFreeVecSize    = sizeof(orionldState.delayedKjFreeVec) / sizeof(orionldState.delayedKjFreeVec[0]);

  // TRoE
  orionldState.troeOpMode = TROE_ENTITY_CREATE;

  // GeoProperty array
  orionldState.geoAttrMax = K_VEC_SIZE(orionldState.geoAttr);
  orionldState.geoAttrV   = orionldState.geoAttr;

  //  Default values for HTTP headers
  orionldState.attrsFormat = (char*) "normalized";
  orionldState.correlator  = (char*) "";

  //
  // Outgoing HTTP headers
  //
  orionldState.out.contentType    = MT_JSON;           // Default response Content-Type is "application/json"
  orionldHeaderSetInit(&orionldState.out.headers, 5);  // 5 response headers, to start with

  //
  // Default format is Normalized
  //
  orionldState.out.format         = RF_NORMALIZED;

  //
  // Default response status code is 200 OK
  //
  orionldState.httpStatusCode = 200;
}


#if 0
// -----------------------------------------------------------------------------
//
// orionldOutHeaderAdd -
//
void orionldOutHeaderAdd(char* key, char* sValue, int iValue)
{
  if (orionldState.out.httpHeaderIx >= orionldState.out.httpHeaderSize)
  {
    char** oldArray = orionldState.out.httpHeader;

    orionldState.out.httpHeaderSize += 5;
    orionldState.out.httpHeader      = (char**) kaAlloc(&orionldState.kalloc, sizeof(char*) * orionldState.out.httpHeaderSize);
    if (orionldState.out.httpHeader == NULL)
      LM_X(1, ("Out of memory trying to allocate room for %d outgoing HTTP headers", orionldState.out.httpHeaderSize));

    // Copying the already existing header pointers to the new buffer
    memcpy(orionldState.out.httpHeader, oldArray, orionldState.out.httpHeaderSize - 5);
  }

  int size = strlen(key) + 2;  // 2: colon + end-of-string

  if (sValue != NULL)
    size += strlen(sValue);
  else
    size += 16;  // 16: plenty of room for an integer

  char* header = kaAlloc(&orionldState.kalloc, size);

  if (header == NULL)
    LM_X(1, ("Out of memory trying to allocate %d bytes for an outgoing HTTP header", size));

  orionldState.out.httpHeader[orionldState.out.httpHeaderIx] = header;

  if (sValue != NULL)
    snprintf(header, size - 1, "%s:%s", key, sValue);
  else
    snprintf(header, size - 1, "%s:%d", key, iValue);

  ++orionldState.out.httpHeaderIx;
}
#endif


// -----------------------------------------------------------------------------
//
// orionldStateRelease - release the thread-local variable orionldState
//
void orionldStateRelease(void)
{
  if (orionldState.errorAttributeArrayP != orionldState.errorAttributeArray)
  {
    free(orionldState.errorAttributeArrayP);
    orionldState.errorAttributeArrayP = NULL;
  }

#if 0
  //
  // This was added to fix a leak in contextToPayload(), mhdConnectionTreat.cpp, calling kjClone(). a number of times
  // It happens for responses to GET that contain more than one item in the entity array.
  // Each item in the entity array needs a cloned context
  //
  for  (int ix = 0; ix < orionldState.delayedKjFreeVecIndex; ix++)
  {
    if (orionldState.delayedKjFreeVec[ix] != NULL)
    {
      kjFree(orionldState.delayedKjFreeVec[ix]);
      orionldState.delayedKjFreeVec[ix] = NULL;
    }
  }
#endif

  //
  // Not only KjNode trees may need delayed calls to free - normal allocated buffers may need it as well
  //
  for  (int ix = 0; ix < orionldState.delayedFreeVecIndex; ix++)
  {
    if (orionldState.delayedFreeVec[ix] != NULL)
    {
      free(orionldState.delayedFreeVec[ix]);
      orionldState.delayedFreeVec[ix] = NULL;
    }
  }
  orionldState.delayedFreeVecIndex = 0;

  if (orionldState.delayedFreePointer != NULL)
  {
    free(orionldState.delayedFreePointer);
    orionldState.delayedFreePointer = NULL;
  }

  if (orionldState.qMongoFilterP != NULL)
    delete orionldState.qMongoFilterP;
}



// ----------------------------------------------------------------------------
//
// orionldStateErrorAttributeAdd -
//
void orionldStateErrorAttributeAdd(const char* attributeName)
{
  int len      = strlen(attributeName);
  int growSize = 512;  // Add 512 bytes when growing

  //
  // Will the attribute name fit inside the error attribute string?
  //
  if (orionldState.errorAttributeArrayUsed + len + 2 > orionldState.errorAttributeArraySize)
  {
    if (orionldState.errorAttributeArrayP == orionldState.errorAttributeArray)
    {
      int size = sizeof(orionldState.errorAttributeArray) + growSize;

      orionldState.errorAttributeArrayP = (char*) malloc(size);
      if (orionldState.errorAttributeArrayP == NULL)
        LM_X(1, ("error allocating Error Attribute Array"));

      strncpy(orionldState.errorAttributeArrayP, orionldState.errorAttributeArray, size);
      orionldState.errorAttributeArraySize = size;
    }
    else
    {
      orionldState.errorAttributeArrayP = (char*) realloc(orionldState.errorAttributeArrayP, orionldState.errorAttributeArraySize + growSize);
      if (orionldState.errorAttributeArrayP == NULL)
        LM_X(1, ("error reallocating Error Attribute Array"));
      orionldState.errorAttributeArraySize = orionldState.errorAttributeArraySize + growSize;
    }
  }

  if (orionldState.errorAttributeArrayUsed == 0)
  {
    orionldState.errorAttributeArrayP[0] = '|';
    orionldState.errorAttributeArrayP[1] = 0;
    orionldState.errorAttributeArrayUsed = 1;
  }

  char* lastChar = &orionldState.errorAttributeArrayP[orionldState.errorAttributeArrayUsed];

  strcpy(lastChar, attributeName);  // Safe as we've checked that there is room already

  orionldState.errorAttributeArrayUsed += len;

  orionldState.errorAttributeArrayP[orionldState.errorAttributeArrayUsed] = '|';
  orionldState.errorAttributeArrayUsed += 1;

  orionldState.geoType    = NULL;
  orionldState.geoCoordsP = NULL;
}


#if 0
// -----------------------------------------------------------------------------
//
// orionldStateDelayedKjFreeEnqueue -
//
void orionldStateDelayedKjFreeEnqueue(KjNode* tree)  // Outdeffed
{
  if (orionldState.delayedKjFreeVecIndex >= orionldState.delayedKjFreeVecSize - 1)
    LM_X(1, ("Internal Error (the size of orionldState.delayedKjFreeVec needs to be augmented (current value: %d))", orionldState.delayedKjFreeVecSize));

  orionldState.delayedKjFreeVec[orionldState.delayedKjFreeVecIndex] = tree;
  ++orionldState.delayedKjFreeVecIndex;
}
#endif



// -----------------------------------------------------------------------------
//
// orionldStateDelayedFreeEnqueue -
//
void orionldStateDelayedFreeEnqueue(void* allocatedBuffer)
{
  if (orionldState.delayedFreeVecIndex >= orionldState.delayedFreeVecSize - 1)
    LM_X(1, ("DFREE: Internal Error (the size of orionldState.delayedFreeVec needs to be augmented (delayedFreeVecIndex=%d, delayedFreeVecSize=%d))",
             orionldState.delayedFreeVecIndex, orionldState.delayedFreeVecSize));

  orionldState.delayedFreeVec[orionldState.delayedFreeVecIndex] = allocatedBuffer;
  ++orionldState.delayedFreeVecIndex;
}



// -----------------------------------------------------------------------------
//
// orionldStateDelayedFreeCancel -
//
void orionldStateDelayedFreeCancel(void* allocatedBuffer)
{
  for (int ix = 0; ix < orionldState.delayedFreeVecIndex; ix++)
  {
    if (orionldState.delayedFreeVec[orionldState.delayedFreeVecIndex] == allocatedBuffer)
    {
      orionldState.delayedFreeVec[orionldState.delayedFreeVecIndex] = NULL;
      return;
    }
  }

  LM_E(("DFREE: Internal Error (buffer programmed for delayed free not found (%p))", allocatedBuffer));
}
