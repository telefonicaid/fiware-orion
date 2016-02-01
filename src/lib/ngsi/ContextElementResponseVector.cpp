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
#include "ngsi/ContextElementResponseVector.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* ContextElementResponseVector::render -
*/
std::string ContextElementResponseVector::render
(
  ConnectionInfo*     ciP,
  RequestType         requestType,
  const std::string&  indent,
  bool                comma,
  bool                omitAttributeValues
)
{
  std::string xmlTag   = "contextResponseList";
  std::string jsonTag  = "contextResponses";
  std::string out      = "";

  if (vec.size() == 0)
  {
    return "";
  }

  out += startTag(indent, xmlTag, jsonTag, ciP->outFormat, true, true);

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    out += vec[ix]->render(ciP, requestType, indent + "  ", ix < (vec.size() - 1), omitAttributeValues);
  }

  out += endTag(indent, xmlTag, ciP->outFormat, comma, true);

  return out;
}



/* ****************************************************************************
*
* ContextElementResponseVector::check -
*/
std::string ContextElementResponseVector::check
(
  ConnectionInfo*     ciP,
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  for (uint64_t ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(ciP, requestType, format, indent, predetectedError, counter)) != "OK")
    {
      return res;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextElementResponseVector::present -
*/
void ContextElementResponseVector::present(const std::string& indent)
{
  LM_T(LmtPresent, ("%s%lu ContextElementResponses", 
		    indent.c_str(), 
		    (uint64_t) vec.size()));

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->present(indent + "  ", ix);
  }
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
unsigned int ContextElementResponseVector::size(void)
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
ContextElementResponse* ContextElementResponseVector::lookup(EntityId* eP, HttpStatusCode code)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (vec[ix]->contextElement.entityId.equal(eP) == true)
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
