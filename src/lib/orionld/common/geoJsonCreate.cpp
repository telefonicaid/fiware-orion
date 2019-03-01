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
static bool pointCoordsGet(double* aLongP, double* aLatP, char** errorStringP)
{
  KjNode* aLongNodeP;
  KjNode* aLatNodeP;

  if (orionldState.geoCoordsP->type != KjArray)
  {
    LM_E(("The coordinates must be a JSON Array"));
    *errorStringP = (char*) "The coordinates must be a JSON Array";
    return false;
  }

  aLatNodeP  = orionldState.geoCoordsP->value.firstChildP;
  aLongNodeP = orionldState.geoCoordsP->value.firstChildP->next;

  if ((aLatNodeP == NULL) || (aLongNodeP == NULL) || (aLongNodeP->next != NULL))
  {
    LM_E(("The coordinates must be a JSON Array with TWO members"));
    *errorStringP = (char*) "The coordinates must be a JSON Array";
    return false;
  }

  if      (aLatNodeP->type == KjFloat)  *aLatP = aLatNodeP->value.f;
  else if (aLatNodeP->type == KjInt)    *aLatP = aLatNodeP->value.i;
  else
  {
    LM_E(("The coordinate members must be a Number, not a '%s'", kjValueType(orionldState.geoTypeP->type)));
    *errorStringP = (char*) "invalid JSON type for coordinate member";
    return false;
  }

  if      (aLongNodeP->type == KjFloat)  *aLongP = aLongNodeP->value.f;
  else if (aLongNodeP->type == KjInt)    *aLongP = aLongNodeP->value.i;
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

    LM_TMP(("Geo: Got a point!"));

    if (pointCoordsGet(&aLong, &aLat, errorStringP) == false)
    {
      LM_E(("coordsGet: %s", *errorStringP));
      return false;
    }

    LM_TMP(("Geo: Latitude:  %f", aLat));
    LM_TMP(("Geo: Longitude: %f", aLong));

    geoJsonP->append("type", "Point");
    geoJsonP->append("coordinates", BSON_ARRAY(aLong << aLat));
  }
  else
  {
    LM_E(("Unrecognized geometry: '%s'", orionldState.geoTypeP->value.s));
    *errorStringP = (char*) "Unrecognized geometry";
    return false;
  }

  return true;
}
