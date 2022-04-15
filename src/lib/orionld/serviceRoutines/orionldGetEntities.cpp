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
#include <string>
#include <vector>

extern "C"
{
#include "kbase/kMacros.h"                                     // K_FT
#include "kbase/kStringSplit.h"                                // kStringSplit
#include "kbase/kTime.h"                                       // kTimeGet
#include "kalloc/kaStrdup.h"                                   // kaStrdup
#include "kjson/kjBuilder.h"                                   // kjArray, kjChildAdd, ...
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjRender.h"                                    // kjFastRender
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "ngsi10/QueryContextRequest.h"                        // QueryContextRequest
#include "ngsi10/QueryContextResponse.h"                       // QueryContextResponse
#include "mongoBackend/mongoQueryContext.h"                    // mongoQueryContext

#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/qLex.h"                               // qLex
#include "orionld/common/qParse.h"                             // qParse
#include "orionld/common/qTreeToBsonObj.h"                     // qTreeToBsonObj
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/performance.h"                        // PERFORMANCE
#include "orionld/common/dotForEq.h"                           // dotForEq
#include "orionld/types/OrionldHeader.h"                       // orionldHeaderAdd
#include "orionld/kjTree/kjTreeFromQueryContextResponse.h"     // kjTreeFromQueryContextResponse
#include "orionld/kjTree/kjEntityNormalizedToConcise.h"        // kjEntityNormalizedToConcise
#include "orionld/dbModel/dbModelToApiEntity.h"                // dbModelToApiEntity2
#include "orionld/serviceRoutines/orionldGetEntity.h"          // orionldGetEntity - if URI param 'id' is given
#include "orionld/serviceRoutines/orionldGetEntities.h"        // Own Interface



// ----------------------------------------------------------------------------
//
// geoPropertyInAttrs -
//
static bool geoPropertyInAttrs(char** attrsV, int attrsCount, const char* geoPropertyName)
{
  for (int ix = 0; ix < attrsCount; ix++)
  {
    if (strcmp(attrsV[ix], geoPropertyName) == 0)
      return true;
  }

  return false;
}



// ----------------------------------------------------------------------------
//
// apiEntityLanguageProps -
//
void apiEntityLanguageProps(KjNode* apiEntityP, const char* lang)
{
  // Loop over attributes, if "languageMap" is present . pick the language and convert to Property
  for (KjNode* attrP = apiEntityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (attrP->type != KjObject)  // type, id, ...  but also an attribute with datasetId ...
      continue;
    KjNode* languageMapP = kjLookup(attrP, "languageMap");

    if (languageMapP == NULL)
      continue;

    // Find the language item inside the languageMap
    KjNode* langItemP   = kjLookup(languageMapP, lang);
    char*   stringValue = NULL;
    char*   langChosen  = (char*) lang;

    if (langItemP == NULL)
    {
      langItemP  = kjLookup(languageMapP, "en");    // English is the default if 'lang' is not present
      if (langItemP != NULL) langChosen = (char*) "en";
    }

    if (langItemP == NULL)
    {
      langItemP = languageMapP->value.firstChildP;  // If English also not present, just pick the first one
      if (langItemP != NULL) langChosen = langItemP->name;
    }

    if (langItemP == NULL)
    {
      stringValue = (char*) "empty languageMap";  // If there is no item at all inside the language map, use "empty languageMap"
      langChosen  = (char*) "none";
    }
    else
      stringValue = langItemP->value.s;


    // Lookup the type node and change it to "Property"
    KjNode* typeP = kjLookup(attrP, "type");
    if (typeP != NULL)
      typeP->value.s = (char*) "Property";

    // Change name of "languageMap" to "value"
    languageMapP->name = (char*) "value";

    // Change JSON type to "String" - it was an object
    languageMapP->type = KjString;

    // Give it its new value
    languageMapP->value.s = stringValue;
    LM_TMP(("KZ: stringValue: %s", stringValue));
    // NULL out the lastChild
    languageMapP->lastChild = NULL;

    // Now add the language chosen, if any
    if (langItemP != NULL)
    {
      KjNode* langItemP = kjString(orionldState.kjsonP, "lang", langChosen);
      kjChildAdd(attrP, langItemP);
    }
  }
}



// ----------------------------------------------------------------------------
//
// orionldGetEntities -
//
// URI params:
// - options=keyValues
// - limit
// - offset
// - id
// - idPattern
// - type
// - typePattern  (not possible - ignored (need an exact type name to lookup alias))
// - q
// - attrs
// - geometry
// - coordinates
// - georel
// - maxDistance
//
// If "id" is given, then all other URI params are just to hint the broker on where to look for the
// entity (except for pagination params 'offset' and 'limit', and 'attrs' that has an additional function).
//
// This is necessary in a federated system using for example only entity type in the registrations.
//
// Orion-LD doesn't support federation right now (Oct 2020) and has ALL entities in its local database and thus
// need no help to find the entity.
//
// So, all URI params to help finding the entity are ignored (idPattern, type, q, geometry, coordinates, georel, maxDistance)
// Note that the pagination params (limit, offset) make no sense when returning a single entity.
// 'attrs' is a different deal though. 'attrs' will filter the attributes to be returned.
//
bool orionldGetEntities(void)
{
  char*                 id             = orionldState.uriParams.id;
  char*                 type           = orionldState.uriParams.type;
  char*                 idPattern      = orionldState.uriParams.idPattern;
  char*                 q              = orionldState.uriParams.q;
  char*                 attrs          = orionldState.uriParams.attrs;
  char*                 geometry       = orionldState.uriParams.geometry;
  char*                 georel         = orionldState.uriParams.georel;
  char*                 coordinates    = orionldState.uriParams.coordinates;
  char*                 lang           = orionldState.uriParams.lang;
  bool                  keyValues      = orionldState.uriParamOptions.keyValues;
  bool                  concise        = orionldState.uriParamOptions.concise;
  char*                 idString       = (id   != NULL)? id      : idPattern;
  const char*           isIdPattern    = (id   != NULL)? "false" : "true";
  bool                  isTypePattern  = (type != NULL)? false   : true;
  EntityId*             entityIdP;
  QueryContextRequest   mongoRequest;
  QueryContextResponse  mongoResponse;

  //
  // If URI param 'id' is given AND only one identifier in the list, then let the service routine for
  // GET /entities/{EID} do the work
  //
  if (orionldState.in.idList.items == 1)
  {
    //
    // Only ONE entity 'id' is given, so ...
    // we'll just pretend that `GET /entities/{EID}` was called and not `GET /entities`
    //
    orionldState.wildcard[0] = orionldState.in.idList.array[0];

    //
    // An array must be returned from GET /entities.
    // GET /entities/{entityId} returns just the entity as an object, so we need to create an array here
    // and put the entity inside the array vefore returning the array.
    //
    KjNode* arrayP  = kjArray(orionldState.kjsonP, NULL);

    // GET /entities return 200 OK and payload data [] if not found
    // GET /entities/{EID} returns 404 not found ...
    // Need to fix this:
    // * return true even if orionldGetEntity returns false
    // * change the 404 to a 200
    //
    // If the entity is found, it is added to the array
    //
    if (orionldGetEntity() == true)
    {
      KjNode* entityP             = orionldState.responseTree;
      entityP->next               = NULL;
      arrayP->value.firstChildP   = entityP;
    }
    else
      orionldState.httpStatusCode = 200;  // Overwrite the 404 from orionldGetEntity (it's 200 OK to find nothing in a Query)

    orionldState.responseTree     = arrayP;
    return true;
  }

  if ((id == NULL) && (idPattern == NULL) && (type == NULL) && ((geometry == NULL) || (*geometry == 0)) && (attrs == NULL) && (q == NULL))
  {
    orionldError(OrionldBadRequestData,
                 "Too broad query",
                 "Need at least one of: entity-id, entity-type, geo-location, attribute-list, Q-filter",
                 400);

    return false;
  }

  if ((idPattern != NULL) && (id != NULL))
  {
    orionldError(OrionldBadRequestData, "Incompatible parameters", "id, idPattern", 400);
    return false;
  }


  //
  // If any of "geometry", "georel" and "coordinates" is present, they must all be present
  // If "geoproperty" is present, "geometry", "georel" and "coordinates" must also be present
  //
  if ((geometry != NULL) || (georel != NULL) || (coordinates != NULL))
  {
    if ((geometry == NULL) || (georel == NULL) || (coordinates == NULL))
    {
      orionldError(OrionldBadRequestData, "Incomplete geometry", "geometry, georel, and coordinates must all be present", 400);
      return false;
    }
  }

  //
  // If 'georel' is present, make sure it has a valid value
  //
  if (georel != NULL)
  {
    if ((strncmp(georel, "near", 4)        != 0) &&
        (strncmp(georel, "within", 6)      != 0) &&
        (strncmp(georel, "contains", 8)    != 0) &&
        (strncmp(georel, "overlaps", 8)    != 0) &&
        (strncmp(georel, "intersects", 10) != 0) &&
        (strncmp(georel, "equals", 6)      != 0) &&
        (strncmp(georel, "disjoint", 8)    != 0))
    {
      orionldError(OrionldBadRequestData, "Invalid value for georel", georel, 400);
      return false;
    }

    char* georelExtra = strstr(georel, ";");

    if (georelExtra != NULL)
    {
      ++georelExtra;  // Step over ';', but don't "destroy" the string - it is used as is later on

      if ((strncmp(georelExtra, "minDistance==", 11) != 0) && (strncmp(georelExtra, "maxDistance==", 11) != 0))
      {
        LM_W(("Bad Input (invalid value for georel parameter: %s)", georelExtra));
        orionldError(OrionldBadRequestData, "Invalid value for georel parameter", georel, 400);
        return false;
      }
    }
  }

  if (geometry != NULL)
  {
    if ((strcmp(geometry, "Point")           != 0) &&
        (strcmp(geometry, "MultiPoint")      != 0) &&
        (strcmp(geometry, "Polygon")         != 0) &&
        (strcmp(geometry, "MultiPolygon")    != 0) &&
        (strcmp(geometry, "LineString")      != 0) &&
        (strcmp(geometry, "MultiLineString") != 0))
    {
      LM_W(("Bad Input (invalid value for URI parameter 'geometry'"));
      orionldError(OrionldBadRequestData, "Invalid value for URI parameter /geometry/", geometry, 400);
      return false;
    }

    Scope*  scopeP = new Scope(SCOPE_TYPE_LOCATION, "");
    char*   errorString;

    //
    // In APIv2, the vector is a string without [], in NGSI-LD, [] are present. Must remove ...
    //
    if (coordinates[0] == '[')
    {
      ++coordinates;

      int len = strlen(coordinates);
      if (coordinates[len - 1] == ']')
        coordinates[len - 1] = 0;
    }

    if (scopeP->fill(orionldState.apiVersion, geometry, coordinates, georel, &errorString) != 0)
    {
      scopeP->release();
      delete scopeP;

      LM_E(("Geo: Scope::fill failed"));
      orionldError(OrionldInternalError, "Invalid Geometry", errorString, 400);
      return false;
    }

    LM_E(("Geo: Scope::fill OK"));
    mongoRequest.restriction.scopeVector.push_back(scopeP);
  }

  if (idString == NULL)
  {
    idString    = (char*) ".*";
    isIdPattern = (char*) "true";
  }


  //
  // If ONE or ZERO types in URI param 'type', the prepared array isn't used, just a simple char-pointer (named "type")
  //
  if      (orionldState.in.typeList.items == 0) type = (char*) ".*";
  else if (orionldState.in.typeList.items == 1) type = orionldState.in.typeList.array[0];


  //
  // ID-list and Type-list at the same time is not supported
  //
  if ((orionldState.in.idList.items > 1) && (orionldState.in.typeList.items > 1))
  {
    LM_W(("Bad Input (URI params /id/ and /type/ are both lists - Not Permitted)"));
    orionldError(OrionldBadRequestData, "URI params /id/ and /type/ are both lists", "Not Permitted", 400);
    return false;
  }
  else if (orionldState.in.idList.items > 1)  // A list of Entity IDs, a single Entity TYPE
  {
    for (int ix = 0; ix < orionldState.in.idList.items; ix++)
    {
      entityIdP = new EntityId(orionldState.in.idList.array[ix], type, "false", isTypePattern);
      mongoRequest.entityIdVector.push_back(entityIdP);
    }
  }
  else if (orionldState.in.typeList.items > 1)  // A list of Entity TYPES, a single Entity ID
  {
    for (int ix = 0; ix < orionldState.in.typeList.items; ix++)
    {
      entityIdP = new EntityId(idString, orionldState.in.typeList.array[ix], isIdPattern, false);
      mongoRequest.entityIdVector.push_back(entityIdP);
    }
  }
  else  // ONE Entity ID/PATTERN, ONE Entity TYPE (or .*)
  {
    entityIdP = new EntityId(idString, type, isIdPattern, isTypePattern);
    mongoRequest.entityIdVector.push_back(entityIdP);
  }


  if (orionldState.in.attrsList.items > 0)
  {
    for (int ix = 0; ix < orionldState.in.attrsList.items; ix++)
    {
      mongoRequest.attributeList.push_back(orionldState.in.attrsList.array[ix]);
    }
  }

  if (q != NULL)
  {
    char*  title;
    char*  detail;
    QNode* lexList;
    QNode* qTree;

    if ((lexList = qLex(q, &title, &detail)) == NULL)
    {
      LM_W(("Bad Input (qLex: %s: %s)", title, detail));
      orionldError(OrionldBadRequestData, title, detail, 400);
      mongoRequest.release();
      return false;
    }

    if ((qTree = qParse(lexList, &title, &detail)) == NULL)
    {
      LM_W(("Bad Input (qParse: %s: %s)", title, detail));
      orionldError(OrionldBadRequestData, title, detail, 400);
      mongoRequest.release();
      return false;
    }


    //
    // FIXME: this part about Q-Filter depends on the database and must be moved to
    //        the DB layer
    //
    orionldState.qMongoFilterP = new mongo::BSONObj;

    mongo::BSONObjBuilder objBuilder;
    if (qTreeToBsonObj(qTree, &objBuilder, &title, &detail) == false)
    {
      LM_W(("Bad Input (qTreeToBsonObj: %s: %s)", title, detail));
      orionldError(OrionldBadRequestData, title, detail, 400);
      mongoRequest.release();
      return false;
    }

    *orionldState.qMongoFilterP = objBuilder.obj();
  }


  //
  // Call mongoBackend
  //
  long long   count;
  long long*  countP = (orionldState.uriParams.count == true)? &count : NULL;

  //
  // Special case:
  // If count is asked for and limit == 0 - just do the count query
  //
  if ((countP != NULL) && (orionldState.uriParams.limit == 0))
    orionldState.onlyCount = true;

  PERFORMANCE(mongoBackendStart);
  std::vector<std::string>  servicePathV;
  orionldState.httpStatusCode = mongoQueryContext(&mongoRequest,
                                                  &mongoResponse,
                                                  orionldState.tenantP,
                                                  servicePathV,
                                                  countP,
                                                  orionldState.apiVersion);
  PERFORMANCE(mongoBackendEnd);

  //
  // Transform QueryContextResponse to KJ-Tree
  //
  orionldState.httpStatusCode = SccOk;  // FIXME: What about the response from mongoQueryContext???

  orionldState.responseTree = kjTreeFromQueryContextResponse(false, keyValues, concise, lang, &mongoResponse);

  //
  // Work-around for Accept: application/geo+json
  //
  // If URI-param 'attrs' is used and the Geo-Property is not part of the attrs list,
  // then the GeoProperty will be set to NULL even though it may exist.
  //
  // As it will be really hard to modify mongoBackend to include that attribute that is not asked for (in URI param 'attrs'),
  // a workaround would be to perform an extra query:
  //
  // if (Accept: application/geo+json) && (URI param attrs is in use)
  //   Get the name of the geo-property (default: location)
  //   If geo-property is NOT in 'attrs' URI param
  //     Prepare a query for all entities in orionldState.responseTree, for:
  //     * _id.id and
  //     * geo-property
  //   Now go over the tree (orionldState.responseTree) and replace
  //     "geometry": null
  //   with whatever was found in this second query to mongo
  //
  if ((orionldState.out.contentType == GEOJSON) && (orionldState.in.attrsList.items > 0) && (orionldState.responseTree->type == KjArray))
  {
    const char* geoPropertyName        = (orionldState.uriParams.geometryProperty == NULL)? "location" : orionldState.uriParams.geometryProperty;
    bool        geoPropertyNameInAttrs = geoPropertyInAttrs(orionldState.in.attrsList.array, orionldState.in.attrsList.items, geoPropertyName);

    int ix = 0;
    while (ix < orionldState.in.attrsList.items)
    {
      if (strcmp(geoPropertyName, orionldState.in.attrsList.array[ix]) == 0)
        geoPropertyNameInAttrs = true;

      ++ix;
    }

    if (geoPropertyNameInAttrs == false)
    {
      int entities = 0;

      // Count the number of entities of the response
      for (KjNode* entityP = orionldState.responseTree->value.firstChildP; entityP != NULL; entityP = entityP->next)
      {
        ++entities;
      }

      // Allocate array of entity ids
      char** entityArray = (char**) kaAlloc(&orionldState.kalloc, sizeof(KjNode*) * entities);
      int    entityIx    = 0;

      // Populate the array with all entity ids
      for (KjNode* entityP = orionldState.responseTree->value.firstChildP; entityP != NULL; entityP = entityP->next)
      {
        KjNode* idP = kjLookup(entityP, "id");

        if (idP != NULL)
          entityArray[entityIx++] = idP->value.s;
      }

      // DB Query, to extract the needed info from the DB
      char* geoPropertyNameExpanded = (char*) geoPropertyName;

      if ((strcmp(geoPropertyName, "location")         != 0) &&
          (strcmp(geoPropertyName, "observationSpace") != 0) &&
          (strcmp(geoPropertyName, "operationSpace")   != 0))
      {
        geoPropertyNameExpanded = orionldState.in.geometryPropertyExpanded;
        dotForEq(geoPropertyNameExpanded);
      }

      KjNode* dbEntityArray = dbEntitiesAttributeLookup(entityArray, entities, geoPropertyNameExpanded);
      if (dbEntityArray != NULL)
      {
        orionldState.geoPropertyNodes = kjArray(orionldState.kjsonP, NULL);

        // Transform the Database model into a "valid" NGSI-LD tree
        for (KjNode* dbEntityP = dbEntityArray->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
        {
          OrionldProblemDetails pd;
          KjNode*               entityP = dbModelToApiEntity2(dbEntityP, orionldState.uriParamOptions.sysAttrs, orionldState.out.format, orionldState.uriParams.lang, &pd);

          if (entityP != NULL)
            kjChildAdd(orionldState.geoPropertyNodes, entityP);
        }
      }
    }
  }

  // Can "Accept: GEOJSON" and ?lang=x be combined?

  if ((keyValues == false) && (lang != NULL))  // If key-values, the lang thing has already been taken care of by kjTreeFromQueryContextResponse
  {
    // A language has been selected: lang, if we have an LanguageProperty in the response, the response tree needs to be modified
    for (KjNode* apiEntityP = orionldState.responseTree->value.firstChildP; apiEntityP != NULL; apiEntityP = apiEntityP->next)
    {
      apiEntityLanguageProps(apiEntityP, lang);
    }
  }

  if (orionldState.responseTree->value.firstChildP == NULL)
    orionldState.noLinkHeader = true;

  // Add "count" if asked for
  if (countP != NULL)
    orionldHeaderAdd(&orionldState.out.headers, HttpResultsCount, NULL, *countP);

  if (orionldState.uriParamOptions.concise == true)
    kjEntityNormalizedToConcise(orionldState.responseTree, NULL);  // lang already taken care of by apiEntityLanguageProps

  mongoRequest.release();

  return true;
}
