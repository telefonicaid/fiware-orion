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
* fermin at tid dot es
*
* Author: Fermin Galan Marquez
*/
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoQueryContext.h"

#include "ngsi/ContextRegistrationResponse.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"

/* ****************************************************************************
*
* mongoQueryContext - 
*/
HttpStatusCode mongoQueryContext
(
  QueryContextRequest*                 requestP,
  QueryContextResponse*                responseP,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV,
  std::map<std::string, std::string>&  uriParams
)
{
    int         offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
    int         limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());
    std::string detailsString  = uriParams[URI_PARAM_PAGINATION_DETAILS];
    bool        details        = (strcasecmp("on", detailsString.c_str()) == 0)? true : false;

    LM_T(LmtMongo, ("QueryContext Request"));    
    LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));

    /* FIXME: restriction not supported for the moment */
    if (!requestP->restriction.attributeExpression.isEmpty())
    {
      LM_W(("Bad Input (restriction found, but restrictions are not supported by mongo backend)"));
    }

    std::string err;
    bool        ok;
    long long   count = -1;

    reqSemTake(__FUNCTION__, "ngsi10 query request");
    ok = entitiesQuery(requestP->entityIdVector,
                       requestP->attributeList,
                       requestP->restriction,
                       &responseP->contextElementResponseVector,
                       &err,
                       true,
                       tenant,
                       servicePathV,
                       offset,
                       limit,
                       details,
                       &count);
    reqSemGive(__FUNCTION__, "ngsi10 query request");

    if (!ok)
    {
        responseP->errorCode.fill(SccReceiverInternalError, err);
    }
    else if (responseP->contextElementResponseVector.size() == 0)
    {
      //
      // If the query has an empty response, we have to fill in the status code part in the response.
      //
      // However, if the response was empty due to a too high pagination offset,
      // and if the user has asked for 'details' (as URI parameter, then the response should include information about
      // the number of hits without pagination.
      //

      if (details)
      {
        if ((count > 0) && (offset >= count))
        {
          char details[256];

          snprintf(details, sizeof(details), "Number of matching entities: %lld. Offset is %d", count, offset);
          responseP->errorCode.fill(SccContextElementNotFound, details);
          return SccOk;
        }
      }

      responseP->errorCode.fill(SccContextElementNotFound);
    }
    else if (details == true)
    {
      //
      // If all was OK, but the details URI param was set to 'on', then the responses error code details
      // 'must' contain the total count of hits.
      //

      char details[64];

      snprintf(details, sizeof(details), "Count: %lld", count);
      responseP->errorCode.fill(SccOk, details);
    }

    return SccOk;
}
