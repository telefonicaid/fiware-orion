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
#include "ngsi/EntityIdVector.h"
#include "ngsi/Request.h"


/* ****************************************************************************
*
* EntityIdVector::render - 
*/
std::string EntityIdVector::render(Format format, std::string indent, bool comma)
{
  std::string out     = "";
  std::string xmlTag  = "entityIdList";
  std::string jsonTag = "entities";

  if (vec.size() == 0)
    return "";

  out += startTag(indent, xmlTag, jsonTag, format, true, true);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
    out += vec[ix]->render(format, indent + "  ", ix != vec.size() - 1, true);

  out += endTag(indent, xmlTag, format, comma, true);

  return out;
}



/* ****************************************************************************
*
* EntityIdVector::check - 
*/
std::string EntityIdVector::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  // Only OK to be empty if part of a ContextRegistration
  if ((requestType == DiscoverContextAvailability)           ||
      (requestType == SubscribeContextAvailability)          ||
      (requestType == UpdateContextAvailabilitySubscription) ||
      (requestType == QueryContext)                          ||
      (requestType == SubscribeContext))
  {
    if (vec.size() == 0)
    {
      LM_E(("No entity list when it is mandatory"));
      return "No entities";
    }
  }

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
* EntityIdVector::present - 
*/
void EntityIdVector::present(std::string indent)
{
   PRINTF("%lu EntityIds:\n", (unsigned long) vec.size());

   for (unsigned int ix = 0; ix < vec.size(); ++ix)
      vec[ix]->present(indent, ix);
}



/* ****************************************************************************
*
* EntityIdVector::push_back - 
*/
void EntityIdVector::push_back(EntityId* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* EntityIdVector::get - 
*/
EntityId* EntityIdVector::get(int ix)
{
  return vec[ix];
}



/* ****************************************************************************
*
* EntityIdVector::size - 
*/
unsigned int EntityIdVector::size(void)
{
  return vec.size();
}



/* ****************************************************************************
*
* EntityIdVector::release - 
*/
void EntityIdVector::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->release();
    delete(vec[ix]);
  }

  vec.clear();
}
