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
* Author: Ken Zangelin, Larysse Savanna and Gabriel Quaresma
*/
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "apiTypesV2/Registration.h"                           // ngsiv2::Registration
#include "rest/HttpStatusCode.h"                               // HttpStatusCode
#include "common/statistics.h"                                 // TIME_STAT_MONGO_READ_WAIT_START
#include "mongoBackend/MongoGlobal.h"                          // getMongoConnection
#include "mongoBackend/connectionOperations.h"                 // collectionQuery
#include "mongoBackend/safeMongo.h"                            // moreSafe
#include "mongoBackend/mongoRegistrationAux.h"                 // mongoSetXxx
#include "mongoBackend/mongoLdRegistrationAux.h"               // mongoSetLdRelationshipV, mongoSetLdPropertyV, ...
#include "mongoBackend/mongoLdRegistrationGet.h"               // Own interface



/* ****************************************************************************
*
* mongoLdRegistrationGet -
*/
bool mongoLdRegistrationGet
(
  ngsiv2::Registration*  regP,
  const char*            regId,
  const char*            tenant,
  HttpStatusCode*        statusCodeP,
  char**                 detailsP
)
{
  bool                                  reqSemTaken = false;
  std::string                           err;
  std::auto_ptr<mongo::DBClientCursor>  cursor;
  mongo::BSONObj                        q = BSON("_id" << regId);

  reqSemTake(__FUNCTION__, "Mongo Get Registration", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Registration"));

  TIME_STAT_MONGO_READ_WAIT_START();
  mongo::DBClientBase* connection = getMongoConnection();
  if (!collectionQuery(connection, getRegistrationsCollectionName(tenant), q, &cursor, &err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
    *detailsP    = (char*) "Internal Error during DB-query";
    *statusCodeP = SccReceiverInternalError;
    return false;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  LM_TMP(("qBSON %s", q.toString().c_str()));

  /* Process query result */
  if (moreSafe(cursor))
  {
    mongo::BSONObj bob;

    if (!nextSafeOrErrorF(cursor, &bob, &err))
    {
      releaseMongoConnection(connection);
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), q.toString().c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      *detailsP    = (char*) "Runtime Error (exception in nextSafe)";
      *statusCodeP = SccReceiverInternalError;
      return false;
    }
    LM_T(LmtMongo, ("retrieved document: '%s'", bob.toString().c_str()));

    mongoSetLdRegistrationId(regP, bob);
    mongoSetLdName(regP, bob);
    mongoSetDescription(regP, bob);

    if (mongoSetDataProvided(regP, bob, false) == false)
    {
      releaseMongoConnection(connection);
      LM_W(("Bad Input (getting registrations with more than one CR is not yet implemented, see issue 3044)"));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      *statusCodeP = SccReceiverInternalError;
      return false;
    }

    mongoSetLdObservationInterval(regP, bob);
    mongoSetLdManagementInterval(regP, bob);
    mongoSetExpires(regP, bob);
    mongoSetStatus(regP, bob);

    if (moreSafe(cursor))
    {
      releaseMongoConnection(connection);

      // Ooops, we expected only one
      LM_T(LmtMongo, ("more than one registration: '%s'", regId));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      *detailsP    = (char*) "more than one registration matched";
      *statusCodeP = SccConflict;
      return false;
    }
  }
  else
  {
    releaseMongoConnection(connection);
    LM_T(LmtMongo, ("registration not found: '%s'", regId));
    reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
    *detailsP    = (char*) "registration not found";
    *statusCodeP = SccContextElementNotFound;
    return false;
  }

  releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);

  *statusCodeP = SccOk;
  return true;
}
