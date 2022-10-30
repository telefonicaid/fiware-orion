/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include "kjson/kjClone.h"                                     // kjClone
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjChildRemove, kjChildAdd, kjInteger
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/numberToDate.h"                       // numberToDate
#include "orionld/payloadCheck/PCHECK.h"                       // PCHECK_URI
#include "orionld/context/orionldContextItemAliasLookup.h"     // orionldContextItemAliasLookup
#include "orionld/regCache/regCacheItemLookup.h"               // regCacheItemLookup
#include "orionld/legacyDriver/legacyGetRegistration.h"        // legacyGetRegistration
#include "orionld/mongoc/mongocRegistrationGet.h"              // mongocRegistrationGet
#include "orionld/dbModel/dbModelToApiRegistration.h"          // dbModelToApiRegistration
#include "orionld/kjTree/kjTreeLog.h"                          // kjTreeLog
#include "orionld/serviceRoutines/orionldGetRegistration.h"    // Own Interface



// -----------------------------------------------------------------------------
//
// dbModelToApiTimestamp - from dbModelToApiRegistration.cpp
//
extern void dbModelToApiTimestamp(KjNode* tsP);



// -----------------------------------------------------------------------------
//
// apiModelFromDbInterval -
//
void apiModelFromDbInterval(KjNode* managementIntervalP)
{
  KjNode* startAtP = kjLookup(managementIntervalP, "startAt");
  KjNode* endAtP   = kjLookup(managementIntervalP, "endAt");

  if (startAtP != NULL)
    dbModelToApiTimestamp(startAtP);
  if (endAtP != NULL)
    dbModelToApiTimestamp(endAtP);
}



// ----------------------------------------------------------------------------
//
// apiModelFromCachedRegistration -
//
void apiModelFromCachedRegistration(KjNode* regTree, RegCacheItem* cachedRegP, bool sysAttrs)
{
  //
  // System Attributes
  //
  KjNode* createdAtP  = kjLookup(regTree, "createdAt");
  KjNode* modifiedAtP = kjLookup(regTree, "modifiedAt");

  if (sysAttrs == false)
  {
    // System Attributes NOT wanted - REMOVE them from the cloned copy of the registration from the reg-cache
    if (createdAtP != NULL)
      kjChildRemove(regTree, createdAtP);
    if (modifiedAtP != NULL)
      kjChildRemove(regTree, modifiedAtP);
  }
  else
  {
    // System Attributes WANTED - turn them into ISO8601
    if (createdAtP != NULL)
      dbModelToApiTimestamp(createdAtP);

    if (modifiedAtP != NULL)
      dbModelToApiTimestamp(modifiedAtP);
  }


  //
  // Counters: timesSent, timesFailed
  // ONLY added if non-zero
  //
  KjNode* timesSentP   = kjLookup(regTree, "timesSent");
  KjNode* timesFailedP = kjLookup(regTree, "timesFailed");

  if (timesSentP != NULL)
  {
    if ((timesSentP->value.i == 0) && (cachedRegP->deltas.timesSent == 0))
      kjChildRemove(regTree, timesSentP);
    else if (cachedRegP->deltas.timesSent > 0)
      timesSentP->value.i += cachedRegP->deltas.timesSent;
  }
  else if (cachedRegP->deltas.timesSent > 0)
  {
    timesSentP = kjInteger(orionldState.kjsonP, "timesSent", cachedRegP->deltas.timesSent);
    kjChildAdd(regTree, timesSentP);
  }

  if (timesFailedP != NULL)
  {
    if ((timesFailedP->value.i == 0) && (cachedRegP->deltas.timesFailed == 0))
      kjChildRemove(regTree, timesFailedP);
    else if (cachedRegP->deltas.timesFailed > 0)
      timesFailedP->value.i += cachedRegP->deltas.timesFailed;
  }
  else if (cachedRegP->deltas.timesFailed > 0)
  {
    timesFailedP = kjInteger(orionldState.kjsonP, "timesFailed", cachedRegP->deltas.timesFailed);
    kjChildAdd(regTree, timesFailedP);
  }


  //
  // Timestamps: lastSuccess, lastFailure
  // - if newer values found in "deltas" - use those timestamps instead
  // - If ZERO - remove it
  //
  KjNode* lastSuccessP  = kjLookup(regTree, "lastSuccess");
  KjNode* lastFailureP  = kjLookup(regTree, "lastFailure");

  if ((lastSuccessP != NULL) && (lastSuccessP->value.f < 1))
    kjChildRemove(regTree, lastSuccessP);    // FIXME: This is stupid ... ?   Why store it in cache iof it's not gonna be used?
  else
  {
    if (cachedRegP->deltas.lastSuccess > 0)  // Existing need update, or, creation
    {
      char* dateBuf = kaAlloc(&orionldState.kalloc, 64);
      numberToDate(cachedRegP->deltas.lastSuccess, dateBuf, 64);

      if (lastSuccessP != NULL)
      {
        lastSuccessP = kjString(orionldState.kjsonP, "lastSuccess", dateBuf);
        kjChildAdd(regTree, lastSuccessP);
      }
      else
      {
        // Change to ISO8601 string
        lastSuccessP->type    = KjString;
        lastSuccessP->value.s = dateBuf;
      }
    }
    else if (lastSuccessP != NULL)  // Change to ISO8601 string
      dbModelToApiTimestamp(lastSuccessP);
  }

  if ((lastFailureP != NULL) && (lastFailureP->value.f < 1))
    kjChildRemove(regTree, lastFailureP);
  else
  {
    if (cachedRegP->deltas.lastFailure > 0)  // Existing need update, or, creation
    {
      char* dateBuf = kaAlloc(&orionldState.kalloc, 64);
      numberToDate(cachedRegP->deltas.lastFailure, dateBuf, 64);

      if (lastFailureP != NULL)
      {
        lastFailureP = kjString(orionldState.kjsonP, "lastFailure", dateBuf);
        kjChildAdd(regTree, lastFailureP);
      }
      else
      {
        // Change to ISO8601 string
        lastFailureP->type    = KjString;
        lastFailureP->value.s = dateBuf;
      }
    }
    else if (lastFailureP != NULL)  // Change to ISO8601 string
      dbModelToApiTimestamp(lastFailureP);
  }


  //
  // Find aliases for entity type, propertyNames, and relationshipNames
  //
  KjNode* informationP = kjLookup(regTree, "information");
  if (informationP)
  {
    for (KjNode* infoItemP = informationP->value.firstChildP; infoItemP != NULL; infoItemP = infoItemP->next)
    {
      KjNode* entitiesArray          = kjLookup(infoItemP, "entities");
      KjNode* propertyNamesArray     = kjLookup(infoItemP, "propertyNames");
      KjNode* relationshipNamesArray = kjLookup(infoItemP, "relationshipNames");

      if (entitiesArray != NULL)
      {
        for (KjNode* entityItemP = entitiesArray->value.firstChildP; entityItemP != NULL; entityItemP = entityItemP->next)
        {
          KjNode* eType = kjLookup(entityItemP, "type");
          if (eType != NULL)
            eType->value.s = orionldContextItemAliasLookup(orionldState.contextP, eType->value.s, NULL, NULL);
        }
      }

      if (propertyNamesArray != NULL)
      {
        for (KjNode* propertyNameP = propertyNamesArray->value.firstChildP; propertyNameP != NULL; propertyNameP = propertyNameP->next)
        {
          propertyNameP->value.s = orionldContextItemAliasLookup(orionldState.contextP, propertyNameP->value.s, NULL, NULL);
        }
      }

      if (relationshipNamesArray != NULL)
      {
        for (KjNode* relationshipNameP = relationshipNamesArray->value.firstChildP; relationshipNameP != NULL; relationshipNameP = relationshipNameP->next)
        {
          relationshipNameP->value.s = orionldContextItemAliasLookup(orionldState.contextP, relationshipNameP->value.s, NULL, NULL);
        }
      }
    }
  }

  //
  // Remove the "properties" from "regTree" AND
  // link in all children from "properties" to the end of "regTree"
  //
  KjNode* propertiesP = kjLookup(regTree, "properties");
  if (propertiesP != NULL)
  {
    // Lookup aliases for the properties
    for (KjNode* propertyP = propertiesP->value.firstChildP; propertyP != NULL; propertyP = propertyP->next)
    {
      propertyP->name = orionldContextItemAliasLookup(orionldState.contextP, propertyP->name, NULL, NULL);
    }

    //
    // Remove "properties" from regTree and link the contexts of "properties" to "regTree"
    //
    // FIXME: Seems like "properties can be empty ...
    //        If it is empty, it shouldn't be para of the tree
    //
    kjChildRemove(regTree, propertiesP);

    if (propertiesP->value.firstChildP != NULL)
    {
      regTree->lastChild->next = propertiesP->value.firstChildP;
      regTree->lastChild       = propertiesP->lastChild;
    }
  }

  //
  // expires (expiresAt)
  //
  KjNode* expiresP = kjLookup(regTree, "expires");  // Just to change the name to "expiresAt"

  if (expiresP != NULL)
    expiresP->name = (char*) "expiresAt";


  //
  // managementInterval + observationInterval
  //
  KjNode* managementIntervalP  = kjLookup(regTree, "managementInterval");
  KjNode* observationIntervalP = kjLookup(regTree, "observationInterval");

  if (managementIntervalP != NULL)
    apiModelFromDbInterval(managementIntervalP);
  if (observationIntervalP != NULL)
    apiModelFromDbInterval(observationIntervalP);
}



// -----------------------------------------------------------------------------
//
// regTypeAndOrigin -
//
void regTypeAndOrigin(KjNode* regP, bool fromCache)
{
  // Add "type": "ContextSourceRegistration" in the start of the registration
  KjNode* typeP = kjString(orionldState.kjsonP, "type", "ContextSourceRegistration");
  typeP->next = regP->value.firstChildP;
  regP->value.firstChildP = typeP;

  // Add "origin": "database" at the end
  KjNode* originP = kjString(orionldState.kjsonP, "origin", (fromCache == true)? "cache" : "database");
  kjChildAdd(regP, originP);
}



// ----------------------------------------------------------------------------
//
// orionldGetRegistration -
//
bool orionldGetRegistration(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))
    return legacyGetRegistration();

  // Is orionldState.wildcard[0] a valid id for a registration?
  PCHECK_URI(orionldState.wildcard[0], true, 0, NULL, "Invalid Registration ID", 400);

  if (orionldState.uriParamOptions.fromDb == false)
  {
    RegCacheItem* cachedRegP = regCacheItemLookup(orionldState.tenantP, orionldState.wildcard[0]);

    if (cachedRegP != NULL)
    {
      orionldState.httpStatusCode  = 200;
      orionldState.responseTree    = kjClone(orionldState.kjsonP, cachedRegP->regTree);  // Work on a cloned copy from the reg-cache

      apiModelFromCachedRegistration(orionldState.responseTree, cachedRegP, orionldState.uriParamOptions.sysAttrs);
      regTypeAndOrigin(orionldState.responseTree, true);
      return true;
    }
  }
  else
  {
    KjNode* dbRegP = mongocRegistrationGet(orionldState.wildcard[0]);

    if (dbRegP != NULL)
    {
      dbModelToApiRegistration(dbRegP, orionldState.uriParamOptions.sysAttrs);

      orionldState.httpStatusCode  = 200;
      orionldState.responseTree    = dbRegP;

      regTypeAndOrigin(orionldState.responseTree, false);
      return true;
    }
  }

  orionldError(OrionldResourceNotFound, "Registration not found", orionldState.wildcard[0], 404);
  return false;
}
