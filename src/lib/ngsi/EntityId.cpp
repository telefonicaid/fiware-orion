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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/Format.h"
#include "ngsi/EntityId.h"



/* ****************************************************************************
*
* EntityId::EntityId -
*/
EntityId::EntityId() : tag("entityId")
{
}



/* ****************************************************************************
*
* EntityId::EntityId -
*/
EntityId::EntityId(std::string _id, std::string _type, std::string _isPattern, std::string _tag) : id(_id), type(_type), isPattern(_isPattern), tag(_tag)
{
}



/* ****************************************************************************
*
* tagSet - 
*/
void EntityId::tagSet(std::string tagName)
{
  tag = tagName;
}



/* ****************************************************************************
*
* EntityId::render - 
*
* FIXME P1: startTag() is not enough for the XML case of entityId - XML attributes ...
*           Perhaps add a vector of attributes to startTag() ?
*/
std::string EntityId::render(Format format, std::string indent, bool comma, bool isInVector, std::string assocTag)
{
  std::string out     = "";

  if (format == XML)
  {
    out += indent + "<" + tag + " type=\"" + type + "\" isPattern=\"" + isPattern + "\">\n";
    out += indent + "  " + "<id>"        + id        + "</id>"        + "\n";
    out += indent + "</" + tag + ">\n";
  }
  else
  {
    bool        isAssoc = !assocTag.empty();
    std::string indent2 = indent;

    if (isInVector)
       indent2 += "  ";

    out += (isInVector? indent + (isAssoc? "\"" + assocTag + "\" : ": "") + "{\n": "");
    out += indent2 + "\"type\" : \""      + type      + "\","  + "\n";
    out += indent2 + "\"isPattern\" : \"" + isPattern + "\","  + "\n";
    out += indent2 + "\"id\" : \""        + id        + "\"";

    if ((comma == true) && (isInVector == false))
       out += ",\n";
    else
    {
      out += "\n";
      out += (isInVector? indent + "}" : "");
      out += (comma == true)? ",\n" : (isInVector? "\n" : "");
    }
  }

  return out;
}



/* ****************************************************************************
*
* EntityId::check - 
*/
std::string EntityId::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  if (id == "")
    return "empty entityId:id";

  if (!isTrue(isPattern) && !isFalse(isPattern) && isPattern != "")
     return std::string("invalid isPattern (boolean) value for entity: '") + isPattern + "'";

  if ((requestType == RegisterContext) && (isTrue(isPattern)))
    return "'isPattern' set to true for a registration";

  return "OK";
}



/* ****************************************************************************
*
* EntityId::fill -
*/
void EntityId::fill(std::string _id, std::string _type, std::string _isPattern)
{
  id        = _id;
  type      = _type;
  isPattern = _isPattern;
}



/* ****************************************************************************
*
* EntityId::present - 
*/
void EntityId::present(std::string indent, int ix)
{
  if (ix == -1)
    PRINTF("%sEntity Id:\n",       indent.c_str());
  else
    PRINTF("%sEntity Id %d:\n",       indent.c_str(), ix);

  PRINTF("%s  Id:         %s\n", indent.c_str(), id.c_str());
  PRINTF("%s  Type:       %s\n", indent.c_str(), type.c_str());
  PRINTF("%s  isPattern:  %s\n", indent.c_str(), isPattern.c_str());
}



/* ****************************************************************************
*
* release  - 
*/
void EntityId::release(void)
{
   /* This method is included for the sake of homogeneity */
}



/* ****************************************************************************
*
* toString - 
*/
std::string EntityId::toString(bool useIsPattern, std::string delimiter)
{
  std::string s;

  s = id + delimiter + type;

  if (useIsPattern)
    s += delimiter + isPattern;

  return s;
}
