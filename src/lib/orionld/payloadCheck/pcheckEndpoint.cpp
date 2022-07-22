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
* Author: Ken Zangelin
*/
extern "C"
{
#include "kjson/KjNode.h"                                       // KjNode
}

#include "logMsg/logMsg.h"                                      // LM_*
#include "logMsg/traceLevels.h"                                 // Lmt*

#include "orionld/common/CHECK.h"                               // STRING_CHECK, ...
#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/payloadCheck/pcheckReceiverInfo.h"            // pcheckReceiverInfo
#include "orionld/payloadCheck/pcheckNotifierInfo.h"            // pcheckNotifierInfo
#include "orionld/payloadCheck/pcheckEndpoint.h"                // Own interface


// ----------------------------------------------------------------------------
//
// pcheckEndpoint -
//
bool pcheckEndpoint(KjNode* endpointP, bool patch, KjNode** uriPP, KjNode** notifierInfoPP, bool* mqttChangeP)
{
  KjNode* uriP           = NULL;
  KjNode* acceptP        = NULL;
  KjNode* receiverInfoP  = NULL;
  KjNode* notifierInfoP  = NULL;

  for (KjNode* epItemP = endpointP->value.firstChildP; epItemP != NULL; epItemP = epItemP->next)
  {
    if (strcmp(epItemP->name, "uri") == 0)
    {
      DUPLICATE_CHECK(uriP, "endpoint::uri", epItemP);
      STRING_CHECK(uriP, "endpoint::uri");
      URI_CHECK(uriP->value.s, "endpoint::uri", true);

      if (strncmp(uriP->value.s, "mqtt", 4) == 0)
        *mqttChangeP = true;
    }
    else if (strcmp(epItemP->name, "accept") == 0)
    {
      DUPLICATE_CHECK(acceptP, "endpoint::accept", epItemP);
      STRING_CHECK(acceptP, "endpoint::accept");
      EMPTY_STRING_CHECK(acceptP, "endpoint::accept");
      if ((strcmp(acceptP->value.s, "application/json") != 0) && (strcmp(acceptP->value.s, "application/ld+json") != 0) && (strcmp(acceptP->value.s, "application/geo+json") != 0))
      {
        orionldError(OrionldBadRequestData, "Unsupported Mime-type in 'accept'", epItemP->value.s, 400);
        return false;
      }
    }
    else if (strcmp(epItemP->name, "receiverInfo") == 0)
    {
      DUPLICATE_CHECK(receiverInfoP, "receiverInfo", epItemP);
      ARRAY_CHECK(receiverInfoP, "receiverInfo");
      EMPTY_ARRAY_CHECK(receiverInfoP, "receiverInfo");
      if (pcheckReceiverInfo(receiverInfoP) == false)
        return false;
    }
    else if (strcmp(epItemP->name, "notifierInfo") == 0)
    {
      DUPLICATE_CHECK(notifierInfoP, "notifierInfo", epItemP);
      ARRAY_CHECK(notifierInfoP, "notifierInfo");
      EMPTY_ARRAY_CHECK(notifierInfoP, "notifierInfo");
      if (pcheckNotifierInfo(notifierInfoP, mqttChangeP) == false)
        return false;
    }
    else
    {
      orionldError(OrionldBadRequestData, "Invalid field for 'endpoint'", epItemP->name, 400);
      return false;
    }
  }

  if ((patch == false) && (uriP == NULL))
  {
    orionldError(OrionldBadRequestData, "Mandatory field missing in 'endpoint'", "uri", 400);
    return false;
  }

  *uriPP          = uriP;
  *notifierInfoPP = notifierInfoP;

  return true;
}
