/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Ken Zangelin
*/
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/uriParamNames.h"                                // URI_PARAM_PAGINATION_OFFSET, URI_PARAM_PAGINATION_LIMIT
#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "mongoBackend/mongoGetSubscriptions.h"                // mongoListSubscriptions
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/serviceRoutines/orionldGetSubscriptions.h"   // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetSubscriptions -
//
bool orionldGetSubscriptions(ConnectionInfo* ciP)
{
  std::vector<ngsiv2::Subscription> subVec;
  OrionError                        oe;
  long long                         count  = 0;
  int                               offset = atoi(ciP->uriParam[URI_PARAM_PAGINATION_OFFSET].c_str());
  int                               limit  = atoi(ciP->uriParam[URI_PARAM_PAGINATION_LIMIT].c_str());

  LM_T(LmtServiceRoutine, ("In orionldGetSubscription"));

  mongoListSubscriptions(&subVec, &oe, ciP->uriParam, ciP->tenant, ciP->servicePathV[0], limit, offset, &count);

  LM_TMP(("Got %d subs (there is a total of %d)", subVec.size(), count));

  ciP->responsePayload = (char*) "{ \"status\": \"OK\" }";

  return true;
}
