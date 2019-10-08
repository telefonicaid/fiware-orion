/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include "orionld/db/dbConfiguration.h"                                    // This is where the DB is selected

#if DB_DRIVER_MONGO_CPP_LEGACY

#include "orionld/mongoCppLegacy/mongoCppLegacyInit.h"                     // mongoCppLegacyInit
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityUpdate.h"             // mongoCppLegacyEntityUpdate
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityLookup.h"             // mongoCppLegacyEntityLookup
#include "orionld/mongoCppLegacy/mongoCppLegacyKjTreeFromBsonObj.h"        // mongoCppLegacyKjTreeFromBsonObj
#include "orionld/mongoCppLegacy/mongoCppLegacyKjTreeToBsonObj.h"          // mongoCppLegacyKjTreeToBsonObj
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityOperationsUpsert.h"   // mongoCppLegacyKjTreeToBsonObj
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityBatchDelete.h"        // mongoCppLegacyEntityBatchDelete
#include "orionld/mongoCppLegacy/mongoCppLegacySubscriptionMatchEntityIdAndAttributes.h"   // mongoCppLegacySubscriptionMatchEntityIdAndAttributes

#elif DB_DRIVER_MONGOC
#include "orionld/mongoc/mongocInit.h"                                     // mongocInit
#include "orionld/mongoc/mongocEntityUpdate.h"                             // mongocEntityUpdate
#include "orionld/mongoc/mongocEntityLookup.h"                             // mongocEntityLookup
#include "orionld/mongoc/mongocKjTreeFromBson.h"                           // mongocKjTreeFromBson
#endif
#include "orionld/db/dbInit.h"                                             // Own interface



// ----------------------------------------------------------------------------
//
// dbInit -
//
// PARAMETERS
//   dbHost - the host and port where the mongobd server runs. E.g. "localhost:27017"
//   dbName - the name of the database. Default is 'orion'
//
void dbInit(const char* dbHost, const char* dbName)
{
#if DB_DRIVER_MONGO_CPP_LEGACY

  dbEntityLookup                           = mongoCppLegacyEntityLookup;
  dbEntityUpdate                           = mongoCppLegacyEntityUpdate;
  dbDataToKjTree                           = mongoCppLegacyKjTreeFromBsonObj;
  dbDataFromKjTree                         = mongoCppLegacyKjTreeToBsonObj;
  dbEntityBatchDelete                      = mongoCppLegacyEntityBatchDelete;
  dbEntityOperationsUpsert                 = mongoCppLegacyEntityOperationsUpsert;
  dbSubscriptionMatchEntityIdAndAttributes = mongoCppLegacySubscriptionMatchEntityIdAndAttributes;

  mongoCppLegacyInit(dbHost, dbName);

#elif DB_DRIVER_MONGOC

  dbEntityLookup                           = mongocEntityLookup;
  dbEntityUpdate                           = mongocEntityUpdate;
  dbDataToKjTree                           = mongocKjTreeFromBsonObj;
  dbDataFromKjTree                         = NULL;  // FIXME: Implement mongocKjTreeToBson
  dbSubscriptionMatchEntityIdAndAttributes = NULL;  // FIXME: Implement mongocSubscriptionMatchEntityIdAndAttributes

  mongocInit(dbHost, dbName);

#else
  #error Please define either DB_DRIVER_MONGO_CPP_LEGACY or DB_DRIVER_MONGOC in src/lib/orionld/db/dbConfiguration.h
#endif
}
