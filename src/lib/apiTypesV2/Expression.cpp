/*
 *
 * Copyright 2025 Telefonica Investigacion y Desarrollo, S.A.U
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
 * Author: Fermin Galan
 */



#include "apiTypesV2/Expression.h"



/* ****************************************************************************
*
* Expression::fill -
*/
bool Expression::fill(Expression *exprP, std::string *errP)
{
  q = exprP->q;
  mq = exprP->mq;
  geometry = exprP->geometry;
  coords = exprP->coords;
  georel = exprP->georel;

  // geoFilter is filled only if all needed parameters are present
  bool geoFillResult = true;
  if (!geometry.empty() && !coords.empty() && !georel.empty())
  {
    geoFillResult = (geoFilter.fill(exprP->geometry, exprP->coords, exprP->georel, errP) == 0);
  }

  return geoFillResult &&
         stringFilter.fill(&exprP->stringFilter, errP) &&
         mdStringFilter.fill(&exprP->mdStringFilter, errP);
}