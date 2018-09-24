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

#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "common/tag.h"
#include "convenience/ContextAttributeResponseVector.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* ContextAttributeResponseVector::render - 
*/
std::string ContextAttributeResponseVector::render
(
  ApiVersion          apiVersion,
  bool                asJsonObject,
  RequestType         request
)
{
  std::string out = "";
  std::string key = "contextResponses";

  if (vec.size() == 0)
  {
    return "";
  }

  out += startTag(key, true);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    out += vec[ix]->render(asJsonObject, request);
  }
  out += endTag(false, true);

  return out;
}



/* ****************************************************************************
*
* ContextAttributeResponseVector::check - 
*/
std::string ContextAttributeResponseVector::check
(
  ApiVersion          apiVersion,
  bool                asJsonObject,
  RequestType         request,
  const std::string&  predetectedError
)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(apiVersion, asJsonObject, request, predetectedError)) != "OK")
    {
      return res;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextAttributeResponseVector::push_back - 
*/
void ContextAttributeResponseVector::push_back(ContextAttributeResponse* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* ContextAttributeResponseVector::operator[] -
*/
ContextAttributeResponse* ContextAttributeResponseVector::operator[](unsigned int ix) const
{
  if (ix < vec.size())
  {
    return vec[ix];
  }
  return NULL;
}


/* ****************************************************************************
*
* ContextAttributeResponseVector::size - 
*/
unsigned int ContextAttributeResponseVector::size(void)
{
  return vec.size();
}



/* ****************************************************************************
*
* ContextAttributeResponseVector::release - 
*/
void ContextAttributeResponseVector::release(void)
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
* ContextAttributeResponseVector::fill -
*/
void ContextAttributeResponseVector::fill(ContextAttributeVector* cavP, const StatusCode& statusCode)
{
  vec.push_back(new ContextAttributeResponse());
  vec[0]->fill(cavP, statusCode);
}
