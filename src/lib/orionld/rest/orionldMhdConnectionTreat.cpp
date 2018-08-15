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
#include "rest/ConnectionInfo.h"                      // ConnectionInfo
#include "rest/restReply.h"                           // restReply

#include "orionld/serviceRoutines/orionldBadVerb.h"   // orionldBadVerb
#include "orionld/rest/orionldServiceInit.h"          // orionldRestServiceV 
#include "orionld/rest/orionldServiceLookup.h"        // orionldServiceLookup
#include "orionld/rest/temporaryErrorPayloads.h"      // Temporary Error Payloads
#include "orionld/rest/orionldMhdConnectionTreat.h"   // Own Interface



// -----------------------------------------------------------------------------
//
// orionldMhdConnectionTreat -
//
int orionldMhdConnectionTreat(ConnectionInfo* ciP)
{
  LM_TMP(("Read all the payload - treating the request!"));

  // If no error predetected, lookup the service and call it
  if (ciP->httpStatusCode == SccOk)
  {
    if ((ciP->verb == POST) || (ciP->verb == GET) || (ciP->verb == DELETE) || (ciP->verb == PATCH))
      ciP->serviceP = orionldServiceLookup(ciP, &orionldRestServiceV[ciP->verb]);

    if (ciP->serviceP != NULL)
    {
      LM_TMP(("Calling the service routine"));
      ciP->serviceP->serviceRoutine(ciP);
    }
    else
    {
      orionldBadVerb(ciP);
    }
  }


#if 0
  // Is there a KJSON response tree to render?
  if (ciP->jsonResponseTree != NULL)
    ciP->responsePayload = kjsonRender(ciP->jsonResponseTree);
#endif

  LM_TMP(("Responding"));
  ciP->outMimeType = JSON;  // ALL responses have payload
  
  if (ciP->responsePayload != NULL)
  {
    LM_TMP(("Responding with '%s'", ciP->responsePayload));
    restReply(ciP, ciP->responsePayload);
  }
  else
  {
    restReply(ciP, genericErrorPayload);
  }

  LM_TMP(("Read all the payload"));

  return MHD_YES;
}
