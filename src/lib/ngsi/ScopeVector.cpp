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
#include "ngsi/ScopeVector.h"



/* ****************************************************************************
*
* ScopeVector::render - 
*/
std::string ScopeVector::render(Format format, std::string indent, bool comma)
{
  std::string out = "";
  std::string tag = "scope";

  if (vec.size() == 0)
    return "";

  out += startTag(indent, tag, tag, format, true, true);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
     out += vec[ix]->render(format, indent + "  ", ix != vec.size() - 1);
  out += endTag(indent, tag, format, comma, true);

  return out;
}



/* ****************************************************************************
*
* ScopeVector::check - 
*/
std::string ScopeVector::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  LM_M(("Checking scope vector of %d scopes", vec.size()));

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(requestType, format, indent, predetectedError, counter)) != "OK")
    {
      LM_E(("error for scope %d: %s", ix, res.c_str()));
      return res;
    }
    else
      LM_M(("for scope %d: %s", ix, res.c_str()));
  }

  return "OK";
}



/* ****************************************************************************
*
* ScopeVector::present - 
*/
void ScopeVector::present(std::string indent)
{
  if (vec.size() == 0)
    PRINTF("No scopes\n");
  else
    PRINTF("%lu Scopes:\n", (unsigned long) vec.size());

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
    vec[ix]->present(indent, ix);
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
* ScopeVector::get - 
*/
Scope* ScopeVector::get(int ix)
{
  return vec[ix];
}



/* ****************************************************************************
*
* ScopeVector::size - 
*/
unsigned int ScopeVector::size(void)
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
