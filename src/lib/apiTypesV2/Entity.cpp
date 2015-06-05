/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <vector>

#include "apiTypesV2/Entity.h"



/* ****************************************************************************
*
* Entity::Entity - 
*/
Entity::Entity()
{
}



/* ****************************************************************************
*
* Entity::~Entity - 
*/
Entity::~Entity()
{
  release();
}


/* ****************************************************************************
*
* Entity::render - 
*/
std::string Entity::render(ConnectionInfo* ciP, RequestType requestType, bool comma)
{
  std::string out = "{\"id\":\"" + id + "\"";

  if (type != "")
  {
    out += ",\"type\":\"" + type + "\"";
  }

  if (attributeVector.size() != 0)
  {
    out += ",";
    out += attributeVector.renderV2(ciP, requestType);
  }    

  out += "}";

  if (comma)
  {
    out += ",";
  }

  return out;
}



/* ****************************************************************************
*
* Entity::check - 
*/
std::string Entity::check(ConnectionInfo* ciP, RequestType requestType)
{
  return "OK";
}



/* ****************************************************************************
*
* Entity::present - 
*/
void Entity::present(const std::string& indent, const std::string& caller)
{
  LM_F(("%sid:        %s", indent.c_str(), id.c_str()));
  LM_F(("%stype:      %s", indent.c_str(), type.c_str()));
  LM_F(("%sisPattern: %s", indent.c_str(), isPattern.c_str()));

  attributeVector.present(indent + "  ");
}



/* ****************************************************************************
*
* Entity::fill - 
*/
void Entity::fill(const std::string& _id, const std::string& _type, const std::string& _isPattern, ContextAttributeVector* aVec)
{
  id         = _id;
  type       = _type;
  isPattern  = _isPattern;

  attributeVector.fill(aVec);
}



/* ****************************************************************************
*
* Entity::release - 
*/
void Entity::release(void)
{
  attributeVector.release();
}
