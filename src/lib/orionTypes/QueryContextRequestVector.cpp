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
#include <stdio.h>
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "orionTypes/QueryContextRequestVector.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* QueryContextRequestVector::QueryContextRequestVector -
*/
QueryContextRequestVector::QueryContextRequestVector()
{
}



/* ****************************************************************************
*
* QueryContextRequestVector::size -
*/
unsigned int QueryContextRequestVector::size(void)
{
  return vec.size();
}


/* ****************************************************************************
*
* QueryContextRequestVector::operator[] -
*/
QueryContextRequest*  QueryContextRequestVector::operator[](unsigned int ix) const
{
   if (ix < vec.size())
   {
      return vec[ix];
   }
   return NULL;
}


/* ****************************************************************************
*
* QueryContextRequestVector::push_back -
*/
void QueryContextRequestVector::push_back(QueryContextRequest* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* entityIdMatch -
*/
static bool entityIdMatch(EntityId* e1, EntityId* e2)
{
  if (e1->id != e2->id)
  {
    return false;
  }

  if ((!e2->type.empty()) && (e1->type != e2->type))
  {
    return false;
  }

  return true;
}



/* ****************************************************************************
*
* QueryContextRequestVector::lookup -
*/
QueryContextRequest* QueryContextRequestVector::lookup(const std::string& contextProvider, EntityId* eP)
{
  for (unsigned int ix = 0; ix < size(); ++ix)
  {
    if ((vec[ix]->contextProvider == contextProvider) && (entityIdMatch(vec[ix]->entityIdVector[0], eP) == true))
    {
      return vec[ix];
    }
  }

  return NULL;
}


/* ****************************************************************************
*
* QueryContextRequestVector::release -
*/
void QueryContextRequestVector::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->release();
    delete(vec[ix]);
  }

  vec.clear();
}
