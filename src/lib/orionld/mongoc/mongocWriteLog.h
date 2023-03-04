#ifndef SRC_LIB_ORIONLD_MONGOC_MONGOCWRITELOG_H_
#define SRC_LIB_ORIONLD_MONGOC_MONGOCWRITELOG_H_

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
#include <bson/bson.h>                                           // bson_t

#include "logMsg/logMsg.h"                                       // TraceLevels



// -----------------------------------------------------------------------------
//
// MONGOC_WLOG -
//
#define MONGOC_WLOG(msg, dbName, collectionName, selectorP, requestP, traceLevel)                                   \
do                                                                                                                  \
{                                                                                                                   \
  if (LM_MASK(LogLevelDebug) && lmOk('T', traceLevel) == LmsOk)                                                     \
    mongocWriteLog(msg, dbName, collectionName, selectorP, requestP, __FILE__, __LINE__, __FUNCTION__, traceLevel); \
} while (0)



// -----------------------------------------------------------------------------
//
// MONGOC_RLOG -
//
#define MONGOC_RLOG(msg, dbName, collectionName, filterP, optionsP, traceLevel)                                     \
do                                                                                                                  \
{                                                                                                                   \
  if (LM_MASK(LogLevelDebug) && lmOk('T', traceLevel) == LmsOk)                                                     \
    mongocReadLog(msg, dbName, collectionName, filterP, optionsP, __FILE__, __LINE__, __FUNCTION__, traceLevel);    \
} while (0)



// -----------------------------------------------------------------------------
//
// mongocWriteLog -
//
// NOTE
//   This function is not meant to be called directly. Always via the MONGOC_WLOG macro
//
extern void mongocWriteLog
(
  const char*  msg,
  const char*  dbName,
  const char*  collectionName,
  bson_t*      selectorP,
  bson_t*      requestP,
  const char*  fileName,
  int          lineNo,
  const char*  functionName,
  int          traceLevel
);



// -----------------------------------------------------------------------------
//
// mongocReadLog -
//
// NOTE
//   This function is not meant to be called directly. Always via the MONGOC_RLOG macro
//
extern void mongocReadLog
(
  const char*  msg,
  const char*  dbName,
  const char*  collectionName,
  bson_t*      filterP,
  bson_t*      optionsP,
  const char*  fileName,
  int          lineNo,
  const char*  functionName,
  int          traceLevel
);

#endif  // SRC_LIB_ORIONLD_MONGOC_MONGOCWRITELOG_H_
