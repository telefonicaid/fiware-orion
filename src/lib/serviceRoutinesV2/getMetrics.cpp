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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "alarmMgr/alarmMgr.h"
#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/rest.h"
#include "metricsMgr/metricsMgr.h"
#include "serviceRoutinesV2/getMetrics.h"


// FIXME PoC: I know, this is not the place for this :) A new file should be created for the servicer routine
// However, this is a PoC and for the sake of simplificy I have chosen a random existing serviceRouting file
std::string getNgsiTest
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  LM_W(("PoC: GET /ngsi-ld/v1/entities/{}/attrs/{} received"));
  LM_W(("PoC:   the entity is:    %s", compV[3].c_str()));
  LM_W(("PoC:   the attribute is: %s", compV[5].c_str()));
  return "OK";
}


/* ****************************************************************************
*
* getMetrics -
*
* GET /admin/metrics
*
* URI parameters:
*   - reset
*/
std::string getMetrics
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  if (!metricsMgr.isOn())
  {
    OrionError oe(SccBadRequest, "metrics desactivated");

    ciP->httpStatusCode = SccBadRequest;
    return oe.toJson();
  }

  bool         doReset  = (ciP->uriParam["reset"] == "true")? true : false;
  std::string  payload  = metricsMgr.toJson(doReset);

  return payload;
}
