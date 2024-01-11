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
#include "kjson/kjBuilder.h"                                     // kjChildRemove
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/dateTime.h"                             // dateTimeFromString
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/mongoc/mongocEntityReplace.h"                  // mongocEntityReplace
#include "orionld/dbModel/dbModelFromApiEntity.h"                // dbModelFromApiEntity
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity2
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/payloadCheck/pCheckEntityType.h"               // pCheckEntityType
#include "orionld/payloadCheck/pCheckEntity.h"                   // pCheckEntity
#include "orionld/notifications/previousValues.h"                // previousValues
#include "orionld/serviceRoutines/orionldPutEntity.h"            // Own Interface



// ----------------------------------------------------------------------------
//
// entityIdCheck -
//
static bool entityIdCheck(KjNode* idP, const char* entityIdFromUrl)
{
  if (idP != NULL)
  {
    if (idP->type != KjString)
    {
      orionldError(OrionldBadRequestData, "Invalid JSON type", "id", 400);
      return false;
    }

    if (strcmp(idP->value.s, entityIdFromUrl) != 0)
    {
      orionldError(OrionldBadRequestData, "Non-matching entity id in payload body", idP->value.s, 400);
      return false;
    }
  }

  if (pCheckUri(entityIdFromUrl, "id", true) == false)  // This can never happen!
    return false;

  return true;
}



// -----------------------------------------------------------------------------
//
// apiEntityToDbEntity -
//
// NOTE
//   This function destroys the old DB Entity tree
//
KjNode* apiEntityToDbEntity(KjNode* apiEntityP, KjNode* oldDbEntityP, const char* entityId)
{
  KjNode* dbEntityP   = kjObject(orionldState.kjsonP, NULL);
  KjNode* attrNamesP  = kjArray(orionldState.kjsonP,  "attrNames");
  KjNode* attrsP      = kjObject(orionldState.kjsonP, "attrs");
  KjNode* modDateP    = kjFloat(orionldState.kjsonP,  "modDate", orionldState.requestTime);
  KjNode* creDateP    = kjLookup(oldDbEntityP, "creDate");

  if (creDateP == NULL)
    creDateP = kjFloat(orionldState.kjsonP, "creDate", orionldState.requestTime);
  else
    kjChildRemove(oldDbEntityP, creDateP);

  if ((dbEntityP == NULL) || (attrNamesP == NULL) || (attrsP == NULL) || (modDateP == NULL) || (creDateP == NULL))
  {
    orionldError(OrionldInternalError, "Internal Error", "Out of memory", 500);
    return NULL;
  }

  // Get the _id object from the old DB Entity - that hasn't changed ... well, the entity type might change ... see loop later
  KjNode* _idP = kjLookup(oldDbEntityP, "_id");
  if (_idP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error (entity without _id)", entityId, 500);
    return NULL;
  }

  kjChildRemove(oldDbEntityP, _idP);

  kjChildAdd(dbEntityP, _idP);
  kjChildAdd(dbEntityP, attrNamesP);
  kjChildAdd(dbEntityP, attrsP);
  kjChildAdd(dbEntityP, creDateP);
  kjChildAdd(dbEntityP, modDateP);

  for (KjNode* attrP = apiEntityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (strcmp(attrP->name, "id")         == 0) continue;
    if (strcmp(attrP->name, "type")       == 0) continue;
    if (strcmp(attrP->name, "createdAt")  == 0) continue;
    if (strcmp(attrP->name, "modifiedAt") == 0) continue;

    char*   longName = orionldAttributeExpand(orionldState.contextP, attrP->name, true, NULL);
    KjNode* nameNode = kjString(orionldState.kjsonP, NULL, longName);
    kjChildAdd(attrNamesP, nameNode);

    KjNode* newAttrP = kjClone(orionldState.kjsonP, attrP);  // The incoming payload is not destroyed (apiEntityP)
    KjNode* mdNamesP = kjArray(orionldState.kjsonP,  "mdNames");
    KjNode* mdP      = kjObject(orionldState.kjsonP, "md");

    if ((newAttrP == NULL) || (mdNamesP == NULL))
    {
      orionldError(OrionldInternalError, "Internal Error", "Out of memory", 500);
      return NULL;
    }

    dotForEq(newAttrP->name);
    kjChildAdd(attrsP, newAttrP);

    KjNode* saP      = newAttrP->value.firstChildP;
    KjNode* subAttrP = saP;
    while (subAttrP != NULL)
    {
      //
      // This construction of saP (used) and subAttrP (incrementor) makes it possible to remove
      // items from the list inside the loop
      //
      saP      = subAttrP;   // saP is used inside this loop
      subAttrP = saP->next;  // this is just the "loop incrementor"

      if (strcmp(saP->name, "type")        == 0) continue;
      if (strcmp(saP->name, "value")       == 0) continue;
      if (strcmp(saP->name, "object")      == 0) { saP->name = (char*) "value"; continue; }
      if (strcmp(saP->name, "languageMap") == 0) { saP->name = (char*) "value"; continue; }

      if (strcmp(saP->name, "datasetId") == 0)
      {
        orionldError(OrionldOperationNotSupported, "Not Implemented (for this request type)", "datasetId", 501);
        return NULL;
      }

      // Add sub-attribute name to "mdNames"
      KjNode* mdName = kjString(orionldState.kjsonP, NULL, saP->name);
      kjChildAdd(mdNamesP, mdName);

      // Move sub-attribute down to "md"
      kjChildRemove(newAttrP, saP);
      kjChildAdd(mdP, saP);

      if (strcmp(saP->name, "observedAt") == 0)
      {
        char errorString[256];

        double  dateTime = dateTimeFromString(saP->value.s, errorString, sizeof(errorString));

        if (dateTime < 0)
        {
          orionldError(OrionldBadRequestData, "Invalid ISO8601 for 'observedAt'", errorString, 400);
          return NULL;
        }

        KjNode* oaP      = kjFloat(orionldState.kjsonP, "observedAt", dateTime);

        saP->type = KjObject;
        saP->value.firstChildP = oaP;
        saP->lastChild         = oaP;
        oaP->next = NULL;
        oaP->name = (char*) "value";
      }
      else if (strcmp(saP->name, "unitCode") == 0)
      {
        // Make it a property without a type ...
        KjNode* ucP = kjString(orionldState.kjsonP, "unitCode", saP->value.s);
        saP->type = KjObject;
        saP->value.firstChildP = ucP;
        saP->lastChild         = ucP;
        ucP->next = NULL;
        ucP->name = (char*) "value";
      }
      else
      {
        dotForEq(saP->name);

        for (KjNode* subAttrFieldP = saP->value.firstChildP; subAttrFieldP != NULL; subAttrFieldP = subAttrFieldP->next)
        {
          if (strcmp(subAttrFieldP->name, "type")        == 0) continue;
          if (strcmp(subAttrFieldP->name, "value")       == 0) continue;
          if (strcmp(subAttrFieldP->name, "object")      == 0) { subAttrFieldP->name = (char*) "value"; continue; }
          if (strcmp(subAttrFieldP->name, "languageMap") == 0) { subAttrFieldP->name = (char*) "value"; continue; }
        }

        creDateP = kjFloat(orionldState.kjsonP,  "createdAt",  orionldState.requestTime);
        modDateP = kjFloat(orionldState.kjsonP,  "modifiedAt", orionldState.requestTime);
        kjChildAdd(saP, creDateP);
        kjChildAdd(saP, modDateP);
      }
    }

    kjChildAdd(newAttrP, mdNamesP);          // mdNames always present in the DB
    if (mdP->value.firstChildP != NULL)      // md only present if there are actually any metadata
      kjChildAdd(newAttrP, mdP);

    creDateP = kjFloat(orionldState.kjsonP,  "creDate", orionldState.requestTime);
    modDateP = kjFloat(orionldState.kjsonP,  "modDate", orionldState.requestTime);
    kjChildAdd(newAttrP, creDateP);
    kjChildAdd(newAttrP, modDateP);
  }

  //
  // "lastCorrelator" ... not used in NGSI-LD, but, for NGSIv2 backwards compatibility, it should be present in DB
  //
  KjNode* lastCorrelatorP = kjString(orionldState.kjsonP,  "lastCorrelator", "");
  kjChildAdd(dbEntityP, lastCorrelatorP);

  return dbEntityP;
}



// ----------------------------------------------------------------------------
//
// orionldPutEntity -
//
bool orionldPutEntity(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))
  {
    orionldError(OrionldResourceNotFound, "Service Not Found", orionldState.urlPath, 404);
    return false;
  }

  if (orionldState.requestTree->type != KjObject)
  {
    orionldError(OrionldResourceNotFound, "Invalid payload body", "Invalid JSON type for Entity - not a JSON Object", 400);
    return false;
  }

  if (orionldState.requestTree->value.firstChildP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Invalid payload body", "Empty JSON Object for Entity", 400);
    return false;
  }

  char* entityId = orionldState.wildcard[0];

  //
  // Check Entity ID - if entity id is present in payload body, it must be identical to the entity id in the URL PATH
  //
  KjNode* idNodeP = kjLookup(orionldState.requestTree, "id");
  if (entityIdCheck(idNodeP, entityId) == false)
    return false;

  //
  // Check Entity Type
  //
  char*   entityType = orionldState.uriParams.type;  // Set by pCheckEntityType - need to look at that ...
  KjNode* typeNodeP  = kjLookup(orionldState.requestTree, "type");

  if (pCheckEntityType(typeNodeP, true, &entityType) == false)
    return false;

  //
  // Get the entity from the database
  //
  KjNode* oldDbEntityP = mongocEntityLookup(entityId, NULL, NULL, NULL, NULL);
  if (oldDbEntityP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Entity does not exist", entityId, 404);
    return false;
  }

  //
  // Check the attributes
  //
  KjNode* dbAttrsP = kjLookup(oldDbEntityP, "attrs");
  if (pCheckEntity(orionldState.requestTree, false, dbAttrsP) == false)
    return false;

  previousValues(orionldState.requestTree, dbAttrsP);

  //
  // Create the new DB Entity, based mainly on orionldState.requestTree
  // From oldDbEntityP (old db content) we just keep the creDate of the entity
  //
  // FIXME: Use dbModelFromApiEntity instead of apiEntityToDbEntity
  //
  KjNode* dbEntityP = apiEntityToDbEntity(orionldState.requestTree, oldDbEntityP, entityId);

  if (dbEntityP == NULL)
    return false;

  if (mongocEntityReplace(dbEntityP, entityId) == false)
  {
    orionldError(OrionldInternalError, "Database Error", "mongocEntityReplace failed", 500);
    return false;
  }

  KjNode* finalApiEntityWithSysAttrs = dbModelToApiEntity2(dbEntityP, true, RF_NORMALIZED, NULL, false, &orionldState.pd);

  orionldState.alterations = (OrionldAlteration*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlteration));
  orionldState.alterations->entityId                    = entityId;
  orionldState.alterations->entityType                  = typeNodeP->value.s;
  orionldState.alterations->inEntityP                   = orionldState.requestTree;
  orionldState.alterations->dbEntityP                   = NULL;
  orionldState.alterations->finalApiEntityWithSysAttrsP = finalApiEntityWithSysAttrs;
  orionldState.alterations->finalApiEntityP             = orionldState.requestTree;
  orionldState.alterations->alteredAttributes           = 0;
  orionldState.alterations->alteredAttributeV           = NULL;
  orionldState.alterations->next                        = NULL;

  //
  // Is the entity id inside orionldState.requestTree?
  // If not, it must be added for the notification
  //
  idNodeP = kjLookup(orionldState.requestTree, "id");
  if (idNodeP == NULL)
  {
    idNodeP = kjString(orionldState.kjsonP, "id", entityId);
    kjChildAdd(orionldState.requestTree, idNodeP);
  }

  orionldState.httpStatusCode = 204;

  return true;
}
