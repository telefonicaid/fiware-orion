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
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/ConditionValueList.h"



/* ****************************************************************************
*
* render - 
*/
std::string ConditionValueList::render(Format format, std::string indent)
{
  std::string  out = "";
  std::string  tag = "condValueList";

  if (vec.size() == 0)
    return "";

  out += startTag(indent, tag, format);

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
    out += valueTag(indent + "  ", "condValue", vec[ix], format, ix != vec.size() - 1);

  out += endTag(indent, tag, format);

  return out;
}



/* ****************************************************************************
*
* ConditionValueList::check - 
*/
std::string ConditionValueList::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (vec[ix] == "")
      return "empty condValue name";
  }

  return "OK";
}



/* ****************************************************************************
*
* ConditionValueList::present - 
*/
void ConditionValueList::present(std::string indent)
{
  PRINTF("%sConditionValue List\n",    indent.c_str());

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
    PRINTF("%s  %s\n", indent.c_str(), vec[ix].c_str());
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
void ConditionValueList::push_back(std::string attributeName)
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
* ConditionValueList::get - 
*/
std::string ConditionValueList::get(int ix)
{
   return vec[ix];
}
