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

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"

#include "common/globals.h"
#include "common/string.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/ContextElementResponse.h"
#include "rest/OrionError.h"
#include "ngsi10/UpdateContextResponse.h"



/* ****************************************************************************
*
* UpdateContextResponse::UpdateContextResponse -
*/
UpdateContextResponse::UpdateContextResponse()
{
}



/* ****************************************************************************
*
* UpdateContextResponse::~UpdateContextResponse -
*/
UpdateContextResponse::~UpdateContextResponse()
{
  contextElementResponseVector.release();
  LM_T(LmtDestructor, ("destroyed"));
}



/* ****************************************************************************
*
* UpdateContextResponse::release -
*/
void UpdateContextResponse::release(void)
{
  LM_T(LmtRelease, ("In UpdateContextResponse::release"));
  contextElementResponseVector.release();
}



/* ****************************************************************************
*
* UpdateContextResponse::notFoundPush -
*
* 1. Find contextElementResponse in contextElementResponseVector and add the ContextAttribute.
* 2. If not found: create a new one.
*
*/
void UpdateContextResponse::notFoundPush(Entity* eP, ContextAttribute* aP, OrionError* oeP)
{
  ContextElementResponse* cerP = contextElementResponseVector.lookup(eP, SccContextElementNotFound);

  if (cerP == NULL)
  {
    // Build ContextElementResponse
    cerP = new ContextElementResponse();
    EntityId enId(eP->entityId.id, eP->entityId.idPattern, eP->entityId.type, eP->entityId.typePattern);
    cerP->entity.fill(enId);
    if (aP != NULL)
    {
      // We copy ContextAttribute given Entity destructor does release() on the vector
      cerP->entity.attributeVector.push_back(new ContextAttribute(aP));
    }

    if (oeP != NULL)
    {
      cerP->error.fill(oeP);
    }
    else
    {
      cerP->error.fill(SccContextElementNotFound, eP->entityId.id);
    }

    contextElementResponseVector.push_back(cerP);
  }
  else
  {
    cerP->entity.attributeVector.push_back(new ContextAttribute(aP));
  }
}



/* ****************************************************************************
*
* UpdateContextResponse::foundPush -
*
* 1. Find contextElementResponse in contextElementResponseVector and add the ContextAttribute.
* 2. If no contextElementResponse is found for this Entity (eP), then create a new
*    contextElementResponse and push the attribute onto it.
*
*/
void UpdateContextResponse::foundPush(Entity* eP, ContextAttribute* aP)
{
  ContextElementResponse* cerP = contextElementResponseVector.lookup(eP, SccOk);

  if (cerP == NULL)
  {
    // Build ContextElementResponse
    cerP = new ContextElementResponse();
    EntityId enId(eP->entityId.id, eP->entityId.idPattern, eP->entityId.type, eP->entityId.typePattern);
    cerP->entity.fill(enId);
    if (aP != NULL)
    {
      // We copy ContextAttribute given Entity destructor does release() on the vector
      cerP->entity.attributeVector.push_back(new ContextAttribute(aP));
    }

    cerP->error.fill(SccOk);
    contextElementResponseVector.push_back(cerP);
  }
  else
  {
    cerP->entity.attributeVector.push_back(new ContextAttribute(aP));
  }
}



/* ****************************************************************************
*
* UpdateContextResponse::fill -
*/
void UpdateContextResponse::fill(UpdateContextResponse* upcrsP)
{
  contextElementResponseVector.fill(upcrsP->contextElementResponseVector);
  error.fill(upcrsP->error);
}


/* ****************************************************************************
*
* UpdateContextResponse::fill -
*/
void UpdateContextResponse::fill(UpdateContextRequest* upcrP, HttpStatusCode sc)
{
  contextElementResponseVector.fill(upcrP->entityVector, sc);

  // Note that "external" OrionError is always SccOk, sc is not used here
  // FIXME PR: internal error should be avoided. Review this
  error.fill(sc, "");
}


/* ****************************************************************************
*
* UpdateContextResponse::merge -
*
* For each attribute in upcrsP::ContextElementResponse[cerIx]::ContextElement::ContextAttributeVector
*   - if found: use foundPush to add the attribute to its correct place
*   - if not found, use notFoundPush
*
*/
void UpdateContextResponse::merge(UpdateContextResponse* upcrsP)
{
  if (upcrsP->contextElementResponseVector.size() == 0)
  {
    // If no contextElementResponses, copy errorCode if empty
    if ((error.code == SccNone) || (error.code == SccOk))
    {
      error.fill(upcrsP->error);
    }
    else if (error.description.empty())
    {
      error.description = upcrsP->error.description;
    }
  }

  for (unsigned int cerIx = 0; cerIx < upcrsP->contextElementResponseVector.size(); ++cerIx)
  {
    Entity*      eP = &upcrsP->contextElementResponseVector[cerIx]->entity;
    OrionError*  oeP = &upcrsP->contextElementResponseVector[cerIx]->error;

    for (unsigned int aIx = 0; aIx < eP->attributeVector.size(); ++aIx)
    {
      ContextAttribute* aP = eP->attributeVector[aIx];

      if (oeP->code != SccOk)
      {
        notFoundPush(eP, aP, oeP);
      }
      else
      {
        foundPush(eP, aP);
      }
    }
  }
}
