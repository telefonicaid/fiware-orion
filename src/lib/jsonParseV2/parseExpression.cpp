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
  ScopeVector*                 svP,
  ngsiv2::SubscriptionUpdate*  subsP
)
{
  if (!expression.IsObject())
  {
    return "expression is not an object";
  }

  if (expression.ObjectEmpty())
  {
    return "expression is empty";
  }

  if (subsP != NULL)
  {
    subsP->subject.condition.expression.isSet = true;
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

    if (subsP != NULL) {
      subsP->subject.condition.expression.q = qString;
    }

    std::string  errorString;
    Scope*       scopeP = new Scope(SCOPE_TYPE_SIMPLE_QUERY, qString);

    scopeP->stringFilterP = new StringFilter(SftQ);
    if (scopeP->stringFilterP->parse(scopeP->value.c_str(), &errorString) == false)
    {
      delete scopeP->stringFilterP;
      delete scopeP;

      return errorString;
    }

    svP->push_back(scopeP);
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

    if (subsP != NULL)
    {
      subsP->subject.condition.expression.mq = mqString;
    }

    std::string  errorString;
    Scope*       scopeP = new Scope(SCOPE_TYPE_SIMPLE_QUERY_MD, mqString);

    scopeP->mdStringFilterP = new StringFilter(SftMq);
    if (scopeP->mdStringFilterP->parse(scopeP->value.c_str(), &errorString) == false)
    {
      delete scopeP->mdStringFilterP;
      delete scopeP;

      return errorString;
    }

    svP->push_back(scopeP);
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

      if (subsP != NULL)
      {
        subsP->subject.condition.expression.geometry = geometry;
      }
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

      if (subsP != NULL)
      {
        subsP->subject.condition.expression.coords = coords;
      }
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

      if (subsP != NULL)
      {
        subsP->subject.condition.expression.georel = georel;
      }
    }
  }

  bool atLeastOne = ((!georel.empty()) || (!geometry.empty()) || (!coords.empty()));
  bool allThem    = ((!georel.empty()) && (!geometry.empty()) && (!coords.empty()));

  if (atLeastOne && !allThem)
  {
    return ERROR_DESC_BAD_REQUEST_PARTIAL_GEOEXPRESSION;
  }

  //
  // If geometry, coords and georel are filled, then attempt to create a filter scope
  // with them
  //
  if (allThem)
  {
    Scope*       scopeP = new Scope(SCOPE_TYPE_LOCATION, "");
    std::string  err;

    if (scopeP->fill(V2, geometry, coords, georel, &err) != 0)
    {
      delete scopeP;
      return "error parsing geo-query fields: " + err;
    }
    svP->push_back(scopeP);
  }

  return "OK";
}
