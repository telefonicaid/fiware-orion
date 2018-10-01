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
* Author: Fermín Galán & Ken Zangelin
*/
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/tag.h"
#include "common/globals.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/NotifyContextResponse.h"
#include "rest/HttpStatusCode.h"



/* ****************************************************************************
*
* NotifyContextResponse::NotifyContextResponse - 
*/
NotifyContextResponse::NotifyContextResponse()
{
  responseCode.fill(SccOk);
}



/* ****************************************************************************
*
* NotifyContextResponse::NotifyContextResponse - 
*/
NotifyContextResponse::NotifyContextResponse(StatusCode& sc)
{
   responseCode.fill(&sc);
}



/* ****************************************************************************
*
* NotifyContextResponse::render -
*/
std::string NotifyContextResponse::toJsonV1(void)
{
  std::string out = "";

  responseCode.keyNameSet("responseCode");

  out += startTag();
  out += responseCode.toJsonV1(false);
  out += endTag();

  return out;
}



/* ****************************************************************************
*
* NotifyContextResponse::release - 
*/
void NotifyContextResponse::release(void)
{
  responseCode.release();
}
