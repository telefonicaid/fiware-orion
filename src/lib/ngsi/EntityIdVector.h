#ifndef SRC_LIB_NGSI_ENTITYIDVECTOR_H_
#define SRC_LIB_NGSI_ENTITYIDVECTOR_H_

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

#include "ngsi/EntityId.h"
#include "apiTypesV2/EntityVector.h"



/* ****************************************************************************
*
* EntityIdVector - 
*
* NOTE
* The method 'lookup' looks up an *exact* match for an entity.
* I.e. the id must be exactly the same, the type exactly the same and isPattern, exactly the same.
* This is not a *match* method that would find 'matching' entityIds.
*
* If you need an entity-matching method, then a new method, called 'match' should be implemented.
*
*/
typedef struct EntityIdVector
{
  std::vector<EntityId*>  vec;

  std::string  toJsonV1(bool comma);
  void         push_back(EntityId* item);
  bool         push_back_if_absent(EntityId* item);
  unsigned int size(void) const;
  EntityId*    lookup(const std::string& name, const std::string& type, const std::string& isPattern);
  void         release();
  void         fill(EntityVector& _vec);

  EntityId* operator[](unsigned int ix) const;

  std::string  check(RequestType requestType);
} EntityIdVector;

#endif  // SRC_LIB_NGSI_ENTITYIDVECTOR_H_
