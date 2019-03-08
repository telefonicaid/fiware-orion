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
#include "common/errorMessages.h"
#include "common/string.h"

#include "apiTypesV2/Attribute.h"
#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "ngsi/ContextAttribute.h"
#include "rest/EntityTypeInfo.h"
#include "serviceRoutines/postQueryContext.h"
#include "serviceRoutinesV2/getEntityAttribute.h"
#include "serviceRoutinesV2/serviceRoutinesCommon.h"
#include "parse/forbiddenChars.h"
#include "rest/OrionError.h"
#include "rest/uriParamNames.h"



/* ****************************************************************************
*
* getEntityAttributeValue -
*
* GET /v2/entities/<id>/attrs/<attrName>/value
*
* Payload In:  None
* Payload Out: Entity Attribute
*
* URI parameters:
*   - options=keyValues
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
  Attribute    attribute;
  std::string  answer;
  std::string  type       = ciP->uriParam["type"];

  if (forbiddenIdChars(ciP->apiVersion,  compV[2].c_str(), NULL) ||
      (forbiddenIdChars(ciP->apiVersion, compV[4].c_str(), NULL)))
  {
    OrionError oe(SccBadRequest, ERROR_DESC_BAD_REQUEST_INVALID_CHAR_URI, ERROR_BAD_REQUEST);
    ciP->httpStatusCode = oe.code;
    return oe.toJson();
  }

  // Fill in QueryContextRequest
  parseDataP->qcr.res.fill(compV[2], type, "false", EntityTypeEmptyOrNotEmpty, "");

  // Call standard op postQueryContext
  OrionError oe;
  postQueryContext(ciP, components, compV, parseDataP);
  attribute.fill(parseDataP->qcrs.res, compV[4], &oe);

  if (oe.code != SccNone)
  {
    TIMED_RENDER(answer = oe.toJson());
    ciP->httpStatusCode = oe.code;
  }
  else
  {
    // save the original attribute type
    std::string attributeType = attribute.contextAttributeP->type;

    // the same of the wrapped operation
    ciP->httpStatusCode = parseDataP->qcrs.res.errorCode.code;

    // Remove unwanted fields from attribute before rendering
    attribute.contextAttributeP->type = "";
    attribute.contextAttributeP->metadataVector.release();

    if (ciP->outMimeType == JSON)
    {
      // Do not use attribute name, change to 'value'
      attribute.contextAttributeP->name = "value";

      StringList metadataFilter;
      setMetadataFilter(ciP->uriParam, &metadataFilter);

      TIMED_RENDER(answer = attribute.toJson(ciP->httpHeaders.accepted("text/plain"),
                                             ciP->httpHeaders.accepted("application/json"),
                                             ciP->httpHeaders.outformatSelect(),
                                             &(ciP->outMimeType),
                                             &(ciP->httpStatusCode),
                                             ciP->uriParamOptions[OPT_KEY_VALUES],
                                             metadataFilter.stringV,
                                             EntityAttributeValueRequest));
    }
    else
    {
      if (attribute.contextAttributeP->compoundValueP != NULL)
      {
        TIMED_RENDER(answer = attribute.contextAttributeP->compoundValueP->toJson());
      }
      else
      {
        if ((attributeType == DATE_TYPE) || (attributeType == DATE_TYPE_ALT))
        {
          TIMED_RENDER(answer = isodate2str(attribute.contextAttributeP->numberValue));
        }
        else
        {
          TIMED_RENDER(answer = attribute.contextAttributeP->getValue());
          if (attribute.contextAttributeP->valueType == orion::ValueTypeString)
          {
            answer = '"' + answer + '"';
          }
        }
      }
    }
  }

  // Cleanup and return result
  parseDataP->qcr.res.release();

  return answer;
}
