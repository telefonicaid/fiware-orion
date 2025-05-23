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
#include <map>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/JsonHelper.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/Request.h"
#include "ngsi10/QueryContextResponse.h"

#include "apiTypesV2/EntityVector.h"



/* ****************************************************************************
*
* EntityVector::toJson -
*/
std::string EntityVector::toJson
(
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  const std::vector<std::string>&  metadataFilter
)
{
  JsonVectorHelper jh;

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    // This is to avoid spurious entities like '{"id": "E", "type": "T"}'
    // typically generated when CPrs are in use. These entities have
    // creation date equal to 0 and no attributes
    if ((vec[ix]->creDate == 0) && (vec[ix]->attributeVector.size() == 0))
    {
      continue;
    }
    jh.addRaw(vec[ix]->toJson(renderFormat, attrsFilter, blacklist, metadataFilter));
  }

  return jh.str();
}


/* ****************************************************************************
*
* EntityVector::toJson -
*/
std::string EntityVector::toJson(RenderFormat renderFormat)
{
  JsonVectorHelper jh;

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    // This is to avoid spurious entities like '{"id": "E", "type": "T"}'
    // typically generated when CPrs are in use. These entities have
    // creation date equal to 0 and no attributes
    if ((vec[ix]->creDate == 0) && (vec[ix]->attributeVector.size() == 0))
    {
      continue;
    }
    jh.addRaw(vec[ix]->toJson(renderFormat));
  }

  return jh.str();
}



/* ****************************************************************************
*
* EntityVector::push_back -
*/
void EntityVector::push_back(Entity* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* EntityVector::operator[] -
*/
Entity*  EntityVector::operator[] (unsigned int ix) const
{
  if (ix < vec.size())
  {
    return vec[ix];
  }

  return NULL;
}



/* ****************************************************************************
*
* EntityVector::size -
*/
unsigned int EntityVector::size(void)
{
  return vec.size();
}



/* ****************************************************************************
*
* EntityVector::lookup -
*/
Entity* EntityVector::lookup(const std::string& name, const std::string& type)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if ((vec[ix]->entityId.id == name) && (vec[ix]->entityId.type == type))
    {
      return vec[ix];
    }
  }

  return NULL;
}



/* ****************************************************************************
*
* EntityVector::fill -
*/
void EntityVector::fill(const QueryContextResponse& qcrs, OrionError* oeP)
{
  if (qcrs.error.code == SccContextElementNotFound)
  {
    //
    // If no entities are found, we respond with a 200 OK
    // and an empty vector of entities ( [] )
    //

    oeP->fill(SccOk, "", "OK");
    return;
  }
  else if (qcrs.error.code != SccOk)
  {
    //
    // If any other error - use the error for the response
    //

    oeP->fill(qcrs.error.code, qcrs.error.description, qcrs.error.error);
    return;
  }

  for (unsigned int ix = 0; ix < qcrs.contextElementResponseVector.size(); ++ix)
  {
    Entity* eP = &qcrs.contextElementResponseVector[ix]->entity;

    if ((&qcrs.contextElementResponseVector[ix]->error)->code == SccReceiverInternalError)
    {
      // FIXME P4: Do we need to release the memory allocated in 'vec' before returning? I don't
      // think so, as the releasing logic in the upper layer will deal with that but
      // let's do anyway just in case... (we don't have a ft covering this, so valgrind suite
      // cannot help here and it is better to ensure)
      oeP->fill(&qcrs.contextElementResponseVector[ix]->error);
      release();
      return;
    }
    else
    {
      Entity*         newP  = new Entity();

      newP->entityId = eP->entityId;
      newP->creDate  = eP->creDate;
      newP->modDate  = eP->modDate;

      newP->attributeVector.fill(eP->attributeVector);
      push_back(newP);
    }
  }
}



/* ****************************************************************************
*
* EntityVector::release -
*/
void EntityVector::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->release();
    delete(vec[ix]);
  }

  vec.clear();
}
