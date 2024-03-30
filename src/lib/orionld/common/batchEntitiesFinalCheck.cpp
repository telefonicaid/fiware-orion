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
#include <string.h>                                            // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjArray, ...
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/entityErrorPush.h"                    // entityErrorPush
#include "orionld/common/entityLookupById.h"                   // entityLookupBy_id_Id
#include "orionld/payloadCheck/pCheckEntity.h"                 // pCheckEntity
#include "orionld/context/orionldContextFromTree.h"            // orionldContextFromTree
#include "orionld/common/batchEntitiesFinalCheck.h"            // Own interface



// -----------------------------------------------------------------------------
//
// entityTypeCheck -
//
static bool entityTypeCheck(const char* oldEntityType, KjNode* entityP)
{
  KjNode* newEntityTypeNodeP = kjLookup(entityP, "type");

  if (newEntityTypeNodeP == NULL)
    newEntityTypeNodeP = kjLookup(entityP, "@type");

  if (newEntityTypeNodeP != NULL)
  {
    if (strcmp(newEntityTypeNodeP->value.s, oldEntityType) != 0)
      LM_RE(false, ("Attempt to change Entity Type"));
  }
  else
  {
    // No Entity Type present in the incoming payload - no problem, I'll just add it
    KjNode* entityTypeP = kjString(orionldState.kjsonP, "type", oldEntityType);
    kjChildAdd(entityP, entityTypeP);
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// creationByPreviousInstance -
//
static bool creationByPreviousInstance(KjNode* creationArrayP, KjNode* entityP, const char* entityId, char** oldTypeP)
{
  for (KjNode* eP = creationArrayP->value.firstChildP; eP != NULL; eP = eP->next)
  {
    KjNode* idNodeP = kjLookup(eP, "id");

    if (idNodeP == NULL)  // Can't happen
      continue;

    if (strcmp(entityId, idNodeP->value.s) == 0)
    {
      KjNode* typeNodeP = kjLookup(eP, "type");

      *oldTypeP = typeNodeP->value.s;
      return true;
    }
  }

  //
  // The entity is not part of the "Creation Array" - so, we add it
  // Any new er instances of this entity must have the exact same entity type
  //
  KjNode* newTypeNodeP = kjLookup(entityP, "type");  // What if newTypeNodeP is NULL - that would be an error!

  if (newTypeNodeP == NULL)
    *oldTypeP = NULL;
  else
  {
    *oldTypeP = newTypeNodeP->value.s;

    KjNode* eP    = kjObject(orionldState.kjsonP, NULL);
    KjNode* idP   = kjString(orionldState.kjsonP, "id", entityId);
    KjNode* typeP = kjString(orionldState.kjsonP, "type", newTypeNodeP->value.s);

    kjChildAdd(eP, idP);
    kjChildAdd(eP, typeP);
    kjChildAdd(creationArrayP, eP);
  }

  return false;
}



// ----------------------------------------------------------------------------
//
// batchEntitiesFinalCheck -
//
int batchEntitiesFinalCheck(KjNode* requestTree, KjNode* errorsArrayP, KjNode* dbEntityArray, bool update, bool mustExist, bool cannotExist)
{
  int      noOfEntities   = 0;
  KjNode*  eP             = requestTree->value.firstChildP;
  KjNode*  next;
  KjNode*  creationArrayP = kjArray(orionldState.kjsonP, NULL);  // Array of entity id+type of entities that are to be created

  while (eP != NULL)
  {
    next = eP->next;

    KjNode*         idNodeP      = kjLookup(eP, "id");  // batchEntityStringArrayPopulate makes sure that "id" exists
    char*           entityId     = idNodeP->value.s;
    KjNode*         contextNodeP = kjLookup(eP, "@context");
    OrionldContext* contextP     = NULL;

    //
    // If Content-Type is application/ld+json, then every entity must have an @context
    // If instead the Content-Type is application/json then an @context cannot be present
    //
    if (orionldState.in.contentType == MT_JSONLD)
    {
      if (contextNodeP == NULL)
      {
        const char* title  = "Invalid payload";
        const char* detail = "Content-Type is 'application/ld+json', but no @context in payload data array item";

        LM_E(("Content-Type is 'application/ld+json', but no @context found for entity '%s'", entityId));
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, title, detail, 400);
        kjChildRemove(orionldState.requestTree, eP);
        eP = next;
        continue;
      }

      contextP = orionldContextFromTree(NULL, OrionldContextFromInline, NULL, contextNodeP);
      if (contextP == NULL)
      {
        LM_E(("orionldContextFromTree reports error: %s: %s", orionldState.pd.title, orionldState.pd.detail));
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, orionldState.pd.title, orionldState.pd.detail, orionldState.pd.status);
        kjChildRemove(orionldState.requestTree, eP);
        eP = next;
        continue;
      }

      orionldState.contextP = contextP;
    }
    else
    {
      if (contextNodeP != NULL)
      {
        const char* title  = "Invalid payload";
        const char* detail = "Content-Type is 'application/json', and an @context is present in the payload data array item";

        LM_E(("Content-Type is 'application/json', and an @context is present in the payload data array item of entity '%s'", entityId));
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, title, detail, 400);
        kjChildRemove(orionldState.requestTree, eP);
        eP = next;
        continue;
      }
    }

    KjNode* dbAttrsP           = NULL;
    KjNode* dbEntityTypeNodeP  = NULL;
    KjNode* dbEntityP          = entityLookupBy_id_Id(dbEntityArray, entityId, &dbEntityTypeNodeP);

    if (update == true)
    {
      if (dbEntityP != NULL)
        dbAttrsP = kjLookup(dbEntityP, "attrs");
    }

    if (pCheckEntity(eP, true, dbAttrsP) == false)
    {
      entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, orionldState.pd.title, orionldState.pd.detail, orionldState.pd.status);
      kjChildRemove(orionldState.requestTree, eP);
      eP = next;
      continue;
    }

    if ((mustExist == true) && (dbEntityP == NULL))  // FIXME: Only interesting for BATCH UPSERT
    {
      LM_E(("The entity '%s' does not exist", entityId));
      entityErrorPush(errorsArrayP, entityId, OrionldResourceNotFound, "Entity not found", "Cannot update a non-existing entity", 404);
      kjChildRemove(orionldState.requestTree, eP);
      eP = next;
      continue;
    }

    if ((cannotExist == true) && (dbEntityP != NULL))  // FIXME: Only interesting for BATCH CREATE
    {
      LM_E(("The entity '%s' already exists", entityId));
      entityErrorPush(errorsArrayP, entityId, OrionldAlreadyExists, "Entity already exists", "Cannot create an existing entity", 409);
      kjChildRemove(orionldState.requestTree, eP);
      eP = next;
      continue;
    }

    //
    // If the entity to be upserted is found in the DB, we must make sure the entity type isn't changing
    // This check is done after pCheckEntity as pCheckEntity expands the entity type
    //
    if (dbEntityP != NULL)
    {
      if ((orionldState.uriParamOptions.replace == false) && (entityTypeCheck(dbEntityTypeNodeP->value.s, eP) == false))
      {
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Invalid Entity", "the Entity Type cannot be altered", 400);
        kjChildRemove(orionldState.requestTree, eP);
        eP = next;
        continue;
      }
    }
    else  // FIXME: if UPSERT - not interesting for BATCH UPDATE
    {
      //
      // The entity 'entityId' does not exist in the DB - seems like it's being CREATED
      // BUT - might not be the first instance of the entity in the incoming Array
      // Must check that as well.
      // AND, if not the first instance, then the entity type must be checked for altering (perhaps even entityTypeCheck can be used?)
      //

      char* oldType = NULL;
      if (creationByPreviousInstance(creationArrayP, eP, entityId, &oldType) == true)
      {
        if ((orionldState.uriParamOptions.replace == false) && (entityTypeCheck(oldType, eP) == false))
        {
          entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Invalid Entity", "the Entity Type cannot be altered", 400);
          kjChildRemove(orionldState.requestTree, eP);
          eP = next;
          continue;
        }
      }
      else if (oldType == NULL)
      {
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Invalid Entity", "no type in incoming payload for CREATION of Entity", 400);
        kjChildRemove(orionldState.requestTree, eP);
        eP = next;
        continue;
      }
    }

    // The @context has been applied (by pCheckEntity) - if in the payload body, it needs to go
    if (contextNodeP != NULL)
      kjChildRemove(eP, contextNodeP);

    ++noOfEntities;

    eP = next;
  }

  return noOfEntities;
}

