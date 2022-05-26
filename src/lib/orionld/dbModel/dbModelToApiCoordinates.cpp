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
#include <string.h>                                              // strlen

extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjParse.h"                                       // kjParse
#include "kjson/kjBuilder.h"                                     // kjString
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/dbModel/dbModelToApiCoordinates.h"             // Own interface



// -----------------------------------------------------------------------------
//
// dbModelToApiCoordinates -
//
KjNode* dbModelToApiCoordinates(const char* coordinatesString)
{
  char*    coordinatesP         = (char*) coordinatesString;
  KjNode*  coordValueP;

  //
  // The "coords" are stored as a STRING in orion's database ...
  // This here is a hack to make the string an array
  // and then parse it using kjParse and get a KjNode tree
  //
  if (coordinatesString[0] != '[')
  {
    int      coordinatesVectorLen = strlen(coordinatesString) * 4 + 10;
    char*    coordinatesVector    = kaAlloc(&orionldState.kalloc, coordinatesVectorLen);

    snprintf(coordinatesVector, coordinatesVectorLen, "[%s]", coordinatesString);
    coordinatesP = coordinatesVector;
  }

  coordValueP = kjParse(orionldState.kjsonP, coordinatesP);
  if (coordValueP == NULL)
    coordValueP = kjString(orionldState.kjsonP, "coordinates", "internal error parsing DB coordinates");
  else
    coordValueP->name = (char*) "coordinates";

  return coordValueP;
}
