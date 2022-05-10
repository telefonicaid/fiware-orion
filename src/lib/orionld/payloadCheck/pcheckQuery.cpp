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

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/context/orionldCoreContext.h"                  // orionldDefaultUrl, orionldCoreContext
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/q/QNode.h"                                     // QNode
#include "orionld/q/qLex.h"                                      // qLex
#include "orionld/q/qParse.h"                                    // qParse
#include "orionld/payloadCheck/pcheckEntityInfoArray.h"          // pcheckEntityInfoArray
#include "orionld/payloadCheck/pcheckAttrs.h"                    // pcheckAttrs
#include "orionld/payloadCheck/pcheckQ.h"                        // pcheckQ
#include "orionld/payloadCheck/pcheckGeoQ.h"                     // pcheckGeoQ
#include "orionld/payloadCheck/pcheckQuery.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// pcheckQueryEntities -
//
static bool pcheckQueryEntities(KjNode* entitiesP)
{
  ARRAY_CHECK(entitiesP, "Query::entities");
  EMPTY_ARRAY_CHECK(entitiesP, "Query::entities");

  for (KjNode* entitySelectorP = entitiesP->value.firstChildP; entitySelectorP != NULL; entitySelectorP = entitySelectorP->next)
  {
    OBJECT_CHECK(entitySelectorP, "Query::entities[X]");
    EMPTY_OBJECT_CHECK(entitySelectorP, "Query::entities[X]");

    KjNode* typeP       = NULL;
    KjNode* idP         = NULL;
    KjNode* idPatternP  = NULL;

    for (KjNode* selectorItemP = entitySelectorP->value.firstChildP; selectorItemP != NULL; selectorItemP = selectorItemP->next)
    {
      if (strcmp(selectorItemP->name, "id") == 0)
      {
        DUPLICATE_CHECK(idP, "Query::entities[X]::id", selectorItemP);
        STRING_CHECK(idP, "Query::entities[X]::id");
        URI_CHECK(idP->value.s, "Query::entities[X]::id", true);
      }
      else if (strcmp(selectorItemP->name, "type") == 0)
      {
        DUPLICATE_CHECK(typeP, "Query::entities[X]::type", selectorItemP);
        STRING_CHECK(typeP, "Query::entities[X]::type");
        URI_CHECK(typeP->value.s, "Query::entities[X]::type", false);
      }
      else if (strcmp(selectorItemP->name, "idPattern") == 0)
      {
        DUPLICATE_CHECK(idPatternP, "Query::entities[X]::idPattern", selectorItemP);
        STRING_CHECK(idPatternP, "Query::entities[X]::idPattern");
      }
      else
      {
        orionldError(OrionldBadRequestData, "Invalid field for Query::entities[X]", selectorItemP->name, 400);
        return false;
      }
    }

    if (typeP == NULL)
    {
      orionldError(OrionldBadRequestData, "Mandatory field missing", "Query::entities[X]::type", 400);
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pcheckQuery -
//
bool pcheckQuery(KjNode* tree, KjNode** entitiesPP, KjNode** attrsPP, QNode** qTreePP, KjNode** geoqPP, char** langPP)
{
  KjNode*  entitiesP   = NULL;
  KjNode*  attrsP      = NULL;
  KjNode*  qP          = NULL;
  KjNode*  geoqP       = NULL;
  KjNode*  langP       = NULL;
  KjNode*  typeP       = NULL;  // Must be a string with the value "Query"

  OBJECT_CHECK(tree, "payload body");
  EMPTY_OBJECT_CHECK(tree, "payload body");

  //
  // Check for duplicated items and that data types are correct
  //
  for (KjNode* kNodeP = tree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    if (strcmp(kNodeP->name, "type") == 0)
    {
      DUPLICATE_CHECK(typeP, "type", kNodeP);
      STRING_CHECK(typeP, "type");
      EMPTY_STRING_CHECK(typeP, "type");
      if (strcmp(typeP->value.s, "Query") != 0)
      {
        orionldError(OrionldBadRequestData, "Invalid value for 'type' member of a POST Query", "Must be a JSON String with the value /Query/", 400);
        return false;
      }
    }
    else if (strcmp(kNodeP->name, "entities") == 0)
    {
      DUPLICATE_CHECK(entitiesP, "entities", kNodeP);
      ARRAY_CHECK(entitiesP, "entities");
      EMPTY_ARRAY_CHECK(entitiesP, "entities");
      if (pcheckQueryEntities(kNodeP) == false)
        return false;

      *entitiesPP = entitiesP;
    }
    else if (strcmp(kNodeP->name, "attrs") == 0)
    {
      DUPLICATE_CHECK(attrsP, "attrs", kNodeP);
      ARRAY_CHECK(attrsP, "attrs");
      EMPTY_ARRAY_CHECK(attrsP, "attrs");
      *attrsPP = attrsP;
    }
    else if ((kNodeP->name[0] == 'q') && (kNodeP->name[1] == 0))
    {
      DUPLICATE_CHECK(qP, "q", kNodeP);
      STRING_CHECK(qP, "q");
      EMPTY_STRING_CHECK(qP, "q");
    }
    else if (strcmp(kNodeP->name, "geoQ") == 0)
    {
      DUPLICATE_CHECK(geoqP, "geoQ", kNodeP);
      OBJECT_CHECK(geoqP, "geoQ");
      EMPTY_OBJECT_CHECK(geoqP, "geoQ");
      *geoqPP = geoqP;
    }
    else if (strcmp(kNodeP->name, "lang") == 0)
    {
      DUPLICATE_CHECK(langP, "lang", kNodeP);
      STRING_CHECK(langP, "lang");
      EMPTY_STRING_CHECK(langP, "lang");
      *langPP = langP->value.s;
    }
    else if (strcmp(kNodeP->name, "temporalQ") == 0)
    {
      //
      // Temporal Queries are recognized but not allowed
      //
      orionldError(OrionldBadRequestData, "Not Implemented", "Temporal Query as part of POST Query", 501);
      return false;
    }
    else  // Not Recognized - Error
    {
      orionldError(OrionldBadRequestData, "Invalid field for query", kNodeP->name, 400);
      return false;
    }
  }

  if (typeP == NULL)
  {
    orionldError(OrionldBadRequestData, "Mandatory field missing", "Query::type", 400);
    return false;
  }

  if ((entitiesP != NULL) && (pcheckEntityInfoArray(entitiesP, false, "Query::entities") == false))
    return false;
  if ((attrsP != NULL) && (pcheckAttrs(attrsP) == false))
    return false;
  if ((qP != NULL) && ((*qTreePP = pcheckQ(qP->value.s)) == NULL))
    return false;
  if ((geoqP != NULL) && (pcheckGeoQ(geoqP, false) == false))  // Don't change coordinates from array to string
    return false;

  return true;
}
