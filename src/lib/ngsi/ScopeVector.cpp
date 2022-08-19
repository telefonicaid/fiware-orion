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
#include "common/limits.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/ScopeVector.h"



/* ****************************************************************************
*
* ScopeVector::toJsonV1 -
*/
std::string ScopeVector::toJsonV1(bool comma)
{
  std::string out = "";

  if (vec.size() == 0)
  {
    return "";
  }

  out += startTag("scope", true);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
     out += vec[ix]->toJsonV1(ix != vec.size() - 1);
  }
  out += endTag(comma, true);

  return out;
}



/* ****************************************************************************
*
* ScopeVector::check -
*/
std::string ScopeVector::check(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check()) != "OK")
    {
      char ixV[STRING_SIZE_FOR_INT];
      snprintf(ixV, sizeof(ixV), "%d", ix);
      std::string details = std::string("error in scope ") + ixV + ": " + res;
      alarmMgr.badInput(clientIp, details);
      return res;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* ScopeVector::push_back -
*/
void ScopeVector::push_back(Scope* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* ScopeVector::operator[] -
*/
Scope* ScopeVector::operator[](unsigned int ix) const
{
   if (ix < vec.size())
   {
     return vec[ix];
   }
   return NULL;
}



/* ****************************************************************************
*
* ScopeVector::size -
*/
unsigned int ScopeVector::size(void) const
{
  return vec.size();
}



/* ****************************************************************************
*
* ScopeVector::release -
*/
void ScopeVector::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->release();
    delete(vec[ix]);
  }

  vec.clear();
}



/* ****************************************************************************
*
* ScopeVector::fill(ScopeVector) - 
*
* If the parameter 'copy' is set to false, then no copy of the scopes is made, 
* they are just referenced from this ScopeVector as well.
* This case is meant to save some time, allocating new scopes anf freeing the old scopes.
* Doesn't make much sense to on allocate new and delete the 'original'.
* So, what the caller of this method must do after calling ScopeVector::fill, is to
* simply *clear* the original ScopeVector to avoid a double free on the scopes.
*/
void ScopeVector::fill(const ScopeVector& scopeV, bool copy)
{
  for (unsigned int ix = 0; ix < scopeV.vec.size(); ++ix)
  {
    if (copy == false)
    {
      vec.push_back(scopeV[ix]);
    }
    else
    {
      // FIXME P5: if this is ever needed, it must be implemented here
    }
  }
}
