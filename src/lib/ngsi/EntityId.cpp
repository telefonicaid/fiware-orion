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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/Format.h"
#include "ngsi/EntityId.h"
#include "common/tag.h"



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
EntityId::EntityId(EntityId* eP) : tag("entityId")
{
  fill(eP);
}


/* ****************************************************************************
*
* EntityId::EntityId -
*/
EntityId::EntityId
(
  const std::string&  _id,
  const std::string&  _type,
  const std::string&  _isPattern,
  const std::string&  _tag
) : id(_id),
    type(_type),
    isPattern(_isPattern),
    tag(_tag)
{
}



/* ****************************************************************************
*
* tagSet -
*/
void EntityId::tagSet(const std::string& tagName)
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
std::string EntityId::render
(
  Format              format,
  const std::string&  indent,
  bool                comma,
  bool                isInVector
)
{
  std::string  out              = "";
  char*        isPatternEscaped = htmlEscape(isPattern.c_str());
  char*        typeEscaped      = htmlEscape(type.c_str());
  char*        idEscaped        = htmlEscape(id.c_str());

  if (format == XML)
  {
    out += indent + "<"  + tag + " type=\"" + typeEscaped + "\" isPattern=\"" + isPatternEscaped + "\">\n";
    out += indent + "  " + "<id>"           + idEscaped   + "</id>"           + "\n";
    out += indent + "</" + tag + ">\n";
  }
  else
  {    
    std::string indent2 = indent;

    if (isInVector)
    {
       indent2 += "  ";
    }

    out += (isInVector? indent + "{\n" : "");
    out += indent2 + "\"type\" : \""      + typeEscaped      + "\","  + "\n";
    out += indent2 + "\"isPattern\" : \"" + isPatternEscaped + "\","  + "\n";
    out += indent2 + "\"id\" : \""        + idEscaped        + "\"";

    if ((comma == true) && (isInVector == false))
    {
       out += ",\n";
    }
    else
    {
      out += "\n";
      out += (isInVector? indent + "}" : "");
      out += (comma == true)? ",\n" : (isInVector? "\n" : "");
    }
  }

  free(typeEscaped);
  free(idEscaped);
  free(isPatternEscaped);

  return out;
}



/* ****************************************************************************
*
* EntityId::check -
*/
std::string EntityId::check
(
  ConnectionInfo* ciP,
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  if (id == "")
  {
    return "empty entityId:id";
  }

  if (!isTrue(isPattern) && !isFalse(isPattern) && isPattern != "")
  {
    return std::string("invalid isPattern value for entity: /") + isPattern + "/";
  }

  if ((requestType == RegisterContext) && (isTrue(isPattern)))
  {
    return "isPattern set to true for a registration";
  }

  return "OK";
}



/* ****************************************************************************
*
* EntityId::fill -
*/
void EntityId::fill(const std::string& _id, const std::string& _type, const std::string& _isPattern)
{
  id        = _id;
  type      = _type;
  isPattern = _isPattern;
}



/* ****************************************************************************
*
* EntityId::fill -
*/
void EntityId::fill(const struct EntityId* eidP)
{
  id          = eidP->id;
  type        = eidP->type;
  isPattern   = eidP->isPattern;
  servicePath = eidP->servicePath;
}



/* ****************************************************************************
*
* EntityId::present - 
*/
void EntityId::present(const std::string& indent, int ix)
{
  if (ix == -1)
  {
    LM_T(LmtPresent, ("%sEntity Id:",       indent.c_str()));
  }
  else
  {
    LM_T(LmtPresent, ("%sEntity Id %d:",       
		      indent.c_str(), 
		      ix));
  }

  LM_T(LmtPresent, ("%s  Id:         '%s'", 
		    indent.c_str(), 
		    id.c_str()));
  LM_T(LmtPresent, ("%s  Type:       '%s'", 
		    indent.c_str(), 
		    type.c_str()));
  LM_T(LmtPresent, ("%s  isPattern:  '%s'", 
		    indent.c_str(), 
		    isPattern.c_str()));
}



/* ****************************************************************************
*
* release -
*/
void EntityId::release(void)
{
  /* This method is included for the sake of homogeneity */
}



/* ****************************************************************************
*
* toString -
*/
std::string EntityId::toString(bool useIsPattern, const std::string& delimiter)
{
  std::string s;

  s = id + delimiter + type;

  if (useIsPattern)
  {
    s += delimiter + isPattern;
  }

  return s;
}



/* ****************************************************************************
*
* EntityId::equal - return TRUE if EXACT match
*/
bool EntityId::equal(EntityId* eP)
{
  if (eP->id != id)
  {
    return false;
  }

  if (eP->type != type)
  {
    return false;
  }

  if (eP->isPatternIsTrue() == isPatternIsTrue())
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* isPatternIsTrue - 
*/
bool EntityId::isPatternIsTrue(void)
{
  return isTrue(isPattern);
}
