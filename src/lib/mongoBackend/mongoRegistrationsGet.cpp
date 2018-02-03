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
#include <vector>

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

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
#include "mongoBackend/MongoCommonRegister.h"
#include "mongoBackend/mongoRegistrationsGet.h"



/* ****************************************************************************
*
* mongoRegistrationsGet - 
*/
void mongoRegistrationsGet
(
  std::vector<ngsiv2::Registration>*  regV,
  const std::string&                  tenant,
  const std::vector<std::string>&     servicePathV,
  OrionError*                         oeP
)
{
  bool         reqSemTaken = false;
  std::string  err;
  mongo::OID   oid;
  StatusCode   sc;
  int          limit  = 5000;  // FIXME: Pagination: limit, offset and count are a preparation for it - see mongoGetSubscriptions()
  int          offset = 0;
  long long    count;

  reqSemTake(__FUNCTION__, "Mongo Get Registrations", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Registrations"));

  std::auto_ptr<mongo::DBClientCursor>  cursor;
  mongo::Query                          q;

  if ((servicePathV.size() != 0) && (servicePathV[0] != "/#"))
  {
    q = BSON(REG_SERVICE_PATH << fillQueryServicePath(servicePathV));
  }
  
  q.sort(BSON("_id" << 1));

  TIME_STAT_MONGO_READ_WAIT_START();
  mongo::DBClientBase* connection = getMongoConnection();
  if (!collectionRangedQuery(connection, getRegistrationsCollectionName(tenant), q, limit, offset, &cursor, &count, &err))
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
    mongo::BSONObj        r;
    ngsiv2::Registration  reg;

    if (!nextSafeOrErrorF(cursor, &r, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), q.toString().c_str()));
      continue;
    }

    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, r.toString().c_str()));
    ++docs;

    //
    // Fill in the Registration with data retrieved from the data base
    //
    mongoRegistrationIdExtract(&reg, r);
    mongoDescriptionExtract(&reg, r, REG_DESCRIPTION);

    if (mongoDataProvidedExtract(&reg, r, false, REG_CONTEXT_REGISTRATION) == false)
    {
      releaseMongoConnection(connection);
      LM_W(("Bad Input (getting registrations with more than one CR is not yet implemented, see issue 3044)"));
      reqSemGive(__FUNCTION__, "Mongo Get Registrations", reqSemTaken);
      oeP->fill(SccReceiverInternalError, err);
      return;
    }

    mongoExpiresExtract(&reg, r, REG_EXPIRATION);
    mongoStatusExtract(&reg, r, REG_STATUS);
    mongoForwardingInformationExtract(&reg, r, REG_FORWARDING_INFORMATION);

    // FIXME PR: What about the Service Path of the Registration ... ?
    regV->push_back(reg);
  }

  releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo Get Registrations", reqSemTaken);

  oeP->fill(SccOk, "");
}
