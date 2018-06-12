/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <vector>

#include "common/clockFunctions.h"
#include "common/statistics.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "alarmMgr/alarmMgr.h"
#include "apiTypesV2/SubscriptionUpdate.h"
#include "jsonParseV2/parseEntity.h"
#include "jsonParseV2/parseContextAttribute.h"
#include "jsonParseV2/parseAttributeValue.h"
#include "jsonParseV2/parseSubscription.h"
#include "jsonParseV2/parseBatchQuery.h"
#include "jsonParseV2/parseBatchUpdate.h"
#include "jsonParseV2/parseRegistration.h"
#include "jsonParseV2/parseNotification.h"
#include "jsonParseV2/jsonRequestTreat.h"



/* ****************************************************************************
*
* jsonRequestTreat - 
*/
std::string jsonRequestTreat
(
  ConnectionInfo*            ciP,
  ParseData*                 parseDataP,
  RequestType                requestType,
  JsonDelayedRelease*        releaseP,
  std::vector<std::string>&  compV
)
{
  std::string      answer;
  struct timespec  start;
  struct timespec  end;

  if (timingStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &start);
  }

  switch (requestType)
  {
  case EntitiesRequest:  // POST /v2/entities
    releaseP->entity = &parseDataP->ent.res;
    answer = parseEntity(ciP, &parseDataP->ent.res, false);
    if (answer != "OK")
    {
      return answer;
    }

    if ((answer = parseDataP->ent.res.check(EntitiesRequest)) != "OK")
    {
      OrionError oe(SccBadRequest, answer);
      return oe.setStatusCodeAndSmartRender(ciP->apiVersion, &(ciP->httpStatusCode));
    }
    break;

  case EntityRequest:  // POST|PUT /v2/entities/<eid>
    releaseP->entity = &parseDataP->ent.res;
    answer = parseEntity(ciP, &parseDataP->ent.res, true);
    if (answer != "OK")
    {
      return answer;
    }

    if ((answer = parseDataP->ent.res.check(EntityRequest)) != "OK")
    {
      OrionError oe(SccBadRequest, answer);
      return oe.setStatusCodeAndSmartRender(ciP->apiVersion, &(ciP->httpStatusCode));
    }
    break;

  case EntityAttributeRequest:
    releaseP->attribute = &parseDataP->attr.attribute;
    releaseP->attribute->name = compV[4];
    answer = parseContextAttribute(ciP, &parseDataP->attr.attribute);
    if (answer != "OK")
    {
      return answer;
    }

    if ((answer = parseDataP->attr.attribute.check(ciP->apiVersion, EntityAttributeRequest)) != "OK")
    {
      OrionError oe(SccBadRequest, answer);
      return oe.setStatusCodeAndSmartRender(ciP->apiVersion, &(ciP->httpStatusCode));
    }
    break;

  case EntityAttributeValueRequest:
    releaseP->attribute = &parseDataP->av.attribute;
    answer = parseAttributeValue(ciP, &parseDataP->av.attribute);
    if (answer != "OK")
    {
      return answer;
    }
    break;

  case SubscriptionsRequest:
    answer = parseSubscription(ciP, &parseDataP->subsV2);
    if (answer != "OK")
    {
      return answer;
    }
    break;

  case IndividualSubscriptionRequest:
    answer = parseSubscription(ciP, &parseDataP->subsV2, true);  // NOTE: partial == true
    if (answer != "OK")
    {
      return answer;
    }
    break;

  case BatchQueryRequest:
    answer = parseBatchQuery(ciP, &parseDataP->bq.res);
    if (answer != "OK")
    {
      return answer;
    }
    break;

  case BatchUpdateRequest:
    answer = parseBatchUpdate(ciP, &parseDataP->bu.res);
    if (answer != "OK")
    {
      return answer;
    }
    break;

  case RegistrationsRequest:
    answer = parseRegistration(ciP, &parseDataP->reg);
    if (answer != "OK")
    {
      return answer;
    }
    break;

  case NotifyContext:
    answer = parseNotification(ciP, &parseDataP->ncr.res);
    if (answer != "OK")
    {
      return answer;
    }
    break;

  default:
    OrionError error(SccNotImplemented, "Request Treat function not implemented");
    answer = error.render();
    ciP->httpStatusCode = SccNotImplemented;
    break;
  }

  if (timingStatistics)
  {
    clock_gettime(CLOCK_REALTIME, &end);
    clock_difftime(&end, &start, &threadLastTimeStat.jsonV2ParseTime);
  }

  return answer;
}
