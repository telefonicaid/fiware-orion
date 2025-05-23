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
* Author: Fermin Galan
*/
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/RenderFormat.h"
#include "common/JsonHelper.h"
#include "ngsi10/NotifyContextRequest.h"
#include "ngsi10/NotifyContextResponse.h"
#include "rest/OrionError.h"
#include "alarmMgr/alarmMgr.h"



/* ****************************************************************************
*
* NotifyContextRequest::toJson -
*/
std::string NotifyContextRequest::toJson
(
  RenderFormat                        renderFormat,
  const std::vector<std::string>&     attrsFilter,
  bool                                blacklist,
  const std::vector<std::string>&     metadataFilter,
  ExprContextObject*                  exprContextObjectP
)
{
  if ((renderFormat != NGSI_V2_NORMALIZED) && (renderFormat != NGSI_V2_KEYVALUES) && (renderFormat != NGSI_V2_VALUES) && (renderFormat != NGSI_V2_SIMPLIFIEDKEYVALUES) && (renderFormat != NGSI_V2_SIMPLIFIEDNORMALIZED))
  {
    OrionError oe(SccBadRequest, "Invalid notification format");
    alarmMgr.badInput(clientIp, "Invalid notification format");

    return oe.toJson();
  }
  else if (renderFormat == NGSI_V2_SIMPLIFIEDNORMALIZED)
  {
    if (contextElementResponseVector.size() == 0)
    {
      LM_E(("Runtime Error (contextElementResponser MUST NOT be zero length)"));
      return "{}";
    }
    else
    {
      std::string out;
      out += contextElementResponseVector[0]->toJson(NGSI_V2_NORMALIZED, attrsFilter, blacklist, metadataFilter, exprContextObjectP);
      return out;
    }
  }
  else if (renderFormat == NGSI_V2_SIMPLIFIEDKEYVALUES)
  {
    if (contextElementResponseVector.size() == 0)
    {
      LM_E(("Runtime Error (contextElementResponser MUST NOT be zero length)"));
      return "{}";
    }
    else
    {
      std::string out;
      out += contextElementResponseVector[0]->toJson(NGSI_V2_KEYVALUES, attrsFilter, blacklist, metadataFilter, exprContextObjectP);
      return out;
    }
  }
  else
  {
    JsonObjectHelper jh;

    jh.addString("subscriptionId", subscriptionId);
    jh.addRaw("data", contextElementResponseVector.toJson(renderFormat, attrsFilter, blacklist, metadataFilter, exprContextObjectP));
    return jh.str();
  }
}



/* ****************************************************************************
*
* NotifyContextRequest::release -
*
*/
void NotifyContextRequest::release(void)
{
  contextElementResponseVector.release();
}

