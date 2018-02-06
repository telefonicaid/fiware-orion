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
#include "mongoBackend/mongoRegistrationGet.h"



/* ****************************************************************************
*
* setRegistrationId -
*/
static void setRegistrationId(ngsiv2::Registration* regP, const mongo::BSONObj& r)
{
  regP->id = getFieldF(r, "_id").OID().toString();
}



/* ****************************************************************************
*
* setDescription -
*/
static void setDescription(ngsiv2::Registration* regP, const mongo::BSONObj& r, const char* dbLabel)
{
  if (r.hasField(dbLabel))
  {
    regP->description         = getStringFieldF(r, dbLabel);
    regP->descriptionProvided = true;
  }
  else
  {
    regP->description         = "";
    regP->descriptionProvided = false;
  }
}



/* ****************************************************************************
*
* setProvider -
*/
static void setProvider(ngsiv2::Registration* regP, const mongo::BSONObj& r, const char* dbLabel)
{
  regP->provider.http.url = (r.hasField(dbLabel))? getStringFieldF(r, dbLabel): "";

  //
  // FIXME P4: for the moment supportedForwardingMode and legacyForwardingMode are hardwired (i.e. DB is not taken
  // into account for them)
  //
  regP->provider.supportedForwardingMode = ngsiv2::ForwardAll;
  regP->provider.legacyForwardingMode = true;
}



/* ****************************************************************************
*
* setEntities -
*/
static void setEntities(ngsiv2::Registration* regP, const mongo::BSONObj& cr0, const char* dbLabel)
{
  std::vector<mongo::BSONElement>  dbEntityV = getFieldF(cr0, dbLabel).Array();

  for (unsigned int ix = 0; ix < dbEntityV.size(); ++ix)
  {
    ngsiv2::EntID    entity;
    mongo::BSONObj   ce = dbEntityV[ix].embeddedObject();

    if (ce.hasField(REG_ENTITY_ISPATTERN))
    {
      std::string isPattern = getStringFieldF(ce, REG_ENTITY_ISPATTERN);

      if (isPattern == "true")
      {
        entity.idPattern = getStringFieldF(ce, REG_ENTITY_ID);
      }
      else
      {
        entity.id = getStringFieldF(ce, REG_ENTITY_ID);
      }
    }
    else
    {
      entity.id = getStringFieldF(ce, REG_ENTITY_ID);
    }

    if (ce.hasField(REG_ENTITY_ISTYPEPATTERN))
    {
      std::string isPattern = getStringFieldF(ce, REG_ENTITY_ISTYPEPATTERN);

      if (isPattern == "true")
      {
        entity.typePattern = getStringFieldF(ce, REG_ENTITY_TYPE);
      }
      else
      {
        entity.type = getStringFieldF(ce, REG_ENTITY_TYPE);
      }
    }
    else
    {
      entity.type = getStringFieldF(ce, REG_ENTITY_TYPE);
    }

    regP->dataProvided.entities.push_back(entity);
  }
}



/* ****************************************************************************
*
* setAttributes -
*/
static void setAttributes(ngsiv2::Registration* regP, const mongo::BSONObj& cr0, const char* dbLabel)
{
  std::vector<mongo::BSONElement> dbAttributeV = getFieldF(cr0, dbLabel).Array();

  for (unsigned int ix = 0; ix < dbAttributeV.size(); ++ix)
  {
    mongo::BSONObj  aobj     = dbAttributeV[ix].embeddedObject();
    std::string     attrName = getStringFieldF(aobj, REG_ATTRS_NAME);

    if (attrName != "")
    {
      regP->dataProvided.attributes.push_back(attrName);
    }
  }
}


/* ****************************************************************************
*
* setDataProvided -
*
* Make sure there is only ONE "contextRegistration" in the vector
* If we have more than one, then the Registration is made in API V1 as this is not
* possible in V2 and we cannot respond to the request using the current implementation of V2.
* This function will be changed to work in a different way once issue #3044 is dealt with.
*/
static bool setDataProvided(ngsiv2::Registration* regP, const mongo::BSONObj& r, bool arrayAllowed, const char* dbLabel)
{
  std::vector<mongo::BSONElement> crV = getFieldF(r, dbLabel).Array();

  if (crV.size() > 1)
  {
    return false;
  }

  //
  // Extract the first (and only) CR from the contextRegistration vector
  //
  mongo::BSONObj cr0 = crV[0].embeddedObject();

  setEntities(regP, cr0, REG_ENTITIES);
  setAttributes(regP, cr0, REG_ATTRS);
  setProvider(regP, cr0, REG_PROVIDING_APPLICATION);

  return true;
}



/* ****************************************************************************
*
* setExpires -
*/
static void setExpires(ngsiv2::Registration* regP, const mongo::BSONObj& r, const char* dbLabel)
{
  regP->expires = (r.hasField(dbLabel))? getIntFieldF(r, dbLabel) : -1;
}



/* ****************************************************************************
*
* setStatus -
*/
static void setStatus(ngsiv2::Registration* regP, const mongo::BSONObj& r, const char* dbLabel)
{
  regP->status = (r.hasField(dbLabel))? getStringFieldF(r, dbLabel): "";
}



/* ****************************************************************************
*
* setForwardingInformation -
*/
static void setForwardingInformation(ngsiv2::Registration* regP, const mongo::BSONObj& r, const char* dbLabel)
{
  // No forwarding info until API V2 forwarding is implemented
}



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
  mongo::OID   oid;
  StatusCode   sc;

  if (safeGetRegId(regId, &oid, &sc) == false)
  {
    oeP->fill(sc);
    return;
  }

  reqSemTake(__FUNCTION__, "Mongo Get Registration", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Registration"));

  std::auto_ptr<mongo::DBClientCursor>  cursor;
  mongo::BSONObj                        q;

  // FIXME P0: what about the service path ... ?   See issue #3051
  q = BSON("_id" << oid);

  TIME_STAT_MONGO_READ_WAIT_START();
  mongo::DBClientBase* connection = getMongoConnection();
  if (!collectionQuery(connection, getRegistrationsCollectionName(tenant), q, &cursor, &err))
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
    mongo::BSONObj r;
    if (!nextSafeOrErrorF(cursor, &r, &err))
    {
      releaseMongoConnection(connection);
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), q.toString().c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      oeP->fill(SccReceiverInternalError, std::string("exception in nextSafe(): ") + err.c_str());
      return;
    }
    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

    //
    // Fill in the Registration with data retrieved from the data base
    //
    setRegistrationId(regP, r);
    setDescription(regP, r, REG_DESCRIPTION);

    if (setDataProvided(regP, r, false, REG_CONTEXT_REGISTRATION) == false)
    {
      releaseMongoConnection(connection);
      LM_W(("Bad Input (getting registrations with more than one CR is not yet implemented, see issue 3044)"));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      oeP->fill(SccReceiverInternalError, err);
      return;
    }

    setExpires(regP, r, REG_EXPIRATION);
    setStatus(regP, r, REG_STATUS);
    setForwardingInformation(regP, r, REG_FORWARDING_INFORMATION);
    
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
    setRegistrationId(&reg, r);
    setDescription(&reg, r, REG_DESCRIPTION);

    if (setDataProvided(&reg, r, false, REG_CONTEXT_REGISTRATION) == false)
    {
      releaseMongoConnection(connection);
      LM_W(("Bad Input (getting registrations with more than one CR is not yet implemented, see issue 3044)"));
      reqSemGive(__FUNCTION__, "Mongo Get Registrations", reqSemTaken);
      oeP->fill(SccReceiverInternalError, err);
      return;
    }

    setExpires(&reg, r, REG_EXPIRATION);
    setStatus(&reg, r, REG_STATUS);
    setForwardingInformation(&reg, r, REG_FORWARDING_INFORMATION);

    // FIXME PR: What about the Service Path of the Registration ... ?
    regV->push_back(reg);
  }

  releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo Get Registrations", reqSemTaken);

  oeP->fill(SccOk, "");
}
