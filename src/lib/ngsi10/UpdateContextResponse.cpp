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
#include "common/tag.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/ContextElementResponse.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/UpdateContextResponse.h"



/* ****************************************************************************
*
* UpdateContextResponse::UpdateContextResponse -
*/
UpdateContextResponse::UpdateContextResponse()
{
  errorCode.keyNameSet("errorCode");
}



/* ****************************************************************************
*
* UpdateContextResponse::UpdateContextResponse -
*/
UpdateContextResponse::UpdateContextResponse(StatusCode& _errorCode)
{
  errorCode.fill(&_errorCode);
  errorCode.keyNameSet("errorCode");
  LM_T(LmtDestructor, ("destroyed"));
}



/* ****************************************************************************
*
* UpdateContextResponse::~UpdateContextResponse -
*/
UpdateContextResponse::~UpdateContextResponse()
{
  errorCode.release();
  contextElementResponseVector.release();
  LM_T(LmtDestructor, ("destroyed"));
}



/* ****************************************************************************
*
* UpdateContextResponse::render -
*/
std::string UpdateContextResponse::render(bool asJsonObject)
{
  std::string out = "";

  out += startTag();

  if ((errorCode.code != SccNone) && (errorCode.code != SccOk))
  {
    out += errorCode.render(false);
  }
  else
  {
    if (contextElementResponseVector.size() == 0)
    {
      errorCode.fill(SccContextElementNotFound, errorCode.details);
      out += errorCode.render(false);
    }
    else
    {      
      // No attribute or metadata filter in this case, an empty vector is used to fulfil method signature
      std::vector<std::string> emptyV;

      out += contextElementResponseVector.render(asJsonObject, RtUpdateContextResponse, emptyV, false, emptyV, false);
    }
  }
  
  out += endTag();

  return out;
}



/* ****************************************************************************
*
* UpdateContextResponse::check -
*/
std::string UpdateContextResponse::check
(
  ApiVersion          apiVersion,
  bool                asJsonObject,
  const std::string&  predetectedError
)
{
  std::string  res;

  if (predetectedError != "")
  {
    errorCode.fill(SccBadRequest, predetectedError);
  }  
  else if (contextElementResponseVector.check(apiVersion, UpdateContext, predetectedError, 0) != "OK")
  {
    alarmMgr.badInput(clientIp, res);
    errorCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return render(asJsonObject);
}



/* ****************************************************************************
*
* UpdateContextResponse::release -
*/
void UpdateContextResponse::release(void)
{
  LM_T(LmtRelease, ("In UpdateContextResponse::release"));
  contextElementResponseVector.release();
  errorCode.release();
}



/* ****************************************************************************
*
* UpdateContextResponse::notFoundPush -
*
* 1. Find contextElementResponse in contextElementResponseVector and add the ContextAttribute.
* 2. If not found: create a new one.
*
*/
void UpdateContextResponse::notFoundPush(Entity* eP, ContextAttribute* aP, StatusCode* scP)
{
  ContextElementResponse* cerP = contextElementResponseVector.lookup(eP, SccContextElementNotFound);

  if (cerP == NULL)
  {
    // Build ContextElementResponse
    cerP = new ContextElementResponse();
    cerP->entity.fill(eP->id, eP->type, eP->isPattern);
    if (aP != NULL)
    {
      // We copy ContextAttribute given Entity destructor does release() on the vector
      cerP->entity.attributeVector.push_back(new ContextAttribute(aP));
    }

    if (scP != NULL)
    {
      cerP->statusCode.fill(scP);
    }
    else
    {
      cerP->statusCode.fill(SccContextElementNotFound, eP->id);
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
    cerP->entity.fill(eP->id, eP->type, eP->isPattern);
    if (aP != NULL)
    {
      // We copy ContextAttribute given Entity destructor does release() on the vector
      cerP->entity.attributeVector.push_back(new ContextAttribute(aP));
    }

    cerP->statusCode.fill(SccOk);
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
  errorCode.fill(upcrsP->errorCode);
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
    if ((errorCode.code == SccNone) || (errorCode.code == SccOk))
    {
      errorCode.fill(upcrsP->errorCode);
    }
    else if (errorCode.details == "")
    {
      errorCode.details = upcrsP->errorCode.details;
    }
  }

  for (unsigned int cerIx = 0; cerIx < upcrsP->contextElementResponseVector.size(); ++cerIx)
  {
    Entity*      eP = &upcrsP->contextElementResponseVector[cerIx]->entity;
    StatusCode*  scP = &upcrsP->contextElementResponseVector[cerIx]->statusCode;

    for (unsigned int aIx = 0; aIx < eP->attributeVector.size(); ++aIx)
    {
      ContextAttribute* aP = eP->attributeVector[aIx];

      if (scP->code != SccOk)
      {
        notFoundPush(eP, aP, scP);
      }
      else
      {
        foundPush(eP, aP);
      }
    }
  }
}
