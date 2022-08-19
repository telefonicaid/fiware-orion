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
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "common/statistics.h"
#include "common/clockFunctions.h"

#include "rest/ConnectionInfo.h"
#include "rest/HttpHeaders.h"
#include "ngsi/ParseData.h"
#include "serviceRoutinesV2/getEntityAllTypes.h"
#include "orionTypes/EntityTypeVectorResponse.h"
#include "mongoBackend/mongoQueryTypes.h"



/* ****************************************************************************
*
* getEntityAllTypes -
*
* GET /v2/types
*
* Payload In:  None
* Payload Out: EntityTypeVectorResponse
*
* URI parameters:
*   - options=noAttrDetail
*
*/
std::string getEntityAllTypes
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  EntityTypeVectorResponse  response;
  std::string               answer;
  unsigned int              totalTypes   = 0;
  bool                      noAttrDetail = ciP->uriParamOptions[OPT_NO_ATTR_DETAIL];
  unsigned int*             totalTypesP  = NULL;

  // NGSIv2 uses options=count to request count
  if (ciP->uriParamOptions[OPT_COUNT])
  {
    totalTypesP = &totalTypes;
  }

  if (ciP->uriParamOptions[OPT_VALUES])
  {
    TIMED_MONGO(mongoEntityTypesValues(&response, ciP->tenant, ciP->servicePathV, ciP->uriParam, totalTypesP));
  }
  else  // default
  {
    TIMED_MONGO(mongoEntityTypes(&response,
                                 ciP->tenant,
                                 ciP->servicePathV,
                                 ciP->uriParam,
                                 ciP->apiVersion,
                                 totalTypesP,
                                 noAttrDetail));
  }
  TIMED_RENDER(answer = response.toJson(ciP->uriParamOptions[OPT_VALUES]));

  if (ciP->uriParamOptions[OPT_COUNT])
  {
    char cVec[64];

    snprintf(cVec, sizeof(cVec), "%u", totalTypes);
    ciP->httpHeader.push_back(HTTP_FIWARE_TOTAL_COUNT);
    ciP->httpHeaderValue.push_back(cVec);
  }

  response.release();
  return answer;
}
