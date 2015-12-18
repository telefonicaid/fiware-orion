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
#include "ngsi/ScopeVector.h"



/* ****************************************************************************
*
* ScopeVector::render -
*/
std::string ScopeVector::render(Format format, const std::string& indent, bool comma)
{
  std::string out = "";
  std::string tag = "scope";

  if (vec.size() == 0)
  {
    return "";
  }

  out += startTag(indent, tag, tag, format, true, true);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
     out += vec[ix]->render(format, indent + "  ", ix != vec.size() - 1);
  }
  out += endTag(indent, tag, format, comma, true);

  return out;
}



/* ****************************************************************************
*
* ScopeVector::check -
*/
std::string ScopeVector::check
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

    if ((res = vec[ix]->check(requestType, format, indent, predetectedError, counter)) != "OK")
    {
      LM_W(("Bad Input (error in scope %d: %s)", ix, res.c_str()));
      return res;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* ScopeVector::present -
*/
void ScopeVector::present(const std::string& indent)
{
  if (vec.size() == 0)
  {
    LM_T(LmtPresent, ("%sNo scopes", indent.c_str()));
  }
  else
  {
    LM_T(LmtPresent, ("%s%lu Scopes:", 
		      indent.c_str(), 
		      (uint64_t) vec.size()));
  }

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->present(indent + "  ", ix);
  }
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
* ScopeVector::operator -
*/
/*Scope* ScopeVector::operator[](unsigned int ix)
{
  return (ix < vec.size())? vec[ix] : NULL;
}*/
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
