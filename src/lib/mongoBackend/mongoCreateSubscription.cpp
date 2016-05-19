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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "rest/OrionError.h"
#include "apiTypesV2/Subscription.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/MongoCommonSubscription.h"
#include "mongoBackend/dbConstants.h"
#include "common/defaultValues.h"
#include "cache/subCache.h"

#include "mongo/client/dbclient.h"

using namespace mongo;
using namespace ngsiv2;


/* ****************************************************************************
*
* mongoCreateSubscription -
*
* Returns:
* - subId: subscription susscessfully created ('oe' must be ignored), the subId
*   must be used to fill Location header
* - "": subscription creation fail (look to 'oe')
*/
std::string mongoCreateSubscription
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
  std::string    servicePath = servicePathV[0] == "" ? DEFAULT_SERVICE_PATH_QUERIES : servicePathV[0];
  bool           notificationDone = false;
  long long      lastNotification = 0;

  const std::string subId = setNewSubscriptionId(&b);
  setExpiration(sub, &b);
  setHttpInfo(sub, &b);
  setThrottling(sub, &b);
  setServicePath(servicePath, &b);
  setDescription(sub, &b);
  setStatus(sub, &b);
  setEntities(sub, &b);
  setAttrs(sub, &b);
  setBlacklist(sub, &sub);

  std::string status = sub.status == ""?  STATUS_ACTIVE : sub.status;
  setCondsAndInitialNotify(sub, subId, status, sub.notification.httpInfo.url, sub.attrsFormat,
                           tenant, servicePathV, xauthToken, fiwareCorrelator,
                           &b, &notificationDone);
  if (notificationDone)
  {
    long long lastNotification = (long long) getCurrentTime();
    setLastNotification(lastNotification, &b);
    setCount(1, &b);
  }

  setExpression(sub, &b);
  setFormat(sub, &b);

  BSONObj doc = b.obj();

  // Insert in DB
  std::string err;
  if (!collectionInsert(getSubscribeContextCollectionName(tenant), doc, &err))
  {
    reqSemGive(__FUNCTION__, "ngsiv2 create subscription request", reqSemTaken);
    oe->fill(SccReceiverInternalError, err);
    return "";
  }

  // Insert in csub cache

  //
  // StringFilter in Scope?
  //
  // Any Scope of type SCOPE_TYPE_SIMPLE_QUERY sub.restriction.scopeVector?
  // If so, set it as string filter to the sub-cache item
  //
  StringFilter*  stringFilterP = NULL;

  for (unsigned int ix = 0; ix < sub.restriction.scopeVector.size(); ++ix)
  {
    if (sub.restriction.scopeVector[ix]->type == SCOPE_TYPE_SIMPLE_QUERY)
    {
      stringFilterP = sub.restriction.scopeVector[ix]->stringFilterP;
    }
  }

  cacheSemTake(__FUNCTION__, "Inserting subscription in cache");
  subCacheItemInsert(tenant.c_str(),
                     servicePath.c_str(),
                     sub.notification.httpInfo,
                     sub.subject.entities,
                     sub.notification.attributes,
                     sub.subject.condition.attributes,
                     subId.c_str(),
                     sub.expires,
                     sub.throttling,
                     sub.attrsFormat,
                     notificationDone,
                     lastNotification,
                     stringFilterP,
                     sub.status,
                     sub.subject.condition.expression.q,
                     sub.subject.condition.expression.geometry,
                     sub.subject.condition.expression.coords,
                     sub.subject.condition.expression.georel);

  cacheSemGive(__FUNCTION__, "Inserting subscription in cache");

  reqSemGive(__FUNCTION__, "ngsiv2 create subscription request", reqSemTaken);

  return subId;
}
