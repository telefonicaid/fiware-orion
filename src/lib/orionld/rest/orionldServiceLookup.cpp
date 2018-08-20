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
#include "logMsg/logMsg.h"

#include "orionld/rest/OrionLdRestService.h"
#include "orionld/rest/orionldServiceLookup.h"



// -----------------------------------------------------------------------------
//
// requestPrepare -
//
// The cSumV values are interesting only for URL paths without wildcard of just uptil the first wildcard
// The longest URL path without/before first wildcard is "/ngsi-ld/v1/subscriptions/".
// The initial part ("/ngsi-ld/v1/") doesn't count, so ... 14 chars is all we need for the cSumV.
//
static void requestPrepare(char* url, int* cSumV, int* cSumsP, int* sLenP)
{
  // First of all, skip the first 12 characters in the URL path ("/ngsi-ld/v1/")
  url = &url[12];
  *cSumsP = 0;

  // Initialize counters with the first byte, then skip the first byte for the loop
  cSumV[0] = url[0];

  int sLen = 1;
  int ix   = 1;

  while ((url[ix] != 0) && (ix < 14))
  {
    cSumV[ix]  = cSumV[ix - 1] + url[ix];
    sLen      += 1;
    ++ix;
  }
  *cSumsP = ix;

  while (url[ix] != 0)
  {
    sLen += 1;
    ++ix;
  }

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
  int cSumV[14];
  int cSums;
  int sLen;

  LM_TMP(("Looking up service routine for %s %s", verbName(ciP->verb), ciP->urlPath));
  requestPrepare(ciP->urlPath, cSumV, &cSums, &sLen);
  LM_TMP(("Request prepared: strlen(%s): %d", ciP->urlPath, sLen));
  LM_TMP(("Request prepared: cSums: %d", cSums));
#if 0
  for (int ix = 0; ix < cSums; ix++)
    LM_TMP(("Request prepared: cSumV[%d]: %d", ix, cSumV[ix]));
#endif

  LM_TMP(("---------------- orionldServiceLookup ------------------"));
  LM_TMP(("%d Services for verb %s", serviceV->services, verbName(ciP->verb)));
  while (serviceIx < serviceV->services)
  {
    OrionLdRestService* serviceP = &serviceV->serviceV[serviceIx];

    LM_TMP(("Comparing incoming '%s' to service '%s' (%d wildcards)", ciP->urlPath, serviceP->url, serviceP->wildcards));

    if (serviceP->wildcards == 0)
    {
      LM_TMP(("No wildcard. Comparing strlens: %d vs %d", serviceP->charsBeforeFirstWildcard, sLen));
      if (serviceP->charsBeforeFirstWildcard == sLen)
      {
        LM_TMP(("No wildcard. strlens match. Comparing cSums[%d]: %d vs %d", sLen, serviceP->charsBeforeFirstWildcardSum, cSumV[sLen - 1]));
        if (serviceP->charsBeforeFirstWildcardSum == cSumV[sLen - 1])
        {
          LM_TMP(("Possible match. Not a complete strcmp of '%s' vs '%s'", &serviceP->url[12], &ciP->urlPath[12]));
          if (strcmp(&serviceP->url[12], &ciP->urlPath[12]) == 0)
          {
            LM_TMP(("******************* %s matches", serviceP->url));
            return serviceP;
          }
        }
      }
    }
    else if (serviceP->wildcards == 1)
    {
      LM_TMP(("One wildcard. Comparing strlens: %d vs %d", serviceP->charsBeforeFirstWildcard, sLen));
      
      if (serviceP->charsBeforeFirstWildcard < sLen)
      {
        LM_TMP(("One wildcard. strlens OK. Comparing charsBeforeFirstWildcardSum: %d vs %d", serviceP->charsBeforeFirstWildcardSum, cSumV[serviceP->charsBeforeFirstWildcard - 1]));
        if (serviceP->charsBeforeFirstWildcardSum == cSumV[serviceP->charsBeforeFirstWildcard - 1])
        {
          LM_TMP(("One wildcard. Possible match. Parcial strcmp of '%s' vs '%s' (%d bytes)", &serviceP->url[12], &ciP->urlPath[12], serviceP->charsBeforeFirstWildcard));
          if (strncmp(&serviceP->url[12], &ciP->urlPath[12], serviceP->charsBeforeFirstWildcard) == 0)
          {
            LM_TMP(("One wildcard. Possible match. "));
            // Ending the same?
            if (serviceP->matchForSecondWildcardLen != 0)  // An ending to match
            {
              int indexOfIncomingUrlPath = 12 + sLen - serviceP->matchForSecondWildcardLen;
              
              LM_TMP(("***************************************************************"));
              LM_TMP(("Comparing string ends"));
              LM_TMP(("Incoming URL Path: %s", ciP->urlPath));
              LM_TMP(("Being compared to: %s", serviceP->url));
              LM_TMP(("sLen == %d (strlen of incoming URL '%s'", sLen, ciP->urlPath));
              LM_TMP(("Number of chars to compare: %d", serviceP->matchForSecondWildcardLen));
              LM_TMP(("matchForSecondWildcardLen == %d", serviceP->matchForSecondWildcardLen));
              LM_TMP(("Comparing string ends: index %d for Incoming URL Path", indexOfIncomingUrlPath));
              LM_TMP(("Comparing string ends: string that the Service the Incoming URL Path is compared to: %s", serviceP->matchForSecondWildcard));
              LM_TMP(("Comparing string ends: Number of chars in the string: %d", serviceP->matchForSecondWildcardLen));
              LM_TMP(("Comparing string ends: End of Incoming URL Path: %s", &ciP->urlPath[indexOfIncomingUrlPath]));

              if (strncmp(&ciP->urlPath[indexOfIncomingUrlPath], serviceP->matchForSecondWildcard, serviceP->matchForSecondWildcardLen) == 0)
              {
                LM_TMP(("******************* %s matches", serviceP->url));

                ciP->wildcard[0] = &ciP->urlPath[serviceP->charsBeforeFirstWildcard + 12];
                
                // Destroying the incoming URL path, to extract the wildcard string
                ciP->urlPath[sLen - serviceP->matchForSecondWildcardLen + 12] = 0;
                LM_TMP(("WILDCARD:  '%s'", ciP->wildcard[0]));
                return serviceP;
              }
            }
            else
            {
              LM_TMP(("******************* %s matches", serviceP->url));
              ciP->wildcard[0] = &ciP->urlPath[serviceP->charsBeforeFirstWildcard + 12];
              LM_TMP(("WILDCARD:  '%s'", ciP->wildcard[0]));
              return serviceP;
            }
          }
        }
      }
    }
    else
    {
      LM_TMP(("Two wildcards. Comparing strlens: %d vs %d", serviceP->charsBeforeFirstWildcard, sLen));
      
      if (serviceP->charsBeforeFirstWildcard < sLen)
      {
        LM_TMP(("Two wildcards. strlens OK. Comparing charsBeforeFirstWildcardSum: %d vs %d", serviceP->charsBeforeFirstWildcardSum, cSumV[serviceP->charsBeforeFirstWildcard - 1]));
        if (serviceP->charsBeforeFirstWildcardSum == cSumV[serviceP->charsBeforeFirstWildcard - 1])
        {
          LM_TMP(("Two wildcards. Possible match. Finding '%s' inside '%s'", serviceP->matchForSecondWildcard, &ciP->urlPath[12]));
          char* matchP;
          if ((matchP = strstr(&ciP->urlPath[12], serviceP->matchForSecondWildcard)) != NULL)
          {
            LM_TMP(("Two wildcards. Found '%s' inside incoming URL - possible match", serviceP->matchForSecondWildcard));
            LM_TMP(("Second wildcard: %s", &matchP[serviceP->matchForSecondWildcardLen]));
            {
              LM_TMP(("******************* %s matches", serviceP->url));
              ciP->wildcard[0] = &ciP->urlPath[serviceP->charsBeforeFirstWildcard + 12];
              ciP->wildcard[1] = &matchP[serviceP->matchForSecondWildcardLen];

              // Destroying the incoming URL path, to extract first wildcard string
              *matchP = 0;
              LM_TMP(("First WILDCARD:  '%s'", ciP->wildcard[0]));
              LM_TMP(("Second WILDCARD: '%s'", ciP->wildcard[1]));

              return serviceP;
            }
          }
        }
      }
      LM_TMP(("No match"));
    }

    ++serviceIx;
  }
    
  return NULL;
}
