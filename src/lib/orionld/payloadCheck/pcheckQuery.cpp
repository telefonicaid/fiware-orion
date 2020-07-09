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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/context/orionldCoreContext.h"                  // orionldDefaultUrl, orionldCoreContext
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/QNode.h"                                // QNode
#include "orionld/common/qLex.h"                                 // qLex
#include "orionld/common/qParse.h"                               // qParse
#include "orionld/payloadCheck/pcheckEntityInfoArray.h"          // pcheckEntityInfoArray
#include "orionld/payloadCheck/pcheckAttrs.h"                    // pcheckAttrs
#include "orionld/payloadCheck/pcheckQ.h"                        // pcheckQ
#include "orionld/payloadCheck/pcheckGeoQ.h"                     // pcheckGeoQ
#include "orionld/payloadCheck/pcheckQuery.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// pcheckQuery -
//
bool pcheckQuery(KjNode* tree, KjNode** entitiesPP, KjNode** attrsPP, QNode** qTreePP, KjNode** geoqPP)
{
  KjNode*  entitiesP   = NULL;
  KjNode*  attrsP      = NULL;
  KjNode*  qP          = NULL;
  KjNode*  geoqP       = NULL;

  //
  // Check for duplicated items and that data types are correct
  //
  for (KjNode* kNodeP = tree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    if (SCOMPARE9(kNodeP->name, 'e', 'n', 't', 'i', 't', 'i', 'e', 's', 0))
    {
      DUPLICATE_CHECK(entitiesP, "entities", kNodeP);
      ARRAY_CHECK(entitiesP, "entities");
      EMPTY_ARRAY_CHECK(entitiesP, "entities");
      *entitiesPP = entitiesP;
    }
    else if (SCOMPARE6(kNodeP->name, 'a', 't', 't', 'r', 's', 0))
    {
      DUPLICATE_CHECK(attrsP, "attrs", kNodeP);
      ARRAY_CHECK(attrsP, "attrs");
      EMPTY_ARRAY_CHECK(attrsP, "attrs");
      *attrsPP = attrsP;
    }
    else if (SCOMPARE2(kNodeP->name, 'q', 0))
    {
      DUPLICATE_CHECK(qP, "q", kNodeP);
      STRING_CHECK(qP, "q");
      EMPTY_STRING_CHECK(qP, "q");
    }
    else if (SCOMPARE5(kNodeP->name, 'g', 'e', 'o', 'Q', 0))
    {
      DUPLICATE_CHECK(geoqP, "geoQ", kNodeP);
      OBJECT_CHECK(geoqP, "geoQ");
      EMPTY_OBJECT_CHECK(geoqP, "geoQ");
      *geoqPP = geoqP;
    }
    else if (SCOMPARE10(kNodeP->name, 't', 'e', 'm', 'p', 'o', 'r', 'a', 'l', 'Q', 0))
    {
      //
      // Temporal Queries are recognized but not allowed
      //
      orionldErrorResponseCreate(OrionldBadRequestData, "Not Implemented", "Temporal Query as part of POST Query");
      orionldState.httpStatusCode = 501;

      return false;
    }
    else  // Not Recognized - Error
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for query", kNodeP->name);
      orionldState.httpStatusCode = 400;

      return false;
    }
  }

  if ((entitiesP != NULL) && (pcheckEntityInfoArray(entitiesP, false) == false))
    return false;
  if ((attrsP != NULL) && (pcheckAttrs(attrsP) == false))
    return false;
  if ((qP != NULL) && ((*qTreePP = pcheckQ(qP->value.s)) == NULL))
    return false;
  if ((geoqP != NULL) && (pcheckGeoQ(geoqP, false) == false))  // Don't change coordinates from array to string
    return false;

  return true;
}
