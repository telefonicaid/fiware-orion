#ifndef SRC_LIB_ORIONLD_TYPES_DISTOP_H_
#define SRC_LIB_ORIONLD_TYPES_DISTOP_H_

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
#include <curl/curl.h>                                           // CURL

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/types/OrionldGeoInfo.h"                        // OrionldGeoInfo
#include "orionld/types/QNode.h"                                 // QNode
#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/types/DistOpType.h"                            // DistOpType



// -----------------------------------------------------------------------------
//
// DistOp -
//
typedef struct DistOp
{
  char                id[16];            // Unique identifier for this DistOp
  RegCacheItem*       regP;              // Pointer to the registration cache item
  DistOpType          operation;         // Operation
  KjNode*             requestBody;       // For Create/Update Requests (also used for GET - tree of response)

  char*               rawResponse;       // Response buffer as raw ASCII as it was received by libcurl (parsed and stored as DistOp::body)
  uint64_t            httpResponseCode;  // Response HTTP Status Code  (64 bit due to libcurl curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE))
  KjNode*             responseBody;      // Parsed body of the response

  char*               entityId;          // Used by GET /entities/{entityId}, BATCH /delete, and GET /entities
  char*               entityIdPattern;   // Used by GET /entities
  char*               entityType;        // Used by GET /entities/{entityId} and GET /entities
  char*               attrName;          // Used by PATCH /entities/{entityId}/attrs/{attrName}
  StringArray*        attrList;          // Attribute list - for URI Param "attrs" for GET Requests
  char*               attrsParam;        // Rendered attrs URL parameter - to do it just once
  int                 attrsParamLen;     // Length of attrsParam to avoid a call to strlen()

  StringArray*        idList;            // Used by GET /entities (unless entityId is used)
  StringArray*        typeList;          // Used by GET /entities (unless entityType is used)
  bool                onlyIds;           // Used to compile the list of entity ids in the preparation for GET /entities

  OrionldGeoInfo      geoInfo;
  QNode*              qNode;
  char*               lang;
  char*               geometryProperty;  // URI Param "geometryProperty" for GET Requests

  bool                error;
  char*               title;
  char*               detail;

  CURL*               curlHandle;
  struct curl_slist*  curlHeaders;
  struct DistOp*      next;
} DistOp;

#endif  // SRC_LIB_ORIONLD_TYPES_DISTOP_H_
