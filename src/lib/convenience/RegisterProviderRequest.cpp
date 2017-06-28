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
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "alarmMgr/alarmMgr.h"

#include "convenience/RegisterProviderRequest.h"
#include "ngsi/MetadataVector.h"
#include "ngsi/Duration.h"
#include "ngsi/ProvidingApplication.h"
#include "ngsi/RegistrationId.h"
#include "ngsi9/DiscoverContextAvailabilityResponse.h"



/* ****************************************************************************
*
* Constructor - 
*/
RegisterProviderRequest::RegisterProviderRequest()
{
}



/* ****************************************************************************
*
* RegisterProviderRequest::render - 
*/
std::string RegisterProviderRequest::render
(
  int         indent
)
{
  if (indent < 0) {
    indent = DEFAULT_JSON_INDENT_V1;
  }
  JsonHelper writer(indent);


  writer.StartObject();

  metadataVector.toJsonV1(writer);
  duration.toJson(writer);
  providingApplication.toJson(writer);
  registrationId.toJson(writer, RegisterContext);

  writer.EndObject();

  return writer.str();
}



/* ****************************************************************************
*
* RegisterProviderRequest::check - 
*/
std::string RegisterProviderRequest::check
(
  ApiVersion          apiVersion,
  RequestType         requestType,
  std::string         indent,
  const std::string&  predetectedError
)
{
  DiscoverContextAvailabilityResponse  response;
  std::string                          res;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = metadataVector.check(apiVersion))                        != "OK") ||
           ((res = duration.check(requestType, indent, "", 0))              != "OK") ||
           ((res = providingApplication.check(requestType, indent, "", 0))  != "OK") ||
           ((res = registrationId.check(requestType, indent, "", 0))        != "OK"))
  {
    response.errorCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  std::string details = std::string("RegisterProviderRequest Error: '") + res + "'";
  alarmMgr.badInput(clientIp, details);

  return response.render();
}



/* ****************************************************************************
*
* RegisterProviderRequest::present - 
*/
void RegisterProviderRequest::present(std::string indent)
{
  LM_T(LmtPresent, ("%sRegisterProviderRequest:\n", indent.c_str()));
  metadataVector.present("Registration", indent + "  ");
  duration.present(indent + "  ");
  providingApplication.present(indent + "  ");
  registrationId.present(indent + "  ");
  LM_T(LmtPresent, ("\n"));
}



/* ****************************************************************************
*
* RegisterProviderRequest::release - 
*/
void RegisterProviderRequest::release(void)
{
  metadataVector.release();
}
