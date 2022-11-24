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
#include "apiTypesV2/Subscription.h"
#include "apiTypesV2/CustomPayloadType.h"
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
    ContextElementResponse*          notifyCerP,
    const ngsiv2::Notification&      notification,
    const notifStaticFields&         nsf,
    long long                        maxFailsLimit,
    long long                        failsCounter,
    RenderFormat                     renderFormat,
    const std::vector<std::string>&  attrsFilter,
    bool                             blacklist,
    bool                             covered,
    const std::vector<std::string>&  metadataFilter
)
{
  pthread_t                         tid;
  std::vector<SenderThreadParams*>* paramsV = Notifier::buildSenderParams(notifyCerP,
                                                                          nsf.subId,
                                                                          notification,
                                                                          nsf.tenant,
                                                                          maxFailsLimit,
                                                                          failsCounter,
                                                                          nsf.xauthToken,
                                                                          nsf.fiwareCorrelator,
                                                                          nsf.correlatorCounter,
                                                                          renderFormat,
                                                                          attrsFilter,
                                                                          blacklist,
                                                                          covered,
                                                                          metadataFilter);

  // FIXME PR: is this ok?
  //if (!paramsV->empty()) // al least one param, an empty vector means an error occurred
  if ((paramsV != NULL) && (!paramsV->empty()))
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
* setPayload -
*
* Return false if some problem occur
*/
static bool setPayload
(
  bool                             includePayload,
  const std::string&               notifPayload,
  const SubscriptionId&            subscriptionId,
  Entity&                          en,
  const std::string&               service,
  const std::string&               token,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  const std::vector<std::string>&  metadataFilter,
  std::string*                     payloadP,
  std::string*                     mimeTypeP,
  RenderFormat*                    renderFormatP
)
{
  if (!includePayload)
  {
    *payloadP      = "";
    *renderFormatP = NGSI_V2_CUSTOM;
  }
  else if (notifPayload.empty())
  {
    NotifyContextRequest   ncr;
    ContextElementResponse cer;

    cer.entity.fill(en.id, en.type, en.isPattern, en.servicePath);
    cer.entity.attributeVector.push_back(en.attributeVector);

    cer.statusCode.code = SccOk;

    ncr.subscriptionId  = subscriptionId;
    ncr.contextElementResponseVector.push_back(&cer);

    if (*renderFormatP == NGSI_V1_LEGACY)
    {
      *payloadP = ncr.toJsonV1(false, attrsFilter, blacklist, metadataFilter);
    }
    else
    {
      *payloadP = ncr.toJson(*renderFormatP, attrsFilter, blacklist, metadataFilter);
    }

    *mimeTypeP = "application/json";
  }
  else
  {
    if (!macroSubstitute(payloadP, notifPayload, en, service, token))
    {
      return false;
    }

    char* pload    = curl_unescape(payloadP->c_str(), payloadP->length());
    *payloadP      = std::string(pload);
    *renderFormatP = NGSI_V2_CUSTOM;
    *mimeTypeP     = "text/plain";  // May be overridden by 'Content-Type' in 'headers'
    curl_free(pload);
  }

  return true;
}



/* ****************************************************************************
*
* setJsonPayload -
*/
static bool setJsonPayload
(
  orion::CompoundValueNode*  json,
  const Entity&              en,
  const std::string&         service,
  const std::string&         token,
  std::string*               payloadP,
  std::string*               mimeTypeP
)
{
  // Prepare a map for macro replacements. We firstly tried to pass Entity object to
  // orion::CompoundValueNode()::toJson(), but the include Entity.h in CompoundValueNode.h
  // makes compiler to cry (maybe some kind of circular dependency problem?)
  std::map<std::string, std::string> replacements;
  replacements.insert(std::pair<std::string, std::string>("id", "\"" + en.id + "\""));
  replacements.insert(std::pair<std::string, std::string>("type", "\"" + en.type + "\""));
  replacements.insert(std::pair<std::string, std::string>("service", "\"" + service + "\""));
  replacements.insert(std::pair<std::string, std::string>("servicePath", "\"" + en.servicePath + "\""));
  replacements.insert(std::pair<std::string, std::string>("authToken", "\"" + token + "\""));
  for (unsigned int ix = 0; ix < en.attributeVector.size(); ix++)
  {
    // Note that if some attribute is named service, servicePath or authToken (although it would be
    // an anti-pattern), the attribute takes precedence
    replacements[en.attributeVector[ix]->name] = en.attributeVector[ix]->toJsonValue();
  }

  *payloadP = json->toJson(&replacements);
  *mimeTypeP = "application/json";  // this can be overriden by headers field
  return true;
}



/* ****************************************************************************
*
* setNgsiPayload -
*
* Return false if some problem occur
*/
static bool setNgsiPayload
(
  const Entity&                    ngsi,
  const SubscriptionId&            subscriptionId,
  Entity&                          en,
  const std::string&               service,
  const std::string&               token,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  const std::vector<std::string>&  metadataFilter,
  std::string*                     payloadP
)
{
  // Prepare a map for macro replacements. We firstly tried to pass Entity object to
  // orion::CompoundValueNode()::toJson(), but the include Entity.h in CompoundValueNode.h
  // makes compiler to cry (maybe some kind of circular dependency problem?)
  // FIXME PR: duplicated code in setJsonPayload()
  std::map<std::string, std::string> replacements;
  replacements.insert(std::pair<std::string, std::string>("id", "\"" + en.id + "\""));
  replacements.insert(std::pair<std::string, std::string>("type", "\"" + en.type + "\""));
  replacements.insert(std::pair<std::string, std::string>("service", "\"" + service + "\""));
  replacements.insert(std::pair<std::string, std::string>("servicePath", "\"" + en.servicePath + "\""));
  replacements.insert(std::pair<std::string, std::string>("authToken", "\"" + token + "\""));
  for (unsigned int ix = 0; ix < en.attributeVector.size(); ix++)
  {
    // Note that if some attribute is named service, servicePath or authToken (although it would be
    // an anti-pattern), the attribute takes precedence
    replacements[en.attributeVector[ix]->name] = en.attributeVector[ix]->toJsonValue();
  }

  NotifyContextRequest   ncr;
  ContextElementResponse cer;

  std::string effectiveId;
  if (ngsi.id.empty())
  {
    effectiveId = en.id;
  }
  else
  {
    // FIXME PR: duplicated code in CompoundValueNode.cpp
    if ((ngsi.id.rfind("${") == 0) && (ngsi.id.rfind("}", ngsi.id.size()) == ngsi.id.size() - 1))
    {
      // "Full replacement" case. In this case, the result is not always a string
      // len("${") + len("}") = 3
      std::string macroName = ngsi.id.substr(2, ngsi.id.size() - 3);
      std::map<std::string, std::string>::iterator iter = replacements.find(macroName);
      if (iter == replacements.end())
      {
        // macro doesn't exist in the replacement map, so we use en.id as failsave
        effectiveId = en.id;
      }
      else
      {
        // Note we have to remove double quotes here
        effectiveId = iter->second.substr(1, iter->second.size()-2);
      }
    }
    else
    {
      // "Partial replacement" case. In this case, the result is always a string
      std::string effectiveValue;
      if (!macroSubstitute(&effectiveValue, ngsi.id, &replacements))
      {
        // error already logged in macroSubstitute, using en.id itself as failsafe
        effectiveValue = en.id;
      }
      effectiveId = effectiveValue;
    }
  }

  std::string effectiveType;
  if (ngsi.type.empty())
  {
    effectiveType = en.type;
  }
  else
  {
    // FIXME PR: duplicated code in CompoundValueNode.cpp
    if ((ngsi.type.rfind("${") == 0) && (ngsi.type.rfind("}", ngsi.type.size()) == ngsi.type.size() - 1))
    {
      // "Full replacement" case. In this case, the result is not always a string
      // len("${") + len("}") = 3
      std::string macroName = ngsi.type.substr(2, ngsi.type.size() - 3);
      std::map<std::string, std::string>::iterator iter = replacements.find(macroName);
      if (iter == replacements.end())
      {
        // macro doesn't exist in the replacement map, so we use en.id as failsave
        effectiveType = en.type;
      }
      else
      {
        // Note we have to remove double quotes here
        effectiveType = iter->second.substr(1, iter->second.size()-2);
      }
    }
    else
    {
      // "Partial replacement" case. In this case, the result is always a string
      std::string effectiveValue;
      if (!macroSubstitute(&effectiveValue, ngsi.type, &replacements))
      {
        // error already logged in macroSubstitute, using en.id itself as failsafe
        effectiveValue = en.type;
      }
      effectiveType = effectiveValue;
    }
  }

  cer.entity.fill(effectiveId, effectiveType, en.isPattern, en.servicePath);

  // First we add attributes in the ngsi field
  for (unsigned int ix = 0; ix < ngsi.attributeVector.size(); ix++)
  {
    cer.entity.attributeVector.push_back(new ContextAttribute(ngsi.attributeVector[ix], false, true));
  }
  // Next, other attributes in the original entity not already added
  for (unsigned int ix = 0; ix < en.attributeVector.size(); ix++)
  {
    if (cer.entity.attributeVector.get(en.attributeVector[ix]->name) < 0)
    {
      cer.entity.attributeVector.push_back(new ContextAttribute(en.attributeVector[ix], false, true));
    }
  }

  cer.statusCode.code = SccOk;

  ncr.subscriptionId  = subscriptionId;
  ncr.contextElementResponseVector.push_back(&cer);

 *payloadP = ncr.toJson(NGSI_V2_NORMALIZED, attrsFilter, blacklist, metadataFilter, &replacements);

  return true;
}



/* ****************************************************************************
*
* buildSenderParamsCustom -
*/
static std::vector<SenderThreadParams*>* buildSenderParamsCustom
(
    const SubscriptionId&                subscriptionId,
    ContextElementResponse*              notifyCerP,
    const ngsiv2::Notification&          notification,
    const std::string&                   tenant,
    long long                            maxFailsLimit,
    long long                            failsCounter,
    const std::string&                   xauthToken,
    const std::string&                   fiwareCorrelator,
    unsigned int                         correlatorCounter,
    RenderFormat                         renderFormat,
    const std::vector<std::string>&      attrsFilter,
    bool                                 blacklist,
    const std::vector<std::string>&      metadataFilter
)
{
  std::vector<SenderThreadParams*>*  paramsV = NULL;

  paramsV = new std::vector<SenderThreadParams*>;

  // FIXME PR: reindent
  //for (unsigned ix = 0; ix < cv.size(); ix++)
  //{
    std::string                         method;
    std::string                         url;
    std::string                         payload;
    std::string                         topic;
    std::string                         mimeType;
    std::map<std::string, std::string>  qs;
    std::map<std::string, std::string>  headers;
    Entity&                             en      = notifyCerP->entity;

    //
    // 1. Verb/Method
    //
    if (notification.type == ngsiv2::HttpNotification)
    {
      Verb  verb = notification.httpInfo.verb;
      if (verb == NOVERB)
      {
        // Default verb/method is POST
        verb = POST;
      }
      method = verbName(verb);
    }
    else  // MqttNotification
    {
      // Verb/method is irrelevant in this case
      method = verbName(NOVERB);
    }


    //
    // 2. URL
    //
    std::string notifUrl = (notification.type == ngsiv2::HttpNotification ? notification.httpInfo.url : notification.mqttInfo.url);
    if (macroSubstitute(&url, notifUrl, en, tenant, xauthToken) == false)
    {
      // Warning already logged in macroSubstitute()
      // FIXME PR: should return NULL?
      // FIXME PR: we are inside a for loop, maybe should use 'continue' instead of return?
      return paramsV;  // empty vector
    }


    //
    // 3. Payload
    //
    ngsiv2::CustomPayloadType customPayloadType = (notification.type == ngsiv2::HttpNotification ? notification.httpInfo.payloadType : notification.mqttInfo.payloadType);

    if (customPayloadType == ngsiv2::CustomPayloadType::Text)
    {
     bool         includePayload = (notification.type == ngsiv2::HttpNotification ? notification.httpInfo.includePayload : notification.mqttInfo.includePayload);
     std::string  notifPayload   = (notification.type == ngsiv2::HttpNotification ? notification.httpInfo.payload : notification.mqttInfo.payload);
     if (!setPayload(includePayload, notifPayload, subscriptionId, en, tenant, xauthToken, attrsFilter, blacklist, metadataFilter, &payload, &mimeType, &renderFormat))
     {
       // Warning already logged in macroSubstitute()
       // FIXME PR: should return NULL?
       // FIXME PR: we are inside a for loop, maybe should use 'continue' instead of return?
       return paramsV;  // empty vector
     }
    }
    else if (customPayloadType == ngsiv2::CustomPayloadType::Json)
    {
      orion::CompoundValueNode*  json = (notification.type == ngsiv2::HttpNotification ? notification.httpInfo.json : notification.mqttInfo.json);
      setJsonPayload(json, en, tenant, xauthToken, &payload, &mimeType);
      renderFormat = NGSI_V2_CUSTOM;
    }
    else  // customPayloadType == ngsiv2::CustomPayloadType::Ngsi
    {
      // Important to use const& for Entity here. Otherwise problems may occur in the object release logic
      const Entity& ngsi = (notification.type == ngsiv2::HttpNotification ? notification.httpInfo.ngsi : notification.mqttInfo.ngsi);
      if (!setNgsiPayload(ngsi, subscriptionId, en, tenant, xauthToken, attrsFilter, blacklist, metadataFilter, &payload))
      {
        // Warning already logged in macroSubstitute()
        // FIXME PR: should return NULL?
        // FIXME PR: we are inside a for loop, maybe should use 'continue' instead of return?
        return paramsV;  // empty vector
      }
      renderFormat = NGSI_V2_CUSTOM;
      mimeType = "application/json";
    }


    //
    // 4. URI Params (Query Strings) (only in the case of HTTP notifications)
    //
    if (notification.type == ngsiv2::HttpNotification)
    {
      for (std::map<std::string, std::string>::const_iterator it = notification.httpInfo.qs.begin(); it != notification.httpInfo.qs.end(); ++it)
      {
        std::string key   = it->first;
        std::string value = it->second;

        if ((macroSubstitute(&key, it->first, en, tenant, xauthToken) == false) || (macroSubstitute(&value, it->second, en, tenant, xauthToken) == false))
        {
          // Warning already logged in macroSubstitute()
          // FIXME PR: should return NULL?
          // FIXME PR: we are inside a for loop, maybe should use 'continue' instead of return?
          return paramsV;  // empty vector
        }

        if ((value.empty()) || (key.empty()))
        {
          // To avoid e.g '?a=&b=&c='
          continue;
        }
        qs[key] = value;
      }
    }


    //
    // 5. HTTP Headers (only in the case of HTTP notifications)
    //
    if (notification.type == ngsiv2::HttpNotification)
    {
      for (std::map<std::string, std::string>::const_iterator it = notification.httpInfo.headers.begin(); it != notification.httpInfo.headers.end(); ++it)
      {
        std::string key   = it->first;
        std::string value = it->second;

        if ((macroSubstitute(&key, it->first, en, tenant, xauthToken) == false) || (macroSubstitute(&value, it->second, en, tenant, xauthToken) == false))
        {
          // Warning already logged in macroSubstitute()
          // FIXME PR: should return NULL?
          // FIXME PR: we are inside a for loop, maybe should use 'continue' instead of return?
          return paramsV;  // empty vector
        }

        if (key.empty())
        {
          // To avoid empty header name
          continue;
        }

        // Decode header value
        char* pvalue = curl_unescape(value.c_str(), value.length());
        value        = std::string(pvalue);
        curl_free(pvalue);

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        headers[key] = value;
      }
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
      LM_E(("Runtime Error (not sending NotifyContextRequest: malformed URL: '%s')", url.c_str()));
      // FIXME PR: should return NULL?
      // FIXME PR: we are inside a for loop, maybe should use 'continue' instead of return?
      return paramsV;  // empty vector
    }


    //
    // 7. Add URI params from template to uriPath
    //
    // Note qs.size() == 0 in the case of MQTT notificationa, as step 4 is not executed.
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

    // 8. Topic (only in the case of MQTT notifications)
    if (notification.type == ngsiv2::MqttNotification)
    {
      if (macroSubstitute(&topic, notification.mqttInfo.topic, en, tenant, xauthToken) == false)
      {
        // Warning already logged in macroSubstitute()
        // FIXME PR: should return NULL?
        // FIXME PR: we are inside a for loop, maybe should use 'continue' instead of return?
        return paramsV;  // empty vector
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
    params->resource         = notification.type == ngsiv2::HttpNotification? uri : topic;
    params->content_type     = mimeType;
    params->content          = payload;
    params->mimeType         = JSON;
    params->renderFormat     = renderFormatToString(renderFormat);
    params->extraHeaders     = headers;
    params->registration     = false;
    params->subscriptionId   = subscriptionId.get();
    params->qos              = notification.mqttInfo.qos;     // unspecified in case of HTTP notifications
    params->timeout          = notification.httpInfo.timeout; // unspecified in case of MQTT notifications
    params->user             = notification.mqttInfo.user;   // unspecified in case of HTTP notifications
    params->passwd           = notification.mqttInfo.passwd; // unspecified in case of HTTP notifications

    char suffix[STRING_SIZE_FOR_INT];
    snprintf(suffix, sizeof(suffix), "%u", correlatorCounter);
    params->fiwareCorrelator = fiwareCorrelator + "; cbnotif=" + suffix;

    paramsV->push_back(params);
  //}

  // FIXME PR: paramsV is always mono-item...
  return paramsV;
}



/* ****************************************************************************
*
* Notifier::buildSenderParams -
*/
std::vector<SenderThreadParams*>* Notifier::buildSenderParams
(
  ContextElementResponse*          notifyCerP,
  const std::string&               subId,
  const ngsiv2::Notification&      notification,
  const std::string&               tenant,
  long long                        maxFailsLimit,
  long long                        failsCounter,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  unsigned int                     correlatorCounter,
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  bool                             covered,
  const std::vector<std::string>&  metadataFilter
)
{
    ConnectionInfo                    ci;
    std::vector<SenderThreadParams*>* paramsV = NULL;

    Verb verb;
    if (notification.type == ngsiv2::HttpNotification)
    {
      verb = notification.httpInfo.verb;
      if ((verb == NOVERB) || (verb == UNKNOWNVERB) || disableCusNotif)
      {
        // Default verb/method (or the one in case of disabled custom notifications) is POST
        verb = POST;
      }
    }
    else  // MqttNotification
    {
      // Verb/methodd is irrelevant in this case
      verb = NOVERB;
    }

    if (covered)
    {
      for (unsigned int ix = 0; ix < attrsFilter.size(); ix++)
      {
        // Aviod over-adding attribute, checking first that the attribute is not already added
        std::string attrName = attrsFilter[ix];
        if (notifyCerP->entity.attributeVector.get(attrName) < 0)
        {
          ContextAttribute* caP = new ContextAttribute(attrName, DEFAULT_ATTR_NULL_TYPE, "");
          caP->valueType = orion::ValueTypeNull;
          notifyCerP->entity.attributeVector.push_back(caP);
        }
      }
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
    bool custom = notification.type == ngsiv2::HttpNotification ? notification.httpInfo.custom : notification.mqttInfo.custom;
    if (custom && !disableCusNotif)
    {
      return buildSenderParamsCustom(subId,
                                     notifyCerP,
                                     notification,
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

    NotifyContextRequest   ncr;
    ContextElementResponse cer;

    cer.entity.fill(notifyCerP->entity.id,
                    notifyCerP->entity.type,
                    notifyCerP->entity.isPattern,
                    notifyCerP->entity.servicePath);

    // FIXME PR: maybe filtering is already done by filtering logic and most
    // of this block is unneded?
    for (unsigned int ix = 0; ix < notifyCerP->entity.attributeVector.size(); ix++)
    {
      ContextAttribute* caP = notifyCerP->entity.attributeVector[ix];

      /* 'skip' field is used to mark deleted attributes that must not be included in the
     * notification (see deleteAttrInNotifyCer function for details) */
      if ((attrsFilter.size() == 0) ||
          (std::find(attrsFilter.begin(), attrsFilter.end(), std::string(ALL_ATTRS)) != attrsFilter.end()) ||
          (blacklist == true))
      {
        /* Empty attribute list in the subscription mean that all attributes are added
       * Note we use cloneCompound=true in the ContextAttribute constructor. This is due to
       * cer.entity destructor does release() on the attrs vector */
        if (!caP->skip)
        {
          cer.entity.attributeVector.push_back(new ContextAttribute(caP, false, true));
        }
      }
      else
      {
        for (unsigned int jx = 0; jx < attrsFilter.size(); jx++)
        {
          if (caP->name == attrsFilter[jx] && !caP->skip)
          {
            /* Note we use cloneCompound=true in the ContextAttribute constructor. This is due to
           * cer.entity destructor does release() on the attrs vector */
            cer.entity.attributeVector.push_back(new ContextAttribute(caP, false, true));
          }
        }
      }
    }

    /* Early exit without sending notification if attribute list is empty */
    if (cer.entity.attributeVector.size() == 0)
    {
      ncr.contextElementResponseVector.release();
      // FIXME PR: check if we can process correctly NULL as params
      return NULL;
    }
    // FIXME PR: ...useless until this point?

    /* Setting status code in CER */
    cer.statusCode.fill(SccOk);

    ncr.contextElementResponseVector.push_back(&cer);

    /* Complete the fields in NotifyContextRequest */
    ncr.subscriptionId.set(subId);
    // FIXME: we use a proper origin name
    ncr.originator.set("localhost");

    ncr.subscriptionId.set(subId);

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

    std::string url = (notification.type == ngsiv2::HttpNotification ? notification.httpInfo.url : notification.mqttInfo.url);
    if (!parseUrl(url, host, port, uriPath, protocol))
    {
      LM_E(("Runtime Error (not sending NotifyContextRequest: malformed URL: '%s')", url.c_str()));
      // FIXME PR: should be NULL?
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
    params->resource         = notification.type == ngsiv2::HttpNotification? uriPath : notification.mqttInfo.topic;
    params->content_type     = content_type;
    params->content          = payloadString;
    params->mimeType         = JSON;
    params->renderFormat     = renderFormatToString(renderFormat);
    params->subscriptionId   = ncr.subscriptionId.get();
    params->registration     = false;
    params->qos              = notification.mqttInfo.qos; // unspecified in case of HTTP notifications
    params->timeout          = notification.httpInfo.timeout; // unspecified in case of MQTT notifications
    params->user             = notification.mqttInfo.user;   // unspecified in case of HTTP notifications
    params->passwd           = notification.mqttInfo.passwd; // unspecified in case of HTTP notifications

    char suffix[STRING_SIZE_FOR_INT];
    snprintf(suffix, sizeof(suffix), "%u", correlatorCounter);
    params->fiwareCorrelator = fiwareCorrelator + "; cbnotif=" + suffix;

    paramsV->push_back(params);
    return paramsV;
}
