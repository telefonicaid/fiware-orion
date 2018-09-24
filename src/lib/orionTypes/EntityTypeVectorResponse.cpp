/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"

#include "common/globals.h"
#include "common/tag.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/Request.h"
#include "orionTypes/EntityTypeVectorResponse.h"



/* ****************************************************************************
*
* EntityTypeVectorResponse::render -
*/
std::string EntityTypeVectorResponse::render
(
  bool        asJsonObject,
  bool        asJsonOut,
  bool        collapsed
)
{
  std::string out  = "";

  out += startTag();

  if (entityTypeVector.size() > 0)
  {
    out += entityTypeVector.render(asJsonObject, asJsonOut, collapsed, true);
  }

  out += statusCode.render(false);

  out += endTag();

  return out;
}



/* ****************************************************************************
*
* EntityTypeVectorResponse::check -
*/
std::string EntityTypeVectorResponse::check
(
  ApiVersion          apiVersion,
  bool                asJsonObject,
  bool                asJsonOut,
  bool                collapsed,
  const std::string&  predetectedError)
{
  std::string res;

  if (predetectedError != "")
  {
    statusCode.fill(SccBadRequest, predetectedError);
  }
  else if ((res = entityTypeVector.check(apiVersion, predetectedError)) != "OK")
  {
    alarmMgr.badInput(clientIp, res);
    statusCode.fill(SccBadRequest, res);
  }
  else
  {
    return "OK";
  }

  return render(asJsonObject, asJsonOut, collapsed);
}



/* ****************************************************************************
*
* EntityTypeVectorResponse::release -
*/
void EntityTypeVectorResponse::release(void)
{
  entityTypeVector.release();
  statusCode.release();
}


/* ****************************************************************************
*
* EntityTypeVectorResponse::toJson -
*/
std::string EntityTypeVectorResponse::toJson(bool values)
{
  std::string  out = "[";

  for (unsigned int ix = 0; ix < entityTypeVector.vec.size(); ++ix)
  {
    if (values)
    {
      out += JSON_STR(entityTypeVector.vec[ix]->type);
    }
    else  // default
    {
      out += entityTypeVector.vec[ix]->toJson(true);
    }

    if (ix != entityTypeVector.vec.size() - 1)
    {
      out += ",";
    }

  }

  out += "]";

  return out;
}
