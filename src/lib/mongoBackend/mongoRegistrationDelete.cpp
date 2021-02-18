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
* Author: Burak Karaboga
*/
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"
#include "common/statistics.h"
#include "common/errorMessages.h"
#include "rest/OrionError.h"
#include "rest/HttpStatusCode.h"
#include "apiTypesV2/Registration.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoRegistrationDelete.h"

#include "mongoDriver/safeMongo.h"
#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/mongoConnectionPool.h"
#include "mongoDriver/BSONObjBuilder.h"



/* ****************************************************************************
*
* mongoRegistrationDelete - 
*/
void mongoRegistrationDelete
(
  const std::string&     regId,
  const std::string&     tenant,
  const std::string&     servicePath,
  OrionError*            oeP
)
{
  bool         reqSemTaken = false;
  std::string  err;
  orion::OID   oid = orion::OID(regId);

  reqSemTake(__FUNCTION__, "Mongo Delete Registration", SemWriteOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Delete Registration"));

  orion::DBCursor  cursor;
  orion::BSONObj   q;

  orion::BSONObjBuilder  bob;
  bob.append("_id", oid);

  q = bob.obj();

  TIME_STAT_MONGO_READ_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();

  // FIXME P5: maybe an implemetnation based in collectionFindOne() would be better, have a look to mongoUnsubscribeContext.cpp
  // Note also that current implementation calls collectionRemove(), which uses a connection internally, without having
  // released the connection object (this is not a big problem, but a bit unneficient).
  // If the change is done, then MB-27 diagram (and related texts) in devel manual should be also changed.
  if (!orion::collectionQuery(connection, composeDatabaseName(tenant), COL_REGISTRATIONS, q, &cursor, &err))
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo Delete Registration", reqSemTaken);
    oeP->fill(SccReceiverInternalError, err);
    return;
  }

  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  orion::BSONObj r;
  if (cursor.next(&r))
  {
    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

    if (!orion::collectionRemove(composeDatabaseName(tenant), COL_REGISTRATIONS, q, &err))
    {
      orion::releaseMongoConnection(connection);
      LM_E(("Runtime Error (exception in collectionRemove(): %s - query: %s", err.c_str(), q.toString().c_str()));
      reqSemGive(__FUNCTION__, "Mongo Delete Registration", reqSemTaken);
      oeP->fill(SccReceiverInternalError, std::string("exception in collectionRemove(): ") + err.c_str());
      return;
    }
  }
  else
  {
    orion::releaseMongoConnection(connection);
    LM_T(LmtMongo, ("registration not found: '%s'", regId.c_str()));
    reqSemGive(__FUNCTION__, "Mongo Delete Registration", reqSemTaken);
    oeP->fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_REGISTRATION, ERROR_NOT_FOUND);

    return;
  }

  orion::releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo Delete Registration", reqSemTaken);

  oeP->fill(SccNoContent, "");
}
