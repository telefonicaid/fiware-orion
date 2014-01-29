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
#include <stdio.h>
#include <string>

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "common/globals.h"
#include "common/string.h"
#include "common/tag.h"
#include "common/Format.h"
#include "ngsi9/DiscoverContextAvailabilityResponse.h"



/* ****************************************************************************
*
* DiscoverContextAvailabilityResponse::DiscoverContextAvailabilityResponse - 
*/
DiscoverContextAvailabilityResponse::DiscoverContextAvailabilityResponse()
{
  errorCode.tagSet("errorCode");
}

/* ****************************************************************************
*
* DiscoverContextAvailabilityResponse::~DiscoverContextAvailabilityResponse -
*/
DiscoverContextAvailabilityResponse::~DiscoverContextAvailabilityResponse()
{
  responseVector.release();
  errorCode.release();
}


/* ****************************************************************************
*
* DiscoverContextAvailabilityResponse::DiscoverContextAvailabilityResponse - 
*/
DiscoverContextAvailabilityResponse::DiscoverContextAvailabilityResponse(StatusCode& _errorCode)
{
  errorCode.code         = _errorCode.code;
  errorCode.reasonPhrase = _errorCode.reasonPhrase;
  errorCode.details      = _errorCode.details;

  errorCode.tagSet("errorCode");
}



/* ****************************************************************************
*
* DiscoverContextAvailabilityResponse::render - 
*/
std::string DiscoverContextAvailabilityResponse::render(RequestType requestType, Format format, std::string indent)
{
  std::string  out = "";
  std::string  tag = "discoverContextAvailabilityResponse";

  //
  // JSON commas:
  // Exactly ONE of responseVector|errorCode is included in the discovery response so,
  // no JSON commas necessary
  //
  out += startTag(indent, tag, format, false);

  if (errorCode.code == SccNone)
     out += responseVector.render(format, indent + "  ", false);
  else
     out += errorCode.render(format, indent + "  ", false);

  out += endTag(indent, tag, format);

  return out;
}



/* ****************************************************************************
*
* DiscoverContextAvailabilityResponse::release - 
*/
void DiscoverContextAvailabilityResponse::release(void)
{
  responseVector.release();
  errorCode.release();
}
