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
#include <bson/bson.h>                                           // bson_t, ...
#include <mongoc/mongoc.h>                                       // MongoDB C Client Driver

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjArray
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/types/OrionldGeoInfo.h"                        // OrionldGeoInfo
#include "orionld/types/QNode.h"                                 // QNode
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/q/qTreeToBson.h"                               // qTreeToBson
#include "orionld/mongoc/mongocWriteLog.h"                       // MONGOC_RLOG - FIXME: change name to mongocLog.h
#include "orionld/mongoc/mongocConnectionGet.h"                  // mongocConnectionGet
#include "orionld/mongoc/mongocKjTreeToBson.h"                   // mongocKjTreeToBson
#include "orionld/mongoc/mongocKjTreeFromBson.h"                 // mongocKjTreeFromBson
#include "orionld/mongoc/mongocAuxAttributesFilter.h"            // mongocAuxAttributesFilter
#include "orionld/mongoc/mongocEntitiesQuery.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// entityTypeFilter -
//
static bool entityTypeFilter(bson_t* mongoFilterP, StringArray* entityTypes)
{
  if (entityTypes->items == 1)  // Just a single type?
  {
    bson_append_utf8(mongoFilterP, "_id.type", 8, entityTypes->array[0], -1);
    return true;
  }

  bson_t in;
  bson_t entityTypeArray;

  bson_init(&in);
  bson_init(&entityTypeArray);

  for (int ix = 0; ix < entityTypes->items; ix++)
  {
    char num[32];
    int  numLen;

    if (ix < 10)
    {
      num[0] = '0' + ix;
      num[1] = 0;
      numLen = 1;
    }
    else
    {
      num[0] = '0' + ix % 10;
      num[1] = '0' + ix / 10;
      num[2] = 0;
      numLen = 2;
    }

    bson_append_utf8(&entityTypeArray, num, numLen, entityTypes->array[ix], -1);
  }
  bson_append_array(&in, "$in", 3, &entityTypeArray);
  bson_append_document(mongoFilterP, "_id.type", 8, &in);

  bson_destroy(&in);                // It's safe to destroy once incorporated into mongoFilterP
  bson_destroy(&entityTypeArray);   // It's safe to destroy once incorporated into mongoFilterP

  return true;
}



// -----------------------------------------------------------------------------
//
// entityIdFilter -
//
static bool entityIdFilter(bson_t* mongoFilterP, StringArray* entityIds)
{
  if (entityIds->items == 1)  // Just a single id?
  {
    bson_append_utf8(mongoFilterP, "_id.id", 6, entityIds->array[0], -1);
    return true;
  }

  bson_t in;
  bson_t entityIdArray;

  bson_init(&in);
  bson_init(&entityIdArray);

  for (int ix = 0; ix < entityIds->items; ix++)
  {
    char num[32];
    int  numLen;

    if (ix < 10)
    {
      num[0] = '0' + ix;
      num[1] = 0;
      numLen = 1;
    }
    else
    {
      num[0] = '0' + ix % 10;
      num[1] = '0' + ix / 10;
      num[2] = 0;
      numLen = 2;
    }

    bson_append_utf8(&entityIdArray, num, numLen, entityIds->array[ix], -1);
  }
  bson_append_array(&in, "$in", 3, &entityIdArray);
  bson_append_document(mongoFilterP, "_id.id", 6, &in);

  bson_destroy(&in);                // It's safe to destroy once incorporated into mongoFilterP
  bson_destroy(&entityIdArray);     // It's safe to destroy once incorporated into mongoFilterP

  return true;
}



// -----------------------------------------------------------------------------
//
// entityIdPatternFilter -
//
static bool entityIdPatternFilter(bson_t* mongoFilterP, const char* idPattern)
{
  bson_append_regex(mongoFilterP, "_id.id", 6, idPattern, "m");
  return true;
}



// -----------------------------------------------------------------------------
//
// qFilter -
//
bool qFilter(bson_t* mongoFilterP, QNode* qNode)
{
  char* title;
  char* detail;

  if (qTreeToBson(qNode, mongoFilterP, &title, &detail) == false)
  {
    orionldError(OrionldInternalError, title, detail, 500);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// geoPropertyDbPath -
//
static bool geoPropertyDbPath(char* geoPropertyPath, int geoPropertyPathSize, const char* geoPropertyName, int* actualLen)
{
  int  len = snprintf(geoPropertyPath, geoPropertyPathSize - 7, "attrs.%s", geoPropertyName);  // 7 chars needed for ".value" and zero termination

  if (len + 7 > geoPropertyPathSize)
  {
    orionldError(OrionldInternalError, "Recompilation needed", "geoPropertyPath char array is too small", 500);
    return false;
  }

  dotForEq(&geoPropertyPath[6]);
  strncpy(&geoPropertyPath[len], ".value", geoPropertyPathSize - len - 1);

  *actualLen = len + 6;

  return true;
}



// -----------------------------------------------------------------------------
//
// geoNearFilter -
//
// Example Filter: location and { Point, near, and maxDistance }:
// {
//   "location": {
//     "$near": {
//       "$geometry": {
//         "type": "Point",
//         "coordinates": [ -73.9667, 40.78 ]
//        },
//        "$maxDistance": 5000
//      }
//   }
// }
//
//
static bool geoNearFilter(bson_t* mongoFilterP, OrionldGeoInfo*  geoInfoP)
{
  bson_t location;
  bson_t near;
  bson_t geometry;
  bson_t coordinates;

  if (geoInfoP->geometry != GeoPoint)
  {
    orionldError(OrionldBadRequestData, "Invalid Geometry for Near Query (must be a Point)", orionldGeometryToString(geoInfoP->geometry), 400);
    return false;
  }

  bson_init(&location);
  bson_init(&near);
  bson_init(&geometry);
  bson_init(&coordinates);

  mongocKjTreeToBson(geoInfoP->coordinates, &coordinates);

  bson_append_utf8(&geometry,  "type",         4, "Point", 5);
  bson_append_array(&geometry, "coordinates", 11, &coordinates);
  bson_append_document(&near,  "$geometry",    9, &geometry);

  if (geoInfoP->minDistance > 0)    bson_append_int32(&near, "$minDistance", 12, geoInfoP->minDistance);
  if (geoInfoP->maxDistance > 0)    bson_append_int32(&near, "$maxDistance", 12, geoInfoP->maxDistance);

  bson_append_document(&location, "$nearSphere", 11, &near);

  char geoPropertyPath[512];
  int  geoPropertyPathLen;
  if (geoPropertyDbPath(geoPropertyPath, sizeof(geoPropertyPath), geoInfoP->geoProperty, &geoPropertyPathLen) == false)
    LM_RE(false, ("Failed to aeemble the geoProperty path"));

  LM_T(LmtMongoc, ("geoPropertyPath: '%s'", geoPropertyPath));

  bson_append_document(mongoFilterP, geoPropertyPath, geoPropertyPathLen, &location);

  bson_destroy(&location);
  bson_destroy(&near);
  bson_destroy(&geometry);
  bson_destroy(&coordinates);

  return true;
}



// -----------------------------------------------------------------------------
//
// geoWithinFilter -
//
// Example Filter: location and { Polygon and $within }
// {
//   "location": {
//     $geoWithin: {
//       $geometry: {
//         type: <"Polygon" or "MultiPolygon"> ,
//         coordinates: [ <coordinates> ]
//       }
//     }
//   }
// }
//
static bool geoWithinFilter(bson_t* mongoFilterP, OrionldGeoInfo* geoInfoP)
{
  bson_t location;
  bson_t within;
  bson_t geometry;
  bson_t coordinates;

  bson_init(&geometry);

  //
  // "within" definitely needs a polygon - can't be "within" a point nor a LineString, can you?
  //
  if      (geoInfoP->geometry == GeoPolygon)       bson_append_utf8(&geometry,  "type", 4, "Polygon", 7);
  else if (geoInfoP->geometry == GeoMultiPolygon)  bson_append_utf8(&geometry,  "type", 4, "MultiPolygon", 12);
  else
  {
    bson_destroy(&geometry);
    orionldError(OrionldBadRequestData, "Invalid Geometry for Within Query", orionldGeometryToString(geoInfoP->geometry), 400);
    return false;
  }

  bson_init(&location);
  bson_init(&within);
  bson_init(&coordinates);

  kjTreeLog(geoInfoP->coordinates, "coordinates", LmtMongoc);
  mongocKjTreeToBson(geoInfoP->coordinates, &coordinates);

  bson_append_array(&geometry,    "coordinates", 11, &coordinates);
  bson_append_document(&within,   "$geometry",    9, &geometry);
  bson_append_document(&location, "$within",      7,  &within);

  char geoPropertyPath[512];
  int  geoPropertyPathLen;
  if (geoPropertyDbPath(geoPropertyPath, sizeof(geoPropertyPath), geoInfoP->geoProperty, &geoPropertyPathLen) == false)
    return false;

  bson_append_document(mongoFilterP, geoPropertyPath, geoPropertyPathLen, &location);

  bson_destroy(&location);
  bson_destroy(&within);
  bson_destroy(&geometry);
  bson_destroy(&coordinates);

  return true;
}



// -----------------------------------------------------------------------------
//
// geoFilterIntersects - $geoIntersects
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
static bool geoIntersectsFilter(bson_t* mongoFilterP, OrionldGeoInfo* geoInfoP)
{
  bson_t location;
  bson_t intersects;
  bson_t geometry;
  bson_t coordinates;

  bson_init(&geometry);

#if 0
  //
  // intersect ... should be valid for any geometry ...  Right?
  //
  // According to https://stackoverflow.com/questions/35656520/find-overlapping-trajectories,
  // geoIntersects requires polygons or multipolygons in the query,
  // But, I haven't found this piece of info still in the mongo documentation, and it kind of seems to be working ...
  //
  if      (geoInfoP->geometry == GeoPolygon)       bson_append_utf8(&geometry,  "type", 4, "Polygon", 7);
  else if (geoInfoP->geometry == GeoMultiPolygon)  bson_append_utf8(&geometry,  "type", 4, "MultiPolygon", 12);
  else
  {
    bson_destroy(&geometry);
    orionldError(OrionldBadRequestData, "Invalid Geometry for Intersect Query", orionldGeometryToString(geoInfoP->geometry), 400);
    return false;
  }
#endif

  bson_init(&location);
  bson_init(&intersects);
  bson_init(&coordinates);

  mongocKjTreeToBson(geoInfoP->coordinates, &coordinates);

  bson_append_utf8(&geometry,       "type",            4, orionldGeometryToString(geoInfoP->geometry), -1);
  bson_append_array(&geometry,      "coordinates",    11, &coordinates);
  bson_append_document(&intersects, "$geometry",       9, &geometry);
  bson_append_document(&location,   "$geoIntersects", 14, &intersects);

  char geoPropertyPath[512];
  int  geoPropertyPathLen;
  if (geoPropertyDbPath(geoPropertyPath, sizeof(geoPropertyPath), geoInfoP->geoProperty, &geoPropertyPathLen) == false)
    return false;

  bson_append_document(mongoFilterP, geoPropertyPath, geoPropertyPathLen, &location);

  bson_destroy(&location);
  bson_destroy(&intersects);
  bson_destroy(&geometry);
  bson_destroy(&coordinates);

  return true;
}



// -----------------------------------------------------------------------------
//
// geoEqualsFilter - just compare that they're equal
//
// FIXME:
//   I can't make this query work - no clue why it's failing.
//
//   I'm just comparing arrays.
//   Doesn't even work in mongo shell ...
//
//   However, mongoCppLegacyEntitiesQuery (ngsild_post_query_georel-equals.test) works and here is a trace line of the geo-query (from mongoCppLegacyEntitiesQuery.cpp[484]):
//     geoqFilter : EQ: { attrs.https://uri=etsi=org/ngsi-ld/default-context/geo.value: { type: "Polygon", coordinates: [ [ [ 0, 0 ], [ 0, 1 ], [ 1, 1 ], [ 0, 0 ] ] ] } }
//
//   The same trace line for mongocEntitiesQuery (mongocEntitiesQuery.cpp[709]):
//     mongocEntitiesQuery : Running the query with filter '{ "_id.type" : "https://uri.etsi.org/ngsi-ld/default-context/T", "attrs.geo.value" : { "type" : "Polygon", "coordinates" : [ [ [ 1, 1 ], [ 1, 2 ], [ 2, 2 ], [ 2, 1 ], [ 1, 1 ] ] ] } }'
//
//   Seems to be exactly the same, but works in mongoCppLegacy but not in mongoc ;(
//   So, I give up for now, setting the all geo-equals functests as DISABLED - will try to fix this some other day :(
//
static bool geoEqualsFilter(bson_t* mongoFilterP, OrionldGeoInfo* geoInfoP)
{
#if 1
  orionldError(OrionldOperationNotSupported, "Not Implemented", "Geo Equals Query", 501);
  return false;
#else
  bson_t location;
  bson_t coordinates;
  char   geoPropertyPath[512];
  int    geoPropertyPathLen;

  if (geoPropertyDbPath(geoPropertyPath, sizeof(geoPropertyPath), geoInfoP->geoProperty, &geoPropertyPathLen) == false)
    return false;

  mongocKjTreeToBson(geoInfoP->coordinates, &coordinates);

  bson_init(&location);
  bson_append_utf8(&location,  "type",         4, orionldGeometryToString(geoInfoP->geometry), -1);
  bson_append_array(&location, "coordinates", 11, &coordinates);

  bson_append_document(mongoFilterP, geoPropertyPath, geoPropertyPathLen, &location);

  bson_destroy(&location);

  return true;
#endif
}



// -----------------------------------------------------------------------------
//
// geoDisjointFilter - { "location":  { $not: { $geoIntersects: { $geometry: { type, coordinates }}}}}
//
static bool geoDisjointFilter(bson_t* mongoFilterP, OrionldGeoInfo* geoInfoP)
{
  bson_t location;
  bson_t notIntersects;
  bson_t intersects;
  bson_t geometry;
  bson_t coordinates;

  bson_init(&geometry);

#if 0
  //
  // disjoint ... should be valid for any geometry ...  Right?
  //
  // 'disjoint' queries use $geoIntersects" and ...
  // according to https://stackoverflow.com/questions/35656520/find-overlapping-trajectories,
  // geoIntersects requires polygons or multipolygons in the query,
  //
  // But, I haven't found this piece of info still in the mongo documentation, and it kind of seems to be working ...
  //
  if      (geoInfoP->geometry == GeoPolygon)       bson_append_utf8(&geometry,  "type", 4, "Polygon", 7);
  else if (geoInfoP->geometry == GeoMultiPolygon)  bson_append_utf8(&geometry,  "type", 4, "MultiPolygon", 12);
  else
  {
    bson_destroy(&geometry);
    orionldError(OrionldBadRequestData, "Invalid Geometry for Disjoint Query", orionldGeometryToString(geoInfoP->geometry), 400);
    return false;
  }
#endif

  bson_init(&location);
  bson_init(&notIntersects);
  bson_init(&intersects);
  bson_init(&coordinates);

  mongocKjTreeToBson(geoInfoP->coordinates, &coordinates);

  bson_append_utf8(&geometry,          "type",            4, orionldGeometryToString(geoInfoP->geometry), -1);
  bson_append_array(&geometry,         "coordinates",    11, &coordinates);
  bson_append_document(&intersects,    "$geometry",       9, &geometry);
  bson_append_document(&notIntersects, "$geoIntersects", 14, &intersects);
  bson_append_document(&location,      "$not",            4, &notIntersects);

  char geoPropertyPath[512];
  int  geoPropertyPathLen;
  if (geoPropertyDbPath(geoPropertyPath, sizeof(geoPropertyPath), geoInfoP->geoProperty, &geoPropertyPathLen) == false)
    return false;

  bson_append_document(mongoFilterP, geoPropertyPath, geoPropertyPathLen, &location);

  bson_destroy(&location);
  bson_destroy(&notIntersects);
  bson_destroy(&intersects);
  bson_destroy(&geometry);
  bson_destroy(&coordinates);

  return true;
}



// -----------------------------------------------------------------------------
//
// geoOverlapsFilter - intersects AND is of the same GEO-Type
//
// {
//   "location": {
//      $geoIntersects: {
//         $geometry: {
//            type: "<GeoJSON object type>" ,
//            coordinates: [ <coordinates> ]
//         }
//      }
//   }
// },
// {
//   "location.value.type": <geoType>
// }
//
static bool geoOverlapsFilter(bson_t* mongoFilterP, OrionldGeoInfo* geoInfoP)
{
  bson_t coordinates;
  bson_t geometry;
  bson_t intersects;
  bson_t location;

  bson_init(&geometry);

#if 0
  //
  // overlaps ... should be valid for any geometry ...  Right?
  //
  // 'overlaps' queries use $geoIntersects" and ...
  // according to https://stackoverflow.com/questions/35656520/find-overlapping-trajectories,
  // geoIntersects requires polygons or multipolygons in the query,
  //
  // But, I haven't found this piece of info still in the mongo documentation, and it kind of seems to be working ...
  //
  if      (geoInfoP->geometry == GeoPolygon)       bson_append_utf8(&geometry,  "type", 4, "Polygon", 7);
  else if (geoInfoP->geometry == GeoMultiPolygon)  bson_append_utf8(&geometry,  "type", 4, "MultiPolygon", 12);
  else
  {
    bson_destroy(&geometry);
    orionldError(OrionldBadRequestData, "Invalid Geometry for Overlaps Query", orionldGeometryToString(geoInfoP->geometry), 400);
    return false;
  }
#endif

  bson_init(&location);
  bson_init(&intersects);
  bson_init(&coordinates);

  mongocKjTreeToBson(geoInfoP->coordinates, &coordinates);

  bson_append_utf8(&geometry,          "type",            4, orionldGeometryToString(geoInfoP->geometry), -1);
  bson_append_array(&geometry,         "coordinates",    11, &coordinates);
  bson_append_document(&intersects,    "$geometry",       9, &geometry);
  bson_append_document(&location,      "$geoIntersects", 14, &intersects);

  char geoPropertyPath[512];
  int  geoPropertyPathLen;
  if (geoPropertyDbPath(geoPropertyPath, sizeof(geoPropertyPath), geoInfoP->geoProperty, &geoPropertyPathLen) == false)
    return false;

  bson_append_document(mongoFilterP, geoPropertyPath, geoPropertyPathLen, &location);

  strcat(geoPropertyPath, ".type");
  bson_append_utf8(mongoFilterP, geoPropertyPath, geoPropertyPathLen + 5, orionldGeometryToString(geoInfoP->geometry), -1);

  bson_destroy(&location);
  bson_destroy(&intersects);
  bson_destroy(&geometry);
  bson_destroy(&coordinates);

  return true;
}



// -----------------------------------------------------------------------------
//
// geoContainsFilter - Returns TRUE if no point of geography_2 is outside geography_1, and the interiors intersect
//
static bool geoContainsFilter(bson_t* mongoFilterP, OrionldGeoInfo* geoInfoP)
{
  orionldError(OrionldOperationNotSupported,  "Not Implemented", "georel 'contains' is not supported by mongodb and thus also not by Orion-LD", 501);
  return false;
}



// -----------------------------------------------------------------------------
//
// geoFilter -
//
bool geoFilter(bson_t* mongoFilterP, OrionldGeoInfo*  geoInfoP)
{
  switch (geoInfoP->georel)
  {
  case GeorelNear:        return geoNearFilter(mongoFilterP,       geoInfoP);
  case GeorelWithin:      return geoWithinFilter(mongoFilterP,     geoInfoP);
  case GeorelIntersects:  return geoIntersectsFilter(mongoFilterP, geoInfoP);
  case GeorelEquals:      return geoEqualsFilter(mongoFilterP,     geoInfoP);
  case GeorelDisjoint:    return geoDisjointFilter(mongoFilterP,   geoInfoP);
  case GeorelOverlaps:    return geoOverlapsFilter(mongoFilterP,   geoInfoP);
  case GeorelContains:    return geoContainsFilter(mongoFilterP,   geoInfoP);

  default:
    orionldError(OrionldOperationNotSupported, "Not Implemented", "Only near, within, and intersect implemented as of right now", 501);
    return false;
  }

  orionldError(OrionldBadRequestData, "Invalid Georel", orionldGeorelToString(geoInfoP->georel), 400);
  return false;
}



// -----------------------------------------------------------------------------
//
// mongocEntitiesQuery -
//
// Parameters passed via orionldState:
// - tenant
// - limit
// - offset
// - count
// - geometry
// - georel
// - coordinates
// - geoproperty
//
KjNode* mongocEntitiesQuery
(
  StringArray*     entityTypeList,
  StringArray*     entityIdList,
  const char*      entityIdPattern,
  StringArray*     attrList,
  QNode*           qNode,
  OrionldGeoInfo*  geoInfoP,
  int64_t*         countP,
  const char*      geojsonGeometry,
  bool             onlyIds,
  bool             onlyIdAndType
)
{
  if ((attrList != NULL) && (attrList->items > 99))
  {
    orionldError(OrionldBadRequestData, "Too many attributes", "maximum is 99", 400);
    return NULL;
  }

  bson_t                mongoFilter;
  const bson_t*         mongoDocP;
  mongoc_cursor_t*      mongoCursorP;
  char*                 title;
  char*                 detail;
  KjNode*               entityNodeP = NULL;
  mongoc_read_prefs_t*  readPrefs   = mongoc_read_prefs_new(MONGOC_READ_NEAREST);

  //
  // Sort, Limit, Offset
  //
  bson_t options;
  bson_t projection;

  bson_init(&options);
  bson_init(&projection);

  if (onlyIds == false)
  {
    bson_t sortDoc;
    int    limit       = orionldState.uriParams.limit;
    int    offset      = orionldState.uriParams.offset;

    bson_init(&sortDoc);

    bson_append_int32(&sortDoc, "creDate", 7, 1);
    bson_append_int32(&sortDoc, "_id.id", 6, 1);
    bson_append_document(&options, "sort", 4, &sortDoc);
    bson_destroy(&sortDoc);

    bson_append_int32(&options, "limit", 5, limit);
    if (offset != 0)
      bson_append_int32(&options, "skip", 4, offset);


    //
    // Projection (will be added to if attrList != NULL)
    //
    bson_append_bool(&projection, "_id.id",          6, true);
    bson_append_bool(&projection, "_id.type",        8, true);
    bson_append_bool(&projection, "attrNames",       9, true);
    bson_append_bool(&projection, "creDate",         7, true);
    bson_append_bool(&projection, "modDate",         7, true);
    bson_append_bool(&projection, "lastCorrelator", 14, true);
  }
  else if (onlyIdAndType == true)
  {
    bson_append_bool(&projection, "_id.id",   6, true);
    bson_append_bool(&projection, "_id.type", 8, true);
  }
  else
    bson_append_bool(&projection, "_id.id", 6, true);

  //
  // Create the filter for the query
  //
  bson_init(&mongoFilter);

  // Entity Types
  if ((entityTypeList != NULL) && (entityTypeList->items > 0))
  {
    if (entityTypeFilter(&mongoFilter, entityTypeList) == false)
      return NULL;
  }

  // Entity IDs
  if ((entityIdList != NULL) && (entityIdList->items > 0))
  {
    if (entityIdFilter(&mongoFilter, entityIdList) == false)
      return NULL;
  }

  // Entity ID-Pattern
  if (entityIdPattern != NULL)
  {
    if (entityIdPatternFilter(&mongoFilter, entityIdPattern) == false)
      return NULL;
  }

  // Attribute List
  if ((attrList != NULL) && (attrList->items > 0))
  {
    if (mongocAuxAttributesFilter(&mongoFilter, attrList, &projection, geojsonGeometry, onlyIds) == false)
      return NULL;
  }
  else
  {
    if (onlyIds == false)
    {
      bson_append_bool(&projection, "attrs",     5, true);
      bson_append_bool(&projection, "@datasets", 9, true);
    }
  }

  // Query Language
  if (qNode != NULL)
  {
    if (qFilter(&mongoFilter, qNode) == false)
      return NULL;
  }

  // GEO Query
  if ((geoInfoP != NULL) && (geoInfoP->geometry != GeoNoGeometry))
  {
    if (geoFilter(&mongoFilter, geoInfoP) == false)
      return NULL;
  }

  bson_append_document(&options, "projection", 10, &projection);
  bson_destroy(&projection);

  mongocConnectionGet(orionldState.tenantP, DbEntities);

  // semTake(&mongoEntitiesSem);

  // count?
  if (countP != NULL)
  {
    bson_error_t error;

    *countP = mongoc_collection_count_documents(orionldState.mongoc.entitiesP, &mongoFilter, NULL, readPrefs, NULL, &error);
    if (*countP == -1)
    {
      *countP = 0;
      LM_E(("Database Error (error counting entities: %d.%d: %s)", error.domain, error.code, error.message));
    }
  }

  //
  // Run the query
  //
  KjNode* entityArray = kjArray(orionldState.kjsonP, NULL);

  if (orionldState.uriParams.limit != 0)
  {
    MONGOC_RLOG("Querying Entities", orionldState.tenantP->mongoDbName, "entities", &mongoFilter, &options, LmtMongoc);
    mongoCursorP = mongoc_collection_find_with_opts(orionldState.mongoc.entitiesP, &mongoFilter, &options, readPrefs);
    bson_destroy(&options);

    if (mongoCursorP == NULL)
    {
      LM_E(("Database Error (mongoc_collection_find_with_opts ERROR)"));
      bson_destroy(&mongoFilter);
      mongoc_read_prefs_destroy(readPrefs);
      orionldError(OrionldInternalError, "Database Error", "mongoc_collection_find_with_opts failed", 500);
      return NULL;
    }

    const bson_t* lastError = mongoc_collection_get_last_error(orionldState.mongoc.entitiesP);
    if (lastError != NULL)
      LM_E(("MongoC Error: %s", bson_as_canonical_extended_json(lastError, NULL)));

    int hits = 0;
    while (mongoc_cursor_next(mongoCursorP, &mongoDocP))
    {
      entityNodeP = mongocKjTreeFromBson(mongoDocP, &title, &detail);

      if (entityNodeP != NULL)
      {
        kjChildAdd(entityArray, entityNodeP);
        ++hits;
      }
      else
        LM_E(("Database Error (%s: %s)", title, detail));
    }

    bson_error_t error;
    if (mongoc_cursor_error(mongoCursorP, &error) == true)
      LM_E(("mongoc_cursor_error: %d.%d: '%s'", error.domain, error.code, error.message));

    mongoc_cursor_destroy(mongoCursorP);
  }
  else
    bson_destroy(&options);

  // semGive(&mongoEntitiesSem);
  bson_destroy(&mongoFilter);
  mongoc_read_prefs_destroy(readPrefs);

  return entityArray;
}
