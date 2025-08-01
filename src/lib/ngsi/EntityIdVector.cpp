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
#include <stdio.h>
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/JsonHelper.h"
#include "alarmMgr/alarmMgr.h"
#include "apiTypesV2/EntityVector.h"

#include "ngsi/EntityIdVector.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* EntityIdVector::toJson -
*/
std::string EntityIdVector::toJson(void)
{
  JsonVectorHelper jh;

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    jh.addRaw(vec[ix]->toJson());
  }

  return jh.str();
}



/* ****************************************************************************
*
* EntityIdVector::lookup - find a matching entity in the entity-vector
*/
EntityId* EntityIdVector::lookup(const std::string& id, const std::string& idPattern, const std::string& type, const std::string& typePattern)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if ((vec[ix]->id == id) && (vec[ix]->type == type) && (vec[ix]->idPattern == idPattern) && (vec[ix]->typePattern == typePattern))
    {
      return vec[ix];
    }
  }

  return NULL;
}



/* ****************************************************************************
*
* EntityIdVector::push_back -
*/
void EntityIdVector::push_back(EntityId* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* EntityIdVector::push_back_if_absent -
*
* RETURN VALUE
*   - true:  on successful push_back
*   - false: if no push_back was made
*/
bool EntityIdVector::push_back_if_absent(EntityId* item)
{
  if (lookup(item->id, item->idPattern, item->type, item->typePattern) == NULL)
  {
    vec.push_back(item);
    return true;
  }

  return false;
}


/* ****************************************************************************
*
* EntityIdVector::operator[] -
*/
EntityId* EntityIdVector::operator[] (unsigned int ix) const
{
   if (ix < vec.size())
   {
     return vec[ix];
   }
   return NULL;
}



/* ****************************************************************************
*
* EntityIdVector::size -
*/
unsigned int EntityIdVector::size(void) const
{
  return vec.size();
}



/* ****************************************************************************
*
* EntityIdVector::release -
*/
void EntityIdVector::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    delete(vec[ix]);
  }

  vec.clear();
}



/* ****************************************************************************
*
* EntityIdVector::fill(EntityIdVector) -
*
*/
void EntityIdVector::fill(EntityVector& _vec)
{
  for (unsigned int ix = 0; ix < _vec.size(); ++ix)
  {
    Entity*   entityP   = _vec[ix];
    EntityId* entityIdP = new EntityId(entityP->entityId.id, entityP->entityId.idPattern, entityP->entityId.type, entityP->entityId.typePattern);

    vec.push_back(entityIdP);
  }
}
