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

  LM_T(LmtRestriction, ("In Restriction::check"));
  if (counter == 0)  // Restriction is always optional
  {
    LM_T(LmtRestriction, ("Restriction::check returns OK (counter == 0)"));
    return "OK";
  }

  if (scopeVector.size() == 0)
  {
    alarmMgr.badInput(clientIp, "empty restriction");
    return "empty restriction";
  }

  if ((res = scopeVector.check()) != "OK")
  {
    LM_T(LmtRestriction, ("Restriction::check returns '%s'", res.c_str()));
    alarmMgr.badInput(clientIp, res);

    return res;
  }

  LM_T(LmtRestriction, ("Restriction::check returns OK (2)"));
  return "OK";
}



/* ****************************************************************************
*
* Restriction::release -
*/
void Restriction::release(void)
{
  scopeVector.release();
}



/* ****************************************************************************
*
* Restriction::fill - 
*/
void Restriction::fill(Restriction* rP)
{
  for (unsigned int ix = 0; ix < rP->scopeVector.size(); ++ix)
  {
    scopeVector.push_back(new Scope(rP->scopeVector[ix]->type, rP->scopeVector[ix]->value, rP->scopeVector[ix]->oper));
  }
}
