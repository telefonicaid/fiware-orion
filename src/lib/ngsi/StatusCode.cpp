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
#include <string.h>
#include <stdlib.h>
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/string.h"
#include "common/JsonHelper.h"
#include "common/limits.h"
#include "ngsi/Request.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/UpdateContextResponse.h"
#include "rest/HttpStatusCode.h"



/* ****************************************************************************
*
* StatusCode::StatusCode -
*/
StatusCode::StatusCode()
{
  code         = SccNone;
  reasonPhrase = "";
  details      = "";
}




/* ****************************************************************************
*
* StatusCode::toJson -
*
* For version 2 of the API, based on OrionError.
*/
std::string StatusCode::toJson(void)
{
  OrionError oe(code, details, reasonPhrase);
  return oe.toJson();
}



/* ****************************************************************************
*
* StatusCode::fill -
*/
void StatusCode::fill(HttpStatusCode _code, const std::string& _details)
{
  code          = _code;
  reasonPhrase  = httpStatusCodeString(code);
  details       = _details;
}



/* ****************************************************************************
*
* StatusCode::fill -
*/
void StatusCode::fill(StatusCode* scP)
{
  fill(scP->code, scP->details);
}



/* ****************************************************************************
*
* StatusCode::fill -
*/
void StatusCode::fill(const StatusCode& sc)
{
  fill(sc.code, sc.details);
}



/* ****************************************************************************
*
* release -
*
* FIXME PR: rellay needed?
*/
void StatusCode::release(void)
{
  code         = SccNone;
  reasonPhrase = "";
  details      = "";
}

