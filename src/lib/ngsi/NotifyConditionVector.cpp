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
#include "ngsi/NotifyConditionVector.h"



/* ****************************************************************************
*
* NotifyConditionVector::NotifyConditionVector - 
*/
NotifyConditionVector::NotifyConditionVector()
{
}



/* ****************************************************************************
*
* NotifyConditionVector::render -
*/
std::string NotifyConditionVector::render(bool comma)
{
  std::string out = "";

  if (vec.size() == 0)
  {
    return "";
  }

  out += startTag("notifyConditions", true);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    out += vec[ix]->render(ix != vec.size() - 1);
  }
  out += endTag(comma, true);

  return out;
}



/* ****************************************************************************
*
* NotifyConditionVector::check -
*/
std::string NotifyConditionVector::check
(
  RequestType         requestType,
  const std::string&  predetectedError,
  int                 counter
)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(requestType, predetectedError, counter)) != "OK")
    {
      return res;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* NotifyConditionVector::present -
*/
void NotifyConditionVector::present(const std::string& indent)
{
  LM_T(LmtPresent, ("%s%lu NotifyConditions", 
		    indent.c_str(), 
		    (uint64_t) vec.size()));

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->present(indent, ix);
  }
}



/* ****************************************************************************
*
* NotifyConditionVector::push_back -
*/
void NotifyConditionVector::push_back(NotifyCondition* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* NotifyConditionVector::operator[] -
*/
NotifyCondition* NotifyConditionVector::operator[] (unsigned int ix) const
{
   if (ix < vec.size())
   {
     return vec[ix];
   }
   return NULL;
}



/* ****************************************************************************
*
* NotifyConditionVector::size -
*/
unsigned int NotifyConditionVector::size(void) const
{
  return vec.size();
}



/* ****************************************************************************
*
* NotifyConditionVector::release -
*/
void NotifyConditionVector::release(void)
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
* NotifyConditionVector::fill - 
*/
void NotifyConditionVector::fill(const NotifyConditionVector& nv)
{
  for (unsigned int ncIx = 0; ncIx < nv.size(); ++ncIx)
  {
    NotifyCondition* ncP = new NotifyCondition(nv[ncIx]);
    push_back(ncP);
  }
}
