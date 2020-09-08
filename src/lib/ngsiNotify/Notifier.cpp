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
* Author: Fermin Galan
*/
#include <vector>

#include <curl/curl.h>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/string.h"
#include "common/statistics.h"
#include "common/limits.h"
#include "common/RenderFormat.h"
#include "common/macroSubstitute.h"
#include "alarmMgr/alarmMgr.h"
#include "apiTypesV2/HttpInfo.h"
#include "ngsi10/NotifyContextRequest.h"
#include "ngsiNotify/senderThread.h"
#include "rest/uriParamNames.h"
#include "rest/ConnectionInfo.h"
#include "rest/httpHeaderAdd.h"

#ifdef ORIONLD
extern "C" {
#include "kjson/kjRender.h"                                    // kjRender
#include "kjson/kjson.h"                                       // Kjson
}
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/context/orionldCoreContext.h"                // ORIONLD_CORE_CONTEXT_URL
#include "orionld/kjTree/kjTreeFromNotification.h"             // kjTreeFromNotification
#include "cache/subCache.h"                                    // CachedSubscription
#endif

#include "ngsiNotify/Notifier.h"



/* ****************************************************************************
*
* ~Notifier -
*/
Notifier::~Notifier (void)
{
  //
  // NOTE:
  //   This destructor is needed to avoid a warning message.
  //   The compilation fails when a warning occurs as the compilation option
  //     -Werror "warnings being treated as errors"
  //   is enabled
  //
  LM_T(LmtNotImplemented, ("Notifier destructor is not implemented"));
}


/* ****************************************************************************
*
* Notifier::sendNotifyContextRequest -
*/
void Notifier::sendNotifyContextRequest
(
    NotifyContextRequest*            ncrP,
    const ngsiv2::HttpInfo&          httpInfo,
    const std::string&               tenant,
    const std::string&               xauthToken,
    const std::string&               fiwareCorrelator,
    RenderFormat                     renderFormat,
    const std::vector<std::string>&  attrsOrder,
    const std::vector<std::string>&  metadataFilter,
    bool                             blackList
)
{
  pthread_t                         tid;

  std::vector<SenderThreadParams*>* paramsV = Notifier::buildSenderParams(ncrP,
                                                                          httpInfo,
                                                                          tenant,
                                                                          xauthToken,
                                                                          fiwareCorrelator,
                                                                          renderFormat,
                                                                          attrsOrder,
                                                                          metadataFilter,
                                                                          blackList);

  if (!paramsV->empty()) // al least one param, an empty vector means an error occurred
  {
    int ret = pthread_create(&tid, NULL, startSenderThread, paramsV);

    if (ret != 0)
    {
      LM_E(("Runtime Error (error creating thread: %d)", ret));
      for (unsigned ix = 0; ix < paramsV->size(); ix++)
      {
        delete (*paramsV)[ix];
      }
      delete paramsV;
      return;
    }
    pthread_detach(tid);
  }
}



/* ****************************************************************************
*
* Notifier::sendNotifyContextAvailabilityRequest -
*
* FIXME: this method is very similar to sendNotifyContextRequest and probably
* they could be refactored in the future to have a common part using a parent
* class for both types of notifications and using it as first argument
*/
void Notifier::sendNotifyContextAvailabilityRequest
(
  NotifyContextAvailabilityRequest*  ncar,
  const std::string&                 url,
  const std::string&                 tenant,
  const std::string&                 fiwareCorrelator,
  RenderFormat                       renderFormat
)
{
    /* Render NotifyContextAvailabilityRequest */
    std::string payload = ncar->render();

    /* Parse URL */
    std::string  host;
    int          port;
    std::string  uriPath;
    std::string  protocol;

    if (!parseUrl(url, host, port, uriPath, protocol))
    {
      std::string details = std::string("sending NotifyContextAvailabilityRequest: malformed URL: '") + url + "'";
      alarmMgr.badInput(clientIp, details);

      return;
    }

    /* Set Content-Type */
    std::string content_type = "application/json";

    /* Send the message (without awaiting response, in a separate thread to avoid blocking) */
    pthread_t            tid;
    SenderThreadParams*  params = new SenderThreadParams();

    params->ip               = host;
    params->port             = port;
    params->protocol         = protocol;
    params->verb             = "POST";
    params->tenant           = tenant;
    params->resource         = uriPath;
    params->content_type     = content_type;
    params->content          = payload;
    params->mimeType         = JSON;
    params->fiwareCorrelator = fiwareCorrelator;
    params->renderFormat     = renderFormatToString(renderFormat);
    params->registration     = true;

    strncpy(params->transactionId, transactionId, sizeof(params->transactionId));

    std::vector<SenderThreadParams*>* paramsV = new std::vector<SenderThreadParams*>;
    paramsV->push_back(params);

    int ret = pthread_create(&tid, NULL, startSenderThread, paramsV);
    if (ret != 0)
    {
      LM_E(("Runtime Error (error creating thread: %d)", ret));
      return;
    }
    pthread_detach(tid);
}




/* ****************************************************************************
*
* buildSenderParamsCustom -
*/
static std::vector<SenderThreadParams*>* buildSenderParamsCustom
(
    const SubscriptionId&                subscriptionId,
    const ContextElementResponseVector&  cv,
    const ngsiv2::HttpInfo&              httpInfo,
    const std::string&                   tenant,
    const std::string&                   xauthToken,
    const std::string&                   fiwareCorrelator,
    RenderFormat                         renderFormat,
    const std::vector<std::string>&      attrsOrder,
    const std::vector<std::string>&      metadataFilter
)
{
  std::vector<SenderThreadParams*>*  paramsV;

  paramsV = new std::vector<SenderThreadParams*>;

  for (unsigned ix = 0; ix < cv.size(); ix++)
  {
    Verb                                verb    = httpInfo.verb;
    std::string                         method;
    std::string                         url;
    std::string                         payload;
    std::string                         mimeType;
    std::map<std::string, std::string>  qs;
    std::map<std::string, std::string>  headers;
    const ContextElement&               ce      = cv[ix]->contextElement;

    //
    // 1. Verb/Method
    //
    if (verb == NOVERB)
    {
      // Default verb/method is POST
      verb = POST;
    }
    method = verbName(verb);


    //
    // 2. URL
    //
    if (macroSubstitute(&url, httpInfo.url, ce) == false)
    {
      // Warning already logged in macroSubstitute()
      return paramsV;  // empty vector
    }


    //
    // 3. Payload
    //
    if (httpInfo.payload == "")
    {
      NotifyContextRequest   ncr;
      ContextElementResponse cer;

      cer.contextElement  = ce;
      cer.statusCode.code = SccOk;

      ncr.subscriptionId  = subscriptionId;
      ncr.contextElementResponseVector.push_back(&cer);

      if (renderFormat == NGSI_V1_LEGACY)
      {
        payload = ncr.render(V1, false);
      }
      else
      {
        payload  = ncr.toJson(renderFormat, attrsOrder, metadataFilter);
      }

      mimeType = "application/json";
    }
    else
    {
      if (macroSubstitute(&payload, httpInfo.payload, ce) == false)
      {
        // Warning already logged in macroSubstitute()
        return paramsV;  // empty vector
      }

      char* pload  = curl_unescape(payload.c_str(), payload.length());
      payload      = std::string(pload);
      renderFormat = NGSI_V2_CUSTOM;
      mimeType     = "text/plain";  // May be overridden by 'Content-Type' in 'headers'
      curl_free(pload);
    }


    //
    // 4. URI Params (Query Strings)
    //
    for (std::map<std::string, std::string>::const_iterator it = httpInfo.qs.begin(); it != httpInfo.qs.end(); ++it)
    {
      std::string key   = it->first;
      std::string value = it->second;

      if ((macroSubstitute(&key, it->first, ce) == false) || (macroSubstitute(&value, it->second, ce) == false))
      {
        // Warning already logged in macroSubstitute()
        return paramsV;  // empty vector
      }

      if ((value == "") || (key == ""))
      {
        // To avoid e.g '?a=&b=&c='
        continue;
      }
      qs[key] = value;
    }


    //
    // 5. HTTP Headers
    //
    for (std::map<std::string, std::string>::const_iterator it = httpInfo.headers.begin(); it != httpInfo.headers.end(); ++it)
    {
      std::string key   = it->first;
      std::string value = it->second;

      if ((macroSubstitute(&key, it->first, ce) == false) || (macroSubstitute(&value, it->second, ce) == false))
      {
        // Warning already logged in macroSubstitute()
        return paramsV;  // empty vector
      }

      if (key == "")
      {
        // To avoid empty header name
        continue;
      }

      std::transform(key.begin(), key.end(), key.begin(), ::tolower);
      headers[key] = value;
    }


    //
    // 6. Split URI in protocol, host, port and path
    //
    std::string  protocol;
    std::string  host;
    int          port;
    std::string  uriPath;

    if (!parseUrl(url, host, port, uriPath, protocol))
    {
      LM_E(("Runtime Error (not sending NotifyContextRequest: malformed URL: '%s')", httpInfo.url.c_str()));
      return paramsV;  // empty vector
    }


    //
    // 7. Add URI params from template to uriPath
    //
    std::string  uri = uriPath;

    if (qs.size() != 0)
    {
      uri += "?";

      int ix = 0;
      for (std::map<std::string, std::string>::iterator it = qs.begin(); it != qs.end(); ++it)
      {
        if (ix != 0)
        {
          uri += "&";
        }

        uri += it->first + "=" + it->second;
        ++ix;
      }
    }

    SenderThreadParams*  params = new SenderThreadParams();

    params->ip               = host;
    params->port             = port;
    params->protocol         = protocol;
    params->verb             = method;
    params->tenant           = tenant;
    params->servicePath      = ce.entityId.servicePath;
    params->xauthToken       = xauthToken;
    params->resource         = uri;
    params->content_type     = mimeType;
    params->content          = payload;
    params->mimeType         = JSON;
    params->renderFormat     = renderFormatToString(renderFormat);
    params->fiwareCorrelator = fiwareCorrelator;
    params->extraHeaders     = headers;
    params->registration     = false;
    params->subscriptionId   = subscriptionId.get();

    paramsV->push_back(params);
  }

  return paramsV;
}



/* ****************************************************************************
*
* Notifier::buildSenderParams -
*/
std::vector<SenderThreadParams*>* Notifier::buildSenderParams
(
  NotifyContextRequest*            ncrP,
  const ngsiv2::HttpInfo&          httpInfo,
  const std::string&               tenant,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsOrder,
  const std::vector<std::string>&  metadataFilter,
  bool                             blackList
)
{
    ConnectionInfo                    ci;
    Verb                              verb    = httpInfo.verb;
    std::vector<SenderThreadParams*>* paramsV = NULL;
#ifdef ORIONLD
    CachedSubscription*  subP    = NULL;
    char*                toFree  = NULL;
#endif

    if ((verb == NOVERB) || (verb == UNKNOWNVERB) || disableCusNotif)
    {
      // Default verb/method (or the one in case of disabled custom notifications) is POST
      verb = POST;
    }

    //
    // If any of the 'template parameters' is used by the subscripion, then it is not a question of an ordinary notification but one using templates.
    // Ordinary notifications are simply sent, all of it as one message.
    //
    // Notifications for subscriptions using templates are more complex:
    //   - if there is more than one entity for the notification, split into N notifications, one per entity
    //   - if 'url' contains substitution keys, substitute the keys for their current values
    //   - if 'qs' contains query strings, add this to 'url', with the proper substitutions done
    //   - if 'headers' is non-empty, perform eventual substitutions and make sure the information is added as HTTP headers for the notification
    //   - if 'payload' is given, use that string as template instead of the default payload string, substituting all fields that are to be substituted
    //   - if 'method' is given, then a custom HTTP method is used (instead of POST, which is default)
    //
    //
    // Note that disableCusNotif (taken from CLI) could disable custom notifications and force to use regular ones
    //
    if (httpInfo.custom && !disableCusNotif)
    {
      return buildSenderParamsCustom(ncrP->subscriptionId,
                                     ncrP->contextElementResponseVector,
                                     httpInfo,
                                     tenant,
                                     xauthToken,
                                     fiwareCorrelator,
                                     renderFormat,
                                     attrsOrder,
                                     metadataFilter);
    }

    paramsV = new std::vector<SenderThreadParams*>();

    //
    // Creating the value of the Fiware-ServicePath HTTP header.
    // This is a comma-separated list of the service-paths in the same order as the entities come in the payload
    //
    std::string spathList;
    bool        atLeastOneNotDefault = false;

    for (unsigned int ix = 0; ix < ncrP->contextElementResponseVector.size(); ++ix)
    {
      EntityId* eP = &ncrP->contextElementResponseVector[ix]->contextElement.entityId;

      if (spathList != "")
      {
        spathList += ",";
      }
      spathList += eP->servicePath;
      atLeastOneNotDefault = atLeastOneNotDefault || (eP->servicePath != "/");
    }

    //
    // FIXME P8: the stuff about atLeastOneNotDefault was added after PR #729, which makes "/" the default servicePath in
    // request not having that header. However, this causes as side-effect that a
    // "Fiware-ServicePath: /" or "Fiware-ServicePath: /,/" header is added in notifications, thus breaking several tests harness.
    // Given that the "clean" implementation of Fiware-ServicePath propagation will be implemented
    // soon (it has been scheduled for version 0.19.0, see https://github.com/telefonicaid/fiware-orion/issues/714)
    // we introduce the atLeastOneNotDefault hack. Once #714 gets implemented,
    // this FIXME will be removed (and all the test harness adjusted, if needed)
    //
    if (!atLeastOneNotDefault)
    {
      spathList = "";
    }

    ci.outMimeType = JSON;

    std::string payloadString;
    if (renderFormat == NGSI_V1_LEGACY)
    {
      bool asJsonObject = (ci.uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ci.outMimeType == JSON);
      payloadString = ncrP->render(ci.apiVersion, asJsonObject);
    }
#ifdef ORIONLD
    else if ((renderFormat == NGSI_LD_V1_NORMALIZED) || (renderFormat == NGSI_LD_V1_KEYVALUES))
    {
      subP = subCacheItemLookup(tenant.c_str(), ncrP->subscriptionId.c_str());
      if (subP == NULL)
      {
        LM_E(("Unable to find subscription: %s", ncrP->subscriptionId.c_str()));
        return paramsV;
      }

      char*    details;
      KjNode*  kjTree = kjTreeFromNotification(ncrP, subP->ldContext.c_str(), subP->httpInfo.mimeType, subP->renderFormat, &details);

      if (kjTree == NULL)
      {
        LM_E(("kjTreeFromNotification error: %s", details));
        return paramsV;
      }

      int   bufSize = 512 * 1024;
      char* buf     = (char*) malloc(bufSize);

      kjRender(orionldState.kjsonP, kjTree, buf, bufSize);
      payloadString = buf;
      toFree        = buf;
    }
#endif
    else
    {
      payloadString = ncrP->toJson(renderFormat, attrsOrder, metadataFilter, blackList);
    }

    /* Parse URL */
    std::string  host;
    int          port;
    std::string  uriPath;
    std::string  protocol;

    if (!parseUrl(httpInfo.url, host, port, uriPath, protocol))
    {
      LM_E(("Runtime Error (not sending NotifyContextRequest: malformed URL: '%s')", httpInfo.url.c_str()));
      return paramsV;  //empty vector
    }

    /* Set Content-Type */
    std::string content_type = (httpInfo.mimeType == JSONLD)? "application/ld+json" : "application/json";


    SenderThreadParams*  params = new SenderThreadParams();

    params->ip               = host;
    params->port             = port;
    params->protocol         = protocol;
    params->verb             = verbName(verb);
    params->tenant           = tenant;
    params->servicePath      = spathList;
    params->xauthToken       = xauthToken;
    params->resource         = uriPath;
    params->content_type     = content_type;
    params->content          = payloadString;
#ifdef ORIONLD
    params->toFree           = toFree;
    params->mimeType         = httpInfo.mimeType;
#else
    params->mimeType         = JSON;
#endif
    params->renderFormat     = renderFormatToString(renderFormat);
    params->fiwareCorrelator = fiwareCorrelator;
    params->subscriptionId   = ncrP->subscriptionId.get();
    params->registration     = false;

#ifdef ORIONLD
    //
    // If mime-type is application/ld+json, then the @context goes in the payload
    // If application/json, then @context goes in the "LInk" HTTP Header
    // - Only if the subscription is ngsi-ld, of course
    //

    if (subP == NULL)
      subP = subCacheItemLookup(tenant.c_str(), ncrP->subscriptionId.c_str());

    //
    // This is where the Link HTTP header is added, for ngsi-ld subscriptions only
    //
    if (subP != NULL)
    {
      if (subP->ldContext != "")  // Subscriptions created using ngsi-ld have a context. those from APIV1/2 do not
      {
        if (httpInfo.mimeType == JSON)
        {
          if (subP->ldContext == "")
            params->extraHeaders["Link"] = std::string("<") + ORIONLD_CORE_CONTEXT_URL + ">; " + LINK_REL_AND_TYPE;
          else
            params->extraHeaders["Link"] = std::string("<") + subP->ldContext + ">; " + LINK_REL_AND_TYPE;
        }
        else
        {
          //
          // Not that this would ever happen, but, what do we do here ?
          // Best choice seems to be to simply send the notification without the Link header.
          //
        }
      }
    }


    //
    // HTTP Headers defined for the subscription
    //
    for (std::map<std::string, std::string>::const_iterator it = httpInfo.headers.begin(); it != httpInfo.headers.end(); ++it)
    {
      std::string key   = it->first;
      std::string value = it->second;

      if ((key == "") || (key == "Link"))  // To avoid empty header name AND an extra Link header
        continue;

      params->extraHeaders[key] = value;
    }

#endif

    paramsV->push_back(params);
    return paramsV;
}
