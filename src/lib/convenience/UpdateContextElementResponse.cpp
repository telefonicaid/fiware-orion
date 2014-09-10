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
#include <vector>

#include "common/Format.h"
#include "common/tag.h"
#include "convenience/ContextAttributeResponse.h"
#include "ngsi/StatusCode.h"
#include "convenience/UpdateContextElementResponse.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* UpdateContextElementResponse::UpdateContextElementResponse - 
*/
UpdateContextElementResponse::UpdateContextElementResponse()
{
  errorCode.tagSet("errorCode");
}



/* ****************************************************************************
*
* render - 
*/
std::string UpdateContextElementResponse::render(ConnectionInfo* ciP, RequestType requestType, const std::string& indent)
{
   std::string tag = "updateContextElementResponse";
   std::string out = "";

   out += startTag(indent, tag, ciP->outFormat, false);

   if ((errorCode.code != SccNone) && (errorCode.code != SccOk))
     out += errorCode.render(ciP->outFormat, indent + "  ");
   else
     out += contextAttributeResponseVector.render(ciP, requestType, indent + "  ");

   out += endTag(indent, tag, ciP->outFormat);

   return out;
}



/* ****************************************************************************
*
* check - 
*/
std::string UpdateContextElementResponse::check(ConnectionInfo* ciP, RequestType requestType, const std::string& indent, const std::string& predetectedError, int counter)
{
  std::string res;
  
  if (predetectedError != "")
  {
    errorCode.fill(SccBadRequest, predetectedError);
  }
  else if ((res = contextAttributeResponseVector.check(ciP, requestType, indent, "", counter)) != "OK")
  {
    errorCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return render(ciP, requestType, indent);
}



/* ****************************************************************************
*
* UpdateContextElementResponse::release - 
*/
void UpdateContextElementResponse::release(void)
{
  contextAttributeResponseVector.release();
  errorCode.release();
}
