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
#include "ngsi/AttributeList.h"



/* ****************************************************************************
*
* AttributeList::fill - 
*/
void AttributeList::fill(std::vector<std::string> aVec)
{
  for (unsigned int ix = 0; ix < aVec.size(); ++ix)
  {
    attributeV.push_back(aVec[ix]);
  }
}



/* ****************************************************************************
*
* render - 
*/
std::string AttributeList::render(Format format, const std::string& indent, bool comma)
{
  std::string  out     = "";
  std::string  xmlTag  = "attributeList";
  std::string  jsonTag = "attributes";

  if (attributeV.size() == 0)
    return "";

  out += startTag(indent, xmlTag, jsonTag, format, true, true, false);

  for (unsigned int ix = 0; ix < attributeV.size(); ++ix)
  {
    out += valueTag(indent + "  ", "attribute", attributeV[ix], format, ix != attributeV.size() - 1, true);
  }

  out += endTag(indent, xmlTag, format, comma, true);

  return out;
}



/* ****************************************************************************
*
* AttributeList::check - 
*/
std::string AttributeList::check
(
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  for (unsigned int ix = 0; ix < attributeV.size(); ++ix)
  {
    if (attributeV[ix] == "")
      return "empty attribute name";
  }

  return "OK";
}



/* ****************************************************************************
*
* AttributeList::present - 
*/
void AttributeList::present(const std::string& indent)
{
  LM_T(LmtPresent, ("%sAttribute List",    indent.c_str()));

  for (unsigned int ix = 0; ix < attributeV.size(); ++ix)
  {
    LM_T(LmtPresent, ("%s  %s", 
		      indent.c_str(), 
		      attributeV[ix].c_str()));
  }
}



/* ****************************************************************************
*
* AttributeList::release - 
*/
void AttributeList::release(void)
{
  attributeV.clear();
}



/* ****************************************************************************
*
* lookup - 
*/
bool AttributeList::lookup(const std::string& attributeName)
{
  for (unsigned int ix = 0; ix < attributeV.size(); ++ix)
  {
    if (attributeV[ix] == attributeName)
    {
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* push_back - 
*/
void AttributeList::push_back(const std::string& attributeName)
{
  attributeV.push_back(attributeName);
}



/* ****************************************************************************
*
* push_back_if_absent - 
*/
void AttributeList::push_back_if_absent(const std::string& attributeName)
{
  if (lookup(attributeName) == false)
  {
    attributeV.push_back(attributeName);
  }
}



/* ****************************************************************************
*
* AttributeList::size - 
*/
unsigned int AttributeList::size(void) const
{
  return attributeV.size();
}



/* ****************************************************************************
*
* AttributeList::get - 
*/
std::string AttributeList::get(int ix) const
{
  return attributeV[ix];
}



/* ****************************************************************************
*
* AttributeList::clone - 
*/
void AttributeList::clone(AttributeList& aList)
{
  for (unsigned int ix = 0; ix < aList.size(); ++ix)
  {
    push_back(aList[ix]);
  }
}
