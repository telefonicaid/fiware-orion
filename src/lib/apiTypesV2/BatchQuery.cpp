/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <vector>

#include "alarmMgr/alarmMgr.h"
#include "rest/OrionError.h"
#include "apiTypesV2/BatchQuery.h"



/* ****************************************************************************
*
* BatchQuery::BatchQuery - 
*/
BatchQuery::BatchQuery()
{
}



/* ****************************************************************************
*
* BatchQuery::~BatchQuery - 
*/
BatchQuery::~BatchQuery()
{
}



/* ****************************************************************************
*
* BatchQuery::check - 
*/
std::string BatchQuery::check(ConnectionInfo* ciP, RequestType requestType)
{
  std::string res;
  std::string err;

  if (((res = entities.check(ciP, requestType))          != "OK") ||
      ((res = attributeV.check(requestType, "", err, 0)) != "OK") ||
      ((res = scopeV.check(requestType, "", err, 0))     != "OK"))
  {
    std::string error = res;

    if (err != "")
    {
      error += ": ";
      error += err;
    }

    OrionError oe(SccBadRequest, res);

    alarmMgr.badInput(clientIp, error);
    return oe.render();
  }

  return "OK";
}



/* ****************************************************************************
*
* BatchQuery::present - 
*/
void BatchQuery::present(const std::string& indent)
{
  entities.present(indent);
  attributeV.present(indent);
  scopeV.present(indent);
}



/* ****************************************************************************
*
* BatchQuery::release - 
*/
void BatchQuery::release(void)
{
  entities.release();
  attributeV.release();
  scopeV.release();
}
 
