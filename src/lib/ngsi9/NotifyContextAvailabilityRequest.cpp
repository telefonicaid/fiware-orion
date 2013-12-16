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
* Author: Fermin Galan
*/
#include <string>

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "ngsi9/NotifyContextAvailabilityRequest.h"
#include "ngsi9/NotifyContextAvailabilityResponse.h"



/* ****************************************************************************
*
* NotifyContextAvailabilityRequest::NotifyContextAvailabilityRequest - 
*/
NotifyContextAvailabilityRequest::NotifyContextAvailabilityRequest()
{
}



/* ****************************************************************************
*
* NotifyContextAvailabilityRequest::render -
*/
std::string NotifyContextAvailabilityRequest::render(RequestType requestType, Format format, std::string indent)
{
  std::string out = "";
  std::string tag = "notifyContextAvailabilityRequest";

  //
  // Note on JSON commas:
  //  Both subscriptionId and contextRegistrationResponseVector are MANDATORY.
  //  Always comma for subscriptionId.
  //  With an empty contextRegistrationResponseVector there would be no notification
  //
  out += startTag(indent, tag, format, false);
  out += subscriptionId.render(format, indent + "  ", true);
  out += contextRegistrationResponseVector.render(format, indent  + "  ", false);
  out += endTag(indent, tag, format);

  return out;
}



/* ****************************************************************************
*
* NotifyContextAvailabilityRequest::check - 
*/
std::string NotifyContextAvailabilityRequest::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  std::string                        res;
  NotifyContextAvailabilityResponse  response;

  if (predetectedError != "")
  {
    response.responseCode.code         = SccBadRequest;
    response.responseCode.reasonPhrase = predetectedError;
  }
  else if (((res = subscriptionId.check(QueryContext, format, indent, predetectedError, 0))                    != "OK") ||
           ((res = contextRegistrationResponseVector.check(QueryContext, format, indent, predetectedError, 0)) != "OK"))
  {
    response.responseCode.code         = SccBadRequest;
    response.responseCode.reasonPhrase = res;
  }
  else
    return "OK";

  return response.render(NotifyContextAvailability, format, indent);
}


/* ****************************************************************************
*
* NotifyContextAvailabilityRequest::present -
*/
void NotifyContextAvailabilityRequest::present(std::string indent)
{
  subscriptionId.present(indent);
  contextRegistrationResponseVector.present(indent);
}



/* ****************************************************************************
*
* NotifyContextAvailabilityRequest::release -
*/
void NotifyContextAvailabilityRequest::release(void)
{
  contextRegistrationResponseVector.release();
}
