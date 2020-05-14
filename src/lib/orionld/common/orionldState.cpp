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

extern "C"
{
#include "kbase/kTime.h"                                         // kTimeGet
#include "kjson/kjBufferCreate.h"                                // kjBufferCreate
#include "kjson/kjFree.h"                                        // kjFree
#include "kalloc/kaBufferInit.h"                                 // kaBufferInit
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldGeoIndex.h"                       // OrionldGeoIndex
#include "orionld/db/dbConfiguration.h"                          // DB_DRIVER_MONGOC
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContext
#include "orionld/common/QNode.h"                                // QNode
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
char              orionldHostName[128];
int               orionldHostNameLen       = -1;
char*             tenantV[100];
unsigned int      tenants                  = 0;
OrionldGeoIndex*  geoIndexList             = NULL;

//
// Variables for Mongo C Driver
//
#ifdef DB_DRIVER_MONGOC
mongoc_collection_t*  mongoEntitiesCollectionP      = NULL;
mongoc_collection_t*  mongoRegistrationsCollectionP = NULL;
#endif



// -----------------------------------------------------------------------------
//
// orionldStateInit - initialize the thread-local variable orionldState
//
void orionldStateInit(void)
{
  //
  // NOTE
  //   About 'bzero(&orionldState, sizeof(orionldState))'
  //   This is NOT DONE by the operating system, so, it needs to be done here 'manually'
  //
  bzero(&orionldState, sizeof(orionldState));

  //
  // Creating kjson environment for KJson parse and render
  //
  kaBufferInit(&orionldState.kalloc, orionldState.kallocBuffer, sizeof(orionldState.kallocBuffer), 16 * 1024, NULL, "Thread KAlloc buffer");

  kTimeGet(&orionldState.timestamp);

  orionldState.kjsonP                  = kjBufferCreate(&orionldState.kjson, &orionldState.kalloc);
  orionldState.requestNo               = requestNo;
  orionldState.tenant                  = (char*) "";
  orionldState.servicePath             = (char*) "";
  orionldState.errorAttributeArrayP    = orionldState.errorAttributeArray;
  orionldState.errorAttributeArraySize = sizeof(orionldState.errorAttributeArray);
  orionldState.contextP                = orionldCoreContextP;
  orionldState.prettyPrintSpaces       = 2;
  orionldState.forwardAttrsCompacted   = true;
  orionldState.delayedKjFreeVecSize    = sizeof(orionldState.delayedKjFreeVec) / sizeof(orionldState.delayedKjFreeVec[0]);
  orionldState.delayedFreeVecSize      = sizeof(orionldState.delayedFreeVec) / sizeof(orionldState.delayedFreeVec[0]);
  orionldState.uriParams.limit         = 20;
}



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

  //
  // This was added to fix a leak in contextToPayload(), orionldMhdConnectionTreat.cpp, calling kjClone(). a number of times
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
      orionldState.errorAttributeArrayP = (char*) malloc(sizeof(orionldState.errorAttributeArray) + growSize);
      if (orionldState.errorAttributeArrayP == NULL)
        LM_X(1, ("error allocating Error Attribute Array"));

      strncpy(orionldState.errorAttributeArrayP, orionldState.errorAttributeArray, sizeof(orionldState.errorAttributeArray));
      orionldState.errorAttributeArraySize = sizeof(orionldState.errorAttributeArray) + growSize;
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



// -----------------------------------------------------------------------------
//
// orionldStateDelayedKjFreeEnqueue -
//
void orionldStateDelayedKjFreeEnqueue(KjNode* tree)
{
  if (orionldState.delayedKjFreeVecIndex >= orionldState.delayedKjFreeVecSize - 1)
    LM_X(1, ("Internal Error (the size of orionldState.delayedKjFreeVec needs to be augmented (current value: %d))", orionldState.delayedKjFreeVecSize));

  orionldState.delayedKjFreeVec[orionldState.delayedKjFreeVecIndex] = tree;
  ++orionldState.delayedKjFreeVecIndex;
}



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
