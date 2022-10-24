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

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/types/OrionldHeader.h"                         // orionldHeaderAdd, HttpResultsCount
#include "orionld/legacyDriver/legacyGetRegistrations.h"         // legacyGetRegistrations
#include "orionld/mongoc/mongocRegistrationsGet.h"               // mongocRegistrationsGet
#include "orionld/dbModel/dbModelToApiRegistration.h"            // dbModelToApiRegistration
#include "orionld/regCache/RegCache.h"                           // RegCache, RegCacheItem
#include "orionld/regCache/regCacheGet.h"                        // regCacheGet
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/serviceRoutines/orionldGetRegistrations.h"     // Own Interface



// ----------------------------------------------------------------------------
//
// apiModelFromCachedRegistration - FIXME: Move to dbModel library?
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

  if (informationP == NULL)
    return false;  // Erroneous Registration - should never happen

  LM(("RU: orionldState.uriParams.type: '%s'", orionldState.uriParams.type));
  for (KjNode* infoItemP = informationP->value.firstChildP; infoItemP != NULL; infoItemP = infoItemP->next)
  {
    // KjNode* entitiesP          = kjLookup(infoItemP, "entities");
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

    LM(("RU: attrListP->items: %d", attrListP->items));
    for (int ix = 0; ix < attrListP->items; ix++)
    {
      LM(("RU: Looking up '%s' in propertyNamesP and relationshipNamesP", attrListP->array[ix]));
      // If any of the attr in attrListP is found in either propertyNamesP or relationshipNamesP, then it's a match
      if (kjStringValueLookupInArray(propertyNamesP, attrListP->array[ix]) != NULL)
      {
        LM(("RU: Hit in propertyNames for '%s'", attrListP->array[ix]));
        attrsHit = true;
        break;
      }
      else if (kjStringValueLookupInArray(relationshipNamesP, attrListP->array[ix]) != NULL)
      {
        LM(("RU: Hit in relationshipNames for '%s'", attrListP->array[ix]));
        attrsHit = true;
        break;
      }
      else
        LM(("RU: no hit for '%s'", attrListP->array[ix]));
    }

    if (typeHit && idHit && idPatternHit && attrsHit)
    {
      LM(("RU: ************ HIT ***********"));
      return true;
    }
    LM(("RU: ************ NO HIT ***********"));
    LM(("RU: typeHit:      %s", (typeHit      == true)? "TRUE" : "FALSE"));
    LM(("RU: idHit:        %s", (idHit        == true)? "TRUE" : "FALSE"));
    LM(("RU: idPatternHit: %s", (idPatternHit == true)? "TRUE" : "FALSE"));
    LM(("RU: attrsHit:     %s", (attrsHit     == true)? "TRUE" : "FALSE"));
  }

  return false;
}



// ----------------------------------------------------------------------------
//
// orionldGetRegistrations -
//
bool orionldGetRegistrations(void)
{
  if (experimental == false)
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
  RegCache* rcP = regCacheGet(orionldState.tenantP, false);

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

  if ((limit != 0) && (rcP != NULL))
  {
    for (RegCacheItem* cRegP = rcP->regList; cRegP != NULL; cRegP = cRegP->next)
    {
      // Filter: attrs
      if ((orionldState.uriParams.attrs != NULL) || (orionldState.uriParams.id != NULL) || (orionldState.uriParams.idPattern != NULL) || (orionldState.uriParams.type != NULL))
      {
        if (entityMatch(cRegP, &orionldState.in.idList, orionldState.uriParams.idPattern, &orionldState.in.typeList,  &orionldState.in.attrList) == false)
          continue;
      }

      if (ix < offset)
      {
        ++ix;
        continue;
      }

      KjNode* apiRegP = kjClone(orionldState.kjsonP, cRegP->regTree);  // Work on a cloned copy from the reg-cache
      apiModelFromCachedRegistration(apiRegP, cRegP, orionldState.uriParamOptions.sysAttrs);
      kjChildAdd(regArray, apiRegP);

      ++ix;
      ++regs;
      if (regs >= limit)
        break;
    }
  }
  else
    orionldState.noLinkHeader = true;  // Don't want the Link header if there is no payload body (empty array)

  orionldState.httpStatusCode = 200;
  orionldState.responseTree   = regArray;

  return true;
}
