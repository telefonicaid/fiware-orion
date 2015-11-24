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
std::string ContextAttributeResponseVector::render(ConnectionInfo* ciP, RequestType request, std::string indent)
{
  std::string out     = "";
  std::string xmlTag  = "contextResponseList";
  std::string jsonTag = "contextResponses";

  if (vec.size() == 0)
  {
    if (request == IndividualContextEntityAttributes)
      return indent + "<contextAttributeList></contextAttributeList>\n";

    return "";
  }

  out += startTag(indent, xmlTag, jsonTag, ciP->outFormat, true);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
    out += vec[ix]->render(ciP, request, indent + "  ");
  out += endTag(indent, xmlTag, ciP->outFormat, false, true);

  return out;
}



/* ****************************************************************************
*
* ContextAttributeResponseVector::check - 
*/
std::string ContextAttributeResponseVector::check
(
  ConnectionInfo*  ciP,
  RequestType      request,
  std::string      indent,
  std::string      predetectedError,
  int              counter
)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(ciP, request, indent, predetectedError, counter)) != "OK")
    {
      return res;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextAttributeResponseVector::present - 
*/
void ContextAttributeResponseVector::present(std::string indent)
{
  LM_T(LmtPresent, ("%lu ContextAttributeResponses", (uint64_t) vec.size()));

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->present(indent + "  ");
  }
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
* ContextAttributeResponseVector::get - 
*/
ContextAttributeResponse* ContextAttributeResponseVector::get(int ix)
{
  return vec[ix];
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
