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
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/Request.h"
#include "ngsi/Restriction.h"



/* ****************************************************************************
*
* Restriction::check -
*/
std::string Restriction::check(int counter)
{
  std::string res;

  if (counter == 0)  // Restriction is always optional
  {
    return "OK";
  }

  if ((scopeVector.size() == 0) && (attributeExpression.isEmpty()))
  {
    alarmMgr.badInput(clientIp, "empty restriction");
    return "empty restriction";
  }

  if (((res = scopeVector.check())         != "OK") ||
      ((res = attributeExpression.check()) != "OK"))
  {
    alarmMgr.badInput(clientIp, res);

    return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* Restriction::render -
*/
std::string Restriction::render(int restrictions, bool comma)
{
  std::string  tag = "restriction";
  std::string  out = "";
  bool         scopeVectorRendered = scopeVector.size() != 0;

  if (restrictions == 0)
  {
    return "";
  }

  out += startTag(tag);
  out += attributeExpression.render(scopeVectorRendered);
  out += scopeVector.render(false);
  out += endTag(comma);

  return out;
}



/* ****************************************************************************
*
* Restriction::release -
*/
void Restriction::release(void)
{
  attributeExpression.release();
  scopeVector.release();
}



/* ****************************************************************************
*
* Restriction::fill - 
*/
void Restriction::fill(Restriction* rP)
{
  const std::string ae = rP->attributeExpression.get();
  attributeExpression.set(ae);

  for (unsigned int ix = 0; ix < rP->scopeVector.size(); ++ix)
  {
    scopeVector.push_back(new Scope(rP->scopeVector[ix]->type, rP->scopeVector[ix]->value, rP->scopeVector[ix]->oper));
  }
}
