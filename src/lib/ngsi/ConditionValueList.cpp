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
#include "ngsi/ConditionValueList.h"



/* ****************************************************************************
*
* toJsonV1 -
*/
std::string ConditionValueList::toJsonV1(bool comma)
{
  std::string  out = "";

  if (vec.size() == 0)
  {
    return "";
  }

  out += startTag("condValueList", true);

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    out += valueTag("", vec[ix], ix != vec.size() - 1, true);
  }

  out += endTag(comma, true);

  return out;
}



/* ****************************************************************************
*
* ConditionValueList::check - 
*/
std::string ConditionValueList::check(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (vec[ix].empty())
    {
      return "empty condValue name";
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* ConditionValueList::release - 
*/
void ConditionValueList::release(void)
{
  vec.clear();
}



/* ****************************************************************************
*
* push_back - 
*/
void ConditionValueList::push_back(const std::string& attributeName)
{
  vec.push_back(attributeName);
}



/* ****************************************************************************
*
* ConditionValueList::size - 
*/
unsigned int ConditionValueList::size(void)
{
  return vec.size();
}



/* ****************************************************************************
*
* ConditionValueList::operator
*/
std::string ConditionValueList::operator[] (unsigned int ix) const
{
  if (ix < vec.size())
  {
    return vec[ix];
  }

  return "";
}



/* ****************************************************************************
*
* ConditionValueList::fill - 
*/
void ConditionValueList::fill(ConditionValueList& list)
{
  for (unsigned int cvIx = 0; cvIx < list.size(); ++cvIx)
  {
    push_back(list[cvIx]);
  }
}
