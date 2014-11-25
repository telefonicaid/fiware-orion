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
#include "ngsi/EntityAssociation.h"



/* ****************************************************************************
*
* EntityAssociation::EntityAssociation -
*/
EntityAssociation::EntityAssociation() : source("", "", "", "sourceEntityId"), target("", "", "", "targetEntityId")
{
}



/* ****************************************************************************
*
* render -
*/
std::string EntityAssociation::render(Format format, const std::string& indent, bool comma)
{
  std::string out;
  std::string xmlTag = "entityAssociation";
  std::string jsonTag = "entities";

  out += startTag(indent, xmlTag, jsonTag, format, false, true);
  out += source.render(format, indent + "  ", true, true, "source");
  out += target.render(format, indent + "  ", false, true, "target");
  out += endTag(indent, xmlTag, format, comma);

  return out;
}



/* ****************************************************************************
*
* check -
*/
std::string EntityAssociation::check
(
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  std::string res;

  if ((res = source.check(requestType, format, indent, predetectedError, counter)) != "OK")
  {
    return res;
  }

  if ((res = target.check(requestType, format, indent, predetectedError, counter)) != "OK")
  {
    return res;
  }

  return "OK";
}
