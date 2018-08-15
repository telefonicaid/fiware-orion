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
#include <stdio.h>                                   // printf

#include "orionld/rest/OrionLdRestService.h"         // OrionLdRestServiceVector
#include "orionld/rest/orionldServiceInitPresent.h"  // Own Interface



// -----------------------------------------------------------------------------
//
// orionldRestServiceV -
//
extern OrionLdRestServiceVector orionldRestServiceV[];



// -----------------------------------------------------------------------------
//
// orionldServiceInitPresent - 
//
void orionldServiceInitPresent(void)
{
  for (int svIx = 0; svIx < 9; svIx++)
  {
    OrionLdRestServiceVector* serviceV = &orionldRestServiceV[svIx];
    
    printf("%d REST Services for %s\n", serviceV->services, verbName((Verb) svIx));

    if (serviceV->services == 0)
      continue;

    for (int sIx = 0; sIx < serviceV->services; sIx++)
    {
      OrionLdRestService* serviceP = &serviceV->serviceV[sIx];

      printf("  %s %s\n", verbName((Verb) svIx), serviceP->url);
      printf("  Service routine at:           %p\n", serviceP->serviceRoutine);
      printf("  Wildcards:                    %d\n", serviceP->wildcards);
      printf("  charsBeforeFirstWildcard:     %d\n", serviceP->charsBeforeFirstWildcard);
      printf("  charsBeforeFirstWildcardSum:  %d\n", serviceP->charsBeforeFirstWildcardSum);
      printf("  charsBeforeSecondWildcard:    %d\n", serviceP->charsBeforeSecondWildcard);
      printf("  charsBeforeSecondWildcardSum: %d\n", serviceP->charsBeforeSecondWildcardSum);
      printf("  matchForSecondWildcard:       %s\n", serviceP->matchForSecondWildcard);
      printf("  matchForSecondWildcardLen:    %d\n", serviceP->matchForSecondWildcardLen);
      printf("  supportedVerbMask:            0x%x\n", serviceP->supportedVerbMask);
      printf("\n");
    }
  }
}
