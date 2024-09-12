/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <sys/types.h>
#include <regex.h>
#include <string>
#include <vector>
#include <map>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"
#include "common/string.h"
#include "apiTypesV2/HttpInfo.h"
#include "apiTypesV2/MqttInfo.h"
#include "apiTypesV2/Subscription.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoSubCache.h"
#include "ngsi10/SubscribeContextRequest.h"
#include "cache/subCache.h"
#include "alarmMgr/alarmMgr.h"
#include "mongoBackend/dbConstants.h"
#include "mongoDriver/connectionOperations.h"

using std::map;



/* ****************************************************************************
*
* subCacheState - maintains the state of the subscription cache
*/
volatile SubCacheState subCacheState = ScsIdle;


//
// The subscription cache maintains in memory all subscriptions.
// The reason for this cache is to avoid a very slow mongo operation ... (Fermin)
//
// The 'mongo part' of the cache is implemented in mongoBackend/mongoSubCache.cpp/h and used in:
//   - MongoCommonUpdate.cpp               (in function addTriggeredSubscriptions_withCache)
//   - mongoSubscribeContext.cpp           (in function mongoSubscribeContext)
//   - mongoUnsubscribeContext.cpp         (in function mongoUnsubscribeContext)
//   - mongoUpdateContextSubscription.cpp  (in function mongoUpdateContextSubscription)
//   - contextBroker.cpp                   (to initialize and sybchronize)
//
// To manipulate the subscription cache, a semaphore is necessary, as various threads can be
// reading/inserting/removing subs at the same time.
// This semaphore is NOT optional, like the mongo request semaphore.
//
// Two functions have been added to common/sem.cpp|h for this: cacheSemTake/Give.
//


/* ****************************************************************************
*
* EntityInfo::EntityInfo -
*/
EntityInfo::EntityInfo
(
  const std::string&  _entityId,
  const std::string&  _entityType,
  const std::string&  _isPattern,
  bool                _isTypePattern
)
:
entityId(_entityId), entityType(_entityType), isTypePattern(_isTypePattern)
{
  isPattern    = (_isPattern == "true") || (_isPattern == "TRUE") || (_isPattern == "True");

  if (isPattern)
  {
    if (!regComp(&entityIdPattern, _entityId.c_str(), REG_EXTENDED))
    {
      alarmMgr.badInput(clientIp, "invalid regular expression for idPattern", _entityId);
      isPattern = false;  // FIXME P6: this entity should not be let into the system. Must be stopped before.
                          //           Right here, best thing to do is simply to say it is not a regex
      entityIdPatternToBeFreed = false;
    }
    else
    {
      entityIdPatternToBeFreed = true;
    }
  }
  else
  {
    entityIdPatternToBeFreed = false;
  }

  if (isTypePattern)
  {
    if (!regComp(&entityTypePattern, _entityType.c_str(), REG_EXTENDED))
    {
      alarmMgr.badInput(clientIp, "invalid regular expression for typePattern", _entityType);
      isTypePattern = false;  // FIXME P6: this entity should not be let into the system. Must be stopped before.
                          //           Right here, best thing to do is simply to say it is not a regex
      entityTypePatternToBeFreed = false;
    }
    else
    {
      entityTypePatternToBeFreed = true;
    }
  }
  else
  {
    entityTypePatternToBeFreed = false;
  }
}



/* ****************************************************************************
*
* EntityInfo::match -
*/
bool EntityInfo::match
(
  const std::string&  id,
  const std::string&  type
)
{
  bool matchedType = false;
  bool matchedId   = false;

  // Check id
  if (isPattern)
  {
    // REGEX-comparison this->entityIdPattern VS id
    matchedId =  (regexec(&entityIdPattern, id.c_str(), 0, NULL, 0) == 0);
  }
  else if (id == entityId)
  {
    matchedId =  true;
  }
  else
  {
    matchedId = false;
  }

  // short-circuit, optimization
  if (matchedId)
  {
    // Check type
    if (isTypePattern)
    {
      // REGEX-comparison this->entityTypePattern VS type
      matchedType = (regexec(&entityTypePattern, type.c_str(), 0, NULL, 0) == 0);
    }
    else if ((!type.empty())  && (!entityType.empty()) && (entityType != type))
    {
      matchedType = false;
    }
    else
    {
      matchedType = true;
    }
  }

  return matchedId && matchedType;
}



/* ****************************************************************************
*
* EntityInfo::release -
*/
void EntityInfo::release(void)
{
  if (entityIdPatternToBeFreed)
  {
    regfree(&entityIdPattern);  // If regcomp fails it frees up itself (see glibc sources for details)
    entityIdPatternToBeFreed = false;
  }

  if (entityTypePatternToBeFreed)
  {
    regfree(&entityTypePattern);  // If regcomp fails it frees up itself (see glibc sources for details)
    entityTypePatternToBeFreed = false;
  }
}



/* ****************************************************************************
*
* SubCache -
*/
typedef struct SubCache
{
  CachedSubscription* head;
  CachedSubscription* tail;

  // Statistics counters
  int                 noOfRefreshes;
  int                 noOfInserts;
  int                 noOfRemoves;
  int                 noOfUpdates;
} SubCache;



/* ****************************************************************************
*
* subCache -
*/
static SubCache  subCache            = { NULL, NULL, 0, 0, 0, 0 };
bool             subCacheActive      = false;
bool             subCacheMultitenant = false;



/* ****************************************************************************
*
* subCacheInit -
*/
void subCacheInit(bool multitenant)
{
  LM_T(LmtSubCache, ("Initializing subscription cache"));
  subCacheMultitenant = multitenant;

  subCache.head   = NULL;
  subCache.tail   = NULL;

  subCacheStatisticsReset("subCacheInit");

  subCacheActive = true;
}



/* ****************************************************************************
*
* subCacheDisable -
*
*/
#ifdef UNIT_TEST
void subCacheDisable(void)
{
  subCacheActive = false;
}
#endif



/* ****************************************************************************
*
* subCacheItems -
*/
int subCacheItems(void)
{
  CachedSubscription* cSubP = subCache.head;
  int                 items = 0;

  while (cSubP != NULL)
  {
    ++items;
    cSubP = cSubP->next;
  }

  return items;
}



/* ****************************************************************************
*
* attributeMatch -
*/
static bool attributeMatch(CachedSubscription* cSubP, const std::vector<std::string>& attrV)
{
  // If the list of attributes to check is empty, then no match
  if (attrV.size() == 0)
  {
    return false;
  }

  if (cSubP->notifyConditionV.size() == 0)
  {
    return true;
  }

  for (unsigned int ncvIx = 0; ncvIx < cSubP->notifyConditionV.size(); ++ncvIx)
  {
    for (unsigned int aIx = 0; aIx < attrV.size(); ++aIx)
    {
      if (cSubP->notifyConditionV[ncvIx] == attrV[aIx])
      {
        return true;
      }
    }
  }

  return false;
}



/* ****************************************************************************
*
* servicePathMatch -
*/
static bool servicePathMatch(CachedSubscription* cSubP, char* servicePath)
{
  const char*  spath;

  // Special case. "/#" matches EVERYTHING
  if ((strlen(servicePath) == 2) && (servicePath[0] == '/') && (servicePath[1] == '#') && (servicePath[2] == 0))
  {
    return true;
  }

  //
  // Empty service path?
  //
  if ((strlen(servicePath) == 0) && (strlen(cSubP->servicePath) == 0))
  {
    return true;
  }

  //
  // Default service path for a subscription is "/".
  // So, if empty, set it to "/"
  //
  if (servicePath[0] == 0)
  {
    servicePath = (char*) "/";
  }

  spath = cSubP->servicePath;

  //
  // No wildcard - exact match
  //
  if (spath[strlen(spath) - 1] != '#')
  {
    return (strcmp(servicePath, cSubP->servicePath) == 0);
  }


  //
  // Wildcard - remove '#' and compare to the length of _servicePath.
  //            If equal upto the length of _servicePath, then it is a match
  //
  // Actually, there is more than one way to match here:
  // If we have a servicePath of the subscription as
  // "/a/b/#", then the following service-paths must match:
  //   1. /a/b
  //   2. /a/b/ AND /a/b/.+  (any path below /a/b/)
  //
  // What should NOT match is "/a/b2".
  //
  unsigned int len = strlen(spath) - 2;

  // 1. /a/b - (removing 2 chars from the cache-subscription removes "/#"

  if ((spath[len] == '/') && (strlen(servicePath) == len) && (strncmp(spath, servicePath, len) == 0))
  {
    return true;
  }

  // 2. /a/b/.+
  len = strlen(spath) - 1;
  if (strncmp(spath, servicePath, len) == 0)
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* matchAltType -
*
*/
static bool matchAltType(CachedSubscription* cSubP, ngsiv2::SubAltType targetAltType)
{
  // If subAltTypeV size == 0 default alteration types are update with change and create
  if (cSubP->subAltTypeV.size() == 0)
  {
    if ((isChangeAltType(targetAltType)) || (targetAltType == ngsiv2::SubAltType::EntityCreate))
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  for (unsigned int ix = 0; ix < cSubP->subAltTypeV.size(); ix++)
  {
    ngsiv2::SubAltType altType = cSubP->subAltTypeV[ix];

    // EntityUpdate is special, it is a "sub-type" of EntityChange
    if (isChangeAltType(targetAltType))
    {
      if ((altType == ngsiv2::SubAltType::EntityUpdate) || (isChangeAltType(altType)))
      {
        return true;
      }
    }
    else if (altType == targetAltType)
    {
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* subMatch -
*/
static bool subMatch
(
  CachedSubscription*              cSubP,
  const char*                      tenant,
  const char*                      servicePath,
  const char*                      entityId,
  const char*                      entityType,
  const std::vector<std::string>&  attributes,
  const std::vector<std::string>&  attrsWithModifiedValue,
  const std::vector<std::string>&  attrsWithModifiedMd,
  ngsiv2::SubAltType               targetAltType
)
{
  // If notifyOnMetadataChange is false and only metadata has been changed, we "downgrade" to ngsiv2::EntityUpdate
  if (!cSubP->notifyOnMetadataChange && (targetAltType == ngsiv2::EntityChangeOnlyMetadata))
  {
    targetAltType = ngsiv2::EntityUpdate;
  }

  // Check alteration type
  if (!matchAltType(cSubP, targetAltType))
  {
    return false;
  }

  //
  // Check to filter out due to tenant - only valid if Broker has started with -multiservice option
  //
  if (subCacheMultitenant == true)
  {
    if ((cSubP->tenant == NULL) || (tenant == NULL) || (cSubP->tenant[0] == 0) || (tenant[0] == 0))
    {
      if ((cSubP->tenant != NULL) && (cSubP->tenant[0] != 0))
      {
        LM_T(LmtSubCacheMatch, ("No match due to tenant I"));
        return false;
      }

      if ((tenant != NULL) && (tenant[0] != 0))
      {
        LM_T(LmtSubCacheMatch, ("No match due to tenant II"));
        return false;
      }
    }
    else if (strcmp(cSubP->tenant, tenant) != 0)
    {
      LM_T(LmtSubCacheMatch, ("No match due to tenant III"));
      return false;
    }
  }

  if (servicePathMatch(cSubP, (char*) servicePath) == false)
  {
    LM_T(LmtSubCacheMatch, ("No match due to servicePath"));
    return false;
  }

  //
  // If one of the attribute names in the scope vector
  // of the subscription has the same name as the incoming attribute. there is a match.
  // Additionaly, if the attribute list in cSubP is empty, there is a match (this is the
  // case of ONANYCHANGE subscriptions).
  //
  // Depending of the alteration type, we use the list of attributes in the request or the list
  // with effective modifications. Note that EntityDelete doesn't check the list
  if (targetAltType == ngsiv2::EntityUpdate)
  {
    if (!attributeMatch(cSubP, attributes))
    {
      LM_T(LmtSubCacheMatch, ("No match due to attributes"));
      return false;
    }
  }
  else if ((isChangeAltType(targetAltType)) || (targetAltType == ngsiv2::EntityCreate))
  {
    // No match if: 1) there is no change in the *value* of attributes listed in conditions.attrs and 2) there is no change
    // in the *metadata* of the attributes listed in conditions.attrs (the 2) only if notifyOnMetadataChange is true)
    bool b1 = attributeMatch(cSubP, attrsWithModifiedValue);
    bool b2 = attributeMatch(cSubP, attrsWithModifiedMd);
    if (!b1 && !(cSubP->notifyOnMetadataChange && b2))
    {
      LM_T(LmtSubCacheMatch, ("No match due to attributes"));
      return false;
    }
  }

  for (unsigned int ix = 0; ix < cSubP->entityIdInfos.size(); ++ix)
  {
    EntityInfo* eiP = cSubP->entityIdInfos[ix];

    if (eiP->match(entityId, entityType))
    {
      return true;
    }
  }

  LM_T(LmtSubCacheMatch, ("No match due to EntityInfo"));
  return false;
}



/* ****************************************************************************
*
* subCacheMatch -
*/
void subCacheMatch
(
  const char*                        tenant,
  const char*                        servicePath,
  const char*                        entityId,
  const char*                        entityType,
  const char*                        attr,
  ngsiv2::SubAltType                 targetAltType,
  std::vector<CachedSubscription*>*  subVecP
)
{
  CachedSubscription* cSubP = subCache.head;

  while (cSubP != NULL)
  {
    std::vector<std::string> attrV;

    attrV.push_back(attr);

    if (subMatch(cSubP, tenant, servicePath, entityId, entityType, attrV, attrV, attrV, targetAltType))
    {
      subVecP->push_back(cSubP);
      LM_T(LmtSubCache, ("added subscription '%s': lastNotificationTime: %lu",
                         cSubP->subscriptionId, cSubP->lastNotificationTime));
    }

    cSubP = cSubP->next;
  }
}



/* ****************************************************************************
*
* subCacheMatch -
*/
void subCacheMatch
(
  const char*                        tenant,
  const char*                        servicePath,
  const char*                        entityId,
  const char*                        entityType,
  const std::vector<std::string>&    attributes,
  const std::vector<std::string>&    attrsWithModifiedValue,
  const std::vector<std::string>&    attrsWithModifiedMd,
  ngsiv2::SubAltType                 targetAltType,
  std::vector<CachedSubscription*>*  subVecP
)
{
  CachedSubscription* cSubP = subCache.head;

  while (cSubP != NULL)
  {
    if (subMatch(cSubP, tenant, servicePath, entityId, entityType, attributes, attrsWithModifiedValue, attrsWithModifiedMd, targetAltType))
    {
      subVecP->push_back(cSubP);
      LM_T(LmtSubCache, ("added subscription '%s': lastNotificationTime: %lu",
                         cSubP->subscriptionId, cSubP->lastNotificationTime));
    }

    cSubP = cSubP->next;
  }
}



/* ****************************************************************************
*
* subCacheItemDestroy -
*/
void subCacheItemDestroy(CachedSubscription* cSubP)
{
  if (cSubP->tenant != NULL)
  {
    free(cSubP->tenant);
    cSubP->tenant = NULL;
  }

  if (cSubP->servicePath != NULL)
  {
    free(cSubP->servicePath);
    cSubP->servicePath = NULL;
  }

  if (cSubP->subscriptionId != NULL)
  {
    free(cSubP->subscriptionId);
    cSubP->subscriptionId = NULL;
  }

  cSubP->httpInfo.release();
  cSubP->mqttInfo.release();

  for (unsigned int ix = 0; ix < cSubP->entityIdInfos.size(); ++ix)
  {
    cSubP->entityIdInfos[ix]->release();
    delete cSubP->entityIdInfos[ix];
  }

  cSubP->entityIdInfos.clear();

  while (cSubP->attributes.size() > 0)
  {
    cSubP->attributes.erase(cSubP->attributes.begin());
  }
  cSubP->attributes.clear();

  cSubP->notifyConditionV.clear();

  cSubP->next = NULL;
}



/* ****************************************************************************
*
* subCacheDestroy -
*/
void subCacheDestroy(void)
{
  LM_T(LmtSubCache, ("destroying subscription cache"));

  CachedSubscription* cSubP  = subCache.head;

  if (subCache.head == NULL)
  {
    return;
  }

  while (cSubP->next)
  {
    CachedSubscription* prev = cSubP;

    cSubP = cSubP->next;
    subCacheItemDestroy(prev);
    LM_T(LmtSubCache,  ("removing CachedSubscription at %p", prev));
    delete prev;
  }

  subCacheItemDestroy(cSubP);
  LM_T(LmtSubCache,  ("removing CachedSubscription at %p", cSubP));
  delete cSubP;

  subCache.head  = NULL;
  subCache.tail  = NULL;
}



/* ****************************************************************************
*
* tenantMatch -
*/
static bool tenantMatch(const char* tenant1, const char* tenant2)
{
  if (!subCacheMultitenant)
  {
    // If no multitenant is in use, then tenant match always
    return true;
  }

  //
  // Removing complications with NULL, giving NULL tenants the value of an empty string;
  //
  tenant1 = (tenant1 == NULL)? "" : tenant1;
  tenant2 = (tenant2 == NULL)? "" : tenant2;

  if (strlen(tenant1) != strlen(tenant2))
  {
    return false;
  }

  if (strcmp(tenant1, tenant2) == 0)
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* subCacheItemLookup -
*
* FIXME P7: lookups would be A LOT faster if the subCache used a hash-table instead of
*           just a single linked link for the subscriptions.
*           The hash could be adding each char in 'tenant' and 'subscriptionId' to a 8-bit
*           integer and then we'd have a vector of 256 linked lists.
*           Another way of implementing this would be to use a std::vector or std::map.
*/
CachedSubscription* subCacheItemLookup(const char* tenant, const char* subscriptionId)
{
  CachedSubscription* cSubP = subCache.head;

  while (cSubP != NULL)
  {
    if ((tenantMatch(tenant, cSubP->tenant)) && (strcmp(subscriptionId, cSubP->subscriptionId) == 0))
    {
      return cSubP;
    }

    cSubP = cSubP->next;
  }

  return NULL;
}



/* ****************************************************************************
*
* subCacheUpdateStatisticsIncrement -
*/
void subCacheUpdateStatisticsIncrement(void)
{
  ++subCache.noOfUpdates;
}



/* ****************************************************************************
*
* subCacheItemInsert -
*
* First of all, insertion is done at the end of the list, so,
* cSubP->next must ALWAYS be zero at insertion time
*
* Note that this is the insert function that *really inserts* the
* CachedSubscription in the list of CachedSubscriptions.
*
* All other subCacheItemInsert functions create the subscription and then
* calls this function.
*
* So, the subscription itself is untouched by this function, is it ONLY inserted
* in the list (only the 'next' field is modified).
*
*/
void subCacheItemInsert(CachedSubscription* cSubP)
{
  cSubP->next = NULL;

  LM_T(LmtSubCache, ("inserting sub '%s', lastNotificationTime: %lu",
                     cSubP->subscriptionId, cSubP->lastNotificationTime));

  ++subCache.noOfInserts;

  // First insertion?
  if ((subCache.head == NULL) && (subCache.tail == NULL))
  {
    subCache.head   = cSubP;
    subCache.tail   = cSubP;

    return;
  }

  subCache.tail->next  = cSubP;
  subCache.tail        = cSubP;
}



/* ****************************************************************************
*
* subCacheItemInsert - create a new sub, fill it in, and add it to cache
*
* Note that 'count' and 'failsCounter', which is the counter of how many times a notification has been
* fired and has failed, are set to ZERO
*/
void subCacheItemInsert
(
  const char*                             tenant,
  const char*                             servicePath,
  const ngsiv2::HttpInfo&                 httpInfo,
  const ngsiv2::MqttInfo&                 mqttInfo,
  const std::vector<ngsiv2::EntID>&       entIdVector,
  const std::vector<std::string>&         attributes,
  const std::vector<std::string>&         metadata,
  const std::vector<std::string>&         conditionAttrs,
  const std::vector<ngsiv2::SubAltType>&  altTypes,
  const char*                             subscriptionId,
  int64_t                                 expirationTime,
  int64_t                                 maxFailsLimit,
  int64_t                                 throttling,
  RenderFormat                            renderFormat,
  int64_t                                 lastNotificationTime,
  int64_t                                 lastNotificationSuccessTime,
  int64_t                                 lastNotificationFailureTime,
  int64_t                                 lastSuccessCode,
  const std::string&                      lastFailureReason,
  StringFilter*                           stringFilterP,
  StringFilter*                           mdStringFilterP,
  const std::string&                      status,
  double                                  statusLastChange,
  const std::string&                      q,
  const std::string&                      geometry,
  const std::string&                      coords,
  const std::string&                      georel,
  bool                                    blacklist,
  bool                                    onlyChanged,
  bool                                    covered,
  bool                                    notifyOnMetadataChange
)
{
  //
  // Add the subscription to the subscription cache.
  //

  CachedSubscription* cSubP = new CachedSubscription();
  LM_T(LmtSubCache,  ("allocated CachedSubscription at %p", cSubP));


  //
  // First the non-complex values
  //
  cSubP->tenant                = (tenant[0] == 0)? NULL : strdup(tenant);
  cSubP->servicePath           = strdup(servicePath);
  cSubP->subscriptionId        = strdup(subscriptionId);
  cSubP->expirationTime        = expirationTime;
  cSubP->maxFailsLimit         = maxFailsLimit;
  cSubP->throttling            = throttling;
  cSubP->lastNotificationTime  = lastNotificationTime;
  cSubP->lastFailure           = lastNotificationFailureTime;
  cSubP->lastSuccess           = lastNotificationSuccessTime;
  cSubP->lastFailureReason     = lastFailureReason;
  cSubP->lastSuccessCode       = lastSuccessCode;
  cSubP->renderFormat          = renderFormat;
  cSubP->next                  = NULL;
  cSubP->count                 = 0;
  cSubP->failsCounter          = 0;
  cSubP->status                = status;
  cSubP->statusLastChange      = statusLastChange;
  cSubP->expression.q          = q;
  cSubP->expression.geometry   = geometry;
  cSubP->expression.coords     = coords;
  cSubP->expression.georel     = georel;
  cSubP->blacklist             = blacklist;
  cSubP->onlyChanged           = onlyChanged;
  cSubP->covered               = covered;
  cSubP->notifyOnMetadataChange= notifyOnMetadataChange;
  cSubP->notifyConditionV      = conditionAttrs;
  cSubP->subAltTypeV           = altTypes;
  cSubP->attributes            = attributes;
  cSubP->metadata              = metadata;

  // empty cache entry, no relation with DB actually exists
  // (thus validity flag set to false)
  cSubP->failsCounterFromDb      = 0;
  cSubP->failsCounterFromDbValid = false;

  cSubP->httpInfo.fill(httpInfo);
  cSubP->mqttInfo.fill(mqttInfo);

  //
  // String filters
  //
  std::string  errorString;
  if (stringFilterP != NULL)
  {
    //
    // NOTE (for both 'q' and 'mq' string filters)
    //   Here, the cached subscription should have a String Filter but if 'fill()' fails, it won't.
    //   The subscription is already in mongo and hopefully this erroneous situation is fixed
    //   once the sub-cache is refreshed.
    //
    //   This 'but' should be minimized once the issue 2082 gets implemented.
    //   [ Only reason for fill() to fail (apart from out-of-memory) seems to be an invalid regex ]
    //
    cSubP->expression.stringFilter.fill(stringFilterP, &errorString);
  }

  if (mdStringFilterP != NULL)
  {
    cSubP->expression.mdStringFilter.fill(mdStringFilterP, &errorString);
  }



  //
  // Convert all EntIds to EntityInfo
  //
  for (unsigned int ix = 0; ix < entIdVector.size(); ++ix)
  {
    const ngsiv2::EntID* eIdP = &entIdVector[ix];
    std::string          isPattern      = (eIdP->id.empty())? "true" : "false";
    bool                 isTypePattern  = (eIdP->type.empty());
    std::string          id             = (eIdP->id.empty())? eIdP->idPattern   : eIdP->id;
    std::string          type           = (eIdP->type.empty())? eIdP->typePattern : eIdP->type;

    EntityInfo* eP = new EntityInfo(id, type, isPattern, isTypePattern);

    cSubP->entityIdInfos.push_back(eP);
  }


  //
  // Insert the subscription in the cache
  //
  LM_T(LmtSubCache, ("Inserting NEW sub '%s', lastNotificationTime: %lu",
                     cSubP->subscriptionId, cSubP->lastNotificationTime));

  subCacheItemInsert(cSubP);
}



/* ****************************************************************************
*
* subCacheStatisticsGet -
*/
void subCacheStatisticsGet
(
  int*   refreshes,
  int*   inserts,
  int*   removes,
  int*   updates,
  int*   items,
  char*  list,
  int    listSize
)
{
  *refreshes = subCache.noOfRefreshes;
  *inserts   = subCache.noOfInserts;
  *removes   = subCache.noOfRemoves;
  *updates   = subCache.noOfUpdates;
  *items     = subCacheItems();

  CachedSubscription* cSubP = subCache.head;

  //
  // NOTE
  //   If the listBuffer is big enough to hold the entire list of cached subscriptions,
  //   subCacheStatisticsGet returns that list for the response of "GET /cache/statistics".
  //   Minimum size of the list is set to 128 bytes and the needed size is detected later.
  //   If the list-buffer is not big enough to hold the entire list, a warning text is showed instead.
  //
  //
  *list = 0;
  if (listSize > 128)
  {
    while (cSubP != NULL)
    {
      char          msg[256];
      unsigned int  bytesLeft = listSize - strlen(list);

#if 0
      //
      // FIXME P5: To be removed once sub-update is OK in QA
      //
      int now = getCurrentTime();
      snprintf(msg, sizeof(msg), "%s|N:%lu|E:%lu|T:%lu|DUR:%lu",
               cSubP->subscriptionId,
               cSubP->lastNotificationTime,
               cSubP->expirationTime,
               cSubP->throttling,
               cSubP->expirationTime - now);
#else
        snprintf(msg, sizeof(msg), "%s", cSubP->subscriptionId);
#endif
      //
      // If "msg" and ", " has no room in the "list", then no list is shown.
      // (the '+ 2' if for the string ", ")
      //
      if (strlen(msg) + 2 > bytesLeft)
      {
        snprintf(list, listSize, "too many subscriptions");
        break;
      }

      if (list[0] != 0)
        strcat(list, ", ");

      strcat(list, msg);

      cSubP = cSubP->next;
    }
  }
  else
  {
    snprintf(list, listSize, "too many subscriptions");
  }

  LM_T(LmtSubCache, ("Statistics: refreshes: %d (%d)", *refreshes, subCache.noOfRefreshes));
}



/* ****************************************************************************
*
* subCacheStatisticsReset -
*/
void subCacheStatisticsReset(const char* by)
{
  subCache.noOfRefreshes  = 0;
  subCache.noOfInserts    = 0;
  subCache.noOfRemoves    = 0;
  subCache.noOfUpdates    = 0;

  LM_T(LmtSubCache, ("Statistics reset by %s: refreshes is now %d", by, subCache.noOfRefreshes));
}


/* ****************************************************************************
*
* subCacheItemRemove -
*/
int subCacheItemRemove(CachedSubscription* cSubP)
{
  CachedSubscription* current = subCache.head;
  CachedSubscription* prev    = NULL;

  LM_T(LmtSubCache, ("in subCacheItemRemove, trying to remove '%s'", cSubP->subscriptionId));
  while (current != NULL)
  {
    if (current == cSubP)
    {
      // Removing first item ?
      if (cSubP == subCache.head)
      {
        subCache.head = cSubP->next;
      }

      // Removing last item?
      if (cSubP == subCache.tail)
      {
        subCache.tail = prev;
      }

      // Removing middle item?
      if (prev != NULL)
      {
        prev->next = cSubP->next;
      }

      LM_T(LmtSubCache, ("in subCacheItemRemove, REMOVING '%s'", cSubP->subscriptionId));
      ++subCache.noOfRemoves;

      subCacheItemDestroy(cSubP);
      delete cSubP;

      return 0;
    }

    prev    = current;
    current = current->next;
  }

  LM_E(("Runtime Error (item to remove from sub-cache not found)"));
  return -1;
}



/* ****************************************************************************
*
* subCacheRefresh -
*
* WARNING
*  The cache semaphore must be taken before this function is called:
*    cacheSemTake(__FUNCTION__, "Reason");
*  And released after subCacheRefresh finishes, of course.
*/
void subCacheRefresh(void)
{
  std::vector<std::string> databases;

  LM_T(LmtSubCache, ("Refreshing subscription cache"));

  // Empty the cache
  subCacheDestroy();

  // Get list of database
  if (mongoMultitenant())
  {
    getOrionDatabases(&databases);
  }

  // Add the 'default tenant'
  databases.push_back(getDbPrefix());


  // Now refresh the subCache for each and every tenant
  for (unsigned int ix = 0; ix < databases.size(); ++ix)
  {
    LM_T(LmtSubCache, ("DB %d: %s", ix, databases[ix].c_str()));
    mongoSubCacheRefresh(databases[ix]);
  }

  ++subCache.noOfRefreshes;
  LM_T(LmtSubCache, ("Refreshed subscription cache [%d]", subCache.noOfRefreshes));
}



/* ****************************************************************************
*
* CachedSubSaved -
*/
typedef struct CachedSubSaved
{
  int64_t      lastNotificationTime;
  int64_t      count;
  int64_t      failsCounter;
  int64_t      lastFailure;
  int64_t      lastSuccess;
  std::string  lastFailureReason;
  int64_t      lastSuccessCode;
  std::string  status;
  double       statusLastChange;
} CachedSubSaved;



/* ****************************************************************************
*
* subCacheSync -
*
* 1. Save subscriptionId, lastNotificationTime, count, failsCounter, lastFailure, and lastSuccess for all items in cache (savedSubV)
* 2. Refresh cache (count and failsCounter set to 0)
* 3. Compare lastNotificationTime/Failure/Success in savedSubV with the new cache-contents and:
*    3.1 Update cache-items where 'saved lastNotificationTime/Failure/Success' > 'cached lastNotificationTime/Failure/Success'
*    3.2 Remember this more correct lastNotificationTime/Failure/Success (must be flushed to mongo) -
*        by clearing out (set to -1) those lastNotificationTimes/Failure/Success that are newer in cache
* 4. Update 'count' and 'failsCounter' for each item in savedSubV where non-zero
* 5. Update 'lastNotificationTime/Failure/Success' for each item in savedSubV where non -1
* 6. Free the vector created in step 1 - savedSubV
*
* NOTE
*   This function runs in a separate thread and it allocates temporal objects (in savedSubV).
*   If the broker dies when this function is executing, all these temporal objects will be reported
*   as memory leaks.
*   We see this in our valgrind tests, where we force the broker to die.
*   This is of course not a real leak, we only see this as a leak as the function hasn't finished to
*   execute until the point where the temporal objects are deleted (See '6. Free the vector savedSubV').
*   To fix this little problem, we have created a variable 'subCacheState' that is set to ScsSynchronizing while
*   the sub-cache synchronization is working.
*   In serviceRoutines/exitTreat.cpp this variable is checked and if iot is set to ScsSynchronizing, then a
*   sleep for a few seconds is performed before the broker exits (this is only for DEBUG compilations).
*/
void subCacheSync(void)
{
  std::map<std::string, CachedSubSaved*> savedSubV;

  cacheSemTake(__FUNCTION__, "Synchronizing subscription cache");
  subCacheState = ScsSynchronizing;


  //
  // 1. Save subscriptionId, lastNotificationTime, count, failsCounter, lastFailure, and lastSuccess for all items in cache
  //
  CachedSubscription* cSubP = subCache.head;

  while (cSubP != NULL)
  {
    //
    // FIXME P7: For some reason, sometimes the same subscription is found twice in the cache (Issue 2216)
    //           Once the issue 2216 is fixed, this if-block must be removed.
    //
    if (savedSubV[cSubP->subscriptionId] != NULL)
    {
      cSubP = cSubP->next;
      continue;
    }

    CachedSubSaved* cssP       = new CachedSubSaved();

    cssP->lastNotificationTime = cSubP->lastNotificationTime;
    cssP->count                = cSubP->count;
    cssP->failsCounter         = cSubP->failsCounter;
    cssP->lastFailure          = cSubP->lastFailure;
    cssP->lastSuccess          = cSubP->lastSuccess;
    cssP->lastFailureReason    = cSubP->lastFailureReason;
    cssP->lastSuccessCode      = cSubP->lastSuccessCode;
    cssP->status               = cSubP->status;
    cssP->statusLastChange     = cSubP->statusLastChange;

    savedSubV[cSubP->subscriptionId] = cssP;
    cSubP = cSubP->next;
  }

  LM_T(LmtCacheSync, ("Pushed back %d items to savedSubV", savedSubV.size()));


  //
  // 2. Refresh cache (count and failsCounter set to 0)
  //
  subCacheRefresh();

  // 3. Check if DB or local cache is newer, updating DB in the second case
  cSubP = subCache.head;
  while (cSubP != NULL)
  {
    CachedSubSaved* cssP = savedSubV[cSubP->subscriptionId];

    // Not-NULL in some of this means "dirty" data to update in DB at the end
    // Note that count and failsCounter are special: they are update in DB
    // using $inc and not $set and they are flushed during the cache refresh process.
    // There aren't pointer for them
    int64_t*      lastNotificationTimeP = NULL;
    int64_t*      lastFailureP          = NULL;
    int64_t*      lastSuccessP          = NULL;
    std::string*  failureReasonP        = NULL;
    int64_t*      statusCodeP           = NULL;
    std::string*  statusP               = NULL;
    double*       statusLastChangeP     = NULL;

    if (cssP != NULL)
    {
      if (cssP->lastNotificationTime > cSubP->lastNotificationTime)
      {
        // cssP->lastNotificationTime is newer than what's currently in DB => update in cSubP and DB
        cSubP->lastNotificationTime = cssP->lastNotificationTime;
        lastNotificationTimeP = &cSubP->lastNotificationTime;
      }

      if (cssP->lastFailure > cSubP->lastFailure)
      {
        // cssP->lastFailure is newers than what's currently in DB => update in cSubP and DB
        cSubP->lastFailure       = cssP->lastFailure;
        cSubP->lastFailureReason = cssP->lastFailureReason;
        lastFailureP   = &cSubP->lastFailure;
        failureReasonP = &cSubP->lastFailureReason;
      }

      if (cssP->lastSuccess > cSubP->lastSuccess)
      {
        // cssP->lastSuccess is newer than what's currently in DB => update in cSubP and DB
        cSubP->lastSuccess     = cssP->lastSuccess;
        cSubP->lastSuccessCode = cssP->lastSuccessCode;
        lastSuccessP = &cSubP->lastSuccess;
        statusCodeP  = &cSubP->lastSuccessCode;
      }

      if (cssP->statusLastChange > cSubP->statusLastChange)
      {
        // cssP->status is newer than what's currently in DB => update in cSubP and DB
        cSubP->status           = cssP->status;
        cSubP->statusLastChange = cssP->statusLastChange;
        statusP           = &cSubP->status;
        statusLastChangeP = &cSubP->statusLastChange;
      }

      std::string tenant = (cSubP->tenant == NULL)? "" : cSubP->tenant;

      // Note that in step 2 subCacheRefresh() sets failsCounterFromDb from DB,
      // but that is done *before* updating the counter with mongoSubUpdateOnCacheSync()
      // Thus, we have to do a mirror increase in the cache
      cSubP->failsCounterFromDb += cssP->failsCounter;

      mongoSubUpdateOnCacheSync(tenant,
                                cSubP->subscriptionId,
                                cssP->count,
                                cssP->failsCounter,
                                lastNotificationTimeP,
                                lastFailureP,
                                lastSuccessP,
                                failureReasonP,
                                statusCodeP,
                                statusP,
                                statusLastChangeP);
    }

    cSubP = cSubP->next;
  }

  //
  // 5. Free the vector savedSubV
  //
  for (std::map<std::string, CachedSubSaved*>::iterator it = savedSubV.begin(); it != savedSubV.end(); ++it)
  {
    delete it->second;
  }
  savedSubV.clear();


  subCacheState = ScsIdle;
  cacheSemGive(__FUNCTION__, "Synchronizing subscription cache");
}



/* ****************************************************************************
*
* subCacheRefresherThread -
*/
static void* subCacheRefresherThread(void* vP)
{
  extern int subCacheInterval;

  while (1)
  {
    sleep(subCacheInterval);
    subCacheSync();
  }

  return NULL;
}



/* ****************************************************************************
*
* subCacheStart -
*/
void subCacheStart(void)
{
  pthread_t  tid;
  int        ret;

  // Populate subscription cache from database
  subCacheRefresh();

  ret = pthread_create(&tid, NULL, subCacheRefresherThread, NULL);

  if (ret != 0)
  {
    LM_E(("Runtime Error (error creating thread: %d)", ret));
    return;
  }
  pthread_detach(tid);
}



extern bool noCache;
/* ****************************************************************************
*
* subNotificationErrorStatus -
*
* This function marks a subscription to be erroneous, i.e. notifications are
* not working.
* A timestamp for this last failure is set for the sub-item in the sub-cache  and
* the consecutive number of notification errors for the subscription is incremented.
*
* If error == true, then the subscription is marked as non-erroneous.
*/
void subNotificationErrorStatus
(
  const std::string&  tenant,
  const std::string&  subscriptionId,
  bool                error,
  long long           statusCode,
  const std::string&  failureReason,
  long long           failsCounter,
  long long           maxFailsLimit
)
{
  bool maxFailsReached = maxFailsLimit > 0 && failsCounter >= maxFailsLimit;

  // FIXME P5: the noCache block is missplaced: subCache.cpp should contain only the
  // logic related with cache
  if (noCache)
  {
    // The field 'count' has already been taken care of (see processSubscriptions())
    // It is not used taken into account in mongoSubUpdateOnNotif()

    time_t now = time(NULL);

    if (error)
    {
      std::string status           = "";
      double      statusLastChange = -1;
      if (maxFailsReached)
      {
        LM_W(("Subscription %s automatically disabled due to failsCounter (%d) overpasses maxFailsLimit (%d)", subscriptionId.c_str(), failsCounter + 1, maxFailsLimit));
        status           = STATUS_INACTIVE;
        statusLastChange = getCurrentTime();
      }
      // fails == 1, lastSuccess == -1, statusCode == -1, status == "" | "inactive"
      mongoSubUpdateOnNotif(tenant, subscriptionId, 1, now, now, -1, failureReason, -1, status, statusLastChange);
    }
    else
    {
      // fails = 0, lastFailure == -1, failureReason == "", status == ""
      mongoSubUpdateOnNotif(tenant, subscriptionId, 0, now, -1, now, "", statusCode, "", -1);
    }

    return;
  }

  time_t now = time(NULL);

  cacheSemTake(__FUNCTION__, "Looking up an item for lastSuccess/Failure");

  CachedSubscription* subP = subCacheItemLookup(tenant.c_str(), subscriptionId.c_str());

  if (subP == NULL)
  {
    cacheSemGive(__FUNCTION__, "Looking up an item for lastSuccess/Failure");
    const char* errorString = "intent to update error status of non-existing subscription";

    alarmMgr.badInput(clientIp, errorString);
    return;
  }

  if (error)
  {
    subP->failsCounter++;
    subP->lastFailure       = now;
    subP->lastFailureReason = failureReason;

    if (maxFailsReached)
    {
      // update the status to inactive
      LM_W(("Subscription %s automatically disabled due to failsCounter (%d) overpasses maxFailsLimit (%d)", subscriptionId.c_str(), failsCounter + 1, maxFailsLimit));

      double now = getCurrentTime();
      subP->status = STATUS_INACTIVE;
      subP->statusLastChange = now;

      LM_T(LmtSubCache, ("Update the status to %s (lastChange: %d)", subP->status.c_str(), now));
    }
  }
  else
  {
    // no fails mean notification ok, thus reseting the counter
    // in addition, DB consolidated data is marked as invalid
    subP->failsCounter            = 0;
    subP->lastSuccess             = now;
    subP->lastSuccessCode         = statusCode;
    subP->failsCounterFromDbValid = false;
  }

  cacheSemGive(__FUNCTION__, "Looking up an item for lastSuccess/Failure");
}
