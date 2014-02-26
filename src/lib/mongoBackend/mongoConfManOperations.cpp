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
* fermin at tid dot es
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
void mongoSetFwdRegId(std::string regId, std::string fwdRegId)
{
    reqSemTake(__FUNCTION__, "Mongo Set Forward RegId");

    LM_T(LmtMongo, ("Mongo Set Forward RegId"));

    DBClientConnection* connection = getMongoConnection();

    try {
        BSONObj updateQuery = BSON("$set" << BSON(REG_FWS_REGID << fwdRegId));
        LM_T(LmtMongo, ("update() in '%s' collection doc _id '%s': %s",
                        getRegistrationsCollectionName(),
                        regId.c_str(), updateQuery.toString().c_str()));

        mongoSemTake(__FUNCTION__, "update in RegistrationsCollection");
        connection->update(getRegistrationsCollectionName(), BSON("_id" << OID(regId)), updateQuery);
        mongoSemGive(__FUNCTION__, "update in RegistrationsCollection");
    }
    catch( const DBException &e ) {
        // FIXME: probably we can do something apart of printint the error, but currently
        // we haven't a use case for that
        mongoSemGive(__FUNCTION__, "update in RegistrationsCollection (DBException)");
        LM_E(("Database error '%s'", e.what()));
    }
    catch(...) {
        // FIXME: probably we can do something apart of printint the error, but currently
        // we haven't a use case for that
        mongoSemGive(__FUNCTION__, "update in RegistrationsCollection (Generic Exception)");
        LM_E(("Database error: '%s'", "generic exception"));
    }

    reqSemGive(__FUNCTION__, "Mongo Set Forward RegId");
}

/* ****************************************************************************
*
* mongoGetFwdRegId -
*/
std::string mongoGetFwdRegId(std::string regId)
{
    std::string retVal = "";

    reqSemTake(__FUNCTION__, "Mongo Get Forward RegId");

    LM_T(LmtMongo, ("Mongo Get Forward RegId"));

    DBClientConnection* connection = getMongoConnection();

    try {
        LM_T(LmtMongo, ("findOne() in '%s' collection doc _id '%s'", getRegistrationsCollectionName(), regId.c_str()));
        BSONObj doc;

        mongoSemTake(__FUNCTION__, "findOne in RegistrationsCollection");
        doc = connection->findOne(getRegistrationsCollectionName(), BSON("_id" << OID(regId)));
        mongoSemGive(__FUNCTION__, "findOne in RegistrationsCollection");

        LM_T(LmtMongo, ("reg doc: '%s'", doc.toString().c_str()));
        retVal = STR_FIELD(doc, REG_FWS_REGID);
    }
    catch( const DBException &e ) {
        // FIXME: probably we can do something apart of printing the error, but currently
        // we haven't a use case for that
        mongoSemGive(__FUNCTION__, "findOne in RegistrationsCollection (DBException)");
        LM_E(("Database error '%s'", e.what()));
    }
    catch(...) {
        // FIXME: probably we can do something apart of printing the error, but currently
        // we haven't a use case for that
        mongoSemGive(__FUNCTION__, "findOne in RegistrationsCollection (Generic Exception)");
        LM_E(("Database error: 'generic exception'"));
    }

    reqSemGive(__FUNCTION__, "Mongo Get Forward RegId");

    return retVal;
}

