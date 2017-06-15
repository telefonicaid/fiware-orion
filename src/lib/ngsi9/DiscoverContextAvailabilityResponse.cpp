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
#include <stdio.h>
#include <string>

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "common/globals.h"
#include "common/string.h"
#include "ngsi9/DiscoverContextAvailabilityResponse.h"



/* ****************************************************************************
*
* DiscoverContextAvailabilityResponse::DiscoverContextAvailabilityResponse -
*/
DiscoverContextAvailabilityResponse::DiscoverContextAvailabilityResponse()
{
  errorCode.keyNameSet("errorCode");
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
  errorCode.fill(&_errorCode);
  errorCode.keyNameSet("errorCode");
}



/* ****************************************************************************
*
* DiscoverContextAvailabilityResponse::render -
*/
std::string DiscoverContextAvailabilityResponse::render
(
  int indent
)
{
  rapidjson::StringBuffer sb;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
  if (indent < 0)
  {
    indent = DEFAULT_JSON_INDENT;
  }
  writer.SetIndent(' ', indent);


  writer.StartObject();

  if (responseVector.size() > 0)
  {
    responseVector.toJson(writer);
  }

  if (errorCode.code != SccNone)
  {
    errorCode.toJsonV1(writer);
  }

  /* Safety check: neither errorCode nor CER vector was filled by mongoBackend */
  if (errorCode.code == SccNone && responseVector.size() == 0)
  {
      errorCode.fill(SccReceiverInternalError, "Both the error-code structure and the response vector were empty");
      errorCode.toJsonV1(writer);
  }

  writer.EndObject();

  return sb.GetString();
}



/* ****************************************************************************
*
* DiscoverContextAvailabilityResponse::release -
*/
void DiscoverContextAvailabilityResponse::release(void)
{
  responseVector.release();
  errorCode.release();
  errorCode.keyNameSet("errorCode");
}
