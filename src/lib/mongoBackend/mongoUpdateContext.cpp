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
* Author: Fermin Galan Marquez
*/
#include <string.h>
#include <string>
#include <vector>
#include <map>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "common/sem.h"
#include "common/limits.h"
#include "common/errorMessages.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/UpdateContextRequest.h"
#include "ngsi/UpdateContextResponse.h"
#include "rest/HttpStatusCode.h"

#include "ngsiNotify/QueueNotifier.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonUpdate.h"
#include "mongoBackend/mongoUpdateContext.h"



/* ****************************************************************************
*
* flowControlAwait -
*/
static void flowControlAwait(unsigned int q0, unsigned int notifSent, const std::string& service)
{
  unsigned int pass = 0;
  unsigned int accumulatedDelay = 0;

  // target may range from q0 (for fcGauge = 1) to q0 + notifSent (for fcGauge = 0)
  unsigned int target = q0 + (1 - fcGauge) * notifSent;
  LM_T(LmtNotifier, ("flow control target is %d", target));

#ifdef UNIT_TEST
  // not used in test (and the QueueNotifier class cause build problems)
  unsigned int qi = 0;
#else
  // get current size of the queue
  unsigned int qi = ((QueueNotifier*) getNotifier())->queueSize(service);
#endif
  LM_T(LmtNotifier, ("flow control pass %d, delay %d, notification queue size is: %d",
                     pass, accumulatedDelay, qi));

  // FIXME P4: we should check in each pass that the connection with the client is still
  // active or break in negative case. Not sure how to do this... we have a
  // ciP->connection object (a MDH_Connection) which hopefully could be used to check
  // this. However libmicrohttpd library documentation is not very clear on this.
  // Maybe the MHD_get_connection_info() function can be used for this.
  // This is not a big issue as it's unrare the client cuts the connection (thus the P4)
  // but it may happen...
  while (qi > target)
  {
    pass++;
    usleep(fcStepDelay * 1000);
    accumulatedDelay += fcStepDelay;

#ifndef UNIT_TEST
    qi = ((QueueNotifier*) getNotifier())->queueSize(service);
#endif
    LM_T(LmtNotifier, ("flow control pass %d, delay %d, notification queue size is: %d",
                       pass, accumulatedDelay, qi));

    if (accumulatedDelay > fcMaxInterval)
    {
      LM_T(LmtNotifier, ("flow control algorithm interrupted due to maxInterval"));
      break;
    }
  }
}



/* ****************************************************************************
*
* mongoUpdateContext - 
*/
HttpStatusCode mongoUpdateContext
(
  UpdateContextRequest*                 requestP,
  UpdateContextResponse*                responseP,
  const std::string&                    tenant,
  const std::vector<std::string>&       servicePathV,
  std::map<std::string, std::string>&   uriParams,    // FIXME P7: we need this to implement "restriction-based" filters
  const std::string&                    xauthToken,
  const std::string&                    fiwareCorrelator,
  const std::string&                    ngsiV2AttrsFormat,
  const bool&                           forcedUpdate,
  const bool&                           overrideMetadata,
  Ngsiv2Flavour                         ngsiv2Flavour,
  bool                                  flowControl
)
{
  bool reqSemTaken;

  reqSemTake(__FUNCTION__, "update request", SemWriteOp, &reqSemTaken);

  // Initial size of notification queue (could be used by flow control algorithm)
  unsigned int q0 = 0;
  if (strcmp(notificationMode, "threadpool") == 0)
  {
#ifndef UNIT_TEST
    q0 = ((QueueNotifier*) getNotifier())->queueSize(tenant);
#endif
    LM_T(LmtNotifier, ("notification queue size before processing update: %d", q0));
  }
  unsigned int notifSent = 0;

  /* Check that the service path vector has only one element, returning error otherwise */
  if (servicePathV.size() > 1)
  {
    char lenV[STRING_SIZE_FOR_INT];

    snprintf(lenV, sizeof(lenV), "%lu", (unsigned long) servicePathV.size());

    std::string details = std::string("service path length ") + lenV + " is greater than the one in update";
    alarmMgr.badInput(clientIp, details);
    responseP->error.fill(SccBadRequest, "service path length greater than the one in update");
    responseP->error.fill(SccBadRequest, "service path length greater than the one in update", ERROR_BAD_REQUEST);
  }
  else
  {
    /* Process each ContextElement */
    UpdateCoverage         updateCoverage = UC_NONE;
    for (unsigned int ix = 0; ix < requestP->entityVector.size(); ++ix)
    {
      UpdateCoverage entityUpdateCoverage;
      notifSent += processContextElement(requestP->entityVector[ix],
                                         responseP,
                                         requestP->updateActionType,
                                         tenant,
                                         servicePathV,
                                         uriParams,
                                         xauthToken,
                                         fiwareCorrelator,
                                         ngsiV2AttrsFormat,
                                         forcedUpdate,
                                         overrideMetadata,
                                         notifSent,
                                         ngsiv2Flavour,
                                         &entityUpdateCoverage);
      switch(updateCoverage)
      {
        case UC_NONE:
          // If global UC is not set yet, then take the UC corresponding to the (first) processed entity
          updateCoverage = entityUpdateCoverage;
          break;
        case UC_SUCCESS:
          // If global UC is success, we need also success in the processed entity to keep global success
          // Otherwise (full attrs fail, partial, not found entity), the global UC changes to partial
          if (entityUpdateCoverage != UC_SUCCESS)
          {
            updateCoverage = UC_PARTIAL;
          }
          break;
        case UC_FULL_ATTRS_FAIL:
          // If global UC is full attrs fail, we need also full attrs fail or not found entity in the processed entity to keep global full attrs fail
          // Otherwise (success, partial), the global UC changes to partial
          if ((entityUpdateCoverage != UC_FULL_ATTRS_FAIL) && (entityUpdateCoverage != UC_ENTITY_NOT_FOUND))
          {
            updateCoverage = UC_PARTIAL;
          }
          break;
        case UC_ENTITY_NOT_FOUND:
          // If global UC is entity not found, we need also entity not found in the processed entity to keep entity not found
          // Otherwise, two possibilities: 1) if processed entity is full attrs fail, global UC changes so, or 2) if processed entity is
          // success/partial, the global UC changes to partial
          if (entityUpdateCoverage == UC_ENTITY_NOT_FOUND)
          {
            // do nothing (explicity block here for the sake of clearness)
          }
          else
          {
            if (entityUpdateCoverage == UC_FULL_ATTRS_FAIL)
            {
              updateCoverage = UC_FULL_ATTRS_FAIL;
            }
            else
            {
              updateCoverage = UC_PARTIAL;
            }
          }
          break;
        case UC_PARTIAL:
          // If global UC is partial, we keep partial (no matter the result of processed entity)
          break;
      }
    }

    // Only the PartialUpdate case (at least one success + at least one fail) needs to be "intercepted" here
    // Other cases follow the usual response processing flow (whatever it is :)
    if (updateCoverage == UC_PARTIAL)
    {
      responseP->error.code  = SccInvalidModification;
      responseP->error.error = ERROR_PARTIAL_UPDATE;
    }

    LM_T(LmtNotifier, ("total notifications sent during update: %d", notifSent));

    /* Note that although individual processContextElements() invocations return ConnectionError, this
       error gets "encapsulated" in the OrionError of the corresponding ContextElementResponse and we
       consider the overall mongoUpdateContext() as OK.
    */
    if (responseP->error.code == SccNone)
    {
      responseP->error.fill(SccOk);
    }
  }

  reqSemGive(__FUNCTION__, "update request", reqSemTaken);

  if (flowControl && fcEnabled && (responseP->error.code == SccOk))
  {
    LM_T(LmtNotifier, ("start notification flow control algorithm"));
    flowControlAwait(q0, notifSent, tenant);
    LM_T(LmtNotifier, ("end notification flow control algorithm"));
  }

  return SccOk;
}
