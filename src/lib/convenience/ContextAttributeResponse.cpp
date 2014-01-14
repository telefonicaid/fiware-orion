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

#include "logMsg/logMsg.h"

#include "common/Format.h"
#include "common/tag.h"
#include "ngsi/ContextAttributeVector.h"
#include "ngsi/StatusCode.h"
#include "convenience/ContextAttributeResponse.h"



/* ****************************************************************************
*
* render - 
*/
std::string ContextAttributeResponse::render(Format format, std::string indent)
{
  std::string tag = "contextAttributeResponse";
  std::string out = "";

  out += startTag(indent, tag, format);
  out += contextAttributeVector.render(format, indent + "  ");
  out += statusCode.render(format, indent + "  ");
  out += endTag(indent, tag, format);

  return out;
}



/* ****************************************************************************
*
* check - 
*/
std::string ContextAttributeResponse::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
   std::string  res;

   if (predetectedError != "")
   {
     statusCode.code         = SccBadRequest;
     statusCode.reasonPhrase = predetectedError;
   }
   else if ((res = contextAttributeVector.check(requestType, format, indent, predetectedError, counter)) != "OK")
   {
     LM_E(("contextAttributeVector::check flags an error: '%s'", res.c_str()));
     statusCode.code         = SccBadRequest;
     statusCode.reasonPhrase = res;

     //
     // If this ContextAttributeResponse is part of an IndividualContextEntity, the complete rendered 
     // response is not desired, just the string returned from the check method
     //
     if (requestType == IndividualContextEntity)
       return res;
   }
   else 
      return "OK";

   res = render(format, indent);

   return render(format, indent);
}



/* ****************************************************************************
*
* present - 
*/
void ContextAttributeResponse::present(std::string indent)
{
  contextAttributeVector.present(indent);
  statusCode.present(indent);
}



/* ****************************************************************************
*
* release - 
*/
void ContextAttributeResponse::release(void)
{
   contextAttributeVector.release();
}
