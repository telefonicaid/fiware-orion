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
* Author: Fermín Galán
*/
#include <string>
#include <map>

#include "logMsg/traceLevels.h"

#include "orionld/types/OrionldTenant.h"             // OrionldTenant
#include "orionld/common/orionldState.h"             // orionldState

#include "common/sem.h"
#include "ngsi9/RegisterContextRequest.h"
#include "ngsi9/RegisterContextResponse.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonRegister.h"
#include "mongoBackend/mongoNotifyContextAvailability.h"



/* ****************************************************************************
*
* mongoNotifyContextAvailability -
*
* The fields "subscriptionId" and "originator" of the request are ignored,
* as we don't have anything interesting to do with them.
*/
HttpStatusCode mongoNotifyContextAvailability
(
  NotifyContextAvailabilityRequest*    requestP,
  NotifyContextAvailabilityResponse*   responseP,
  std::map<std::string, std::string>&  uriParam,
  const std::string&                   fiwareCorrelator,
  OrionldTenant*                       tenantP,
  const std::string&                   servicePath
)
{
  if (tenantP == NULL)
    tenantP = orionldState.tenantP;

  bool                    reqSemTaken;
  RegisterContextRequest  rcr;

  reqSemTake(__FUNCTION__, "mongo ngsi9 notification", SemWriteOp, &reqSemTaken);

  /* Process each ContextRegistrationElement to create a "fake" RegisterContextRequest */
  for (unsigned int ix= 0; ix < requestP->contextRegistrationResponseVector.size(); ++ix)
  {
    ContextRegistration* crP = &requestP->contextRegistrationResponseVector[ix]->contextRegistration;
    rcr.contextRegistrationVector.push_back(crP);
  }

  /* notifyContextAvailability doesn't include duration information, so we will use the default */
  rcr.duration.set(DEFAULT_DURATION);

  /* We use processRegisterContext() function. Note that in this case the response is not needed, so we will
   * only use it to conform to function signature. In addition, take into account that from a registerContext
   * point of view, notifyContextAvailability is considered as a new registration (as no registratinId is
   * received in the notification message)
   */
  RegisterContextResponse rcRes;
  processRegisterContext(&rcr, &rcRes, NULL, tenantP, servicePath, "JSON", fiwareCorrelator);

  responseP->responseCode.fill(SccOk);

  reqSemGive(__FUNCTION__, "mongo ngsi9 notification", reqSemTaken);

  return SccOk;
}
