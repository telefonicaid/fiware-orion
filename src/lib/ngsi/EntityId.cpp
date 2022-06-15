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
#include <regex.h>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/string.h"
#include "common/JsonHelper.h"
#include "ngsi/EntityId.h"
#include "common/tag.h"
#include "common/JsonHelper.h"



/* ****************************************************************************
*
* EntityId::EntityId -
*/
EntityId::EntityId(): creDate(0), modDate(0)
{
  isTypePattern = false;
}



/* ****************************************************************************
*
* EntityId::EntityId -
*/
EntityId::EntityId(EntityId* eP)
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
  bool                _isTypePattern
) : id(_id),
    type(_type),
    isPattern(_isPattern),
    isTypePattern(_isTypePattern),
    creDate(0),
    modDate(0)
{
}



/* ****************************************************************************
*
* EntityId::toJson -
*
*/
std::string EntityId::toJson(void)
{
  JsonObjectHelper jh;

  if (isTrue(isPattern))
  {
    jh.addString("idPattern", id);
  }
  else
  {
    jh.addString("id", id);
  }

  if (!type.empty())
  {
    jh.addString("type", type);
  }

  return jh.str();
}



/* ****************************************************************************
*
* EntityId::toJsonV1 -
*
*/
std::string EntityId::toJsonV1(bool comma, bool isInVector)
{
  std::string  out              = "";
  char*        isPatternEscaped = htmlEscape(isPattern.c_str());
  char*        typeEscaped      = htmlEscape(type.c_str());
  char*        idEscaped        = htmlEscape(id.c_str());

  out += (isInVector? "{" : "");
  out = out + "\"type\":\""      + typeEscaped      + "\",";
  out = out + "\"isPattern\":\"" + isPatternEscaped + "\",";
  out = out + "\"id\":\""        + idEscaped        + "\"";

  if ((comma == true) && (isInVector == false))
  {
    out += ",";
  }
  else
  {
    out += (isInVector? "}" : "");
    out += (comma == true)? "," : "";
  }

  free(typeEscaped);
  free(idEscaped);
  free(isPatternEscaped);

  return out;
}



/* ****************************************************************************
*
* EntityId::toJson - 
*/
std::string EntityId::toJson(void) const
{
  JsonObjectHelper jh;

  char*  typeEscaped  = htmlEscape(type.c_str());
  char*  idEscaped    = htmlEscape(id.c_str());

  jh.addString("id", idEscaped);
  jh.addString("type", typeEscaped);

  free(typeEscaped);
  free(idEscaped);

  return jh.str();
}



/* ****************************************************************************
*
* EntityId::check -
*/
std::string EntityId::check(RequestType requestType)
{
  if (id.empty())
  {
    return "empty entityId:id";
  }

  if (!isTrue(isPattern) && !isFalse(isPattern) && !isPattern.empty())
  {
    return std::string("invalid isPattern value for entity: /") + isPattern + "/";
  }

  if ((requestType == RegisterContext) && (isTrue(isPattern)))
  {
    return "isPattern set to true for registrations is currently not supported";
  }

  if (isTrue(isPattern))
  {
    regex_t re;
    if ((id.find('\0') != std::string::npos) || (!regComp(&re, id.c_str(), REG_EXTENDED)))
    {
      return "invalid regex for entity id pattern";
    }
    regfree(&re);  // If regcomp fails it frees up itself (see glibc sources for details)
  }
  return "OK";
}



/* ****************************************************************************
*
* EntityId::fill -
*/
void EntityId::fill(const std::string& _id, const std::string& _type, const std::string& _isPattern, bool _isTypePattern)
{
  id            = _id;
  type          = _type;
  isPattern     = _isPattern;
  isTypePattern = _isTypePattern;
}



/* ****************************************************************************
*
* EntityId::fill -
*/
void EntityId::fill(const struct EntityId* eidP, bool useDefaultType)
{
  id            = eidP->id;
  type          = eidP->type;
  isPattern     = eidP->isPattern;
  isTypePattern = eidP->isTypePattern;
  servicePath   = eidP->servicePath;
  creDate       = eidP->creDate;
  modDate       = eidP->modDate;

  if (useDefaultType && (type.empty()))
  {
    type = DEFAULT_ENTITY_TYPE;
  }
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
  return ((eP->id                == id)                &&
          (eP->type              == type)              &&
          (eP->isPatternIsTrue() == isPatternIsTrue()) &&
          (eP->isTypePattern     == isTypePattern));
}



/* ****************************************************************************
*
* isPatternIsTrue - 
*/
bool EntityId::isPatternIsTrue(void)
{
  return isTrue(isPattern);
}
