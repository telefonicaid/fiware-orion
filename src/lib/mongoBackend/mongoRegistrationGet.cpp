/*
*
* Copyright 2017 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "mongoBackend/mongoRegistrationGet.h"

#include "mongoDriver/safeMongo.h"
#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/mongoConnectionPool.h"
#include "mongoDriver/BSONObjBuilder.h"



/* ****************************************************************************
*
* mongoRegistrationGet - 
*/
void mongoRegistrationGet
(
  ngsiv2::Registration*  regP,
  const std::string&     regId,
  const std::string&     tenant,
  const std::string&     servicePath,
  OrionError*            oeP
)
{
  bool         reqSemTaken = false;
  std::string  err;
  orion::OID   oid = orion::OID(regId);

  reqSemTake(__FUNCTION__, "Mongo Get Registration", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Registration"));

  orion::DBCursor  cursor;
  orion::BSONObj   q;

  orion::BSONObjBuilder bob;
  bob.append("_id", oid);
  if (!servicePath.empty())
  {
    bob.append(REG_SERVICE_PATH, servicePath);
  }
  q = bob.obj();

  TIME_STAT_MONGO_READ_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();
  if (!orion::collectionQuery(connection, composeDatabaseName(tenant), COL_REGISTRATIONS, q, &cursor, &err))
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
    oeP->fill(SccReceiverInternalError, err);
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  orion::BSONObj r;
  if (cursor.next(&r))
  {
    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

    //
    // Fill in the Registration with data retrieved from the data base
    //
    if (!regP->fromBson(r))
    {
      // FIXME #4611: this check will be no longer needed after fixing the issue. fromBson return type could be changed to void
      orion::releaseMongoConnection(connection);
      LM_E(("Runtime Error (registrations with more than one CR are considered runtime errors since Orion 4.1.0, please fix reg %s at DB)", regP->id.c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      oeP->fill(SccReceiverInternalError, err);
      return;
    }
  }
  else
  {
    orion::releaseMongoConnection(connection);
    LM_T(LmtMongo, ("registration not found: '%s'", regId.c_str()));
    reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
    oeP->fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_REGISTRATION, ERROR_NOT_FOUND);

    return;
  }

  orion::releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);

  oeP->fill(SccOk, "");
}



/* ****************************************************************************
*
* mongoRegistrationsGet - 
*/
void mongoRegistrationsGet
(
  std::vector<ngsiv2::Registration>*  regV,
  const std::string&                  tenant,
  const std::vector<std::string>&     servicePathV,
  int                                 offset,
  int                                 limit,
  long long*                          countP,
  OrionError*                         oeP
)
{
  bool         reqSemTaken = false;
  std::string  err;
  orion::OID   oid;
  OrionError   oe;
  std::string  servicePath = (servicePathV.size() == 0)? "/#" : servicePathV[0];  // FIXME P4: see #3100

  reqSemTake(__FUNCTION__, "Mongo Get Registrations", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Registrations"));

  orion::DBCursor  cursor;
  orion::BSONObj   q;

  // FIXME P6: This here is a bug ... See #3099 for more info
  if (!servicePath.empty() && (servicePath != "/#"))
  {
    orion::BSONObjBuilder bob;
    bob.append(REG_SERVICE_PATH, servicePathV[0]);
    q = bob.obj();
  }

  orion::BSONObjBuilder bSort;
  bSort.append("_id", 1);

  TIME_STAT_MONGO_READ_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();
  if (!orion::collectionRangedQuery(connection, composeDatabaseName(tenant), COL_REGISTRATIONS, q, q, bSort.obj(), limit, offset, &cursor, countP, &err))
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo Get Registrations", reqSemTaken);
    oeP->fill(SccReceiverInternalError, err);
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  // Note limit != 0 will cause skipping the while loop in case request didn't actually ask for any result */
  int docs = 0;
  orion::BSONObj        r;
  while ((limit != 0) && (cursor.next(&r)))
  {
    ngsiv2::Registration  reg;

    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, r.toString().c_str()));
    ++docs;

    //
    // Fill in the Registration with data retrieved from the data base
    //
    if (!reg.fromBson(r))
    {
      // FIXME #4611: this check will be no longer needed after fixing the issue. fromBson return type could be changed to void
      orion::releaseMongoConnection(connection);
      LM_E(("Runtime Error (registrations with more than one CR are considered runtime errors since Orion 4.1.0, please fix reg %s at DB)", reg.id.c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Registrations", reqSemTaken);
      oeP->fill(SccReceiverInternalError, err);
      return;
    }

    regV->push_back(reg);
  }

  orion::releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo Get Registrations", reqSemTaken);

  oeP->fill(SccOk, "");
}
