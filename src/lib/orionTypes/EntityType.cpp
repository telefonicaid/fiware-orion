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

#include "common/limits.h"
#include "common/JsonHelper.h"
#include "ngsi/Request.h"
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
* EntityType::check -
*/
std::string EntityType::check(const std::string&  predetectedError)
{
  if (!predetectedError.empty())
  {
    return predetectedError;
  }
  else if (type.empty())
  {
    return "Empty Type";
  }

  return contextAttributeVector.check(EntityTypes);
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
std::string EntityType::toJson(bool includeType)
{
  JsonObjectHelper jh;

  if (includeType)
  {
    jh.addString("type", type);
  }

  jh.addRaw("attrs", contextAttributeVector.toJsonTypes());
  jh.addNumber("count", count);

  return jh.str();
}
