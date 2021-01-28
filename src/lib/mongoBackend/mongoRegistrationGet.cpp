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
* setRegistrationId -
*/
static void setRegistrationId(ngsiv2::Registration* regP, const orion::BSONObj& r)
{
  regP->id = getFieldFF(r, "_id").OID();
}



/* ****************************************************************************
*
* setDescription -
*/
static void setDescription(ngsiv2::Registration* regP, const orion::BSONObj& r)
{
  if (r.hasField(REG_DESCRIPTION))
  {
    regP->description         = getStringFieldFF(r, REG_DESCRIPTION);
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
static void setProvider(ngsiv2::Registration* regP, const ngsiv2::ForwardingMode forwardingMode, const orion::BSONObj& r)
{
  regP->provider.http.url = (r.hasField(REG_PROVIDING_APPLICATION))? getStringFieldFF(r, REG_PROVIDING_APPLICATION): "";

  regP->provider.supportedForwardingMode = forwardingMode;

  std::string format = r.hasField(REG_FORMAT)? getStringFieldFF(r, REG_FORMAT) : "JSON";
  if (format == "JSON")
  {
    regP->provider.legacyForwardingMode = true;
  }
  else
  {
    regP->provider.legacyForwardingMode = false;
  }
}



/* ****************************************************************************
*
* setEntities -
*/
static void setEntities(ngsiv2::Registration* regP, const orion::BSONObj& cr0)
{
  std::vector<orion::BSONElement>  dbEntityV = getFieldFF(cr0, REG_ENTITIES).Array();

  for (unsigned int ix = 0; ix < dbEntityV.size(); ++ix)
  {
    ngsiv2::EntID    entity;
    orion::BSONObj   ce = dbEntityV[ix].embeddedObject();

    if (ce.hasField(REG_ENTITY_ISPATTERN))
    {
      std::string isPattern = getStringFieldFF(ce, REG_ENTITY_ISPATTERN);

      if (isPattern == "true")
      {
        entity.idPattern = getStringFieldFF(ce, REG_ENTITY_ID);
      }
      else
      {
        entity.id = getStringFieldFF(ce, REG_ENTITY_ID);
      }
    }
    else
    {
      entity.id = getStringFieldFF(ce, REG_ENTITY_ID);
    }

    if (ce.hasField(REG_ENTITY_ISTYPEPATTERN))
    {
      std::string isPattern = getStringFieldFF(ce, REG_ENTITY_ISTYPEPATTERN);

      if (isPattern == "true")
      {
        entity.typePattern = getStringFieldFF(ce, REG_ENTITY_TYPE);
      }
      else
      {
        entity.type = getStringFieldFF(ce, REG_ENTITY_TYPE);
      }
    }
    else
    {
      entity.type = getStringFieldFF(ce, REG_ENTITY_TYPE);
    }

    regP->dataProvided.entities.push_back(entity);
  }
}



/* ****************************************************************************
*
* setAttributes -
*/
static void setAttributes(ngsiv2::Registration* regP, const orion::BSONObj& cr0)
{
  std::vector<orion::BSONElement> dbAttributeV = getFieldFF(cr0, REG_ATTRS).Array();

  for (unsigned int ix = 0; ix < dbAttributeV.size(); ++ix)
  {
    orion::BSONObj  aobj     = dbAttributeV[ix].embeddedObject();
    std::string     attrName = getStringFieldFF(aobj, REG_ATTRS_NAME);

    if (!attrName.empty())
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
static bool setDataProvided(ngsiv2::Registration* regP, const orion::BSONObj& r, bool arrayAllowed)
{
  std::vector<orion::BSONElement> crV = getFieldFF(r, REG_CONTEXT_REGISTRATION).Array();

  if (crV.size() > 1)
  {
    return false;
  }

  // Get the forwarding mode to be used later in setProvider()
  ngsiv2::ForwardingMode forwardingMode =
      ngsiv2::stringToForwardingMode(r.hasField(REG_FORWARDING_MODE)? getStringField(r, REG_FORWARDING_MODE) : "all");

  //
  // Extract the first (and only) CR from the contextRegistration vector
  //
  orion::BSONObj cr0 = crV[0].embeddedObject();

  setEntities(regP, cr0);
  setAttributes(regP, cr0);
  setProvider(regP, forwardingMode, cr0);

  return true;
}



/* ****************************************************************************
*
* setExpires -
*/
static void setExpires(ngsiv2::Registration* regP, const orion::BSONObj& r)
{
  regP->expires = (r.hasField(REG_EXPIRATION))? getIntOrLongFieldAsLongFF(r, REG_EXPIRATION) : -1;
}



/* ****************************************************************************
*
* setStatus -
*/
static void setStatus(ngsiv2::Registration* regP, const orion::BSONObj& r)
{
  regP->status = (r.hasField(REG_STATUS))? getStringFieldFF(r, REG_STATUS): "";
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
  orion::OID   oid;
  StatusCode   sc;

  if (safeGetRegId(regId, &oid, &sc) == false)
  {
    oeP->fill(sc);
    return;
  }

  reqSemTake(__FUNCTION__, "Mongo Get Registration", SemReadOp, &reqSemTaken);

  LM_T(LmtMongo, ("Mongo Get Registration"));

  orion::DBCursor  cursor;
  orion::BSONObj   q;

  orion::BSONObjBuilder bob;
  bob.append("_id", oid);
  q = bob.obj();

  TIME_STAT_MONGO_READ_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();
  if (!orion::collectionQuery(connection, getRegistrationsCollectionName(tenant), q, &cursor, &err))
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
    /* FIXME OLD-DR: remove?
    if (!nextSafeOrErrorFF(cursor, &r, &err))
    {
      orion::releaseMongoConnection(connection);
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), q.toString().c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      oeP->fill(SccReceiverInternalError, std::string("exception in nextSafe(): ") + err.c_str());
      return;
    }*/
    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

    //
    // Fill in the Registration with data retrieved from the data base
    //
    setRegistrationId(regP, r);
    setDescription(regP, r);

    if (setDataProvided(regP, r, false) == false)
    {
      orion::releaseMongoConnection(connection);
      LM_W(("Bad Input (getting registrations with more than one CR is not yet implemented, see issue 3044)"));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      oeP->fill(SccReceiverInternalError, err);
      return;
    }

    setExpires(regP, r);
    setStatus(regP, r);

    /* FIXME OLD-DR: excesive checking. Looking by _id you cannot get more than one result!
    if (orion::moreSafe(&cursor))  // Can only be one ...
    {
      orion::releaseMongoConnection(connection);
      LM_T(LmtMongo, ("more than one registration: '%s'", regId.c_str()));
      reqSemGive(__FUNCTION__, "Mongo Get Registration", reqSemTaken);
      oeP->fill(SccConflict, "");
      return;
    }*/
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
  StatusCode   sc;
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
  if (!orion::collectionRangedQuery(connection, getRegistrationsCollectionName(tenant), q, bSort.obj(), limit, offset, &cursor, countP, &err))
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    reqSemGive(__FUNCTION__, "Mongo Get Registrations", reqSemTaken);
    oeP->fill(SccReceiverInternalError, err);
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  int docs = 0;
  orion::BSONObj        r;
  while (cursor.next(&r))
  {
    ngsiv2::Registration  reg;

    /* FIXME OLD-DR: remove?
    if (!nextSafeOrErrorFF(cursor, &r, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), q.toString().c_str()));
      continue;
    }*/

    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, r.toString().c_str()));
    ++docs;

    //
    // Fill in the Registration with data retrieved from the data base
    //
    setRegistrationId(&reg, r);
    setDescription(&reg, r);

    if (setDataProvided(&reg, r, false) == false)
    {
      orion::releaseMongoConnection(connection);
      LM_W(("Bad Input (getting registrations with more than one CR is not yet implemented, see issue 3044)"));
      reqSemGive(__FUNCTION__, "Mongo Get Registrations", reqSemTaken);
      oeP->fill(SccReceiverInternalError, err);
      return;
    }

    setExpires(&reg, r);
    setStatus(&reg, r);

    regV->push_back(reg);
  }

  orion::releaseMongoConnection(connection);
  reqSemGive(__FUNCTION__, "Mongo Get Registrations", reqSemTaken);

  oeP->fill(SccOk, "");
}
