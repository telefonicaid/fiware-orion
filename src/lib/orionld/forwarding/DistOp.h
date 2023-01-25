#ifndef SRC_LIB_ORIONLD_FORWARDING_DISTOP_H_
#define SRC_LIB_ORIONLD_FORWARDING_DISTOP_H_

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
#include "orionld/forwarding/DistOpType.h"                       // DistOpType
#include "orionld/regCache/RegCache.h"                           // RegCacheItem



// -----------------------------------------------------------------------------
//
// DistOp -
//
typedef struct DistOp
{
  RegCacheItem*       regP;
  DistOpType          operation;
  KjNode*             body;              // For Create/Update Requests (also used for GET - tree of response)
  char*               rawResponse;       // Response buffer as raw ASCII as it was received by libcurl (parsed and stored as DistOp::body)
  int                 httpResponseCode;  // Response HTTP Status Code
  KjNode*             responseBody;      // Parsed body of the response
  char*               entityId;          // Used by GET /entities/{entityId} (and as intermediate result for BATCH Delete)
  char*               entityType;        // Used by GET /entities/{entityId}
  char*               attrName;          // Used by PATCH /entities/{entityId}/attrs/{attrName}
  StringArray*        attrList;          // URI Param "attrs" for GET Requests
  char*               geoProp;           // URI Param "geometryProperty" for GET Requests
  bool                error;
  CURL*               curlHandle;
  struct curl_slist*  curlHeaders;
  struct DistOp*      next;
} DistOp;

#endif  // SRC_LIB_ORIONLD_FORWARDING_DISTOP_H_
