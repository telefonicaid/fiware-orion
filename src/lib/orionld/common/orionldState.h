#ifndef SRC_LIB_ORIONLD_COMMON_ORIONLDSTATE_H_
#define SRC_LIB_ORIONLD_COMMON_ORIONLDSTATE_H_

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
#include "kjson/kjson.h"                                       // Kjson
#include "kjson/KjNode.h"                                      // KjNode
}
#include "common/globals.h"                                    // ApiVersion
#include "orionld/context/OrionldContext.h"                    // OrionldContext



// -----------------------------------------------------------------------------
//
// OrionldUriParamOptions - flags for all possible members in URI Param ?options=x,y,z
//
typedef struct OrionldUriParamOptions
{
  bool noOverwrite;
} OrionldUriParamOptions;



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
  Kjson                   kjson;
  Kjson*                  kjsonP;
  KAlloc                  kalloc;
  char                    kallocBuffer[8 * 1024];
  char*                   tenant;
  char*                   link;
  bool                    useLinkHeader;
  bool                    linkToBeFreed;
  bool                    linkHeaderAdded;
  OrionldContext          inlineContext;
  OrionldContext*         contextP;
  ApiVersion              apiVersion;
  int                     requestNo;
  KjNode*                 locationAttributeP;
  KjNode*                 geoTypeP;
  KjNode*                 geoCoordsP;
  int64_t                 overriddenCreationDate;
  int64_t                 overriddenModificationDate;
  bool                    entityCreated;                // If an entity is created, if complex context, it must be stored
  char*                   entityId;
  char*                   httpReqBuffer;
  OrionldUriParamOptions  uriParamOptions;
  char*                   errorAttributeArrayP;
  char                    errorAttributeArray[512];
  int                     errorAttributeArrayUsed;
  int                     errorAttributeArraySize;
} OrionldConnectionState;



// -----------------------------------------------------------------------------
//
// orionldState -
//
extern __thread OrionldConnectionState orionldState;



// -----------------------------------------------------------------------------
//
// Global state
//
extern int       requestNo;  // Never mind protecting with semaphore. Just a debugging help
extern char      kallocBuffer[32 * 1024];
extern KAlloc    kalloc;
extern Kjson     kjson;
extern Kjson*    kjsonP;
extern char*     hostname;
extern uint16_t  portNo;



// -----------------------------------------------------------------------------
//
// orionldStateInit - initialize the thread-local variable orionldState
//
extern void orionldStateInit(void);



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

#endif  // SRC_LIB_ORIONLD_COMMON_ORIONLDSTATE_H_
