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
#include "ngsi/ContextRegistrationAttributeVector.h"



/* ****************************************************************************
*
* ContextRegistrationAttributeVector::render -
*/
std::string ContextRegistrationAttributeVector::render(const std::string& indent, bool comma)
{

  std::string key = "attributes";
  std::string out = "";

  if (vec.size() == 0)
  {
    return "";
  }

  out += startTag2(indent, key, true, true);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    out += vec[ix]->render(indent + "  ", ix != vec.size() - 1);
  }
  out += endTag(indent, comma, true);

  return out;
}



/* ****************************************************************************
*
* ContextRegistrationAttributeVector::check -
*/
std::string ContextRegistrationAttributeVector::check
(
  const std::string&  apiVersion,
  RequestType         requestType,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(apiVersion, indent)) != "OK")
    {
      return res;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextRegistrationAttributeVector::present -
*/
void ContextRegistrationAttributeVector::present(const std::string& indent)
{
  LM_T(LmtPresent, ("%lu ContextRegistrationAttributes", (uint64_t) vec.size()));

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->present(ix, indent);
  }
}



/* ****************************************************************************
*
* ContextRegistrationAttributeVector::push_back -
*/
void ContextRegistrationAttributeVector::push_back(ContextRegistrationAttribute* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* ContextRegistrationAttributeVector::operator[] -
*/
ContextRegistrationAttribute* ContextRegistrationAttributeVector::operator[] (unsigned int ix) const
{
    if (ix < vec.size())
    {
      return vec[ix];
    }
    return NULL;  
}



/* ****************************************************************************
*
* ContextRegistrationAttributeVector::size -
*/
unsigned int ContextRegistrationAttributeVector::size(void)
{
  return vec.size();
}



/* ****************************************************************************
*
* ContextRegistrationAttributeVector::release -
*/
void ContextRegistrationAttributeVector::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->release();
    delete(vec[ix]);
  }

  vec.clear();
}
