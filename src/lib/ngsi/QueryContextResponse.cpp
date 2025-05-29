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

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"

#include "common/string.h"
#include "common/errorMessages.h"
#include "alarmMgr/alarmMgr.h"
#include "rest/HttpStatusCode.h"
#include "rest/OrionError.h"
#include "ngsi/QueryContextResponse.h"



/* ****************************************************************************
*
* QueryContextResponse::QueryContextResponse -
*/
QueryContextResponse::QueryContextResponse()
{
}



/* ****************************************************************************
*
* QueryContextResponse::~QueryContextResponse -
*/
QueryContextResponse::~QueryContextResponse()
{
  contextElementResponseVector.release();
}



/* ****************************************************************************
*
* QueryContextResponse::release -
*/
void QueryContextResponse::release(void)
{
  contextElementResponseVector.release();
}



/* ****************************************************************************
*
* QueryContextResponse::fill -
*/
void QueryContextResponse::fill(const EntityVector& entities)
{
  for (unsigned int eIx = 0; eIx < entities.vec.size(); eIx++)
  {
    ContextElementResponse* cerP = new ContextElementResponse(entities.vec[eIx]);
    contextElementResponseVector.push_back(cerP);
  }
}


/* ****************************************************************************
*
* QueryContextResponse::getAttr -
*
* If attribute is found:
* - It is returned by the function
* - The OrionError is set to SccNone
*
* If attribute is not found
* - Function returns NULL
* - The OrionError is not touched
*
*/
ContextAttribute* QueryContextResponse::getAttr(const std::string& attrName, OrionError* oeP)
{
  if (error.code == SccContextElementNotFound)
  {
    oeP->fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY, ERROR_NOT_FOUND);
    return NULL;
  }

  if (error.code != SccOk)
  {
    //
    // any other error distinct from Not Found
    //
    oeP->fill(error.code, error.description, error.error);
    return NULL;
  }

  if (contextElementResponseVector.size() > 1)  // error.code == SccOk
  {
    //
    // If there are more than one entity, we return an error
    //
    oeP->fill(SccConflict, ERROR_DESC_TOO_MANY_ENTITIES, ERROR_TOO_MANY);
    return NULL;
  }

  // Look for the attribute by name
  ContextElementResponse* cerP = contextElementResponseVector[0];

  for (std::size_t i = 0; i < cerP->entity.attributeVector.size(); ++i)
  {
    if (cerP->entity.attributeVector[i]->name == attrName)
    {
      return cerP->entity.attributeVector[i];
    }
  }

  oeP->fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ATTRIBUTE, ERROR_NOT_FOUND);
  return NULL;
}
