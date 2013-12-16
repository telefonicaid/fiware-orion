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
* Author: Ken Zangelin
*/
#include <string>

#include "common/Format.h"
#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/AttributeList.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/Duration.h"
#include "ngsi/Reference.h"
#include "ngsi/Restriction.h"
#include "ngsi/StatusCode.h"
#include "ngsi/SubscriptionId.h"
#include "ngsi9/SubscribeContextAvailabilityResponse.h"
#include "ngsi9/SubscribeContextAvailabilityRequest.h"

/* ****************************************************************************
*
* SubscribeContextAvailabilityRequest::SubscribeContextAvailabilityRequest
*
* Explicit constructor needed to initialize primitive types so they don't get
* random values from the stack
*/
SubscribeContextAvailabilityRequest::SubscribeContextAvailabilityRequest()
{
  restrictions = 0;
}

/* ****************************************************************************
*
* SubscribeContextAvailabilityRequest::render - 
*/
std::string SubscribeContextAvailabilityRequest::render(RequestType requestType, Format format, std::string indent)
{
  std::string out      = "";
  std::string tag      = "subscribeContextAvailabilityRequest";
  std::string indent2  = indent + "  ";

  out += startTag(indent, tag, format, false);
  out += entityIdVector.render(format, indent2);
  out += attributeList.render(format, indent2);
  out += reference.render(format, indent2, true); // FIXME P9: durationRendered || restrictionRendered || subscriptionIdRendered
  out += duration.render(format, indent2, true);  // FIXME P9: restrictionRendered || subscriptionIdRendered
  out += restriction.render(format, indent2);
  out += subscriptionId.render(format, indent2);
  out += endTag(indent, tag, format);

  return out;
}



/* ****************************************************************************
*
* SubscribeContextAvailabilityRequest::check - 
*/
std::string SubscribeContextAvailabilityRequest::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  SubscribeContextAvailabilityResponse response;
  std::string                          res;

  if (predetectedError != "")
  {
    response.errorCode.code         = SccBadRequest;
    response.errorCode.reasonPhrase = predetectedError;
  }
  else if (((res = entityIdVector.check(SubscribeContextAvailability, format, indent, predetectedError, counter))   != "OK") ||
           ((res = attributeList.check(SubscribeContextAvailability, format, indent, predetectedError, counter))    != "OK") ||
           ((res = reference.check(SubscribeContextAvailability, format, indent, predetectedError, counter))        != "OK") ||
           ((res = duration.check(SubscribeContextAvailability, format, indent, predetectedError, counter))         != "OK") ||
           ((res = restriction.check(SubscribeContextAvailability, format, indent, predetectedError, restrictions)) != "OK"))
  {
    response.errorCode.code         = SccBadRequest;
    response.errorCode.reasonPhrase = res;
  }
  else
    return "OK";

  return response.render(SubscribeContextAvailability, format, indent);
}



/* ****************************************************************************
*
* SubscribeContextAvailabilityRequest::release - 
*/
void SubscribeContextAvailabilityRequest::release(void)
{
   entityIdVector.release();
   restriction.release();
   attributeList.release();
}



/* ****************************************************************************
*
* SubscribeContextAvailabilityRequest::present - 
*/
void SubscribeContextAvailabilityRequest::present(std::string indent)
{
   entityIdVector.present(indent);
   attributeList.present(indent);
   reference.present(indent);
   duration.present(indent);
   restriction.present(indent);   
   subscriptionId.present(indent);
}
