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
#include <unistd.h>                                            // NULL

extern "C"
{
#include "kalloc/kaStrdup.h"                                   // kaStrdup
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjArray, ...
#include "kjson/kjClone.h"                                     // kjClone
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/entitySuccessPush.h"                  // entitySuccessPush
#include "orionld/common/entityErrorPush.h"                    // entityErrorPush
#include "orionld/common/entityLookupById.h"                   // entityLookupById
#include "orionld/common/dotForEq.h"                           // dotForEq
#include "orionld/common/tenantList.h"                         // tenant0
#include "orionld/types/OrionldAlteration.h"                   // OrionldAlteration
#include "orionld/payloadCheck/PCHECK.h"                       // PCHECK_*
#include "orionld/payloadCheck/pCheckUri.h"                    // pCheckUri
#include "orionld/payloadCheck/pCheckEntity.h"                 // pCheckEntity
#include "orionld/context/orionldContextFromTree.h"            // orionldContextFromTree
#include "orionld/kjTree/kjTreeLog.h"                          // kjTreeLog
#include "orionld/dbModel/dbModelFromApiEntity.h"              // dbModelFromApiEntity
#include "orionld/dbModel/dbModelToApiEntity.h"                // dbModelToApiEntity
#include "orionld/dbModel/dbModelFromApiAttribute.h"           // dbModelFromApiAttribute
#include "orionld/mongoc/mongocEntitiesQuery.h"                // mongocEntitiesQuery
#include "orionld/mongoc/mongocEntitiesUpsert.h"               // mongocEntitiesUpsert
#include "orionld/legacyDriver/legacyPostBatchUpsert.h"        // legacyPostBatchUpsert
#include "orionld/notifications/orionldAlterations.h"          // orionldAlterations
#include "orionld/serviceRoutines/orionldPostBatchUpsert.h"    // Own interface



// ----------------------------------------------------------------------------
//
// entityCountAndFirstCheck -
//
static int entityCountAndFirstCheck(KjNode* requestTree, KjNode* errorsArrayP)
{
  KjNode*  eP = requestTree->value.firstChildP;
  KjNode*  next;
  int      noOfEntities = 0;

  while (eP != NULL)  // Count the number of items in the array, and basic first validity check of entities
  {
    next = eP->next;

    if (eP->type != KjObject)
    {
      entityErrorPush(errorsArrayP, "No Entity::id", OrionldBadRequestData, "Invalid Entity", "must be a JSON Object", 400, false);
      kjChildRemove(orionldState.requestTree, eP);
      eP = next;
      continue;
    }

    if (eP->value.firstChildP == NULL)
    {
      entityErrorPush(errorsArrayP, "No Entity::id", OrionldBadRequestData, "Empty Entity", "must be a non-empty JSON Object", 400, false);
      kjChildRemove(orionldState.requestTree, eP);
      eP = next;
      continue;
    }

    ++noOfEntities;
    eP = next;
  }

  return noOfEntities;
}



// ----------------------------------------------------------------------------
//
// entityStringArrayPopulate -
//
static int entityStringArrayPopulate(KjNode* requestTree, StringArray* eIdArrayP, KjNode* errorsArrayP)
{
  int     idIndex = 0;
  KjNode* eP      = requestTree->value.firstChildP;
  KjNode* next;

  while (eP != NULL)
  {
    next = eP->next;

    KjNode* idNodeP = kjLookup(eP, "id");

    if (idNodeP == NULL)
    {
      idNodeP = kjLookup(eP, "@id");

      if (idNodeP == NULL)
      {
        entityErrorPush(errorsArrayP, "No Entity::id", OrionldBadRequestData, "Mandatory field missing", "Entity::id", 400, false);
        kjChildRemove(orionldState.requestTree, eP);
        eP = next;
        continue;
      }

      idNodeP->name = (char*) "id";  // From this point, there are no @id, only id
    }

    if (idNodeP->type != KjString)
    {
      entityErrorPush(errorsArrayP, "No Entity::id", OrionldBadRequestData, "Invalid JSON type", "Entity::id", 400, false);
      kjChildRemove(orionldState.requestTree, eP);
      eP = next;
      continue;
    }

    if (pCheckUri(idNodeP->value.s, NULL, true) == false)
    {
      entityErrorPush(errorsArrayP, idNodeP->value.s, OrionldBadRequestData, "Invalid URI", "Entity::id", 400, false);
      kjChildRemove(orionldState.requestTree, eP);
      eP = next;
      continue;
    }

    eIdArrayP->array[idIndex++] = idNodeP->value.s;
    eP = next;
  }

  eIdArrayP->items = idIndex;
  return eIdArrayP->items;
}



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
// entitiesFinalCheck -
//
static int entitiesFinalCheck(KjNode* requestTree, KjNode* errorsArrayP, KjNode* dbEntityArray, bool update)
{
  int      noOfEntities   = 0;
  KjNode*  eP             = requestTree->value.firstChildP;
  KjNode*  next;
  KjNode*  creationArrayP = kjArray(orionldState.kjsonP, NULL);  // Array of entity id+type of entities that are to be created

  while (eP != NULL)
  {
    next = eP->next;

    KjNode*         idNodeP      = kjLookup(eP, "id");  // entityStringArrayPopulate makes sure that "id" exists
    char*           entityId     = idNodeP->value.s;
    KjNode*         contextNodeP = kjLookup(eP, "@context");
    OrionldContext* contextP     = NULL;

    //
    // If Content-Type is application/ld+json, then every entity must have an @context
    // If instead the Content-Type is application/json then an @context cannot be present
    //
    if (orionldState.in.contentType == JSONLD)
    {
      if (contextNodeP == NULL)
      {
        LM_E(("Content-Type is 'application/ld+json', but no @context found for entity '%s'", entityId));
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Invalid payload", "Content-Type is 'application/ld+json', but no @context in payload data array item", 400, false);
        kjChildRemove(orionldState.requestTree, eP);
        eP = next;
        continue;
      }

      contextP = orionldContextFromTree(NULL, OrionldContextFromInline, NULL, contextNodeP);
      if (contextP == NULL)
      {
        LM_E(("orionldContextFromTree reports error: %s: %s", orionldState.pd.title, orionldState.pd.detail));
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, orionldState.pd.title, orionldState.pd.detail, orionldState.pd.status, false);
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
        LM_E(("Content-Type is 'application/json', and an @context is present in the payload data array item of entity '%s'", entityId));
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Invalid payload", "Content-Type is 'application/json', and an @context is present in the payload data array item", 400, false);
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
      entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, orionldState.pd.title, orionldState.pd.detail, orionldState.pd.status, false);
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
      if (entityTypeCheck(dbEntityTypeNodeP->value.s, eP) == false)
      {
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Invalid Entity", "the Entity Type cannot be altered", 400, false);
        kjChildRemove(orionldState.requestTree, eP);
        eP = next;
        continue;
      }
    }
    else
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
        if (entityTypeCheck(oldType, eP) == false)
        {
          entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Invalid Entity", "the Entity Type cannot be altered", 400, false);
          kjChildRemove(orionldState.requestTree, eP);
          eP = next;
          continue;
        }
      }
      else if (oldType == NULL)
      {
        entityErrorPush(errorsArrayP, entityId, OrionldBadRequestData, "Invalid Entity", "no type in incoming payload for CREATION of Entity", 400, false);
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



// -----------------------------------------------------------------------------
//
// alteration -
//
void alteration(char* entityId, char* entityType, KjNode* apiEntityP, KjNode* incomingP)
{
  OrionldAlteration* alterationP = (OrionldAlteration*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlteration));

  if (entityType == NULL)
  {
    KjNode* typeP = kjLookup(apiEntityP, "type");

    if (typeP != NULL)
      entityType = typeP->value.s;
  }

  alterationP->entityId          = entityId;
  alterationP->entityType        = entityType;
  alterationP->inEntityP         = incomingP;
  alterationP->dbEntityP         = NULL;
  alterationP->finalApiEntityP   = apiEntityP;
  alterationP->alteredAttributes = 0;
  alterationP->alteredAttributeV = NULL;

  // Link it into the list
  alterationP->next        = orionldState.alterations;
  orionldState.alterations = alterationP;

  LM(("MI: Added alteration for entity '%s'", entityId));
}



// -----------------------------------------------------------------------------
//
// dbEntityFields -
//
// FIXME: Move dbEntityFields from orionldPatchEntity2.cpp to orionld/common/dbEntityFields
//
extern bool dbEntityFields(KjNode* dbEntityP, const char* entityId, char** entityTypeP, KjNode** attrsPP);



// ----------------------------------------------------------------------------
//
// entityListWithIdAndType -
//
//   HERE dbEntityArray is "cloned" into the format dbEntityListLookupWithIdTypeCreDate() gives back
//   [
//     {
//       "id": "urn:E1",
//       "type": "T"
//     },
//     ...
//   ]
//   orionldState.batchEntities is set to point to the array (used in troePostBatchUpsert)
//
static KjNode* entityListWithIdAndType(KjNode* dbEntityArray)
{
  KjNode* entityArray = kjArray(orionldState.kjsonP, NULL);

  for (KjNode* dbEntityP = dbEntityArray->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
  {
    KjNode* _idP = kjLookup(dbEntityP, "_id");

    if (_idP == NULL)
    {
      LM_E(("Database Error (unable to extract _id field from entity in database)"));
      continue;
    }

    KjNode* dbIdNodeP   = kjLookup(_idP, "id");
    KjNode* dbTypeNodeP = kjLookup(_idP, "type");

    if ((dbIdNodeP == NULL) || (dbTypeNodeP == NULL))
    {
      LM_E(("Database Error (unable to extract id/type fields from entity::_id in database)"));
      continue;
    }

    KjNode* entityP    = kjObject(orionldState.kjsonP, NULL);
    KjNode* idNodeP    = kjString(orionldState.kjsonP, "id",   dbIdNodeP->value.s);
    KjNode* typeNodeP  = kjString(orionldState.kjsonP, "type", dbTypeNodeP->value.s);

    kjChildAdd(entityP, idNodeP);
    kjChildAdd(entityP, typeNodeP);

    kjChildAdd(entityArray, entityP);
    LM(("X2: Inserted entity '%s' in the entityIdAndTypeArray", dbIdNodeP->value.s));
  }

  kjTreeLog(entityArray, "X2: entityIdAndTypeArray");
  return entityArray;
}



// ----------------------------------------------------------------------------
//
// kjConcatenate - move all children from srcP to the end of destP)
//
static KjNode* kjConcatenate(KjNode* destP, KjNode* srcP)
{
  if (destP->value.firstChildP == NULL)
    destP->value.firstChildP = srcP->value.firstChildP;
  else
    destP->lastChild->next   = srcP->value.firstChildP;

  destP->lastChild = srcP->lastChild;

  // Empty srcP
  srcP->value.firstChildP = NULL;
  srcP->lastChild         = NULL;

  return destP;
}



// -----------------------------------------------------------------------------
//
// multipleInstances -
//
// IMPORTANT NOTE
//   The entity type cannot be modified (well, not until multi-typing is implemented).
//   The "entity-type-change check" is already implemented - for entities that existed in the DB prior to the batch upsert request.
//   BUT, what if the entity does not exist, but we have more than one instance of an entity ?
//
//   Well, in such case, the first instance creates the entity (AND DEFINES THE ENTITY TYPE).
//   Instances after this first ("creating") instance MUST have the same entity type. If not, they're erroneous.
//   - Erroneous instances must be removed also for TRoE
//
static bool multipleInstances(const char* entityId, KjNode* updatedArrayP, KjNode* createdArrayP, bool* entityIsNewP)
{
  *entityIsNewP = false;

  for (KjNode* itemP = updatedArrayP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (strcmp(entityId, itemP->value.s) == 0)
      return true;
  }

  for (KjNode* itemP = createdArrayP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (strcmp(entityId, itemP->value.s) == 0)
    {
      *entityIsNewP = true;
      return true;
    }
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// batchCreateEntity -
//
// * An entity in DB-Model format is returned
// * 'inEntityP' is left untouched
//
static KjNode* batchCreateEntity(KjNode* inEntityP, char* entityId, char* entityType)
{
  KjNode* dbFinalEntityP = kjClone(orionldState.kjsonP, inEntityP);  // Starts out as API-Entity but dbModelFromApiEntity makes it a DB-Entity

  if (dbModelFromApiEntity(dbFinalEntityP, NULL, true, entityId, entityType) == false)
    return NULL;

  return dbFinalEntityP;
}



// -----------------------------------------------------------------------------
//
// batchReplaceEntity -
//
// About the current state in mongo -
//   The only difference between BATCH Create and BATCH Replace is that
//   the Entity creDate needs to stay intact in the latter
//
// About Notifications
//   We're REPLACING an entire entity, so, some attributes that existed may disappear
//   This should be notified.
//   Having the "Original DB Entity", it's easy to figure out which attributes are removed in the REPLACE operation
//
static KjNode* batchReplaceEntity(KjNode* inEntityP, char* entityId, char* entityType, double entityCreDate)
{
  KjNode* dbFinalEntityP = kjClone(orionldState.kjsonP, inEntityP);  // Starts out as API-Entity but dbModelFromApiEntity makes it a DB-Entity

  if (dbModelFromApiEntity(dbFinalEntityP, NULL, true, entityId, entityType) == false)
    return NULL;

  //
  // Fix the entity's creDate (from the version of the entity that was fouind in the database)
  //
  KjNode* creDateNodeP = kjLookup(dbFinalEntityP, "creDate");
  if (creDateNodeP != NULL)
    creDateNodeP->value.f = entityCreDate;
  else
  {
    creDateNodeP = kjFloat(orionldState.kjsonP, "creDate", entityCreDate);
    kjChildAdd(dbFinalEntityP, creDateNodeP);
  }

  return dbFinalEntityP;
}



// -----------------------------------------------------------------------------
//
// batchUpdateEntity -
//
// Take the Original DB Entity and add/remove from it (not sure if we need to clone it - it might noe be used after this)
// Loop over inEntityP, all the attributes
//   * Transform API Attribute into DB-Attribute (dbModelToDbAttribute)
//   * if attr in "Original DB Entity"
//       remove it from "Original DB Entity"
//     else
//       add attr name to "attrNames"
//   * Add DB-Attribute to "Original DB Entity"
//   * Return "Original DB Entity"
//   * The creDate is already OK
//   * The modDate needs to be updated
//
static KjNode* batchUpdateEntity(KjNode* inEntityP, KjNode* originalDbEntityP, char* entityId, char* entityType)
{
  KjNode* dbFinalEntityP = kjClone(orionldState.kjsonP, originalDbEntityP);

  //
  // "attr" member of the DB Entity - might not be present (stupid DB Model !!!)
  //
  KjNode* dbAttrsP     = kjLookup(dbFinalEntityP, "attrs");
  KjNode* dbAttrNamesP = kjLookup(dbFinalEntityP, "attrNames");

  // No "attrs" in the entity in the database? - normal, if the entity has no attributes
  if (dbAttrsP == NULL)
  {
    dbAttrsP = kjObject(orionldState.kjsonP, "attrs");
    kjChildAdd(dbFinalEntityP, dbAttrsP);
  }

  // No "attrNames" in the entity in the database? - that should not happen
  if (dbAttrNamesP == NULL)
  {
    dbAttrNamesP = kjArray(orionldState.kjsonP, "attrNames");
    kjChildAdd(dbFinalEntityP, dbAttrsP);
  }

  for (KjNode* apiAttrP = inEntityP->value.firstChildP; apiAttrP != NULL; apiAttrP = apiAttrP->next)
  {
    if (strcmp(apiAttrP->name, "id")   == 0) continue;
    if (strcmp(apiAttrP->name, "type") == 0) continue;

    char eqAttrName[512];
    strncpy(eqAttrName, apiAttrP->name, sizeof(eqAttrName) - 1);
    dotForEq(eqAttrName);
    KjNode* dbAttrP = kjLookup(dbAttrsP, eqAttrName);

    if (dbAttrP != NULL)  // The attribute already existed - we remove it before the new version of the attribute is added
      kjChildRemove(dbAttrsP, dbAttrP);
    else
    {
      // The attribute is to be ADDED, so it must be added to "attrNames" (with dots, not eq)
      KjNode* attrNameNodeP = kjString(orionldState.kjsonP, NULL, apiAttrP->name);
      kjChildAdd(dbAttrNamesP, attrNameNodeP);
    }

    //
    // Transforming the attribute into DB Model and adding it to "attrs"
    // ".added" and ".removed" ...   need to look into this !!!
    //
    KjNode* attrAddedV   = kjArray(orionldState.kjsonP, ".added");
    KjNode* attrRemovedV = kjArray(orionldState.kjsonP, ".removed");

    dbAttrP = kjClone(orionldState.kjsonP, apiAttrP);  // Copy the API attribute and then transform it into DB Model

    dbModelFromApiAttribute(dbAttrP, dbAttrsP, attrAddedV, attrRemovedV, NULL);
    kjChildAdd(dbAttrsP, dbAttrP);
  }

  //
  // Update the entity's modDate (or create it if it for some obscure reason is missing!)
  //
  KjNode* modDateNodeP = kjLookup(dbFinalEntityP, "modDate");
  if (modDateNodeP != NULL)
    modDateNodeP->value.f = orionldState.requestTime;
  else
  {
    modDateNodeP = kjFloat(orionldState.kjsonP, "modDate", orionldState.requestTime);
    kjChildAdd(dbFinalEntityP, modDateNodeP);
  }

  return dbFinalEntityP;
}



// ----------------------------------------------------------------------------
//
// orionldPostBatchUpsert -
//
// Still to implement
// - datasetId  (don't want to ...)
// - More than one instance of a specific entity in the array
// - Forwarding (need new registrations for that)
//
bool orionldPostBatchUpsert(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))  // If Legacy header - use old implementation
    return legacyPostBatchUpsert();

  //
  // Prerequisites for the payload in orionldState.requestTree:
  // * must be an array with objects
  // * cannot be empty
  // * all entities must contain an entity::id (one level down)
  // * no entity can contain an entity::type (one level down)
  //
  PCHECK_ARRAY(orionldState.requestTree,       0, NULL, "payload body must be a JSON Array",           400);
  PCHECK_ARRAY_EMPTY(orionldState.requestTree, 0, NULL, "payload body must be a non-empty JSON Array", 400);

  //
  // Prerequisites for URI params:
  // * both 'update' and 'replace' cannot be set in options (replace is default)
  //
  if ((orionldState.uriParamOptions.update == true) && (orionldState.uriParamOptions.replace == true))
  {
    orionldError(OrionldBadRequestData, "URI Param Error", "options: both /update/ and /replace/ present", 400);
    return false;
  }


  //
  // By default, already existing entitiers are OVERWRITTEN.
  // If ?options=update is used, then already existing entities are to be updated, and in such case we need to
  // extract those to-be-updated entities from the database (the updating algorithm here is according to "Append Attributes).
  // However, for subscriptions, we'll need the old values anyways to help decide whether any notifications are to be sent.
  // So, no other way around it than to extract all the entitiers from the DB :(
  //
  // For the mongoc query, we need a StringArray for the entity IDs, and to set up the StringArray we need to know the number of
  // entities.
  //
  // Very basic error checking is performed in this first loop to count the number of entities in the array.
  // entityCountAndFirstCheck() takes care of that.
  //
  KjNode*      outArrayErroredP = kjArray(orionldState.kjsonP, "errors");
  int          noOfEntities     = entityCountAndFirstCheck(orionldState.requestTree, outArrayErroredP);

  LM(("Number of valid Entities after 1st check-round: %d", noOfEntities));


  //
  // Now that we know the max number of entities (some may drop out after calling pCheckEntity - part of entitiesFinalCheck),
  // we can create the StringArray with the Entity IDs
  //
  StringArray  eIdArray;

  eIdArray.items = noOfEntities;
  eIdArray.array = (char**) kaAlloc(&orionldState.kalloc, sizeof(char*) * noOfEntities);

  if (eIdArray.array == NULL)
  {
    orionldError(OrionldInternalError, "Out of memory", "allocating StringArray for entity ids", 500);
    return false;
  }


  //
  // We have the StringArray (eIdArray), so, now we can loop through the incoming array of entities and populate eIdArray
  // (extract the entity ids) later to be used by mongocEntitiesQuery().
  //
  noOfEntities = entityStringArrayPopulate(orionldState.requestTree, &eIdArray, outArrayErroredP);
  LM(("Number of valid Entities after 2nd check-round: %d", noOfEntities));


  //
  // The entity id array is ready - time to query mongo
  //
  KjNode* dbEntityArray = mongocEntitiesQuery(NULL, &eIdArray, NULL, NULL, NULL, NULL, NULL, NULL);
  if (dbEntityArray == NULL)
  {
    orionldError(OrionldInternalError, "Database Error", "error querying the database for entities", 500);
    return false;
  }

  //
  // Finally we have everything we need to 100% CHECK the incoming entities
  //
  noOfEntities = entitiesFinalCheck(orionldState.requestTree, outArrayErroredP, dbEntityArray, orionldState.uriParamOptions.update);
  LM(("Number of valid Entities after 3rd check-round: %d", noOfEntities));

  if (troe)
    orionldState.batchEntities = entityListWithIdAndType(dbEntityArray);  // For TRoE - but, a new method should be found

  KjNode* outArrayCreatedP  = kjArray(orionldState.kjsonP, "created");
  KjNode* outArrayUpdatedP  = kjArray(orionldState.kjsonP, "updated");

  //
  // Looping over all the accepted entities
  //
  // 1. orionldState.requestTree must remain untouched (as it came out of pCheckEntity)
  //
  // 2. inEntityP            - current entity in orionldState.requestTree
  // 3. originalDbEntityP    - as the entity was in the DB before this request
  // 4. originalApiEntityP   - as the entity was in API format before this request  (not sure it's necessary)
  // 5. finalApiEntityP      - merged entity, in API format
  // 6. finalDbEntityP       - as is goes to mongoc
  // 7. dbCreateArray        - array of finalDbEntityP that are to be created - for mongoc
  // 8. dbUpdateArray        - array of finalDbEntityP that are to be updated - for mongoc
  //
  // TRoE needs         incomingEntityP
  // Alteration needs   incomingEntityP (to check for matching subscriptions)
  // Alteration needs   finalApiEntityP (for the notification - filter attrs etc)
  // mongoc needs       finalDbEntityP
  //
  KjNode* dbCreateArray = kjArray(orionldState.kjsonP, NULL);
  KjNode* dbUpdateArray = kjArray(orionldState.kjsonP, NULL);

  KjNode* next;
  KjNode* inEntityP = orionldState.requestTree->value.firstChildP;

  while (inEntityP != NULL)
  {
    next = inEntityP->next;

    KjNode*  idNodeP            = kjLookup(inEntityP, "id");    // pCheckEntity assures "id" is present (as a String and not named "@id")
    KjNode*  typeNodeP          = kjLookup(inEntityP, "type");  // pCheckEntity assures "type" if present is a String and not named "@type"
    char*    entityId           = idNodeP->value.s;
    char*    entityType         = (typeNodeP != NULL)? typeNodeP->value.s : NULL;
    KjNode*  originalDbEntityP  = entityLookupBy_id_Id(dbEntityArray, entityId, NULL);
    KjNode*  finalDbEntityP;
    bool     entityIsNew;

    if (multipleInstances(entityId, outArrayUpdatedP, outArrayCreatedP, &entityIsNew) == true)
    {
      LM_W(("--------------------------------------------------------------------------------"));
      LM_W(("Multiple Instances of an Entity in New BATCH Upsert is not yet implemented"));
      LM_W(("For now, the first instance is used and the rest of the instances are ignored"));
      LM_W(("--------------------------------------------------------------------------------"));

      if (entityIsNew == true)
      {
        //
        // The entity is created by a previous instance in the incoming entity array
        // Can't be created twice
        //
        entityErrorPush(outArrayErroredP, entityId, OrionldBadRequestData, "Entity already exists", "Created by a previous instance", 409, true);
        kjChildRemove(orionldState.requestTree, inEntityP);
        inEntityP = next;
        continue;
      }

      inEntityP = next;
      continue;  // All is good - I just need to implement the "multiple instance treatment"
    }

    if (originalDbEntityP == NULL)  // The entity did not exist before - CREATION
    {
      finalDbEntityP = batchCreateEntity(inEntityP, entityId, entityType);

      if (finalDbEntityP != NULL)
      {
        kjChildAdd(dbCreateArray, finalDbEntityP);
        entitySuccessPush(outArrayCreatedP, entityId);
      }
    }
    else
    {
      if (orionldState.uriParamOptions.update == false)
      {
        KjNode* entityCreDateNodeP = kjLookup(originalDbEntityP, "creDate");  // I'd really prefer dbModelFromApiEntity to fix "creDate"
        double  entityCreDate      = (entityCreDateNodeP != NULL)? entityCreDateNodeP->value.f : orionldState.requestTime;

        finalDbEntityP = batchReplaceEntity(inEntityP, entityId, entityType, entityCreDate);
      }
      else
        finalDbEntityP = batchUpdateEntity(inEntityP, originalDbEntityP, entityId, entityType);

      if (finalDbEntityP != NULL)
      {
        kjChildAdd(dbUpdateArray, finalDbEntityP);
        entitySuccessPush(outArrayUpdatedP, entityId);
      }
    }

    if (finalDbEntityP == NULL)
    {
      inEntityP = next;
      continue;
    }

    //
    // Alterations need the complete API entity (I might change that for the complete DB Entity ...)
    // dbModelToApiEntity is DESTRUCTIVE, so I need to clone the 'finalDbEntityP' first
    //
    KjNode* dbEntityCopy    = kjClone(orionldState.kjsonP, finalDbEntityP);
    KjNode* finalApiEntityP = dbModelToApiEntity(dbEntityCopy, false, entityId);

    kjTreeLog(finalApiEntityP, "MI: After dbModelToApiEntity");
    alteration(entityId, entityType, finalApiEntityP, inEntityP);

    inEntityP = next;
  }

  //
  // Any correct Entity to be created/updated??
  //
  if ((dbCreateArray->value.firstChildP != NULL) || (dbUpdateArray->value.firstChildP != NULL))
  {
    int r = mongocEntitiesUpsert(dbCreateArray, dbUpdateArray);

    if (r == false)
    {
      orionldError(OrionldInternalError, "Database Error", "mongocEntitiesUpsert failed", 500);
      return false;
    }
  }

  //
  // Returning the three arrays, or not ...
  //
  // 1. The broker returns 201 if there are no errors
  //
  //    - updatedArrayP EMPTY
  //    - createdArrayP NOT EMPTY
  //
  // 2. The broker returns 204 if all entities have been updated:
  //    - errorsArrayP  EMPTY
  //    - createdArrayP EMPTY
  //    - updatedArrayP NOT EMPTY
  //
  // 3. Else, 207 is returned
  //
  bool    noErrors    = (outArrayErroredP->value.firstChildP == NULL);
  bool    noCreations = (outArrayCreatedP->value.firstChildP == NULL);

  if (noErrors)
  {
    if (noCreations)  // Only Updates - 204
    {
      orionldState.httpStatusCode = 204;
      orionldState.responseTree   = NULL;
    }
    else  // Some Creations (and perhaps Updates) - 201 with creation array as body (and ignoring the updates)
    {
      orionldState.httpStatusCode = 201;
      orionldState.responseTree   = outArrayCreatedP;
    }
  }
  else  // There are errors - 207
  {
    KjNode* response = kjObject(orionldState.kjsonP, NULL);

    orionldState.httpStatusCode  = 207;

    kjConcatenate(outArrayCreatedP, outArrayUpdatedP);
    outArrayCreatedP->name = (char*) "success";
    kjChildAdd(response, outArrayCreatedP);
    kjChildAdd(response, outArrayErroredP);
    orionldState.responseTree = response;
  }

  orionldState.out.contentType = JSON;
  orionldState.noLinkHeader    = true;

  if ((orionldState.tenantP != &tenant0) && (orionldState.httpStatusCode != 204))
    orionldHeaderAdd(&orionldState.out.headers, HttpTenant, orionldState.tenantP->tenant, 0);

  return true;
}
