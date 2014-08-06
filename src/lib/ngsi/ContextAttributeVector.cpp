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
#include "ngsi/ContextAttributeVector.h"
#include "ngsi/Request.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* ContextAttributeVector::ContextAttributeVector - 
*/
ContextAttributeVector::ContextAttributeVector()
{
  vec.clear();
}



/* ****************************************************************************
*
* ContextAttributeVector::render - 
*/
std::string ContextAttributeVector::render(ConnectionInfo* ciP, RequestType request, const std::string& indent, bool comma)
{
  std::string out      = "";
  std::string xmlTag   = "contextAttributeList";
  std::string jsonTag  = "attributes";

  if (vec.size() == 0)
  {
     if (((request == IndividualContextEntityAttribute)
         || (request == AttributeValueInstance)
         || (request == IndividualContextEntityAttributes)) && ciP->outFormat == XML)
      return indent + "<contextAttributeList></contextAttributeList>\n";

    return "";
  }

  out += startTag(indent, xmlTag, jsonTag, ciP->outFormat, true, true);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
    out += vec[ix]->render(ciP, indent + "  ", ix != vec.size() - 1);
  out += endTag(indent, xmlTag, ciP->outFormat, comma, true);

  return out;
}



/* ****************************************************************************
*
* ContextAttributeVector::check - 
*/
std::string ContextAttributeVector::check(RequestType requestType, Format format, const std::string& indent, const std::string& predetectedError, int counter)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(requestType, format, indent, predetectedError, 0)) != "OK")
      return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextAttributeVector::present - 
*/
void ContextAttributeVector::present(const std::string& indent)
{
   PRINTF("%lu ContextAttributes", (unsigned long) vec.size());

   for (unsigned int ix = 0; ix < vec.size(); ++ix)
      vec[ix]->present(indent, ix);
}



/* ****************************************************************************
*
* ContextAttributeVector::push_back - 
*/
void ContextAttributeVector::push_back(ContextAttribute* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* ContextAttributeVector::get - 
*/
ContextAttribute* ContextAttributeVector::get(unsigned int ix)
{
  if (ix < vec.size())
    return vec[ix];
  return NULL;
}



/* ****************************************************************************
*
* ContextAttributeVector::size - 
*/
unsigned int ContextAttributeVector::size(void)
{
  return vec.size();
}



/* ****************************************************************************
*
* ContextAttributeVector::release - 
*/
void ContextAttributeVector::release(void)
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
* ContextAttributeVector::fill - 
*/
void ContextAttributeVector::fill(ContextAttributeVector& caV)
{
  for (unsigned int ix = 0; ix < caV.size(); ++ix)
  {
    ContextAttribute* caP = new ContextAttribute(caV.get(ix));

    push_back(caP);
  }
}
