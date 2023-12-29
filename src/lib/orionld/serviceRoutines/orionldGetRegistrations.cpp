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
#include "kjson/kjBuilder.h"                                     // kjObject, kjArray
#include "kjson/kjClone.h"                                       // kjClone
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/OrionldHeader.h"                         // orionldHeaderAdd, HttpResultsCount
#include "orionld/types/RegCache.h"                              // RegCache
#include "orionld/types/RegCacheItem.h"                          // RegCache, RegCacheItem
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/legacyDriver/legacyGetRegistrations.h"         // legacyGetRegistrations
#include "orionld/mongoc/mongocRegistrationsGet.h"               // mongocRegistrationsGet
#include "orionld/dbModel/dbModelToApiRegistration.h"            // dbModelToApiRegistration
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/serviceRoutines/orionldGetRegistrations.h"     // Own Interface



// ----------------------------------------------------------------------------
//
// apiModelFromCachedRegistration - FIXME: Move to dbModel library, or elsewhere?
//
extern void apiModelFromCachedRegistration(KjNode* regTree, RegCacheItem* cachedRegP, bool sysAttrs);



// -----------------------------------------------------------------------------
//
// entityMatch -
//
// "information": [
//   {
//     "entities": [
//       {
//         "type": "",
//         "id": "",
//         "idPattern": ""
//       }
//     ],
//     "propertyNames": [ "P1", "P2", ... ],
//     "relationshipNames": [ "R1", "R2", ... ],
//   }
//
bool entityMatch(RegCacheItem* cRegP, StringArray* idListP, const char* idPattern, StringArray* typeListP, StringArray* attrListP)
{
  KjNode* informationP = kjLookup(cRegP->regTree, "information");
  int     idPatternLen = (idPattern != NULL)? strlen(idPattern) : -1;

  if (informationP == NULL)
    return false;  // Erroneous Registration - should never happen

  for (KjNode* infoItemP = informationP->value.firstChildP; infoItemP != NULL; infoItemP = infoItemP->next)
  {
    KjNode* entitiesP          = kjLookup(infoItemP, "entities");
    KjNode* propertyNamesP     = kjLookup(infoItemP, "propertyNames");
    KjNode* relationshipNamesP = kjLookup(infoItemP, "relationshipNames");
    bool    typeHit      = false;
    bool    idHit        = false;
    bool    idPatternHit = false;
    bool    attrsHit     = false;

    if (idListP->items   == 0)     idHit        = true;
    if (idPattern        == NULL)  idPatternHit = true;
    if (typeListP->items == 0)     typeHit      = true;
    if (attrListP->items == 0)     attrsHit     = true;

    //
    // Match on attributes
    //
    if ((propertyNamesP == NULL) && (relationshipNamesP == NULL))
      attrsHit = true;
    else
    {
      for (int ix = 0; ix < attrListP->items; ix++)
      {
        // If any of the attr in attrListP is found in either propertyNamesP or relationshipNamesP, then it's a match
        if ((propertyNamesP != NULL) && (kjStringValueLookupInArray(propertyNamesP, attrListP->array[ix]) != NULL))
        {
          attrsHit = true;
          break;
        }
        else if ((relationshipNamesP != NULL) && (kjStringValueLookupInArray(relationshipNamesP, attrListP->array[ix]) != NULL))
        {
          attrsHit = true;
          break;
        }
      }
    }


    if (entitiesP == NULL)
    {
      idHit        = true;
      typeHit      = true;
      idPatternHit = true;
    }
    else
    {
      //
      // Match on entityId
      //
      for (int ix = 0; ix < idListP->items; ix++)
      {
        // Does any of the Entity ID's match?
        for (KjNode* entityInfoP = entitiesP->value.firstChildP; entityInfoP != NULL; entityInfoP = entityInfoP->next)
        {
          KjNode* idP = kjLookup(entityInfoP, "id");

          if ((idP != NULL) && (strcmp(idP->value.s, idListP->array[ix]) == 0))
          {
            idHit = true;
            break;
          }
        }
      }

      //
      // Match on entity type
      //
      for (int ix = 0; ix < typeListP->items; ix++)
      {
        // Does any of the Entity types match?
        for (KjNode* entityInfoP = entitiesP->value.firstChildP; entityInfoP != NULL; entityInfoP = entityInfoP->next)
        {
          KjNode* typeP = kjLookup(entityInfoP, "type");

          if ((typeP != NULL) && (strcmp(typeP->value.s, typeListP->array[ix]) == 0))
          {
            typeHit = true;
            break;
          }
        }
      }

      if (idPattern != NULL)
      {
        // Does any of the Entity ID's match?
        for (KjNode* entityInfoP = entitiesP->value.firstChildP; entityInfoP != NULL; entityInfoP = entityInfoP->next)
        {
          KjNode* idP = kjLookup(entityInfoP, "id");

          if ((idP != NULL) && (strncmp(idP->value.s, idPattern, idPatternLen) == 0))
          {
            idPatternHit = true;
            break;
          }
        }
      }
    }

    if (typeHit && idHit && idPatternHit && attrsHit)
      return true;
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// regTypeAndOrigin - from orionldGetRegistration.cpp (FIXME: move to its own modulee)
//
extern void regTypeAndOrigin(KjNode* regP, bool fromCache);



// ----------------------------------------------------------------------------
//
// orionldGetRegistrations -
//
bool orionldGetRegistrations(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))
    return legacyGetRegistrations();

  if (orionldState.uriParamOptions.fromDb == true)
  {
    //
    // GET Registrations with mongoc
    //
    // return orionldGetRegistrationsFromDb();
  }

  //
  // Not Legacy, not "From DB" - Getting the registrations from the registration cache
  //
  RegCache* rcP = orionldState.tenantP->regCache;

  //
  // Empty loop over the registrations, just to count how many there are
  //
  // FIXME: THIS IS NOT HOW IT SHOULD WORK !!!
  //        This query has URL-parameters to narrow down the result.
  //        The count should NOT return the total number of registrations BUT
  //        the total number of MATCHING registrations
  //
  // The counting mechanism needs to be part of the second loop (and this first loop needs to go away)
  // - If count is set, instead of breaking when the "limit has been reached", the loop needs go on, but not
  // add any more registrations to the outgoing data, just counting the hits.
  //
  if (orionldState.uriParams.count == true)
  {
    int count  = 0;

    if (rcP != NULL)
    {
      for (RegCacheItem* cRegP = rcP->regList; cRegP != NULL; cRegP = cRegP->next)
      {
        ++count;
      }
    }

    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, count);
  }

  int        offset      = orionldState.uriParams.offset;
  int        limit       = orionldState.uriParams.limit;
  KjNode*    regArray    = kjArray(orionldState.kjsonP, NULL);
  int        regs        = 0;
  int        ix          = 0;


  //
  // idPattern ...
  //
  // FIXME: Implement REGEX.
  //        For now, I only support REGEX ending with .*
  //        Meaning, I cut the last two characters (assuming they're ".*") and performa a strncmp
  //
  int    idPatternLen = -1;
  char*  idPattern    = orionldState.uriParams.idPattern;

  if (idPattern != NULL)
  {
    LM_W(("The idPattern URL parameter isn't implemented for GET Registrations (pattern ending in .* is assumed)"));
    idPatternLen = strlen(idPattern) - 2;
    idPattern[idPatternLen] = 0;  // CUT OFF the ".*"
  }

  if ((limit != 0) && (rcP != NULL))
  {
    for (RegCacheItem* cRegP = rcP->regList; cRegP != NULL; cRegP = cRegP->next)
    {
      // Filter: attrs || id || idPattern || type
      if ((orionldState.uriParams.attrs != NULL) || (orionldState.uriParams.id != NULL) || (orionldState.uriParams.idPattern != NULL) || (orionldState.uriParams.type != NULL))
      {
        if (entityMatch(cRegP, &orionldState.in.idList, idPattern, &orionldState.in.typeList,  &orionldState.in.attrList) == false)
          continue;
      }

      if (ix < offset)
      {
        ++ix;
        continue;
      }

      KjNode* apiRegP = kjClone(orionldState.kjsonP, cRegP->regTree);  // Work on a cloned copy from the reg-cache
      apiModelFromCachedRegistration(apiRegP, cRegP, orionldState.uriParamOptions.sysAttrs);
      regTypeAndOrigin(apiRegP, true);

      kjChildAdd(regArray, apiRegP);

      ++ix;
      ++regs;
      if (regs >= limit)
        break;
    }
  }

  if (regArray->value.firstChildP == NULL)
    orionldState.noLinkHeader = true;  // Don't want the Link header if there is no payload body (empty array)

  orionldState.httpStatusCode = 200;
  orionldState.responseTree   = regArray;

  return true;
}
