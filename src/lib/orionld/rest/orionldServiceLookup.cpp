/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/rest/OrionLdRestService.h"                   // OrionLdRestService, ORION_LD_SERVICE_PREFIX_LEN
#include "orionld/rest/orionldServiceLookup.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// requestPrepare -
//
// The cSumV values are interesting only for URL paths without wildcard of just uptil the first wildcard
// The longest URL path without/before first wildcard is "/ngsi-ld/v1/csourceRegistrations/".
// The initial part ("/ngsi-ld/") doesn't count, so ... 24 chars is all we need for the cSumV.
//
// strlen("/ngsi-ld/v1/entityOperations/delete")   == 35
// strlen("/ngsi-ld/")                           == 9   (ORION_LD_SERVICE_PREFIX_LEN)
//
//  35 - 9 == 26
//
#define MAX_CHARS_BEFORE_WILDCARD 26
static void requestPrepare(char* url, int* cSumV, int* cSumsP, int* sLenP)
{
  // First of all, skip the first 9 characters in the URL path ("/ngsi-ld/")
  url = &url[ORION_LD_SERVICE_PREFIX_LEN];
  *cSumsP = 0;

  // Initialize counters with the first byte, then skip the first byte for the loop
  cSumV[0] = url[0];

  int sLen;
  int ix   = 1;

  while ((url[ix] != 0) && (ix < MAX_CHARS_BEFORE_WILDCARD))
  {
    cSumV[ix]  = cSumV[ix - 1] + url[ix];
    ++ix;
  }
  *cSumsP = ix;

  while (url[ix] != 0)
  {
    ++ix;
  }

  sLen   = ix;
  *sLenP = sLen;
}



// -----------------------------------------------------------------------------
//
// orionldServiceLookup -
//
// The Verb must be a valid verb before calling this function (GET | POST | DELETE).
// This is assured by the function orionldMhdConnectionTreat()
//
OrionLdRestService* orionldServiceLookup(OrionLdRestServiceVector* serviceV)
{
  int serviceIx = 0;
  int cSumV[MAX_CHARS_BEFORE_WILDCARD];
  int cSums;
  int sLen;

  requestPrepare(orionldState.urlPath, cSumV, &cSums, &sLen);

  while (serviceIx < serviceV->services)
  {
    OrionLdRestService* serviceP = &serviceV->serviceV[serviceIx];

    if (serviceP->wildcards == 0)
    {
      if (serviceP->charsBeforeFirstWildcard == sLen)
      {
        if (serviceP->charsBeforeFirstWildcardSum == cSumV[sLen - 1])
        {
          if (strcmp(&serviceP->url[ORION_LD_SERVICE_PREFIX_LEN], &orionldState.urlPath[ORION_LD_SERVICE_PREFIX_LEN]) == 0)
          {
            return serviceP;
          }
        }
      }
    }
    else if (serviceP->wildcards == 1)
    {
      if (serviceP->charsBeforeFirstWildcard < sLen)
      {
        if (serviceP->charsBeforeFirstWildcardSum == cSumV[serviceP->charsBeforeFirstWildcard - 1])
        {
          if (strncmp(&serviceP->url[ORION_LD_SERVICE_PREFIX_LEN], &orionldState.urlPath[ORION_LD_SERVICE_PREFIX_LEN], serviceP->charsBeforeFirstWildcard) == 0)
          {
            // Ending the same?
            if (serviceP->matchForSecondWildcardLen != 0)  // An ending to match
            {
              int indexOfIncomingUrlPath = ORION_LD_SERVICE_PREFIX_LEN + sLen - serviceP->matchForSecondWildcardLen;

              if (strncmp(&orionldState.urlPath[indexOfIncomingUrlPath], serviceP->matchForSecondWildcard, serviceP->matchForSecondWildcardLen) == 0)
              {
                orionldState.wildcard[0] = &orionldState.urlPath[serviceP->charsBeforeFirstWildcard + ORION_LD_SERVICE_PREFIX_LEN];

                // Destroying the incoming URL path, to extract the wildcard string
                orionldState.urlPath[sLen - serviceP->matchForSecondWildcardLen + ORION_LD_SERVICE_PREFIX_LEN] = 0;
                return serviceP;
              }
            }
            else
            {
              orionldState.wildcard[0] = &orionldState.urlPath[serviceP->charsBeforeFirstWildcard + ORION_LD_SERVICE_PREFIX_LEN];
              return serviceP;
            }
          }
        }
      }
    }
    else
    {
      if (serviceP->charsBeforeFirstWildcard < sLen)
      {
        if (serviceP->charsBeforeFirstWildcardSum == cSumV[serviceP->charsBeforeFirstWildcard - 1])
        {
          char* matchP;
          if ((matchP = strstr(&orionldState.urlPath[ORION_LD_SERVICE_PREFIX_LEN], serviceP->matchForSecondWildcard)) != NULL)
          {
            {
              orionldState.wildcard[0] = &orionldState.urlPath[serviceP->charsBeforeFirstWildcard + ORION_LD_SERVICE_PREFIX_LEN];
              orionldState.wildcard[1] = &matchP[serviceP->matchForSecondWildcardLen];

              // Destroying the incoming URL path, to extract first wildcard string
              *matchP = 0;

              return serviceP;
            }
          }
        }
      }
    }

    ++serviceIx;
  }

  return NULL;
}
