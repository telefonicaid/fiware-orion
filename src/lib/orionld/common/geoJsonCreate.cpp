/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "mongo/client/dbclient.h"                             // BSONObjBuilder

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/OrionldConnection.h"                  // orionldState
#include "orionld/common/geoJsonCreate.h"                      // Own interface



// -----------------------------------------------------------------------------
//
// pointCoordsGet -
//
static bool pointCoordsGet(KjNode* coordsNodeP, double* aLongP, double* aLatP, char** errorStringP)
{
  KjNode* aLatitudeNodeP;
  KjNode* aLongitudeNodeP;

  if (coordsNodeP->type != KjArray)
  {
    LM_E(("The coordinates must be a JSON Array"));
    *errorStringP = (char*) "The coordinates must be a JSON Array";
    return false;
  }

  aLongitudeNodeP  = coordsNodeP->value.firstChildP;
  aLatitudeNodeP   = coordsNodeP->value.firstChildP->next;

  if ((aLongitudeNodeP == NULL) || (aLatitudeNodeP == NULL) || (aLatitudeNodeP->next != NULL))
  {
    LM_E(("The coordinates must be a JSON Array with TWO members"));
    *errorStringP = (char*) "The coordinates must be a JSON Array";
    return false;
  }

  if      (aLongitudeNodeP->type == KjFloat)  *aLongP = aLongitudeNodeP->value.f;
  else if (aLongitudeNodeP->type == KjInt)    *aLongP = aLongitudeNodeP->value.i;
  else
  {
    LM_E(("The coordinate members must be a Number, not a '%s'", kjValueType(orionldState.geoTypeP->type)));
    *errorStringP = (char*) "invalid JSON type for coordinate member";
    return false;
  }

  if      (aLatitudeNodeP->type == KjFloat)  *aLatP = aLatitudeNodeP->value.f;
  else if (aLatitudeNodeP->type == KjInt)    *aLatP = aLatitudeNodeP->value.i;
  else
  {
    LM_E(("The coordinate members must be a Number, not a '%s'", kjValueType(orionldState.geoTypeP->type)));
    *errorStringP = (char*) "invalid JSON type for coordinate member";
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// geoJsonCreate -
//
bool geoJsonCreate(KjNode* attrP, mongo::BSONObjBuilder* geoJsonP, char** errorStringP)
{
  if ((orionldState.geoTypeP == NULL) || (orionldState.geoCoordsP == NULL))
  {
    LM_E(("fields missing in GEO attribute value: geoTypeP=%p, geoCoordsP=%p", orionldState.geoTypeP, orionldState.geoCoordsP));
    *errorStringP = (char*) "fields missing in GEO attribute value";
    return false;
  }

  if (strcmp(orionldState.geoTypeP->value.s, "Point") == 0)
  {
    double  aLat;
    double  aLong;

    if (pointCoordsGet(orionldState.geoCoordsP, &aLong, &aLat, errorStringP) == false)
    {
      LM_E(("coordsGet: %s", *errorStringP));
      return false;
    }

    geoJsonP->append("type", "Point");
    geoJsonP->append("coordinates", BSON_ARRAY(aLong << aLat));
  }
  else if (strcmp(orionldState.geoTypeP->value.s, "Polygon") == 0)
  {
    //
    // In its simplest case, a polygon is a vector of vector of positio-vectors. E.g.:
    //   [[ [0,0], [0,6], [-4,6], [-4,0], [0,0] ]]
    // The points that form the polygon must go in counter-clockwise direction and the last
    // point must be identical to the first, to close the polygon.
    //
    // If clockwise direction, a HOLE is formed.
    // FIXME: This is a bit more complex and not implemented in the first "round".
    // To make a big rectangle { 0,0 -> -4,6 } with a hole in it { -1,1 -> -2,2 }:
    //
    // [[ [0,0], [0,6], [-4,6], [-4,0], [0,0] ], [ [-1,1], [-1,2], [-2,2], [-2,0], [0,0], ]]
    //

    // First level
    if (orionldState.geoCoordsP->type != KjArray)
    {
      LM_E(("The coordinates must be a JSON Array in the first level of a Polygon"));
      *errorStringP = (char*) "The coordinates must be a JSON Array";
      return false;
    }

    // Second level
    KjNode* l2P = orionldState.geoCoordsP->value.firstChildP;
    if (l2P->type != KjArray)
    {
      LM_E(("The coordinates must be a JSON Array in the second level of a Polygon"));
      *errorStringP = (char*) "The coordinates must be a JSON Array";
      return false;
    }

    //
    // More than one linear ring is not implemented yet => l2P->next must be NULL
    //
    if (l2P->next != NULL)
    {
      LM_E(("More than one linear ring is not implemented"));
      *errorStringP = (char*) "More than one linear ring is not implemented";
      return false;
    }

    // Third level
    mongo::BSONArrayBuilder  ba;

    for (KjNode* l3P = l2P->value.firstChildP; l3P != NULL; l3P = l3P->next)
    {
      double aLat;
      double aLong;

      if (pointCoordsGet(l3P, &aLong, &aLat, errorStringP) == false)
      {
        LM_E(("error extracting coordinates from third level of polygon: %s", *errorStringP));
        *errorStringP = (char*) "error extracting coordinates from third level of polygon";
        return false;
      }

      ba.append(BSON_ARRAY(aLong << aLat));
    }

    geoJsonP->append("type", "Polygon");
    geoJsonP->append("coordinates", BSON_ARRAY(ba.arr()));
  }
  else
  {
    LM_E(("Unrecognized geometry: '%s'", orionldState.geoTypeP->value.s));
    *errorStringP = (char*) "Unrecognized geometry";
    return false;
  }

  return true;
}
