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
* processSubscriptions -
*
* For each one of the subscriptions in the map, send notification
*
* FIXME: this function is pretty similar to the one with the same name in
* mongoUpdateContext.cpp, so maybe it makes sense to factorize it.
*/
static bool processSubscriptions
(
  const EntityIdVector&                           triggerEntitiesV,
  std::map<std::string, TriggeredSubscription*>&  subs,
  std::string&                                    err,
  const std::string&                              tenant,
  const std::string&                              fiwareCorrelator
)
{
  bool ret = true;

  for (std::map<std::string, TriggeredSubscription*>::iterator it = subs.begin(); it != subs.end(); ++it)
  {
    std::string             mapSubId = it->first;
    TriggeredSubscription*  trigs    = it->second;

    /* Send notification */
    if (!processAvailabilitySubscription(triggerEntitiesV,
                                         trigs->attrL,
                                         mapSubId,
                                         trigs->httpInfo.url,
                                         trigs->renderFormat,
                                         tenant,
                                         fiwareCorrelator))
    {
      LM_T(LmtMongo, ("Notification failure"));
      ret = false;
    }

    //
    // Release object created dynamically (including the value in the map created by
    // addTriggeredSubscriptions
    //
    trigs->attrL.release();
    delete it->second;
  }

  subs.clear();
  return ret;
}



/* ****************************************************************************
*
* addTriggeredSubscriptions -
*/
static bool addTriggeredSubscriptions
(
  ContextRegistration                             cr,
  std::map<std::string, TriggeredSubscription*>&  subs,
  std::string&                                    err,
  std::string                                     tenant
)
{
  BSONArrayBuilder          entitiesNoPatternA;
  std::vector<std::string>  idJsV;
  std::vector<std::string>  typeJsV;

  for (unsigned int ix = 0; ix < cr.entityIdVector.size(); ++ix)
  {
    // FIXME: take into account subscriptions with no type
    EntityId* enP = cr.entityIdVector[ix];

    // The registration of isPattern=true entities is not supported, so we don't include them here
    if (enP->isPattern == "false")
    {
      entitiesNoPatternA.append(BSON(CASUB_ENTITY_ID << enP->id <<
                                     CASUB_ENTITY_TYPE << enP->type <<
                                     CASUB_ENTITY_ISPATTERN << "false"));
      idJsV.push_back(enP->id);
      typeJsV.push_back(enP->type);
    }
  }

  BSONArrayBuilder attrA;
  for (unsigned int ix = 0; ix < cr.contextRegistrationAttributeVector.size(); ++ix)
  {
    ContextRegistrationAttribute* craP = cr.contextRegistrationAttributeVector[ix];
    attrA.append(craP->name);
  }

  BSONObjBuilder queryNoPattern;
  queryNoPattern.append(CASUB_ENTITIES, BSON("$in" << entitiesNoPatternA.arr()));
  if (attrA.arrSize() > 0)
  {
    // If we don't do this checking, the {$in: [] } in the attribute name part will
    // make the query fail
    //

    // queryB.append(CASUB_ATTRS, BSON("$in" << attrA.arr()));
    queryNoPattern.append("$or", BSON_ARRAY(
                            BSON(CASUB_ATTRS << BSON("$in" << attrA.arr())) <<
                            BSON(CASUB_ATTRS << BSON("$size" << 0))));
  }
  else
  {
    queryNoPattern.append(CASUB_ATTRS, BSON("$size" << 0));
  }
  queryNoPattern.append(CASUB_EXPIRATION, BSON("$gt" << (long long) getCurrentTime()));


  //
  // This is JavaScript code that runs in MongoDB engine. As far as I know, this is the only
  // way to do a "reverse regex" query in MongoDB (see
  // http://stackoverflow.com/questions/15966991/mongodb-reverse-regex/15989520).
  // Note that although we are using a isPattern=true in the MongoDB query besides $where, we
  // also need to check that in the if statement in the JavaScript function given that a given
  // sub document could include both isPattern=true and isPattern=false documents
  //
  std::string idJsString = "[ ";
  for (unsigned int ix = 0; ix < idJsV.size(); ++ix)
  {
    idJsString += "\"" + idJsV[ix] + "\"";

    if (ix != idJsV.size() - 1)
    {
      idJsString += " ,";
    }
  }
  idJsString += " ]";

  std::string typeJsString = "[ ";
  for (unsigned int ix = 0; ix < typeJsV.size(); ++ix)
  {
    typeJsString += "\"" + typeJsV[ix] + "\"";

    if (ix != typeJsV.size() - 1)
    {
      typeJsString += " ,";
    }
  }
  typeJsString += " ]";

  std::string function = std::string("function()") +
    "{" +
      "enId = " + idJsString + ";" +
      "enType = " + typeJsString + ";" +
      "for (var i=0; i < this." + CASUB_ENTITIES + ".length; i++) {" +
        "if (this." + CASUB_ENTITIES + "[i]." + CASUB_ENTITY_ISPATTERN + " == \"true\") {" +
          "for (var j = 0; j < enId.length; j++) {" +
            "if (enId[j].match(this." + CASUB_ENTITIES + "[i]." + CASUB_ENTITY_ID +
              ") && this." + CASUB_ENTITIES + "[i]." + CASUB_ENTITY_TYPE + " == enType[j]) {" +
              "return true; " +
            "}" +
          "}" +
        "}" +
      "}" +
      "return false; " +
    "}";
  LM_T(LmtMongo, ("JS function: %s", function.c_str()));


  std::string     entPatternQ = CSUB_ENTITIES "." CSUB_ENTITY_ISPATTERN;
  BSONObjBuilder  queryPattern;

  queryPattern.append(entPatternQ, "true");
  queryPattern.append(CASUB_EXPIRATION, BSON("$gt" << (long long) getCurrentTime()));
  queryPattern.appendCode("$where", function);

  std::auto_ptr<DBClientCursor>  cursor;
  BSONObj                        query = BSON("$or" << BSON_ARRAY(queryNoPattern.obj() << queryPattern.obj()));

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();
  if (!collectionQuery(connection, getSubscribeContextAvailabilityCollectionName(tenant), query, &cursor, &err))
  {
    TIME_STAT_MONGO_READ_WAIT_STOP();
    releaseMongoConnection(connection);
    return false;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* For each one of the subscriptions found, add it to the map (if not already there) */
  while (moreSafe(cursor))
  {
    BSONObj     sub;
    std::string err;

    if (!nextSafeOrErrorF(cursor, &sub, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), query.toString().c_str()));
      continue;
    }
    BSONElement idField = getFieldF(sub, "_id");

    //
    // BSONElement::eoo returns true if 'not found', i.e. the field "_id" doesn't exist in 'sub'
    //
    // Now, if 'getFieldF(sub, "_id")' is not found, if we continue, calling OID() on it, then we get
    // an exception and the broker crashes.
    //
    if (idField.eoo() == true)
    {
      std::string details = std::string("error retrieving _id field in doc: '") + sub.toString() + "'";

      alarmMgr.dbError(details);
      continue;
    }
    alarmMgr.dbErrorReset();

    std::string subIdStr = idField.OID().toString();

    if (subs.count(subIdStr) == 0)
    {
      ngsiv2::HttpInfo httpInfo;

      httpInfo.url = getStringFieldF(sub, CASUB_REFERENCE);

      LM_T(LmtMongo, ("adding subscription: '%s'", sub.toString().c_str()));

      //
      // FIXME P4: Once ctx availability notification formats get defined for NGSIv2,
      //           the first parameter for TriggeredSubscription will have "normalized" as default value
      //
      RenderFormat           renderFormat = sub.hasField(CASUB_FORMAT)? stringToRenderFormat(getStringFieldF(sub, CASUB_FORMAT)) : NGSI_V1_LEGACY;
      TriggeredSubscription* trigs        = new TriggeredSubscription(renderFormat, httpInfo, subToAttributeList(sub));

      subs.insert(std::pair<std::string, TriggeredSubscription*>(subIdStr, trigs));
    }
  }
  releaseMongoConnection(connection);

  return true;
}



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

  // In NGISv1 forwarding mode is always "all"
  reg.append(REG_FORWARDING_MODE, "all");

  //
  // We accumulate the subscriptions in a map. The key of the map is the string representing subscription id
  // 'triggerEntitiesV' is used to define which entities to include in notifications
  //
  std::map<std::string, TriggeredSubscription*>  subsToNotify;
  EntityIdVector                                 triggerEntitiesV;
  BSONArrayBuilder                               contextRegistration;

  for (unsigned int ix = 0; ix < requestP->contextRegistrationVector.size(); ++ix)
  {
    ContextRegistration*  cr = requestP->contextRegistrationVector[ix];
    BSONArrayBuilder      entities;

    for (unsigned int jx = 0; jx < cr->entityIdVector.size(); ++jx)
    {
      EntityId* en = cr->entityIdVector[jx];

      triggerEntitiesV.push_back(en);

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

    std::string err;

    if (!addTriggeredSubscriptions(*cr, subsToNotify, err, tenant))
    {
      responseP->errorCode.fill(SccReceiverInternalError, err);
      return SccOk;
    }
  }
  reg.append(REG_CONTEXT_REGISTRATION, contextRegistration.arr());

  /* Note that we are using upsert = "true". This means that if the document doesn't previously
   * exist in the collection, it is created. Thus, this way both uses of registerContext are OK
   * (either new registration or updating an existing one)
   */
  if (!collectionUpdate(getRegistrationsCollectionName(tenant), BSON("_id" << oid), reg.obj(), true, &err))
  {
    responseP->errorCode.fill(SccReceiverInternalError, err);
    releaseTriggeredSubscriptions(&subsToNotify);
    return SccOk;
  }

  //
  // Send notifications for each one of the subscriptions accumulated by
  // previous addTriggeredSubscriptions() invocations
  //
  processSubscriptions(triggerEntitiesV, subsToNotify, err, tenant, fiwareCorrelator);

  // Fill the response element
  responseP->duration = requestP->duration;
  responseP->registrationId.set(oid.toString());
  responseP->errorCode.fill(SccOk);

  return SccOk;
}
