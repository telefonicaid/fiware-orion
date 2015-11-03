#include "QueueNotifier.h"

#include "common/string.h"

/* ****************************************************************************
*
* Notifier::sendNotifyContextRequest -
*/
void QueueNotifier::sendNotifyContextRequest(NotifyContextRequest* ncr, const std::string& url, const std::string& tenant, const std::string& xauthToken, Format format)
{
  ConnectionInfo ci;

  //
  // Creating the value of the Fiware-ServicePath HTTP header.
  // This is a comma-separated list of the service-paths in the same order as the entities come in the payload
  //
  std::string spathList;
  bool atLeastOneNotDefault = false;
  for (unsigned int ix = 0; ix < ncr->contextElementResponseVector.size(); ++ix)
  {
    EntityId* eP = &ncr->contextElementResponseVector[ix]->contextElement.entityId;

    if (spathList != "")
    {
      spathList += ",";
    }
    spathList += eP->servicePath;
    atLeastOneNotDefault = atLeastOneNotDefault || (eP->servicePath != "/");
  }
  // FIXME P8: the stuff about atLeastOneNotDefault was added after PR #729, which makes "/" the default servicePath in
  // request not having that header. However, this causes as side-effect that a
  // "Fiware-ServicePath: /" or "Fiware-ServicePath: /,/" header is added in notifications, thus breaking several tests harness.
  // Given that the "clean" implementation of Fiware-ServicePath propagation will be implemented
  // soon (it has been scheduled for version 0.19.0, see https://github.com/telefonicaid/fiware-orion/issues/714)
  // we introduce the atLeastOneNotDefault hack. Once #714 gets implemented,
  // this FIXME will be removed (and all the test harness adjusted, if needed)
  if (!atLeastOneNotDefault)
  {
    spathList = "";
  }

  ci.outFormat = format;
  std::string payload = ncr->render(&ci, NotifyContext, "");

  /* Parse URL */
  std::string  host;
  int          port;
  std::string  uriPath;
  std::string  protocol;

  if (!parseUrl(url, host, port, uriPath, protocol))
  {
    LM_W(("Bad Input (sending NotifyContextRequest: malformed URL: '%s')", url.c_str()));
    return;
  }

  /* Set Content-Type depending on the format */
  std::string content_type = (format == XML)? "application/xml" : "application/json";

  SenderThreadParams* params = new SenderThreadParams();
  params->ip            = host;
  params->port          = port;
  params->protocol      = protocol;
  params->verb          = "POST";
  params->tenant        = tenant;
  params->servicePath   = spathList;
  params->xauthToken    = xauthToken;
  params->resource      = uriPath;
  params->content_type  = content_type;
  params->content       = payload;
  params->format        = format;
  strncpy(params->transactionId, transactionId, sizeof(params->transactionId));

  if (pQueue == NULL)
  {
   LM_E(("Runtime Error (notification queue is NULL)"));
   return;
  }
  bool enqueued = pQueue->try_push(params);
  if (!enqueued)
  {
   LM_E(("Runtime Error (notification queue is full)"));
   return;
  }
}
