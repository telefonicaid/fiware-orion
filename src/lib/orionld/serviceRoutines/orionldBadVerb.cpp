/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldState.h"                       // orionldState, orionldStateInit
#include "orionld/rest/temporaryErrorPayloads.h"               // notFoundPayload
#include "orionld/rest/orionldServiceInit.h"                   // orionldRestServiceV
#include "orionld/rest/orionldServiceLookup.h"                 // orionldServiceLookup
#include "orionld/serviceRoutines/orionldBadVerb.h"            // Own Interface



// -----------------------------------------------------------------------------
//
// orionldBadVerb -
//
bool orionldBadVerb(ConnectionInfo* ciP)
{
  uint16_t  bitmask = 0;
  bool      found   = false;

  LM_T(LmtBadVerb, ("PATH: %s", orionldState.urlPath));
  LM_T(LmtBadVerb, ("VERB: %s", orionldState.verbString));

  //
  // There are nine verbs/methods, but only GET, POST, PATCH and DELETE are supported by ORIONLD
  // This loop looks up the URL PATH for each "orionld-valid" verb and keeps a bitmask of the hits
  //
  for (uint16_t verbNo = 0; verbNo <= PATCH; verbNo++)  // 0:GET, 1:PUT, 2:POST, 3:DELETE, 4:PATCH
  {
    if (verbNo == PUT) continue;

    if (orionldServiceLookup(ciP, &orionldRestServiceV[verbNo]) != NULL)
    {
      bitmask |= (1 << verbNo);
      found = true;
    }
  }

  if (found == false)
    return false;

  char allowValue[128];

  allowValue[0] = 0;

  if (bitmask & (1 << GET))    strcat(allowValue, ",GET");
  if (bitmask & (1 << POST))   strcat(allowValue, ",POST");
  if (bitmask & (1 << DELETE)) strcat(allowValue, ",PATCH");
  if (bitmask & (1 << PATCH))  strcat(allowValue, ",DELETE");

  ciP->httpHeader.push_back(HTTP_ALLOW);
  ciP->httpHeaderValue.push_back(&allowValue[1]);  // Skipping first comma

  return true;
}
