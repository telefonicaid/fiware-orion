/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include <stdio.h>                                               // snprintf
#include <string.h>                                              // strstr
#include <stdlib.h>                                              // atoi

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString
}

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/dbModel/dbModelToApiGeorel.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// dbModelToApiGeorel -
//
KjNode* dbModelToApiGeorel(char* georel)
{
  char* newGeorel = NULL;
  char* distanceP = strstr(georel, "Distance:");
  char  adjustedGeorelBuf[32];

  if (distanceP != NULL)
  {
    int distance = atoi(&distanceP[9]);

    distanceP[8] = 0;  // NULL out the ':' inside "georel"
    snprintf(adjustedGeorelBuf, sizeof(adjustedGeorelBuf) - 1, "%s==%d", georel, distance);
    newGeorel = adjustedGeorelBuf;

    KjNode* georelP = kjString(orionldState.kjsonP, "georel", newGeorel);  // Here the new BIGGER string is allocated
    return georelP;
  }

  return NULL;  // georel is OK - nothing needs to change
}
