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
#include "common/JsonHelper.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/Request.h"
#include "apiTypesV2/EntityVector.h"



/* ****************************************************************************
*
* EntityVector::toJson -
*/
std::string EntityVector::toJson
(
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsFilter,
  bool                             blacklist,
  const std::vector<std::string>&  metadataFilter
)
{
  JsonVectorHelper jh;

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    // This is to avoid spurious entities like '{"id": "E", "type": "T"}'
    // typically generated when CPrs are in use
    if (vec[ix]->attributeVector.size() == 0)
    {
      continue;
    }
    jh.addRaw(vec[ix]->toJson(renderFormat, attrsFilter, blacklist, metadataFilter));
  }

  return jh.str();
}



/* ****************************************************************************
*
* EntityVector::toJsonV1 -
*
* Ported from old class ContextElementVector
*/
std::string EntityVector::toJsonV1
(
  bool         asJsonObject,
  RequestType  requestType,
  bool         comma
)
{
  std::string  out = "";

  if (vec.size() == 0)
  {
    return "";
  }

  out += startTag("contextElements", true);

  // No attribute or metadata filter in this case, an empty vector is used to fulfil method signature
  std::vector<std::string> emptyV;

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    out += vec[ix]->toJsonV1(asJsonObject, requestType, emptyV, false, emptyV, ix != vec.size() - 1);
  }

  out += endTag(comma, true);

  return out;
}



/* ****************************************************************************
*
* EntityVector::check -
*/
std::string EntityVector::check(ApiVersion apiVersion, RequestType requestType)
{
  if ((apiVersion == V1) && (requestType == UpdateContext))
  {
    if (vec.size() == 0)
    {
      return "No context elements";
    }
  }

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(apiVersion, requestType)) != "OK")
    {
      if (apiVersion == V2)
      {
        alarmMgr.badInput(clientIp, "invalid vector of Entity");
      }
      return res;
    }
  }

  return "OK";
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
* EntityVector::lookup -
*/
Entity* EntityVector::lookup(const std::string& name, const std::string& type)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if ((vec[ix]->id == name) && (vec[ix]->type == type))
    {
      return vec[ix];
    }
  }

  return NULL;
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
