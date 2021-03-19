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
#include <string.h>                                              // strlen
#include <string>                                                // std::string
#include <vector>                                                // std::vector

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjChildAdd, kjObject, kjArray, ...
#include "kalloc/kaStrdup.h"                                     // kaStrdup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                  // httpHeaderAdd

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/QNode.h"                                // QNode
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/payloadCheck/pcheckQuery.h"                    // pcheckQuery
#include "orionld/db/dbConfiguration.h"                          // dbEntitiesQuery
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/serviceRoutines/orionldPostQuery.h"            // Own Interface



// -----------------------------------------------------------------------------
//
// dmodelMetadata -
//
KjNode* dmodelMetadata(KjNode* dbMetadataP, bool sysAttrs, OrionldProblemDetails* pdP)
{
  char*   longName = kaStrdup(&orionldState.kalloc, dbMetadataP->name);
  eqForDot(longName);

  char*   alias    = orionldContextItemAliasLookup(orionldState.contextP, longName, NULL, NULL);
  KjNode* mdP      = kjObject(orionldState.kjsonP, alias);
  KjNode* nodeP    = dbMetadataP->value.firstChildP;
  KjNode* next;

  while (nodeP != NULL)
  {
    next = nodeP->next;
    if      (strcmp(nodeP->name, "type")       == 0) kjChildAdd(mdP, nodeP);
    else if (strcmp(nodeP->name, "value")      == 0) kjChildAdd(mdP, nodeP);
    else if (strcmp(nodeP->name, "object")     == 0) kjChildAdd(mdP, nodeP);
    else if (strcmp(nodeP->name, "unitCode")   == 0) kjChildAdd(mdP, nodeP);
    else if (strcmp(nodeP->name, "observedAt") == 0) kjChildAdd(mdP, nodeP);
    else if (sysAttrs == true)
    {
      if (strcmp(nodeP->name, "creDate") == 0)
      {
        nodeP->name = (char*) "createdAt";
        kjChildAdd(mdP, nodeP);
      }
      else if (strcmp(nodeP->name, "modDate") == 0)
      {
        nodeP->name = (char*) "modifiedAt";
        kjChildAdd(mdP, nodeP);
      }
    }
    else
      LM_W(("Skipping sub-sub-attribute '%s'", nodeP->name));

    nodeP = next;
  }

  return mdP;
}



// -----------------------------------------------------------------------------
//
// dmodelAttribute -
//
KjNode* dmodelAttribute(KjNode* dbAttrP, bool sysAttrs, OrionldProblemDetails* pdP)
{
  char*   longName = kaStrdup(&orionldState.kalloc, dbAttrP->name);

  eqForDot(longName);

  char*   alias = orionldContextItemAliasLookup(orionldState.contextP, longName, NULL, NULL);
  KjNode* attrP = kjObject(orionldState.kjsonP, alias);
  KjNode* nodeP = dbAttrP->value.firstChildP;
  KjNode* mdsP  = NULL;
  KjNode* next;

  while (nodeP != NULL)
  {
    next = nodeP->next;

    if (strcmp(nodeP->name, "type") == 0)
      kjChildAdd(attrP, nodeP);
    else if (strcmp(nodeP->name, "value") == 0)
      kjChildAdd(attrP, nodeP);
    else if (strcmp(nodeP->name, "object") == 0)
      kjChildAdd(attrP, nodeP);
    else if (sysAttrs == true)
    {
      if (strcmp(nodeP->name, "creDate") == 0)
      {
        nodeP->name = (char*) "createdAt";
        kjChildAdd(attrP, nodeP);
      }
      else if (strcmp(nodeP->name, "modDate") == 0)
      {
        nodeP->name = (char*) "modifiedAt";
        kjChildAdd(attrP, nodeP);
      }
    }
    else if (strcmp(nodeP->name, "md") == 0)
      mdsP = nodeP;

    nodeP = next;
  }

  if (mdsP != NULL)
  {
    for (KjNode* mdP = mdsP->value.firstChildP; mdP != NULL; mdP = mdP->next)
    {
      KjNode* metadataP;

      if ((metadataP = dmodelMetadata(mdP, sysAttrs, pdP)) == NULL)
      {
        LM_E(("Datamodel Error (%s: %s)", pdP->title, pdP->detail));
        return NULL;
      }

      kjChildAdd(attrP, metadataP);
    }
  }

  return attrP;
}



// -----------------------------------------------------------------------------
//
// dmodelEntity - move to its own, new, library
//
KjNode* dmodelEntity(KjNode* dbEntityP, bool sysAttrs, OrionldProblemDetails* pdP)
{
  KjNode* _idP            = NULL;
  KjNode* attrsP          = NULL;

  for (KjNode* nodeP = dbEntityP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (strcmp(nodeP->name, "_id") == 0)
    {
      _idP = nodeP;
      if (attrsP != NULL)
        break;
    }
    else if (strcmp(nodeP->name, "attrs") == 0)
    {
      attrsP = nodeP;
      if (_idP != NULL)
        break;
    }
  }

  if (_idP == NULL)
  {
    LM_E(("Database Error (the field '_id' is missing)"));
    pdP->title  = (char*) "Database Error";
    pdP->detail = (char*) "the field '_id' is missing";
    return NULL;
  }

  if (attrsP == NULL)
  {
    LM_E(("Database Error (the field 'attrs' is missing)"));
    pdP->title  = (char*) "Database Error";
    pdP->detail = (char*) "the field 'attrs' is missing";
    return NULL;
  }

  KjNode* idP   = NULL;
  KjNode* typeP = NULL;

  for (KjNode* nodeP = _idP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if ((strcmp(nodeP->name, "id") == 0) || (strcmp(nodeP->name, "@id") == 0))
      idP = nodeP;
    else if ((strcmp(nodeP->name, "type") == 0) || (strcmp(nodeP->name, "@type") == 0))
      typeP = nodeP;
  }

  if (idP == NULL)
  {
    LM_E(("Database Error (the field '_id.id' is missing)"));
    pdP->title  = (char*) "Database Error";
    pdP->detail = (char*) "the field '_id.id' is missing";
    return NULL;
  }

  if (typeP == NULL)
  {
    LM_E(("Database Error (the field '_id.type' is missing)"));
    pdP->title  = (char*) "Database Error";
    pdP->detail = (char*) "the field '_id.type' is missing";
    return NULL;
  }

  KjNode* entityP = kjObject(orionldState.kjsonP, NULL);
  kjChildAdd(entityP, idP);

  typeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, typeP->value.s, NULL, NULL);
  kjChildAdd(entityP, typeP);

  //
  // Second loop over dbEntityP if sysAttrs == true (normally it's not - this saves time when sysAttrs is not used)
  //
  if (sysAttrs == true)
  {
    KjNode* creDateP = NULL;
    KjNode* modDateP = NULL;

    for (KjNode* nodeP = dbEntityP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      if (strcmp(nodeP->name, "creDate") == 0)
      {
        creDateP = nodeP;
        creDateP->name = (char*) "createdAt";
        if (modDateP != NULL)
          break;
      }
      else if (strcmp(nodeP->name, "modDate") == 0)
      {
        modDateP = nodeP;
        modDateP->name = (char*) "modifiedAt";
        if (creDateP != NULL)
          break;
      }
    }

    kjChildAdd(entityP, creDateP);
    kjChildAdd(entityP, modDateP);
  }


  //
  // Now the attributes
  //
  if (attrsP != NULL)
  {
    for (KjNode* attrP = attrsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
    {
      KjNode* attributeP;

      if ((attributeP = dmodelAttribute(attrP, sysAttrs, pdP)) == NULL)
      {
        LM_E(("Datamodel Error (%s: %s)", pdP->title, pdP->detail));
        return NULL;
      }

      kjChildAdd(entityP, attributeP);
    }
  }

  return entityP;
}



// ----------------------------------------------------------------------------
//
// orionldPostQuery -
//
// POST /ngsi-ld/v1/entityOperations/query
//
bool orionldPostQuery(ConnectionInfo* ciP)
{
  if (orionldState.requestTree->type != KjObject)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Bad Request", "The payload data for POST Query must be a JSON Object");
    orionldState.httpStatusCode = 400;

    return false;
  }

  KjNode*  entitiesP = NULL;
  KjNode*  attrsP    = NULL;
  QNode*   qTree     = NULL;
  KjNode*  geoqP     = NULL;

  // pcheckQuery makes sure the Payload Data is correct and it expands all fields that should be expanded
  if (pcheckQuery(orionldState.requestTree, &entitiesP, &attrsP, &qTree, &geoqP) == false)
    return false;

  int      count;
  int      limit  = orionldState.uriParams.limit;
  int      offset = orionldState.uriParams.offset;
  int*     countP = (orionldState.uriParams.count == true)? &count : NULL;
  KjNode*  dbEntityArray;

  if ((dbEntityArray = dbEntitiesQuery(entitiesP, attrsP, qTree, geoqP, limit, offset, countP)) == NULL)
  {
    // Not an error - just "nothing found" - return an empty array
    orionldState.responsePayload = (char*) "[]";
    if (countP != NULL)
    {
      char number[16];

      snprintf(number, sizeof(number), "%d", count);
      httpHeaderAdd(ciP, "NGSILD-Results-Count", number);
    }
    return true;
  }

  //
  // Now the "raw db entities" must be fixed.
  // Also, do we need another field in the payload for filtering out which attrs to be returned???
  //
  orionldState.responseTree = kjArray(orionldState.kjsonP, NULL);

  if (dbEntityArray->value.firstChildP != NULL)
  {
    for (KjNode* dbEntityP = dbEntityArray->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
    {
      KjNode*                entityP;
      OrionldProblemDetails  pd;

      if ((entityP = dmodelEntity(dbEntityP, orionldState.uriParamOptions.sysAttrs, &pd)) == NULL)
      {
        LM_E(("Database Error (%s: %s)", pd.title, pd.detail));
        orionldState.httpStatusCode = 500;
        return false;
      }

      kjChildAdd(orionldState.responseTree, entityP);
    }
  }
  else
    orionldState.noLinkHeader = true;

  orionldState.httpStatusCode = 200;

  if (countP != NULL)
  {
    char number[16];

    snprintf(number, sizeof(number), "%d", count);
    httpHeaderAdd(ciP, "NGSILD-Results-Count", number);
  }

  return true;
}

