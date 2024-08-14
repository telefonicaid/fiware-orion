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
* isPatternIsTrue - 
*/
bool EntityId::isPatternIsTrue(void)
{
  return isTrue(isPattern);
}
