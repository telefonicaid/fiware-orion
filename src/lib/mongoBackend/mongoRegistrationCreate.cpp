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
#include "mongoBackend/mongoRegistrationCreate.h"

#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/safeMongo.h"
#include "mongoDriver/BSONObjBuilder.h"
#include "mongoDriver/BSONArrayBuilder.h"



/* ****************************************************************************
*
* setRegistrationId - 
*/
static void setRegistrationId(orion::BSONObjBuilder* bobP, std::string* regIdP)
{
  orion::OID oId;

  oId.init();

  *regIdP = oId.toString();
  bobP->append("_id", oId);
}



/* ****************************************************************************
*
* setDescription -
*/
static void setDescription(const std::string& description, orion::BSONObjBuilder* bobP)
{
  if (!description.empty())
  {
    bobP->append(REG_DESCRIPTION, description);
  }
}



/* ****************************************************************************
*
* setExpiration -
*/
static void setExpiration(long long expires, orion::BSONObjBuilder* bobP)
{
  if (expires != -1)
  {
    bobP->append(REG_EXPIRATION, expires);
  }
}



/* ****************************************************************************
*
* setServicePath -
*/
static void setServicePath(const std::string& servicePath, orion::BSONObjBuilder* bobP)
{
  if (!servicePath.empty())
  {
    bobP->append(REG_SERVICE_PATH, servicePath);
  }
}



/* ****************************************************************************
*
* setContextRegistrationVector - 
*/
static void setContextRegistrationVector(ngsiv2::Registration* regP, orion::BSONObjBuilder* bobP)
{
  orion::BSONArrayBuilder  contextRegistration;
  orion::BSONArrayBuilder  entities;
  orion::BSONArrayBuilder  attrs;

  for (unsigned int eIx = 0; eIx < regP->dataProvided.entities.size(); ++eIx)
  {
    ngsiv2::EntID* eP = &regP->dataProvided.entities[eIx];

    orion::BSONObjBuilder bob;
    if (eP->idPattern.empty())
    {
      bob.append(REG_ENTITY_ID, eP->id);
      if (!eP->type.empty())
      {
        bob.append(REG_ENTITY_TYPE, eP->type);
      }
    }
    else
    {
      bob.append(REG_ENTITY_ID, eP->idPattern);
      bob.append(REG_ENTITY_ISPATTERN, "true");
      if (!eP->type.empty())
      {
        bob.append(REG_ENTITY_TYPE, eP->type);
      }
    }
    entities.append(bob.obj());
  }

  for (unsigned int aIx = 0; aIx < regP->dataProvided.attributes.size(); ++aIx)
  {
    orion::BSONObjBuilder bob;
    bob.append(REG_ATTRS_NAME, regP->dataProvided.attributes[aIx]);
    bob.append(REG_ATTRS_TYPE, "");
    attrs.append(bob.obj());
  }

  // FIXME OLD-DR: previously this part was based in streamming construction instead of append()
  // should be changed?
  orion::BSONObjBuilder bob;
  bob.append(REG_ENTITIES, entities.arr());
  bob.append(REG_ATTRS, attrs.arr());
  bob.append(REG_PROVIDING_APPLICATION, regP->provider.http.url);

  contextRegistration.append(bob.obj());

  bobP->append(REG_CONTEXT_REGISTRATION, contextRegistration.arr());
}



/* ****************************************************************************
*
* setStatus -
*/
static void setStatus(const std::string& status, orion::BSONObjBuilder* bobP)
{
  if (!status.empty())
  {
    bobP->append(REG_STATUS, status);
  }
}



/* ****************************************************************************
*
* setFormat -
*/
static void setFormat(const std::string& format, orion::BSONObjBuilder* bobP)
{
  if (!format.empty())
  {
    bobP->append(REG_FORMAT, format);
  }
}



/* ****************************************************************************
*
* mongoRegistrationCreate - 
*/
void mongoRegistrationCreate
(
  ngsiv2::Registration*  regP,
  const std::string&     tenant,
  const std::string&     servicePath,
  std::string*           regIdP,
  OrionError*            oeP
)
{
  bool         reqSemTaken = false;

  reqSemTake(__FUNCTION__, "Mongo Create Registration", SemReadOp, &reqSemTaken);

  //
  // Build the BSON object to insert
  //
  orion::BSONObjBuilder  bob;

  setRegistrationId(&bob, regIdP);
  setDescription(regP->description, &bob);
  setExpiration(regP->expires, &bob);
  setServicePath(servicePath, &bob);
  setContextRegistrationVector(regP, &bob);
  setStatus(regP->status, &bob);

  std::string format = (regP->provider.legacyForwardingMode == true)? "JSON" : "normalized";
  setFormat(format, &bob);

  //
  // Insert in DB
  //
  orion::BSONObj  doc = bob.obj();
  std::string     err;

  if (!orion::collectionInsert(getRegistrationsCollectionName(tenant), doc, &err))
  {
    reqSemGive(__FUNCTION__, "Mongo Create Registration", reqSemTaken);
    oeP->fill(SccReceiverInternalError, err);
    return;
  }

  reqSemGive(__FUNCTION__, "Mongo Create Registration", reqSemTaken);

  oeP->fill(SccOk, "");
}
