/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan
*/
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongo/client/dbclient.h"

using namespace mongo;

/* ****************************************************************************
*
* mongoSetFwdRegId -
*/
void mongoSetFwdRegId(const std::string& regId, const std::string& fwdRegId, const std::string& tenant)
{
  DBClientBase*  connection  = NULL;
  bool           reqSemTaken = false;

  reqSemTake(__FUNCTION__, "Mongo Set Forward RegId", SemWriteOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Set Forward RegId"));

  try
  {
    BSONObj updateQuery = BSON("$set" << BSON(REG_FWS_REGID << fwdRegId));
    LM_T(LmtMongo, ("update() in '%s' collection doc _id '%s': %s",
                    getRegistrationsCollectionName(tenant).c_str(),
                    regId.c_str(), updateQuery.toString().c_str()));

    connection = getMongoConnection();
    connection->update(getRegistrationsCollectionName(tenant).c_str(), BSON("_id" << OID(regId)), updateQuery);
    releaseMongoConnection(connection);

    LM_I(("Database Operation Successful (update _id: %s, query: %s)",
          regId.c_str(),
          updateQuery.toString().c_str()));
  }
  catch (const DBException &e)
  {
    //
    // FIXME: probably we can do something apart of printing the error, but currently
    // we haven't a use case for that
    //
    releaseMongoConnection(connection);

    LM_E(("Database Error ('update tenant=%s, id=%s', '%s')", tenant.c_str(), regId.c_str(), e.what()));
  }
  catch (...)
  {
    //
    // FIXME: probably we can do something apart of printing the error, but currently
    // we haven't a use case for that
    //
    releaseMongoConnection(connection);

    LM_E(("Database Error ('update tenant=%s, id=%s', '%s')", tenant.c_str(), regId.c_str(), "generic exception"));
  }

  reqSemGive(__FUNCTION__, "Mongo Set Forward RegId", reqSemTaken);
}


/* ****************************************************************************
*
* mongoGetFwdRegId -
*/
std::string mongoGetFwdRegId(const std::string& regId, const std::string& tenant)
{
  DBClientBase*  connection  = NULL;
  std::string    retVal      = "";
  bool           reqSemTaken = false;

  reqSemTake(__FUNCTION__, "Mongo Get Forward RegId", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Forward RegId"));

  try
  {
    LM_T(LmtMongo, ("findOne() in '%s' collection doc _id '%s'",
                    getRegistrationsCollectionName(tenant).c_str(),
                    regId.c_str()));
    BSONObj doc;

    connection = getMongoConnection();
    doc = connection->findOne(getRegistrationsCollectionName(tenant).c_str(), BSON("_id" << OID(regId)));
    releaseMongoConnection(connection);

    LM_T(LmtMongo, ("reg doc: '%s'", doc.toString().c_str()));
    retVal = STR_FIELD(doc, REG_FWS_REGID);
    LM_I(("Database Operation Successful (findOne _id: %s)", regId.c_str()));
  }
  catch (const DBException &e)
  {
    //
    // FIXME: probably we can do something apart from printing the error, but currently
    // we haven't a use case for that
    //
    releaseMongoConnection(connection);

    LM_E(("Database Error ('findOne tenant=%s, id=%s', '%s')", tenant.c_str(), regId.c_str(), e.what()));
  }
  catch (...)
  {
    //
    // FIXME: probably we can do something apart from printing the error, but currently
    // we haven't a use case for that
    //
    releaseMongoConnection(connection);

    LM_E(("Database Error ('findOne tenant=%s, id=%s', 'generic exception')", tenant.c_str(), regId.c_str()));
  }

  reqSemGive(__FUNCTION__, "Mongo Get Forward RegId", reqSemTaken);

  return retVal;
}
