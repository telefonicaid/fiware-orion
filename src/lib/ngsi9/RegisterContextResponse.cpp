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

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "common/globals.h"
#include "common/tag.h"
#include "common/Format.h"
#include "ngsi/StatusCode.h"
#include "ngsi/StatusCode.h"
#include "ngsi9/RegisterContextRequest.h"
#include "ngsi9/RegisterContextResponse.h"



/* ****************************************************************************
*
* RegisterContextResponse::RegisterContextResponse - 
*/ 
RegisterContextResponse::RegisterContextResponse()
{
  registrationId.set("");
  duration.set("");
  errorCode.tagSet("errorCode");
}

/* ****************************************************************************
*
* RegisterContextResponse::~RegisterContextResponse -
*/
RegisterContextResponse::~RegisterContextResponse()
{
    LM_T(LmtDestructor,("destroyed"));
}


/* ****************************************************************************
*
* RegisterContextResponse::RegisterContextResponse - 
*/ 
RegisterContextResponse::RegisterContextResponse(RegisterContextRequest* rcrP)
{
  registrationId.set(rcrP->registrationId.get());
  duration.set(rcrP->duration.get());
  errorCode.tagSet("errorCode");
}



/* ****************************************************************************
*
* RegisterContextResponse::RegisterContextResponse - 
*/ 
RegisterContextResponse::RegisterContextResponse(const std::string& _registrationId, const std::string& _duration)
{
  registrationId.set(_registrationId);
  duration.set(_duration);
  errorCode.tagSet("errorCode");
}



/* ****************************************************************************
*
* RegisterContextResponse::RegisterContextResponse - 
*/ 
RegisterContextResponse::RegisterContextResponse(const std::string& _registrationId, StatusCode& _errorCode)
{
  registrationId.set(_registrationId);
  errorCode     = _errorCode;
  errorCode.tagSet("errorCode");
}



/* ****************************************************************************
*
* RegisterContextResponse::render - 
*/
std::string RegisterContextResponse::render(RequestType requestType, Format format, const std::string& indent)
{
  std::string  out = "";
  std::string  tag = "registerContextResponse";
  bool         errorCodeRendered = (errorCode.code != SccNone) && (errorCode.code != SccOk);

  //out += startTag(indent, tag, format, false);
  out += startTag1(indent, tag, false);

  out += duration.render(format, indent + "  ", true);
  out += registrationId.render(RegisterResponse, format, indent + "  ", errorCodeRendered);

  if (errorCodeRendered)
    out += errorCode.render(format, indent + "  ");

  out += endTag(indent);

  return out;
}



/* ****************************************************************************
*
* RegisterContextResponse::check - 
*/
std::string RegisterContextResponse::check(RequestType requestType, Format format, const std::string& indent, const std::string& predetectedError, int counter)
{
  RegisterContextResponse  response;
  std::string              res;

  if (predetectedError != "")
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = duration.check(RegisterResponse, format, indent, predetectedError, counter))       != "OK") ||
           ((res = registrationId.check(RegisterResponse, format, indent, predetectedError, counter)) != "OK"))
  {
    response.errorCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  return response.render(RegisterContext, format, indent);
}



/* ****************************************************************************
*
* present - 
*/
void RegisterContextResponse::present(const std::string& indent)
{
   registrationId.present(indent);
   duration.present(indent);
   errorCode.present(indent);
}



/* ****************************************************************************
*
* release - 
*/
void RegisterContextResponse::release(void)
{
   duration.release();
   registrationId.release();
   errorCode.release();
}
