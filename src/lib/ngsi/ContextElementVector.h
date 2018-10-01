#ifndef SRC_LIB_NGSI_CONTEXTELEMENTVECTOR_H_
#define SRC_LIB_NGSI_CONTEXTELEMENTVECTOR_H_

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
#include <vector>

#include "apiTypesV2/Entity.h"


/* ****************************************************************************
*
* ContextElementVector -
*/
typedef struct ContextElementVector
{
  // FIXME #1298: the name of this class is no longer meaninfull, as the class now contains
  // a vector of Entity*, not ContextElement*. Rename (maybe a good name would be EntityVector)
  // In general we have some other "ContextElementSomething". Do a global search in the code
  std::vector<Entity*>  vec;

  void             push_back(Entity* item);
  unsigned int     size(void);
  std::string      toJsonV1(bool asJsonObject, RequestType requestType,bool comma);
  void             release(void);
  Entity*          lookup(Entity* eP);
  Entity*          operator[](unsigned int ix) const;

  std::string      check(ApiVersion apiVersion, RequestType requestType);
} ContextElementVector;

#endif  // SRC_LIB_NGSI_CONTEXTELEMENTVECTOR_H_
