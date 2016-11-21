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

#include "common/tag.h"
#include "ngsi/StatusCode.h"
#include "ngsi/ContextRegistrationResponse.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* ContextRegistrationResponse::ContextRegistrationResponse -
*/
ContextRegistrationResponse::ContextRegistrationResponse()
{
  errorCode.keyNameSet("errorCode");
}



/* ****************************************************************************
*
* ContextRegistrationResponse::render -
*/
std::string ContextRegistrationResponse::render(const std::string& indent, bool comma)
{
  std::string  out               = "";
  bool         errorCodeRendered = errorCode.code != SccNone;

  out += startTag(indent);

  out += contextRegistration.render(indent + "  ", errorCodeRendered, false);

  if (errorCodeRendered)
  {
    out += errorCode.render(indent + "  ", false);
  }

  out += endTag(indent, comma);

  return out;
}



/* ****************************************************************************
*
* ContextRegistrationResponse::check -
*/
std::string ContextRegistrationResponse::check
(
  const std::string&  apiVersion,
  RequestType         requestType,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  return contextRegistration.check(apiVersion, requestType, indent, predetectedError, counter);
}



/* ****************************************************************************
*
* ContextRegistrationResponse::present -
*/
void ContextRegistrationResponse::present(const std::string& indent)
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
