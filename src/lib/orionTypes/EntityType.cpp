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
* EntityType::toJsonV1 -
*
* This method is used by:
*   o EntityTypeVector
*   o EntityTypeResponse
*
* 'typeNameBefore' is set to TRUE when called from EntityTypeResponse
*/
void EntityType::toJsonV1
(
  JsonHelper& writer,
  ApiVersion  apiVersion,
  bool        asJsonObject,
  bool        asJsonOut,
  bool        collapsed,
  bool        typeNameBefore
)
{
  if (typeNameBefore && asJsonOut)
  {
    writer.String("name", type);
    contextAttributeVector.toJsonV1(writer, asJsonObject, EntityTypes, true, true);
  }
  else
  {
    writer.StartObject();

    writer.String("name", type);
    if (!collapsed && contextAttributeVector.size() != 0)
    {
      contextAttributeVector.toJsonV1(writer, asJsonObject, EntityTypes, true, true);
    }

    writer.EndObject();
  }
}



/* ****************************************************************************
*
* EntityType::check -
*/
std::string EntityType::check(ApiVersion apiVersion, const std::string&  predetectedError)
{
  if (predetectedError != "")
  {
    return predetectedError;
  }
  else if (type == "")
  {
    return "Empty Type";
  }

  return contextAttributeVector.check(apiVersion, EntityTypes);
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
void EntityType::toJson
(
  JsonHelper& writer,
  bool includeType
)
{
  writer.StartObject();

  if (includeType)
  {
    writer.String("type", type);
  }

  writer.StartObject("attrs");
  contextAttributeVector.toJsonTypes(writer);
  writer.EndObject();

  writer.Int("count", count);

  writer.EndObject();
}
