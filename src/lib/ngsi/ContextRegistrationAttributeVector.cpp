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
std::string ContextRegistrationAttributeVector::render(bool comma)
{
  std::string out = "";

  if (vec.size() == 0)
  {
    return "";
  }

  out += startTag("attributes", true);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    out += vec[ix]->render(ix != vec.size() - 1);
  }
  out += endTag(comma, true);

  return out;
}



/* ****************************************************************************
*
* ContextRegistrationAttributeVector::check -
*/
std::string ContextRegistrationAttributeVector::check(ApiVersion apiVersion)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(apiVersion)) != "OK")
    {
      return res;
    }
  }

  return "OK";
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
    delete(vec[ix]);
  }

  vec.clear();
}
