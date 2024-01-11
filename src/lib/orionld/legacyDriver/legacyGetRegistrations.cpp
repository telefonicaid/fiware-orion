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

#include "common/defaultValues.h"
#include "common/string.h"                                    // toString

#include "orionld/common/orionldState.h"                      // orionldState
#include "orionld/common/orionldError.h"                      // orionldError
#include "orionld/types/OrionldHeader.h"                      // orionldHeaderAdd
#include "orionld/mongoBackend/mongoLdRegistrationsGet.h"     // mongoLdRegistrationsGet
#include "orionld/legacyDriver/kjTreeFromRegistration.h"      // kjTreeFromRegistration
#include "orionld/legacyDriver/legacyGetRegistrations.h"      // Own Interface



// ----------------------------------------------------------------------------
//
// legacyGetRegistrations -
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
bool legacyGetRegistrations(void)
{
  std::vector<ngsiv2::Registration>  registrationVec;
  OrionError                         oe;
  long long                          count;

  if (!mongoLdRegistrationsGet(&registrationVec, orionldState.tenantP, &count, &oe))
  {
    orionldError(OrionldBadRequestData, "Bad Request", oe.details.c_str(), 400);
    return false;
  }

  if (orionldState.uriParams.count == true)
    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, count);

  orionldState.responseTree = kjArray(orionldState.kjsonP, NULL);

  for (unsigned int ix = 0; ix < registrationVec.size(); ix++)
  {
    KjNode* registrationNodeP = kjTreeFromRegistration(&registrationVec[ix]);
    kjChildAdd(orionldState.responseTree, registrationNodeP);
  }

  orionldState.httpStatusCode = 200;

  return true;
}
