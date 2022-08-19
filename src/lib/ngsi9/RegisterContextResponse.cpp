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
  errorCode.keyNameSet("errorCode");
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
  errorCode.keyNameSet("errorCode");
}



/* ****************************************************************************
*
* RegisterContextResponse::RegisterContextResponse - 
*/ 
RegisterContextResponse::RegisterContextResponse(const std::string& _registrationId, const std::string& _duration)
{
  registrationId.set(_registrationId);
  duration.set(_duration);
  errorCode.keyNameSet("errorCode");
}



/* ****************************************************************************
*
* RegisterContextResponse::RegisterContextResponse - 
*/ 
RegisterContextResponse::RegisterContextResponse(const std::string& _registrationId, StatusCode& _errorCode)
{
  registrationId.set(_registrationId);
  errorCode     = _errorCode;
  errorCode.keyNameSet("errorCode");
}



/* ****************************************************************************
*
* RegisterContextResponse::toJsonV1 -
*/
std::string RegisterContextResponse::toJsonV1(void)
{
  std::string  out = "";
  bool         errorCodeRendered = (errorCode.code != SccNone) && (errorCode.code != SccOk);

  out += startTag();

  if (!errorCodeRendered)
  {
    out += duration.toJsonV1(true);
  }

  out += registrationId.toJsonV1(RegisterResponse, errorCodeRendered);

  if (errorCodeRendered)
  {
    out += errorCode.toJsonV1(false);
  }

  out += endTag();

  return out;
}



/* ****************************************************************************
*
* RegisterContextResponse::check - 
*/
std::string RegisterContextResponse::check(const std::string& predetectedError)
{
  RegisterContextResponse  response;
  std::string              res;

  if (!predetectedError.empty())
  {
    response.errorCode.fill(SccBadRequest, predetectedError);
  }
  else if (((res = duration.check())       != "OK") ||
           ((res = registrationId.check()) != "OK"))
  {
    response.errorCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  return response.toJsonV1();
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
