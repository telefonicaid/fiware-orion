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
#include <string.h>                                            // strcmp

extern "C"
{
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjRenderSize.h"                                // kjFastRenderSize
#include "kjson/kjRender.h"                                    // kjFastRender
}

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/dbModel/dbModelFromApiCoordinates.h"         // Own interface



// -----------------------------------------------------------------------------
//
// dbModelFromApiCoordinates -
//
bool dbModelFromApiCoordinates(KjNode* coordinatesP, const char* fieldName, KjNode* geometryP)
{
  bool   isPoint = false;
  char*  buf;

  if (geometryP == NULL)
  {
    orionldError(OrionldBadRequestData, "Internal Error", "Unable to extract the geometry of a geoQ for coordinmate APIv1 fix", 400);
    return false;
  }

  // Must be called "coords" in the database
  coordinatesP->name = (char*) "coords";

  // If already a String, then we're almost done - just need to remove the '[]'
  if (coordinatesP->type == KjString)
  {
    if (coordinatesP->value.s[0] == '[')
    {
      coordinatesP->value.s = &coordinatesP->value.s[1];
      char* endP = &coordinatesP->value.s[strlen(coordinatesP->value.s) - 1];
      *endP = 0;
    }

    return true;
  }

  if (strcmp(geometryP->value.s, "point") == 0)
    isPoint = true;

  if (isPoint)
  {
    // A point is an array ( [ 1, 2 ] ) in NGSI-LD, but in APIv1 database mode it is a string ( "1,2" )
    int    coords    = 0;
    float  coordV[3] = { 0, 0, 0 };
    bool   floats    = false;

    for (KjNode* coordP = coordinatesP->value.firstChildP; coordP != NULL; coordP = coordP->next)
    {
      if (coordP->type == KjFloat)
      {
        coordV[coords] = coordP->value.f;
        floats = true;
      }
      else
        coordV[coords] = (float) coordP->value.i;

      ++coords;
    }

    int bufSize = 128;
    buf = kaAlloc(&orionldState.kalloc, bufSize);

    if (floats == true)
    {
      if (coords == 2) snprintf(buf, bufSize, "%f,%f",    coordV[0], coordV[1]);
      else             snprintf(buf, bufSize, "%f,%f,%f", coordV[0], coordV[1], coordV[2]);
    }
    else
    {
      if (coords == 2) snprintf(buf, bufSize, "%d,%d",    (int) coordV[0], (int) coordV[1]);
      else             snprintf(buf, bufSize, "%d,%d,%d", (int) coordV[0], (int) coordV[1], (int) coordV[2]);
    }
  }
  else
  {
    int bufSize = kjFastRenderSize(coordinatesP);
    buf         = kaAlloc(&orionldState.kalloc, bufSize);
    kjFastRender(coordinatesP, buf);
  }

  coordinatesP->type    = KjString;
  coordinatesP->value.s = buf;

  return true;
}

