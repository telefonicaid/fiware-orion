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

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/mongoc/mongocAuxAttributesFilter.h"            // Own interface



// -----------------------------------------------------------------------------
//
// mongocAuxAttributesFilter -
//
bool mongocAuxAttributesFilter(bson_t* mongoFilterP, StringArray* attrList, bson_t* projectionP, const char* geojsonGeometry, bool onlyIds)
{
  char    path[512];  // Assuming 512 is always enough ...
  bson_t  exists;
  bool    geojsonGeometryToProjection = (geojsonGeometry == NULL)? false : true;  // if GEOJSON, the "geometry" must be present

  if (onlyIds == true)
    geojsonGeometryToProjection = false;

  bson_init(&exists);
  bson_append_int32(&exists, "$exists", 7, 1);

  //
  // Remember, an attribute may be either in "attrs", or "@datasets", or both ...
  // Example filter for attr A1 and A2:
  //
  // { $or: [ { attrs.A1: {$exists: 1} }, { attrs.A2: {$exists: 1} }, { @datasets.A1: {$exists: 1}}, { @datasets.A2: {$exists: 1}} ] }
  //
  bson_t array;
  bson_init(&array);

  char num[3] = { '0', '0', 0 };
  int  numLen = 1;

  //
  // First "attrs.X"
  //
  for (int ix = 0; ix < attrList->items; ix++)
  {
    int    len = snprintf(path, sizeof(path) - 1, "attrs.%s", attrList->array[ix]);
    bson_t attrExists;

    bson_init(&attrExists);
    dotForEq(&path[6]);

    bson_append_document(&attrExists, path, len, &exists);

    if (onlyIds == false)
      bson_append_bool(projectionP, path, len, true);

    bson_append_document(&array, &num[2-numLen], numLen, &attrExists);

    num[1] += 1;
    if (((ix + 1) % 10) == 0)
    {
      num[1] = '0';
      num[0] += 1;
      numLen = 2;
    }

    bson_destroy(&attrExists);

    if ((geojsonGeometry != NULL) && (strcmp(attrList->array[ix], geojsonGeometry) == 0))
    {
      geojsonGeometryToProjection = false;  // Already present - no need to add to projection
    }
  }

  //
  // Then "@datasets.X" - geojsonGeometryToProjection is already taken care of by "attrs" loop
  //
  if (onlyIds == false)
  {
    int offset = attrList->items;
    for (int ix = 0; ix < attrList->items; ix++)
    {
      int    len = snprintf(path, sizeof(path) - 1, "@datasets.%s", attrList->array[ix]);
      bson_t attrExists;

      bson_init(&attrExists);
      dotForEq(&path[10]);

      bson_append_document(&attrExists, path, len, &exists);
      bson_append_bool(projectionP, path, len, true);

      bson_append_document(&array, &num[2-numLen], numLen, &attrExists);

      num[1] += 1;
      if (((offset + ix + 1) % 10) == 0)
      {
        num[1] = '0';
        num[0] += 1;
        numLen = 2;
      }

      bson_destroy(&attrExists);
    }
  }

  bson_append_array(mongoFilterP, "$or", 3, &array);
  bson_destroy(&array);

  if (geojsonGeometryToProjection == true)
  {
    int len = snprintf(path, sizeof(path) - 1, "attrs.%s", geojsonGeometry);
    dotForEq(&path[6]);
    bson_append_bool(projectionP, path, len, true);
    orionldState.geoPropertyFromProjection = true;

    // What about the dataset ... ?
  }

  bson_destroy(&exists);
  return true;
}
