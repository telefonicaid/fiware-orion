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
#include <string.h>                                            // strlen

extern "C"
{
#include "kjson/kjBufferCreate.h"                              // kjBufferCreate
#include "kjson/kjFree.h"                                      // kjFree
#include "kalloc/kaBufferInit.h"                               // kaBufferInit
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/db/dbConfiguration.h"                        // DB_DRIVER_MONGOC
#include "orionld/context/orionldContextFree.h"                // orionldContextFree
#include "orionld/common/QNode.h"                              // QNode
#include "orionld/common/orionldState.h"                       // Own interface



// -----------------------------------------------------------------------------
//
// orionldState - the state of the connection
//
__thread OrionldConnectionState orionldState = { 0 };



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
char            kallocBuffer[32 * 1024];
int             requestNo                = 0;             // Never mind protecting with semaphore. Just a debugging help
KAlloc          kalloc;
Kjson           kjson;
Kjson*          kjsonP;
uint16_t        portNo                   = 0;
int             dbNameLen;
char            orionldHostName[128];
int             orionldHostNameLen       = -1;


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
  // FIXME: bzero(&orionldState, sizeof(orionldState)) ... ?
  //        This is NOT DONE by the operating system
  //        If I bzero orionldState, I get a SIGSEGV inside kjson ...
  //        Must try again!
  //
  bzero(orionldState.errorAttributeArray, sizeof(orionldState.errorAttributeArray));

  //
  // Creating kjson environment for KJson parse and render
  //
  bzero(orionldState.kallocBuffer, sizeof(orionldState.kallocBuffer));
  kaBufferInit(&orionldState.kalloc, orionldState.kallocBuffer, sizeof(orionldState.kallocBuffer), 2 * 1024, NULL, "Thread KAlloc buffer");

  orionldState.ciP                         = NULL;
  orionldState.requestNo                   = requestNo;
  orionldState.tenant                      = (char*) "";
  orionldState.kjsonP                      = kjBufferCreate(&orionldState.kjson, &orionldState.kalloc);
  orionldState.linkHttpHeaderPresent       = false;
  orionldState.link                        = NULL;
  orionldState.useLinkHeader               = true;  // Service routines can set this value to 'false' to avoid having the Link HTTP Header in its output
  orionldState.entityCreated               = false;
  orionldState.entityId                    = NULL;
  orionldState.linkHeaderAdded             = false;
  orionldState.httpReqBuffer               = NULL;
  orionldState.errorAttributeArrayP        = orionldState.errorAttributeArray;
  orionldState.errorAttributeArraySize     = sizeof(orionldState.errorAttributeArray);
  orionldState.errorAttributeArrayUsed     = 0;
  orionldState.contextToBeFreed            = false;
  orionldState.uriParamOptions.noOverwrite = false;
  orionldState.uriParamOptions.update      = false;
  orionldState.uriParamOptions.replace     = false;
  orionldState.prettyPrintSpaces           = 2;
  orionldState.prettyPrint                 = false;
  orionldState.locationAttributeP          = NULL;
  orionldState.contextP                    = NULL;
  orionldState.payloadContextNode          = NULL;
  orionldState.payloadIdNode               = NULL;
  orionldState.payloadTypeNode             = NULL;
  orionldState.acceptJson                  = false;
  orionldState.acceptJsonld                = false;
  orionldState.qMongoFilterP               = NULL;

  //
  // FIXME: This initialization of qNodeV is only necessary if a String-Filter is part of the request
  //        Should be moved elsewhere (unless I do the bzero of the entire orionldState):
  //          if (ciP->uriParam["q"] != "")
  //            bzero(orionldState.qNodeV, sizeof(orionldState.qNodeV));
  //
  bzero(orionldState.qNodeV, sizeof(orionldState.qNodeV));
  orionldState.qNodeIx               = 0;
  orionldState.jsonBuf               = NULL;

  bzero(orionldState.delayedKjFreeVec, sizeof(orionldState.delayedKjFreeVec));
  orionldState.delayedKjFreeVecIndex = 0;
  orionldState.delayedKjFreeVecSize  = sizeof(orionldState.delayedKjFreeVec) / sizeof(orionldState.delayedKjFreeVec[0]);

  orionldState.notify                = false;
  orionldState.notificationRecords   = 0;
}



// -----------------------------------------------------------------------------
//
// orionldStateRelease - release the thread-local variable orionldState
//
void orionldStateRelease(void)
{
  if (orionldState.httpReqBuffer != NULL)
  {
    free(orionldState.httpReqBuffer);
    orionldState.httpReqBuffer = NULL;
  }

  if (orionldState.errorAttributeArrayP != orionldState.errorAttributeArray)
  {
    free(orionldState.errorAttributeArrayP);
    orionldState.errorAttributeArrayP = NULL;
  }

#if 0
  // This part crashes the broker in a few functests ...
  if ((orionldState.contextP != NULL) && (orionldState.contextToBeFreed == true))
    orionldContextFree(orionldState.contextP);
#endif

  //
  // This was added to fix a leak in contextToPayload(), orionldMhdConnectionTreat.cpp, calling kjClone(). a number of times
  // It happens for responses to GET that contain more than one item in the entity array.
  // Each item in the entity array needs a cloned context
  //
  for  (int ix = 0; ix < orionldState.delayedKjFreeVecIndex; ix++)
    kjFree(orionldState.delayedKjFreeVec[ix]);

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
// orionldStateDelayedKjFree -
//
void orionldStateDelayedKjFree(KjNode* tree)
{
  if (orionldState.delayedKjFreeVecIndex >= orionldState.delayedKjFreeVecSize - 1)
    LM_X(1, ("Internal Error (the size of orionldState.delayedKjFreeVec needs to be aumented)"));

  orionldState.delayedKjFreeVec[orionldState.delayedKjFreeVecIndex] = tree;
  ++orionldState.delayedKjFreeVecIndex;
}
