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

#include "logMsg/logMsg.h"
#include "common/Format.h"
#include "common/tag.h"
#include "common/globals.h"
#include "ngsi/AttributeAssociationList.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* push_back -
*/
void AttributeAssociationList::push_back(AttributeAssociation* aaP)
{
  vec.push_back(aaP);
}



/* ****************************************************************************
*
* render -
*/
std::string AttributeAssociationList::render(Format format, const std::string& indent, bool comma)
{
  std::string out     = "";
  std::string xmlTag  = "attributeAssociationList";
  std::string jsonTag = "attributeAssociations";

  if (vec.size() == 0)
    return "";

  out += startTag(indent, xmlTag, jsonTag, format, true, true);

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
     out += vec[ix]->render(format, indent + "  ", ix != vec.size() - 1);

  out += endTag(indent, xmlTag, format, comma, true, false);

  return out;
}



/* ****************************************************************************
*
* check -
*/
std::string AttributeAssociationList::check
(
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(requestType, format, indent, predetectedError, 0)) != "OK")
    {
      return res;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* present -
*/
void AttributeAssociationList::present(const std::string& indent)
{
  LM_F(("%lu Attribute Associations", (uint64_t) vec.size()));

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->present(indent, ix);
  }
}



/* ****************************************************************************
*
* AttributeAssociationList::get -
*/
AttributeAssociation* AttributeAssociationList::get(int ix)
{
  return vec[ix];
}



/* ****************************************************************************
*
* AttributeAssociationList::size -
*/
unsigned int AttributeAssociationList::size(void)
{
  return vec.size();
}



/* ****************************************************************************
*
* AttributeAssociationList::release -
*/
void AttributeAssociationList::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    delete vec[ix];
  }

  vec.clear();
}
