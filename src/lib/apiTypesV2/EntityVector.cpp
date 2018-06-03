/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <map>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/Request.h"
#include "apiTypesV2/EntityVector.h"



/* ****************************************************************************
*
* EntityVector::render -
*/
std::string EntityVector::render
(
  std::map<std::string, bool>&         uriParamOptions,
  std::map<std::string, std::string>&  uriParam
)
{
  if (vec.size() == 0)
  {
    return "[]";
  }

  std::string out;

  out += "[";

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    out += vec[ix]->render(uriParamOptions, uriParam, ix != vec.size() - 1);
  }

  out += "]";

  return out;
}



/* ****************************************************************************
*
* EntityVector::check -
*/
std::string EntityVector::check(RequestType requestType)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(requestType)) != "OK")
    {
      alarmMgr.badInput(clientIp, "invalid vector of Entity");
      return res;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* EntityVector::present -
*/
void EntityVector::present(const std::string& indent)
{
  LM_T(LmtPresent, ("%lu Entities:\n", (uint64_t) vec.size()));

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->present(indent + "  ");
  }
}



/* ****************************************************************************
*
* EntityVector::push_back -
*/
void EntityVector::push_back(Entity* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* EntityVector::operator[] -
*/
Entity*  EntityVector::operator[] (unsigned int ix) const
{
  if (ix < vec.size())
  {
    return vec[ix];
  }

  return NULL;
}



/* ****************************************************************************
*
* EntityVector::size -
*/
unsigned int EntityVector::size(void)
{
  return vec.size();
}



/* ****************************************************************************
*
* EntityVector::release -
*/
void EntityVector::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->release();
    delete(vec[ix]);
  }

  vec.clear();
}
