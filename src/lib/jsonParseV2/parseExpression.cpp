/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán
*/

#include <string>

#include "jsonParseV2/parseExpression.h"

#include "jsonParseV2/utilsParse.h"
#include "common/errorMessages.h"



/* ****************************************************************************
*
* parseExpression -
*
* This function can be called from the batch query parsing logic (subsP == NULL)
* and from the subscription creation/update logic (subP != NULL). In the second case
* the subscription object is filled properly
*/
std::string parseExpression
(
  const rapidjson::Value&      expression,
  Expression&                  expr
)
{
  std::string  errorString;

  if (!expression.IsObject())
  {
    return "expression is not an object";
  }

  if (expression.ObjectEmpty())
  {
    return "expression is empty";
  }

  if (expression.HasMember("q"))
  {
    const rapidjson::Value& q = expression["q"];
    std::string  qString;

    if (!q.IsString())
    {
      return "q is not a string";
    }

    qString = q.GetString();
    if (qString.empty())
    {
      return "q is empty";
    }

    expr.q = qString;

    if (expr.stringFilter.parse(qString.c_str(), &errorString) == false)
    {
      return errorString;
    }
  }

  if (expression.HasMember("mq"))
  {
    const rapidjson::Value& mq = expression["mq"];
    std::string  mqString;

    if (!mq.IsString())
    {
      return "mq is not a string";
    }

    mqString = mq.GetString();
    if (mqString.empty())
    {
      return "mq is empty";
    }

    expr.mq = mqString;

    if (expr.mdStringFilter.parse(mqString.c_str(), &errorString) == false)
    {
      return errorString;
    }
  }

  // geometry
  std::string  georel;
  std::string  geometry;
  std::string  coords;
  {
    Opt<std::string> geometryOpt = getStringOpt(expression, "geometry");

    if (!geometryOpt.ok())
    {
      return geometryOpt.error;
    }
    else if (geometryOpt.given)
    {
      if (geometryOpt.value.empty())
      {
        return "geometry is empty";
      }

      geometry = geometryOpt.value;

      expr.geometry = geometry;
    }
  }

  // coords
  {
    Opt<std::string> coordsOpt = getStringOpt(expression, "coords");

    if (!coordsOpt.ok())
    {
      return coordsOpt.error;
    }
    else if (coordsOpt.given)
    {
      if (coordsOpt.value.empty())
      {
        return "coords is empty";
      }

      coords = coordsOpt.value;

      expr.coords = coords;
    }
  }

  // georel
  {
    Opt<std::string> georelOpt = getStringOpt(expression, "georel");

    if (!georelOpt.ok())
    {
      return georelOpt.error;
    }

    if (georelOpt.given)
    {
      if (georelOpt.value.empty())
      {
        return "georel is empty";
      }

      georel = georelOpt.value;

      expr.georel = georel;
    }
  }

  bool atLeastOne = ((!georel.empty()) || (!geometry.empty()) || (!coords.empty()));
  bool allThem    = ((!georel.empty()) && (!geometry.empty()) && (!coords.empty()));

  if (atLeastOne && !allThem)
  {
    return ERROR_DESC_BAD_REQUEST_PARTIAL_GEOEXPRESSION;
  }

  //
  // If geometry, coords and georel are filled, then attempt to create a geo filter
  // with them
  //
  if (allThem)
  {
    std::string  err;

    if (expr.geoFilter.fill(geometry, coords, georel, &err) != 0)
    {
      return "error parsing geo-query fields: " + err;
    }
  }

  return "OK";
}
