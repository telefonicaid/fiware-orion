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
#include "common/tag.h"
#include "common/RenderFormat.h"
#include "common/JsonHelper.h"
#include "ngsi/ContextElementResponseVector.h"



/* ****************************************************************************
*
* ContextElementResponseVector::toJsonV1 -
*/
std::string ContextElementResponseVector::toJsonV1
(
  bool                             asJsonObject,
  RequestType                      requestType,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  const std::vector<std::string>&  metadataFilter,
  bool                             comma,
  bool                             omitAttributeValues
)
{
  std::string out = "";

  if (vec.size() == 0)
  {
    return "";
  }

  out += startTag("contextResponses", true);

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    out += vec[ix]->toJsonV1(asJsonObject, requestType, attrsFilter, blacklist, metadataFilter, ix < (vec.size() - 1), omitAttributeValues);
  }

  out += endTag(comma, true);

  return out;
}



/* ****************************************************************************
*
* ContextElementResponseVector::toJson - 
*/
std::string ContextElementResponseVector::toJson
(
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  const std::vector<std::string>&  metadataFilter
)
{
  JsonVectorHelper jvh;

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    jvh.addRaw(vec[ix]->toJson(renderFormat, attrsFilter, blacklist, metadataFilter));
  }

  return jvh.str();
}



/* ****************************************************************************
*
* ContextElementResponseVector::check -
*/
std::string ContextElementResponseVector::check
(
  ApiVersion          apiVersion,
  RequestType         requestType,
  const std::string&  predetectedError,
  int                 counter
)
{
  for (uint64_t ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(apiVersion, requestType, predetectedError, counter)) != "OK")
    {
      return res;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextElementResponseVector::push_back -
*/
void ContextElementResponseVector::push_back(ContextElementResponse* item)
{
  vec.push_back(item);
}

/* ****************************************************************************
*
* ContextElementResponseVector::size -
*/
unsigned int ContextElementResponseVector::size(void) const
{
    
  return vec.size();

}


/* ****************************************************************************
*
* ContextElementResponseVector::operator[] -
*/
ContextElementResponse*  ContextElementResponseVector::operator[] (unsigned int ix) const
{
  if (ix < vec.size())
  {
    return vec[ix];
  }
  return NULL;
}


/* ****************************************************************************
*
* ContextElementResponseVector::release -
*/
void ContextElementResponseVector::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->release();
    delete vec[ix];
  }

  vec.clear();
}



/* ****************************************************************************
*
* ContextElementResponseVector::lookup -
*/
ContextElementResponse* ContextElementResponseVector::lookup(Entity* eP, HttpStatusCode code)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (vec[ix]->entity.equal(eP) == true)
    {
      if ((code == SccNone) || (vec[ix]->statusCode.code == code))
      {
        return vec[ix];
      }
    }
  }

  return NULL;
}



/* ****************************************************************************
*
* ContextElementResponseVector::fill - 
*/
void ContextElementResponseVector::fill(ContextElementResponseVector& cerV)
{
  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    ContextElementResponse* cerP = new ContextElementResponse(cerV[ix]);

    push_back(cerP);
  }
}
