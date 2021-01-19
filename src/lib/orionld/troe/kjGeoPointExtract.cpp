/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*


#include "orionld/troe/kjGeoPointExtract.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// kjGeoPointExtract -
//
bool kjGeoPointExtract(KjNode* coordinatesP, double* longitudeP, double* latitudeP, double* altitudeP)
{
  // Count children, make sure there are two or three and that all of them are Int or Float
  int     children = 0;
  KjNode* childP   = coordinatesP->value.firstChildP;

  while (childP != NULL)
  {
    ++children;

    if ((childP->type != KjFloat) && (childP->type != KjInt))
      LM_RE(false, ("Bad Input (an item of 'coordinates' is not a Number)"));

    childP = childP->next;
  }

  if ((children < 2) || (children > 3))
    LM_RE(false, ("Bad Input (invalid num,ber of coordinates: %d)", children));

  KjNode* longitudeNodeP = coordinatesP->value.firstChildP;
  KjNode* latitudeNodeP  = longitudeNodeP->next;
  KjNode* altitudeNodeP  = latitudeNodeP->next;

  if (longitudeNodeP->type == KjFloat)
    *longitudeP = longitudeNodeP->value.f;
  else
    *longitudeP = longitudeNodeP->value.i;

  if (latitudeNodeP->type == KjFloat)
    *latitudeP = latitudeNodeP->value.f;
  else
    *latitudeP = latitudeNodeP->value.i;

  if (altitudeNodeP != NULL)
  {
    if (altitudeNodeP->type == KjFloat)
      *altitudeP = altitudeNodeP->value.f;
    else
      *altitudeP = altitudeNodeP->value.i;
  }
  else
    *altitudeP = 0;

  return true;
}
