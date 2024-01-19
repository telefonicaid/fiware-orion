/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include <curl/curl.h>                                           // curl

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjParse.h"                                       // kjParse
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove, kjChildAdd, kjArray, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/tenantList.h"                           // tenant0
#include "orionld/context/orionldEntityExpand.h"                 // orionldEntityExpand
#include "orionld/context/orionldEntityCompact.h"                // orionldEntityCompact
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/legacyDriver/legacyGetEntity.h"                // legacyGetEntity
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity
#include "orionld/kjTree/kjGeojsonEntityTransform.h"             // kjGeojsonEntityTransform
#include "orionld/kjTree/kjSysAttrsRemove.h"                     // kjSysAttrsRemove
#include "orionld/apiModel/ntonEntity.h"                         // ntonEntity
#include "orionld/apiModel/ntosEntity.h"                         // ntosEntity
#include "orionld/apiModel/ntocEntity.h"                         // ntocEntity
#include "orionld/regMatch/regMatchForEntityGet.h"               // regMatchForEntityGet
#include "orionld/distOp/distOpListsMerge.h"                     // distOpListsMerge
#include "orionld/distOp/distOpSend.h"                           // distOpSend
#include "orionld/distOp/distOpLookupByCurlHandle.h"             // distOpLookupByCurlHandle
#include "orionld/distOp/distOpEntityMerge.h"                    // distOpEntityMerge
#include "orionld/distOp/distOpListRelease.h"                    // distOpListRelease
#include "orionld/distOp/xForwardedForCompose.h"                 // xForwardedForCompose
#include "orionld/distOp/viaCompose.h"                           // viaCompose
#include "orionld/serviceRoutines/orionldGetEntity.h"            // Own interface



// ----------------------------------------------------------------------------
//
// orionldGetEntity -
//
// URL PARAMETERS
// - attrs
// - geometryProperty
// - lang
// - options=simplified/concise  (normalized is default)
//
bool orionldGetEntity(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyGetEntity();

  bool         sysAttrs    = orionldState.uriParamOptions.sysAttrs;
  char*        lang        = orionldState.uriParams.lang;
  char*        entityType  = NULL;  // If the entity is found locally, its type will be included as help in the forwarded requests
  const char*  entityId    = orionldState.wildcard[0];

  if (pCheckUri(entityId, "Entity ID in URL PATH", true) == false)
    return false;

  //
  // If the entity type is set as URI param (only ONE), then it will be used, but first it needs to be expanded.
  // In the case the entity "entityId" exists, but with another type, it is not considered a hit - 404
  //
  if (orionldState.in.typeList.items > 0)
  {
    if (orionldState.in.typeList.items == 1)
      entityType = orionldState.in.typeList.array[0];
    else
    {
      orionldError(OrionldBadRequestData, "Invalid URI param (type)", "more than one entity type", 400);
      return false;
    }
  }

  KjNode* dbEntityP      = mongocEntityLookup(entityId, entityType, &orionldState.in.attrList, orionldState.uriParams.geometryProperty, NULL);
  KjNode* apiEntityP     = NULL;
  bool    forcedSysAttrs = false;

  if (dbEntityP != NULL)  // Convert from DB to API Entity + GET the entity type
  {
    // In the distributed case, one and the same attribute may come from different providers,
    // and we'll have to pick ONE of them.
    // The algorithm to pick one is;
    // 1. Newest observedAt
    // 2. If none of the attributes has an observedAt:  newest modifiedAt
    // 3. If none of the attributes has an modifiedAt - pick the first one
    //
    // So, in order for this to work, in a distributed GET, we can't get rid of the sysAttrs in dbModelToApiEntity2.
    // We also cannot get rid of the sub-attrs (observedAt may be needed) - that would be if orionldState.out.format == Simplified
    // We might need those two for the attribute-instance-decision
    //
    // For the very same reason, all forwarded GET requests must carry the URI-params "?options=sysAttrs,normalized" + type=X is available
    //

    if (entityType == NULL)
    {
      //
      // As we need the expanded entity type, we look it up in the DB Model version of the entity, not the API model version
      // AND, we must do this before calling dbModelToApiEntity2 as "id"/"type" are removed from "_id".
      //
      KjNode* _idP        = kjLookup(dbEntityP, "_id");
      KjNode* entityTypeP = (_idP != NULL)? kjLookup(_idP, "type") : NULL;

      if (entityTypeP != NULL)
        entityType = entityTypeP->value.s;
    }

    if (orionldState.distributed == true)
    {
      forcedSysAttrs = true;
      //
      // For forwarded requests, I NEED sysAttrs (to pick attribute in case there's more than one)
      // And, Normalized is the format for Distributed operations
      //
      apiEntityP = dbModelToApiEntity2(dbEntityP, true, RF_NORMALIZED, lang, true, &orionldState.pd);
    }
    else
      apiEntityP = dbModelToApiEntity2(dbEntityP, sysAttrs, orionldState.out.format, lang, true, &orionldState.pd);
  }

  DistOp*  distOpList = NULL;
  int      forwards       = 0;

  if (orionldState.distributed)
  {
    StringArray*  attrV   = (orionldState.uriParams.attrs != NULL)? &orionldState.in.attrList : NULL;
    const char*   geoProp = orionldState.uriParams.geometryProperty;

    char dateHeader[70];
    snprintf(dateHeader, sizeof(dateHeader), "Date: %s", orionldState.requestTimeString);

    //
    // If attrV AND geoProp are set, then geoProp needs to be merged in into attrV
    // UNLESS it's not present already.
    //
    // If attrV is not used BUT geoProp is, then attrV needs to be an array on ONE ITEM: geoProp
    //
    // This must be done as local copies, as otherwiose we fuck it up for the query lin the local database
    //
    //
    // If 'attrs' is used, any matches in Exclusive/Redirect registrations must chop off attributes from 'attrs'
    //
    // We want EVERYTHING from Auxiliary regs (and we may throw it all away in the end),
    // so we create the DistOp list FIRST, before any attrs are removed
    //
    DistOp* auxiliarList  = regMatchForEntityGet(RegModeAuxiliary, DoRetrieveEntity, entityId, entityType, attrV, geoProp);
    DistOp* exclusiveList = regMatchForEntityGet(RegModeExclusive, DoRetrieveEntity, entityId, entityType, attrV, geoProp);
    DistOp* redirectList  = regMatchForEntityGet(RegModeRedirect,  DoRetrieveEntity, entityId, entityType, attrV, geoProp);
    DistOp* inclusiveList = regMatchForEntityGet(RegModeInclusive, DoRetrieveEntity, entityId, entityType, attrV, geoProp);

    distOpList = distOpListsMerge(exclusiveList,  redirectList);
    distOpList = distOpListsMerge(distOpList, inclusiveList);
    distOpList = distOpListsMerge(distOpList, auxiliarList);

    // Enqueue all forwarded requests
    if (distOpList != NULL)
    {
      // Now that we've found all matching registrations we can add ourselves to the X-forwarded-For header
      char* xff = xForwardedForCompose(orionldState.in.xForwardedFor, localIpAndPort);
      char* via = viaCompose(orionldState.in.via, brokerId);

      for (DistOp* distOpP = distOpList; distOpP != NULL; distOpP = distOpP->next)
      {
        // Send the forwarded request and await all responses
        if (distOpP->regP != NULL)
        {
          LM_T(LmtDistOpAttributes, ("distOp::attrsParam: '%s'", distOpP->attrsParam));
          if (distOpSend(distOpP, dateHeader, xff, via, false, NULL) == 0)
          {
            ++forwards;
            distOpP->error = false;
          }
          else
          {
            LM_W(("Forwarded request failed"));
            distOpP->error = true;
          }
        }
      }

      int stillRunning = 1;
      int loops        = 0;

      while (stillRunning != 0)
      {
        CURLMcode cm = curl_multi_perform(orionldState.curlDoMultiP, &stillRunning);
        if (cm != 0)
        {
          LM_E(("Internal Error (curl_multi_perform: error %d)", cm));
          forwards = 0;
          break;
        }

        if (stillRunning != 0)
        {
          cm = curl_multi_wait(orionldState.curlDoMultiP, NULL, 0, 1000, NULL);
          if (cm != CURLM_OK)
          {
            LM_E(("Internal Error (curl_multi_wait: error %d", cm));
            break;
          }
        }

        if ((++loops >= 50) && ((loops % 25) == 0))
          LM_W(("curl_multi_perform doesn't seem to finish ... (%d loops)", loops));
      }

      if (loops >= 100)
        LM_W(("curl_multi_perform finally finished!   (%d loops)", loops));
    }
  }

  if (dbEntityP == NULL)
  {
    if (forwards == 0)
    {
      const char* title = (orionldState.in.attrList.items != 0)? "Combination Entity/Attributes Not Found" : "Entity Not Found";
      orionldError(OrionldResourceNotFound, title, entityId, 404);

      return false;
    }
  }


  //
  // Read the responses to the forwarded requests
  //
  int fwdMerges = 0;

  if (forwards > 0)
  {
    CURLMsg* msgP;
    int      msgsLeft;

    while ((msgP = curl_multi_info_read(orionldState.curlDoMultiP, &msgsLeft)) != NULL)
    {
      if (msgP->msg != CURLMSG_DONE)
        continue;

      if (msgP->data.result == CURLE_OK)
      {
        DistOp* distOpP = distOpLookupByCurlHandle(distOpList, msgP->easy_handle);

        curl_easy_getinfo(msgP->easy_handle, CURLINFO_RESPONSE_CODE, &distOpP->httpResponseCode);

        if ((distOpP->rawResponse != NULL) && (distOpP->rawResponse[0] != 0))
          distOpP->responseBody = kjParse(orionldState.kjsonP, distOpP->rawResponse);

        if (distOpP->responseBody != NULL)
        {
          if (distOpP->httpResponseCode >= 400)
          {
            LM_W(("Got an ERROR response (%d) from a forwarded request: '%s'", distOpP->httpResponseCode, distOpP->rawResponse));
            continue;
          }
          else if (distOpP->httpResponseCode != 200)
          {
            LM_W(("Got a non-200 response code (%d)", distOpP->httpResponseCode));
            continue;
          }

          if (distOpP->regP->acceptJsonld == true)
          {
            KjNode* atContextP = kjLookup(distOpP->responseBody, "@context");
            if (atContextP != NULL)
              kjChildRemove(distOpP->responseBody, atContextP);
          }

          //
          // If the original @context was not used when forwarding (a jsonldContext is present in the registration)
          // then we now must expand the entity using the context 'distOpP->regP->contextP'
          // and then compact it again, using the @context of the original request
          //
          if ((distOpP->regP->contextP != NULL) && (distOpP->regP->contextP != orionldState.contextP))
          {
            orionldEntityExpand(distOpP->responseBody, distOpP->regP->contextP);
            orionldEntityCompact(distOpP->responseBody, orionldState.contextP);
          }

          // If the mode of the registration is Auxiliar, then the merge must be delayed
          if (distOpP->regP->mode == RegModeAuxiliary)
          {
            continue;
          }

          // Merge in the received body into the local (or nothing)
          if (apiEntityP == NULL)
            apiEntityP = distOpP->responseBody;
          else
          {
            distOpEntityMerge(apiEntityP, distOpP->responseBody, sysAttrs, distOpP->regP->mode == RegModeAuxiliary);
            fwdMerges += 1;
          }
        }
        else
          LM_E(("Internal Error (parse error for the received response of a forwarded request)"));
      }
      else
      {
        DistOp* distOpP = distOpLookupByCurlHandle(distOpList, msgP->easy_handle);
        LM_E(("CURL Error %d awaiting response to forwarded request (reg: %s): %s",
              msgP->data.result,
              distOpP->regP->regId,
              curl_easy_strerror(msgP->data.result)));
      }
    }

    //
    // Now we take the responses from auxiliar registrations
    //
    for (DistOp* fwdP = distOpList; fwdP != NULL; fwdP = fwdP->next)
    {
      if (fwdP->regP->mode != RegModeAuxiliary)
        continue;

      if (apiEntityP == NULL)
        apiEntityP = fwdP->responseBody;
      else
        distOpEntityMerge(apiEntityP, fwdP->responseBody, sysAttrs, fwdP->regP->mode == RegModeAuxiliary);
    }

    distOpListRelease(distOpList);
  }

  if ((apiEntityP != NULL) && (forcedSysAttrs == true))
  {
    // Transform the apiEntityP according to in case orionldState.out.format, lang, and sysAttrs

    if      (orionldState.out.format == RF_SIMPLIFIED) ntosEntity(apiEntityP, lang);
    else if (orionldState.out.format == RF_CONCISE)    ntocEntity(apiEntityP, lang, sysAttrs);
    else                                               ntonEntity(apiEntityP, lang, sysAttrs);
  }

  if ((apiEntityP != NULL) && (sysAttrs == false))
    kjSysAttrsRemove(apiEntityP, 2);

  if (orionldState.out.contentType == MT_GEOJSON)
  {
    apiEntityP = kjGeojsonEntityTransform(apiEntityP,
                                          orionldState.geoPropertyNode,
                                          orionldState.link,
                                          orionldState.preferHeader,
                                          orionldState.uriParams.geometryProperty,
                                          orionldState.geoPropertyMissing,
                                          orionldState.linkHeaderAdded,
                                          orionldState.out.format == RF_CONCISE,
                                          orionldState.contextP->url);

    //
    // If URI params 'attrs' and 'geometryProperty' are given BUT 'geometryProperty' is not part of 'attrs', then we need to remove 'geometryProperty' from
    // the "properties" object
    //
    if ((orionldState.in.attrList.items > 0) && (orionldState.uriParams.geometryProperty != NULL))
    {
      bool geometryPropertyInAttrList = false;

      for (int ix = 0; ix < orionldState.in.attrList.items; ix++)
      {
        if (strcmp(orionldState.in.geometryPropertyExpanded, orionldState.in.attrList.array[ix]) == 0)
        {
          geometryPropertyInAttrList = true;
          break;
        }
      }

      if (geometryPropertyInAttrList == false)
      {
        KjNode* propertiesP = kjLookup(apiEntityP, "properties");

        if (propertiesP != NULL)
        {
          KjNode* geometryPropertyP = kjLookup(propertiesP, orionldState.in.geometryPropertyExpanded);

          if (geometryPropertyP != NULL)
            kjChildRemove(propertiesP, geometryPropertyP);
        }
      }
    }
  }

  //
  // If a tenant is in use, it must be included in the response
  //
  if (orionldState.tenantP != &tenant0)
    orionldHeaderAdd(&orionldState.out.headers, HttpTenant, orionldState.tenantP->tenant, 0);

  if (apiEntityP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Entity not found", entityId, 404);
    return false;
  }

  orionldState.responseTree   = apiEntityP;
  orionldState.httpStatusCode = 200;
  return true;
}
