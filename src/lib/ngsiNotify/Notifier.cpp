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
#include "ngsiNotify/Notifier.h"



/* ****************************************************************************
*
* ~Notifier -
*/
Notifier::~Notifier (void)
{
  // FIXME: This destructor is needed to avoid warning message.
  // Compilation fails when a warning occurs, and it is enabled
  // compilation option -Werror "warnings being treated as errors"
  LM_T(LmtNotImplemented, ("Notifier destructor is not implemented"));
}


/* ****************************************************************************
*
* Notifier::sendNotifyContextRequest -
*/
void Notifier::sendNotifyContextRequest
(
    NotifyContextRequest&            ncr,
    const ngsiv2::HttpInfo&          httpInfo,
    const std::string&               tenant,
    long long                        maxFailsLimit,
    long long                        failsCounter,
    const std::string&               xauthToken,
    const std::string&               fiwareCorrelator,
    unsigned int                     correlatorCounter,
    RenderFormat                     renderFormat,
    const std::vector<std::string>&  attrsFilter,
    bool                             blacklist,
    const std::vector<std::string>&  metadataFilter
)
{
  pthread_t                         tid;
  std::vector<SenderThreadParams*>* paramsV = Notifier::buildSenderParams(ncr,
                                                                          httpInfo,
                                                                          tenant,
                                                                          maxFailsLimit,
                                                                          failsCounter,
                                                                          xauthToken,
                                                                          fiwareCorrelator,
                                                                          correlatorCounter,
                                                                          renderFormat,
                                                                          attrsFilter,
                                                                          blacklist,
                                                                          metadataFilter);

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
* buildSenderParamsCustom -
*/
static std::vector<SenderThreadParams*>* buildSenderParamsCustom
(
    const SubscriptionId&                subscriptionId,
    const ContextElementResponseVector&  cv,
    const ngsiv2::HttpInfo&              httpInfo,
    const std::string&                   tenant,
    long long                            maxFailsLimit,
    long long                        failsCounter,
    const std::string&                   xauthToken,
    const std::string&                   fiwareCorrelator,
    unsigned int                         correlatorCounter,
    RenderFormat                         renderFormat,
    const std::vector<std::string>&      attrsFilter,
    bool                                 blacklist,
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
    Entity&                             en      = cv[ix]->entity;

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
    if (macroSubstitute(&url, httpInfo.url, en) == false)
    {
      // Warning already logged in macroSubstitute()
      return paramsV;  // empty vector
    }


    //
    // 3. Payload
    //
    if (!httpInfo.includePayload)
    {
      payload      = "";
      renderFormat = NGSI_V2_CUSTOM;
    }
    else if (httpInfo.payload.empty())
    {
      NotifyContextRequest   ncr;
      ContextElementResponse cer;

      cer.entity.fill(en.id, en.type, en.isPattern, en.servicePath);
      cer.entity.attributeVector.push_back(en.attributeVector);

      cer.statusCode.code = SccOk;

      ncr.subscriptionId  = subscriptionId;
      ncr.contextElementResponseVector.push_back(&cer);

      if (renderFormat == NGSI_V1_LEGACY)
      {
        payload = ncr.toJsonV1(false, attrsFilter, blacklist, metadataFilter);
      }
      else
      {
        payload  = ncr.toJson(renderFormat, attrsFilter, blacklist, metadataFilter);
      }

      mimeType = "application/json";
    }
    else
    {
      if (macroSubstitute(&payload, httpInfo.payload, en) == false)
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

      if ((macroSubstitute(&key, it->first, en) == false) || (macroSubstitute(&value, it->second, en) == false))
      {
        // Warning already logged in macroSubstitute()
        return paramsV;  // empty vector
      }

      if ((value.empty()) || (key.empty()))
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

      if ((macroSubstitute(&key, it->first, en) == false) || (macroSubstitute(&value, it->second, en) == false))
      {
        // Warning already logged in macroSubstitute()
        return paramsV;  // empty vector
      }

      if (key.empty())
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

    params->type             = QUEUE_MSG_NOTIF;
    params->from             = fromIp;  // note fromIp is a thread variable
    params->ip               = host;
    params->port             = port;
    params->protocol         = protocol;
    params->verb             = method;
    params->tenant           = tenant;
    params->maxFailsLimit    = maxFailsLimit;
    params->failsCounter     = failsCounter;
    params->servicePath      = en.servicePath;
    params->xauthToken       = xauthToken;
    params->resource         = uri;
    params->content_type     = mimeType;
    params->content          = payload;
    params->mimeType         = JSON;
    params->renderFormat     = renderFormatToString(renderFormat);
    params->extraHeaders     = headers;
    params->registration     = false;
    params->subscriptionId   = subscriptionId.get();

    char suffix[STRING_SIZE_FOR_INT];
    snprintf(suffix, sizeof(suffix), "%u", correlatorCounter);
    params->fiwareCorrelator = fiwareCorrelator + "; cbnotif=" + suffix;

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
  NotifyContextRequest&            ncr,
  const ngsiv2::HttpInfo&          httpInfo,
  const std::string&               tenant,
  long long                        maxFailsLimit,
  long long                        failsCounter,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  unsigned int                     correlatorCounter,
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  const std::vector<std::string>&  metadataFilter
)
{
    ConnectionInfo                    ci;
    Verb                              verb    = httpInfo.verb;
    std::vector<SenderThreadParams*>* paramsV = NULL;

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
      return buildSenderParamsCustom(ncr.subscriptionId,
                                     ncr.contextElementResponseVector,
                                     httpInfo,
                                     tenant,
                                     maxFailsLimit,
                                     failsCounter,
                                     xauthToken,
                                     fiwareCorrelator,
                                     correlatorCounter,
                                     renderFormat,
                                     attrsFilter,
                                     blacklist,
                                     metadataFilter);
    }

    paramsV = new std::vector<SenderThreadParams*>();

    //
    // Creating the value of the Fiware-ServicePath HTTP header.
    // Since the removal of initial notification feature, notifications are mono-entity
    // by construction, i.e. ncr vector has only one element.
    //
    // FIXME P3: not sure if the check on ncr size > 0 is done in some previous point
    // In that case the if guard could be removed
    //
    std::string spath;

    if (ncr.contextElementResponseVector.size() > 0)
    {
      spath = ncr.contextElementResponseVector[0]->entity.servicePath;
    }

    ci.outMimeType = JSON;

    std::string payloadString;
    if (renderFormat == NGSI_V1_LEGACY)
    {
      bool asJsonObject = (ci.uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ci.outMimeType == JSON);
      payloadString = ncr.toJsonV1(asJsonObject, attrsFilter, blacklist, metadataFilter);
    }
    else
    {
      payloadString = ncr.toJson(renderFormat, attrsFilter, blacklist, metadataFilter);
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
    std::string content_type = "application/json";


    SenderThreadParams*  params = new SenderThreadParams();

    params->type             = QUEUE_MSG_NOTIF;
    params->from             = fromIp;  // note fromIp is a thread variable
    params->ip               = host;
    params->port             = port;
    params->protocol         = protocol;
    params->verb             = verbName(verb);
    params->tenant           = tenant;
    params->maxFailsLimit    = maxFailsLimit;
    params->failsCounter     = failsCounter;
    params->servicePath      = spath;
    params->xauthToken       = xauthToken;
    params->resource         = uriPath;
    params->content_type     = content_type;
    params->content          = payloadString;
    params->mimeType         = JSON;
    params->renderFormat     = renderFormatToString(renderFormat);
    params->subscriptionId   = ncr.subscriptionId.get();
    params->registration     = false;

    char suffix[STRING_SIZE_FOR_INT];
    snprintf(suffix, sizeof(suffix), "%u", correlatorCounter);
    params->fiwareCorrelator = fiwareCorrelator + "; cbnotif=" + suffix;

    paramsV->push_back(params);
    return paramsV;
}
