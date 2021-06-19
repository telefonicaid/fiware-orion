/*
*
* Copyright 2019 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin and Gabriel Quaresma
*/
#include <vector>                                             // std::vector

extern "C"
{
#include "kjson/KjNode.h"                                     // KjNode
#include "kjson/kjBuilder.h"                                  // kjObject, kjArray
#include "kbase/kStringSplit.h"                               // kStringSplit
}

#include "logMsg/logMsg.h"                                    // LM_*
#include "logMsg/traceLevels.h"                               // Lmt*

#include "common/defaultValues.h"
#include "common/string.h"                                    // toString
#include "rest/uriParamNames.h"                               // URI_PARAM_PAGINATION_OFFSET, URI_PARAM_PAGINATION_LIMIT
#include "rest/ConnectionInfo.h"                              // ConnectionInfo
#include "orionld/mongoBackend/mongoLdRegistrationsGet.h"     // mongoLdRegistrationsGet
#include "orionld/common/orionldState.h"                      // orionldState
#include "orionld/common/orionldErrorResponse.h"              // orionldErrorResponseCreate
#include "orionld/kjTree/kjTreeFromRegistration.h"            // kjTreeFromRegistration
#include "orionld/serviceRoutines/orionldGetRegistrations.h"  // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetRegistrations -
// URI params:
// - id
// - type
// - idPattern
// - attrs
// - q
// - csf
// - georel
// - geometry
// - coordinates
// - geoproperty
// - timeproperty
// - timerel
// - time
// - endTime
// - limit
// - offset
// - options=count
//
bool orionldGetRegistrations(ConnectionInfo* ciP)
{
  std::vector<ngsiv2::Registration>  registrationVec;
  OrionError                         oe;
  long long                          count;

  if (!mongoLdRegistrationsGet(&registrationVec, orionldState.tenantP, &count, &oe))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Bad Request", oe.details.c_str());
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  if (orionldState.uriParams.count == true)
  {
    ciP->httpHeader.push_back("NGSILD-Results-Count");
    ciP->httpHeaderValue.push_back(toString(count));
  }

  orionldState.responseTree = kjArray(orionldState.kjsonP, NULL);

  for (unsigned int ix = 0; ix < registrationVec.size(); ix++)
  {
    KjNode* registrationNodeP = kjTreeFromRegistration(ciP, &registrationVec[ix]);
    kjChildAdd(orionldState.responseTree, registrationNodeP);
  }

  orionldState.httpStatusCode = SccOk;

  return true;
}
