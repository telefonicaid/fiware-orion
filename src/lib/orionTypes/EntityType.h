#ifndef SRC_LIB_ORIONTYPES_ENTITYTYPE_H_
#define SRC_LIB_ORIONTYPES_ENTITYTYPE_H_

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
#include <string>

#include "ngsi/ContextAttributeVector.h"



/* ****************************************************************************
*
* EntityType -
*/
class EntityType
{
 public:
  std::string              type;
  ContextAttributeVector   contextAttributeVector;
  long long                count;

  EntityType();
  explicit EntityType(std::string _type);

  std::string   check(ApiVersion apiVersion, const std::string& predetectedError);
  std::string   toJsonV1(bool  asJsonObject,
                         bool  asJsonOut,
                         bool  collapsed,
                         bool  comma = false,
                         bool  typeNameBefore = false);
  void          release(void);
  std::string   toJson(bool includeType = false);
};

#endif  // SRC_LIB_ORIONTYPES_ENTITYTYPE_H_
