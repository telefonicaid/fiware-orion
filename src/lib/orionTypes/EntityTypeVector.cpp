/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <map>
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "orionTypes/EntityType.h"
#include "orionTypes/EntityTypeVector.h"



/* ****************************************************************************
*
* EntityTypeVector::EntityTypeVector -
*/
EntityTypeVector::EntityTypeVector()
{
  vec.clear();
}


/* ****************************************************************************
*
* EntityTypeVector::render -
*/
std::string EntityTypeVector::render
(
  ApiVersion  apiVersion,
  bool        asJsonObject,
  bool        asJsonOut,
  bool        collapsed,
  bool        comma
)
{
  std::string out  = "";

  if (vec.size() > 0)
  {
    out += startTag("types", true);

    for (unsigned int ix = 0; ix < vec.size(); ++ix)
    {
      out += vec[ix]->render(apiVersion, asJsonObject, asJsonOut, collapsed, ix != vec.size() - 1);
    }
    out += endTag(comma, true);
  }

  return out;
}



/* ****************************************************************************
*
* EntityTypeVector::check -
*/
std::string EntityTypeVector::check(ApiVersion apiVersion, const std::string& predetectedError)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(apiVersion, predetectedError)) != "OK")
    {
     return res;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* EntityTypeVector::push_back -
*/
void EntityTypeVector::push_back(EntityType* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* EntityTypeVector::operator[] -
*/
EntityType* EntityTypeVector::operator[] (unsigned int ix) const
{
   if (ix < vec.size())
   {
     return vec[ix];
   }
   return NULL;
}



/* ****************************************************************************
*
* EntityTypeVector::size -
*/
unsigned int EntityTypeVector::size(void)
{
  return vec.size();
}



/* ****************************************************************************
*
* EntityTypeVector::release -
*/
void EntityTypeVector::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->release();
    delete vec[ix];
  }

  vec.clear();
}
