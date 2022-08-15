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



// ----------------------------------------------------------------------------
//
// entitiesFinalCheck -
//
static int entitiesFinalCheck(KjNode* requestTree, KjNode* errorsArrayP, KjNode* dbEntityArray, bool update)
{
  int     noOfEntities = 0;
  KjNode* eP           = requestTree->value.firstChildP;
  KjNode* next;

  while (eP != NULL)
  {
    next = eP->next;

    KjNode*         idNodeP      = kjLookup(eP, "id");  // entityStringArrayPopulate makes sure that "id" exists
    char*           entityId     = idNodeP->value.s;
    KjNode*         contextNodeP = kjLookup(eP, "@context");
    OrionldContext* contextP     = NULL;

    if (contextNodeP != NULL)
      contextP = orionldContextFromTree(NULL, OrionldContextFromInline, NULL, contextNodeP);

    if (contextP != NULL)
      orionldState.contextP = contextP;

    KjNode* dbAttrsP = NULL;
    if (update == true)
    {
      KjNode* dbEntityP = entityLookupById(dbEntityArray, entityId);
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

    ++noOfEntities;
    LM(("Got Entity '%s'", entityId));
    eP = next;
  }

  return noOfEntities;
}



// -----------------------------------------------------------------------------
//
// attributeCreDateFromDbAttr -
//
// There's really no merge to be done - the new attribute overwrites the old one.
// Just, the old creDate needs to be preserved.
//
static void attributeCreDateFromDbAttr(KjNode* attrP, KjNode* dbAttrP)
{
  KjNode* dbCreDateP = kjLookup(dbAttrP, "creDate");

  if (dbCreDateP != NULL)
    kjChildRemove(dbAttrP, dbCreDateP);
  else
    dbCreDateP = kjFloat(orionldState.kjsonP, "creDate", orionldState.requestTime);

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
  for (KjNode* apiAttrP = apiEntityP->value.firstChildP; apiAttrP != NULL; apiAttrP = apiAttrP->next)
  {
    if (strcmp(apiAttrP->name, "id")   == 0)  { LM(("UP: skipping 'id'"));   continue; }
    if (strcmp(apiAttrP->name, "type") == 0)  { LM(("UP: skipping 'type'")); continue; }

    char    eqAttrName[512];
    char*   dotAttrName = apiAttrP->name;

    strncpy(eqAttrName, apiAttrP->name, sizeof(eqAttrName) - 1);
    dotForEq(eqAttrName);
    KjNode* dbAttrP = kjLookup(dbAttrsP, eqAttrName);

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
  }
  LM(("UP: ---------- Looped over API-Entity attributes"));

  kjChildAdd(apiEntityP, attrAddedV);
  kjChildAdd(apiEntityP, attrRemovedV);
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
      attributeCreDateFromDbAttr(apiAttrP, dbAttrP);
    }
  }
}



// -----------------------------------------------------------------------------
//
// alteration -
//
static void alteration(char* entityId, char* entityType, KjNode* apiEntityP)
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
  alterationP->patchTree         = NULL;        // For new, more fine-granulated notifications
  alterationP->dbEntityP         = NULL;
  alterationP->patchedEntity     = apiEntityP;
  alterationP->alteredAttributes = 0;
  alterationP->alteredAttributeV = NULL;
  alterationP->next              = orionldState.alterations;

  kjTreeLog(alterationP->patchedEntity, "NOTIF: patchedEntity");
  orionldState.alterations = alterationP;
}



// -----------------------------------------------------------------------------
//
// entityTypeCheck -
//
static bool entityTypeCheck(KjNode* dbEntityTypeNodeP, KjNode* entityP)
{
  KjNode* newEntityTypeNodeP = kjLookup(entityP, "type");

  if (newEntityTypeNodeP == NULL)
    newEntityTypeNodeP = kjLookup(entityP, "@type");

  if (newEntityTypeNodeP != NULL)
  {
    if (strcmp(newEntityTypeNodeP->value.s, dbEntityTypeNodeP->value.s) != 0)
      LM_RE(false, ("Attempt to change Entity Type"));
  }
  else
  {
    // No Entity Type present in the incoming payload - no problem, I'll just add it
    KjNode* entityTypeP = kjString(orionldState.kjsonP, "type", dbEntityTypeNodeP->value.s);
    kjChildAdd(entityP, entityTypeP);
  }

  return true;
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

  LM(("Number of valid Entities after first check-round: %d", noOfEntities));


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
  LM(("Number of valid Entities after second check-round: %d", noOfEntities));


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
  LM(("Number of valid Entities after second check-round: %d", noOfEntities));

  KjNode* outArrayCreatedP  = kjArray(orionldState.kjsonP, "created");
  KjNode* outArrayUpdatedP  = kjArray(orionldState.kjsonP, "updated");


  // One Array for entities that did not priorly exist  (creation)
  // Another array for already existing entities        (replacement)
  //
  KjNode* creationArray = orionldState.requestTree;
  KjNode* updateArray   = kjArray(orionldState.kjsonP, NULL);

  //
  // Merge new entity data into "old" DB entity data
  //
  KjNode* entityP = orionldState.requestTree->value.firstChildP;
  KjNode* next;

  // kjTreeLog(dbEntityArray, "dbEntityArray");
  kjTreeLog(dbEntityArray, "AN: Entities from DB");
  while (entityP != NULL)
  {
    next = entityP->next;

    KjNode* idNodeP                = kjLookup(entityP, "id");
    char*   entityId               = idNodeP->value.s;
    KjNode* dbEntityTypeNodeP      = NULL;
    KjNode* dbEntityP              = entityLookupBy_id_Id(dbEntityArray, entityId, &dbEntityTypeNodeP);
    bool    creation               = true;
    KjNode* apiEntityP             = NULL;
    bool    alreadyInDbModelFormat = false;

    LM(("AN: dbEntityP for entity '%s' at %p", entityId, dbEntityP));
    if (dbEntityP != NULL)
    {
      kjTreeLog(dbEntityP, "AN: dbEntityP");
      //
      // The entity already exists (and it has an Entity Type)
      // The entity type cannot be modified, so ... need to check that
      //
      if (entityTypeCheck(dbEntityTypeNodeP, entityP) == false)
      {
        entityErrorPush(outArrayErroredP, entityId, OrionldBadRequestData, "Invalid Entity", "the Entity Type cannot be altered", 400, false);
        kjChildRemove(orionldState.requestTree, entityP);
        entityP = next;
        noOfEntities -= 1;
        continue;
      }

      entitySuccessPush(outArrayUpdatedP, entityId);
      kjChildRemove(creationArray, entityP);

      if (orionldState.uriParamOptions.update == false)
        entityMergeForReplace(entityP, dbEntityP);  // Resulting Entity is entityP
      else
      {
        // entityMergeForUpdate:
        // - DOES NOT modify entityP
        // - leaves the merged entity already in DB Model format - in dbEntityP
        entityMergeForUpdate(entityP, dbEntityP);
        entityP = dbEntityP;
        kjTreeLog(dbEntityP, "UP: Merged entity - for alterations/notifications");
        alreadyInDbModelFormat = true;
        KjNode* clonedDbEntityP = kjClone(orionldState.kjsonP, dbEntityP);
        apiEntityP = dbModelToApiEntity(clonedDbEntityP, false, entityId);  // For notifications we need the entire entity
      }

      // Adding entityP to the updateArray for mongocEntitiesUpsert, even though it will be transformed later by dbModelFromApiEntity
      kjChildAdd(updateArray, entityP);
    }
    else
      entitySuccessPush(outArrayCreatedP, entityId);

    //
    // * entityP is transformed info DB-Model for mongocEntitiesUpsert
    // * apiEntityP must stay API-Entity and is used for alterations (so, entityP is cloned before dbModelFromApiEntity)
    // * TRoE ... can probably use apiEntityP (wait, entityMergeForUpdate will destroy that ...)
    //
    if (apiEntityP == NULL)
      apiEntityP = kjClone(orionldState.kjsonP, entityP);

    // Transform the API entity (entityP) into the database model
    if (alreadyInDbModelFormat == false)
    {
      kjTreeLog(dbEntityP, "AN: dbEntityP before dbModelFromApiEntity");
      dbModelFromApiEntity(entityP, dbEntityP, creation, NULL, NULL);
    }

    kjTreeLog(apiEntityP, "ALT: API-Entity for Alteration/Notification");
    alteration(entityId, NULL, apiEntityP);
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
  // Returning the three arrays
  //
  KjNode* response = kjObject(orionldState.kjsonP, NULL);
  kjChildAdd(response, outArrayCreatedP);
  kjChildAdd(response, outArrayUpdatedP);
  kjChildAdd(response, outArrayErroredP);

  orionldState.httpStatusCode  = 207;
  orionldState.responseTree    = response;
  orionldState.out.contentType = JSON;

  return true;
}
