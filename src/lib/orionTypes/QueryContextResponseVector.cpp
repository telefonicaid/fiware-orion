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
#include "orionTypes/QueryContextResponseVector.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* QueryContextResponseVector::size -
*/
unsigned int QueryContextResponseVector::size(void)
{
  return vec.size();
}

/* ****************************************************************************
*
* QueryContextResponseVector::operator[] -
*/
QueryContextResponse*  QueryContextResponseVector::operator[](unsigned int ix) const
{
   if (ix < vec.size())
   {
      return vec[ix];
   }
   return NULL;
}

/* ****************************************************************************
*
* QueryContextResponseVector::push_back -
*/
void QueryContextResponseVector::push_back(QueryContextResponse* item)
{
  vec.push_back(item);
}

/* ****************************************************************************
*
* QueryContextResponseVector::release -
*/
void QueryContextResponseVector::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->release();
    delete(vec[ix]);
  }

  vec.clear();
}



/* ****************************************************************************
*
* QueryContextResponseVector::render -
*/
std::string QueryContextResponseVector::render(ApiVersion apiVersion, bool asJsonObject, bool details, const std::string& detailsString)
{
  QueryContextResponse* responseP = new QueryContextResponse();
  std::string           answer;

  //
  // Here we have a vector of QueryContextResponse.
  // What we need is ONE QueryContextResponse, so, we'll take all the
  // contextElementResponses from each of the QueryContextResponses in the vector and
  // move them to ONE QueryContextResponse (responseP)
  //
  // [ This might give me some problems with freeing the memory afterwards ...]
  //


  if (vec.size() == 0)
  {
    //
    // Special case: vector is empty: translate to 404
    //
    if ((responseP->errorCode.code == SccOk) || (responseP->errorCode.code == SccNone))
    {
      responseP->errorCode.fill(SccContextElementNotFound);
    }
  }
  else if ((vec.size() == 1) && (vec[0]->contextElementResponseVector.size() == 0))
  {
    //
    // Special case: only one QueryContextResponse in vec, and it has 0 contextElementResponses
    // This is clearly a Not Found ...
    //
    if ((responseP->errorCode.code == SccOk) || (responseP->errorCode.code == SccNone))
    {
      responseP->errorCode.fill(SccContextElementNotFound);
    }

    if ((vec[0]->errorCode.code == SccOk) ||(vec[0]->errorCode.code == SccNone))
    {
      vec[0]->errorCode.fill(SccContextElementNotFound);
    }

    //
    // Also, if same errorCode.code but no details ...
    //
    if ((responseP->errorCode.code == vec[0]->errorCode.code) && (responseP->errorCode.details == ""))
    {
      responseP->errorCode.details = vec[0]->errorCode.details;
    }
  }
  else
  {
    //
    // We have found something, so, all good
    //
    responseP->errorCode.fill(SccOk);
  }

  if (details)
  {
    responseP->errorCode.fill(SccOk, detailsString);
  }

  for (unsigned int qIx = 0; qIx < vec.size(); ++qIx)
  {
    //
    // If the response vector is empty and the errorCode also, then a 404 Not Found
    // is inserted.
    //
    if (vec[qIx]->contextElementResponseVector.size() == 0)
    {
      if ((vec[qIx]->errorCode.code == SccOk) || (vec[qIx]->errorCode.code == SccNone))
      {
        ContextElementResponse* cerP = new ContextElementResponse();

        cerP->statusCode.fill(SccContextElementNotFound);
        responseP->contextElementResponseVector.push_back(cerP);
      }
    }

    for (unsigned int cerIx = 0; cerIx < vec[qIx]->contextElementResponseVector.size(); ++cerIx)
    {
      ContextElementResponse* cerP = vec[qIx]->contextElementResponseVector[cerIx];

      if ((cerP->statusCode.code != SccOk) && (cerP->statusCode.code != SccNone))  // Error - not to be added to output
      {
        continue;
      }

      //
      // Does the EntityId of cerP already exist in any of the contextElementResponses in the contextElementResponseVector?
      // If so, we just add the attributes of cerP to that contextElementResponse
      //
      ContextElementResponse* targetCerP = responseP->contextElementResponseVector.lookup(&cerP->contextElement.entityId);

      if (targetCerP != NULL)
      {
        targetCerP->contextElement.contextAttributeVector.push_back(&cerP->contextElement.contextAttributeVector);
      }
      else  // Not found so we will have to create a new ContextElementResponse
      {
        ContextElementResponse* newCerP = new ContextElementResponse(cerP);

        newCerP->statusCode.fill(SccOk);
        responseP->contextElementResponseVector.push_back(newCerP);
      }
    }
  }

  answer = responseP->render(apiVersion, asJsonObject);
  responseP->release();
  delete responseP;

  return answer;
}



/* ****************************************************************************
*
* QueryContextResponseVector::populate -
*/
void QueryContextResponseVector::populate(QueryContextResponse* responseP)
{
  //
  // We have a vector of QueryContextResponse.
  // What we need is ONE QueryContextResponse, so, we'll take all the
  // contextElementResponses from each of the QueryContextResponses in the vector and
  // move them to ONE QueryContextResponse (responseP)
  //

  if (vec.size() == 0)
  {
    //
    // Special case: vector is empty: translate to 404
    //
    responseP->errorCode.fill(SccContextElementNotFound);
  }
  else if ((vec.size() == 1) && (vec[0]->contextElementResponseVector.size() == 0))
  {
    //
    // Special case: only one QueryContextResponse in vec, and it has 0 contextElementResponses
    // This is clearly a Not Found ...
    //
    responseP->errorCode.fill(SccContextElementNotFound);
    vec[0]->errorCode.fill(SccContextElementNotFound);
  }
  else
  {
    //
    // We have found something, so, all good
    //
    responseP->errorCode.fill(SccOk);
  }

  for (unsigned int qIx = 0; qIx < vec.size(); ++qIx)
  {
    //
    // If the response vector is empty and the errorCode also, then a 404 Not Found
    // is inserted.
    //
    if (vec[qIx]->contextElementResponseVector.size() == 0)
    {
      if ((vec[qIx]->errorCode.code == SccOk) || (vec[qIx]->errorCode.code == SccNone))
      {
        ContextElementResponse* cerP = new ContextElementResponse();

        cerP->statusCode.fill(SccContextElementNotFound);
        responseP->contextElementResponseVector.push_back(cerP);
      }
    }

    for (unsigned int cerIx = 0; cerIx < vec[qIx]->contextElementResponseVector.size(); ++cerIx)
    {
      ContextElementResponse* cerP = vec[qIx]->contextElementResponseVector[cerIx];

      if ((cerP->statusCode.code != SccOk) && (cerP->statusCode.code != SccNone))  // Error - not to be added to output
      {
        continue;
      }

      //
      // Does the EntityId of cerP already exist in any of the contextElementResponses in the contextElementResponseVector?
      // If so, we just add the attributes of cerP to that contextElementResponse
      //
      ContextElementResponse* targetCerP = responseP->contextElementResponseVector.lookup(&cerP->contextElement.entityId);

      if (targetCerP != NULL)
      {
        targetCerP->contextElement.contextAttributeVector.push_back(&cerP->contextElement.contextAttributeVector);
      }
      else  // Not found so we will have to create a new ContextElementResponse
      {
        ContextElementResponse* newCerP = new ContextElementResponse(cerP);

        newCerP->statusCode.fill(SccOk);
        responseP->contextElementResponseVector.push_back(newCerP);
      }
    }
  }
}
