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
* Author: Fermín Galán
*/
#include <stdint.h>
#include <utility>
#include <map>
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/string.h"
#include "common/globals.h"
#include "common/statistics.h"
#include "common/sem.h"
#include "common/RenderFormat.h"
#include "common/defaultValues.h"
#include "alarmMgr/alarmMgr.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/TriggeredSubscription.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/mongoConnectionPool.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/MongoCommonRegister.h"



/* ****************************************************************************
*
* USING
*/
using mongo::BSONArrayBuilder;
using mongo::BSONObjBuilder;
using mongo::BSONObj;
using mongo::BSONElement;
using mongo::DBClientBase;
using mongo::DBClientCursor;
using mongo::OID;



/* ****************************************************************************
*
* processRegisterContext -
*
* This function has a slightly different behaviour depending on whether the id
* parameter is null (new registration case) or not null (update case), in
* particular:
*
* - In the new registration case, the _id is generated and insert() is used to
*   put the document in the DB.
* - In the update case, the _id is set according to the argument 'id' and update() is
*   used to put the document in the DB.
*/
HttpStatusCode processRegisterContext
(
  RegisterContextRequest*   requestP,
  RegisterContextResponse*  responseP,
  OID*                      id,
  const std::string&        tenant,
  const std::string&        servicePath,
  const std::string&        format,
  const std::string&        fiwareCorrelator
)
{
  std::string err;

  /* If expiration is not present, then use a default one */
  if (requestP->duration.isEmpty())
  {
    requestP->duration.set(DEFAULT_DURATION);
  }

  /* Calculate expiration (using the current time and the duration field in the request) */
  long long expiration = getCurrentTime() + requestP->duration.parse();

  LM_T(LmtMongo, ("Registration expiration: %lu", expiration));

  /* Create the mongoDB registration document */
  BSONObjBuilder reg;
  OID oid;

  if (id == NULL)
  {
    oid.init();
  }
  else
  {
    oid = *id;
  }

  reg.append("_id", oid);
  reg.append(REG_EXPIRATION, expiration);

  // FIXME P4: See issue #3078
  reg.append(REG_SERVICE_PATH, servicePath.empty() ? SERVICE_PATH_ROOT : servicePath);
  reg.append(REG_FORMAT, format);

  BSONArrayBuilder  contextRegistration;

  for (unsigned int ix = 0; ix < requestP->contextRegistrationVector.size(); ++ix)
  {
    ContextRegistration*  cr = requestP->contextRegistrationVector[ix];
    BSONArrayBuilder      entities;

    for (unsigned int jx = 0; jx < cr->entityIdVector.size(); ++jx)
    {
      EntityId* en = cr->entityIdVector[jx];


      if (en->type.empty())
      {
        entities.append(BSON(REG_ENTITY_ID << en->id));
        LM_T(LmtMongo, ("Entity registration: {id: %s}", en->id.c_str()));
      }
      else
      {
        entities.append(BSON(REG_ENTITY_ID << en->id << REG_ENTITY_TYPE << en->type));
        LM_T(LmtMongo, ("Entity registration: {id: %s, type: %s}", en->id.c_str(), en->type.c_str()));
      }
    }

    BSONArrayBuilder attrs;

    for (unsigned int jx = 0; jx < cr->contextRegistrationAttributeVector.size(); ++jx)
    {
      ContextRegistrationAttribute* cra = cr->contextRegistrationAttributeVector[jx];

      attrs.append(BSON(REG_ATTRS_NAME << cra->name << REG_ATTRS_TYPE << cra->type));
      LM_T(LmtMongo, ("Attribute registration: {name: %s, type: %s}",
                      cra->name.c_str(),
                      cra->type.c_str()));
    }

    contextRegistration.append(
      BSON(
        REG_ENTITIES << entities.arr() <<
        REG_ATTRS << attrs.arr() <<
        REG_PROVIDING_APPLICATION << requestP->contextRegistrationVector[ix]->providingApplication.get()));

    LM_T(LmtMongo, ("providingApplication registration: %s",
                    requestP->contextRegistrationVector[ix]->providingApplication.c_str()));
  }
  reg.append(REG_CONTEXT_REGISTRATION, contextRegistration.arr());

  /* Note that we are using upsert = "true". This means that if the document doesn't previously
   * exist in the collection, it is created. Thus, this way both uses of registerContext are OK
   * (either new registration or updating an existing one)
   */
  if (!collectionUpdate(getRegistrationsCollectionName(tenant), BSON("_id" << oid), reg.obj(), true, &err))
  {
    responseP->errorCode.fill(SccReceiverInternalError, err);
    return SccOk;
  }

  // Fill the response element
  responseP->duration = requestP->duration;
  responseP->registrationId.set(oid.toString());
  responseP->errorCode.fill(SccOk);

  return SccOk;
}
