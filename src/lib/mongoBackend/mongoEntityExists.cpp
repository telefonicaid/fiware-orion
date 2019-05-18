/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <string>

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/statistics.h"

#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoEntityExists.h"



// -----------------------------------------------------------------------------
//
// USING
//
using mongo::DBClientBase;
using mongo::BSONObjBuilder;
using mongo::BSONObj;
using mongo::DBClientCursor;



// -----------------------------------------------------------------------------
//
// mongoEntityExists -
//
bool mongoEntityExists(const char* entityId, const char* tenant)
{
  BSONObjBuilder bob;

  bob.append("_id." ENT_ENTITY_ID, entityId);

  /* Do the query on MongoDB */
  std::auto_ptr<DBClientCursor>  cursor;
  BSONObj                        query = bob.obj();

  TIME_STAT_MONGO_READ_WAIT_START();

  DBClientBase* connection = getMongoConnection();
  std::string   err;

  if (collectionQuery(connection, getEntitiesCollectionName(tenant), query, &cursor, &err) == false)
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    return false;
  }

  unsigned int docs = 0;

  while (moreSafe(cursor))
  {
    BSONObj  bo;

    try
    {
      bo = cursor->nextSafe();
      ++docs;
    }
    catch (...)
    {
    }
  }

  releaseMongoConnection(connection);

  return (docs == 0)? false : true;
}
