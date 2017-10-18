#ifndef SRC_LIB_ORIONTYPES_ENTITYTYPERESPONSE_H_
#define SRC_LIB_ORIONTYPES_ENTITYTYPERESPONSE_H_

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
#include <vector>

#include "ngsi/Request.h"
#include "ngsi/StatusCode.h"
#include "orionTypes/EntityType.h"



/* ****************************************************************************
*
* EntityTypeResponse -
*/
class EntityTypeResponse
{
 public:
  EntityType    entityType;
  StatusCode    statusCode;

  std::string   render(ApiVersion          apiVersion,
                       bool                asJsonObject,
                       bool                asJsonOut,
                       bool                collapsed);
  std::string   toJson(void);
  std::string   check(ApiVersion          apiVersion,
                      bool                asJsonObject,
                      bool                asJsonOut,
                      bool                collapsed,
                      const std::string&  predetectedError);
  void          present(const std::string& indent);
  void          release(void);
};

#endif  // SRC_LIB_ORIONTYPES_ENTITYTYPERESPONSE_H_
