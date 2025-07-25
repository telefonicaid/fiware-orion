#ifndef SRC_LIB_MONGOBACKEND_MONGOGLOBAL_H_
#define SRC_LIB_MONGOBACKEND_MONGOGLOBAL_H_

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
#include <stdint.h>   // int64_t et al

#include <string>
#include <vector>
#include <map>

#include "logMsg/logMsg.h"

#include "common/RenderFormat.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/StringList.h"
#include "ngsi/ContextElementResponseVector.h"
#include "ngsi/UpdateContextResponse.h"
#include "ngsiNotify/Notifier.h"
#include "rest/uriParamNames.h"
#include "apiTypesV2/Subscription.h"
#include "apiTypesV2/HttpInfo.h"
#include "apiTypesV2/Registration.h"
#include "mongoBackend/TriggeredSubscription.h"

#include "mongoDriver/BSONArray.h"


/* ****************************************************************************
*
* mongoMultitenant -
*/
extern bool mongoMultitenant(void);



/* ****************************************************************************
*
* mongoInit -
*/
void mongoInit
(
  const char*  dbURI,
  std::string  dbName,
  const char*  pwd,
  bool         mtenant,
  int          writeConcern,
  int          dbPoolSize,
  bool         mutexTimeStat
);



/* ****************************************************************************
*
* getNotifier -
*/
extern Notifier* getNotifier(void);



/* ****************************************************************************
*
* setNotifier -
*/
extern void setNotifier(Notifier* n);



/* ****************************************************************************
*
* setDbPrefix -
*/
extern void setDbPrefix(const std::string& dbPrefix);



/* ****************************************************************************
*
* getDbPrefix -
*/
extern const std::string& getDbPrefix(void);



/* ****************************************************************************
*
* getOrionDatabases -
*
* Return the list of Orion databases (the ones that start with the dbPrefix + "_").
* Note that the DB belonging to the default service is not included in the
* returned list
*
* Function return value is false in the case of problems accessing database,
* true otherwise.
*/
extern bool getOrionDatabases(std::vector<std::string>* dbs);



/* ****************************************************************************
*
* tenantFromDb -
*/
extern std::string tenantFromDb(const std::string& database);



/* ****************************************************************************
*
* composeDatabaseName -
*
*/
extern std::string composeDatabaseName(const std::string& tenant);



/* ****************************************************************************
*
* mongoLocationCapable -
*/
extern bool mongoLocationCapable(void);



/* ****************************************************************************
*
* mongoExpirationCapable -
*/
extern bool mongoExpirationCapable(void);



/* ****************************************************************************
*
* ensureLocationIndex -
*/
extern void ensureLocationIndex(const std::string& tenant);



/* ****************************************************************************
*
* ensureDateExpirationIndex -
*/
extern void ensureDateExpirationIndex(const std::string& tenant);



/* ****************************************************************************
*
* matchEntity -
*/
extern bool matchEntity(const EntityId* en1, const EntityId& en2);



/* ****************************************************************************
*
* includedEntity -
*/
extern bool includedEntity(EntityId en, const EntityIdVector& entityIdV);



/* ****************************************************************************
*
* includedAttribute -
*/
extern bool includedAttribute(const std::string& attrName, const StringList& attrsV);



/* *****************************************************************************
*
* processAreaScope -
*
*/
extern bool processAreaScope(const Scope* scoP, orion::BSONObjBuilder* queryP, orion::BSONObjBuilder* countQueryP);



/* ****************************************************************************
*
* entitiesQuery -
*/
extern bool entitiesQuery
(
  const EntityIdVector&            enV,
  const StringList&                attrL,
  const ScopeVector&               spV,
  ContextElementResponseVector*    cerV,
  OrionError*                      oeP,
  const std::string&               tenant,
  const std::vector<std::string>&  servicePath,
  const std::vector<std::string>&  sortOrderList,
  int                              offset         = DEFAULT_PAGINATION_OFFSET_INT,
  int                              limit          = DEFAULT_PAGINATION_LIMIT_INT,
  bool*                            limitReached   = NULL,
  long long*                       countP         = NULL
);



/* ****************************************************************************
*
* pruneContextElements -
*/
extern void pruneContextElements
(
  const StringList&                    attrsV,
  const ContextElementResponseVector&  oldCerV,
  ContextElementResponseVector*        newCerVP
);



/* ****************************************************************************
*
* registrationsQuery -
*/
extern bool registrationsQuery
(
  const EntityIdVector&               enV,
  const StringList&                   attrL,
  const ngsiv2::ForwardingMode        forwardingMode,
  std::vector<ngsiv2::Registration>*  regV,
  std::string*                        err,
  const std::string&                  tenant,
  const std::vector<std::string>&     servicePathV
);



/* ****************************************************************************
*
* condValueAttrMatch -
*/
extern bool condValueAttrMatch(const orion::BSONObj& sub, const std::vector<std::string>& modifiedAttrs);



/* ****************************************************************************
*
* subToNotifyList -
*/
void subToNotifyList
(
  const std::vector<std::string>&  notificationVector,
  const std::vector<std::string>&  entityAttrsVector,
  StringList&                      attrL,
  const bool&                      blacklist
);



/* ****************************************************************************
*
* subToAttributeList -
*
* Extract the attribute list from a BSON document (in the format of the csubs collection)
*/
extern StringList subToAttributeList
(
  const orion::BSONObj&           attrL,
  const bool&                     blacklist,
  const std::vector<std::string>  attributes
);



/* ****************************************************************************
*
* subToAttributeList -
*
* Extract the attribute list from a BSON document (in the format of the csubs collection)
*/
extern StringList subToAttributeList(const orion::BSONObj& attrL);



/* ****************************************************************************
*
* slashEscape - 
*
* When the 'to' buffer is full, slashEscape returns.
* No warnings, no nothing.
* Make sure 'to' is big enough!
*/
extern void slashEscape(const char* from, char* to, unsigned int toLen);



/* ****************************************************************************
*
* releaseTriggeredSubscriptions -
*/
extern void releaseTriggeredSubscriptions(std::map<std::string, TriggeredSubscription*>* subsP);



/* ****************************************************************************
*
* servicePathFilterNeeded -
*
*/
bool servicePathFilterNeeded(const std::vector<std::string>& servicePath);



/* ****************************************************************************
*
* fillQueryServicePath -
*/
extern orion::BSONObj fillQueryServicePath(const std::string& spKey, const std::vector<std::string>& servicePath);



/* ****************************************************************************
*
* fillContextProviders -
*/
extern void fillContextProviders(ContextElementResponse* cer, const std::vector<ngsiv2::Registration>& crrV);



/* ****************************************************************************
*
* someContextElementNotFound -
*/
extern bool someContextElementNotFound(const ContextElementResponse& cer);



/* ****************************************************************************
*
* cprLookupByAttribute -
*/
extern void cprLookupByAttribute
(
  const Entity&                            en,
  const std::string&                       attrName,
  const std::vector<ngsiv2::Registration>& regV,
  std::string*                             perEntProviderP,
  std::string*                             perAttrProviderP,
  bool*                                    legacyProviderFormatP,
  std::string*                             regId
);


/* ****************************************************************************
*
* addBuiltins -
*
*/
extern void addBuiltins(ContextElementResponse* cerP, const std::string& alterationType);

#endif  // SRC_LIB_MONGOBACKEND_MONGOGLOBAL_H_
