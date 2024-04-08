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
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "orionld/types/RegistrationMode.h"                     // RegistrationMode
#include "orionld/types/RegCache.h"                             // RegCache
#include "orionld/types/RegCacheItem.h"                         // RegCacheItem
#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/common/CHECK.h"                               // STRING_CHECK, ...
#include "orionld/context/orionldAttributeExpand.h"             // orionldAttributeExpand
#include "orionld/kjTree/kjStringValueLookupInArray.h"          // kjStringValueLookupInArray
#include "orionld/kjTree/kjChildCount.h"                        // kjChildCount
#include "orionld/mongoc/mongocEntitiesQuery.h"                 // mongocEntitiesQuery
#include "orionld/mongoc/mongocEntityLookup.h"                  // mongocEntityLookup
#include "orionld/payloadCheck/fieldPaths.h"                    // RegistrationInformationEntitiesPath, ...
#include "orionld/payloadCheck/pcheckEntityInfoArray.h"         // pcheckEntityInfoArray
#include "orionld/payloadCheck/pcheckInformationItem.h"         // Own interface



// ----------------------------------------------------------------------------
//
// kjValueInArrayLookup - FIXME: Move to kjTree lib (or kjson library!)
//
// ALSO
//   I think I have a similar function somewhere ...
//
KjNode* kjValueInArrayLookup(KjNode* arrayItemP, const char* value)
{
  while (arrayItemP != NULL)
  {
    if (arrayItemP->type == KjString)
    {
      if (strcmp(arrayItemP->value.s, value) == 0)
        return arrayItemP;
    }

    arrayItemP = arrayItemP->next;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// attrsMatch -
//
// Match if propertiesP and relationshipsP are empty AND the other two
// No match if rciPropertiesArray or rciRelationshipsArray are empty
// If both have attributes, then any matching attribute provokes an error
//
static bool attrsMatch(KjNode* propertiesP, KjNode* relationshipsP, KjNode* rciPropertiesArray, KjNode* rciRelationshipsArray)
{
  bool propertiesEmpty       = (propertiesP           == NULL) || (propertiesP->value.firstChildP           == NULL);
  bool relationshipsEmpty    = (relationshipsP        == NULL) || (relationshipsP->value.firstChildP        == NULL);
  bool rciPropertiesEmpty    = (rciPropertiesArray    == NULL) || (rciPropertiesArray->value.firstChildP    == NULL);
  bool rciRelationshipsEmpty = (rciRelationshipsArray == NULL) || (rciRelationshipsArray->value.firstChildP == NULL);


  if ((propertiesEmpty == true) && (relationshipsEmpty == true))
  {
    LM_T(LmtRegMatch, ("Overlap as both reg-attrs-arrays are empty"));
    return true;
  }

  if ((rciPropertiesEmpty == true) && (rciRelationshipsEmpty == true))
  {
    LM_T(LmtRegMatch, ("Overlap as both rci-reg-attrs-arrays are empty"));
    return true;
  }

  if (propertiesP != NULL)
  {
    for (KjNode* attrNameP = propertiesP->value.firstChildP; attrNameP != NULL; attrNameP = attrNameP->next)
    {
      if (kjStringValueLookupInArray(rciPropertiesArray, attrNameP->value.s) != NULL)
      {
        LM_T(LmtRegMatch, ("overlap for attribute '%s'", attrNameP->value.s));
        return true;
      }

      if (kjStringValueLookupInArray(rciRelationshipsArray, attrNameP->value.s) != NULL)
      {
        LM_T(LmtRegMatch, ("overlap for attribute '%s'", attrNameP->value.s));
        return true;
      }
    }
  }

  if (relationshipsP != NULL)
  {
    for (KjNode* attrNameP = relationshipsP->value.firstChildP; attrNameP != NULL; attrNameP = attrNameP->next)
    {
      if (kjStringValueLookupInArray(rciPropertiesArray, attrNameP->value.s) != NULL)
      {
        LM_T(LmtRegMatch, ("overlap for attribute '%s'", attrNameP->value.s));
        return true;
      }

      if (kjStringValueLookupInArray(rciRelationshipsArray, attrNameP->value.s) != NULL)
      {
        LM_T(LmtRegMatch, ("overlap for attribute '%s'", attrNameP->value.s));
        return true;
      }
    }
  }

  LM_T(LmtRegMatch, ("No overlap for attributes"));

  return false;
}



// -----------------------------------------------------------------------------
//
// pCheckOverlappingRegistrations -
//
static bool pCheckOverlappingRegistrations
(
  const char*       currentRegId,
  RegistrationMode  regMode,
  KjNode*           entitiesP,
  KjNode*           propertiesP,
  KjNode*           relationshipsP
)
{
  if (entitiesP == NULL)
  {
    LM_T(LmtToDo, ("ToDo: check conflict for reg with only attributes"));
    return false;
  }

  for (KjNode* entityInfoP = entitiesP->value.firstChildP; entityInfoP != NULL; entityInfoP = entityInfoP->next)
  {
    // Go over entire reg-cache, to find matching registrations
    KjNode* entityTypeP      = kjLookup(entityInfoP, "type");
    KjNode* entityIdP        = kjLookup(entityInfoP, "id");
    KjNode* entityIdPatternP = kjLookup(entityInfoP, "idPattern");
    char*   entityType       = (entityTypeP      != NULL)? entityTypeP->value.s      : NULL;
    char*   entityId         = (entityIdP        != NULL)? entityIdP->value.s        : NULL;
    char*   entityIdPattern  = (entityIdPatternP != NULL)? entityIdPatternP->value.s : NULL;

    for (RegCacheItem* rciP = orionldState.tenantP->regCache->regList; rciP != NULL; rciP = rciP->next)
    {
      // In case it's an update, don't compare the registration with itself
      if ((currentRegId != NULL) && (strcmp(currentRegId, rciP->regId) == 0))
        continue;

      LM_T(LmtRegMatch, ("Trying registration '%s'", rciP->regId));

      //
      // Conflict must be checked if any of the two regs are Exclusive, BUT not if the other is Auxiliary
      //
      KjNode* rciRegModeNodeP = kjLookup(rciP->regTree, "mode");
      if ((rciRegModeNodeP != NULL) && (rciRegModeNodeP->type != KjString))
      {
        // orionldError();  this 'mode is a String check' will come later
        return false;
      }

      RegistrationMode  rciRegMode = (rciRegModeNodeP == NULL)? RegModeInclusive : registrationMode(rciRegModeNodeP->value.s);

      if ((regMode == RegModeAuxiliary) || (rciRegMode == RegModeAuxiliary))
        continue;
      if ((regMode != RegModeExclusive) && (rciRegMode != RegModeExclusive))
        continue;

      //
      // From here on none of the two registrations are AUXILIARY
      //                    And at least one of them is EXCLUSIVE
      //
      // Meaning, we'll have a collision if any of them is for ALL ATTRIBUTES
      // And if both specify the Properties/Relationships, there can be no match
      //

      KjNode* rciInformationArray = kjLookup(rciP->regTree, "information");

      for (KjNode* rciInformationItemP = rciInformationArray->value.firstChildP; rciInformationItemP != NULL; rciInformationItemP = rciInformationItemP->next)
      {
        KjNode* rciEntitiesArray      = kjLookup(rciInformationItemP, "entities");
        KjNode* rciPropertiesArray    = kjLookup(rciInformationItemP, "propertyNames");
        KjNode* rciRelationshipsArray = kjLookup(rciInformationItemP, "relationshipNames");

        if (rciEntitiesArray != NULL)
        {
          int rciIx = -1;
          for (KjNode* rciEntitiesP = rciEntitiesArray->value.firstChildP; rciEntitiesP != NULL; rciEntitiesP = rciEntitiesP->next)
          {
            ++rciIx;
            KjNode* rciEntityTypeP      = kjLookup(rciEntitiesP, "type");
            KjNode* rciEntityIdP        = kjLookup(rciEntitiesP, "id");
            KjNode* rciEntityIdPatternP = kjLookup(rciEntitiesP, "idPattern");

            if ((entityType != NULL) && (rciEntityTypeP != NULL))
            {
              if (strcmp(entityType, rciEntityTypeP->value.s) != 0)
                continue;  // different entity types - can't collide
            }

            if ((entityIdP != NULL) && (rciEntityIdP != NULL))
            {
              if (strcmp(entityId, rciEntityIdP->value.s) != 0)
                continue;  // different entity ids - can't collide
            }

            if ((entityIdPattern != NULL) && (rciEntityIdP != NULL))
            {
              LM_W(("ToDo: Does the pattern '%s' include the entity id '%s'?  If not - continue!", entityIdPattern, rciEntityIdP->value.s));
              continue;
            }

            if ((entityId != NULL) && (rciEntityIdPatternP != NULL))
            {
              LM_W(("ToDo: Does the pattern '%s' include the entity id '%s'?  If not - continue!", rciEntityIdPatternP->value.s, entityId));
              continue;
            }

            if ((entityIdPattern != NULL) && (rciEntityIdPatternP != NULL))
            {
              LM_W(("ToDo: Comparison between two idPatterns ..."));
              continue;
            }

            if (attrsMatch(propertiesP, relationshipsP, rciPropertiesArray, rciRelationshipsArray) == true)
            {
              orionldError(OrionldAlreadyExists, "Conflicting Registration", rciP->regId, 409);
              return true;
            }
          }
        }
      }
    }
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// pCheckOverlappingEntities -
//
bool pCheckOverlappingEntities(KjNode* entitiesP, KjNode* propertiesP, KjNode* relationshipsP)
{
  int properties    = (propertiesP    != NULL)? kjChildCount(propertiesP)    : 0;
  int relationships = (relationshipsP != NULL)? kjChildCount(relationshipsP) : 0;
  int attributes    = properties + relationships;

  StringArray attrsV;
  attrsV.items = attributes;
  attrsV.array = (char**) NULL;

  if (attributes > 0)
  {
    int attrNo = 0;

    attrsV.array = (char**) kaAlloc(&orionldState.kalloc, attrsV.items * sizeof(char*));

    if (propertiesP != NULL)
    {
      for (KjNode* attrP = propertiesP->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        attrsV.array[attrNo++] = attrP->value.s;
      }
    }

    if (relationshipsP != NULL)
    {
      for (KjNode* attrP = relationshipsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        attrsV.array[attrNo++] = attrP->value.s;
      }
    }
  }

  if (entitiesP == NULL)
  {
    KjNode* entityArray = mongocEntitiesQuery(NULL, NULL, NULL, &attrsV, NULL, NULL, NULL, NULL, false, false);
    if ((entityArray != NULL) && (entityArray->value.firstChildP != NULL))
    {
      KjNode*      _idP = kjLookup(entityArray->value.firstChildP, "_id");
      KjNode*      idP  = (_idP != NULL)? kjLookup(_idP, "id") : NULL;
      const char*  id   = (idP != NULL)? idP->value.s : "unknown";

      orionldError(OrionldAlreadyExists, "Conflicting Entity (due to attributes)", id, 409);
      return true;
    }
  }
  else
  {
    for (KjNode* entityInfoP = entitiesP->value.firstChildP; entityInfoP != NULL; entityInfoP = entityInfoP->next)
    {
      // Get entity info, query DB for collisions
      KjNode* entityTypeP      = kjLookup(entityInfoP, "type");
      KjNode* entityIdP        = kjLookup(entityInfoP, "id");
      KjNode* entityIdPatternP = kjLookup(entityInfoP, "idPattern");
      char*   entityType       = (entityTypeP      != NULL)? entityTypeP->value.s      : NULL;
      char*   entityId         = (entityIdP        != NULL)? entityIdP->value.s        : NULL;
      char*   entityIdPattern  = (entityIdPatternP != NULL)? entityIdPatternP->value.s : NULL;

      if (entityIdP != NULL)
      {
        char* detail = NULL;
        if (mongocEntityLookup(entityId, entityType, &attrsV, NULL, &detail) != NULL)
        {
          orionldError(OrionldAlreadyExists, "Conflicting Entity (due to entity id and type)", entityId, 409);
          return true;
        }
        if (detail != NULL)
          LM_E(("mongocEntityLookup: %s", detail));
      }
      else
      {
        StringArray entityTypeList;
        char*       entityTypeArray[1];

        entityTypeList.items = 1;
        entityTypeList.array = entityTypeArray;
        entityTypeArray[0]   = entityType;

        KjNode* entityArray = mongocEntitiesQuery(&entityTypeList, NULL, entityIdPattern, &attrsV, NULL, NULL, NULL, NULL, false, false);
        if ((entityArray != NULL) && (entityArray->value.firstChildP != NULL))
        {
          KjNode*      _idP = kjLookup(entityArray->value.firstChildP, "_id");
          KjNode*      idP  = (_idP != NULL)? kjLookup(_idP, "id") : NULL;
          const char*  id   = (idP != NULL)? idP->value.s : "unknown";

          orionldError(OrionldAlreadyExists, "Conflicting Entity (due to entity type and attributes)", id, 409);
          return true;
        }
      }
    }
  }

  return false;
}



// ----------------------------------------------------------------------------
//
// pcheckInformationItem -
//
// The "information" field can have only three members:
//   * entities       (Mandatory JSON Array)
//   * properties     (Optional JSON Array with strings)
//   * relationships  (Optional JSON Array with strings)
//
// NOTE
//   This function also expands ATTRIBUTE NAMES and ENTITY TYPES
//
bool pcheckInformationItem(const char* currentRegId, RegistrationMode regMode, KjNode* informationP)
{
  KjNode* entitiesP      = NULL;
  KjNode* propertiesP    = NULL;
  KjNode* relationshipsP = NULL;

  for (KjNode* infoItemP = informationP->value.firstChildP; infoItemP != NULL; infoItemP = infoItemP->next)
  {
    if (strcmp(infoItemP->name, "entities") == 0)
    {
      DUPLICATE_CHECK(entitiesP, RegistrationInformationEntitiesPath, infoItemP);
      ARRAY_CHECK(entitiesP, RegistrationInformationEntitiesPath);
      EMPTY_ARRAY_CHECK(entitiesP, RegistrationInformationEntitiesPath);

      if (pcheckEntityInfoArray(entitiesP, true, regMode == RegModeExclusive, RegistrationInformationEntitiesPathV) == false)
        return false;
    }
    else if ((strcmp(infoItemP->name, "propertyNames") == 0) || (strcmp(infoItemP->name, "properties") == 0))
    {
      DUPLICATE_CHECK(propertiesP, infoItemP->name, infoItemP);
      ARRAY_CHECK(infoItemP, RegistrationInformationPropertyNamesPath);
      EMPTY_ARRAY_CHECK(infoItemP, RegistrationInformationPropertyNamesPath);

      for (KjNode* propP = infoItemP->value.firstChildP; propP != NULL; propP = propP->next)
      {
        STRING_CHECK(propP, RegistrationInformationPropertyNameItemPath);
        EMPTY_STRING_CHECK(propP, RegistrationInformationPropertyNameItemPath);
        propP->value.s = orionldAttributeExpand(orionldState.contextP, propP->value.s, true, NULL);
      }

      infoItemP->name = (char*) "propertyNames";
    }
    else if ((strcmp(infoItemP->name, "relationshipNames") == 0) || (strcmp(infoItemP->name, "relationships") == 0))
    {
      DUPLICATE_CHECK(relationshipsP, infoItemP->name, infoItemP);
      ARRAY_CHECK(infoItemP, RegistrationInformationRelationshipNamesPath);
      EMPTY_ARRAY_CHECK(infoItemP, RegistrationInformationRelationshipNamesPath);

      for (KjNode* relP = infoItemP->value.firstChildP; relP != NULL; relP = relP->next)
      {
        STRING_CHECK(relP, RegistrationInformationRelationshipNamesItemPath);
        EMPTY_STRING_CHECK(relP, RegistrationInformationRelationshipNamesItemPath);
        relP->value.s = orionldAttributeExpand(orionldState.contextP, relP->value.s, true, NULL);
      }

      infoItemP->name = (char*) "relationshipNames";
    }
    else
    {
      orionldError(OrionldBadRequestData, "Invalid field for Registration::information[X]", infoItemP->name, 400);
      return false;
    }
  }

  //
  // Exclusive registrations must have either properties or relationships
  //
  if (regMode == RegModeExclusive)
  {
    if ((propertiesP == NULL) && (relationshipsP == NULL))
    {
      orionldError(OrionldBadRequestData, "Invalid exclusive registration", "information item without specifying attributes", 400);
      return false;
    }
  }

  if (propertiesP != NULL)
  {
    // Check for duplicates
    for (KjNode* propertyP = propertiesP->value.firstChildP; propertyP != NULL; propertyP = propertyP->next)
    {
      // Start looking for duplicates from the nextcoming name in the array (no need to look back - already done)
      if (kjValueInArrayLookup(propertyP->next, propertyP->value.s) != NULL)
      {
        orionldError(OrionldBadRequestData, "Duplicated Property Name", propertyP->value.s, 400);
        return false;
      }
    }
  }

  if (relationshipsP != NULL)
  {
    // Check for duplicates
    for (KjNode* relationshipP = relationshipsP->value.firstChildP; relationshipP != NULL; relationshipP = relationshipP->next)
    {
      // Start looking for duplicates from the nextcoming name in the array (no need to look back - already done)
      if (kjValueInArrayLookup(relationshipP->next, relationshipP->value.s) != NULL)
      {
        orionldError(OrionldBadRequestData, "Duplicated Relationship Name", relationshipP->value.s, 400);
        return false;
      }
    }
  }

  //
  // Check for local overlapping Registrations/Entities
  //
  if (pCheckOverlappingRegistrations(currentRegId, regMode, entitiesP, propertiesP, relationshipsP) == true)
    return false;

  if (regMode == RegModeExclusive)
  {
    if (pCheckOverlappingEntities(entitiesP, propertiesP, relationshipsP) == true)
      return false;
  }

  return true;
}
