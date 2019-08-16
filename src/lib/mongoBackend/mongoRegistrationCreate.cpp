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
#include "orionld/common/orionldState.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/mongoRegistrationCreate.h"



/* ****************************************************************************
*
* setRegistrationId - 
*/
#ifdef ORIONLD
static void setNgsildRegistrationId(mongo::BSONObjBuilder* bobP, const char* regId)
{
  bobP->append("_id", regId);
}
#endif
static void setRegistrationId(mongo::BSONObjBuilder* bobP, std::string* regIdP)
{
  mongo::OID oId;

  oId.init();

  *regIdP = oId.toString();
  bobP->append("_id", oId);
}



/* ****************************************************************************
*
* setDescription -
*/
static void setDescription(const std::string& description, mongo::BSONObjBuilder* bobP)
{
  if (description != "")
  {
    bobP->append(REG_DESCRIPTION, description);
  }
}



/* ****************************************************************************
*
* setExpiration -
*/
static void setExpiration(long long expires, mongo::BSONObjBuilder* bobP)
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
static void setServicePath(const std::string& servicePath, mongo::BSONObjBuilder* bobP)
{
  if (servicePath != "")
  {
    bobP->append(REG_SERVICE_PATH, servicePath);
  }
}



/* ****************************************************************************
*
* setContextRegistrationVector - 
*/
static void setContextRegistrationVector(ngsiv2::Registration* regP, mongo::BSONObjBuilder* bobP)
{
  mongo::BSONArrayBuilder  contextRegistration;
  mongo::BSONArrayBuilder  entities;
  mongo::BSONArrayBuilder  attrs;

  for (unsigned int eIx = 0; eIx < regP->dataProvided.entities.size(); ++eIx)
  {
    ngsiv2::EntID* eP = &regP->dataProvided.entities[eIx];

    if (orionldState.apiVersion == NGSI_LD_V1)
    {
      if (eP->id != "")
      {
        LM_TMP(("KZ: reg entity id == '%s'", eP->id.c_str()));
        if (eP->type == "")
          entities.append(BSON(REG_ENTITY_ID << eP->id));
        else
          entities.append(BSON(REG_ENTITY_ID << eP->id << REG_ENTITY_TYPE << eP->type));
      }
      else if (eP->idPattern != "")
      {
        LM_TMP(("KZ: reg entity idPattern == '%s'", eP->idPattern.c_str()));
        if (eP->type == "")
          entities.append(BSON(REG_ENTITY_ID << eP->idPattern << REG_ENTITY_ISPATTERN << "true"));
        else
          entities.append(BSON(REG_ENTITY_ID << eP->idPattern << REG_ENTITY_ISPATTERN << "true" << REG_ENTITY_TYPE << eP->type));
      }
      else
      {
        // Error
      }
    }
    else
    {
      if (eP->type == "")  // No type provided => all types
      {
        entities.append(BSON(REG_ENTITY_ID << eP->id));
      }
      else
      {
        entities.append(BSON(REG_ENTITY_ID << eP->id << REG_ENTITY_TYPE << eP->type));
      }
    }
  }

  if (orionldState.apiVersion == NGSI_LD_V1)
  {
    for (unsigned int pIx = 0; pIx < regP->dataProvided.propertyV.size(); ++pIx)
      attrs.append(BSON(REG_ATTRS_NAME << regP->dataProvided.propertyV[pIx] << REG_ATTRS_TYPE << "Property" << REG_ATTRS_ISDOMAIN << "false"));
    for (unsigned int rIx = 0; rIx < regP->dataProvided.relationshipV.size(); ++rIx)
      attrs.append(BSON(REG_ATTRS_NAME << regP->dataProvided.relationshipV[rIx] << REG_ATTRS_TYPE << "Relationship" << REG_ATTRS_ISDOMAIN << "false"));
  }
  else
  {
    for (unsigned int aIx = 0; aIx < regP->dataProvided.attributes.size(); ++aIx)
    {
      attrs.append(BSON(REG_ATTRS_NAME << regP->dataProvided.attributes[aIx] << REG_ATTRS_TYPE << "" << REG_ATTRS_ISDOMAIN << "false"));
    }
  }

  contextRegistration.append(
    BSON(REG_ENTITIES              << entities.arr() <<
         REG_ATTRS                 << attrs.arr()    <<
         REG_PROVIDING_APPLICATION << regP->provider.http.url));

  bobP->append(REG_CONTEXT_REGISTRATION, contextRegistration.arr());
}



#ifdef ORIONLD
/* ****************************************************************************
*
* setObservationInterval
*/
static void setObservationInterval(const OrionldTimeInterval& interval, mongo::BSONObjBuilder* bobP)
{
  mongo::BSONObjBuilder intervalObj;

  intervalObj.append("start", (long long) interval.start);
  intervalObj.append("end",   (long long) interval.end);

  bobP->append("observationInterval", intervalObj.obj());
}



/* ****************************************************************************
*
* setManagementInterval
*/
static void setManagementInterval(const OrionldTimeInterval& interval, mongo::BSONObjBuilder* bobP)
{
  mongo::BSONObjBuilder intervalObj;

  intervalObj.append("start", (long long) interval.start);
  intervalObj.append("end",   (long long) interval.end);

  bobP->append("managementInterval", intervalObj.obj());
}
#endif



/* ****************************************************************************
*
* setStatus -
*/
static void setStatus(const std::string& status, mongo::BSONObjBuilder* bobP)
{
  if (status != "")
  {
    bobP->append(REG_STATUS, status);
  }
}



/* ****************************************************************************
*
* setFormat -
*/
static void setFormat(const std::string& format, mongo::BSONObjBuilder* bobP)
{
  if (format != "")
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
  mongo::BSONObjBuilder  bob;

  if (orionldState.apiVersion == NGSI_LD_V1)
    setNgsildRegistrationId(&bob, regP->id.c_str());
  else
    setRegistrationId(&bob, regIdP);

  setDescription(regP->description, &bob);
  setExpiration(regP->expires, &bob);
  setServicePath(servicePath, &bob);
  setContextRegistrationVector(regP, &bob);
  setStatus(regP->status, &bob);
  setFormat("JSON", &bob);   // FIXME #3068: this would be unhardired when we implement NGSIv2-based forwarding

#ifdef ORIONLD
  if (regP->observationInterval.start != 0)
    setObservationInterval(regP->observationInterval, &bob);
  if (regP->managementInterval.start != 0)
    setManagementInterval(regP->managementInterval, &bob);
#endif

  //
  // Insert in DB
  //
  mongo::BSONObj  doc = bob.obj();
  std::string     err;

  if (!collectionInsert(getRegistrationsCollectionName(tenant), doc, &err))
  {
    reqSemGive(__FUNCTION__, "Mongo Create Registration", reqSemTaken);
    oeP->fill(SccReceiverInternalError, err);

    return;
  }

  reqSemGive(__FUNCTION__, "Mongo Create Registration", reqSemTaken);

  oeP->fill(SccOk, "");
}
