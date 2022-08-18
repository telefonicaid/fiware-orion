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
bool entityTypeCheck(const char* oldEntityType, KjNode* entityP)
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
// apiAttributeCreateadAtFromDb -
//
// There's really no merge to be done - the new attribute overwrites the old one.
// Just, the old creDate needs to be preserved.
//
static void apiAttributeCreateadAtFromDb(KjNode* attrP, KjNode* dbAttrP)
{
  KjNode* dbCreDateP = kjLookup(dbAttrP, "creDate");

  if (dbCreDateP != NULL)
  {
    kjChildRemove(dbAttrP, dbCreDateP);
    dbCreDateP->name = (char*) "createdAt";
  }
  else
    dbCreDateP = kjFloat(orionldState.kjsonP, "createdAt", orionldState.requestTime);

  kjChildAdd(attrP, dbCreDateP);
}



// ----------------------------------------------------------------------------
//
// entityMergeForUpdate -
//
// - DOES NOT modify entityP
// - leaves the merged entity already in DB Model format - in dbEntityP
//
// The merge for update keeps the attributes that are already in the DB and replaces those that
// are also in the API entity. New attributes are added.
// The result is in dbEntityP
//
// So:
// Loop over apiEntityP
//   - find attribute in DB entity
//   - if not found - add to DB entity ("attrs") + timestamps
//   - if found     - extract the creDate and "give to" the API Entity attribute,
//                    then transform to DB Model and add to "attrs" of DB-Entity
//   - Remember to update "attrNames"
//
static void entityMergeForUpdate(KjNode* apiEntityP, KjNode* dbEntityP)
{
  LM(("UP: In entityMergeForUpdate"));
  kjTreeLog(dbEntityP, "UP: DB Entity");
  kjTreeLog(apiEntityP, "UP: API Entity");

  //
  // "attr" member of the DB Entity - might not be present (stupid DB Model !!!)
  //
  KjNode* dbAttrsP     = kjLookup(dbEntityP, "attrs");
  KjNode* dbAttrNamesP = kjLookup(dbEntityP, "attrNames");

  if (dbAttrsP == NULL)  // No attributes in the entity in the database
  {
    dbAttrsP = kjObject(orionldState.kjsonP, "attrs");
    kjChildAdd(dbEntityP, dbAttrsP);
  }

  KjNode* attrAddedV   = kjArray(orionldState.kjsonP, ".added");
  KjNode* attrRemovedV = kjArray(orionldState.kjsonP, ".removed");

  kjTreeLog(apiEntityP, "UP: API Entity");
  LM(("UP: ---------- Looping over API-Entity attributes"));

  KjNode* apiAttrP = apiEntityP->value.firstChildP;
  KjNode* next;
  while (apiAttrP != NULL)
  {
    next = apiAttrP->next;

    if (strcmp(apiAttrP->name, "id")   == 0)  { LM(("UP: skipping 'id'"));   apiAttrP = next; continue; }
    if (strcmp(apiAttrP->name, "type") == 0)  { LM(("UP: skipping 'type'")); apiAttrP = next; continue; }

    char    eqAttrName[512];
    char*   dotAttrName = apiAttrP->name;

    strncpy(eqAttrName, apiAttrP->name, sizeof(eqAttrName) - 1);
    dotForEq(eqAttrName);
    KjNode* dbAttrP = kjLookup(dbAttrsP, eqAttrName);

    kjChildRemove(apiEntityP, apiAttrP);

    if (dbAttrP == NULL)  // The attribute is NEW and must be added to attrs and attrNames
    {
      LM(("UP: The attribute '%s' is NEW and must be added to attrs and attrNames", dotAttrName));
      KjNode* attrNameNode = kjString(orionldState.kjsonP, NULL, dotAttrName);
      kjChildAdd(dbAttrNamesP, attrNameNode);

      dbModelFromApiAttribute(apiAttrP, dbAttrsP, attrAddedV, attrRemovedV, NULL);  // ignoreP == NULL ...
      kjChildAdd(dbAttrsP, apiAttrP);  // apiAttrP is now in DB-Model format
    }
    else
    {
      LM(("UP: Attribute '%s' is replaced by the API Attribute", dotAttrName));
      kjChildRemove(dbAttrsP, dbAttrP);

      LM(("UP: Calling dbModelFromApiAttribute"));
      dbModelFromApiAttribute(apiAttrP, dbAttrsP, attrAddedV, attrRemovedV, NULL);  // ignoreP == NULL ...
      kjChildAdd(dbAttrsP, apiAttrP);  // apiAttrP is now in DB-Model format
    }

    apiAttrP = next;
  }

  LM(("UP: ---------- Looped over API-Entity attributes"));
}



// ----------------------------------------------------------------------------
//
// entityMergeForReplace -
//
// NOTE:
//   Built-in TIMESTAMPS (creDate/createdAt/modDate/modifiedAt) are taken care of by dbModelFromApi[Entity/Attribute/SubAttribute]()
//   EXCEPT for the entity creDate in case of update of an entity
//
static void entityMergeForReplace(KjNode* apiEntityP, KjNode* dbEntityP)
{
  KjNode* dbAttrsP = kjLookup(dbEntityP, "attrs");  // "attrs" is not present in the DB if the entity has no attributes ... ?

  for (KjNode* apiAttrP = apiEntityP->value.firstChildP; apiAttrP != NULL; apiAttrP = apiAttrP->next)
  {
    if (strcmp(apiAttrP->name, "id")    == 0)  continue;
    if (strcmp(apiAttrP->name, "type")  == 0)  continue;
    if (strcmp(apiAttrP->name, "@type") == 0)  continue;  // FIXME: @type -> type should be fixed already (by pCheckEntity)
    if (strcmp(apiAttrP->name, "scope") == 0)  continue;
    // FIXME: More special entity fields ...

    // It's not a special Entity field - must be an attribute
    char* attrEqName = kaStrdup(&orionldState.kalloc, apiAttrP->name);
    dotForEq(attrEqName);
    KjNode* dbAttrP = kjLookup(dbAttrsP, attrEqName);
    if (dbAttrP != NULL)
    {
      kjChildRemove(dbEntityP, dbAttrP);  // The attr is removed from the db-entity so we can later add all the rest of attrs
      apiAttributeCreateadAtFromDb(apiAttrP, dbAttrP);
    }
  }
}



// -----------------------------------------------------------------------------
//
// alteration -
//
static void alteration(char* entityId, char* entityType, KjNode* apiEntityP, KjNode* incomingP)
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
  alterationP->patchTree         = incomingP;
  alterationP->dbEntityP         = NULL;
  alterationP->patchedEntity     = apiEntityP;
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



// -----------------------------------------------------------------------------
//
// kjMemberCount
//
static int kjMemberCount(KjNode* arrayP)
{
  if ((arrayP->type != KjArray) && (arrayP->type != KjObject))
    return -1;

  int members = 0;

  for (KjNode* memberP = arrayP->value.firstChildP; memberP != NULL; memberP = memberP->next)
  {
    ++members;
  }

  return members;
}



// ----------------------------------------------------------------------------
//
// kjConcatenate - move all children from srcP to the end of destP)
//
KjNode* kjConcatenate(KjNode* destP, KjNode* srcP)
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



// ----------------------------------------------------------------------------
//
// orionldPostBatchUpsert -
//
// Still to implement
// - datasetId
// - Notifications - before merging entities (4)
// - More than one instance of a specific entity in the array
// - Merge updating entities (1)
// - Write to mongo (2)
// - Forwarding
// - TRoE
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
  kjTreeLog(dbEntityArray, "AN: Entities from DB");

  //
  // Finally we have everything we need to 100% CHECK the incoming entities
  //
  noOfEntities = entitiesFinalCheck(orionldState.requestTree, outArrayErroredP, dbEntityArray, orionldState.uriParamOptions.update);
  LM(("Number of valid Entities after 3rd check-round: %d", noOfEntities));

  KjNode* troeEntityArray = NULL;
  if (troe)
  {
    //
    //   The orionldState.requestTree that is now checked, expanded and in Normalized format must be cloned for TRoE
    //   Remember that the TRoE functions (troePostBatchUpsert) use orionldState.requestTree,
    //   so, the tree needs to be cloned before it is destroyed by XXX.
    //
    //   orionldState.requestTree is set to point to it just before leaving this function - for TRoE
    //
    troeEntityArray        = kjClone(orionldState.kjsonP, orionldState.requestTree);
    orionldState.batchEntities = entityListWithIdAndType(dbEntityArray);  // For TRoE
  }

  KjNode* outArrayCreatedP  = kjArray(orionldState.kjsonP, "created");
  KjNode* outArrayUpdatedP  = kjArray(orionldState.kjsonP, "updated");


  //
  // One Array for entities that did not priorly exist  (creation)
  // Another array for already existing entities        (replacement)
  //
  KjNode* creationArray = orionldState.requestTree;
  KjNode* updateArray   = kjArray(orionldState.kjsonP, NULL);

  //
  // Merge new entity data into "old" DB entity data


  //
  // Looping over all the accepted entities
  //
  // Different entity pointers (KjNode*):
  // - incomingP:  untouched entity, just as it came in      - for Alteration
  // - apiEntityP: merged with DB, but in NGSI-LD API format - for Notification
  // - dbEntityP:  merged with DB, in DB Model format        - for mongoc
  //
  //
  KjNode* entityP = orionldState.requestTree->value.firstChildP;
  KjNode* next;

  while (entityP != NULL)
  {
    next = entityP->next;

    KjNode* incomingP              = kjClone(orionldState.kjsonP, entityP);
    KjNode* idNodeP                = kjLookup(entityP, "id");
    char*   entityId               = idNodeP->value.s;
    KjNode* dbEntityTypeNodeP      = NULL;
    KjNode* dbEntityP              = entityLookupBy_id_Id(dbEntityArray, entityId, &dbEntityTypeNodeP);
    bool    creation               = true;
    KjNode* apiEntityP             = NULL;
    bool    alreadyInDbModelFormat = false;
    bool    alterationDone         = false;
    bool    entityIsNew            = false;

    if (multipleInstances(entityId, outArrayUpdatedP, outArrayCreatedP, &entityIsNew))
    {
      //
      // In case of multiple instances of one and the same entity:
      //
      // - options=replace (default):
      //   the newer (later in the array) entity completely replaces the old one.
      //   - Entity Type can't change
      //   - creDate stays
      //
      // - options=update
      //   the newer (later in the array) entity patches the previous by replacing attributes
      //   Need to keep patched the dbEntity but also the resulting API Entity (for Notifications)
      //
      KjNode* baseEntityP;

      if (entityIsNew == true)
      {
        kjTreeLog(outArrayCreatedP, "MI: outArrayCreatedP");
        baseEntityP = entityLookupById(outArrayCreatedP, entityId);
        LM(("MI: The entity '%s' has multiple instances, (creating, then updating entity at %p)", entityId, baseEntityP));
      }
      else
      {
        kjTreeLog(outArrayUpdatedP, "MI: outArrayUpdatedP");
        baseEntityP = entityLookupById(outArrayUpdatedP, entityId);
        LM(("MI: The entity '%s' has multiple instances (updating already existing entity at %p)", entityId, baseEntityP));
      }

      LM_W(("--------------------------------------------------------------------------------"));
      LM_W(("Multiple Instances of an Entity in New BATCH Upsert is not yet implemented"));
      LM_W(("For now, the first instance is used and the rest of the instances arte ignored"));
      LM_W(("--------------------------------------------------------------------------------"));
      entityP = next;
      continue;
    }
    else
      LM(("MI: The entity '%s' is to be %s", entityId, (dbEntityP == NULL)? "CREATED" : "UPDATED"));

    //
    // If the entity to be upserted is found in the DB, then it's not a Creation but un Update
    //
    if (dbEntityP != NULL)
    {
      entitySuccessPush(outArrayUpdatedP, entityId);
      kjChildRemove(creationArray, entityP);
      LM(("X2: Removed entity '%s' from creationArray", entityId));

      if (orionldState.uriParamOptions.update == false)
      {
        kjTreeLog(entityP, "Entity before entityMergeForReplace");
        entityMergeForReplace(entityP, dbEntityP);  // Resulting Entity is entityP and it's an API-Entity, not DB
        kjTreeLog(entityP, "Entity after entityMergeForReplace");
      }
      else
      {
        //
        // entityMergeForUpdate:
        //   * DOES NOT modify entityP
        //   * leaves the merged entity already in DB Model format - in dbEntityP
        //
        entityMergeForUpdate(entityP, dbEntityP);

        alreadyInDbModelFormat = true;
        KjNode* clonedDbEntityP = kjClone(orionldState.kjsonP, dbEntityP);
        apiEntityP = dbModelToApiEntity(clonedDbEntityP, false, entityId);  // For notifications we need the entire resulting entity

        //
        // Alteration for Update
        //
        char*               entityType  = dbEntityTypeNodeP->value.s;
        KjNode*             dbAttrsP    = kjLookup(dbEntityP, "attrs");
        OrionldAlteration*  alterationP = orionldAlterations(entityId, entityType, entityP, dbAttrsP, false);

        alterationP->dbEntityP         = kjClone(orionldState.kjsonP, dbEntityP);
        alterationP->patchedEntity     = apiEntityP;
        alterationP->patchTree         = incomingP;

        // Add the new alteration to the alteration-list (orionldState.alterations)
        alterationP->next              = orionldState.alterations;
        orionldState.alterations       = alterationP;

        alterationDone = true;

        entityP = dbEntityP;  // entityP is in DB-Model format AND alreadyInDbModelFormat is set
      }

      // Adding entityP to the updateArray for mongocEntitiesUpsert, even though it will be transformed later by dbModelFromApiEntity
      LM(("X2: Inserting '%s' in the updateArray", entityId));
      LM(("X2: No Of Members in dbEntityArray: %d", kjMemberCount(dbEntityArray)));

      kjChildRemove(dbEntityArray, dbEntityP);
      kjChildAdd(updateArray, entityP);  // Here dbEntityArray is modified

      LM(("X2: No Of Members in dbEntityArray: %d", kjMemberCount(dbEntityArray)));
      LM(("X2: Inserted '%s' in the updateArray", entityId));
    }
    else
      entitySuccessPush(outArrayCreatedP, entityId);

    //
    // * entityP is transformed info DB-Model for mongocEntitiesUpsert
    // * apiEntityP must stay API-Entity and is used for alterations (so, entityP is cloned before dbModelFromApiEntity for apiEntityP to not get destroyed)
    // * TRoE ... can probably use apiEntityP (wait, entityMergeForUpdate will destroy that ...)
    //
    if (apiEntityP == NULL)
      apiEntityP = kjClone(orionldState.kjsonP, entityP);

    // Transform the API entity (entityP) into the database model
    if (alreadyInDbModelFormat == false)
      dbModelFromApiEntity(entityP, dbEntityP, creation, NULL, NULL);

    if (alterationDone == false)
      alteration(entityId, NULL, apiEntityP, incomingP);
    entityP = next;
  }

  kjTreeLog(creationArray, "Create Array");
  kjTreeLog(updateArray, "Update Array");

  //
  // Any correct Entity to be created/updated??
  //
  if ((creationArray->value.firstChildP != NULL) || (updateArray->value.firstChildP != NULL))
  {
    int r = mongocEntitiesUpsert(creationArray, updateArray);

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

  if (orionldState.tenantP != &tenant0)
    orionldHeaderAdd(&orionldState.out.headers, HttpTenant, orionldState.tenantP->tenant, 0);


  if (troeEntityArray != NULL)
    orionldState.requestTree = troeEntityArray;

  return true;
}
