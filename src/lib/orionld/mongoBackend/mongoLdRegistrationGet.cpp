/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
* Author: Ken Zangelin, Larysse Savanna and Gabriel Quaresma
*/
#include <string>                                              // std::string

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "apiTypesV2/Registration.h"                           // ngsiv2::Registration
#include "common/statistics.h"                                 // TIME_STAT_MONGO_READ_WAIT_START
#include "mongoBackend/MongoGlobal.h"                          // getMongoConnection
#include "mongoBackend/connectionOperations.h"                 // collectionQuery
#include "mongoBackend/safeMongo.h"                            // moreSafe
#include "mongoBackend/mongoRegistrationAux.h"                 // mongoSetXxx
#include "orionld/mongoBackend/mongoLdRegistrationAux.h"       // mongoSetLdRelationshipV, mongoSetLdPropertyV, ...
#include "orionld/mongoBackend/mongoLdRegistrationGet.h"       // Own interface



/* ****************************************************************************
*
* mongoLdRegistrationGet -
*/
bool mongoLdRegistrationGet
(
  ngsiv2::Registration*  regP,
  const char*            regId,
  const char*            tenant,
  int*                   statusCodeP,
  char**                 detailP
)
{
  bool                                  reqSemTaken = false;
  std::string                           err;
  std::auto_ptr<mongo::DBClientCursor>  cursor;
  char*                                 title;
  mongo::BSONObj                        query = BSON("_id" << regId);

  reqSemTake(__FUNCTION__, "Mongo Get Registration", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Registration"));

  //
  // Query
  //
  TIME_STAT_MONGO_READ_WAIT_START();
  mongo::DBClientBase* connection = getMongoConnection();
  if (!collectionQuery(connection, getRegistrationsCollectionName(tenant), query, &cursor, &err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
    *detailP     = (char*) "Internal Error during DB-query";
    *statusCodeP = SccReceiverInternalError;
    return false;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();


  //
  // Process Query Result
  //
  if (moreSafe(cursor))
  {
    mongo::BSONObj bob;

    if (!nextSafeOrErrorF(cursor, &bob, &err))
    {
      releaseMongoConnection(connection);
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), query.toString().c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      *detailP     = (char*) "Runtime Error (exception in nextSafe)";
      *statusCodeP = SccReceiverInternalError;
      return false;
    }
    LM_T(LmtMongo, ("retrieved document: '%s'", bob.toString().c_str()));

    if (regP == NULL)
    {
      //
      // Can't fill in the registration as no reg pointer is supplied.
      // This is used for when we're only interested in whether a registration exists.
      // Used by orionldPostRegistrations().
      //
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);

      return true;
    }

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
    mongoSetExpiration(regP, bob);
    mongoSetStatus(regP, bob);

    mongoSetLdTimestamp(&regP->createdAt, "createdAt", bob);
    mongoSetLdTimestamp(&regP->modifiedAt, "modifiedAt", bob);

    if (mongoSetLdTimeInterval(&regP->location, "location", bob, &title, detailP) == false)
    {
      LM_E(("Internal Error (mongoSetLdTimeInterval: %s: %s)", title, *detailP));
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      *statusCodeP = SccReceiverInternalError;
      return false;
    }

    if (mongoSetLdTimeInterval(&regP->observationSpace, "observationSpace", bob, &title, detailP) == false)
    {
      LM_E(("Internal Error (mongoSetLdTimeInterval: %s: %s)", title, *detailP));
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      *statusCodeP = SccReceiverInternalError;
      return false;
    }

    if (mongoSetLdTimeInterval(&regP->operationSpace, "operationSpace", bob, &title, detailP) == false)
    {
      LM_E(("Internal Error (mongoSetLdTimeInterval: %s: %s)", title, *detailP));
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      *statusCodeP = SccReceiverInternalError;
      return false;
    }

    if (mongoSetLdProperties(regP, "properties", bob, &title, detailP) == false)
    {
      LM_E(("Internal Error (mongoSetLdProperties: %s: %s)", title, *detailP));
      releaseMongoConnection(connection);
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      *statusCodeP = SccReceiverInternalError;
      return false;
    }


    if (moreSafe(cursor))
    {
      releaseMongoConnection(connection);

      // Ooops, we expected only one
      LM_T(LmtMongo, ("more than one registration: '%s'", regId));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      *detailP    = (char*) "more than one registration matched";
      *statusCodeP = SccConflict;
      return false;
    }
  }
  else
  {
    releaseMongoConnection(connection);
    LM_T(LmtMongo, ("registration not found: '%s'", regId));
    reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
    *detailP     = (char*) "registration not found";
    *statusCodeP = SccContextElementNotFound;
    return false;
  }

  releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);

  *statusCodeP = SccOk;
  return true;
}
