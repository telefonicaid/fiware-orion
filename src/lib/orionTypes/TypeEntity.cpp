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

#include "logMsg/logMsg.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "rest/uriParamNames.h"
#include "orionTypes/TypeEntity.h"



/* ****************************************************************************
*
* TypeEntity::TypeEntity -
*/
TypeEntity::TypeEntity()
{
  type = "";
}



/* ****************************************************************************
*
* TypeEntity::TypeEntity -
*/
TypeEntity::TypeEntity(std::string  _type)
{
  type = _type;
}


/* ****************************************************************************
*
* TypeEntity::render -
*
* This method is used by:
*   o TypeEntityVector
*   o EntityTypeAttributesResponse
*
* 'typeNameBefore' is set to TRUE when called from EntityTypeAttributesResponse
*/
std::string TypeEntity::render
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
* TypeEntity::check -
*/
std::string TypeEntity::check
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
* TypeEntity::present -
*/
void TypeEntity::present(const std::string& indent)
{
  LM_F(("%stype:   %s", indent.c_str(), type.c_str()));
  contextAttributeVector.present(indent);
}



/* ****************************************************************************
*
* TypeEntity::release -
*/
void TypeEntity::release(void)
{
  contextAttributeVector.release();
}
