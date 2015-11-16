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
* Author: Orion dev team
*/
#include <string>
#include <vector>

#include "common/statistics.h"
#include "common/clockFunctions.h"

#include "apiTypesV2/Attribute.h"
#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutines/postQueryContext.h"
#include "serviceRoutinesV2/getEntityAttribute.h"



/* ****************************************************************************
*
* getEntityAttributeValue -
*
* GET /v2/entities/:id:/attrs/:attrName:
*
* Payload In:  None
* Payload Out: Entity Attribute
*
*/
std::string getEntityAttributeValue
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  std::string  answer;
  Attribute    attribute;
  bool         text = (ciP->uriParam["options"] == "text" || ciP->outFormat == TEXT);

  // Fill in QueryContextRequest
  parseDataP->qcr.res.fill(compV[2], "", "false", EntityTypeEmptyOrNotEmpty, "");

  // Call standard op postQueryContext
  postQueryContext(ciP, components, compV, parseDataP);

  attribute.fill(&parseDataP->qcrs.res, compV[4]);

  // Render entity attribute response
  if (attribute.errorCode.error == "TooManyResults")
  {
    ciP->httpStatusCode = SccConflict;

    TIME_STAT_RENDER_START();
    answer = attribute.render(ciP, EntityAttributeResponse);
    TIME_STAT_RENDER_STOP();
  }
  else if (attribute.errorCode.error == "NotFound")
  {
    ciP->httpStatusCode = SccContextElementNotFound;

    TIME_STAT_RENDER_START();
    answer = attribute.render(ciP, EntityAttributeResponse);
    TIME_STAT_RENDER_STOP();
  }
  else
  {
    // the same of the wrapped operation
    ciP->httpStatusCode = parseDataP->qcrs.res.errorCode.code;

    // Remove unwanted fields from attribute before rendering
    attribute.pcontextAttribute->type = "";
    attribute.pcontextAttribute->metadataVector.release();

    if (!text)
    {
      // Do not use attribute name, change to 'value'
      attribute.pcontextAttribute->name = "value";

      TIME_STAT_RENDER_START();
      answer = attribute.render(ciP, EntityAttributeResponse);
      TIME_STAT_RENDER_STOP();
    }
    else
    {
      if (attribute.pcontextAttribute->compoundValueP != NULL)
      {
        TIME_STAT_RENDER_START();

        answer = attribute.pcontextAttribute->compoundValueP->render(ciP, JSON, "");
        if (attribute.pcontextAttribute->compoundValueP->isObject())
        {
          answer = "{" + answer + "}";
        }
        else if (attribute.pcontextAttribute->compoundValueP->isVector())
        {
          answer = "[" + answer + "]";
        }

        TIME_STAT_RENDER_STOP();
      }
      else
      {
        TIME_STAT_RENDER_START();
        answer = attribute.pcontextAttribute->toStringValue();
        TIME_STAT_RENDER_STOP();
      }

      ciP->outFormat = TEXT;
    }
 }

  // Cleanup and return result
  parseDataP->qcr.res.release();

  return answer;
}
