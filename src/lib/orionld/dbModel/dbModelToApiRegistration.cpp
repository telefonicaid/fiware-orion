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
#include <string.h>                                              // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjArray, kjString, kjChildAdd, kjChildRemove, ...
}

#include "logMsg/logMsg.h"                                       // LM

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/dbModel/dbModelToApiRegistration.h"            // Own interface



// -----------------------------------------------------------------------------
//
// dbModelToApiTimestamp -
//
void dbModelToApiTimestamp(KjNode* tsNodeP)
{
  if (tsNodeP->type == KjString)
    return;

  char* dateBuf = kaAlloc(&orionldState.kalloc, 64);
  numberToDate(tsNodeP->value.f, dateBuf, 64);
  tsNodeP->value.s = dateBuf;
  tsNodeP->type    = KjString;
}



// -----------------------------------------------------------------------------
//
// dbModelToApiInterval -
//
void dbModelToApiInterval(KjNode* intervalP)
{
  KjNode* startAtP = kjLookup(intervalP, "startAt");
  KjNode* endAtP   = kjLookup(intervalP, "endAt");

  if (startAtP != NULL)
    dbModelToApiTimestamp(startAtP);
  if (endAtP != NULL)
    dbModelToApiTimestamp(endAtP);
}



// -----------------------------------------------------------------------------
//
// dbModelToApiRegistration - modify the DB Model tree into an API Registration
//
bool dbModelToApiRegistration(KjNode* dbRegP, bool sysAttrs, bool forCache)
{
  //
  // _id (id)
  //
  KjNode* _idP = kjLookup(dbRegP, "_id");

  if (_idP != NULL)
    _idP->name = (char*) "id";


  //
  // expiration (expiresAt)
  //
  KjNode* expiresAtP = kjLookup(dbRegP, "expiration");

  if (expiresAtP != NULL)
  {
    expiresAtP->name = (char*) "expiresAt";
    dbModelToApiTimestamp(expiresAtP);
  }

  //
  // name (registrationName)
  //
  KjNode* nameP = kjLookup(dbRegP, "name");

  if (nameP != NULL)
    nameP->name = (char*) "registrationName";


  //
  // System Attributes
  //
  KjNode*  createdAtP  = kjLookup(dbRegP, "createdAt");
  KjNode*  modifiedAtP = kjLookup(dbRegP, "modifiedAt");

  if (sysAttrs == false)
  {
    // System Attributes NOT wanted - REMOVE them from the cloned copy of the registration from the reg-cache
    if (createdAtP != NULL)
      kjChildRemove(dbRegP, createdAtP);
    if (modifiedAtP != NULL)
      kjChildRemove(dbRegP, modifiedAtP);
  }
  else if (forCache == false)
  {
    // System Attributes WANTED - turn them into ISO8601
    if (createdAtP != NULL)
      dbModelToApiTimestamp(createdAtP);
    if (modifiedAtP != NULL)
      dbModelToApiTimestamp(modifiedAtP);
  }


  //
  // contextRegistration  (information)
  //
  KjNode* crP = kjLookup(dbRegP, "contextRegistration");

  if (crP != NULL)
  {
    crP->name = (char*) "information";

    bool endpointDone = false;
    for (KjNode* crItemP = crP->value.firstChildP; crItemP != NULL; crItemP = crItemP->next)
    {
      KjNode* entitiesP = kjLookup(crItemP, "entities");
      KjNode* attrsP    = kjLookup(crItemP, "attrs");
      KjNode* endpointP = kjLookup(crItemP, "providingApplication");

      if (entitiesP != NULL)
      {
        for (KjNode* eInfoP = entitiesP->value.firstChildP; eInfoP != NULL; eInfoP = eInfoP->next)
        {
          KjNode* idP            = kjLookup(eInfoP, "id");
          KjNode* typeP          = kjLookup(eInfoP, "type");
          KjNode* isPatternP     = kjLookup(eInfoP, "isPattern");
          KjNode* isTypePatternP = kjLookup(eInfoP, "isTypePattern");  // Not supported by NGSI-LD ... Removed!

          if (isPatternP != NULL)
          {
            kjChildRemove(eInfoP, isPatternP);
            if (strcmp(isPatternP->value.s, "true") == 0)
            {
              if (idP != NULL)
                idP->name = (char*) "idPattern";
            }
          }

          if (isTypePatternP != NULL)
            kjChildRemove(eInfoP, isTypePatternP);

          if ((typeP != NULL) && (forCache == false))
            typeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, typeP->value.s, NULL, NULL);
        }
      }

      if (attrsP != NULL)
      {
        KjNode* propertyNamesV     = kjArray(orionldState.kjsonP, "propertyNames");
        KjNode* relationshipNamesV = kjArray(orionldState.kjsonP, "relationshipNames");

        kjChildRemove(crItemP, attrsP);

        for (KjNode* attrP = attrsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
        {
          KjNode* nameP = kjLookup(attrP, "name");
          KjNode* typeP = kjLookup(attrP, "type");

          if ((nameP != NULL) && (typeP != NULL))
          {
            char*   shortName     =  (forCache == true)? nameP->value.s : orionldContextItemAliasLookup(orionldState.contextP, nameP->value.s, NULL, NULL);
            KjNode* attrNameNodeP = kjString(orionldState.kjsonP, NULL, shortName);

            if (strcmp(typeP->value.s, "Property") == 0)
              kjChildAdd(propertyNamesV, attrNameNodeP);
            else if (strcmp(typeP->value.s, "Relationship") == 0)
              kjChildAdd(relationshipNamesV, attrNameNodeP);
          }

          if (propertyNamesV->value.firstChildP != NULL)
            kjChildAdd(crItemP, propertyNamesV);
          if (relationshipNamesV->value.firstChildP != NULL)
            kjChildAdd(crItemP, relationshipNamesV);
        }
      }

      if (endpointP != NULL)
      {
        kjChildRemove(crItemP, endpointP);

        if (endpointDone == false)
        {
          endpointP->name = (char*) "endpoint";
          kjChildAdd(dbRegP, endpointP);
          endpointDone = true;
        }
      }
    }
  }

  if (forCache == false)
  {
    //
    // managementInterval + observationInterval to ISO8601 Strings
    //
    KjNode* managementIntervalP  = kjLookup(dbRegP, "managementInterval");
    KjNode* observationIntervalP = kjLookup(dbRegP, "observationInterval");

    if (managementIntervalP != NULL)
      dbModelToApiInterval(managementIntervalP);
    if (observationIntervalP)
      dbModelToApiInterval(observationIntervalP);
  }

  //
  // properties
  //
  if (forCache == false)
  {
    KjNode* propertiesP = kjLookup(dbRegP, "properties");
    if (propertiesP != NULL)
    {
      // Lookup aliases for the properties
      {
        for (KjNode* propertyP = propertiesP->value.firstChildP; propertyP != NULL; propertyP = propertyP->next)
        {
          propertyP->name = orionldContextItemAliasLookup(orionldState.contextP, propertyP->name, NULL, NULL);
        }
      }

      // Remove "properties" from dbRegP and link the contexts of "properties" to "dbRegP"
      kjChildRemove(dbRegP, propertiesP);

      if (propertiesP->value.firstChildP != NULL)
      {
        dbRegP->lastChild->next = propertiesP->value.firstChildP;
        dbRegP->lastChild       = propertiesP->lastChild;
      }
    }
  }

  return true;
}
