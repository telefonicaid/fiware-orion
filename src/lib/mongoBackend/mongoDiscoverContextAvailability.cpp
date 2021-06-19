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
* Author: Ken Zangelin
*/
#include <string>
#include <map>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "orionld/types/OrionldTenant.h"               // OrionldTenant
#include "orionld/common/orionldState.h"               // orionldState
#include "common/statistics.h"
#include "common/sem.h"
#include "alarmMgr/alarmMgr.h"
#include "mongoBackend/mongoDiscoverContextAvailability.h"
#include "rest/HttpStatusCode.h"
#include "mongoBackend/MongoGlobal.h"
#include "ngsi/StatusCode.h"
#include "ngsi9/DiscoverContextAvailabilityRequest.h"
#include "ngsi9/DiscoverContextAvailabilityResponse.h"
#include "mongo/client/dbclient.h"



/* ****************************************************************************
*
* processDiscoverContextAvailability -
*/
static HttpStatusCode processDiscoverContextAvailability
(
  DiscoverContextAvailabilityRequest*   requestP,
  DiscoverContextAvailabilityResponse*  responseP,
  OrionldTenant*                        tenantP,
  int                                   offset,
  int                                   limit,
  bool                                  details,
  const std::vector<std::string>&       servicePathV
)
{
  std::string  err;
  long long    count = -1;

  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));

  if (!registrationsQuery(requestP->entityIdVector,
                          requestP->attributeList,
                          &responseP->responseVector,
                          &err,
                          tenantP,
                          servicePathV,
                          offset,
                          limit,
                          details,
                          &count))
  {
    responseP->errorCode.fill(SccReceiverInternalError, err);
    return SccOk;
  }

  if (responseP->responseVector.size() == 0)
  {
    //
    // If the responseV is empty, we haven't found any entity and have to fill the status code part in the response.
    //
    // However, if the response was empty due to a too high pagination offset,
    // and if the user has asked for 'details' (as URI parameter), then the response should include information about
    // the number of hits without pagination.
    //
    if (details)
    {
      if ((count > 0) && (offset >= count))
      {
        char details[256];

        snprintf(details, sizeof(details), "Number of matching registrations: %lld. Offset is %d", count, offset);
        responseP->errorCode.fill(SccContextElementNotFound, details);

        return SccOk;
      }
    }

    responseP->errorCode.fill(SccContextElementNotFound);
    return SccOk;
  }
  else if (details == true)
  {
    //
    // If all is OK, but the details URI param has been set to 'on', then the response error code details
    // MUST contain the total count of hits.
    //

    char details[64];

    snprintf(details, sizeof(details), "Count: %lld", count);
    responseP->errorCode.fill(SccOk, details);
  }

  return SccOk;
}



/* ****************************************************************************
*
* mongoDiscoverContextAvailability -
*/
HttpStatusCode mongoDiscoverContextAvailability
(
  DiscoverContextAvailabilityRequest*   requestP,
  DiscoverContextAvailabilityResponse*  responseP,
  OrionldTenant*                        tenantP,
  std::map<std::string, std::string>&   uriParams,
  const std::vector<std::string>&       servicePathV
)
{
  int          offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
  int          limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());
  std::string  detailsString  = uriParams[URI_PARAM_PAGINATION_DETAILS];
  bool         details        = (strcasecmp("on", detailsString.c_str()) == 0)? true : false;
  bool         reqSemTaken;

  reqSemTake(__FUNCTION__, "mongo ngsi9 discovery request", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("DiscoverContextAvailability Request"));

  HttpStatusCode hsCode = processDiscoverContextAvailability(requestP,
                                                             responseP,
                                                             tenantP,
                                                             offset,
                                                             limit,
                                                             details,
                                                             servicePathV);
  if (hsCode != SccOk)
  {
    ++noOfDiscoveryErrors;
  }

  reqSemGive(__FUNCTION__, "mongo ngsi9 discovery request", reqSemTaken);

  return hsCode;
}
