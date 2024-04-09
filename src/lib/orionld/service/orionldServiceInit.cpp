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

#include "orionld/types/OrionLdRestService.h"                        // OrionLdRestService, ORION_LD_SERVICE_PREFIX_LEN
#include "orionld/common/orionldState.h"                             // orionldState, userAgentHeader
#include "orionld/context/orionldCoreContext.h"                      // orionldCoreContext, coreContextUrl
#include "orionld/context/orionldContextInit.h"                      // orionldContextInit
#include "orionld/payloadCheck/pCheckUri.h"                          // pCheckUriInit
#include "orionld/serviceRoutines/orionldPostEntities.h"             // orionldPostEntities
#include "orionld/serviceRoutines/orionldPostEntity.h"               // orionldPostEntity
#include "orionld/serviceRoutines/orionldGetEntities.h"              // orionldGetEntities
#include "orionld/serviceRoutines/orionldGetEntity.h"                // orionldGetEntity
#include "orionld/serviceRoutines/orionldPatchEntity.h"              // orionldPatchEntity
#include "orionld/serviceRoutines/orionldPatchEntity2.h"             // orionldPatchEntity2
#include "orionld/serviceRoutines/orionldPutEntity.h"                // orionldPutEntity
#include "orionld/serviceRoutines/orionldDeleteEntity.h"             // orionldDeleteEntity
#include "orionld/serviceRoutines/orionldDeleteEntities.h"           // orionldDeleteEntities
#include "orionld/serviceRoutines/orionldPatchAttribute.h"           // orionldPatchAttribute
#include "orionld/serviceRoutines/orionldPutAttribute.h"             // orionldPutAttribute
#include "orionld/serviceRoutines/orionldDeleteAttribute.h"          // orionldDeleteAttribute
#include "orionld/serviceRoutines/orionldPostSubscriptions.h"        // orionldPostSubscriptions
#include "orionld/serviceRoutines/orionldGetSubscriptions.h"         // orionldGetSubscriptions
#include "orionld/serviceRoutines/orionldGetSubscription.h"          // orionldGetSubscription
#include "orionld/serviceRoutines/orionldPostRegistrations.h"        // orionldPostRegistrations
#include "orionld/serviceRoutines/orionldGetVersion.h"               // orionldGetVersion
#include "orionld/serviceRoutines/orionldPostBatchDelete.h"          // orionldPostBatchDelete
#include "orionld/serviceRoutines/orionldPostBatchCreate.h"          // orionldPostBatchCreate
#include "orionld/serviceRoutines/orionldPostBatchUpsert.h"          // orionldPostBatchUpsert
#include "orionld/serviceRoutines/orionldPostBatchUpdate.h"          // orionldPostBatchUpdate
#include "orionld/serviceRoutines/orionldPostQuery.h"                // orionldPostQuery
#include "orionld/serviceRoutines/orionldGetTenants.h"               // orionldGetTenants
#include "orionld/serviceRoutines/orionldGetDbIndexes.h"             // orionldGetDbIndexes
#include "orionld/serviceRoutines/orionldGetRegistrations.h"         // orionldGetRegistrations
#include "orionld/serviceRoutines/orionldGetRegistration.h"          // orionldGetRegistration
#include "orionld/serviceRoutines/orionldPatchRegistration.h"        // orionldPatchRegistration
#include "orionld/serviceRoutines/orionldDeleteRegistration.h"       // orionldDeleteRegistration
#include "orionld/serviceRoutines/orionldPatchSubscription.h"        // orionldPatchSubscription
#include "orionld/serviceRoutines/orionldDeleteSubscription.h"       // orionldDeleteSubscription
#include "orionld/serviceRoutines/orionldGetEntityTypes.h"           // orionldGetEntityTypes
#include "orionld/serviceRoutines/orionldGetEntityType.h"            // orionldGetEntityType
#include "orionld/serviceRoutines/orionldGetEntityAttributes.h"      // orionldGetEntityAttributes
#include "orionld/serviceRoutines/orionldGetEntityAttribute.h"       // orionldGetEntityAttribute

#include "orionld/serviceRoutines/orionldGetContexts.h"              // orionldGetContexts
#include "orionld/serviceRoutines/orionldGetContext.h"               // orionldGetContext
#include "orionld/serviceRoutines/orionldPostContexts.h"             // orionldPostContexts
#include "orionld/serviceRoutines/orionldDeleteContext.h"            // orionldDeleteContext
#include "orionld/serviceRoutines/orionldPostNotify.h"               // orionldPostNotify

#include "orionld/serviceRoutines/orionldGetTemporalEntities.h"              // orionldGetTemporalEntities
#include "orionld/serviceRoutines/orionldGetTemporalEntity.h"                // orionldGetTemporalEntity
#include "orionld/serviceRoutines/orionldPostTemporalQuery.h"                // orionldPostTemporalQuery
#include "orionld/serviceRoutines/orionldPostTemporalEntities.h"             // orionldPostTemporalEntities
#include "orionld/serviceRoutines/orionldDeleteTemporalAttribute.h"          // orionldDeleteTemporalAttribute
#include "orionld/serviceRoutines/orionldDeleteTemporalAttributeInstance.h"  // orionldDeleteTemporalAttributeInstance
#include "orionld/serviceRoutines/orionldDeleteTemporalEntity.h"             // orionldDeleteTemporalEntity
#include "orionld/serviceRoutines/orionldPatchTemporalAttributeInstance.h"   // orionldPatchTemporalAttributeInstance
#include "orionld/serviceRoutines/orionldPostTemporalAttributes.h"           // orionldPostTemporalAttributes

#include "orionld/troe/troePostEntities.h"                           // troePostEntities
#include "orionld/troe/troePostBatchDelete.h"                        // troePostBatchDelete
#include "orionld/troe/troeDeleteAttribute.h"                        // troeDeleteAttribute
#include "orionld/troe/troeDeleteEntity.h"                           // troeDeleteEntity
#include "orionld/troe/troePatchAttribute.h"                         // troePatchAttribute
#include "orionld/troe/troePutAttribute.h"                           // troePutAttribute
#include "orionld/troe/troePatchEntity.h"                            // troePatchEntity
#include "orionld/troe/troePatchEntity2.h"                           // troePatchEntity2
#include "orionld/troe/troePutEntity.h"                              // troePutEntity
#include "orionld/troe/troePostBatchCreate.h"                        // troePostBatchCreate
#include "orionld/troe/troePostBatchUpsert.h"                        // troePostBatchUpsert
#include "orionld/troe/troePostBatchUpdate.h"                        // troePostBatchUpdate
#include "orionld/troe/troePostEntity.h"                             // troePostEntity
#include "orionld/mqtt/mqttConnectionInit.h"                         // mqttConnectionInit
#include "orionld/service/orionldServiceInit.h"                      // Own Interface



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
      strncpy(serviceP->matchForSecondWildcard, wildCardStart, wildCardEnd - wildCardStart);
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

  // Valid for almost ALL services - zeroed out by those services that do not support this feature
  serviceP->options = ORIONLD_SERVICE_OPTION_MAKE_SURE_TENANT_EXISTS;

  //
  // URI parameters - set it to NONE - NO URI Parameter supported
  // Later, each service is set to support whatever URI parameters the service supports
  //
  serviceP->uriParams = 0;

  // But first, ALL services support 'prettyPrint' and 'spaces'
  serviceP->uriParams |= ORIONLD_URIPARAM_PRETTYPRINT;
  serviceP->uriParams |= ORIONLD_URIPARAM_SPACES;

  if (serviceP->serviceRoutine == orionldPostEntities)
  {
    serviceP->options    = 0;  // Tenant will be created if necessary

    serviceP->options    = ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE;
    serviceP->options   |= ORIONLD_SERVICE_OPTION_EXPAND_TYPE;
    serviceP->options   |= ORIONLD_SERVICE_OPTION_DATASET_SUPPORT;
  }
  else if (serviceP->serviceRoutine == orionldPostNotify)
  {
    serviceP->uriParams |= ORIONLD_URIPARAM_SUBSCRIPTION_ID;
  }
  else if (serviceP->serviceRoutine == orionldGetEntities)
  {
    serviceP->uriParams |= ORIONLD_URIPARAM_OPTIONS;
    serviceP->uriParams |= ORIONLD_URIPARAM_FORMAT;
    serviceP->uriParams |= ORIONLD_URIPARAM_LIMIT;
    serviceP->uriParams |= ORIONLD_URIPARAM_OFFSET;
    serviceP->uriParams |= ORIONLD_URIPARAM_COUNT;
    serviceP->uriParams |= ORIONLD_URIPARAM_IDLIST;
    serviceP->uriParams |= ORIONLD_URIPARAM_TYPELIST;
    serviceP->uriParams |= ORIONLD_URIPARAM_IDPATTERN;
    serviceP->uriParams |= ORIONLD_URIPARAM_ATTRS;
    serviceP->uriParams |= ORIONLD_URIPARAM_Q;
    serviceP->uriParams |= ORIONLD_URIPARAM_EXPAND_VALUES;
    serviceP->uriParams |= ORIONLD_URIPARAM_CSF;
    serviceP->uriParams |= ORIONLD_URIPARAM_GEOREL;
    serviceP->uriParams |= ORIONLD_URIPARAM_GEOMETRY;
    serviceP->uriParams |= ORIONLD_URIPARAM_COORDINATES;
    serviceP->uriParams |= ORIONLD_URIPARAM_GEOPROPERTY;
    serviceP->uriParams |= ORIONLD_URIPARAM_GEOMETRYPROPERTY;
    serviceP->uriParams |= ORIONLD_URIPARAM_LANG;
    serviceP->uriParams |= ORIONLD_URIPARAM_LOCAL;
    serviceP->uriParams |= ORIONLD_URIPARAM_ONLYIDS;
    serviceP->uriParams |= ORIONLD_URIPARAM_ENTITYMAP;
  }
  else if (serviceP->serviceRoutine == orionldGetEntity)
  {
    serviceP->uriParams |= ORIONLD_URIPARAM_OPTIONS;
    serviceP->uriParams |= ORIONLD_URIPARAM_FORMAT;
    serviceP->uriParams |= ORIONLD_URIPARAM_ATTRS;
    serviceP->uriParams |= ORIONLD_URIPARAM_GEOMETRYPROPERTY;
    serviceP->uriParams |= ORIONLD_URIPARAM_LANG;
    serviceP->uriParams |= ORIONLD_URIPARAM_LOCAL;
    serviceP->uriParams |= ORIONLD_URIPARAM_TYPELIST;
  }
  else if (serviceP->serviceRoutine == orionldDeleteEntity)
  {
    serviceP->options   |= ORIONLD_SERVICE_OPTION_NO_CONTEXT_NEEDED;

    serviceP->uriParams |= ORIONLD_URIPARAM_TYPELIST;
  }
  else if (serviceP->serviceRoutine == orionldDeleteEntities)
  {
    serviceP->options   |= ORIONLD_SERVICE_OPTION_NO_CONTEXT_NEEDED;

    serviceP->uriParams |= ORIONLD_URIPARAM_IDLIST;
    serviceP->uriParams |= ORIONLD_URIPARAM_TYPELIST;
    serviceP->uriParams |= ORIONLD_URIPARAM_IDPATTERN;
    serviceP->uriParams |= ORIONLD_URIPARAM_ATTRS;
    serviceP->uriParams |= ORIONLD_URIPARAM_Q;
    serviceP->uriParams |= ORIONLD_URIPARAM_GEOREL;
    serviceP->uriParams |= ORIONLD_URIPARAM_GEOMETRY;
    serviceP->uriParams |= ORIONLD_URIPARAM_COORDINATES;
    serviceP->uriParams |= ORIONLD_URIPARAM_GEOPROPERTY;
    serviceP->uriParams |= ORIONLD_URIPARAM_GEOMETRYPROPERTY;
  }
  else if (serviceP->serviceRoutine == orionldPostEntity)
  {
    serviceP->uriParams |= ORIONLD_URIPARAM_OPTIONS;
  }
  else if (serviceP->serviceRoutine == orionldPatchAttribute)
  {
    serviceP->options   |= ORIONLD_SERVICE_OPTION_CLONE_PAYLOAD;
    serviceP->options   |= ORIONLD_SERVICE_OPTION_EXPAND_ATTR;

    serviceP->uriParams |= ORIONLD_URIPARAM_TYPELIST;
  }
  else if (serviceP->serviceRoutine == orionldPutAttribute)
  {
    serviceP->options   |= ORIONLD_SERVICE_OPTION_EXPAND_ATTR;

    serviceP->uriParams |= ORIONLD_URIPARAM_TYPELIST;
  }
  else if (serviceP->serviceRoutine == orionldPatchEntity)
  {
    serviceP->options   |= ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE;
    serviceP->options   |= ORIONLD_SERVICE_OPTION_CLONE_PAYLOAD;

    serviceP->uriParams |= ORIONLD_URIPARAM_TYPELIST;
  }
  else if (serviceP->serviceRoutine == orionldPatchEntity2)
  {
    serviceP->options   |= ORIONLD_SERVICE_OPTION_CLONE_PAYLOAD;
    serviceP->options   |= ORIONLD_SERVICE_OPTION_ACCEPT_JSONLD_NULL;

    serviceP->uriParams |= ORIONLD_URIPARAM_OPTIONS;
    serviceP->uriParams |= ORIONLD_URIPARAM_OBSERVEDAT;
    serviceP->uriParams |= ORIONLD_URIPARAM_TYPELIST;
  }
  else if (serviceP->serviceRoutine == orionldPutEntity)
  {
    serviceP->uriParams |= ORIONLD_URIPARAM_TYPELIST;
  }
  else if (serviceP->serviceRoutine == orionldDeleteAttribute)
  {
    serviceP->uriParams |= ORIONLD_URIPARAM_DATASETID;
    serviceP->uriParams |= ORIONLD_URIPARAM_DELETEALL;

    serviceP->options   |= ORIONLD_SERVICE_OPTION_EXPAND_ATTR;
    serviceP->uriParams |= ORIONLD_URIPARAM_TYPELIST;
  }
  else if (serviceP->serviceRoutine == orionldPostRegistrations)
  {
    serviceP->options  = 0;  // Tenant will be created if necessary

    serviceP->options |= ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE;
    serviceP->options |= ORIONLD_SERVICE_OPTION_CREATE_CONTEXT;  // FIXME: Issue #860
  }
  else if (serviceP->serviceRoutine == orionldGetRegistrations)
  {
    serviceP->uriParams |= ORIONLD_URIPARAM_OPTIONS;
    serviceP->uriParams |= ORIONLD_URIPARAM_LIMIT;
    serviceP->uriParams |= ORIONLD_URIPARAM_OFFSET;
    serviceP->uriParams |= ORIONLD_URIPARAM_COUNT;
    serviceP->uriParams |= ORIONLD_URIPARAM_IDLIST;
    serviceP->uriParams |= ORIONLD_URIPARAM_TYPELIST;
    serviceP->uriParams |= ORIONLD_URIPARAM_IDPATTERN;
    serviceP->uriParams |= ORIONLD_URIPARAM_ATTRS;
    serviceP->uriParams |= ORIONLD_URIPARAM_Q;
    serviceP->uriParams |= ORIONLD_URIPARAM_CSF;
    serviceP->uriParams |= ORIONLD_URIPARAM_GEOREL;
    serviceP->uriParams |= ORIONLD_URIPARAM_GEOMETRY;
    serviceP->uriParams |= ORIONLD_URIPARAM_COORDINATES;
    serviceP->uriParams |= ORIONLD_URIPARAM_GEOPROPERTY;
    serviceP->uriParams |= ORIONLD_URIPARAM_TIMEPROPERTY;
    serviceP->uriParams |= ORIONLD_URIPARAM_TIMEREL;
    serviceP->uriParams |= ORIONLD_URIPARAM_TIMEAT;
    serviceP->uriParams |= ORIONLD_URIPARAM_ENDTIMEAT;
  }
  else if (serviceP->serviceRoutine == orionldGetRegistration)
  {
    serviceP->uriParams |= ORIONLD_URIPARAM_OPTIONS;
  }
  else if (serviceP->serviceRoutine == orionldPatchRegistration)
  {
    serviceP->options   |= ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE;
  }
  else if (serviceP->serviceRoutine == orionldDeleteRegistration)
  {
    serviceP->options   |= ORIONLD_SERVICE_OPTION_NO_CONTEXT_NEEDED;
  }
  else if (serviceP->serviceRoutine == orionldPostSubscriptions)
  {
    serviceP->options  = 0;  // Tenant will be created if necessary

    serviceP->options |= ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE;
    serviceP->options |= ORIONLD_SERVICE_OPTION_CREATE_CONTEXT;
  }
  else if (serviceP->serviceRoutine == orionldGetSubscriptions)
  {
    serviceP->options   |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;

    serviceP->uriParams |= ORIONLD_URIPARAM_OPTIONS;
    serviceP->uriParams |= ORIONLD_URIPARAM_COUNT;
    serviceP->uriParams |= ORIONLD_URIPARAM_LIMIT;
    serviceP->uriParams |= ORIONLD_URIPARAM_OFFSET;
  }
  else if (serviceP->serviceRoutine == orionldGetSubscription)
  {
    serviceP->options   |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;

    serviceP->uriParams |= ORIONLD_URIPARAM_OPTIONS;
  }
  else if (serviceP->serviceRoutine == orionldPatchSubscription)
  {
    serviceP->options    = ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE;
  }
  else if (serviceP->serviceRoutine == orionldDeleteSubscription)
  {
    serviceP->options   |= ORIONLD_SERVICE_OPTION_NO_CONTEXT_NEEDED;
  }
  else if (serviceP->serviceRoutine == orionldPostBatchCreate)
  {
    serviceP->isBatchOp  = true;

    serviceP->options    = 0;  // Tenant will be created if necessary
    serviceP->options   |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
  }
  else if (serviceP->serviceRoutine == orionldPostBatchUpdate)
  {
    serviceP->isBatchOp  = true;

    serviceP->uriParams |= ORIONLD_URIPARAM_OPTIONS;

    serviceP->options   |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
  }
  else if (serviceP->serviceRoutine == orionldPostBatchUpsert)
  {
    serviceP->isBatchOp  = true;

    serviceP->options    = 0;  // Tenant will be created if necessary
    serviceP->options   |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;

    serviceP->uriParams |= ORIONLD_URIPARAM_OPTIONS;
  }
  else if (serviceP->serviceRoutine == orionldPostBatchDelete)
  {
    serviceP->isBatchOp  = true;

    serviceP->options   |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
    serviceP->options   |= ORIONLD_SERVICE_OPTION_NO_CONTEXT_NEEDED;
  }
  else if (serviceP->serviceRoutine == orionldPostQuery)
  {
    serviceP->isBatchOp  = true;

    serviceP->uriParams |= ORIONLD_URIPARAM_OPTIONS;
    serviceP->uriParams |= ORIONLD_URIPARAM_COUNT;
    serviceP->uriParams |= ORIONLD_URIPARAM_LIMIT;
    serviceP->uriParams |= ORIONLD_URIPARAM_OFFSET;
  }
  else if (serviceP->serviceRoutine == orionldGetEntityTypes)
  {
    serviceP->uriParams |= ORIONLD_URIPARAM_DETAILS;
  }
  else if (serviceP->serviceRoutine == orionldGetEntityType)
  {
  }
  else if (serviceP->serviceRoutine == orionldGetEntityAttributes)
  {
    serviceP->uriParams |= ORIONLD_URIPARAM_DETAILS;
  }
  else if (serviceP->serviceRoutine == orionldGetEntityAttribute)
  {
  }
  else if (serviceP->serviceRoutine == orionldPostTemporalEntities)
  {
    serviceP->options  = 0;  // Tenant will be created if necessary

    serviceP->options |= ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE;
    serviceP->options |= ORIONLD_SERVICE_OPTION_EXPAND_TYPE;
  }
  else if (serviceP->serviceRoutine == orionldGetTemporalEntities)
    serviceP->mintaka = true;
  else if (serviceP->serviceRoutine == orionldGetTemporalEntity)
    serviceP->mintaka = true;
  else if (serviceP->serviceRoutine == orionldPostTemporalQuery)
    serviceP->mintaka = true;
  else if (serviceP->serviceRoutine == orionldDeleteTemporalAttribute)
    serviceP->notImplemented = true;
  else if (serviceP->serviceRoutine == orionldDeleteTemporalAttributeInstance)
    serviceP->notImplemented = true;
  else if (serviceP->serviceRoutine == orionldDeleteTemporalEntity)
    serviceP->notImplemented = true;
  else if (serviceP->serviceRoutine == orionldPatchTemporalAttributeInstance)
    serviceP->notImplemented = true;
  else if (serviceP->serviceRoutine == orionldPostTemporalAttributes)
    serviceP->notImplemented = true;


  else if (serviceP->serviceRoutine == orionldGetVersion)
  {
    serviceP->options  = 0;  // Tenant is Ignored

    serviceP->options  = ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
    serviceP->options |= ORIONLD_SERVICE_OPTION_NO_CONTEXT_NEEDED;
  }
  else if (serviceP->serviceRoutine == orionldGetTenants)
  {
    serviceP->options = 0;  // Tenant is Ignored

    serviceP->options |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
    serviceP->options |= ORIONLD_SERVICE_OPTION_NO_CONTEXT_NEEDED;
  }
  else if (serviceP->serviceRoutine == orionldGetDbIndexes)
  {
    serviceP->options  = 0;  // Tenant is Ignored

    serviceP->options |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
    serviceP->options |= ORIONLD_SERVICE_OPTION_NO_CONTEXT_NEEDED;
  }
  else if (serviceP->serviceRoutine == orionldGetContexts)
  {
    serviceP->options  = 0;  // Tenant is Ignored

    serviceP->uriParams |= ORIONLD_URIPARAM_DETAILS;
    serviceP->uriParams |= ORIONLD_URIPARAM_LOCATION;
    serviceP->uriParams |= ORIONLD_URIPARAM_URL;
    serviceP->uriParams |= ORIONLD_URIPARAM_KIND;

    serviceP->options   |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
  }
  else if (serviceP->serviceRoutine == orionldGetContext)
  {
    serviceP->options  = 0;  // Tenant is Ignored
    serviceP->options   |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;

    serviceP->uriParams |= ORIONLD_URIPARAM_DETAILS;
  }
  else if (serviceP->serviceRoutine == orionldPostContexts)
  {
    serviceP->options  = 0;  // Tenant is Ignored

    serviceP->options   |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
    serviceP->options   |= ORIONLD_SERVICE_OPTION_NO_CONTEXT_NEEDED;
    serviceP->options   |= ORIONLD_SERVICE_OPTION_NO_CONTEXT_TYPE_CHECK;
  }
  else if (serviceP->serviceRoutine == orionldDeleteContext)
  {
    serviceP->options  = 0;  // Tenant is Ignored

    serviceP->options   |= ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
    serviceP->options   |= ORIONLD_SERVICE_OPTION_NO_CONTEXT_NEEDED;
    serviceP->options   |= ORIONLD_SERVICE_OPTION_NO_CONTEXT_TYPE_CHECK;

    serviceP->uriParams |= ORIONLD_URIPARAM_RELOAD;
  }

  if (troe)  // CLI Option to turn on Temporal Representation of Entities
  {
    if (serviceP->serviceRoutine == orionldPostEntities)
      serviceP->troeRoutine = troePostEntities;
    else if (serviceP->serviceRoutine == orionldPostTemporalEntities)
      serviceP->troeRoutine = NULL;  // TRoE processing is taken care of INSIDE the service routine
    else if (serviceP->serviceRoutine == orionldPostBatchDelete)
      serviceP->troeRoutine = troePostBatchDelete;
    else if (serviceP->serviceRoutine == orionldPostEntity)
      serviceP->troeRoutine = troePostEntity;
    else if (serviceP->serviceRoutine == orionldDeleteAttribute)
      serviceP->troeRoutine = troeDeleteAttribute;
    else if (serviceP->serviceRoutine == orionldDeleteEntity)
      serviceP->troeRoutine = troeDeleteEntity;
    else if (serviceP->serviceRoutine == orionldPatchAttribute)
      serviceP->troeRoutine = troePatchAttribute;
    else if (serviceP->serviceRoutine == orionldPutAttribute)
      serviceP->troeRoutine = troePutAttribute;
    else if (serviceP->serviceRoutine == orionldPatchEntity)
      serviceP->troeRoutine = troePatchEntity;
    else if (serviceP->serviceRoutine == orionldPatchEntity2)
      serviceP->troeRoutine = troePatchEntity2;
    else if (serviceP->serviceRoutine == orionldPutEntity)
      serviceP->troeRoutine = troePutEntity;
    else if (serviceP->serviceRoutine == orionldPostBatchCreate)
      serviceP->troeRoutine = troePostBatchCreate;
    else if (serviceP->serviceRoutine == orionldPostBatchUpsert)
      serviceP->troeRoutine = troePostBatchUpsert;
    else if (serviceP->serviceRoutine == orionldPostBatchUpdate)
      serviceP->troeRoutine = troePostBatchUpdate;
  }
}



// -----------------------------------------------------------------------------
//
// orionldServiceInit -
//
// This function converts the OrionLdRestServiceSimplified vectors to OrionLdRestService vectors
//
void orionldServiceInit(OrionLdRestServiceSimplifiedVector* restServiceVV, int vecItems)
{
  bzero(orionldRestServiceV, sizeof(orionldRestServiceV));

  //
  // FIXME: This "block should move to orionld.cpp ?
  //
  // * userAgentHeader(Len): Notifications with HTTP(s)
  // * userAgentHeaderNoLF:  Forwarding
  //
  userAgentHeaderLen = snprintf(userAgentHeader, sizeof(userAgentHeader) -1, "User-Agent: orionld/%s\r\n", ORIONLD_VERSION);  // Used in notifications as value of HTTP Header User-Agent
  snprintf(userAgentHeaderNoLF, sizeof(userAgentHeaderNoLF) -1, "User-Agent: orionld/%s", ORIONLD_VERSION);  // Used in forwarding as value of HTTP Header User-Agent

  int svIx;    // Service Vector Index
  for (svIx = 0; svIx < vecItems; svIx++)
  {
    // svIx is really the Verb (GET=0, POST=2, up to NOVERB=9. See src/lib/orionld/types/Verb.h)

    if (restServiceVV[svIx].serviceV == NULL)
      continue;

    int services = restServiceVV[svIx].services;

    orionldRestServiceV[svIx].serviceV  = (OrionLdRestService*) calloc(sizeof(OrionLdRestService), services);
    orionldRestServiceV[svIx].services  = services;

    int sIx;  // Service Index inside Rest Service vector

    for (sIx = 0; sIx < services; sIx++)
    {
      restServicePrepare(&orionldRestServiceV[svIx].serviceV[sIx], &restServiceVV[svIx].serviceV[sIx]);
    }
  }

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


  //
  // Initialize checks for incoming payloads
  //
  pCheckUriInit();
}
