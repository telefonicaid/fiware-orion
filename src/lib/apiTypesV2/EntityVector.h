#ifndef SRC_LIB_APITYPESV2_ENTITYVECTOR_H_
#define SRC_LIB_APITYPESV2_ENTITYVECTOR_H_

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
#include <map>

#include "apiTypesV2/Entity.h"



/* ****************************************************************************
*
* EntityVector -
*/
typedef struct EntityVector
{
  std::vector<Entity*>  vec;

  std::string   render(std::map<std::string, bool>&         uriParamOptions,
                       std::map<std::string, std::string>&  uriParam);

  std::string   check(RequestType requestType);
  void          present(const std::string& indent);
  void          push_back(Entity* item);
  unsigned int  size(void);
  Entity*       lookup(const std::string& name, const std::string& type, const std::string& isPattern);
  void          release();

  Entity*  operator[](unsigned int ix) const;
} EntityVector;

#endif  // SRC_LIB_APITYPESV2_ENTITYVECTOR_H_
