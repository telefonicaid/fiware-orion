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

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/curlToBrokerStrerror.h"                 // curlToBrokerStrerror
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
#include "orionld/forwarding/ForwardPending.h"                   // ForwardPending
#include "orionld/forwarding/regMatchForEntityGet.h"             // regMatchForEntityGet
#include "orionld/forwarding/forwardingListsMerge.h"             // forwardingListsMerge
#include "orionld/forwarding/forwardRequestSend.h"               // forwardRequestSend
#include "orionld/forwarding/fwdPendingLookupByCurlHandle.h"     // fwdPendingLookupByCurlHandle
#include "orionld/forwarding/fwdEntityMerge.h"                   // fwdEntityMerge
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

  const char* entityId = orionldState.wildcard[0];
  if (pCheckUri(entityId, "Entity ID in URL PATH", true) == false)
    return false;

  bool             distributed    = (forwarding == true) && (orionldState.uriParams.local == false);
  ForwardPending*  fwdPendingList = NULL;
  int              forwards       = 0;

  if (distributed)
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
    // so we create the ForwardPending list FIRST, before any attrs are removed
    //
    ForwardPending* auxiliarList  = regMatchForEntityGet(RegModeAuxiliary, FwdRetrieveEntity, entityId, attrV, geoProp);
    ForwardPending* exclusiveList = regMatchForEntityGet(RegModeExclusive, FwdRetrieveEntity, entityId, attrV, geoProp);
    ForwardPending* redirectList  = regMatchForEntityGet(RegModeRedirect,  FwdRetrieveEntity, entityId, attrV, geoProp);
    ForwardPending* inclusiveList = regMatchForEntityGet(RegModeInclusive, FwdRetrieveEntity, entityId, attrV, geoProp);

    fwdPendingList = forwardingListsMerge(exclusiveList,  redirectList);
    fwdPendingList = forwardingListsMerge(fwdPendingList, inclusiveList);
    fwdPendingList = forwardingListsMerge(fwdPendingList, auxiliarList);

    // Send - copy from orionldPostEntities
    if (fwdPendingList != NULL)
    {
      for (ForwardPending* fwdPendingP = fwdPendingList; fwdPendingP != NULL; fwdPendingP = fwdPendingP->next)
      {
        // Send the forwarded request and await all responses
        if (fwdPendingP->regP != NULL)
        {
          if (forwardRequestSend(fwdPendingP, dateHeader) == 0)
          {
            ++forwards;
            fwdPendingP->error = false;
          }
          else
          {
            LM_W(("Forwarded request failed"));
            fwdPendingP->error = true;
          }
        }
      }

      int stillRunning = 1;
      int loops        = 0;

      while (stillRunning != 0)
      {
        CURLMcode cm = curl_multi_perform(orionldState.curlFwdMultiP, &stillRunning);
        if (cm != 0)
        {
          LM_E(("Internal Error (curl_multi_perform: error %d)", cm));
          forwards = 0;
          break;
        }

        if (stillRunning != 0)
        {
          cm = curl_multi_wait(orionldState.curlFwdMultiP, NULL, 0, 1000, NULL);
          if (cm != CURLM_OK)
          {
            LM_E(("Internal Error (curl_multi_wait: error %d", cm));
            break;
          }
        }

        if ((++loops >= 50) && ((loops % 10) == 0))
          LM_W(("curl_multi_perform doesn't seem to finish ... (%d loops)", loops));
      }

      if (loops >= 50)
        LM_W(("curl_multi_perform finally finished!"));
    }
  }

  KjNode* dbEntityP  = mongocEntityLookup(entityId, &orionldState.in.attrList, orionldState.uriParams.geometryProperty);
  KjNode* apiEntityP = NULL;
  if (dbEntityP == NULL)
  {
    if (forwards == 0)
    {
      const char* title = (orionldState.in.attrList.items != 0)? "Combination Entity/Attributes Not Found" : "Entity Not Found";
      orionldError(OrionldResourceNotFound, title, entityId, 404);

#if 0
      LM(("@context is: '%s'", orionldState.contextP->url));

      for (int ix = 0; ix < orionldState.in.attrList.items; ix++)
      {
        LM(("Attr %d: '%s'", ix, orionldState.in.attrList.array[ix]));
      }
#endif
      return false;
    }
  }
  else
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
    // For the very same reason, all forwarded GET requests must carry the URI-params "?options=sysAttrs,normalized"
    //
    if (forwards > 0)  // Need Normalized and sysattrs if the operation has parts of the entity distributed (to help pick attr instances)
      apiEntityP = dbModelToApiEntity2(dbEntityP, true, RF_NORMALIZED, orionldState.uriParams.lang, true, &orionldState.pd);
    else
      apiEntityP = dbModelToApiEntity2(dbEntityP, orionldState.uriParamOptions.sysAttrs, orionldState.out.format, orionldState.uriParams.lang, true, &orionldState.pd);
  }

  //
  // Now read responses to the forwarded requests
  //
  if (forwards > 0)
  {
    CURLMsg* msgP;
    int      msgsLeft;

    while ((msgP = curl_multi_info_read(orionldState.curlFwdMultiP, &msgsLeft)) != NULL)
    {
      if (msgP->msg != CURLMSG_DONE)
        continue;

      if (msgP->data.result == CURLE_OK)
      {
        ForwardPending* fwdPendingP = fwdPendingLookupByCurlHandle(fwdPendingList, msgP->easy_handle);

        LM(("Got a response: %s", fwdPendingP->rawResponse));
        fwdPendingP->body = kjParse(orionldState.kjsonP, fwdPendingP->rawResponse);
        if (fwdPendingP->body != NULL)
        {
          if (fwdPendingP->regP->acceptJsonld == true)
          {
            KjNode* atContextP = kjLookup(fwdPendingP->body, "@context");
            if (atContextP != NULL)
              kjChildRemove(fwdPendingP->body, atContextP);
          }

          //
          // If the original @context was not used when forwarding (a jsonldContext is present in the registration)
          // then we now must expand the entity using the context 'fwdPendingP->regP->contextP'
          // and then compact it again, using the @context of the original request
          //
          if ((fwdPendingP->regP->contextP != NULL) && (fwdPendingP->regP->contextP != orionldState.contextP))
          {
            orionldEntityExpand(fwdPendingP->body, fwdPendingP->regP->contextP);
            orionldEntityCompact(fwdPendingP->body, orionldState.contextP);
          }

          // Merge in the received body into the local (or nothing)
          if (apiEntityP == NULL)
            apiEntityP = fwdPendingP->body;
          else
            fwdEntityMerge(apiEntityP, fwdPendingP->body, orionldState.uriParamOptions.sysAttrs, fwdPendingP->regP->mode == RegModeAuxiliary);
        }
        else
          LM_E(("Internal Error (parse error for the received response of a forwarded request)"));
      }
      else
        LM_E(("CURL Error %d awaiting response to forwarded request: %s", msgP->data.result, curl_easy_strerror(msgP->data.result)));
    }

    kjTreeLog(apiEntityP, "API entity after forwarding");
  }

  if (forwards > 0)
  {
    kjTreeLog(apiEntityP, "API Entity to transform");
    // Transform the apiEntityP according to in case orionldState.out.format, lang, and sysAttrs
    bool  sysAttrs = orionldState.uriParamOptions.sysAttrs;
    char* lang     = orionldState.uriParams.lang;

    if (orionldState.out.format == RF_KEYVALUES)
      ntosEntity(apiEntityP, lang);
    else if (orionldState.out.format == RF_CONCISE)
      ntocEntity(apiEntityP, lang, sysAttrs);
    else
      ntonEntity(apiEntityP, lang, sysAttrs);
  }

  if (orionldState.out.contentType == GEOJSON)
  {
    apiEntityP = kjGeojsonEntityTransform(apiEntityP, orionldState.geoPropertyNode);

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

  orionldState.responseTree   = apiEntityP;
  orionldState.httpStatusCode = 200;

  return true;
}
