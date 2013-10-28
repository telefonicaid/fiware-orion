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
#include <vector>

#include "common/globals.h"
#include "common/Format.h"
#include "common/tag.h"
#include "convenience/RegisterProviderRequest.h"
#include "ngsi/StatusCode.h"
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
std::string RegisterProviderRequest::render(Format format, std::string indent)
{
  std::string out = "";
  std::string tag = "registerProviderRequest";

  out += startTag(indent, tag, format);
  out += metadataVector.render(format, indent + "  ");
  out += duration.render(format, indent + "  ");
  out += providingApplication.render(format, indent + "  ");
  out += registrationId.render(format, indent + "  ");
  out += endTag(indent, tag, format);

  return out;   
}



/* ****************************************************************************
*
* RegisterProviderRequest::check - 
*/
std::string RegisterProviderRequest::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
   DiscoverContextAvailabilityResponse  response;
   std::string                          res;

   if (predetectedError != "")
   {
      response.errorCode.code         = SccBadRequest;
      response.errorCode.reasonPhrase = predetectedError;
   }
   else if ((res = metadataVector.check(RegisterProvider, format, indent, predetectedError, counter)) != "OK")
   {
      response.errorCode.code = SccBadRequest;
      response.errorCode.reasonPhrase = res;
   }
   else
      return "OK";

   LM_W(("RegisterProviderRequest Error"));
   return response.render(DiscoverContextAvailability, format, indent);
}



/* ****************************************************************************
*
* RegisterProviderRequest::present - 
*/
void RegisterProviderRequest::present(std::string indent)
{
   PRINTF("%sRegisterProviderRequest:\n", indent.c_str());
   metadataVector.present("Registration", indent + "  ");
   duration.present(indent + "  ");
   providingApplication.present(indent + "  ");
   registrationId.present(indent + "  ");
   PRINTF("\n");
}



/* ****************************************************************************
*
* RegisterProviderRequest::release - 
*/
void RegisterProviderRequest::release(void)
{
   metadataVector.release();
}
