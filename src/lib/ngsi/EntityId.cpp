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
#include "ngsi/EntityId.h"



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
* EntityId::toJsonV1 -
*
*/
void EntityId::toJsonV1
(
  JsonHelper& writer
)
{
  writer.String("type", type);
  writer.String("isPattern", isPattern);
  writer.String("id", id);
}



/* ****************************************************************************
*
* EntityId::toJson - 
*/
void EntityId::toJson
(
  JsonHelper& writer
) const
{
  writer.String("id", id);
  writer.String("type", type);
}



/* ****************************************************************************
*
* EntityId::check -
*/
std::string EntityId::check
(
  RequestType         requestType,
  const std::string&  indent
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
    return "isPattern set to true for registrations is currently not supported";
  }

  if (isTrue(isPattern))
  {
    regex_t re;
    if (regcomp(&re, id.c_str(), REG_EXTENDED) != 0)
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

  if (useDefaultType && (type == ""))
  {
    type = DEFAULT_ENTITY_TYPE;
  }
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
  LM_T(LmtPresent, ("%s  isTypePttern:  '%s'",
            indent.c_str(),
            isTypePattern? "true" : "false"));
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
