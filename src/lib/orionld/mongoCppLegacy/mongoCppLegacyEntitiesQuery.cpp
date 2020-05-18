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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjArray, kjChildAdd
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "mongoBackend/MongoGlobal.h"                            // getMongoConnection, releaseMongoConnection, ...

#include "orionld/common/QNode.h"                                // QNode
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/qTreeToBsonObj.h"                       // qTreeToBsonObj
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/SCOMPARE.h"                             // SCOMPARE
#include "orionld/db/dbCollectionPathGet.h"                      // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                          // dbDataToKjTree
#include "orionld/mongoCppLegacy/mongoCppLegacyKjTreeToBsonObj.h"  // mongoCppLegacyKjTreeToBsonObj
#include "orionld/mongoCppLegacy/mongoCppLegacyEntitiesQuery.h"  // Own interface



// -----------------------------------------------------------------------------
//
// entityInfoArrayFilter -
//
static void entityInfoArrayFilter(mongo::BSONObjBuilder* queryBuilderP, KjNode* entityInfoArrayP)
{
  mongo::BSONObjBuilder    orFilter;
  mongo::BSONArrayBuilder  eArray;
  bool                     touched = false;

  for (KjNode* entityP = entityInfoArrayP->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode* idP        = NULL;
    KjNode* idPatternP = NULL;
    KjNode* typeP      = NULL;

    for (KjNode* nodeP = entityP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      if (strcmp(nodeP->name, "id") == 0)
        idP = nodeP;
      else if (strcmp(nodeP->name, "idPattern") == 0)
        idPatternP = nodeP;
      else if (strcmp(nodeP->name, "type") == 0)
        typeP = nodeP;
    }

    mongo::BSONObjBuilder eFilter;
    mongo::BSONObjBuilder ePart;

    if (idP != NULL)
    {
      ePart.append("_id.id", idP->value.s);
      touched = true;
    }
    if (typeP != NULL)
    {
      ePart.append("_id.type", typeP->value.s);
      touched = true;
    }
    if ((idPatternP != NULL) && (strcmp(idPatternP->value.s, ".*") != 0))
    {
      // Add REGEX to ePart
      touched = true;
    }

    if (touched == true)
    {
      eFilter.appendElements(ePart.obj());
      eArray.append(eFilter.obj());
    }
  }

  if (touched == true)
  {
    orFilter.append("$or", eArray.arr());
    queryBuilderP->appendElements(orFilter.obj());
  }
}



// -----------------------------------------------------------------------------
//
// attrsFilter -
//
// mongo shell: { "attrNames": { "$in": [ "A1", "A2" ] } }
//
static void attrsFilter(mongo::BSONObjBuilder* queryBuilderP, KjNode* attrsP)
{
  mongo::BSONArrayBuilder attrArray;

  for (KjNode* attrP = attrsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    attrArray.append(attrP->value.s);
  }

  mongo::BSONObjBuilder in;
  in.append("$in", attrArray.arr());

  queryBuilderP->append("attrNames", in.obj());
}



// -----------------------------------------------------------------------------
//
// qFilter -
//
static bool qFilter(mongo::BSONObjBuilder* queryBuilderP, QNode* qP, char** titleP, char** detailP)
{
  mongo::BSONObjBuilder  qFilter;

  if (qTreeToBsonObj(qP, &qFilter, titleP, detailP) == false)
  {
    LM_W(("Bad Input (qTreeToBsonObj: %s: %s)", *titleP, *detailP));
    return false;
  }

  queryBuilderP->appendElements(qFilter.obj());
  return true;
}



// -----------------------------------------------------------------------------
//
// geoqNearFilter -
//
// {
//   location: {
//     $near: {
//       $geometry: {
//         type: "Point",
//         coordinates: [ -73.9667, 40.78 ]
//       },
//       $minDistance: 1000,
//       $maxDistance: 5000
//     }
//   }
// }
//
// {
//   location: {
//     $near: { type: "Point", coordinates: [ 102.0, 100.0 ], $maxDistance: 3000000 } } }
//
//
static bool geoqNearFilter(mongo::BSONObjBuilder* queryBuilderP, char* geometry, char* georel, KjNode* coordinatesP, char* geopropName)
{
  char* maxDistance = NULL;
  char* minDistance = NULL;

  if (strncmp(georel, "maxDistance==", 13) == 0)
    maxDistance = &georel[13];
  if (strncmp(georel, "minDistance==", 13) == 0)
    minDistance = &georel[13];

  if ((maxDistance == NULL) && (minDistance == NULL))
  {
    LM_W(("Bad Input (no distance for 'near' georel)"));
    return false;
  }

  //
  // Getting the coordinates
  //
  double  coords[3];
  int     coordIx = 0;
  KjNode* coordsP = coordinatesP->value.firstChildP;

  for (KjNode* coordP = coordsP->value.firstChildP; coordP != NULL; coordP = coordP->next)
  {
    coords[coordIx++] = (coordP->type == KjFloat)? coordP->value.f : coordP->value.i;
  }

  //
  // Creating the mongo query
  //
  mongo::BSONObjBuilder    geoBuilder;
  mongo::BSONObjBuilder    nearBuilder;
  mongo::BSONObjBuilder    geometryBuilder;
  mongo::BSONArrayBuilder  coordsBuilder;

  coordsBuilder.append(coords[0]);
  coordsBuilder.append(coords[1]);

  geometryBuilder.append("type", geometry);
  geometryBuilder.append("coordinates", coordsBuilder.arr());

  nearBuilder.append("$geometry", geometryBuilder.obj());

  if (minDistance != NULL)
    nearBuilder.append("$minDistance", atoi(minDistance));
  if (maxDistance != NULL)
    nearBuilder.append("$maxDistance", atoi(maxDistance));

  geoBuilder.append("$nearSphere", nearBuilder.obj());

  char geoPropertyPath[256] = { 'a', 't', 't', 'r', 's', '.', 0 };

  snprintf(geoPropertyPath, sizeof(geoPropertyPath), "attrs.%s.value", geopropName);

  queryBuilderP->append(geoPropertyPath, geoBuilder.obj());

  return true;
}



// -----------------------------------------------------------------------------
//
// geoqWithinFilter -
//
// {
//    <location field>: {
//       $geoWithin: {
//          $geometry: {
//             type: <"Polygon" or "MultiPolygon"> ,
//             coordinates: [ <coordinates> ]
//          }
//       }
//    }
// }
//
static bool geoqWithinFilter(mongo::BSONObjBuilder* queryBuilderP, char* geometry, KjNode* coordsP, char* geopropName)
{
  mongo::BSONObj           coordsObj;            // coordinates: [ [], [] ]
  mongo::BSONObjBuilder    geometryFields;       // { type: "XXX", coordinates: [] }
  mongo::BSONObjBuilder    geometryBuilder;      // { $geometry: { type+coordinates } }
  mongo::BSONObjBuilder    withinBuilder;        // { $geoWithin: { geometryBuilder } }

  mongoCppLegacyKjTreeToBsonObj(coordsP, &coordsObj);
  geometryFields.append("type", geometry);
  geometryFields.appendElements(coordsObj);
  geometryBuilder.append("$geometry", geometryFields.obj());
  withinBuilder.append("$geoWithin", geometryBuilder.obj());

  char geoPropertyPath[256] = { 'a', 't', 't', 'r', 's', '.', 0 };
  snprintf(geoPropertyPath, sizeof(geoPropertyPath), "attrs.%s.value", geopropName);

  // LM_TMP(("GEO: Query: { %s: %s }", geoPropertyPath, withinBuilder.obj().toString().c_str()));  // DESTRUCTIVE !!!
  queryBuilderP->append(geoPropertyPath, withinBuilder.obj());

  return true;
}



// -----------------------------------------------------------------------------
//
// geoqIntersectsFilter -
//
// {
//   <location field>: {
//      $geoIntersects: {
//         $geometry: {
//            type: "<GeoJSON object type>" ,
//            coordinates: [ <coordinates> ]
//         }
//      }
//   }
// }
//
static bool geoqIntersectsFilter(mongo::BSONObjBuilder* queryBuilderP, char* geometry, KjNode* coordsP, char* geopropName)
{
  mongo::BSONObj         coordsObj;          // coordinates: [ [], [] ]
  mongo::BSONObjBuilder  geometryFields;     // { type: "XXX", coordinates: [] }
  mongo::BSONObjBuilder  geometryBuilder;    // { $geometry: { type+coordinates } }
  mongo::BSONObjBuilder  intersectsBuilder;  // { $geoIntersects: { geometryBuilder } }

  mongoCppLegacyKjTreeToBsonObj(coordsP, &coordsObj);
  geometryFields.append("type", geometry);
  geometryFields.appendElements(coordsObj);
  geometryBuilder.append("$geometry", geometryFields.obj());
  intersectsBuilder.append("$geoIntersects", geometryBuilder.obj());

  char geoPropertyPath[256] = { 'a', 't', 't', 'r', 's', '.', 0 };
  snprintf(geoPropertyPath, sizeof(geoPropertyPath), "attrs.%s.value", geopropName);

  // LM_TMP(("GEO: Query: { %s: %s }", geoPropertyPath, intersectsBuilder.obj().toString().c_str()));  // DESTRUCTIVE !!!
  queryBuilderP->append(geoPropertyPath, intersectsBuilder.obj());

  return true;
}



// -----------------------------------------------------------------------------
//
// geoqEqualsFilter -
//
// This is not really a GEo-Query - just an EQ comparison over objects
//
static bool geoqEqualsFilter(mongo::BSONObjBuilder* queryBuilderP, KjNode* geometryP, KjNode* coordsP, char* geopropName)
{
  mongo::BSONObj  valueObj;
  KjNode*         nodeP = kjObject(orionldState.kjsonP, NULL);

  geometryP->name = (char*) "type";
  kjChildAdd(nodeP, geometryP);
  kjChildAdd(nodeP, coordsP);

  mongoCppLegacyKjTreeToBsonObj(nodeP, &valueObj);

  char geoPropertyPath[256] = { 'a', 't', 't', 'r', 's', '.', 0 };
  snprintf(geoPropertyPath, sizeof(geoPropertyPath), "attrs.%s.value", geopropName);

  // LM_TMP(("GEO: Query: { %s: %s }", geoPropertyPath, valueObj.toString().c_str()));  // DESTRUCTIVE !!!
  queryBuilderP->append(geoPropertyPath, valueObj);

  return true;
}



// -----------------------------------------------------------------------------
//
// geoqDisjointFilter -
//
// {
//   <location field>: {
//     $not: {
//        $geoIntersects: {
//           $geometry: {
//              type: "<GeoJSON object type>" ,
//              coordinates: [ <coordinates> ]
//           }
//         }
//      }
//   }
// }
//
static bool geoqDisjointFilter(mongo::BSONObjBuilder* queryBuilderP, char* geometry, KjNode* coordsP, char* geopropName)
{
  mongo::BSONObj         coordsObj;          // coordinates: [ [], [] ]
  mongo::BSONObjBuilder  geometryFields;     // { type: "XXX", coordinates: [] }
  mongo::BSONObjBuilder  geometryBuilder;    // { $geometry: { type+coordinates } }
  mongo::BSONObjBuilder  intersectsBuilder;  // { $geoIntersects: { geometryBuilder } }
  mongo::BSONObjBuilder  notBuilder;         // { $not: { intersectsBuilder }

  mongoCppLegacyKjTreeToBsonObj(coordsP, &coordsObj);
  geometryFields.append("type", geometry);
  geometryFields.appendElements(coordsObj);
  geometryBuilder.append("$geometry", geometryFields.obj());
  intersectsBuilder.append("$geoIntersects", geometryBuilder.obj());
  notBuilder.append("$not", intersectsBuilder.obj());

  char geoPropertyPath[256] = { 'a', 't', 't', 'r', 's', '.', 0 };
  snprintf(geoPropertyPath, sizeof(geoPropertyPath), "attrs.%s.value", geopropName);

  // LM_TMP(("GEO: Query: { %s: %s }", geoPropertyPath, intersectsBuilder.obj().toString().c_str()));  // DESTRUCTIVE !!!
  queryBuilderP->append(geoPropertyPath, notBuilder.obj());

  return true;
}



// -----------------------------------------------------------------------------
//
// geoqOverlapsFilter - intersects AND is of the same GEO-Type
//
// {
//   <location field>: {
//      $geoIntersects: {
//         $geometry: {
//            type: "<GeoJSON object type>" ,
//            coordinates: [ <coordinates> ]
//         }
//      }
//   }
// },
// {
//   <location field>.type: <geoType>
// }
//
static bool geoqOverlapsFilter(mongo::BSONObjBuilder* queryBuilderP, char* geometry, KjNode* coordsP, char* geopropName)
{
  mongo::BSONObj         coordsObj;          // coordinates: [ [], [] ]
  mongo::BSONObjBuilder  geometryFields;     // { type: "XXX", coordinates: [] }
  mongo::BSONObjBuilder  geometryBuilder;    // { $geometry: { type+coordinates } }
  mongo::BSONObjBuilder  intersectsBuilder;  // { $geoIntersects: { geometryBuilder } }
  mongo::BSONObjBuilder  geoTypeBuilder;     // { attrs.geoProp.value.type: geometry }

  mongoCppLegacyKjTreeToBsonObj(coordsP, &coordsObj);
  geometryFields.append("type", geometry);
  geometryFields.appendElements(coordsObj);
  geometryBuilder.append("$geometry", geometryFields.obj());
  intersectsBuilder.append("$geoIntersects", geometryBuilder.obj());

  char  geoPropertyPath[256] = { 'a', 't', 't', 'r', 's', '.', 0 };
  char* geoPropertyPathP     = geoPropertyPath;
  int   size;

  size = snprintf(geoPropertyPath, sizeof(geoPropertyPath), "attrs.%s.value", geopropName);

  // LM_TMP(("GEO: Query: { %s: %s }", geoPropertyPath, intersectsBuilder.obj().toString().c_str()));  // DESTRUCTIVE !!!

  queryBuilderP->append(geoPropertyPath, intersectsBuilder.obj());

  if (size + 5 >= (int) sizeof(geoPropertyPath))
  {
    geoPropertyPathP = kaAlloc(&orionldState.kalloc, 512);
    snprintf(geoPropertyPathP, 512, "attrs.%s.value.type", geopropName);
  }
  else
    strcat(geoPropertyPath, ".type");

  queryBuilderP->append(geoPropertyPathP, geometry);

  return true;
}



// -----------------------------------------------------------------------------
//
// geoqFilter -
//
static void geoqFilter(mongo::BSONObjBuilder* queryBuilderP, KjNode* geoqP)
{
  KjNode* geometryP      = NULL;
  KjNode* georelP        = NULL;
  KjNode* coordsP        = NULL;
  KjNode* geopropertyP   = NULL;

  for (KjNode* nodeP = geoqP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (strcmp(nodeP->name, "geometry") == 0)
      geometryP = nodeP;
    else if (strcmp(nodeP->name, "georel") == 0)
      georelP = nodeP;
    else if (strcmp(nodeP->name, "coordinates") == 0)
      coordsP = nodeP;
    else if (strcmp(nodeP->name, "geoproperty") == 0)
      geopropertyP = nodeP;
  }

  //
  // Creating an object for "coordinates": []
  // I could also remove geometryP, georel and geopropertyP from geoqP ...
  //
  KjNode* coordinatesP = kjObject(orionldState.kjsonP, NULL);
  kjChildAdd(coordinatesP, coordsP);

  char* georel      = georelP->value.s;
  char* geometry    = geometryP->value.s;
  char* geoproperty = geopropertyP->value.s;

  if (SCOMPARE5(georel, 'n', 'e', 'a', 'r', ';'))
    geoqNearFilter(queryBuilderP, geometry, &georel[5], coordinatesP, geoproperty);
  else if (SCOMPARE7(georel, 'w', 'i', 't', 'h', 'i', 'n', 0))
    geoqWithinFilter(queryBuilderP, geometry, coordinatesP, geoproperty);
  else if (SCOMPARE11(georel, 'i', 'n', 't', 'e', 'r', 's', 'e', 'c', 't', 's', 0))
    geoqIntersectsFilter(queryBuilderP, geometry, coordinatesP, geoproperty);
  else if (SCOMPARE7(georel, 'e', 'q', 'u', 'a', 'l', 's', 0))
    geoqEqualsFilter(queryBuilderP, geometryP, coordsP, geoproperty);
  else if (SCOMPARE9(georel, 'd', 'i', 's', 'j', 'o', 'i', 'n', 't', 0))
    geoqDisjointFilter(queryBuilderP, geometry, coordinatesP, geoproperty);
  else if (SCOMPARE9(georel, 'o', 'v', 'e', 'r', 'l', 'a', 'p', 's', 0))
    geoqOverlapsFilter(queryBuilderP, geometry, coordinatesP, geoproperty);
}



// -----------------------------------------------------------------------------
//
// mongoCppLegacyEntitiesQuery -
//
KjNode* mongoCppLegacyEntitiesQuery(KjNode* entityInfoArrayP, KjNode* attrsP, QNode* qP, KjNode* geoqP, int limit, int offset, int* countP)
{
  char                   collectionPath[256];
  mongo::BSONObjBuilder  queryBuilder;
  char*                  title;
  char*                  detail;

  if ((entityInfoArrayP != NULL) && (entityInfoArrayP->value.firstChildP != NULL))
    entityInfoArrayFilter(&queryBuilder, entityInfoArrayP);

  if (attrsP != NULL)
    attrsFilter(&queryBuilder, attrsP);

  if ((qP != NULL) && (qFilter(&queryBuilder, qP, &title, &detail) == false))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, title, detail);
    return NULL;
  }

  if (geoqP != NULL)
    geoqFilter(&queryBuilder, geoqP);

  KjNode* kjTree = kjArray(orionldState.kjsonP, NULL);

  // semTake()
  mongo::DBClientBase*                  connectionP = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP;
  mongo::Query                          query(queryBuilder.obj());

  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities");

  //
  // Count asked for ?
  //
  if (countP != NULL)
  {
    try
    {
      *countP = connectionP->count(collectionPath, query);
    }
    catch (const std::exception &e)
    {
      LM_E(("Database Error (asking for the number of hits: %s)", e.what()));
      kjTree = NULL;
      limit = 0;  // Just to avoid performing the query
    }
  }

  //
  // Performing the Query to the database
  //
  if (limit != 0)
  {
    // Sort according to creDate
    query.sort("creDate", 1);

    // LM_TMP(("GEO: query: %s", query.toString().c_str()));  // Not Destructive
    cursorP = connectionP->query(collectionPath, query, limit, offset);

    try
    {
      while (cursorP->more())
      {
        mongo::BSONObj  bsonObj = cursorP->nextSafe();
        char*           title;
        char*           details;
        KjNode*         entityP;

        entityP = dbDataToKjTree(&bsonObj, &title, &details);
        if (entityP == NULL)
          LM_E(("dbDataToKjTree: %s: %s", title, details));

        kjChildAdd(kjTree, entityP);
      }
    }
    catch (const std::exception &e)
    {
      LM_E(("Database Error (%s)", e.what()));
    }
  }

  releaseMongoConnection(connectionP);

  // semGive()

  return kjTree;
}
