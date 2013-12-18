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
#include "common/tag.h"
#include "ngsi/ErrorCode.h"
#include "ngsi/ContextRegistrationResponse.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* ContextRegistrationResponse::ContextRegistrationResponse - 
*/
ContextRegistrationResponse::ContextRegistrationResponse()
{
  errorCode.code = NO_ERROR_CODE;
}



/* ****************************************************************************
*
* ContextRegistrationResponse::render - 
*/
std::string ContextRegistrationResponse::render(Format format, std::string indent, bool comma)
{
  std::string  xmlTag            = "contextRegistrationResponse";
  std::string  jsonTag           = "contextRegistration";
  std::string  out               = "";
  bool         errorCodeRendered = errorCode.code != NO_ERROR_CODE;

  out += startTag(indent, xmlTag, jsonTag, format, false, false);

  out += contextRegistration.render(format, indent + "  ", errorCodeRendered, false);

  if (errorCodeRendered)
     out += errorCode.render(format, indent + "  ", false);

  out += endTag(indent, xmlTag, format, comma);

  return out;
}



/* ****************************************************************************
*
* ContextRegistrationResponse::check - 
*/
std::string ContextRegistrationResponse::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
   return contextRegistration.check(requestType, format, indent, predetectedError, counter);
}



/* ****************************************************************************
*
* ContextRegistrationResponse::present - 
*/
void ContextRegistrationResponse::present(std::string indent)
{
  contextRegistration.present(indent, -1);
  errorCode.present(indent);
}



/* ****************************************************************************
*
* ContextRegistrationResponse::release - 
*/
void ContextRegistrationResponse::release(void)
{
  contextRegistration.release();
}
