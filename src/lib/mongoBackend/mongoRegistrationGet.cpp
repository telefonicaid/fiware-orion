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
* Author: Ken Zangelin, Larysse Savanna and Gabriel Quaresma
*/
#include <string>
#include <vector>

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "orionld/types/OrionldTenant.h"

#include "rest/ConnectionInfo.h"
#include "common/idCheck.h"
#include "common/sem.h"
#include "common/statistics.h"
#include "common/errorMessages.h"
#include "rest/OrionError.h"
#include "rest/HttpStatusCode.h"
#include "apiTypesV2/Registration.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/mongoRegistrationAux.h"                 // mongoSetXxx
#include "orionld/mongoBackend/mongoLdRegistrationAux.h"       // mongoSetLdXxx
#include "mongoBackend/mongoRegistrationGet.h"                 // Own interface



/* ****************************************************************************
*
* mongoRegistrationGet -
*/
void mongoRegistrationGet
(
  ngsiv2::Registration*  regP,
  const std::string&     regId,
  OrionldTenant*         tenantP,
  const std::string&     servicePath,
  OrionError*            oeP
)
{
  bool         reqSemTaken = false;
  std::string  err;
  mongo::OID   oid;
  StatusCode   sc;

  if (safeGetRegId(regId.c_str(), &oid, &sc) == false)
  {
    oeP->fill(sc);
    return;
  }

  reqSemTake(__FUNCTION__, "Mongo Get Registration", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Registration"));

  std::auto_ptr<mongo::DBClientCursor>  cursor;
  mongo::BSONObj                        q;

  q = BSON("_id" << oid);

  TIME_STAT_MONGO_READ_WAIT_START();
  mongo::DBClientBase* connection = getMongoConnection();
  if (!collectionQuery(connection, tenantP->registrations, q, &cursor, &err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
    oeP->fill(SccReceiverInternalError, err);
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  if (moreSafe(cursor))
  {
    mongo::BSONObj bob;

    if (!nextSafeOrErrorF(cursor, &bob, &err))
    {
      releaseMongoConnection(connection);
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), q.toString().c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      oeP->fill(SccReceiverInternalError, std::string("exception in nextSafe(): ") + err.c_str());
      return;
    }
    LM_T(LmtMongo, ("retrieved document: '%s'", bob.toString().c_str()));

    //
    // Fill in the Registration with data retrieved from the data base
    //
    mongoSetRegistrationId(regP, &bob);
    mongoSetDescription(regP, &bob);

    if (mongoSetDataProvided(regP, &bob, false) == false)
    {
      releaseMongoConnection(connection);
      LM_W(("Bad Input (getting registrations with more than one CR is not yet implemented, see issue 3044)"));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      oeP->fill(SccReceiverInternalError, err);
      return;
    }

#ifdef ORIONLD
    // FIXME: Can this be removed?
    mongoSetLdObservationInterval(regP, bob);
    mongoSetLdManagementInterval(regP, bob);
#endif
    mongoSetExpires(regP, bob);
    mongoSetStatus(regP, &bob);

    if (moreSafe(cursor))  // Can only be one ...
    {
      releaseMongoConnection(connection);
      LM_T(LmtMongo, ("more than one registration: '%s'", regId.c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      oeP->fill(SccConflict, "");
      return;
    }
  }
  else
  {
    releaseMongoConnection(connection);
    LM_T(LmtMongo, ("registration not found: '%s'", regId.c_str()));
    reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
    oeP->fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_REGISTRATION, ERROR_NOT_FOUND);

    return;
  }

  releaseMongoConnection(connection);
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
  OrionldTenant*                      tenantP,
  const std::vector<std::string>&     servicePathV,
  int                                 offset,
  int                                 limit,
  long long*                          countP,
  OrionError*                         oeP
)
{
  bool         reqSemTaken = false;
  std::string  err;
  mongo::OID   oid;
  StatusCode   sc;
  std::string  servicePath = (servicePathV.size() == 0)? "/#" : servicePathV[0];  // FIXME P4: see #3100

  reqSemTake(__FUNCTION__, "Mongo Get Registrations", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Registrations"));

  std::auto_ptr<mongo::DBClientCursor>  cursor;
  mongo::Query                          q;

  // FIXME P6: This here is a bug ... See #3099 for more info
  if (!servicePath.empty() && (servicePath != "/#"))
  {
    q = BSON(REG_SERVICE_PATH << servicePathV[0]);
  }

  q.sort(BSON("_id" << 1));

  TIME_STAT_MONGO_READ_WAIT_START();
  mongo::DBClientBase* connection = getMongoConnection();
  if (!collectionRangedQuery(connection, tenantP->registrations, q, limit, offset, &cursor, countP, &err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo Get Registrations", reqSemTaken);
    oeP->fill(SccReceiverInternalError, err);
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  int docs = 0;
  while (moreSafe(cursor))
  {
    mongo::BSONObj        bob;
    ngsiv2::Registration  reg;

    if (!nextSafeOrErrorF(cursor, &bob, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), q.toString().c_str()));
      continue;
    }

    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, bob.toString().c_str()));
    ++docs;

    //
    // Fill in the Registration with data retrieved from the data base
    //
    mongoSetRegistrationId(&reg, &bob);
    mongoSetDescription(&reg, &bob);

    if (mongoSetDataProvided(&reg, &bob, false) == false)
    {
      releaseMongoConnection(connection);
      LM_W(("Bad Input (getting registrations with more than one CR is not yet implemented, see issue 3044)"));
      reqSemGive(__FUNCTION__, "Mongo Get Registrations", reqSemTaken);
      oeP->fill(SccReceiverInternalError, err);
      return;
    }

    mongoSetExpires(&reg, bob);
    mongoSetStatus(&reg, &bob);

    regV->push_back(reg);
  }

  releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo Get Registrations", reqSemTaken);

  oeP->fill(SccOk, "");
}
