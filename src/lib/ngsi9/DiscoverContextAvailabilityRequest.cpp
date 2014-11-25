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

#include "logMsg/logMsg.h"

#include "ngsi/StatusCode.h"
#include "ngsi/Request.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/AttributeList.h"
#include "ngsi/Restriction.h"
#include "ngsi9/DiscoverContextAvailabilityRequest.h"
#include "ngsi9/DiscoverContextAvailabilityResponse.h"

/* ****************************************************************************
*
* DiscoverContextAvailabilityRequest::DiscoverContextAvailabilityRequest
*
* Explicit constructor needed to initialize primitive types so they don't get
* random values from the stack
*/
DiscoverContextAvailabilityRequest::DiscoverContextAvailabilityRequest()
{
  restrictions = 0;
}

/* ****************************************************************************
*
* DiscoverContextAvailabilityRequest::release - 
*/
void DiscoverContextAvailabilityRequest::release(void)
{
  entityIdVector.release();
  attributeList.release();
  restriction.release();
}



/* ****************************************************************************
*
* DiscoverContextAvailabilityRequest::check - 
*/
std::string DiscoverContextAvailabilityRequest::check(RequestType requestType, Format format, const std::string& indent, const std::string& predetectedError, int counter)
{
  DiscoverContextAvailabilityResponse  response;
  std::string                          res;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (entityIdVector.size() == 0)
  {
    response.errorCode.fill(SccContextElementNotFound);
  }
  else if (((res = entityIdVector.check(DiscoverContextAvailability, format, indent, predetectedError, restrictions))                      != "OK") ||
           ((res = attributeList.check(DiscoverContextAvailability, format, indent, predetectedError, restrictions))                       != "OK") ||
           ((restrictions != 0) && ((res = restriction.check(DiscoverContextAvailability, format, indent, predetectedError, restrictions)) != "OK")))
  {
    response.errorCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  return response.render(DiscoverContextAvailability, format, indent);
}



/* ****************************************************************************
*
* DiscoverContextAvailabilityRequest::present - 
*/
void DiscoverContextAvailabilityRequest::present(const std::string& indent)
{
   entityIdVector.present(indent);
   attributeList.present(indent);
   restriction.present(indent);
}
