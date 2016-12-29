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
#include "common/limits.h"
#include "apiTypesV2/HttpInfo.h"
#include "apiTypesV2/Subscription.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoSubCache.h"
#include "ngsi10/SubscribeContextRequest.h"
#include "cache/subCache.h"
#include "alarmMgr/alarmMgr.h"

using std::map;



/* ****************************************************************************
*
* subCacheState - maintains the state of the subscription cache
*/
volatile SubCacheState subCacheState = ScsIdle;


//
// The subscription cache maintains in memory all subscriptions of type ONCHANGE.
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
    // FIXME P5: recomp error should be captured? have a look to other usages of regcomp()
    // in order to see how it works
    if (regcomp(&entityIdPattern, _entityId.c_str(), REG_EXTENDED) != 0)
    {
      alarmMgr.badInput(clientIp, "invalid regular expression for idPattern");
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
    // FIXME P5: recomp error should be captured? have a look to other usages of regcomp()
    // in order to see how it works
    if (regcomp(&entityTypePattern, _entityType.c_str(), REG_EXTENDED) != 0)
    {
      alarmMgr.badInput(clientIp, "invalid regular expression for typePattern");
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
    else if ((type != "")  && (entityType != "") && (entityType != type))
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
    regfree(&entityIdPattern);
    entityIdPatternToBeFreed = false;
  }

  if (entityTypePatternToBeFreed)
  {
    regfree(&entityTypePattern);
    entityTypePatternToBeFreed = false;
  }
}



/* ****************************************************************************
*
* EntityInfo::present -
*/
void EntityInfo::present(const std::string& prefix)
{
  LM_T(LmtPresent, ("%sid:        %s", prefix.c_str(), entityId.c_str()));
  LM_T(LmtPresent, ("%sisPattern: %s", prefix.c_str(), FT(isPattern)));
  LM_T(LmtPresent, ("%stype:      %s", prefix.c_str(), entityType.c_str()));
  LM_T(LmtPresent, ("%sisTypePattern: %s", prefix.c_str(), FT(isTypePattern)));
}



/* ****************************************************************************
*
* SubCache -
*/
class SubCache
{
 public:
  std::map<std::string, CachedSubscription*> list;

  SubCache();
  // Statistics counters
  int                 noOfRefreshes;
  int                 noOfInserts;
  int                 noOfRemoves;
  int                 noOfUpdates;
};



/* ****************************************************************************
*
* SubCache::SubCache - 
*/
SubCache::SubCache()
:
  noOfRefreshes(0),
  noOfInserts(0),
  noOfRemoves(0),
  noOfUpdates(0)
{
}



/* ****************************************************************************
*
* subCache -
*/
static SubCache  subCache;
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
  subCacheActive      = true;
  subCacheStatisticsReset("subCacheInit");
  subCache.list.clear();
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
  return subCache.list.size();
}



/* ****************************************************************************
*
* attributeMatch -
*/
static bool attributeMatch(CachedSubscription* cSubP, const std::vector<std::string>& attrV)
{
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
static bool servicePathMatch(CachedSubscription* cSubP, const std::string& servicePath)
{
  //
  // Special case. "/#" matches EVERYTHING
  //
  if (servicePath == "/#")
  {
    return true;
  }

  //
  // Empty service path?
  //
  if ((servicePath == "") && (cSubP->servicePath == ""))
  {
    return true;
  }


  //
  // Default service path for a subscription is "/".
  // So, if empty, set it to "/"
  //
  std::string  _servicePath  = (servicePath == "")? "/" : servicePath;
  const char*  spath         = cSubP->servicePath.c_str();

  //
  // No wildcard - exact match
  //
  if (spath[strlen(spath) - 1] != '#')
  {
    return (_servicePath == cSubP->servicePath);
  }


  //
  // Wildcard - remove '#' and compare to the length of _servicePath.
  //            If equal upto the length of _servicePath, then it is a match
  //
  // Actually, there is more than one way to match here:
  // If we have a service-path of the subscription as
  // "/a/b/#", then the following service-paths must match:
  //   1. /a/b
  //   2. /a/b/ AND /a/b/.+  (any path below /a/b/)
  //
  // What should NOT match is "/a/b2".
  //
  unsigned int len = strlen(spath) - 2;

  // 1. /a/b - (removing 2 chars from the cache-subscription removes "/#"

  if ((spath[len] == '/') && (_servicePath.size() == len) && (strncmp(spath, _servicePath.c_str(), len) == 0))
  {
    return true;
  }

  // 2. /a/b/.+
  len = strlen(spath) - 1;
  if (strncmp(spath, _servicePath.c_str(), len) == 0)
  {
    return true;
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
  const std::string&               tenant,
  const std::string&               servicePath,
  const std::string&               entityId,
  const std::string&               entityType,
  const std::vector<std::string>&  attrV
)
{
  if (cSubP == NULL)
  {
    return false;
  }


  //
  // Check whether to filter out due to tenant - only valid if the broker has started with the option '-multiservice'
  //
  if (subCacheMultitenant == true)
  {
    if ((cSubP->tenant == "") || (tenant == ""))
    {
      if (cSubP->tenant != "")
      {
        LM_T(LmtSubCacheMatch, ("No match due to tenant I"));
        return false;
      }

      if (tenant != "")
      {
        LM_T(LmtSubCacheMatch, ("No match due to tenant II"));
        return false;
      }
    }
    else if (cSubP->tenant != tenant)
    {
      LM_T(LmtSubCacheMatch, ("No match due to tenant III"));
      return false;
    }
  }

  if (servicePathMatch(cSubP, servicePath) == false)
  {
    LM_T(LmtSubCacheMatch, ("No match due to servicePath"));
    return false;
  }


  //
  // If ONCHANGE and one of the attribute names in the scope vector
  // of the subscription has the same name as the incoming attribute. there is a match.
  // Additionaly, if the attribute list in cSubP is empty, there is a match (this is the
  // case of ONANYCHANGE subscriptions).
  //
  if (!attributeMatch(cSubP, attrV))
  {
    LM_T(LmtSubCacheMatch, ("No match due to attributes"));
    return false;
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
  const std::string&                 tenant,
  const std::string&                 servicePath,
  const std::string&                 entityId,
  const std::string&                 entityType,
  const std::string&                 attr,
  std::vector<CachedSubscription*>*  subVecP
)
{
  std::map<std::string, CachedSubscription*>::iterator  it;

  for (it = subCache.list.begin(); it != subCache.list.end(); ++it)
  {
    CachedSubscription*       cSubP = it->second;
    std::vector<std::string>  attrV;

    attrV.push_back(attr);

    if (subMatch(cSubP, tenant, servicePath, entityId, entityType, attrV))
    {
      subVecP->push_back(cSubP);
      LM_T(LmtSubCache, ("added subscription '%s': lastNotificationTime: %lu",
                         cSubP->subscriptionId.c_str(), cSubP->lastNotificationTime));
    }
  }
}



/* ****************************************************************************
*
* subCacheMatch -
*/
void subCacheMatch
(
  const std::string&                 tenant,
  const std::string&                 servicePath,
  const std::string&                 entityId,
  const std::string&                 entityType,
  const std::vector<std::string>&    attrV,
  std::vector<CachedSubscription*>*  subVecP
)
{
  std::map<std::string, CachedSubscription*>::iterator  it;

  for (it = subCache.list.begin(); it != subCache.list.end(); ++it)
  {
    CachedSubscription*  cSubP = it->second;

    LM_W(("Calling subMatch with cSubP at %p", cSubP));
    if (subMatch(cSubP, tenant, servicePath, entityId, entityType, attrV))
    {
      subVecP->push_back(cSubP);
      LM_T(LmtSubCache, ("added subscription '%s': lastNotificationTime: %lu",
                         cSubP->subscriptionId.c_str(), cSubP->lastNotificationTime));
    }
  }
}



/* ****************************************************************************
*
* subCacheItemDestroy -
*/
void subCacheItemDestroy(CachedSubscription* cSubP)
{
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
}



/* ****************************************************************************
*
* subCacheDestroy -
*/
void subCacheDestroy(void)
{
  LM_T(LmtSubCache, ("destroying subscription cache"));

  std::map<std::string, CachedSubscription*>::iterator  it;
  for (it = subCache.list.begin(); it != subCache.list.end(); ++it)
  {
    CachedSubscription* csP = it->second;

    subCacheItemDestroy(csP);
    delete csP;
  }

  subCache.list.clear();
}



/* ****************************************************************************
*
* subCacheItemLookup -
*/
CachedSubscription* subCacheItemLookup(const std::string& tenant, const std::string& subscriptionId)
{
  CachedSubscription* cSubP = subCache.list[tenant + subscriptionId];
  return cSubP;
}



/* ****************************************************************************
*
* subCacheStatisticsIncrementUpdates -
*/
void subCacheStatisticsIncrementUpdates(void)
{
  ++subCache.noOfUpdates;
}



/* ****************************************************************************
*
* subCacheStatisticsIncrementRemoves -
*/
void subCacheStatisticsIncrementRemoves(void)
{
  ++subCache.noOfRemoves;
}



/* ****************************************************************************
*
* subCacheItemInsert -
*
* Note that this is the insert function that *really inserts* the
* CachedSubscription in the list of CachedSubscriptions.
*
* All other subCacheItemInsert functions create the subscription and then
* calls this function.
*
* So, the subscription itself is untouched by this function, is it ONLY inserted in the list.
*
*/
void subCacheItemInsert(CachedSubscription* cSubP)
{
  LM_T(LmtSubCache, ("inserting sub '%s', lastNotificationTime: %lu",
                     cSubP->subscriptionId.c_str(), cSubP->lastNotificationTime));

  ++subCache.noOfInserts;

  subCache.list[cSubP->tenant + cSubP->subscriptionId] = cSubP;
}



/* ****************************************************************************
*
* subCacheItemInsert - create a new sub, fill it in, and add it to cache
*
* Note that 'count', which is the counter of how many times a notification has been
* fired for a subscription is set to 0 or 1. It is set to 1 only if the subscription
* has made a notification to be triggered/fired upon creation-time of the subscription.
*/
void subCacheItemInsert
(
  const std::string&                 tenant,
  const std::string&                 servicePath,
  const ngsiv2::HttpInfo&            httpInfo,
  const std::vector<ngsiv2::EntID>&  entIdVector,
  const std::vector<std::string>&    attributes,
  const std::vector<std::string>&    metadata,
  const std::vector<std::string>&    conditionAttrs,
  const std::string&                 subscriptionId,
  int64_t                            expirationTime,
  int64_t                            throttling,
  RenderFormat                       renderFormat,
  bool                               notificationDone,
  int64_t                            lastNotificationTime,
  int64_t                            lastNotificationSuccessTime,
  int64_t                            lastNotificationFailureTime,
  StringFilter*                      stringFilterP,
  StringFilter*                      mdStringFilterP,
  const std::string&                 status,
  const std::string&                 q,
  const std::string&                 geometry,
  const std::string&                 coords,
  const std::string&                 georel,
  bool                               blacklist
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
  cSubP->tenant                = tenant;
  cSubP->servicePath           = servicePath;
  cSubP->subscriptionId        = subscriptionId;
  cSubP->expirationTime        = expirationTime;
  cSubP->throttling            = throttling;
  cSubP->lastNotificationTime  = lastNotificationTime;
  cSubP->lastFailure           = lastNotificationFailureTime;
  cSubP->lastSuccess           = lastNotificationSuccessTime;
  cSubP->renderFormat          = renderFormat;
  cSubP->count                 = (notificationDone == true)? 1 : 0;
  cSubP->status                = status;
  cSubP->expression.q          = q;
  cSubP->expression.geometry   = geometry;
  cSubP->expression.coords     = coords;
  cSubP->expression.georel     = georel;
  cSubP->blacklist             = blacklist;
  cSubP->httpInfo              = httpInfo;
  cSubP->notifyConditionV      = conditionAttrs;
  cSubP->attributes            = attributes;
  cSubP->metadata              = metadata;


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
    std::string          isPattern      = (eIdP->id   == "")? "true" : "false";
    bool                 isTypePattern  = (eIdP->type == "");
    std::string          id             = (eIdP->id   == "")? eIdP->idPattern   : eIdP->id;
    std::string          type           = (eIdP->type == "")? eIdP->typePattern : eIdP->type;

    EntityInfo* eP = new EntityInfo(id, type, isPattern, isTypePattern);

    cSubP->entityIdInfos.push_back(eP);
  }


  //
  // Insert the subscription in the cache
  //
  LM_T(LmtSubCache, ("Inserting NEW sub '%s', lastNotificationTime: %lu",
                     cSubP->subscriptionId.c_str(), cSubP->lastNotificationTime));

  subCacheItemInsert(cSubP);
}



/* ****************************************************************************
*
* subCacheStatisticsGet -
*/
void subCacheStatisticsGet
(
  int*          refreshes,
  int*          inserts,
  int*          removes,
  int*          updates,
  int*          items,
  std::string*  listP
)
{
  *refreshes = subCache.noOfRefreshes;
  *inserts   = subCache.noOfInserts;
  *removes   = subCache.noOfRemoves;
  *updates   = subCache.noOfUpdates;
  *items     = subCacheItems();
  *listP     = "";

  //
  // NOTE
  //   A maximum of MAX_SUBSCRIPTIONS_FOR_STATISTICS_LIST subscriptions will be shown
  //   in the list of subscription ids.
  //   If the sub-cache contains more subscriptions ... "too many subscriptions" is shown instead.
  //
  if (subCache.list.size() <= MAX_SUBSCRIPTIONS_FOR_STATISTICS_LIST)
  {
    std::map<std::string, CachedSubscription*>::iterator  it;

    for (it = subCache.list.begin(); it != subCache.list.end(); ++it)
    {
      CachedSubscription*  cSubP = it->second;

      if (*listP != "")
      {
        *listP += ", ";  // FIXME PR: This SPACE is here for historical reasons. Do we want it?
      }

      *listP += cSubP->subscriptionId;
    }
  }
  else
  {
    *listP = "too many subscriptions";
  }

  LM_T(LmtSubCache, ("Statistics: refreshes: %d (%d)", *refreshes, subCache.noOfRefreshes));
}



/* ****************************************************************************
*
* subCacheStatisticsReset -
*/
void subCacheStatisticsReset(const std::string& by)
{
  subCache.noOfRefreshes  = 0;
  subCache.noOfInserts    = 0;
  subCache.noOfRemoves    = 0;
  subCache.noOfUpdates    = 0;

  LM_T(LmtSubCache, ("Statistics reset by %s: refreshes is now %d", by.c_str(), subCache.noOfRefreshes));
}



/* ****************************************************************************
*
* subCacheEntryPresent -
*/
void subCacheEntryPresent(CachedSubscription* cSubP)
{
  if (cSubP == NULL)
  {
    return;
  }

  // FIXME P4: complete with the rest of fields in CachedSubscription

  std::string entityIdInfo;
  std::string attributes;
  std::string metadata;
  std::string notifyCondition;

  for (unsigned int ix = 0; ix < cSubP->entityIdInfos.size(); ++ix)
  {
    entityIdInfo +=  cSubP->entityIdInfos[ix]->entityId + "-" + cSubP->entityIdInfos[ix]->entityType;
    if (ix != cSubP->entityIdInfos.size() -1)
    {
      entityIdInfo += ",";
    }
  }

  for (unsigned int ix = 0; ix < cSubP->attributes.size(); ++ix)
  {
    attributes += cSubP->attributes[ix];
    if (ix != cSubP->attributes.size() -1)
    {
      attributes += ",";
    }
  }

  for (unsigned int ix = 0; ix < cSubP->metadata.size(); ++ix)
  {
    metadata += cSubP->metadata[ix];
    if (ix != cSubP->metadata.size() -1)
    {
      metadata += ",";
    }
  }

  for (unsigned int ix = 0; ix < cSubP->notifyConditionV.size(); ++ix)
  {
    notifyCondition += cSubP->notifyConditionV[ix];
    if (ix != cSubP->notifyConditionV.size() -1)
    {
      notifyCondition += ",";
    }
  }

  LM_T(LmtSubCache, ("o %s (tenant: %s, subservice: %s, entities: <%s>, attributes: <%s>, metadata: <%s>, "
                     "notifyCondition: <%s>, LNT: %lu, count: %lu, status: %s, expiration: %lu, THR: %d)",
                     cSubP->subscriptionId.c_str(),
                     cSubP->tenant.c_str(),
                     cSubP->servicePath.c_str(),
                     entityIdInfo.c_str(),
                     attributes.c_str(),
                     metadata.c_str(),
                     notifyCondition.c_str(),
                     cSubP->lastNotificationTime,
                     cSubP->count,
                     cSubP->status.c_str(),
                     cSubP->expirationTime,
                     cSubP->throttling));
}


/* ****************************************************************************
*
* subCachePresent -
*/
void subCachePresent(const std::string& title)
{
  LM_T(LmtSubCache, ("----------- %s ------------", title.c_str()));

  std::map<std::string, CachedSubscription*>::iterator  it;

  for (it = subCache.list.begin(); it != subCache.list.end(); ++it)
  {
    CachedSubscription*  cSubP = it->second;

    subCacheEntryPresent(cSubP);
  }

  LM_T(LmtSubCache, ("--------------------------------"));
}



/* ****************************************************************************
*
* subCacheItemRemove -
*/
int subCacheItemRemove(CachedSubscription* cSubP)
{
  std::map<std::string, CachedSubscription*>::iterator it = subCache.list.find(cSubP->tenant + cSubP->subscriptionId);

  CachedSubscription* csP = it->second;

  subCacheItemDestroy(csP);
  delete csP;
  ++subCache.noOfRemoves;
  subCache.list.erase(it);

  return 0;
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
    getOrionDatabases(databases);
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
  int64_t  lastNotificationTime;
  int64_t  count;
  int64_t  lastFailure;
  int64_t  lastSuccess;
} CachedSubSaved;



/* ****************************************************************************
*
* subCacheSync -
*
* 1. Save subscriptionId, lastNotificationTime, count, lastFailure, and lastSuccess for all items in cache (savedSubV)
* 2. Refresh cache (count set to 0)
* 3. Compare lastNotificationTime/lastFailure/lastSuccess in savedSubV with the new cache-contents and:
*    3.1 Update cache-items where 'saved lastNotificationTime' > 'cached lastNotificationTime'
*    3.2 Remember this more correct lastNotificationTime (must be flushed to mongo) -
*        by clearing out (set to 0) those lastNotificationTimes that are newer in cache
*    Same same with lastFailure and lastSuccess.
* 4. Update 'count' for each item in savedSubV where non-zero
* 5. Update 'lastNotificationTime/lastFailure/lastSuccess' for each item in savedSubV where non-zero
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
  std::map<std::string, CachedSubscription*>::iterator  it;
  std::map<std::string, CachedSubSaved*>                savedSubV;

  cacheSemTake(__FUNCTION__, "Synchronizing subscription cache");
  subCacheState = ScsSynchronizing;


  //
  // 1. Save subscriptionId, lastNotificationTime, count, lastFailure, and lastSuccess for all items in cache
  //
  for (it = subCache.list.begin(); it != subCache.list.end(); ++it)
  {
    CachedSubscription*  cSubP = it->second;

    //
    // FIXME P7: For some reason, sometimes the same subscription is found twice in the cache (Issue 2216)
    //           Once the issue 2216 is fixed, this if-block must be removed.
    //
    if (savedSubV[cSubP->tenant + cSubP->subscriptionId] != NULL)
    {
      continue;
    }

    CachedSubSaved* cssP       = new CachedSubSaved();

    cssP->lastNotificationTime = cSubP->lastNotificationTime;
    cssP->count                = cSubP->count;
    cssP->lastFailure          = cSubP->lastFailure;
    cssP->lastSuccess          = cSubP->lastSuccess;

    savedSubV[cSubP->tenant + cSubP->subscriptionId] = cssP;
  }

  LM_T(LmtCacheSync, ("Pushed back %d items to savedSubV", savedSubV.size()));


  //
  // 2. Refresh cache (count set to 0)
  //
  subCacheRefresh();


  //
  // 3. Compare lastNotificationTime/lastFailure/lastSuccess in savedSubV with the new cache-contents
  //
  for (it = subCache.list.begin(); it != subCache.list.end(); ++it)
  {
    CachedSubscription* cSubP = it->second;
    CachedSubSaved*     cssP  = savedSubV[cSubP->tenant + cSubP->subscriptionId];

    if (cssP != NULL)
    {
      if (cssP->lastNotificationTime <= cSubP->lastNotificationTime)
      {
        // cssP->lastNotificationTime is older than what's currently in DB => throw away
        cssP->lastNotificationTime = 0;
      }

      if (cssP->lastFailure < cSubP->lastFailure)
      {
        // cssP->lastFailure is older than what's currently in DB => throw away
        cssP->lastFailure = 0;
      }

      if (cssP->lastSuccess < cSubP->lastSuccess)
      {
        // cssP->lastSuccess is older than what's currently in DB => throw away
        cssP->lastSuccess = 0;
      }
    }
  }


  //
  // 4. Update 'count' for each item in savedSubV where non-zero
  // 5. Update 'lastNotificationTime/lastFailure/lastSuccess' for each item in savedSubV where non-zero
  //
  for (it = subCache.list.begin(); it != subCache.list.end(); ++it)
  {
    CachedSubscription* cSubP = it->second;
    CachedSubSaved*     cssP  = savedSubV[cSubP->tenant + cSubP->subscriptionId];

    if (cssP != NULL)
    {
      std::string tenant = (cSubP->tenant == NULL)? "" : cSubP->tenant;

      mongoSubCountersUpdate(tenant,
                             cSubP->subscriptionId.c_str(),
                             cssP->count,
                             cssP->lastNotificationTime,
                             cssP->lastFailure,
                             cssP->lastSuccess);

      // Keeping lastFailure and lastSuccess in sub cache
      cSubP->lastFailure = cssP->lastFailure;
      cSubP->lastSuccess = cssP->lastSuccess;
    }
  }


  //
  // 6. Free the vector savedSubV
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
* subCacheItemNotificationErrorStatus - 
*
* This function marks a subscription to be erroneous, i.e. notifications are
* not working.
* A timestamp for this last failure is set for the sub-item in the sub-cache  and
* the consecutive number of notification errors for the subscription is incremented.
*
* If 'errors' == 0, then the subscription is marked as non-erroneous.
*/
void subCacheItemNotificationErrorStatus(const std::string& tenant, const std::string& subscriptionId, int errors)
{
  if (noCache)
  {
    // The field 'count' has already been taken care of. Set to 0 in the calls to mongoSubCountersUpdate()

    time_t now = time(NULL);

    if (errors == 0)
    {
      mongoSubCountersUpdate(tenant, subscriptionId, 0, now, -1, now);  // lastFailure == -1
    }
    else
    {
      mongoSubCountersUpdate(tenant, subscriptionId, 0, now, now, -1);  // lastSuccess == -1, count == 0
    }

    return;
  }

  time_t now = time(NULL);

  cacheSemTake(__FUNCTION__, "Looking up an item for lastSuccess/Failure");

  CachedSubscription* subP = subCacheItemLookup(tenant, subscriptionId);

  if (subP == NULL)
  {
    cacheSemGive(__FUNCTION__, "Looking up an item for lastSuccess/Failure");

    alarmMgr.badInput(clientIp, "intent to update error status of non-existing subscription");

    return;
  }

  if (errors == 0)
  {
    subP->lastSuccess  = now;
  }
  else
  {
    subP->lastFailure  = now;
  }

  cacheSemGive(__FUNCTION__, "Looking up an item for lastSuccess/Failure");
}
