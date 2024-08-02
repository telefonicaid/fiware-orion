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
  SenderThreadParams* paramsP = Notifier::buildSenderParams(notifyCerP,
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

  if (paramsP != NULL)
  {
    int ret = pthread_create(&tid, NULL, startSenderThread, paramsP);

    if (ret != 0)
    {
      LM_E(("Runtime Error (error creating thread: %d)", ret));
      delete paramsP;
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
  ExprContextObject*               exprContextObjectP,
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

    // cloneCompount set to true. Otherwise nasty things as the one
    // described in issue #4263 will happend
    cer.entity.attributeVector.push_back(en.attributeVector, true);

    cer.statusCode.code = SccOk;

    ncr.subscriptionId  = subscriptionId;
    ncr.contextElementResponseVector.push_back(&cer);

    *payloadP = ncr.toJson(*renderFormatP, attrsFilter, blacklist, metadataFilter);

    *mimeTypeP = "application/json";
  }
  else
  {
    if (!macroSubstitute(payloadP, notifPayload, exprContextObjectP, "null", true))
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
  ExprContextObject*         exprContextObjectP,
  std::string*               payloadP,
  std::string*               mimeTypeP
)
{
  *payloadP = json->toJson(exprContextObjectP);
  *mimeTypeP = "application/json";  // this can be overriden by headers field
  return true;
}



/* ****************************************************************************
*
* orderByPriority -
*
* Return false if some problem occur
*/
static void orderByPriority
(
  const Entity&                    ngsi,
  std::vector<ContextAttribute*>*  orderedAttrs
)
{
  // Add all the attributes to a temporal vector
  std::vector<ContextAttribute*> attrs;

  for (unsigned ix = 0; ix < ngsi.attributeVector.size(); ix++)
  {
    attrs.push_back(ngsi.attributeVector[ix]);
  }

  // Pass the content of attrs to orderedAttrs based in the evalPriority element
  while (!attrs.empty())
  {
    ContextAttribute* selectedAttr = attrs[0];
    unsigned int      selectedIx   = 0;
    double            prio         = selectedAttr->getEvalPriority();

    for (unsigned ix = 0; ix < attrs.size(); ix++)
    {
      double newPrio = attrs[ix]->getEvalPriority();
      if (newPrio < prio)
      {
        selectedAttr = attrs[ix];
        selectedIx   = ix;
        prio         = newPrio;
      }
    }

    orderedAttrs->push_back(selectedAttr);
    attrs.erase(attrs.begin() + selectedIx);
  }
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
  ExprContextObject*               exprContextObjectP,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  const std::vector<std::string>&  metadataFilter,
  std::string*                     payloadP,
  RenderFormat                     renderFormat,
  bool                             basic  // used by TIME_EXPR_CTXBLD_START/STOP macros
)
{
  NotifyContextRequest   ncr;
  ContextElementResponse cer;

  std::string effectiveId;
  if (ngsi.id.empty())
  {
    effectiveId = en.id;
  }
  else
  {
    // If id is not found in the replacements macro, we use en.id.
    effectiveId = removeQuotes(smartStringValue(ngsi.id, exprContextObjectP, '"' + en.id + '"'));
  }

  std::string effectiveType;
  if (ngsi.type.empty())
  {
    effectiveType = en.type;
  }
  else
  {
    // If type is not found in the replacements macro, we use en.type.
    effectiveType = removeQuotes(smartStringValue(ngsi.type, exprContextObjectP, '"' + en.type + '"'));
  }

  cer.entity.fill(effectiveId, effectiveType, en.isPattern, en.servicePath);

  // First we add attributes in the ngsi field, adding calculated expressions to context in order of priority
  std::vector<ContextAttribute*>  orderedNgsiAttrs;
  orderByPriority(ngsi, &orderedNgsiAttrs);

  for (unsigned int ix = 0; ix < orderedNgsiAttrs.size(); ix++)
  {
    // Avoid to add context if an attribute with the same name exists in the entity
    if (en.attributeVector.get(orderedNgsiAttrs[ix]->name) < 0)
    {
      TIME_EXPR_CTXBLD_START();
      exprContextObjectP->add(orderedNgsiAttrs[ix]->name, orderedNgsiAttrs[ix]->toJsonValue(exprContextObjectP), true);
      TIME_EXPR_CTXBLD_STOP();
    }

    cer.entity.attributeVector.push_back(new ContextAttribute(orderedNgsiAttrs[ix], false, true));
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

  if ((renderFormat == NGSI_V2_SIMPLIFIEDNORMALIZED) || (renderFormat == NGSI_V2_SIMPLIFIEDKEYVALUES))
  {
    *payloadP = ncr.toJson(renderFormat, attrsFilter, blacklist, metadataFilter, exprContextObjectP);
  }
  else
  {
    *payloadP = ncr.toJson(NGSI_V2_NORMALIZED, attrsFilter, blacklist, metadataFilter, exprContextObjectP);
  }

  return true;
}



/* ****************************************************************************
*
* buildSenderParamsCustom -
*/
static SenderThreadParams* buildSenderParamsCustom
(
    const SubscriptionId&            subscriptionId,
    ContextElementResponse*          notifyCerP,
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
    const std::vector<std::string>&  metadataFilter
)
{
  std::string                         method;
  std::string                         url;
  std::string                         payload;
  std::string                         topic;
  std::string                         mimeType;
  std::map<std::string, std::string>  qs;
  std::map<std::string, std::string>  headers;
  Entity&                             en      = notifyCerP->entity;

#ifdef EXPR_BASIC
  bool basic = true;
#else
  bool basic = false;
#endif

  // Used by several macroSubstitute() calls along this function
  ExprContextObject exprContext(basic);
  ExprContextObject exprMetadataContext(basic);

  // It seems that add() semantics are different in basic and jexl mode. In jexl mode, if the key already exists, it is
  // updated (in other words, the last added keys is the one that takes precedence). In basic model, if the key already
  // exists, the operation is ignored (in other words, the first added key is the one that takes precedence). Taking
  // into account that in the case of an attribute with name "service", "servicePath" or "authToken", it must have precedence
  // over the ones comming from headers of the same name, we conditionally add them depending the case
  TIME_EXPR_CTXBLD_START();
  exprContext.add("id", en.id);
  exprContext.add("type", en.type);

  if (!basic)
  {
    exprContext.add("service", tenant);
    exprContext.add("servicePath", en.servicePath);
    exprContext.add("authToken", xauthToken);
  }

  for (unsigned int ix = 0; ix < en.attributeVector.size(); ix++)
  {
    en.attributeVector[ix]->addToContext(&exprContext, basic);

    // Add attribute metadata to context
    ExprContextObject exprAttrMetadataContext(basic);
    for (unsigned int jx = 0; jx < en.attributeVector[ix]->metadataVector.size(); jx++)
    {
      en.attributeVector[ix]->metadataVector[jx]->addToContext(&exprAttrMetadataContext, basic);
    }
    exprMetadataContext.add(en.attributeVector[ix]->name, exprAttrMetadataContext);
  }

  // Add all metadata under the "metadata" context key
  // (note that in JEXL if the key already exists, it is updated, so attribute with name "metadata" will never be appear in context)
  exprContext.add("metadata", exprMetadataContext);

  if (basic)
  {
    exprContext.add("service", tenant);
    exprContext.add("servicePath", en.servicePath);
    exprContext.add("authToken", xauthToken);
  }
  TIME_EXPR_CTXBLD_STOP();

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
  if (macroSubstitute(&url, notifUrl, &exprContext, "", true) == false)
  {
    // Warning already logged in macroSubstitute()
    return NULL;
  }


  //
  // 3. Payload
  //
  ngsiv2::CustomPayloadType customPayloadType = (notification.type == ngsiv2::HttpNotification ? notification.httpInfo.payloadType : notification.mqttInfo.payloadType);

  if (customPayloadType == ngsiv2::CustomPayloadType::Text)
  {
    bool         includePayload = (notification.type == ngsiv2::HttpNotification ? notification.httpInfo.includePayload : notification.mqttInfo.includePayload);
    std::string  notifPayload   = (notification.type == ngsiv2::HttpNotification ? notification.httpInfo.payload : notification.mqttInfo.payload);
    if (!setPayload(includePayload, notifPayload, subscriptionId, en, &exprContext, attrsFilter, blacklist, metadataFilter, &payload, &mimeType, &renderFormat))
    {
      // Warning already logged in macroSubstitute()
      return NULL;
    }
  }
  else if (customPayloadType == ngsiv2::CustomPayloadType::Json)
  {
    orion::CompoundValueNode*  json = (notification.type == ngsiv2::HttpNotification ? notification.httpInfo.json : notification.mqttInfo.json);
    setJsonPayload(json, &exprContext, &payload, &mimeType);
    renderFormat = NGSI_V2_CUSTOM;
  }
  else  // customPayloadType == ngsiv2::CustomPayloadType::Ngsi
  {
    // Important to use const& for Entity here. Otherwise problems may occur in the object release logic
    const Entity& ngsi = (notification.type == ngsiv2::HttpNotification ? notification.httpInfo.ngsi : notification.mqttInfo.ngsi);
    if (!setNgsiPayload(ngsi, subscriptionId, en, &exprContext, attrsFilter, blacklist, metadataFilter, &payload, renderFormat, basic))
    {
      // Warning already logged in macroSubstitute()
      return NULL;
    }
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

      if ((macroSubstitute(&key, it->first, &exprContext, "", true) == false) || (macroSubstitute(&value, it->second, &exprContext, "", true) == false))
      {
        // Warning already logged in macroSubstitute()
        return NULL;
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

      if ((macroSubstitute(&key, it->first, &exprContext,  "", true) == false) || (macroSubstitute(&value, it->second, &exprContext, "", true) == false))
      {
        // Warning already logged in macroSubstitute()
        return NULL;
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
    LM_E(("Runtime Error (not sending notification: malformed URL: '%s')", url.c_str()));
    return NULL;
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
    if (macroSubstitute(&topic, notification.mqttInfo.topic, &exprContext, "", true) == false)
    {
      // Warning already logged in macroSubstitute()
      return NULL;
    }
  }

  SenderThreadParams*  paramsP = new SenderThreadParams();

  paramsP->type             = QUEUE_MSG_NOTIF;
  paramsP->from             = fromIp;  // note fromIp is a thread variable
  paramsP->ip               = host;
  paramsP->port             = port;
  paramsP->protocol         = protocol;
  paramsP->verb             = method;
  paramsP->tenant           = tenant;
  paramsP->maxFailsLimit    = maxFailsLimit;
  paramsP->failsCounter     = failsCounter;
  paramsP->servicePath      = en.servicePath;
  paramsP->xauthToken       = xauthToken;
  paramsP->resource         = notification.type == ngsiv2::HttpNotification? uri : topic;
  paramsP->content_type     = mimeType;
  paramsP->content          = payload;
  paramsP->mimeType         = JSON;
  paramsP->renderFormat     = renderFormatToString(renderFormat);
  paramsP->extraHeaders     = headers;
  paramsP->registration     = false;
  paramsP->subscriptionId   = subscriptionId.get();
  paramsP->qos              = notification.mqttInfo.qos;     // unspecified in case of HTTP notifications
  paramsP->retain           = notification.mqttInfo.retain;  // unspecified in case of HTTP notifications
  paramsP->timeout          = notification.httpInfo.timeout; // unspecified in case of MQTT notifications
  paramsP->user             = notification.mqttInfo.user;   // unspecified in case of HTTP notifications
  paramsP->passwd           = notification.mqttInfo.passwd; // unspecified in case of HTTP notifications

  char suffix[STRING_SIZE_FOR_INT];
  snprintf(suffix, sizeof(suffix), "%u", correlatorCounter);
  paramsP->fiwareCorrelator = fiwareCorrelator + "; cbnotif=" + suffix;

  return paramsP;
}



/* ****************************************************************************
*
* Notifier::buildSenderParams -
*/
SenderThreadParams* Notifier::buildSenderParams
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
    ConnectionInfo  ci;

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

    NotifyContextRequest   ncr;
    ContextElementResponse cer;

    /* Note we use cloneCompound=true in cer.entity.fill(). This is due to
     * cer.entity destructor does release() on the attrs vector */
    cer.entity.fill(notifyCerP->entity, false, true);
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

    std::string payloadString = ncr.toJson(renderFormat, attrsFilter, blacklist, metadataFilter);

    /* Parse URL */
    std::string  host;
    int          port;
    std::string  uriPath;
    std::string  protocol;

    std::string url = (notification.type == ngsiv2::HttpNotification ? notification.httpInfo.url : notification.mqttInfo.url);
    if (!parseUrl(url, host, port, uriPath, protocol))
    {
      LM_E(("Runtime Error (not sending notification: malformed URL: '%s')", url.c_str()));
      return NULL;
    }

    /* Set Content-Type */
    std::string content_type = "application/json";

    SenderThreadParams* paramsP = new SenderThreadParams();

    paramsP->type             = QUEUE_MSG_NOTIF;
    paramsP->from             = fromIp;  // note fromIp is a thread variable
    paramsP->ip               = host;
    paramsP->port             = port;
    paramsP->protocol         = protocol;
    paramsP->verb             = verbName(verb);
    paramsP->tenant           = tenant;
    paramsP->maxFailsLimit    = maxFailsLimit;
    paramsP->failsCounter     = failsCounter;
    paramsP->servicePath      = spath;
    paramsP->xauthToken       = xauthToken;
    paramsP->resource         = notification.type == ngsiv2::HttpNotification? uriPath : notification.mqttInfo.topic;
    paramsP->content_type     = content_type;
    paramsP->content          = payloadString;
    paramsP->mimeType         = JSON;
    paramsP->renderFormat     = renderFormatToString(renderFormat);
    paramsP->subscriptionId   = ncr.subscriptionId.get();
    paramsP->registration     = false;
    paramsP->qos              = notification.mqttInfo.qos; // unspecified in case of HTTP notifications
    paramsP->retain           = notification.mqttInfo.retain; // unspecified in case of HTTP notifications
    paramsP->timeout          = notification.httpInfo.timeout; // unspecified in case of MQTT notifications
    paramsP->user             = notification.mqttInfo.user;   // unspecified in case of HTTP notifications
    paramsP->passwd           = notification.mqttInfo.passwd; // unspecified in case of HTTP notifications

    char suffix[STRING_SIZE_FOR_INT];
    snprintf(suffix, sizeof(suffix), "%u", correlatorCounter);
    paramsP->fiwareCorrelator = fiwareCorrelator + "; cbnotif=" + suffix;

    return paramsP;
}
