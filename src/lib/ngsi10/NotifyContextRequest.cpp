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
#include <string>

#include "common/globals.h"
#include "common/tag.h"
#include "common/RenderFormat.h"
#include "ngsi10/NotifyContextRequest.h"
#include "ngsi10/NotifyContextResponse.h"
#include "rest/OrionError.h"
#include "alarmMgr/alarmMgr.h"



/* ****************************************************************************
*
* NotifyContextRequest::render -
*/
std::string NotifyContextRequest::render(ApiVersion apiVersion, bool asJsonObject)
{
  std::string  out                                  = "";
  bool         contextElementResponseVectorRendered = contextElementResponseVector.size() != 0;

  //
  // Note on JSON commas:
  //   subscriptionId and originator are MANDATORY.
  //   The only doubt here if whether originator should end in a comma.
  //   This doubt is taken care of by the variable 'contextElementResponseVectorRendered'
  //
  out += startTag();
  out += subscriptionId.render(NotifyContext, true);
  out += originator.render(contextElementResponseVectorRendered);
  out += contextElementResponseVector.render(apiVersion, asJsonObject, NotifyContext, false);
  out += endTag();

  return out;
}



/* ****************************************************************************
*
* NotifyContextRequest::toJson -
*/
std::string NotifyContextRequest::toJson
(
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsFilter,
  const std::vector<std::string>&  metadataFilter,
  bool                             blacklist
)
{
  if ((renderFormat != NGSI_V2_NORMALIZED) && (renderFormat != NGSI_V2_KEYVALUES) && (renderFormat != NGSI_V2_VALUES))
  {
    OrionError oe(SccBadRequest, "Invalid notification format");
    alarmMgr.badInput(clientIp, "Invalid notification format");

    return oe.toJson();
  }

  std::string out;

  out += "{";
  out += JSON_STR("subscriptionId") + ":";
  out += JSON_STR(subscriptionId.get());
  out += ",";
  out += JSON_STR("data") + ":[";

  out += contextElementResponseVector.toJson(renderFormat, attrsFilter, metadataFilter, blacklist);
  out += "]";
  out += "}";

  return out;
}



/* ****************************************************************************
*
* NotifyContextRequest::check
*/
std::string NotifyContextRequest::check(ApiVersion apiVersion, const std::string& predetectedError)
{
  std::string            res;
  NotifyContextResponse  response;

  if (predetectedError != "")
  {
    response.responseCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = subscriptionId.check())                    != "OK") ||
           ((res = originator.check())                        != "OK") ||
           ((res = contextElementResponseVector.check(apiVersion, QueryContext, predetectedError, 0)) != "OK"))
  {
    response.responseCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return response.render();
}



/* ****************************************************************************
*
* NotifyContextRequest::release -
*/
void NotifyContextRequest::release(void)
{
  contextElementResponseVector.release();
}



/* ****************************************************************************
*
* NotifyContextRequest::clone -
*/
NotifyContextRequest* NotifyContextRequest::clone(void)
{
  NotifyContextRequest* ncrP = new NotifyContextRequest();

  ncrP->subscriptionId = subscriptionId;
  ncrP->originator     = originator;

  ncrP->contextElementResponseVector.fill(contextElementResponseVector);

  return ncrP;
}
