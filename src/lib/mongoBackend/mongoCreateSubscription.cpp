/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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

#include <map>
#include <string>
#include <vector>

#include "common/RenderFormat.h"
#include "common/defaultValues.h"
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "rest/OrionError.h"
#include "apiTypesV2/Subscription.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/MongoGlobal.h"

#include "mongo/client/dbclient.h"

using namespace mongo;
using namespace ngsiv2;



/* ****************************************************************************
*
* setSubscriptionId -
*
*/
static const char* setSubscriptionId(const Subscription& sub, BSONObjBuilder* b)
{
  OID  oid;
  oid.init();
  b->append("_id", oid);

  LM_T(LmtMongo, ("Subscription _id: %s", oid.toString().c_str()));
  return oid.toString().c_str();
}



/* ****************************************************************************
*
* setExpiration -
*
*/
static void setExpiration(const Subscription& sub, BSONObjBuilder* b)
{
  b->append(CSUB_EXPIRATION, sub.expires);
  LM_T(LmtMongo, ("Subscription expiration: %lu", sub.expires));
}



/* ****************************************************************************
*
* setReference -
*
*/
static void setReference(const Subscription& sub, BSONObjBuilder* b)
{
  b->append(CSUB_REFERENCE, sub.notification.httpInfo.url);
  LM_T(LmtMongo, ("Subscription reference: %s", sub.notification.httpInfo.url.c_str()));
}



/* ****************************************************************************
*
* setThrottling -
*
*/
static void setThrottling(const Subscription& sub, BSONObjBuilder* b)
{
  b->append(CSUB_THROTTLING, sub.throttling);
  LM_T(LmtMongo, ("Subscription throttling: %lu", sub.throttling));
}



/* ****************************************************************************
*
* setServicePath -
*
*/
static void setServicePath(const std::vector<std::string>& servicePathV, BSONObjBuilder* b)
{
  std::string servicePath = servicePathV[0] == ""? DEFAULT_SERVICE_PATH_QUERIES : servicePathV[0];
  b->append(CSUB_SERVICE_PATH, servicePath);
  LM_T(LmtMongo, ("Subscription servicePath: %s", servicePath.c_str()));
}



/* ****************************************************************************
*
* setDescription -
*
*/
static void setDescription(const Subscription& sub, BSONObjBuilder* b)
{
  if (sub.description != "")
  {
    b->append(CSUB_DESCRIPTION, sub.description);
    LM_T(LmtMongo, ("Subscription description: %s", sub.description.c_str()));
  }
}



/* ****************************************************************************
*
* setDescription -
*
*/
static void setStatus(const Subscription& sub, BSONObjBuilder* b)
{
  std::string status = sub.status == ""?  STATUS_ACTIVE : sub.status;
  b->append(CSUB_STATUS, status);
  LM_T(LmtMongo, ("Subscription status: %s", status.c_str()));
}



/* ****************************************************************************
*
* setEntities -
*
*/
static void setEntities(const Subscription& sub, BSONObjBuilder* b)
{
  BSONArrayBuilder entities;
  for (unsigned int ix = 0; ix < sub.subject.entities.size(); ++ix)
  {
    EntID en = sub.subject.entities[ix];

    if (en.type == "")
    {
      if (en.id != "")
      {
        entities.append(BSON(CSUB_ENTITY_ID << en.id << CSUB_ENTITY_ISPATTERN << "false"));

      }
      else // idPattern
      {
        entities.append(BSON(CSUB_ENTITY_ID << en.idPattern << CSUB_ENTITY_ISPATTERN << "true"));
      }
    }
    else // type != ""
    {
      if (en.id != "")
      {
        entities.append(BSON(CSUB_ENTITY_ID << en.id <<
                             CSUB_ENTITY_TYPE << en.type <<
                             CSUB_ENTITY_ISPATTERN << "false"));
      }
      else // idPattern
      {
        entities.append(BSON(CSUB_ENTITY_ID << en.idPattern <<
                             CSUB_ENTITY_TYPE << en.type <<
                             CSUB_ENTITY_ISPATTERN << "true"));
      }
    }
  }
  BSONArray entitiesArr = entities.arr();
  b->append(CSUB_ENTITIES, entitiesArr);
  LM_T(LmtMongo, ("Subscription entities: %s", entitiesArr.toString().c_str()));
}



/* ****************************************************************************
*
* setAttrs -
*
*/
static void setAttrs(const Subscription& sub, BSONObjBuilder* b)
{
  BSONArrayBuilder attrs;
  for (unsigned int ix = 0; ix < sub.notification.attributes.size(); ++ix)
  {
    attrs.append(sub.notification.attributes[ix]);
  }
  BSONArray attrsArr = attrs.arr();
  b->append(CSUB_ATTRS, attrsArr);
  LM_T(LmtMongo, ("Subscription attributes: %s", attrsArr.toString().c_str()));
}



/* ****************************************************************************
*
* setConds -
*
*/
static void setCondsAndInitialNotify
(
  const Subscription&              sub,
  const std::string&               subId,
  const std::string&               tenant,
  const std::vector<std::string>&  servicePathV,
  const std::string&               xauthToken,
  const std::string&               fiwareCorrelator,
  BSONObjBuilder*                  b
)
{
  /* Conds vector (and maybe and initial notification) */
  bool       notificationDone = false;
  BSONArray  conds = processConditionVector(sub.subject.condition.attributes, //&requestP->notifyConditionVector,
                                            sub.subject.entities, //requestP->entityIdVector,
                                            sub.notification.attributes, //requestP->attributeList,
                                            subId,
                                            sub.notification.httpInfo.url,
                                            &notificationDone,
                                            //requestP->attrsFormat, -> sub.notification.attrsFormat
                                            tenant,
                                            xauthToken,
                                            servicePathV,
                                            //&requestP->restriction, -> sub.restriction ??
                                            sub.status,
                                            fiwareCorrelator,
                                            sub.notification.attributes); // requestP->attributeList.attributeV);

  b->append(CSUB_CONDITIONS, conds);
  LM_T(LmtMongo, ("Subscription conditions: %s", conds.toString().c_str()));

  /* Last notification */
  long long lastNotificationTime = 0;
  if (notificationDone)
  {
    lastNotificationTime = (long long) getCurrentTime();

    b->append(CSUB_LASTNOTIFICATION, lastNotificationTime);
    b->append(CSUB_COUNT, (long long) 1);
    LM_T(LmtMongo, ("Subscription lastNotification: %lu", lastNotificationTime));
    LM_T(LmtMongo, ("Subscription count: 1"));
  }
}



/* ****************************************************************************
*
* setExpression -
*
*/
static void setExpression(const Subscription& sub, BSONObjBuilder* b)
{
  BSONObj expression = BSON(CSUB_EXPR_Q      << sub.subject.condition.expression.q        <<
                            CSUB_EXPR_GEOM   << sub.subject.condition.expression.geometry <<
                            CSUB_EXPR_COORDS << sub.subject.condition.expression.coords   <<
                            CSUB_EXPR_GEOREL << sub.subject.condition.expression.georel
                           );
  b->append(CSUB_EXPR, expression);
  LM_T(LmtMongo, ("Subscription expression: %s", expression.toString().c_str()));
}



/* ****************************************************************************
*
* setFormat -
*
*/
static void setFormat(const Subscription& sub, BSONObjBuilder* b)
{
  //std::string format = renderFormatToString(sub.notification.attrsFormat);
  std::string format = "";
  b->append(CSUB_FORMAT, format);
  LM_T(LmtMongo, ("Subscription format: %s", format.c_str()));
}



/* ****************************************************************************
*
* mongoCreateSubscription -
*
* Returns:
* - true: subscription susscessfully created ('oe' must be ignored)
* - false: subscription creation fail (look to 'oe')
*/
bool mongoCreateSubscription
(
  const Subscription&                  sub,
  OrionError*                          oe,
  std::map<std::string, std::string>&  uriParams,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV,
  const std::string&                   xauthToken,
  const std::string&                   fiwareCorrelator
)
{
  bool reqSemTaken = false;

  reqSemTake(__FUNCTION__, "ngsiv2 create subscription request", SemWriteOp, &reqSemTaken);

  // Build the BSON object to insert
  BSONObjBuilder b;

  const std::string subId = setSubscriptionId(sub, &b);
  setExpiration(sub, &b);
  setReference(sub, &b);
  setThrottling(sub, &b);
  setServicePath(servicePathV, &b);
  setDescription(sub, &b);
  setStatus(sub, &b);
  setEntities(sub, &b);
  setAttrs(sub, &b);
  setCondsAndInitialNotify(sub, subId, tenant, servicePathV, xauthToken, fiwareCorrelator, &b);
  setExpression(sub, &b);
  setFormat(sub, &b);

  BSONObj doc = b.obj();

  // Insert in DB
  std::string err;
  if (!collectionInsert(getSubscribeContextCollectionName(tenant), doc, &err))
  {
    reqSemGive(__FUNCTION__, "ngsiv2 create subscription request", reqSemTaken);
    oe->fill(SccReceiverInternalError, err);
    return false;
  }

  // Insert in csub cache
#if 0

  //
  // StringFilter in Scope?
  //
  // Any Scope of type SCOPE_TYPE_SIMPLE_QUERY in requestP->restriction.scopeVector?
  // If so, set it as string filter to the sub-cache item
  //
  StringFilter*  stringFilterP = NULL;

  for (unsigned int ix = 0; ix < requestP->restriction.scopeVector.size(); ++ix)
  {
    if (requestP->restriction.scopeVector[ix]->type == SCOPE_TYPE_SIMPLE_QUERY)
    {
      stringFilterP = requestP->restriction.scopeVector[ix]->stringFilterP;
    }
  }


  std::string oidString = oid.toString();

  LM_T(LmtSubCache, ("inserting a new sub in cache (%s)", oidString.c_str()));

  cacheSemTake(__FUNCTION__, "Inserting subscription in cache");
  subCacheItemInsert(tenant.c_str(),
                     servicePath.c_str(),
                     requestP,
                     oidString.c_str(),
                     expiration,
                     throttling,
                     requestP->attrsFormat,
                     notificationDone,
                     lastNotificationTime,
                     stringFilterP,
                     status,
                     requestP->expression.q,
                     requestP->expression.geometry,
                     requestP->expression.coords,
                     requestP->expression.georel);

  cacheSemGive(__FUNCTION__, "Inserting subscription in cache");
#endif


  reqSemGive(__FUNCTION__, "ngsiv2 create subscription request", reqSemTaken);

  return true;
}
