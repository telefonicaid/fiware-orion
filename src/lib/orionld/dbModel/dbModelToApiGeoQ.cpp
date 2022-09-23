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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove, kjChildAdd
}

#include "logMsg/logMsg.h"                                       // LM

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/dbModel/dbModelToApiGeometry.h"                // dbModelToApiGeometry
#include "orionld/dbModel/dbModelToApiGeorel.h"                  // dbModelToApiGeorel
#include "orionld/dbModel/dbModelToApiCoordinates.h"             // dbModelToApiCoordinates
#include "orionld/dbModel/dbModelToApiGeoQ.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// dbModelToApiGeoQ -
//
bool dbModelToApiGeoQ(KjNode* geoqP, KjNode** coordinatesPP, bool* emptyP)
{
  KjNode* geometryP     = kjLookup(geoqP, "geometry");
  KjNode* georelP       = kjLookup(geoqP, "georel");
  KjNode* coordsP       = kjLookup(geoqP, "coords");
  KjNode* geopropertyP  = kjLookup(geoqP, "geoproperty");

  if (geometryP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error", "no /geometry/ field for geoQ found in database", 500);
    return false;
  }

  if (georelP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error", "no /georel/ field for geoQ found in database", 500);
    return false;
  }

  if (coordsP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error", "no /coordinates/ field for geoQ found in database", 500);
    return false;
  }

  if (geometryP->type != KjString)
  {
    orionldError(OrionldInternalError, "Database Error", "/geometry/ field for geoQ in database is not a String", 500);
    return false;
  }

  if (georelP->type != KjString)
  {
    orionldError(OrionldInternalError, "Database Error", "/georel/ field for geoQ in database is not a String", 500);
    return false;
  }

  if (geometryP->value.s[0] == 0)  // Empty geometry?
  {
    *emptyP = true;
    return true;
  }

  geometryP->value.s = (char*) dbModelToApiGeometry(geometryP->value.s);

  KjNode* newGeoRelNodeP = dbModelToApiGeorel(georelP->value.s);
  if (newGeoRelNodeP != NULL)
  {
    kjChildRemove(geoqP, georelP);
    kjChildAdd(geoqP, newGeoRelNodeP);
  }


  if (coordsP->type == KjString)
  {
    kjChildRemove(geoqP, coordsP);
    coordsP = dbModelToApiCoordinates(coordsP->value.s);
    kjChildAdd(geoqP, coordsP);
  }
  else if (coordsP->type == KjArray)
    coordsP->name = (char*) "coordinates";
  else
  {
    orionldError(OrionldInternalError, "Database Error", "/coordinates/ field for geoQ in database is not an Array nor a String", 500);
    return false;
  }

  if (geopropertyP != NULL)
  {
    eqForDot(geopropertyP->value.s);
    geopropertyP->value.s = orionldContextItemAliasLookup(orionldState.contextP, geopropertyP->value.s, NULL, NULL);
  }

  *coordinatesPP = coordsP;

  return true;
}
