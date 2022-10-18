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

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/dbModel/dbModelToApiRegistration.h"            // Own interface



// -----------------------------------------------------------------------------
//
// dbModelToApiRegistration - modify the DB Model tree into an API Registration
//
bool dbModelToApiRegistration(KjNode* dbRegP, bool sysAttrs)
{
  //
  // _id (id)
  //
  KjNode* _idP = kjLookup(dbRegP, "_id");

  if (_idP != NULL)
    _idP->name = (char*) "id";


  //
  // type
  //
  KjNode* typeP = kjString(orionldState.kjsonP, "type", "ContextSourceRegistration");
  kjChildAdd(dbRegP, typeP);


  //
  // expires (expiresAt)
  //
  KjNode* expiresP = kjLookup(dbRegP, "expires");

  if (expiresP != NULL)
    expiresP->name = (char*) "expiresAt";


  //
  // name (registrationName)
  //
  KjNode* nameP = kjLookup(dbRegP, "name");

  if (nameP != NULL)
    nameP->name = (char*) "registrationName";


  //
  // origin
  //
  KjNode* originP = kjString(orionldState.kjsonP, "origin", "database");
  kjChildAdd(dbRegP, originP);


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
  else
  {
    // System Attributes WANTED - turn them into ISO8601
    if (createdAtP != NULL)
    {
      char* dateBuf = kaAlloc(&orionldState.kalloc, 64);
      numberToDate(createdAtP->value.f, dateBuf, 64);
      createdAtP->value.s = dateBuf;
      createdAtP->type    = KjString;
    }

    if (modifiedAtP != NULL)
    {
      char* dateBuf = kaAlloc(&orionldState.kalloc, 64);
      numberToDate(modifiedAtP->value.f, dateBuf, 64);
      modifiedAtP->value.s = dateBuf;
      modifiedAtP->type    = KjString;
    }
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

          if (typeP != NULL)
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
            if (strcmp(typeP->value.s, "Property") == 0)
            {
              char*   shortName     = orionldContextItemAliasLookup(orionldState.contextP, nameP->value.s, NULL, NULL);
              KjNode* propertyNameP = kjString(orionldState.kjsonP, NULL, shortName);

              kjChildAdd(propertyNamesV, propertyNameP);
            }
            if (strcmp(typeP->value.s, "Relationship") == 0)
            {
              char*   shortName         = orionldContextItemAliasLookup(orionldState.contextP, nameP->value.s, NULL, NULL);
              KjNode* relationshipNameP = kjString(orionldState.kjsonP, NULL, shortName);

              kjChildAdd(relationshipNamesV, relationshipNameP);
            }
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


  //
  // properties
  //
  KjNode* propertiesP = kjLookup(dbRegP, "properties");
  if (propertiesP != NULL)
  {
    // Lookup aliases for the properties
    for (KjNode* propertyP = propertiesP->value.firstChildP; propertyP != NULL; propertyP = propertyP->next)
    {
      propertyP->name = orionldContextItemAliasLookup(orionldState.contextP, propertyP->name, NULL, NULL);
    }

    // Remove "properties" from dbRegP and link the contexts of "properties" to "dbRegP"
    kjChildRemove(dbRegP, propertiesP);

    dbRegP->lastChild->next = propertiesP->value.firstChildP;
    dbRegP->lastChild       = propertiesP->lastChild;
  }

  return true;
}
