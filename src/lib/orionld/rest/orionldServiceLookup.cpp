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
OrionLdRestService* orionldServiceLookup(ConnectionInfo* ciP, OrionLdRestServiceVector* serviceV)
{
  int serviceIx = 0;
  int cSumV[MAX_CHARS_BEFORE_WILDCARD];
  int cSums;
  int sLen;

  LM_T(LmtServiceLookup, ("Looking up service routine for %s %s", verbName(ciP->verb), orionldState.urlPath));
  requestPrepare(orionldState.urlPath, cSumV, &cSums, &sLen);
  LM_T(LmtServiceLookup, ("Request prepared: strlen(%s): %d", orionldState.urlPath, sLen));
  LM_T(LmtServiceLookup, ("Request prepared: cSums: %d", cSums));

#if 0
  for (int ix = 0; ix < cSums; ix++)
    LM_T(LmtServiceLookup, ("Request prepared: cSumV[%d]: %d", ix, cSumV[ix]));
#endif

  LM_T(LmtServiceLookup, ("---------------- orionldServiceLookup ------------------"));
  LM_T(LmtServiceLookup, ("%d Services for verb %s", serviceV->services, verbName(ciP->verb)));
  while (serviceIx < serviceV->services)
  {
    OrionLdRestService* serviceP = &serviceV->serviceV[serviceIx];

    LM_T(LmtServiceLookup, ("Comparing incoming '%s' to service '%s' (%d wildcards)", orionldState.urlPath, serviceP->url, serviceP->wildcards));

    if (serviceP->wildcards == 0)
    {
      LM_T(LmtServiceLookup, ("No wildcard. Comparing strlens: %d vs %d", serviceP->charsBeforeFirstWildcard, sLen));
      if (serviceP->charsBeforeFirstWildcard == sLen)
      {
        LM_T(LmtServiceLookup, ("No wildcard. strlens match. Comparing cSums[%d]: %d vs %d", sLen, serviceP->charsBeforeFirstWildcardSum, cSumV[sLen - 1]));
        if (serviceP->charsBeforeFirstWildcardSum == cSumV[sLen - 1])
        {
          LM_T(LmtServiceLookup, ("Possible match. Not a complete strcmp of '%s' vs '%s'", &serviceP->url[ORION_LD_SERVICE_PREFIX_LEN], &orionldState.urlPath[ORION_LD_SERVICE_PREFIX_LEN]));
          if (strcmp(&serviceP->url[ORION_LD_SERVICE_PREFIX_LEN], &orionldState.urlPath[ORION_LD_SERVICE_PREFIX_LEN]) == 0)
          {
            LM_T(LmtServiceLookup, ("******************* %s matches", serviceP->url));
            return serviceP;
          }
        }
      }
    }
    else if (serviceP->wildcards == 1)
    {
      LM_T(LmtServiceLookup, ("One wildcard. Comparing strlens: %d vs %d", serviceP->charsBeforeFirstWildcard, sLen));

      if (serviceP->charsBeforeFirstWildcard < sLen)
      {
        LM_T(LmtServiceLookup, ("One wildcard. strlens OK. Comparing charsBeforeFirstWildcardSum: %d vs %d", serviceP->charsBeforeFirstWildcardSum, cSumV[serviceP->charsBeforeFirstWildcard - 1]));
        if (serviceP->charsBeforeFirstWildcardSum == cSumV[serviceP->charsBeforeFirstWildcard - 1])
        {
          LM_T(LmtServiceLookup, ("One wildcard. Possible match. Parcial strcmp of '%s' vs '%s' (%d bytes)", &serviceP->url[ORION_LD_SERVICE_PREFIX_LEN], &orionldState.urlPath[ORION_LD_SERVICE_PREFIX_LEN], serviceP->charsBeforeFirstWildcard));
          if (strncmp(&serviceP->url[ORION_LD_SERVICE_PREFIX_LEN], &orionldState.urlPath[ORION_LD_SERVICE_PREFIX_LEN], serviceP->charsBeforeFirstWildcard) == 0)
          {
            LM_T(LmtServiceLookup, ("One wildcard. Possible match. "));
            // Ending the same?
            if (serviceP->matchForSecondWildcardLen != 0)  // An ending to match
            {
              int indexOfIncomingUrlPath = ORION_LD_SERVICE_PREFIX_LEN + sLen - serviceP->matchForSecondWildcardLen;

              LM_T(LmtServiceLookup, ("***************************************************************"));
              LM_T(LmtServiceLookup, ("Comparing string ends"));
              LM_T(LmtServiceLookup, ("Incoming URL Path: %s", orionldState.urlPath));
              LM_T(LmtServiceLookup, ("Being compared to: %s", serviceP->url));
              LM_T(LmtServiceLookup, ("sLen == %d (strlen of incoming URL '%s'", sLen, orionldState.urlPath));
              LM_T(LmtServiceLookup, ("Number of chars to compare: %d", serviceP->matchForSecondWildcardLen));
              LM_T(LmtServiceLookup, ("matchForSecondWildcardLen == %d", serviceP->matchForSecondWildcardLen));
              LM_T(LmtServiceLookup, ("Comparing string ends: index %d for Incoming URL Path", indexOfIncomingUrlPath));
              LM_T(LmtServiceLookup, ("Comparing string ends: string that the Service the Incoming URL Path is compared to: %s", serviceP->matchForSecondWildcard));
              LM_T(LmtServiceLookup, ("Comparing string ends: Number of chars in the string: %d", serviceP->matchForSecondWildcardLen));
              LM_T(LmtServiceLookup, ("Comparing string ends: End of Incoming URL Path: %s", &orionldState.urlPath[indexOfIncomingUrlPath]));

              if (strncmp(&orionldState.urlPath[indexOfIncomingUrlPath], serviceP->matchForSecondWildcard, serviceP->matchForSecondWildcardLen) == 0)
              {
                LM_T(LmtServiceLookup, ("******************* %s matches", serviceP->url));

                orionldState.wildcard[0] = &orionldState.urlPath[serviceP->charsBeforeFirstWildcard + ORION_LD_SERVICE_PREFIX_LEN];

                // Destroying the incoming URL path, to extract the wildcard string
                orionldState.urlPath[sLen - serviceP->matchForSecondWildcardLen + ORION_LD_SERVICE_PREFIX_LEN] = 0;
                LM_T(LmtServiceLookup, ("WILDCARD:  '%s'", orionldState.wildcard[0]));
                return serviceP;
              }
            }
            else
            {
              LM_T(LmtServiceLookup, ("******************* %s matches", serviceP->url));
              orionldState.wildcard[0] = &orionldState.urlPath[serviceP->charsBeforeFirstWildcard + ORION_LD_SERVICE_PREFIX_LEN];
              LM_T(LmtServiceLookup, ("WILDCARD:  '%s'", orionldState.wildcard[0]));
              return serviceP;
            }
          }
        }
      }
    }
    else
    {
      LM_T(LmtServiceLookup, ("Two wildcards. Comparing strlens: %d vs %d", serviceP->charsBeforeFirstWildcard, sLen));

      if (serviceP->charsBeforeFirstWildcard < sLen)
      {
        LM_T(LmtServiceLookup, ("Two wildcards. strlens OK. Comparing charsBeforeFirstWildcardSum: %d vs %d", serviceP->charsBeforeFirstWildcardSum, cSumV[serviceP->charsBeforeFirstWildcard - 1]));
        if (serviceP->charsBeforeFirstWildcardSum == cSumV[serviceP->charsBeforeFirstWildcard - 1])
        {
          LM_T(LmtServiceLookup, ("Two wildcards. Possible match. Finding '%s' inside '%s'", serviceP->matchForSecondWildcard, &orionldState.urlPath[ORION_LD_SERVICE_PREFIX_LEN]));
          char* matchP;
          if ((matchP = strstr(&orionldState.urlPath[ORION_LD_SERVICE_PREFIX_LEN], serviceP->matchForSecondWildcard)) != NULL)
          {
            LM_T(LmtServiceLookup, ("Two wildcards. Found '%s' inside incoming URL - possible match", serviceP->matchForSecondWildcard));
            LM_T(LmtServiceLookup, ("Second wildcard: %s", &matchP[serviceP->matchForSecondWildcardLen]));
            {
              LM_T(LmtServiceLookup, ("******************* %s matches", serviceP->url));
              orionldState.wildcard[0] = &orionldState.urlPath[serviceP->charsBeforeFirstWildcard + ORION_LD_SERVICE_PREFIX_LEN];
              orionldState.wildcard[1] = &matchP[serviceP->matchForSecondWildcardLen];

              // Destroying the incoming URL path, to extract first wildcard string
              *matchP = 0;
              LM_T(LmtServiceLookup, ("First WILDCARD:  '%s'", orionldState.wildcard[0]));
              LM_T(LmtServiceLookup, ("Second WILDCARD: '%s'", orionldState.wildcard[1]));

              return serviceP;
            }
          }
        }
      }
      LM_T(LmtServiceLookup, ("No match"));
    }

    ++serviceIx;
  }

  return NULL;
}
