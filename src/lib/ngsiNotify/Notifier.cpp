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
#include "rest/httpRequestSend.h"
#include "ngsiNotify/senderThread.h"
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
* templateNotify - 
*
* This function performs the necessary substitutions according to the template of
* subscription to form the desired notification and send it to the endpoint specified
* in the subscription.
* 
* 
*/
static bool templateNotify
(
  const SubscriptionId&            subscriptionId,
  const ContextElement&            ce,
  const ngsiv2::HttpInfo&          httpInfo,
  const std::string&               tenant,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsOrder
)
{
  Verb                                verb = httpInfo.verb;
  std::string                         method;
  std::string                         url;
  std::string                         payload;
  std::map<std::string, std::string>  qs;
  std::map<std::string, std::string>  headers;

  LM_W(("KZ: In templateNotify"));
  //
  // 1. Verb/Method
  //
  if (verb == NOVERB)
  {
    // Default verb/method is POST
    verb = POST;
  }
  method = verbName(verb);
  LM_W(("KZ: In templateNotify: method: '%s'", method.c_str()));

  //
  // 2. URL
  //
  macroSubstitute(&url, httpInfo.url, ce);
  LM_W(("KZ: url: '%s'", url.c_str()));

  //
  // 3. Payload
  //
  if (httpInfo.payload == "")
  {
    LM_W(("KZ: using default payload"));
    payload = "{" + ce.toJson(renderFormat, attrsOrder) + "}";
  }
  else
  {
    macroSubstitute(&payload, httpInfo.payload, ce);
    renderFormat = NGSI_V2_CUSTOM;
    LM_W(("KZ: substituted payload: '%s'", payload.c_str()));
  }


  //
  // 4. URI Params (Query Strings)
  //
  LM_W(("KZ: Adding URI Params"));
  for (std::map<std::string, std::string>::const_iterator it = httpInfo.qs.begin(); it != httpInfo.qs.end(); ++it)
  {
    std::string key   = it->first;
    std::string value = it->second;

    macroSubstitute(&value, it->second, ce);
    qs[key] = value;
    LM_W(("KZ: Added URI Param '%s': '%s'", key.c_str(), value.c_str()));
  }


  //
  // 5. HTTP Headers
  //
  LM_W(("KZ: Adding HTTP Headers"));
  for (std::map<std::string, std::string>::const_iterator it = httpInfo.headers.begin(); it != httpInfo.headers.end(); ++it)
  {
    std::string key   = it->first;
    std::string value = it->second;

    macroSubstitute(&value, it->second, ce);
    headers[key] = value;
    LM_W(("KZ: Added HTTP Header '%s': '%s'", key.c_str(), value.c_str()));
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
    return false;
  }


  //
  // 7. Add URI params from template to uriPath
  //
  std::string  uri = uriPath;

  LM_W(("KZ: Add URI params from template to uriPath?"));
  if (qs.size() != 0)
  {
    LM_W(("KZ: Adding URI params from template to uriPath"));

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
  else
    LM_W(("KZ: NOT adding URI params from template to uriPath"));


  //
  // 8. Send the request
  //
  //    NOTE: the HTTP headers are sent to httpRequestSend via parameter 'extraHeaders'
  //
  std::string  out;
  int          r;

  LM_W(("KZ: uri: '%s'", uri.c_str()));
  r = httpRequestSend(host,
                      port,
                      protocol,
                      method,
                      tenant,
                      ce.entityId.servicePath,
                      xauthToken,
                      uri,
                      "application/json",
                      payload,
                      fiwareCorrelator,
                      renderFormatToString(renderFormat),
                      true,                // Use Rush if CLI '--rush' allows it
                      true,                // wait for response
                      &out,
                      headers,
                      "application/json",  // Accept Format
                      -1);                 // Timeout in milliseconds, depends on CLI '-httpTimeout'

  if (r == 0)
  {
    statisticsUpdate(NotifyContextSent, JSON);
    alarmMgr.notificationErrorReset(url);
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* NotificationAsTemplateParams - 
*/
typedef struct NotificationAsTemplateParams
{
  NotifyContextRequest*            ncrP;
  ngsiv2::HttpInfo                 httpInfo;
  std::string                      tenant;
  std::string                      xauthToken;
  std::string                      fiwareCorrelator;
  RenderFormat                     renderFormat;
  std::vector<std::string>         attrsOrder;
} NotificationAsTemplateParams;



/* ****************************************************************************
*
* sendNotifyContextRequestAsPerTemplate -
*
* This function splits the contextElementResponseVector for the notification
* into N notifications, one per item in the vector.
* This is done like this as otherwise the substitutions in the template would
* not be possible.
*
* Note as well that sendNotifyContextRequestAsPerTemplate runs in a separate thread and all
* N notifications are sent in a serialized manner, awaiting an ACK from the notification-receiver
* before continuing with the next notification.
* Actually, awaiting an ACK or a timeout (which is 5 seconds by default and configurable using the CLI
* option '-httpTimeout').
*/
void* sendNotifyContextRequestAsPerTemplate(void* p)
{
  NotificationAsTemplateParams* paramP = (NotificationAsTemplateParams*) p;

  LM_W(("KZ: In sendNotifyContextRequestAsPerTemplate"));

  for (unsigned int ix = 0; ix < paramP->ncrP->contextElementResponseVector.size(); ++ix)
  {
    templateNotify(paramP->ncrP->subscriptionId,
                   paramP->ncrP->contextElementResponseVector[ix]->contextElement,
                   paramP->httpInfo,
                   paramP->tenant,
                   paramP->xauthToken,
                   paramP->fiwareCorrelator,
                   paramP->renderFormat,
                   paramP->attrsOrder);
  }

  paramP->ncrP->release();
  delete paramP->ncrP;
  delete paramP;

  return NULL;
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
  const std::vector<std::string>&  attrsOrder
)
{
    ConnectionInfo  ci;
    Verb            verb = httpInfo.verb;

    if (verb == NOVERB)
    {
      // Default verb/method is POST
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
    // Redirect to the method sendNotifyContextRequestAsPerTemplate() when 'httpInfo.extended' is TRUE.
    // 'httpInfo.extended' is FALSE by default and set to TRUE by the json parser.
    //
    LM_W(("KZ: httpInfo.extended == '%s'", httpInfo.extended? "true" : "false"));
    if (httpInfo.extended)
    {
      NotificationAsTemplateParams* paramP = new NotificationAsTemplateParams();

      paramP->ncrP             = ncrP->clone();
      paramP->httpInfo         = httpInfo;
      paramP->tenant           = tenant;
      paramP->xauthToken       = xauthToken;
      paramP->fiwareCorrelator = fiwareCorrelator;
      paramP->renderFormat     = renderFormat;
      paramP->attrsOrder       = attrsOrder;

      pthread_t  tid;
      int        r = pthread_create(&tid, NULL, sendNotifyContextRequestAsPerTemplate, (void*) paramP);

      if (r != 0)
      {
        delete paramP;
        LM_E(("Runtime Error (error creating thread for notifications: %d)", r));
        return;
      }

      pthread_detach(tid);
      return;
    }


    LM_W(("KZ: Sending non-extended Notification"));
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
      payloadString = ncrP->render(&ci, NotifyContext, "");
    }
    else
    {
      payloadString = ncrP->toJson(&ci, renderFormat, attrsOrder);
    }

    /* Parse URL */
    std::string  host;
    int          port;
    std::string  uriPath;
    std::string  protocol;

    if (!parseUrl(httpInfo.url, host, port, uriPath, protocol))
    {
      LM_E(("Runtime Error (not sending NotifyContextRequest: malformed URL: '%s')", httpInfo.url.c_str()));
      return;
    }

    /* Set Content-Type */
    std::string content_type = "application/json";

    /* Send the message (no wait for response), in a separate thread to avoid blocking */
    pthread_t            tid;
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
    params->mimeType         = JSON;
    params->renderFormat     = renderFormatToString(renderFormat);
    params->fiwareCorrelator = fiwareCorrelator;

    strncpy(params->transactionId, transactionId, sizeof(params->transactionId));

    int ret = pthread_create(&tid, NULL, startSenderThread, params);
    if (ret != 0)
    {
      LM_E(("Runtime Error (error creating thread: %d)", ret));
      return;
    }
    pthread_detach(tid);
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
    std::string payload = ncar->render(NotifyContextAvailability, "");

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
    params->verb             = "POST";
    params->tenant           = tenant;
    params->resource         = uriPath;   
    params->content_type     = content_type;
    params->content          = payload;
    params->mimeType         = JSON;
    params->fiwareCorrelator = fiwareCorrelator;
    params->renderFormat     = renderFormatToString(renderFormat);

    strncpy(params->transactionId, transactionId, sizeof(params->transactionId));

    int ret = pthread_create(&tid, NULL, startSenderThread, params);
    if (ret != 0)
    {
      LM_E(("Runtime Error (error creating thread: %d)", ret));
      return;
    }
    pthread_detach(tid);
}
