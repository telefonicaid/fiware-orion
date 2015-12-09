/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "common/Format.h"
#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "rest/uriParamNames.h"
#include "orionTypes/EntityTypeResponse.h"



/* ****************************************************************************
*
* EntityTypeResponse::render -
*/
std::string EntityTypeResponse::render(ConnectionInfo* ciP, const std::string& indent)
{
  std::string out                 = "";
  std::string tag                 = "entityTypeAttributesResponse";

  out += startTag(indent, tag, ciP->outFormat, false);

  out += entityType.render(ciP, indent + "  ", true, true);
  out += statusCode.render(ciP->outFormat, indent + "  ");

  out += endTag(indent, tag, ciP->outFormat);

  return out;
}



/* ****************************************************************************
*
* EntityTypeResponse::check -
*/
std::string EntityTypeResponse::check
(
  ConnectionInfo*     ciP,
  const std::string&  indent,
  const std::string&  predetectedError
)
{
  std::string res;

  if (predetectedError != "")
  {
    statusCode.fill(SccBadRequest, predetectedError);
  }
  else if ((res = entityType.check(ciP, indent, predetectedError)) != "OK")
  {
    LM_W(("Bad Input (%s)", res.c_str()));
    statusCode.fill(SccBadRequest, res);
  }
  else
    return "OK";

  return render(ciP, "");
}



/* ****************************************************************************
*
* EntityTypeResponse::present -
*/
void EntityTypeResponse::present(const std::string& indent)
{
  LM_T(LmtPresent,("%sEntityTypeResponse:\n", indent.c_str()));
  entityType.present(indent + "  ");
  statusCode.present(indent + "  ");
}



/* ****************************************************************************
*
* EntityTypeResponse::release -
*/
void EntityTypeResponse::release(void)
{
  entityType.release();
  statusCode.release();
}



/* ****************************************************************************
*
* EntityTypeResponse::toJson -
*/
std::string EntityTypeResponse::toJson(ConnectionInfo* ciP)
{
  std::string  out = "{";
  char         countV[16];

  snprintf(countV, sizeof(countV), "%lld", entityType.count);

  out += JSON_STR("attrs") + ":";

  out += "{";
  out += entityType.contextAttributeVector.toJson(false, true);
  out += "}";  

  out += "," + JSON_STR("count") + ":" + countV;
  out += "}";

  return out;
}
