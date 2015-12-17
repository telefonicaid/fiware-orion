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

#include "common/tag.h"
#include "common/limits.h"
#include "ngsi/Request.h"
#include "rest/uriParamNames.h"
#include "orionTypes/EntityType.h"



/* ****************************************************************************
*
* EntityType::EntityType -
*/
EntityType::EntityType(): count(0)
{

}



/* ****************************************************************************
*
* EntityType::EntityType -
*/
EntityType::EntityType(std::string _type): type(_type), count(0)
{

}


/* ****************************************************************************
*
* EntityType::render -
*
* This method is used by:
*   o EntityTypeVector
*   o EntityTypeResponse
*
* 'typeNameBefore' is set to TRUE when called from EntityTypeResponse
*/
std::string EntityType::render
(
  ConnectionInfo*     ciP,
  const std::string&  indent,
  bool                comma,
  bool                typeNameBefore
)
{
  std::string  out            = "";
  std::string  xmlTag         = "entityType";
  std::string  jsonTag        = "type";

  if ((typeNameBefore == true) && (ciP->outFormat == JSON))
  {
    out += valueTag(indent  + "  ", "name", type, ciP->outFormat, true);
    out += contextAttributeVector.render(ciP, EntityTypes, indent + "  ", true, true, true);
  }
  else
  {
    out += startTag(indent, xmlTag, jsonTag, ciP->outFormat, false, false);

    if (ciP->uriParam[URI_PARAM_COLLAPSE] == "true" || contextAttributeVector.size() == 0)
    {
      out += valueTag(indent  + "  ", "name", type, ciP->outFormat, false);
    }
    else
    {
      out += valueTag(indent  + "  ", "name", type, ciP->outFormat, true);
      out += contextAttributeVector.render(ciP, EntityTypes, indent + "  ", false, true, true);
    }

    out += endTag(indent, xmlTag, ciP->outFormat, comma, false);
  }

  return out;
}



/* ****************************************************************************
*
* EntityType::check -
*/
std::string EntityType::check
(
  ConnectionInfo*     ciP,
  const std::string&  indent,
  const std::string&  predetectedError
)
{
  if (predetectedError != "")
  {
    return predetectedError;
  }
  else if (type == "")
  {
    return "Empty Type";
  }

  return contextAttributeVector.check(EntityTypes, ciP->outFormat, indent, "", 0);
}



/* ****************************************************************************
*
* EntityType::present -
*/
void EntityType::present(const std::string& indent)
{
  LM_T(LmtPresent,("%stype:   %s", indent.c_str(), type.c_str()));
  contextAttributeVector.present(indent);
}



/* ****************************************************************************
*
* EntityType::release -
*/
void EntityType::release(void)
{
  contextAttributeVector.release();
}



/* ****************************************************************************
*
* EntityType::toJson -
*/
std::string EntityType::toJson(ConnectionInfo* ciP)
{
  std::string  out = "{";
  char         countV[STRING_SIZE_FOR_INT];

  snprintf(countV, sizeof(countV), "%lld", count);

  out += JSON_STR("attrs") + ":";

  out += "{";
  out += contextAttributeVector.toJson(false, true, "normalized");
  out += "}";

  out += "," + JSON_STR("count") + ":" + countV;
  out += "}";

  return out;
}
