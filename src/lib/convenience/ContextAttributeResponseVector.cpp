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

#include "common/globals.h"
#include "common/tag.h"
#include "convenience/ContextAttributeResponseVector.h"



/* ****************************************************************************
*
* ContextAttributeResponseVector::render - 
*/
std::string ContextAttributeResponseVector::render(RequestType requestType, Format format, std::string indent)
{
  std::string out = "";
  std::string tag = "contextResponseList";

  if (vec.size() == 0)
    return "";

  out += startTag(indent, tag, format);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
    out += vec[ix]->render(requestType, format, indent + "  ");
  out += endTag(indent, tag, format);

  return out;
}



/* ****************************************************************************
*
* ContextAttributeResponseVector::check - 
*/
std::string ContextAttributeResponseVector::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
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
* ContextAttributeResponseVector::present - 
*/
void ContextAttributeResponseVector::present(std::string indent)
{
   PRINTF("%lu ContextAttributeResponses", (unsigned long) vec.size());

   for (unsigned int ix = 0; ix < vec.size(); ++ix)
      vec[ix]->present(indent + "  ");
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
