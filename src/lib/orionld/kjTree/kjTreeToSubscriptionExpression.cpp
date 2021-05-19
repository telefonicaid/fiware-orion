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
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjRender.h"                                    // kjFastRender
}

#include "apiTypesV2/SubscriptionExpression.h"                 // SubscriptionExpression

#include "orionld/common/CHECK.h"                              // CHECKx()
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/kjTree/kjTreeToSubscriptionExpression.h"     // Own interface



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
      if (itemP->type == KjString)
      {
        DUPLICATE_CHECK(coordinatesNodeP, "GeoQuery::coordinates", itemP);
      }
      else if (itemP->type == KjArray)
      {
        DUPLICATE_CHECK(coordinatesNodeP, "GeoQuery::coordinates", itemP);
      }
      else
      {
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value type (not String nor Array)", "GeoQuery::coordinates");
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
      orionldErrorResponseCreate(OrionldBadRequestData, "Unknown GeoQuery field", itemP->name);
      return false;
    }
  }

  if (geometryP == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "GeoQuery::geometry missing in Subscription", NULL);
    return false;
  }

  if (coordinatesNodeP == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "GeoQuery::coordinates missing in Subscription", NULL);
    return false;
  }

  if (georelP == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "GeoQuery::georel missing in Subscription", NULL);
    return false;
  }

  if (coordinatesNodeP->type == KjArray)
  {
    char coords[512];

    //
    // We have a little problem here ...
    // SubscriptionExpression::coords is a std::string in APIv2.
    // NGSI-LD needs it to be able to be an array.
    // Easiest way to fix this is to render the JSON Array and translate it to a string, and then removing the '[]'
    //
    kjFastRender(orionldState.kjsonP, coordinatesNodeP, coords, sizeof(coords));
    coords[strlen(coords) - 1] = 0;       // Removing last ']'
    subExpressionP->coords = &coords[1];  // Removing first '['
  }
  else
    subExpressionP->coords = coordinatesNodeP->value.s;

  subExpressionP->geometry    = geometryP;
  subExpressionP->georel      = georelP;
  subExpressionP->geoproperty = (geopropertyP != NULL)? geopropertyP : "";

  //
  // FIXME: geoproperty is not part of SubscriptionExpression in APIv2.
  //        For now, we skip geoproperty and only work on 'location'.
  //

  return true;
}
