/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#ifdef DEBUG
#include <sys/types.h>                                              // DIR, dirent
#include <fcntl.h>                                                  // O_RDONLY
#include <dirent.h>                                                 // opendir(), readdir(), closedir()
#include <sys/stat.h>                                               // statbuf
#include <unistd.h>                                                 // stat()
#endif

#include <microhttpd.h>

#include "logMsg/logMsg.h"                                          // LM_*
#include "logMsg/traceLevels.h"                                     // Lmt*

extern "C"
{
#include "kbase/kInit.h"                                             // kInit
#include "kalloc/kaInit.h"                                           // kaInit
#include "kalloc/kaBufferInit.h"                                     // kaBufferInit
#include "kjson/kjBufferCreate.h"                                    // kjBufferCreate
#include "kjson/kjParse.h"                                           // kjParse
}

#include "orionld/common/orionldState.h"                             // orionldState
#include "orionld/common/urlCheck.h"                                 // urlCheck
#include "orionld/context/orionldCoreContext.h"                      // orionldCoreContext, ORIONLD_CORE_CONTEXT_URL
#include "orionld/context/orionldContextInit.h"                      // orionldContextInit
#include "orionld/rest/OrionLdRestService.h"                         // OrionLdRestService, ORION_LD_SERVICE_PREFIX_LEN
#include "orionld/rest/temporaryErrorPayloads.h"                     // Temporary Error Payloads
#include "orionld/serviceRoutines/orionldPostEntities.h"             // orionldPostEntities
#include "orionld/serviceRoutines/orionldPostEntity.h"               // orionldPostEntity
#include "orionld/serviceRoutines/orionldPatchEntity.h"              // orionldPatchEntity
#include "orionld/serviceRoutines/orionldDeleteEntity.h"             // orionldDeleteEntity
#include "orionld/serviceRoutines/orionldPatchAttribute.h"           // orionldPatchAttribute
#include "orionld/serviceRoutines/orionldDeleteAttribute.h"          // orionldDeleteAttribute
#include "orionld/serviceRoutines/orionldPostSubscriptions.h"        // orionldPostSubscriptions
#include "orionld/serviceRoutines/orionldGetSubscriptions.h"         // orionldGetSubscriptions
#include "orionld/serviceRoutines/orionldGetSubscription.h"          // orionldGetSubscription
#include "orionld/serviceRoutines/orionldPostRegistrations.h"        // orionldPostRegistrations
#include "orionld/serviceRoutines/orionldGetVersion.h"               // orionldGetVersion
#include "orionld/serviceRoutines/orionldPostBatchDeleteEntities.h"  // orionldPostBatchDeleteEntities
#include "orionld/serviceRoutines/orionldPostBatchCreate.h"          // orionldPostBatchCreate
#include "orionld/serviceRoutines/orionldPostBatchUpsert.h"          // orionldPostBatchUpsert
#include "orionld/serviceRoutines/orionldPostQuery.h"                // orionldPostQuery
#include "orionld/serviceRoutines/orionldGetTenants.h"               // orionldGetTenants
#include "orionld/serviceRoutines/orionldGetDbIndexes.h"             // orionldGetDbIndexes
#include "orionld/temporal/temporalPostEntities.h"                   // temporalPostEntities
#include "orionld/temporal/temporalPostBatchDelete.h"                // temporalPostBatchDelete
#include "orionld/temporal/temporalDeleteAttribute.h"                // temporalDeleteAttribute
#include "orionld/temporal/temporalDeleteEntity.h"                   // temporalDeleteEntity
#include "orionld/temporal/temporalPatchAttribute.h"                 // temporalPatchAttribute
#include "orionld/temporal/temporalPatchEntity.h"                    // temporalPatchEntity
#include "orionld/temporal/temporalPostBatchCreate.h"                // temporalPostBatchCreate
#include "orionld/temporal/temporalPostBatchUpsert.h"                // temporalPostBatchUpsert
#include "orionld/temporal/temporalPostBatchUpdate.h"                // temporalPostBatchUpdate
#include "orionld/temporal/temporalPostEntity.h"                     // temporalPostEntity
#include "orionld/mqtt/mqttConnectionInit.h"                         // mqttConnectionInit
#include "orionld/rest/orionldMhdConnection.h"                       // Own Interface



// -----------------------------------------------------------------------------
//
// libLogBuffer -
//
thread_local char libLogBuffer[1024 * 32];



// -----------------------------------------------------------------------------
//
// libLogFunction -
//
static void libLogFunction
(
  int          severity,              // 1: Error, 2: Warning, 3: Info, 4: Verbose, 5: Trace
  int          level,                 // Trace level || Error code || Info Code
  const char*  fileName,
  int          lineNo,
  const char*  functionName,
  const char*  format,
  ...
)
{
  va_list  args;

  /* "Parse" the variable arguments */
  va_start(args, format);

  /* Print message to variable */
  vsnprintf(libLogBuffer, sizeof(libLogBuffer), format, args);
  va_end(args);

  // LM_TMP(("Got a message, severity: %d: %s", severity, libLogBuffer));

  if (severity == 1)
    lmOut(libLogBuffer, 'E', fileName, lineNo, functionName, 0, NULL);
  else if (severity == 2)
    lmOut(libLogBuffer, 'W', fileName, lineNo, functionName, 0, NULL);
  else if (severity == 3)
    lmOut(libLogBuffer, 'I', fileName, lineNo, functionName, 0, NULL);
  else if (severity == 4)
    lmOut(libLogBuffer, 'V', fileName, lineNo, functionName, 0, NULL);
  else if (severity == 5)
    lmOut(libLogBuffer, 'T', fileName, lineNo, functionName, level + LmtKjlParse, NULL);
}



// -----------------------------------------------------------------------------
//
// orionldRestServiceV -
//
OrionLdRestServiceVector orionldRestServiceV[9];



// -----------------------------------------------------------------------------
//
// restServicePrepare -
//
static void restServicePrepare(OrionLdRestService* serviceP, OrionLdRestServiceSimplified* simpleServiceP)
{
  // 1. Simply copy/reference the two fields of OrionLdRestServiceSimplified
  serviceP->url             = (char*) simpleServiceP->url;
  serviceP->serviceRoutine  = simpleServiceP->serviceRoutine;

  // 2. Loop over the URL Path, count wildcards, charsBeforeFirstWildcard, etc
  int    ix            = ORION_LD_SERVICE_PREFIX_LEN - 1;
  char*  wildCardStart = NULL;
  char*  wildCardEnd   = NULL;

  while (serviceP->url[++ix] != 0)
  {
    char c = serviceP->url[ix];

    if (c == '*')
    {
      if (serviceP->wildcards == 0)
        wildCardStart = &serviceP->url[ix + 1];
      else if (serviceP->wildcards == 1)
        wildCardEnd = &serviceP->url[ix];

      LM_T(LmtUrlParse, ("Found a wildcard in index %d of '%s'", ix, serviceP->url));
      serviceP->wildcards += 1;
      continue;
    }

    if (serviceP->wildcards == 0)
    {
      ++serviceP->charsBeforeFirstWildcard;
      serviceP->charsBeforeFirstWildcardSum += c;
    }
    else if (serviceP->wildcards == 1)
    {
      ++serviceP->charsBeforeSecondWildcard;
      serviceP->charsBeforeSecondWildcardSum += c;
    }
  }

  if (serviceP->wildcards != 0)
  {
    if (wildCardEnd == NULL)  // If only one '*', make wildCardEnd point to end of URL
      wildCardEnd = &serviceP->url[ix];

    serviceP->matchForSecondWildcardLen = wildCardEnd - wildCardStart;

    if (serviceP->matchForSecondWildcardLen < 0)
      LM_X(1, ("Negative length of matchForSecondWildcard - not possible. SW bug"));
    if (serviceP->matchForSecondWildcardLen > (int) sizeof(serviceP->matchForSecondWildcard))
      LM_X(1, ("Too big matchForSecondWildcard - not possible. SW bug"));

    if (serviceP->matchForSecondWildcardLen != 0)
    {
      strncpy(serviceP->matchForSecondWildcard, wildCardStart, wildCardEnd - wildCardStart);
      LM_T(LmtUrlParse, ("matchForSecondWildcard: %s", serviceP->matchForSecondWildcard));
    }
  }

  //
  // Set options for the OrionLdRestService
  //
  // Most services needs the tenant to exist
  //
  // For POST /ngsi-ld/v1/entities:
  //    - The context may have to be saved in the context cache (if @context is non-string in payload)
  //      - Why?  The broker needs to be able to serve it later
  //    - The Entity ID and type must be looked up and removed from the payload (pointers to the trees in orionldState
  //      - Why? The Entity ID is used when naming the context
  //      - The Entity Type is removed and saved to save time in the service routine (all on level 1 are attributes)
  //
  //

  serviceP->options = ORIONLD_SERVICE_OPTION_MAKE_SURE_TENANT_EXISTS;


  if (serviceP->serviceRoutine == orionldPostEntities)
  {
    serviceP->options  = 0;

    serviceP->options  = ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE;
    serviceP->options |= ORIONLD_SERVICE_OPTION_CREATE_CONTEXT;
  }
  else if (serviceP->serviceRoutine == orionldPostSubscriptions)
  {
    serviceP->options  = 0;

    serviceP->options |= ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE;
    serviceP->options |= ORIONLD_SERVICE_OPTION_CREATE_CONTEXT;
  }
  else if (serviceP->serviceRoutine == orionldGetSubscriptions)
  {
    serviceP->options  |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
  }
  else if (serviceP->serviceRoutine == orionldGetSubscription)
  {
    serviceP->options |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
  }
  else if (serviceP->serviceRoutine == orionldPostBatchDeleteEntities)
  {
    serviceP->options  |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
  }
  else if (serviceP->serviceRoutine == orionldPostBatchCreate)
  {
    serviceP->options  = 0;
  }
  else if (serviceP->serviceRoutine == orionldPostBatchUpsert)
  {
    serviceP->options  = 0;
  }
  else if (serviceP->serviceRoutine == orionldPostQuery)
  {
    serviceP->options  = 0;
  }
  else if (serviceP->serviceRoutine == orionldGetVersion)
  {
    serviceP->options  = 0;

    serviceP->options  = ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
  }
  else if (serviceP->serviceRoutine == orionldPostRegistrations)
  {
    serviceP->options  = 0;

    serviceP->options |= ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE;
    serviceP->options |= ORIONLD_SERVICE_OPTION_CREATE_CONTEXT;
  }
  else if (serviceP->serviceRoutine == orionldGetTenants)
  {
    serviceP->options  = 0;
    serviceP->options  |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
  }
  else if (serviceP->serviceRoutine == orionldGetDbIndexes)
  {
    serviceP->options  = 0;
    serviceP->options  |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
  }

  if (temporal)  // CLI Option to turn on Temporal Evolution of Entities
  {
    if (serviceP->serviceRoutine == orionldPostEntities)
      serviceP->temporalRoutine  = temporalPostEntities;
    else if (serviceP->serviceRoutine == orionldPostBatchDeleteEntities)
      serviceP->temporalRoutine   = temporalPostBatchDelete;
    else if (serviceP->serviceRoutine == orionldPostEntity)
      serviceP->temporalRoutine = temporalPostEntity;
    else if (serviceP->serviceRoutine == orionldDeleteAttribute)
      serviceP->temporalRoutine = temporalDeleteAttribute;
    else if (serviceP->serviceRoutine == orionldDeleteEntity)
      serviceP->temporalRoutine = temporalDeleteEntity;
    else if (serviceP->serviceRoutine == orionldPatchAttribute)
      serviceP->temporalRoutine = temporalPatchAttribute;
    else if (serviceP->serviceRoutine == orionldPatchEntity)
      serviceP->temporalRoutine = temporalPatchEntity;
    else if (serviceP->serviceRoutine == orionldPostBatchCreate)
      serviceP->temporalRoutine = temporalPostBatchCreate;
    else if (serviceP->serviceRoutine == orionldPostBatchUpsert)
      serviceP->temporalRoutine = temporalPostBatchUpsert;
#if 0
    else if (serviceP->serviceRoutine == orionldPostBatchUpdate)
      serviceP->temporalRoutine = temporalPostBatchUpdate;
#endif
  }
}



// -----------------------------------------------------------------------------
//
// orionldServiceInit -
//
// This function converts the OrionLdRestServiceSimplified vectors to OrionLdRestService vectors
//
void orionldServiceInit(OrionLdRestServiceSimplifiedVector* restServiceVV, int vecItems, char* cachedContextDir)
{
  int svIx;    // Service Vector Index

  bzero(orionldRestServiceV, sizeof(orionldRestServiceV));

  for (svIx = 0; svIx < vecItems; svIx++)
  {
    // svIx is really the Verb (GET=0, POST=2, up to NOVERB=9. See src/lib/rest/Verb.h)

    LM_T(LmtUrlParse, ("svIx: %d", svIx));
    if (restServiceVV[svIx].serviceV == NULL)
      continue;

    int services = restServiceVV[svIx].services;

    orionldRestServiceV[svIx].serviceV  = (OrionLdRestService*) calloc(sizeof(OrionLdRestService), services);
    orionldRestServiceV[svIx].services  = services;

    int sIx;  // Service Index inside Rest Service vector

    for (sIx = 0; sIx < services; sIx++)
    {
      LM_T(LmtUrlParse, ("sIx: %d", sIx));
      restServicePrepare(&orionldRestServiceV[svIx].serviceV[sIx], &restServiceVV[svIx].serviceV[sIx]);
    }
  }


  //
  // Initialize the KBASE library
  // This call redirects all log messaghes from the K-libs to the brokers log file.
  //
  kInit(libLogFunction);


  //
  // Initialize the KALLOC library
  //
  kaInit(libLogFunction);
  kaBufferInit(&kalloc, kallocBuffer, sizeof(kallocBuffer), 32 * 1024, NULL, "Global KAlloc buffer");


  //
  // Initialize the KJSON library
  // This sets up the global kjson instance with preallocated kalloc buffer
  //
  kjsonP = kjBufferCreate(&kjson, &kalloc);


  //
  // Get the hostname - needed for contexts created by the broker
  //
  gethostname(orionldHostName, sizeof(orionldHostName));
  orionldHostNameLen = strlen(orionldHostName);

  orionldStateInit();

  //
  // Initialize the @context handling
  //
  OrionldProblemDetails pd;

  if (orionldContextInit(&pd) == false)
    LM_X(1, ("orionldContextInit failed: %s %s", pd.title, pd.detail));


  //
  // Initialize NQTT notifications
  //
  mqttConnectionInit();
}
