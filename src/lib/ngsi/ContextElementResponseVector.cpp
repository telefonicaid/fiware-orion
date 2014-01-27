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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <stdio.h>
#include <string>
#include <vector>

#include "logMsg/logMsg.h"

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/ContextElementResponseVector.h"



/* ****************************************************************************
*
* ContextElementResponseVector::render - 
*/
std::string ContextElementResponseVector::render(RequestType requestType, Format format, std::string indent, bool comma)
{
  std::string xmlTag   = "contextResponseList";
  std::string jsonTag  = "contextResponses";
  std::string out      = "";

  if (vec.size() == 0)
    return "";

  out += startTag(indent, xmlTag, jsonTag, format, true, true);

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
    out += vec[ix]->render(requestType, format, indent + "  ", ix < (vec.size() - 1));

  out += endTag(indent, xmlTag, format, comma, true);

  return out;
}



/* ****************************************************************************
*
* ContextElementResponseVector::check - 
*/
std::string ContextElementResponseVector::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(requestType, format, indent, predetectedError, counter)) != "OK")
      return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextElementResponseVector::present - 
*/
void ContextElementResponseVector::present(std::string indent)
{
   PRINTF("%lu ContextElementResponses", (unsigned long) vec.size());

   for (unsigned int ix = 0; ix < vec.size(); ++ix)
      vec[ix]->present(indent, ix);
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
* ContextElementResponseVector::get - 
*/
ContextElementResponse* ContextElementResponseVector::get(int ix)
{
  return vec[ix];
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
