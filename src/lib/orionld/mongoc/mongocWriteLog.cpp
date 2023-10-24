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
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // TraceLevels, LM_T
#include "orionld/common/fileName.h"                             // fileName
#include "orionld/mongoc/mongocWriteLog.h"                       // Own interface



// -----------------------------------------------------------------------------
//
// mongocWriteLog -
//
void mongocWriteLog
(
  const char*  msg,
  const char*  dbName,
  const char*  collectionName,
  bson_t*      selectorP,
  bson_t*      requestP,
  const char*  path,
  int          lineNo,
  const char*  functionName,
  int          traceLevel
)
{
  char* fileNameOnly = fileName(path);

  char line[2048];

  snprintf(line, sizeof(line), "---------- %s ----------", msg);
  lmOut(line,     'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);

  snprintf(line, sizeof(line), "  * Database Name:         '%s'", dbName);
  lmOut(line,     'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);

  snprintf(line, sizeof(line), "  * Collection Name:       '%s'", collectionName);
  lmOut(line,     'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);

  if (selectorP != NULL)
  {
    char* selector = bson_as_json(selectorP, NULL);

    snprintf(line, sizeof(line), "  * Selector:              '%s'", selector);
    lmOut(line, 'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);
    bson_free(selector);
  }

  if (requestP != NULL)
  {
    char* request = bson_as_json(requestP, NULL);

    snprintf(line, sizeof(line), "  * Request:               '%s'", request);
    lmOut(line,     'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);
    bson_free(request);
  }
}



// -----------------------------------------------------------------------------
//
// mongocReadLog -
//
void mongocReadLog
(
  const char*  msg,
  const char*  dbName,
  const char*  collectionName,
  bson_t*      filterP,
  bson_t*      optionsP,
  const char*  path,
  int          lineNo,
  const char*  functionName,
  int          traceLevel
)
{
  char* fileNameOnly = fileName(path);

  char line[2048];

  snprintf(line, sizeof(line), "---------- %s ----------", msg);
  lmOut(line,     'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);

  snprintf(line, sizeof(line), "  * Database Name:         '%s'", dbName);
  lmOut(line,     'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);

  snprintf(line, sizeof(line), "  * Collection Name:       '%s'", collectionName);
  lmOut(line,     'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);


  if (filterP != NULL)
  {
    char* filter = bson_as_json(filterP, NULL);
    snprintf(line, sizeof(line), "  * Filter:                '%s'", filter);
    lmOut(line,     'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);
    bson_free(filter);
  }

  if (optionsP != NULL)
  {
    char* options = bson_as_json(optionsP, NULL);
    snprintf(line, sizeof(line), "  * Options:             '%s'", options);
    lmOut(line, 'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);
    bson_free(options);
  }
}
