/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include "kjson/KjNode.h"                                         // KjNode
#include "kjson/kjRenderSize.h"                                   // kjFastRenderSize
#include "kjson/kjRender.h"                                       // kjFastRender
}

#include "apiTypesV2/SubscriptionExpression.h"                    // SubscriptionExpression

#include "orionld/common/orionldState.h"                          // orionldState
#include "orionld/common/orionldError.h"                          // orionldError
#include "orionld/common/CHECK.h"                                 // CHECKx()
#include "orionld/common/SCOMPARE.h"                              // SCOMPAREx
#include "orionld/dbModel/dbModelFromGeometry.h"                  // dbModelFromGeometry
#include "orionld/dbModel/dbModelFromGeorel.h"                    // dbModelFromGeorel
#include "orionld/legacyDriver/kjTreeToSubscriptionExpression.h"  // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeToSubscriptionExpression -
//
bool kjTreeToSubscriptionExpression(KjNode* kNodeP, SubscriptionExpression* subExpressionP)
{
  KjNode*  itemP;
  char*    geometryP          = NULL;
  KjNode*  coordinatesNodeP   = NULL;
  char*    georelP            = NULL;
  char*    geopropertyP       = NULL;

  for (itemP = kNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (SCOMPARE9(itemP->name, 'g', 'e', 'o', 'm', 'e', 't', 'r', 'y', 0))
    {
      DUPLICATE_CHECK(geometryP, "GeoQuery::geometry", itemP->value.s);
      STRING_CHECK(itemP, "GeoQuery::geometry");
    }
    else if (SCOMPARE12(itemP->name, 'c', 'o', 'o', 'r', 'd', 'i', 'n', 'a', 't', 'e', 's', 0))
    {
      DUPLICATE_CHECK(coordinatesNodeP, "GeoQuery::coordinates", itemP);
      if ((itemP->type != KjString) && (itemP->type != KjArray))
      {
        orionldError(OrionldBadRequestData, "Invalid value type (not String nor Array)", "GeoQuery::coordinates", 400);
        return false;
      }
    }
    else if (SCOMPARE7(itemP->name, 'g', 'e', 'o', 'r', 'e', 'l', 0))
    {
      DUPLICATE_CHECK(georelP, "GeoQuery::georel", itemP->value.s);
      STRING_CHECK(itemP, "GeoQuery::georel");
    }
    else if (SCOMPARE12(itemP->name, 'g', 'e', 'o', 'p', 'r', 'o', 'p', 'e', 'r', 't', 'y', 0))
    {
      DUPLICATE_CHECK(geopropertyP, "GeoQuery::geoproperty", itemP->value.s);
      STRING_CHECK(itemP, "GeoQuery::geoproperty");
    }
    else
    {
      orionldError(OrionldBadRequestData, "Unknown GeoQuery field", itemP->name, 400);
      return false;
    }
  }

  if (geometryP == NULL)
  {
    orionldError(OrionldBadRequestData, "GeoQuery::geometry missing in Subscription", NULL, 400);
    return false;
  }

  if (coordinatesNodeP == NULL)
  {
    orionldError(OrionldBadRequestData, "GeoQuery::coordinates missing in Subscription", NULL, 400);
    return false;
  }

  if (georelP == NULL)
  {
    orionldError(OrionldBadRequestData, "GeoQuery::georel missing in Subscription", NULL, 400);
    return false;
  }

  //
  // Subscriptions only support "Point"
  //
  if (strcmp(geometryP, "Point") != 0)
  {
    orionldError(OrionldOperationNotSupported, "Not Implemented", "Subscriptions only support Point for geometry (right now)", 501);
    return false;
  }

  char  coords[512];  // 512 should be enough for most cases - if not, kaAlloc
  char* coordsP = NULL;

  if (coordinatesNodeP->type == KjArray)
  {
    int coordinatesLen = kjFastRenderSize(coordinatesNodeP);

    if (coordinatesLen > 512)
      coordsP = kaAlloc(&orionldState.kalloc, coordinatesLen);
    else
      coordsP = coords;

    kjFastRender(coordinatesNodeP, coordsP);
  }
  else  // it's a String
    coordsP = coordinatesNodeP->value.s;

  //
  // The coordinates are stored in the DB as a String.
  // That's why the Array was coinverted to a String.
  // Furthermore, the coordinates are stored in the DB without '[]' - so the brackets must be removed
  //
  if (coordsP[0] == '[')
  {
    coordsP = &coordsP[1];             // Removing first '['
    coordsP[strlen(coordsP) - 1] = 0;  // Removing last ']'
  }

  subExpressionP->coords = coordsP;

  // Database Model: "near;maxDistance==10" => "near;maxDistance:10"
  if (strncmp(georelP, "near;", 5) == 0)
    dbModelFromGeorel(georelP);

  subExpressionP->geometry    = dbModelFromGeometry(geometryP);
  subExpressionP->georel      = georelP;
  subExpressionP->geoproperty = (geopropertyP != NULL)? geopropertyP : "";

  //
  // FIXME: geoproperty is not part of SubscriptionExpression in APIv2.
  //        For now, we skip geoproperty and only work on 'location'.
  //

  return true;
}
