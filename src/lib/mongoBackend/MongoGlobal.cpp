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
#include <semaphore.h>
#include <regex.h>

#include <string>
#include <vector>
#include <map>
#include <set>

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/limits.h"
#include "common/globals.h"
#include "common/sem.h"
#include "common/string.h"
#include "common/wsStrip.h"
#include "common/statistics.h"
#include "common/RenderFormat.h"
#include "alarmMgr/alarmMgr.h"

#include "orionTypes/OrionValueType.h"
#include "apiTypesV2/HttpInfo.h"

#include "ngsi/EntityIdVector.h"
#include "ngsi/StringList.h"
#include "ngsi/ContextElementResponseVector.h"
#include "ngsi/Duration.h"
#include "ngsi/Restriction.h"
#include "ngsiNotify/Notifier.h"
#include "rest/StringFilter.h"
#include "apiTypesV2/Subscription.h"
#include "apiTypesV2/ngsiWrappers.h"

#include "mongoBackend/mongoConnectionPool.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/compoundResponses.h"
#include "mongoBackend/MongoGlobal.h"



/* ****************************************************************************
*
* USING
*/
using mongo::DBClientBase;
using mongo::DBClientCursor;
using mongo::BSONObj;
using mongo::BSONElement;
using mongo::BSONArrayBuilder;
using mongo::BSONObjBuilder;
using mongo::Query;
using mongo::AssertionException;
using mongo::BSONArray;
using mongo::OID;
using ngsiv2::HttpInfo;
using ngsiv2::EntID;



/* ****************************************************************************
*
* Globals
*/
static std::string          dbPrefix;
static std::string          entitiesCollectionName;
static std::string          registrationsCollectionName;
static std::string          subscribeContextCollectionName;
static std::string          subscribeContextAvailabilityCollectionName;
static Notifier*            notifier;
static bool                 multitenant;



/* ****************************************************************************
*
* DelayedRelease -
*
* This structure is used simply to hold the vector of 'ContextElementResponse'
* for the delaying of release() of these structures.
* This was invented to overcome the fact that 'thread_local' is not supported by the
* older compiler in Centos 6. Initially, the following construct was used:
*
*  thread_local std::vector<ContextElementResponse*>  cerVector;
*
* And this works just fine in Ubuntu 17.04, but it fails to compile in CentOS 6.
*
* For more info about this, see github issue #2994
*/
typedef struct DelayedRelease
{
  std::vector<ContextElementResponse*>  cerVector;
} DelayedRelease;

static __thread DelayedRelease* delayedReleaseP = NULL;



/* ****************************************************************************
*
* WORKAROUND_2994 - see github issue #2994
*/
#define WORKAROUND_2994 1




#ifdef WORKAROUND_2994
/* ****************************************************************************
*
* delayedReleaseAdd -
*/
static void delayedReleaseAdd(const ContextElementResponseVector& cerV)
{
  if (delayedReleaseP == NULL)
  {
    delayedReleaseP = new DelayedRelease();
  }

  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    delayedReleaseP->cerVector.push_back(cerV[ix]);
  }
}
#endif



/* ****************************************************************************
*
* delayedReleaseExecute -
*
* NOTE
*   This function doesn't depend on WORKAROUND_2994 being defined, as delayedReleaseP
*   will be NULL if WORKAROUND_2994 is not defined and its action is null and void, if so.
*   'delayedReleaseExecute()' is called in rest/rest.cpp and with this 'idea', that file doesn't
*   need to know about the WORKAROUND_2994 definition.
*/
void delayedReleaseExecute(void)
{
  if (delayedReleaseP == NULL)
  {
    return;
  }

  for (unsigned int ix = 0; ix < delayedReleaseP->cerVector.size(); ++ix)
  {
    delayedReleaseP->cerVector[ix]->release();
    delete delayedReleaseP->cerVector[ix];
  }

  delayedReleaseP->cerVector.clear();
  delete delayedReleaseP;
  delayedReleaseP = NULL;
}



/* ****************************************************************************
*
* mongoMultitenant -
*/
bool mongoMultitenant(void)
{
  return multitenant;
}



/* ****************************************************************************
*
* mongoInit -
*/
void mongoInit
(
  const char*  dbHost,
  const char*  rplSet,
  std::string  dbName,
  const char*  user,
  const char*  pwd,
  const char*  mechanism,
  const char*  authDb,
  bool         dbSSL,
  bool         mtenant,
  int64_t      timeout,
  int          writeConcern,
  int          dbPoolSize,
  bool         mutexTimeStat
)
{
  // Set the global multitenant variable
  multitenant = mtenant;

  double tmo = timeout / 1000.0;  // milliseconds to float value in seconds

  if (mongoConnectionPoolInit(dbHost,
                              dbName.c_str(),
                              rplSet,
                              user,
                              pwd,
                              mechanism,
                              authDb,
                              dbSSL,
                              mtenant,
                              tmo,
                              writeConcern,
                              dbPoolSize,
                              mutexTimeStat) != 0)
  {
    LM_X(1, ("Fatal Error (MongoDB error)"));
  }

  if (user[0] != 0)
  {
    LM_I(("Connected to mongo at %s:%s as user '%s'", dbHost, dbName.c_str(), user));
  }
  else
  {
    LM_I(("Connected to mongo at %s:%s", dbHost, dbName.c_str()));
  }

  setDbPrefix(dbName);
  setEntitiesCollectionName(COL_ENTITIES);
  setRegistrationsCollectionName(COL_REGISTRATIONS);
  setSubscribeContextCollectionName(COL_CSUBS);
  setSubscribeContextAvailabilityCollectionName(COL_CASUBS);

  //
  // Note that index creation operation is idempotent.
  // From http://docs.mongodb.org/manual/reference/method/db.collection.ensureIndex:
  // "If you call multiple ensureIndex() methods with the same index specification at the same time,
  // only the first operation will succeed, all other operations will have no effect."
  //
  ensureLocationIndex("");
  ensureDateExpirationIndex("");
  if (mtenant)
  {
    /* We get tenant database names and apply ensure the location and date expiration indexes in each one */
    std::vector<std::string> orionDbs;

    getOrionDatabases(&orionDbs);

    for (unsigned int ix = 0; ix < orionDbs.size(); ++ix)
    {
      std::string orionDb = orionDbs[ix];
      std::string tenant = orionDb.substr(dbName.length() + 1);   // + 1 for the "_" in "orion_tenantA"
      ensureLocationIndex(tenant);
      ensureDateExpirationIndex(tenant);
    }
  }
}



#ifdef UNIT_TEST

static DBClientBase* connection = NULL;



/* ****************************************************************************
*
* setMongoConnectionForUnitTest -
*
* For unit tests there is only one connection. This connection is stored right here (DBClientBase* connection) and
* given out using the function getMongoConnection().
*/
void setMongoConnectionForUnitTest(DBClientBase* _connection)
{
  connection = _connection;
}



/* ****************************************************************************
*
* mongoInitialConnectionGetForUnitTest -
*
* This function is meant to be used by unit tests, to get a connection from the pool
* and then use that connection, setting it with the function 'setMongoConnectionForUnitTest'.
* This will set the static variable 'connection' in MongoGlobal.cpp and later 'getMongoConnection'
* returns that variable (getMongoConnection is used by the entire mongo backend).
*/
DBClientBase* mongoInitialConnectionGetForUnitTest(void)
{
  return mongoPoolConnectionGet();
}
#endif



/* ****************************************************************************
*
* getMongoConnection -
*
* I would prefer to have per-collection methods, to have a better encapsulation, but
* the Mongo C++ API doesn't seem to work that way
*/
DBClientBase* getMongoConnection(void)
{
#ifdef UNIT_TEST
  return connection;
#else
  return mongoPoolConnectionGet();
#endif
}



/* ****************************************************************************
*
* releaseMongoConnection - give back mongo connection to connection pool
*
* Older versions of this function planned to use a std::auto_ptr<DBClientCursor>* parameter
* in order to invoke kill() on it for a "safer" connection releasing. However, at the end
* it seems that kill() will not help at all (see deetails in https://github.com/telefonicaid/fiware-orion/issues/1568)
* and after some testing we have checked that the current solution is stable.
*/
void releaseMongoConnection(DBClientBase* connection)
{
#ifdef UNIT_TEST
  return;
#else
  return mongoPoolConnectionRelease(connection);
#endif  // UNIT_TEST
}



/* ***************************************************************************
*
* getNotifier -
*/
Notifier* getNotifier()
{
  return notifier;
}



/* ***************************************************************************
*
* setNotifier -
*/
void setNotifier(Notifier* n)
{
  notifier = n;
}



/* ***************************************************************************
*
* setDbPrefix -
*/
void setDbPrefix(const std::string& _dbPrefix)
{
  dbPrefix = _dbPrefix;
  LM_T(LmtBug, ("Set dbPrefix to '%s'", dbPrefix.c_str()));
}



/* ***************************************************************************
*
* getDbPrefix -
*/
const std::string& getDbPrefix(void)
{
  return dbPrefix;
}



/* ***************************************************************************
*
* getOrionDatabases -
*/
bool getOrionDatabases(std::vector<std::string>* dbsP)
{
  BSONObj       result;
  std::string   err;

  if (!runCollectionCommand("admin", BSON("listDatabases" << 1), &result, &err))
  {
    return false;
  }

  if (!result.hasField("databases"))
  {
    LM_E(("Runtime Error (no 'databases' field in %s)", result.toString().c_str()));
    return false;
  }

  std::vector<BSONElement> databases = getFieldF(result, "databases").Array();
  for (std::vector<BSONElement>::iterator i = databases.begin(); i != databases.end(); ++i)
  {
    BSONObj      db      = (*i).Obj();
    std::string  dbName  = getStringFieldF(db, "name");
    std::string  prefix  = dbPrefix + "-";

    if (strncmp(prefix.c_str(), dbName.c_str(), strlen(prefix.c_str())) == 0)
    {
      LM_T(LmtMongo, ("Orion database found: %s", dbName.c_str()));
      dbsP->push_back(dbName);
      LM_T(LmtBug, ("Pushed back db name '%s'", dbName.c_str()));
    }
  }

  return true;
}



/* ***************************************************************************
*
* tenantFromDb -
*
* Given a database name as an argument (e.g. orion-myservice1) it returns the
* corresponding tenant name as result (myservice1) or "" if the string doesn't
* start with the database prefix
*/
std::string tenantFromDb(const std::string& database)
{
  std::string r;
  std::string prefix  = dbPrefix + "-";

  if (strncmp(prefix.c_str(), database.c_str(), strlen(prefix.c_str())) == 0)
  {
    char tenant[SERVICE_NAME_MAX_LEN];

    strncpy(tenant, database.c_str() + strlen(prefix.c_str()), sizeof(tenant));
    r = std::string(tenant);
  }
  else
  {
    r = "";
  }

  LM_T(LmtMongo, ("DB -> tenant: <%s> -> <%s>", database.c_str(), r.c_str()));
  return r;
}



/* ***************************************************************************
*
* setEntitiesCollectionName -
*/
void setEntitiesCollectionName(const std::string& name)
{
  entitiesCollectionName = name;
}



/* ***************************************************************************
*
* setRegistrationsCollectionName -
*/
void setRegistrationsCollectionName(const std::string& name)
{
  registrationsCollectionName = name;
}



/* ***************************************************************************
*
* setSubscribeContextCollectionName -
*/
void setSubscribeContextCollectionName(const std::string& name)
{
  subscribeContextCollectionName = name;
}



/* ***************************************************************************
*
* setSubscribeContextAvailabilityCollectionName -
*/
void setSubscribeContextAvailabilityCollectionName(const std::string& name)
{
  subscribeContextAvailabilityCollectionName = name;
}



/* ***************************************************************************
*
* composeCollectionName -
*
* Common helper function for composing collection names
*/
static std::string composeCollectionName(std::string tenant, std::string colName)
{
  return composeDatabaseName(tenant) + "." + colName;
}



/* ***************************************************************************
*
* composeDatabaseName -
*
* Common helper function for composing database names
*/
std::string composeDatabaseName(const std::string& tenant)
{
  std::string result;

  if (!multitenant || (tenant == ""))
  {
    result = dbPrefix;
  }
  else
  {
    /* Note that we can not use "." as database delimiter. A database cannot contain this
     * character, http://docs.mongodb.org/manual/reference/limits/#Restrictions-on-Database-Names-for-Unix-and-Linux-Systems */
    result = dbPrefix + "-" + tenant;
  }

  LM_T(LmtBug, ("database name composed: '%s'", result.c_str()));
  return result;
}



/* ***************************************************************************
*
* getEntitiesCollectionName -
*/
std::string getEntitiesCollectionName(const std::string& tenant)
{
  return composeCollectionName(tenant, entitiesCollectionName);
}



/* ***************************************************************************
*
* getRegistrationsCollectionName -
*/
std::string getRegistrationsCollectionName(const std::string& tenant)
{
  return composeCollectionName(tenant, registrationsCollectionName);
}



/* ***************************************************************************
*
* getSubscribeContextCollectionName -
*/
std::string getSubscribeContextCollectionName(const std::string& tenant)
{
  return composeCollectionName(tenant, subscribeContextCollectionName);
}



/* ***************************************************************************
*
* getSubscribeContextAvailabilityCollectionName -
*/
std::string getSubscribeContextAvailabilityCollectionName(const std::string& tenant)
{
  return composeCollectionName(tenant, subscribeContextAvailabilityCollectionName);
}



/* ***************************************************************************
*
* mongoLocationCapable -
*/
bool mongoLocationCapable(void)
{
  int mayor;
  int minor;

  /* Geo location based on 2dsphere indexes was introduced in MongoDB 2.4 */
  mongoVersionGet(&mayor, &minor);

  return ((mayor == 2) && (minor >= 4)) || (mayor > 2);
}



/* ***************************************************************************
*
* mongoExpirationCapable -
*/
bool mongoExpirationCapable(void)
{
  int mayor;
  int minor;

  /* TTL (Time To Live) indexes was introduced in MongoDB 2.2,
   *  although the expireAfterSeconds: 0 usage is not shown in documentation until 2.4 */
  mongoVersionGet(&mayor, &minor);

  return ((mayor == 2) && (minor >= 4)) || (mayor > 2);
}



/* ***************************************************************************
*
* ensureLocationIndex -
*/
void ensureLocationIndex(const std::string& tenant)
{
  /* Ensure index for entity locations, in the case of using 2.4 */
  if (mongoLocationCapable())
  {
    std::string index = ENT_LOCATION "." ENT_LOCATION_COORDS;
    std::string err;

    collectionCreateIndex(getEntitiesCollectionName(tenant), BSON(index << "2dsphere"), false, &err);
    LM_T(LmtMongo, ("ensuring 2dsphere index on %s (tenant %s)", index.c_str(), tenant.c_str()));
  }
}



/* ****************************************************************************
 *
 * ensureDateExpirationIndex -
 */
void ensureDateExpirationIndex(const std::string& tenant)
{
  /* Ensure index for entity expiration, in the case of using 2.4 */
  if (mongoExpirationCapable())
  {
    std::string index = ENT_EXPIRATION;
    std::string err;

    collectionCreateIndex(getEntitiesCollectionName(tenant), BSON(index << 1), true, &err);
    LM_T(LmtMongo, ("ensuring TTL date expiration index on %s (tenant %s)", index.c_str(), tenant.c_str()));
  }
}
/* ****************************************************************************
*
* matchEntity -
*
* Taking into account null types and id patterns. Note this function is not
* commutative: en1 is interpreted as the entity to match *in* en2 (i.e.
* it is assumed that the pattern is in en2)
*/
bool matchEntity(const EntityId* en1, const EntityId* en2)
{
  bool idMatch;

  if (isTrue(en2->isPattern))
  {
    regex_t regex;

    idMatch = false;
    if (regcomp(&regex, en2->id.c_str(), REG_EXTENDED) != 0)
    {
      std::string details = std::string("error compiling regex for id: '") + en2->id + "'";
      alarmMgr.badInput(clientIp, details);
    }
    else
    {
      idMatch = (regexec(&regex, en1->id.c_str(), 0, NULL, 0) == 0);

      regfree(&regex);  // If regcomp fails it frees up itself (see glibc sources for details)
    }
  }
  else  /* isPattern == false */
  {
    idMatch = (en2->id == en1->id);
  }

  // Note that type == "" is like a * wildcard for type
  return idMatch && (en1->type == "" || en2->type == "" || en2->type == en1->type);
}



/* ****************************************************************************
*
* includedEntity -
*/
bool includedEntity(EntityId en, const EntityIdVector& entityIdV)
{
  for (unsigned int ix = 0; ix < entityIdV.size(); ++ix)
  {
    if (en.isPatternIsTrue() && en.id == ".*")
    {
      // By the moment the only supported pattern is .*. In this case matching is
      // based exclusively in type
      if (en.type == entityIdV[ix]->type)
      {
        return true;
      }
    }
    else if (matchEntity(&en, entityIdV[ix]))
    {
      return true;
    }
  }
  return false;
}



/* ****************************************************************************
*
* includedAttribute -
*/
bool includedAttribute(const std::string& attrName, const StringList& attrsV)
{
  //
  // attrsV.size() == 0 is the case in which the query request doesn't include attributes,
  // so all the attributes are included in the response
  //
  if ((attrsV.size() == 0) || attrsV.lookup(ALL_ATTRS))
  {
    return true;
  }

  for (unsigned int ix = 0; ix < attrsV.size(); ++ix)
  {
    if (attrsV[ix] == attrName)
    {
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* fillQueryEntity -
*/
static void fillQueryEntity(BSONObjBuilder* bobP, const EntityId* enP)
{
  const std::string  idString    = "_id." ENT_ENTITY_ID;
  const std::string  typeString  = "_id." ENT_ENTITY_TYPE;

  if (enP->isPattern == "true")
  {
    // In the case of "universal pattern" we can avoid adding anything (simpler query)
    if (enP->id != ".*")
    {
      bobP->appendRegex(idString, enP->id);
    }
  }
  else
  {
    bobP->append(idString, enP->id);
  }

  if (enP->type != "")
  {
    if (enP->isTypePattern)
    {
      // In the case of "universal pattern" we can avoid adding anything (simpler query)
      if (enP->type != ".*")
      {
        bobP->appendRegex(typeString, enP->type);
      }
    }
    else
    {
      bobP->append(typeString, enP->type);
    }
  }
}



/* ****************************************************************************
*
* servicePathFilterNeeded -
*
*/
bool servicePathFilterNeeded(const std::vector<std::string>& servicePath)
{
  // Note that by construction servicePath vector must have at least one element. Note that the
  // case in which first element is "" is special, it means that the SP were not provided and
  // we have to apply the default
  if (servicePath[0] == "")
  {
    return false;
  }

  // If some of the elements is /# then it means that all service paths have to be taken into account
  // so no filter has to be used
  for (unsigned int ix = 0 ; ix < servicePath.size(); ++ix)
  {
    if (servicePath[ix] == "/#")
    {
      return false;
    }
  }

  return true;
}



/* ****************************************************************************
*
* addServicePathInTokens -
*
*/
static void addServicePathInTokens(BSONArrayBuilder* servicePathIn, const std::string& sp)
{
  char escapedPath[SERVICE_PATH_MAX_TOTAL * 2];
  slashEscape(sp.c_str(), escapedPath, sizeof(escapedPath));

  /* Remove '/#' and '\/#' trailing part of the string */
  std::string path = sp.substr(0, sp.length() - 2);
  escapedPath[strlen(escapedPath) - 3] = 0;

  /* Two tokens: one for the service path itself (string match) and other for its children (as regex) */
  servicePathIn->append(path);
  servicePathIn->appendRegex(std::string("^") + escapedPath + "\\/.*");
}



/* ****************************************************************************
*
* fillQueryServicePath -
*
* Returns the BSON element associated to the service path query. Note this function
* is invoked only if there is need of a servicePath filter, the caller should call it
* only if servicePathFilterNeeded() in the same servicePath vector is true. That means that
* this fucntions can assume that:
*
* - The servicePath vector has at least one not empty element
* - The servicePath vector doesn't contain any /# element
*/
BSONObj fillQueryServicePath(const std::string& spKey, const std::vector<std::string>& servicePath)
{
  BSONArrayBuilder servicePathIn;

  // Special case (although the most common one): only one service path
  if (servicePath.size() == 1)
  {
    std::string sp = servicePath[0];
    if (sp.at(sp.length() - 1) == '#')
    {
      addServicePathInTokens(&servicePathIn, sp);
      return BSON(spKey << BSON("$in" << servicePathIn.arr()));
    }
    else
    {
      // Use simple matching. Avoid "$in: [ ... ]" pattern. This is the most common case
      return BSON(spKey << servicePath[0]);
    }
  }

  for (unsigned int ix = 0 ; ix < servicePath.size(); ++ix)
  {
    std::string sp = servicePath[ix];
    if (sp.at(sp.length() - 1) == '#')
    {
      addServicePathInTokens(&servicePathIn, sp);
    }
    else
    {
      servicePathIn.append(servicePath[ix]);
    }
  }

  return BSON(spKey << BSON("$in" << servicePathIn.arr()));
}



/* *****************************************************************************
*
* processAreaScope -
*
* Returns true if 'areaQueryP' was filled, false otherwise
*/
static bool processAreaScope(const Scope* scoP, BSONObj* areaQueryP)
{
  if (!mongoLocationCapable())
  {
    std::string details = std::string("location scope was found but your MongoDB version doesn't support it. ") +
      "Please upgrade MongoDB server to 2.4 or newer)";

    alarmMgr.badInput(clientIp, details);
    return false;
  }

  bool     inverted = false;
  BSONObj  geoWithin;

  if (scoP->areaType == orion::CircleType)
  {
    double radians = scoP->circle.radius() / EARTH_RADIUS_METERS;

    geoWithin = BSON("$centerSphere" <<
                     BSON_ARRAY(
                       BSON_ARRAY(scoP->circle.center.longitude() <<
                                  scoP->circle.center.latitude()) <<
                       radians));

    inverted  = scoP->circle.inverted();
  }
  else if (scoP->areaType == orion::PolygonType)
  {
    BSONArrayBuilder  vertex;
    double            lat0 = 0;
    double            lon0 = 0;

    for (unsigned int jx = 0; jx < scoP->polygon.vertexList.size() ; ++jx)
    {
      double  lat  = scoP->polygon.vertexList[jx]->latitude();
      double  lon  = scoP->polygon.vertexList[jx]->longitude();

      if (jx == 0)
      {
        lat0 = lat;
        lon0 = lon;
      }
      vertex.append(BSON_ARRAY(lon << lat));
    }

    /* MongoDB query API needs to "close" the polygon with the same point that the initial point */
    vertex.append(BSON_ARRAY(lon0 << lat0));

    /* Note that MongoDB query API uses an ugly "double array" structure for coordinates */
    geoWithin = BSON("$geometry" << BSON("type" << "Polygon" << "coordinates" << BSON_ARRAY(vertex.arr())));
    inverted  = scoP->polygon.inverted();
  }
  else
  {
    alarmMgr.badInput(clientIp, "unknown area type");
    return false;
  }

  if (inverted)
  {
    /* The "$exist: true" was added to make this work with MongoDB 2.6. Surprisingly, MongoDB 2.4
     * doesn't need it. See http://stackoverflow.com/questions/29388981/different-semantics-in-not-geowithin-with-polygon-geometries-between-mongodb-2 */
    *areaQueryP = BSON("$exists" << true << "$not" << BSON("$geoWithin" << geoWithin));
  }
  else
  {
    *areaQueryP = BSON("$geoWithin" << geoWithin);
  }

  return true;
}



/* *****************************************************************************
*
* addFilterScope -
*/
static void addFilterScope(ApiVersion apiVersion, const Scope* scoP, std::vector<BSONObj>* filtersP)
{
  if ((apiVersion == V2) && (scoP->type == SCOPE_FILTER_EXISTENCE) && (scoP->value == SCOPE_VALUE_ENTITY_TYPE))
  {
    // Early return to avoid _id.type: {$exits: true} in NGSIv2 case. Entity type existence filter only
    // makes sense in NGSIv1 (and may be removed soon as NGSIv1 is deprecated functionality)
    return;
  }

  std::string entityTypeString = std::string("_id.") + ENT_ENTITY_TYPE;

  if (scoP->type == SCOPE_FILTER_EXISTENCE)
  {
    // Entity type existence filter only makes sense in NGSIv1
    if (scoP->value == SCOPE_VALUE_ENTITY_TYPE)
    {
      BSONObj b = scoP->oper == SCOPE_OPERATOR_NOT ?
            BSON(entityTypeString << BSON("$exists" << false)) :
            BSON(entityTypeString << BSON("$exists" << true));

      filtersP->push_back(b);
    }
    else
    {
      std::string details = std::string("unknown value for '") +
        SCOPE_FILTER_EXISTENCE + "' filter: '" + scoP->value + "'";

      alarmMgr.badInput(clientIp, details);
    }
  }
  else
  {
    std::string details = std::string("unknown filter type '") + scoP->type + "'";
    alarmMgr.badInput(clientIp, details);
  }
}



/* ****************************************************************************
*
* sortCriteria -
*/
static std::string sortCriteria(const std::string& sortToken)
{
  if (sortToken == DATE_CREATED)
  {
    return ENT_CREATION_DATE;
  }

  if (sortToken == DATE_MODIFIED)
  {
    return ENT_MODIFICATION_DATE;
  }

  if (sortToken == ENT_ENTITY_ID)
  {
    return std::string("_id.") + ENT_ENTITY_ID;
  }

  if (sortToken == ENT_ENTITY_TYPE)
  {
    return std::string("_id.") + ENT_ENTITY_TYPE;
  }

  return std::string(ENT_ATTRS) + "." + sortToken + "." + ENT_ATTRS_VALUE;
}



/* *****************************************************************************
*
* processAreaScopeV2 -
*
* Returns true if areaQueryP was filled, false otherwise
*/
bool processAreaScopeV2(const Scope* scoP, BSONObj* areaQueryP)
{
  if (!mongoLocationCapable())
  {
    std::string details = std::string("location scope was found but your MongoDB version doesn't support it. ") +
      "Please upgrade MongoDB server to 2.4 or newer)";

    alarmMgr.badInput(clientIp, details);
    return false;
  }

  // Fill BSON corresponding to geometry
  BSONObj geometry;
  if (scoP->areaType == orion::PointType)
  {
    geometry = BSON("type"        << "Point" <<
                    "coordinates" << BSON_ARRAY(scoP->point.longitude() << scoP->point.latitude()));
  }
  else if (scoP->areaType == orion::LineType)
  {
    // Arbitrary number of points
    BSONArrayBuilder ps;

    for (unsigned int ix = 0; ix < scoP->line.pointList.size(); ++ix)
    {
      orion::Point* p = scoP->line.pointList[ix];
      ps.append(BSON_ARRAY(p->longitude() << p->latitude()));
    }
    geometry = BSON("type" << "LineString" << "coordinates" << ps.arr());
  }
  else if (scoP->areaType == orion::BoxType)
  {
    BSONArrayBuilder ps;

    ps.append(BSON_ARRAY(scoP->box.lowerLeft.longitude()  << scoP->box.lowerLeft.latitude()));
    ps.append(BSON_ARRAY(scoP->box.upperRight.longitude() << scoP->box.lowerLeft.latitude()));
    ps.append(BSON_ARRAY(scoP->box.upperRight.longitude() << scoP->box.upperRight.latitude()));
    ps.append(BSON_ARRAY(scoP->box.lowerLeft.longitude()  << scoP->box.upperRight.latitude()));
    ps.append(BSON_ARRAY(scoP->box.lowerLeft.longitude()  << scoP->box.lowerLeft.latitude()));

    geometry = BSON("type" << "Polygon" << "coordinates" << BSON_ARRAY(ps.arr()));
  }
  else if (scoP->areaType == orion::PolygonType)
  {
    // Arbitrary number of points
    BSONArrayBuilder ps;

    for (unsigned int ix = 0; ix < scoP->polygon.vertexList.size(); ++ix)
    {
      orion::Point* p = scoP->polygon.vertexList[ix];

      ps.append(BSON_ARRAY(p->longitude() << p->latitude()));
    }

    geometry = BSON("type" << "Polygon" << "coordinates" << BSON_ARRAY(ps.arr()));
  }
  else
  {
    LM_E(("Runtime Error (unknown area type: %d)", scoP->areaType));
    return false;
  }

  if (scoP->georel.type == "near")
  {
    BSONObjBuilder near;

    near.append("$geometry", geometry);

    if (scoP->georel.maxDistance >= 0)
    {
      near.append("$maxDistance", scoP->georel.maxDistance);
    }

    if (scoP->georel.minDistance >= 0)
    {
      near.append("$minDistance", scoP->georel.minDistance);
    }

    *areaQueryP = BSON("$near" << near.obj());
  }
  else if (scoP->georel.type == "coveredBy")
  {
    *areaQueryP = BSON("$geoWithin" << BSON("$geometry" << geometry));
  }
  else if (scoP->georel.type == "intersects")
  {
    *areaQueryP = BSON("$geoIntersects" << BSON("$geometry" << geometry));
  }
  else if (scoP->georel.type == "disjoint")
  {
    *areaQueryP = BSON("$exists" << true << "$not" << BSON("$geoIntersects" << BSON("$geometry" << geometry)));
  }
  else if (scoP->georel.type == "equals")
  {
    *areaQueryP = geometry;
  }
  else
  {
    LM_E(("Runtime Error (unknown georel type: '%s')", scoP->georel.type.c_str()));
    return false;
  }

  return true;
}



/* ****************************************************************************
*
* addIfNotPresentAttr -
*
* If the attribute doesn't exist in the entity, then add it (shadowed, render will depend on filter)
*/
static void addIfNotPresentAttr
(
  Entity*             eP,
  const std::string&  name,
  const std::string&  type,
  double              value
)
{
  if (eP->attributeVector.get(name) == -1)
  {
    ContextAttribute* caP = new ContextAttribute(name, type, value);
    caP->shadowed = true;
    eP->attributeVector.push_back(caP);
  }
}



/* ****************************************************************************
*
* addIfNotPresentAttrMetadata (double version) -
*
* If the metadata doesn't exist in the attribute, then add it (shadowed, render will depend on filter)
*/
static void addIfNotPresentMetadata
(
  ContextAttribute*   caP,
  const std::string&  name,
  const std::string&  type,
  double              value
)
{
  if (caP->metadataVector.lookupByName(name) == NULL)
  {
    Metadata* mdP = new Metadata(name, type, value);
    mdP->shadowed = true;
    caP->metadataVector.push_back(mdP);
  }
}


/* ****************************************************************************
*
* addIfNotPresentAttrMetadata (string version) -
*
* If the metadata doesn't exist in the attribute, then add it (shadowed, render will depend on filter)
*/
static void addIfNotPresentMetadata
(
  ContextAttribute*   caP,
  const std::string&  name,
  const std::string&  type,
  const std::string&  value
)
{
  if (caP->metadataVector.lookupByName(name) == NULL)
  {
    Metadata* mdP = new Metadata(name, type, value);
    mdP->shadowed = true;
    caP->metadataVector.push_back(mdP);
  }
}



/* ****************************************************************************
*
* addIfNotPresentPreviousValueMetadata
*
* If the metadata doesn't exist in the attribute, then add it (shadowed, render will depend on filter)
*/
static void addIfNotPresentPreviousValueMetadata(ContextAttribute* caP)
{
  if (caP->metadataVector.lookupByName(NGSI_MD_PREVIOUSVALUE) != NULL)
  {
    return;
  }

  // Created the metadata
  Metadata* mdP = NULL;
  ContextAttribute* previousValueP = caP->previousValue;

  if (previousValueP->compoundValueP == NULL)
  {
    switch (previousValueP->valueType)
    {
    case orion::ValueTypeString:
      mdP = new Metadata(NGSI_MD_PREVIOUSVALUE, previousValueP->type, previousValueP->stringValue);
      break;

    case orion::ValueTypeBoolean:
      mdP = new Metadata(NGSI_MD_PREVIOUSVALUE, previousValueP->type, previousValueP->boolValue);
      break;

    case orion::ValueTypeNumber:
      mdP = new Metadata(NGSI_MD_PREVIOUSVALUE, previousValueP->type, previousValueP->numberValue);
      break;

    case orion::ValueTypeNull:
      mdP = new Metadata(NGSI_MD_PREVIOUSVALUE, previousValueP->type, "");
      mdP->valueType = orion::ValueTypeNull;
      break;

    case orion::ValueTypeNotGiven:
      LM_E(("Runtime Error (value not given for metadata)"));
      return;

    default:
      LM_E(("Runtime Error (unknown value type: %d)", previousValueP->valueType));
      return;
    }
  }
  else
  {
    mdP            = new Metadata(NGSI_MD_PREVIOUSVALUE, previousValueP->type, "");
    mdP->valueType = previousValueP->valueType;

    // Steal the compound
    mdP->compoundValueP = previousValueP->compoundValueP;
    previousValueP->compoundValueP = NULL;
  }

  // Add it to the vector (shadowed)
  mdP->shadowed = true;
  caP->metadataVector.push_back(mdP);
}



/* ****************************************************************************
*
* addBuiltins -
*
* Add builtin attributes and metadata. Note that not all are necessarily rendered
* at the end, given the filtering process done at rendering stage.
*
* Attributes:
* - dateCreated
* - dateModified
*
* Metadata:
* - dateModified
* - dateCreated
* - actionType
* - previousValue
*
* Note that dateExpires it not added by this function, as it is implemented
* as regular attribute, recoved from the DB in the "attrs" key-map.
*/
void addBuiltins(ContextElementResponse* cerP)
{
  // dateCreated attribute
  if (cerP->entity.creDate != 0)
  {
    addIfNotPresentAttr(&cerP->entity, DATE_CREATED, DATE_TYPE, cerP->entity.creDate);
  }

  // dateModified attribute
  if (cerP->entity.modDate != 0)
  {
    addIfNotPresentAttr(&cerP->entity, DATE_MODIFIED, DATE_TYPE, cerP->entity.modDate);
  }

  for (unsigned int ix = 0; ix < cerP->entity.attributeVector.size(); ix++)
  {
    ContextAttribute* caP = cerP->entity.attributeVector[ix];

    // dateCreated medatada
    if (caP->creDate != 0)
    {
      addIfNotPresentMetadata(caP, NGSI_MD_DATECREATED, DATE_TYPE, caP->creDate);
    }

    // dateModified metadata
    if (caP->modDate != 0)
    {
      addIfNotPresentMetadata(caP, NGSI_MD_DATEMODIFIED, DATE_TYPE, caP->modDate);
    }

    // actionType
    if (caP->actionType != "")
    {
      addIfNotPresentMetadata(caP, NGSI_MD_ACTIONTYPE, DEFAULT_ATTR_STRING_TYPE, caP->actionType);
    }

    // previosValue
    if (caP->previousValue != NULL)
    {
      addIfNotPresentPreviousValueMetadata(caP);
    }
  }
}



/* ****************************************************************************
*
* isCustomAttr -
*
* Check that the parameter is a not custom attr, e.g. dateCreated
*
* FIXME P2: this function probably could be moved to another place "closer" to attribute classes
*/
static bool isCustomAttr(std::string attrName)
{
  if ((attrName != DATE_CREATED) && (attrName != DATE_MODIFIED) && (attrName != ALL_ATTRS))
  {
    return false;
  }

  return true;
}



/* ****************************************************************************
*
* entitiesQuery -
*
* This method is used by queryContext and subscribeContext (ONCHANGE conditions). It takes
* a vector with entities and a vector with attributes as input and returns the corresponding
* ContextElementResponseVector or error.
*
* Note the includeEmpty argument. This is used if we don't want the result to include empty
* attributes, i.e. the ones that cause '<contextValue></contextValue>'. This is aimed at
* subscribeContext case, as empty values can cause problems in the case of federating Context
* Brokers (the notifyContext is processed as an updateContext and in the latter case, an
* empty value causes an error)
*/
bool entitiesQuery
(
  const EntityIdVector&            enV,
  const StringList&                attrL,
  const Restriction&               res,
  ContextElementResponseVector*    cerV,
  std::string*                     err,
  bool                             includeEmpty,
  const std::string&               tenant,
  const std::vector<std::string>&  servicePath,
  int                              offset,
  int                              limit,
  bool*                            limitReached,
  long long*                       countP,
  const std::string&               sortOrderList,
  ApiVersion                       apiVersion
)
{
  /* Query structure is as follows
   *
   * {
   *    "$or": [ {}1 ... {}N ],    (always, optimized to just {}1 in the case of only one element)
   *    "_id.servicePath: { ... }  (always, in some cases using {$exists: false})
   *    "attrNames": { ... },      (only if attributes are used in the query)
   *    "location.coords": { ... } (only in the case of geo-queries)
   *  }
   *
   */

  BSONObjBuilder    finalQuery;
  BSONArrayBuilder  orEnt;

  /* Part 1: entities - avoid $or in the case of a single element */
  if (enV.size() == 1)
  {
    BSONObjBuilder bob;
    fillQueryEntity(&bob, enV[0]);
    BSONObj entObj = bob.obj();
    finalQuery.appendElements(entObj);

    LM_T(LmtMongo, ("Entity single query token: '%s'", entObj.toString().c_str()));
  }
  else
  {
    for (unsigned int ix = 0; ix < enV.size(); ++ix)
    {
      BSONObjBuilder bob;
      fillQueryEntity(&bob, enV[ix]);
      BSONObj entObj = bob.obj();
      orEnt.append(entObj);

      LM_T(LmtMongo, ("Entity query token: '%s'", entObj.toString().c_str()));
    }
    finalQuery.append("$or", orEnt.arr());
  }

  /* Part 2: service path */
  if (servicePathFilterNeeded(servicePath))
  {
    finalQuery.appendElements(fillQueryServicePath("_id." ENT_SERVICE_PATH, servicePath));
  }

  /* Part 3: attributes */
  BSONArrayBuilder attrs;

  for (unsigned int ix = 0; ix < attrL.size(); ++ix)
  {
    std::string attrName = attrL[ix];

    /* Early exit in the case of ATTR_ALL */
    if (attrName == ALL_ATTRS)
    {
      LM_T(LmtMongo, ("Attributes wildcard found"));
      break;
    }

    /* Custom metadata (e.g. dateCreated) are not "real" attributes in the DB, so they cannot
     * be included in the search query */
    if (!isCustomAttr(attrName))
    {
      attrs.append(attrName);
      LM_T(LmtMongo, ("Attribute query token: '%s'", attrName.c_str()));
    }
  }

  // If the attributes wildcard is in the list, then we ommit the attributes filter
  if ((attrs.arrSize() > 0) && (!attrL.lookup(ALL_ATTRS)))
  {
    /* If we don't do this checking, the {$in: [] } in the attribute name part will
     * make the query fail*/
    finalQuery.append(ENT_ATTRNAMES, BSON("$in" << attrs.arr()));
  }

  /* Part 5: scopes */
  std::vector<BSONObj>  filters;
  unsigned int          geoScopes = 0;

  for (unsigned int ix = 0; ix < res.scopeVector.size(); ++ix)
  {
    const Scope* scopeP = res.scopeVector[ix];

    if (scopeP->type.find(SCOPE_FILTER) == 0)
    {
      // FIXME P5: NGSIv1 filter, probably to be removed in the future
      addFilterScope(apiVersion, scopeP, &filters);
    }
    else if (scopeP->type == FIWARE_LOCATION ||
             scopeP->type == FIWARE_LOCATION_DEPRECATED ||
             scopeP->type == FIWARE_LOCATION_V2)
    {
      geoScopes++;
      if (geoScopes > 1)
      {
        alarmMgr.badInput(clientIp, "current version supports only one area scope, extra geoScope is ignored");
      }
      else
      {
        BSONObj areaQuery;

        bool result;
        if (scopeP->type == FIWARE_LOCATION_V2)
        {
          result = processAreaScopeV2(scopeP, &areaQuery);
        }
        else  // FIWARE Location NGSIv1 (legacy)
        {
          result = processAreaScope(scopeP, &areaQuery);
        }

        if (result)
        {
          std::string locCoords = ENT_LOCATION "." ENT_LOCATION_COORDS;
          finalQuery.append(locCoords, areaQuery);
        }
      }
    }
    else if (scopeP->type == SCOPE_TYPE_SIMPLE_QUERY)
    {
      if (scopeP->stringFilterP)
      {
        for (unsigned int ix = 0; ix < scopeP->stringFilterP->mongoFilters.size(); ++ix)
        {
          finalQuery.appendElements(scopeP->stringFilterP->mongoFilters[ix]);
        }
      }
    }
    else if (scopeP->type == SCOPE_TYPE_SIMPLE_QUERY_MD)
    {
      if (scopeP->mdStringFilterP)
      {
        for (unsigned int ix = 0; ix < scopeP->mdStringFilterP->mongoFilters.size(); ++ix)
        {
          finalQuery.appendElements(scopeP->mdStringFilterP->mongoFilters[ix]);
        }
      }
    }
    else
    {
      std::string details = std::string("unknown scope type '") + scopeP->type + "', ignoring";
      alarmMgr.badInput(clientIp, details);
    }
  }

  for (unsigned int ix = 0; ix < filters.size(); ++ix)
  {
    finalQuery.appendElements(filters[ix]);
  }

  LM_T(LmtPagination, ("Offset: %d, Limit: %d, countP: %p", offset, limit, countP));

  /* Do the query on MongoDB */
  std::auto_ptr<DBClientCursor>  cursor;
  Query                          query(finalQuery.obj());

  if (sortOrderList == "")
  {
    query.sort(BSON(ENT_CREATION_DATE << 1));
  }
  else if ((sortOrderList == ORDER_BY_PROXIMITY))
  {
    // In this case the solution is not setting any query.sort(), as the $near operator will do the
    // sorting itself. Of course, using orderBy=geo:distance without using georel=near will return
    // unexpected ordering, but this is already warned in the documentation.
  }
  else
  {
    std::vector<std::string>  sortedV;
    int                       components = stringSplit(sortOrderList, ',', sortedV);
    BSONObjBuilder            sortOrder;

    for (int ix = 0; ix < components; ix++)
    {
      std::string  sortToken;
      int          sortDirection;

      if (sortedV[ix][0] == '!')
      {
        // reverse
        sortToken     = sortedV[ix].substr(1);
        sortDirection = -1;
      }
      else
      {
        sortToken     = sortedV[ix];
        sortDirection = 1;
      }

      sortOrder.append(sortCriteria(sortToken), sortDirection);
    }

    query.sort(sortOrder.obj());
  }

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();

  if (!collectionRangedQuery(connection, getEntitiesCollectionName(tenant), query, limit, offset, &cursor, countP, err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    return false;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  unsigned int docs = 0;

  while (moreSafe(cursor))
  {
    BSONObj  r;
    try
    {
      // nextSafeOrError cannot be used here, as AssertionException has a special treatment in this case
      r = cursor->nextSafe();
    }
    catch (const AssertionException &e)
    {
      std::string              exErr = e.what();
      ContextElementResponse*  cer   = new ContextElementResponse();

      //
      // We can't return the error 'as is', as it may contain forbidden characters.
      // So, we can just match the error and send a less descriptive text.
      //
      const char* invalidPolygon      = "Exterior shell of polygon is invalid";
      const char* sortError           = "nextSafe(): { $err: \"Executor error: OperationFailed Sort operation used more than the maximum";
      const char* defaultErrorString  = "Error at querying MongoDB";

      alarmMgr.dbError(exErr);

      if (strncmp(exErr.c_str(), invalidPolygon, strlen(invalidPolygon)) == 0)
      {
        exErr = invalidPolygon;
      }
      else if (strncmp(exErr.c_str(), sortError, strlen(sortError)) == 0)
      {
        exErr = "Sort operation used more than the maximum RAM. "
                "You should create an index. "
                "Check the Database Administration section in Orion documentation.";
      }
      else
      {
        exErr = defaultErrorString;
      }

      //
      // It would be nice to fill in the entity but it is difficult to do this.
      //
      // Solution:
      //   If the incoming entity-vector has only *one* entity, I simply fill it in with enV[0] and
      //   if more than one entity is in the vector, an empty entity is returned.
      //
      if (enV.size() == 1)
      {
        cer->entity.fill(enV[0]->id, enV[0]->type, enV[0]->isPattern);
      }
      else
      {
        cer->entity.fill("", "", "");
      }

      cer->statusCode.fill(SccReceiverInternalError, exErr);
      cerV->push_back(cer);
      releaseMongoConnection(connection);
      return true;
    }
    catch (const std::exception &e)
    {
      *err = e.what();
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", e.what(), query.toString().c_str()));
      releaseMongoConnection(connection);
      return false;
    }
    catch (...)
    {
      *err = "generic exception at nextSafe()";
      LM_E(("Runtime Error (generic exception in nextSafe() - query: %s)", query.toString().c_str()));
      releaseMongoConnection(connection);
      return false;
    }

    alarmMgr.dbErrorReset();

    // Build CER from BSON retrieved from DB
    docs++;
    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, r.toString().c_str()));
    ContextElementResponse*  cer = new ContextElementResponse(r, attrL, includeEmpty, apiVersion);

    // Add builtin attributes and metadata (only in NGSIv2)
    if (apiVersion == V2)
    {
      addBuiltins(cer);
    }

    /* All the attributes existing in the request but not found in the response are added with 'found' set to false */
    for (unsigned int ix = 0; ix < attrL.size(); ++ix)
    {
      bool         found     = false;
      std::string  attrName  = attrL[ix];

      /* The special case "*" is not taken into account*/
      if (attrName == ALL_ATTRS)
      {
        continue;
      }

      for (unsigned int jx = 0; jx < cer->entity.attributeVector.size(); ++jx)
      {
        if (attrName == cer->entity.attributeVector[jx]->name)
        {
          found = true;
          break;
        }
      }

      if (!found)
      {
        ContextAttribute* caP = new ContextAttribute(attrName, "", "", false);
        cer->entity.attributeVector.push_back(caP);
      }
    }

    cer->statusCode.fill(SccOk);
    cerV->push_back(cer);
  }
  releaseMongoConnection(connection);

  /* If we have already reached the pagination limit with local entities, we have ended: no more "potential"
   * entities are added. Only if limitReached is being used, i.e. not NULL
   * FIXME P10 (it is easy :) limit should be unsigned int */
  if (limitReached != NULL)
  {
    *limitReached = (cerV->size() >= (unsigned int) limit);
    if (*limitReached)
    {
      LM_T(LmtMongo, ("entities limit reached"));
      return true;
    }
  }

  /* All the not-patterned entities in the request not in the response are added (without attributes), as they are
   * used before pruning in the CPr calculation logic */
  for (unsigned int ix = 0; ix < enV.size(); ++ix)
  {
    if (enV[ix]->isPattern != "true")
    {
      bool needToAdd = true;

      for (unsigned int jx = 0; jx < cerV->size(); ++jx)
      {
        if (((*cerV)[jx]->entity.id == enV[ix]->id) && ((*cerV)[jx]->entity.type == enV[ix]->type))
        {
          needToAdd = false;
          break;  /* jx */
        }
      }

      if (needToAdd)
      {
        ContextElementResponse* cerP = new ContextElementResponse();

        cerP->entity.id = enV[ix]->id;
        cerP->entity.type = enV[ix]->type;
        cerP->entity.isPattern = "false";

        //
        // This entity has to be pruned if after CPr searching no attribute is "added" to it.
        // The prune attribute distinguish this kind of entities from the "real ones" that are
        // without attributes in the Orion local database (a rare case, but may happen)
        //
        cerP->prune = true;

        for (unsigned int jx = 0; jx < attrL.size(); ++jx)
        {
          ContextAttribute* caP = new ContextAttribute(attrL[jx], "", "", false);

          cerP->entity.attributeVector.push_back(caP);
        }

        cerP->statusCode.fill(SccOk);

        cerV->push_back(cerP);
      }
    }
  }

  return true;
}



/* ****************************************************************************
*
* pruneContextElements -
*
* Remove attributes in the vector with 'found' value is 'false'
*
* In the case of NGSIv2 we filter attributes with CPr not included in a list of attributes
* passed as reference (and that comes from the original NGSIv2 query). Otherwise,
* over-querying for attributes may occur and this could break some CPr usage cases (in particular
* the ones with IOTAs, which may report error in the case of being asked for an attribute
* they don't manage).
*
* This can be checked with cases/3068_ngsi_v2_based_forwarding/check_no_overquerying.test. If
* you disable the attrsV processing in this function, you would see how that test fail due
* to over-querying
*/
void pruneContextElements
(
  ApiVersion                           apiVersion,
  const StringList&                    attrsV,
  const ContextElementResponseVector&  oldCerV,
  ContextElementResponseVector*        newCerVP
)
{
  for (unsigned int ix = 0; ix < oldCerV.size(); ++ix)
  {
    ContextElementResponse* cerP    = oldCerV[ix];
    ContextElementResponse* newCerP = new ContextElementResponse();

    newCerP->entity.fill(cerP->entity.id,
                         cerP->entity.type,
                         cerP->entity.isPattern,
                         cerP->entity.servicePath,
                         cerP->entity.creDate,
                         cerP->entity.modDate);

    // FIXME P10: not sure if this is the right way to do it, maybe we need a fill() method for this
    newCerP->entity.providingApplicationList = cerP->entity.providingApplicationList;
    newCerP->statusCode.fill(&cerP->statusCode);

    bool pruneEntity = cerP->prune;

    for (unsigned int jx = 0; jx < cerP->entity.attributeVector.size(); ++jx)
    {
      ContextAttribute* caP = cerP->entity.attributeVector[jx];

      // To be included, it need to be found and one of the following:
      // - It is V1
      // - (Not being V1) The attributes filter list is empty
      // - (Not being V1 and not empty attributes filter) The attribute is included in the filter list (taking into account wildcard)
      if ((caP->found) && ((apiVersion == V1) || (attrsV.size() == 0) || (attrsV.lookup(caP->name, ALL_ATTRS))))
      {
        ContextAttribute* newCaP = new ContextAttribute(caP);
        newCerP->entity.attributeVector.push_back(newCaP);
      }
    }

    /* If after pruning the entity has no attribute and no CPr information, then it is not included
     * in the output vector, except if "prune" is set to false */
    if (pruneEntity &&
        (newCerP->entity.attributeVector.size()          == 0) &&
        (newCerP->entity.providingApplicationList.size() == 0))
    {
      newCerP->release();
      delete newCerP;
    }
    else
    {
      newCerVP->push_back(newCerP);
    }
  }
}



/* ***************************************************************************
*
* processEntity -
*/
static void processEntity(ContextRegistrationResponse* crr, const EntityIdVector& enV, BSONObj entity)
{
  EntityId en;

  en.id        = getStringFieldF(entity, REG_ENTITY_ID);
  en.type      = entity.hasField(REG_ENTITY_TYPE)?      getStringFieldF(entity, REG_ENTITY_TYPE)      : "";
  en.isPattern = entity.hasField(REG_ENTITY_ISPATTERN)? getStringFieldF(entity, REG_ENTITY_ISPATTERN) : "false";

  if (includedEntity(en, enV))
  {
    EntityId* enP = new EntityId(en.id, en.type, en.isPattern);

    crr->contextRegistration.entityIdVector.push_back(enP);
  }
}



/* ***************************************************************************
*
* processAttribute -
*/
static void processAttribute(ContextRegistrationResponse* crr, const StringList& attrL, const BSONObj& attribute)
{
  ContextRegistrationAttribute attr(
    getStringFieldF(attribute, REG_ATTRS_NAME),
    getStringFieldF(attribute, REG_ATTRS_TYPE));

  if (includedAttribute(attr.name, attrL))
  {
    ContextRegistrationAttribute* attrP = new ContextRegistrationAttribute(attr.name, attr.type);
    crr->contextRegistration.contextRegistrationAttributeVector.push_back(attrP);
  }
}



/* ***************************************************************************
*
* processContextRegistrationElement -
*/
static void processContextRegistrationElement
(
  BSONObj                             cr,
  const EntityIdVector&               enV,
  const StringList&                   attrL,
  ContextRegistrationResponseVector*  crrV,
  MimeType                            mimeType,
  ProviderFormat                      providerFormat
)
{
  ContextRegistrationResponse crr;

  crr.contextRegistration.providingApplication.set(getStringFieldF(cr, REG_PROVIDING_APPLICATION));
  crr.contextRegistration.providingApplication.setProviderFormat(providerFormat);

  std::vector<BSONElement> queryEntityV = getFieldF(cr, REG_ENTITIES).Array();

  for (unsigned int ix = 0; ix < queryEntityV.size(); ++ix)
  {
    processEntity(&crr, enV, queryEntityV[ix].embeddedObject());
  }

  /* Note that attributes can be included only if at least one entity has been found */
  if (crr.contextRegistration.entityIdVector.size() > 0)
  {
    if (cr.hasField(REG_ATTRS)) /* To prevent registration in the E-<null> style */
    {
      std::vector<BSONElement> queryAttrV = getFieldF(cr, REG_ATTRS).Array();

      for (unsigned int ix = 0; ix < queryAttrV.size(); ++ix)
      {
        processAttribute(&crr, attrL, queryAttrV[ix].embeddedObject());
      }
    }
  }

  // FIXME: we don't take metadata into account at the moment
  // crr.contextRegistration.registrationMetadataV = ..

  /* Note that the context registration element is only included in one of the following cases:
   * - The number of entities and attributes included are both greater than 0
   * - The number of entities is greater than 0, the number of attributes is 0 but the discover
   *   doesn't use attributes in the request
   */
  if (crr.contextRegistration.entityIdVector.size() == 0)
  {
    return;
  }

  if (crr.contextRegistration.contextRegistrationAttributeVector.size() > 0  ||
      (crr.contextRegistration.contextRegistrationAttributeVector.size() == 0 && attrL.size() == 0))
  {
    ContextRegistrationResponse* crrP = new ContextRegistrationResponse();

    crrP->contextRegistration = crr.contextRegistration;
    crrP->providerFormat      = providerFormat;

    crrV->push_back(crrP);
  }
}



/* ****************************************************************************
*
* registrationsQuery -
*
* This method is used by discoverContextAvailabililty and subscribeContextAvailability. It takes
* a vector with entities and a vector with attributes as input and returns the corresponding
* ContextRegistrationResponseVector or error.
*/
bool registrationsQuery
(
  const EntityIdVector&               enV,
  const StringList&                   attrL,
  ContextRegistrationResponseVector*  crrV,
  std::string*                        err,
  const std::string&                  tenant,
  const std::vector<std::string>&     servicePathV,
  int                                 offset,
  int                                 limit,
  bool                                details,
  long long*                          countP
)
{
  // query structure:
  //
  // NOTE: 'cr' is and abreviation of 'contextRegistration'
  //
  // {
  //   $or: [
  //          { cr.entities.id: E1,cr.entities.type: T1} }, ...,         (isPattern = false, with type)
  //          { cr.entities.id: E2 }, ...                                (isPattern = false, no type)
  //          { cr.entities.id: /E.*3/, crs.entities.type: T3 } }, ...,  (isPattern = true, with type)
  //          { cr.entities.id: /E.*4/, }, ...,                          (isPattern = true, no type)
  //
  //          (next two ones are for "universal pattern" registrations)
  //
  //          { cr.entities.id: ".*", cr.entities.isPattern: "true", crs.entities.type: {$in: [T1, T3, ...} },
  //          { cr.entities.id: ".*", cr.entities.isPattern: "true", crs.entities.type: {$exists: false } },
  //   ],
  //   cr.attrs.name : { $in: [ A1, ... ] },  (only if attrs > 0)
  //   servicePath: ... ,
  //   expiration: { $gt: ... }
  // }
  //
  // Note that by construction the $or array always has at least two elements (the two ones corresponding to the
  // universal pattern) so we cannot avoid to use this operator.
  //
  // FIXME P5: the 'contextRegistration' token (19 chars) repeats in the query BSON. It would be better use 'cr' (2 chars)
  // but this would need a data model migration in DB

  /* Build query based on arguments */
  std::string       crEntitiesId      = REG_CONTEXT_REGISTRATION "." REG_ENTITIES "." REG_ENTITY_ID;
  std::string       crEntitiesType    = REG_CONTEXT_REGISTRATION "." REG_ENTITIES "." REG_ENTITY_TYPE;
  std::string       crEntitiesPattern = REG_CONTEXT_REGISTRATION "." REG_ENTITIES "." REG_ENTITY_ISPATTERN;
  std::string       crAttrsNames      = REG_CONTEXT_REGISTRATION "." REG_ATTRS    "." REG_ATTRS_NAME;
  BSONArrayBuilder  entityOr;
  BSONArrayBuilder  types;

  for (unsigned int ix = 0; ix < enV.size(); ++ix)
  {
    const EntityId* en = enV[ix];
    BSONObjBuilder b;

    if (isTrue(en->isPattern))
    {
      b.appendRegex(crEntitiesId, en->id);
    }
    else  /* isPattern = false */
    {
      b.append(crEntitiesId, en->id);
    }

    if (en->type != "")
    {
      b.append(crEntitiesType, en->type);
      // FIXME P3: this way of accumulating types in the BSONBuilder doesn't avoid duplication. It doesn't
      // hurt too much, but it would be better to use a std::map to ensure uniqueness
      types.append(en->type);
    }
    entityOr.append(b.obj());
  }

  // '.*' pattern match every other pattern and every not pattern entity. We add a query checking only the types
  // and the case of no type
  entityOr.append(BSON(crEntitiesId      << ".*" <<
                       crEntitiesPattern << "true" <<
                       crEntitiesType    << BSON("$in" << types.arr())));

  entityOr.append(BSON(crEntitiesId      << ".*" <<
                       crEntitiesPattern << "true" <<
                       crEntitiesType    << BSON("$exists" << false)));

  BSONArrayBuilder attrs;

  for (unsigned int ix = 0; ix < attrL.size(); ++ix)
  {
    std::string attrName = attrL[ix];

    attrs.append(attrName);
    LM_T(LmtMongo, ("Attribute discovery: '%s'", attrName.c_str()));
  }

  BSONObjBuilder queryBuilder;

  queryBuilder.append("$or", entityOr.arr());
  queryBuilder.append(REG_EXPIRATION, BSON("$gt" << (long long) getCurrentTime()));

  if (attrs.arrSize() > 0)
  {
    /* If we don't do this check, the {$in: [] } of the attribute name part makes the query fail */
    queryBuilder.append(crAttrsNames, BSON("$in" << attrs.arr()));
  }

  //
  // 'And-in' the service path
  //
  if (servicePathFilterNeeded(servicePathV))
  {
    queryBuilder.appendElements(fillQueryServicePath(REG_SERVICE_PATH, servicePathV));
  }


  //
  // Do the query in MongoDB
  // FIXME P2: Use field selector to include the only relevant field:
  //           contextRegistration array (e.g. "expiration" is not needed)
  //
  std::auto_ptr<DBClientCursor>  cursor;
  Query                          query(queryBuilder.obj());

  query.sort(BSON("_id" << 1));

  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();
  std::string   colName    = getRegistrationsCollectionName(tenant);

  if (!collectionRangedQuery(connection, colName, query, limit, offset, &cursor, countP, err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    return false;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  unsigned int docs = 0;

  while (moreSafe(cursor))
  {
    BSONObj r;
    if (!nextSafeOrErrorF(cursor, &r, err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err->c_str(), query.toString().c_str()));
      continue;
    }
    docs++;

    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, r.toString().c_str()));

    MimeType                  mimeType = JSON;
    std::vector<BSONElement>  queryContextRegistrationV = getFieldF(r, REG_CONTEXT_REGISTRATION).Array();
    std::string               format                    = getStringFieldF(r, REG_FORMAT);
    ProviderFormat            providerFormat            = (format == "")? PfJson : (format == "JSON")? PfJson : PfV2;

    for (unsigned int ix = 0 ; ix < queryContextRegistrationV.size(); ++ix)
    {
      processContextRegistrationElement(queryContextRegistrationV[ix].embeddedObject(), enV, attrL, crrV, mimeType, providerFormat);
    }

    /* FIXME: note that given the response doesn't distinguish from which registration ID the
     * response comes, it could have that we have same context registration elements, belong to different
     * registrations ID at DB level, thus causing a duplicated context element response. Moreover,
     * NGSI doesn't forbid to registry exactly twice the same context registration element in the
     * same registration ID. Thus, it could be interesting to post-process the response vector, to
     * "compact" removing duplicated responses.*/
  }
  releaseMongoConnection(connection);

  return true;
}



/* ****************************************************************************
*
* isCondValueInContextElementResponse -
*/
bool isCondValueInContextElementResponse(ConditionValueList* condValues, ContextElementResponseVector* cerV)
{
  /* Empty conValue means that any attribute matches (aka ONANYCHANGE) */
  if (condValues->size() == 0)
  {
    return true;
  }

  for (unsigned int cvlx = 0; cvlx < condValues->size(); ++cvlx)
  {
    for (unsigned int aclx = 0; aclx < cerV->size(); ++aclx)
    {
      ContextAttributeVector caV = (*cerV)[aclx]->entity.attributeVector;

      for (unsigned int kx = 0; kx < caV.size(); ++kx)
      {
        if (caV[kx]->name == (*condValues)[cvlx])
        {
          return true;
        }
      }
    }
  }

  return false;
}



/* ****************************************************************************
*
* condValueAttrMatch -
*/
bool condValueAttrMatch(const BSONObj& sub, const std::vector<std::string>& modifiedAttrs)
{
  std::vector<BSONElement>  conds = getFieldF(sub, CSUB_CONDITIONS).Array();

  if (conds.size() == 0)
  {
    // ONANYCHANGE case: always match
    return true;
  }

  for (unsigned int ix = 0; ix < conds.size() ; ++ix)
  {
    std::string condAttr = conds[ix].String();
    for (unsigned int jx = 0; jx < modifiedAttrs.size(); ++jx)
    {
      if (condAttr == modifiedAttrs[jx])
      {
        return true;
      }
    }
  }

  return false;
}



/* ****************************************************************************
*
* subToEntityIdVector -
*
* Extract the entity ID vector from a BSON document (in the format of the csubs/casub
* collection)
*/
EntityIdVector subToEntityIdVector(const BSONObj& sub)
{
  EntityIdVector            enV;
  std::vector<BSONElement>  subEnts = getFieldF(sub, CSUB_ENTITIES).Array();

  for (unsigned int ix = 0; ix < subEnts.size() ; ++ix)
  {
    BSONObj    subEnt = subEnts[ix].embeddedObject();
    EntityId*  en     = new EntityId(getStringFieldF(subEnt, CSUB_ENTITY_ID),
                                     subEnt.hasField(CSUB_ENTITY_TYPE) ? getStringFieldF(subEnt, CSUB_ENTITY_TYPE) : "",
                                     getStringFieldF(subEnt, CSUB_ENTITY_ISPATTERN));
    enV.push_back(en);
  }

  return enV;
}



/* ****************************************************************************
*
* getCommonAttributes -
*/
static void getCommonAttributes
(
  const bool                        type,
  const std::vector<std::string>&   fVector,
  const std::vector<std::string>&   sVector,
  std::vector<std::string>&         resultVector
)
{
  if (type)
  {
    for (unsigned int cavOc = 0; cavOc < fVector.size(); ++cavOc)
    {
      for (unsigned int avOc = 0; avOc < sVector.size(); ++avOc)
      {
        if (fVector[cavOc] == sVector[avOc])
        {
          resultVector.push_back(fVector[cavOc]);
        }
      }
    }
  }
  else
  {
    for (unsigned int cavOc = 0; cavOc < fVector.size(); ++cavOc)
    {
      for (unsigned int avOc = 0; avOc < sVector.size(); ++avOc)
      {
        if (fVector[cavOc] != sVector[avOc])
        {
          resultVector.push_back(fVector[cavOc]);
        }
      }
    }
  }
}



/* ****************************************************************************
*
* subToNotifyList -
*/
void subToNotifyList
(
  const std::vector<std::string>&  modifiedAttrs,
  const std::vector<std::string>&  conditionVector,
  const std::vector<std::string>&  notificationVector,
  const std::vector<std::string>&  entityAttrsVector,
  StringList&                      attrL,
  const bool&                      blacklist,
  bool&                            op
)
{
    std::vector<std::string>  condAttrs;
    std::vector<std::string>  notifyAttrs;

    if (!blacklist)
    {
      if (conditionVector.size() == 0 && notificationVector.size() == 0)
      {
        attrL.fill(modifiedAttrs);
      }
      else if (conditionVector.size() == 0 && notificationVector.size() != 0)
      {
        getCommonAttributes(true, modifiedAttrs, notificationVector, notifyAttrs);
      }
      else if (conditionVector.size() != 0 && notificationVector.size() == 0)
      {
        getCommonAttributes(true, modifiedAttrs, entityAttrsVector, notifyAttrs);
      }
      else
      {
        getCommonAttributes(true, modifiedAttrs, notificationVector, notifyAttrs);
      }
      if (notifyAttrs.size() == 0 && (conditionVector.size() != 0 || notificationVector.size() != 0))
      {
        op = true;
      }
      attrL.fill(notifyAttrs);
    }
    else if (blacklist)
    {
      if (conditionVector.size() == 0 && notificationVector.size() != 0)
      {
        getCommonAttributes(false, modifiedAttrs, notificationVector, notifyAttrs);
      }
      else
      {
        getCommonAttributes(false, modifiedAttrs, notificationVector, condAttrs);
        getCommonAttributes(true, condAttrs, entityAttrsVector, notifyAttrs);
      }

      if (notifyAttrs.size() == 0)
      {
        op = true;
      }
      attrL.fill(notifyAttrs);
    }
}



/* ****************************************************************************
*
* subToAttributeList -
*
* Extract the attribute list from a BSON document (in the format of the csubs/casub
* collection)
*/
StringList subToAttributeList
(
  const BSONObj&                  sub,
  const bool&                     onlyChanged,
  const bool&                     blacklist,
  const std::vector<std::string>  modifiedAttrs,
  const std::vector<std::string>  attributes,
  bool&                           op
)
{
  if (!onlyChanged)
  {
    return subToAttributeList(sub);
  }
  StringList                attrL;
  std::vector<BSONElement>  subAttrs = getFieldF(sub, CSUB_ATTRS).Array();
  std::vector<BSONElement>  condAttrs = getFieldF(sub, CSUB_CONDITIONS).Array();
  std::vector<std::string>          conditionAttrs;
  std::vector<std::string>          notificationAttrs;
  for (unsigned int ix = 0; ix < subAttrs.size() ; ++ix)
  {
    std::string subAttr = subAttrs[ix].String();
    notificationAttrs.push_back(subAttr);
  }
  for (unsigned int ix = 0; ix < condAttrs.size() ; ++ix)
  {
    std::string subAttr = condAttrs[ix].String();
    conditionAttrs.push_back(subAttr);
  }
  subToNotifyList(modifiedAttrs, conditionAttrs, notificationAttrs, attributes, attrL, blacklist, op);
  return attrL;
}



/* ****************************************************************************
*
* subToAttributeList -
*
* Extract the attribute list from a BSON document (in the format of the csubs/casub
* collection)
*/
StringList subToAttributeList(const BSONObj& sub)
{
  StringList                attrL;
  std::vector<BSONElement>  subAttrs = getFieldF(sub, CSUB_ATTRS).Array();

  for (unsigned int ix = 0; ix < subAttrs.size() ; ++ix)
  {
    std::string subAttr = subAttrs[ix].String();

    attrL.push_back(subAttr);
  }

  return attrL;
}



#if 0
/* ****************************************************************************
*
* setOnSubscriptionMetadata -
*
* FIXME #920: disabled by the moment, maybe removed at the end
*/
static void setOnSubscriptionMetadata(ContextElementResponseVector* cerVP)
{
  for (unsigned int ix = 0; ix < cerVP->size(); ix++)
  {
    ContextElementResponse* cerP = (*cerVP)[ix];

    for (unsigned int jx = 0; jx < cerP->entity.attributeVector.size(); jx++)
    {
      ContextAttribute*  caP     = cerP->entity.attributeVector[jx];
      Metadata*          newMdP  = new Metadata(NGSI_MD_NOTIF_ONSUBCHANGE, DEFAULT_ATTR_BOOL_TYPE, true);

      caP->metadataVector.push_back(newMdP);
    }
  }
}
#endif



/* ****************************************************************************
*
* processOnChangeConditionForSubscription -
*
* This function is called from initial processing of an ONCHANGE condition in
* processConditionVector (used from subscribeContext and updateContextSubscription),
* so an "initial" notification for all the entites/attributes included in the entity
* in the case that some of them are within the ones in the condValues.
*
* Note that ONCHANGE notifications sent due to updateContext are processed by a
* different function (see processOnChangeConditionForUpdateContext)
*
* The argument enV is the entities and attributes in the subscribeContext
* request. The argument attrL is the attributes in the subscribeContext request.
*
* This method returns true if the notification was actually send. Otherwise, false
* is returned. This is used in the caller to know if lastNotification field in the
* subscription document in csubs collection has to be modified or not.
*/
static bool processOnChangeConditionForSubscription
(
  const EntityIdVector&            enV,
  const StringList&                attrL,
  const std::vector<std::string>&  metadataV,
  ConditionValueList*              condValues,
  const std::string&               subId,
  const HttpInfo&                  notifyHttpInfo,
  RenderFormat                     renderFormat,
  const std::string&               tenant,
  const std::string&               xauthToken,
  const std::vector<std::string>&  servicePathV,
  const Restriction*               resP,
  const std::string&               fiwareCorrelator,
  const std::vector<std::string>&  attrsOrder,
  bool                             blacklist,
  ApiVersion                       apiVersion
)
{
  std::string                   err;
  NotifyContextRequest          ncr;
  ContextElementResponseVector  rawCerV;
  StringList                    emptyList;
  StringList                    metadataList;

  metadataList.fill(metadataV);
  if (!blacklist && !entitiesQuery(enV, attrL, *resP, &rawCerV, &err, true, tenant, servicePathV, 0, 0, NULL, NULL, "", apiVersion))
  {
    ncr.contextElementResponseVector.release();
    rawCerV.release();

    return false;
  }
  else if (blacklist && !entitiesQuery(enV, metadataList, *resP, &rawCerV, &err, true, tenant, servicePathV, 0, 0, NULL, NULL, "", apiVersion))
  {
    ncr.contextElementResponseVector.release();
    rawCerV.release();

    return false;
  }

  /* Prune "not found" CERs */
  pruneContextElements(apiVersion, emptyList, rawCerV, &ncr.contextElementResponseVector);

  // Add builtin attributes and metadata (both NGSIv1 and NGSIv2 as this is
  // for notifications and NGSIv2 builtins can be used in NGSIv1 notifications) */
  for (unsigned int ix = 0; ix < ncr.contextElementResponseVector.size() ; ix++)
  {
    addBuiltins(ncr.contextElementResponseVector[ix]);
  }

#ifdef WORKAROUND_2994
  delayedReleaseAdd(rawCerV);
  rawCerV.vec.clear();
#else
  rawCerV.release();
#endif

#if 0
  // FIXME #920: disabled for the moment, maybe to be removed in the end
  /* Append notification metadata */
  if (metadataFlags)
  {
    setOnSubscriptionMetadata(&ncr.contextElementResponseVector);
  }
#endif

  if (ncr.contextElementResponseVector.size() > 0)
  {
    /* Complete the fields in NotifyContextRequest */
    ncr.subscriptionId.set(subId);
    // FIXME: we use a proper origin name
    ncr.originator.set("localhost");

    if (condValues != NULL)
    {
      /* Check if some of the attributes in the NotifyCondition values list are in the entity.
       * Note that in this case we do a query for all the attributes, not restricted to attrV */
      ContextElementResponseVector  allCerV;


      if (!entitiesQuery(enV, emptyList, *resP, &rawCerV, &err, false, tenant, servicePathV, 0, 0, NULL, NULL, "", apiVersion))
      {
#ifdef WORKAROUND_2994
        delayedReleaseAdd(rawCerV);
        rawCerV.vec.clear();
#else
        rawCerV.release();
#endif
        ncr.contextElementResponseVector.release();

        return false;
      }

      /* Prune "not found" CERs */
      pruneContextElements(apiVersion, emptyList, rawCerV, &allCerV);

#ifdef WORKAROUND_2994
      delayedReleaseAdd(rawCerV);
      rawCerV.vec.clear();
#else
      rawCerV.release();
#endif

      if (isCondValueInContextElementResponse(condValues, &allCerV))
      {
        /* Send notification */
        getNotifier()->sendNotifyContextRequest(ncr,
                                                notifyHttpInfo,
                                                tenant,
                                                xauthToken,
                                                fiwareCorrelator,
                                                renderFormat,
                                                attrsOrder,
                                                blacklist,
                                                metadataV);
        allCerV.release();
        ncr.contextElementResponseVector.release();

        return true;
      }

      allCerV.release();
    }
    else
    {
      getNotifier()->sendNotifyContextRequest(ncr,
                                              notifyHttpInfo,
                                              tenant,
                                              xauthToken,
                                              fiwareCorrelator,
                                              renderFormat,
                                              attrsOrder,
                                              blacklist,
                                              metadataV);

      ncr.contextElementResponseVector.release();

      return true;
    }
  }

  ncr.contextElementResponseVector.release();

  return false;
}



/* ****************************************************************************
*
* processConditionVector -
*/
static BSONArray processConditionVector
(
  NotifyConditionVector*           ncvP,
  const EntityIdVector&            enV,
  const StringList&                attrL,
  const std::vector<std::string>&  metadataV,
  const std::string&               subId,
  const HttpInfo&                  httpInfo,
  bool*                            notificationDone,
  RenderFormat                     renderFormat,
  const std::string&               tenant,
  const std::string&               xauthToken,
  const std::vector<std::string>&  servicePathV,
  const Restriction*               resP,
  const std::string&               status,
  const std::string&               fiwareCorrelator,
  const std::vector<std::string>&  attrsOrder,
  bool                             blacklist,
  const bool&                      skipInitialNotification,
  ApiVersion                       apiVersion
)
{
  BSONArrayBuilder conds;

  *notificationDone = false;

  for (unsigned int ix = 0; ix < ncvP->size(); ++ix)
  {
    NotifyCondition* nc = (*ncvP)[ix];

    if (nc->type == ON_CHANGE_CONDITION)
    {
      for (unsigned int jx = 0; jx < nc->condValueList.size(); ++jx)
      {
        conds.append(nc->condValueList[jx]);
      }

      if ((status == STATUS_ACTIVE) && !skipInitialNotification &&
          (processOnChangeConditionForSubscription(enV,
                                                   attrL,
                                                   metadataV,
                                                   &(nc->condValueList),
                                                   subId,
                                                   httpInfo,
                                                   renderFormat,
                                                   tenant,
                                                   xauthToken,
                                                   servicePathV,
                                                   resP,
                                                   fiwareCorrelator,
                                                   attrsOrder,
                                                   blacklist,
                                                   apiVersion)))
      {
        *notificationDone = true;
      }
    }
    else
    {
      LM_E(("Runtime Error (unknown condition type: '%s')", nc->type.c_str()));
    }
  }

  return conds.arr();
}



/* ****************************************************************************
*
* processConditionVector -
*
* This is a wrapper for the other vesion of processConditionVector(), for NGSIv2.
* At the end, this version should be the actual one, once NGSIv1 is removed.
*/
BSONArray processConditionVector
(
  const std::vector<std::string>&  condAttributesV,
  const std::vector<EntID>&        entitiesV,
  const std::vector<std::string>&  notifAttributesV,
  const std::vector<std::string>&  metadataV,
  const std::string&               subId,
  const HttpInfo&                  httpInfo,
  bool*                            notificationDone,
  RenderFormat                     renderFormat,
  const std::string&               tenant,
  const std::string&               xauthToken,
  const std::vector<std::string>&  servicePathV,
  const Restriction*               resP,
  const std::string&               status,
  const std::string&               fiwareCorrelator,
  const std::vector<std::string>&  attrsOrder,
  bool                             blacklist,
  const bool&                      skipInitialNotification,
  ApiVersion                       apiVersion
)
{
  NotifyConditionVector ncV;
  EntityIdVector        enV;
  StringList            attrL;

  attrsStdVector2NotifyConditionVector(condAttributesV, &ncV);
  entIdStdVector2EntityIdVector(entitiesV, &enV);
  attrL.fill(notifAttributesV);

  BSONArray arr = processConditionVector(&ncV,
                                         enV,
                                         attrL,
                                         metadataV,
                                         subId,
                                         httpInfo,
                                         notificationDone,
                                         renderFormat,
                                         tenant,
                                         xauthToken,
                                         servicePathV,
                                         resP,
                                         status,
                                         fiwareCorrelator,
                                         attrsOrder,
                                         blacklist,
                                         skipInitialNotification,
                                         apiVersion);

  enV.release();
  ncV.release();

  return arr;
}



/* ****************************************************************************
*
* mongoUpdateCasubNewNotification -
*/
static HttpStatusCode mongoUpdateCasubNewNotification(std::string subId, std::string* err, std::string tenant)
{
  LM_T(LmtMongo, ("Update NGSI9 Subscription New Notification"));

  /* Update the document */
  BSONObj     query  = BSON("_id" << OID(subId));
  BSONObj     update = BSON("$set" << BSON(CASUB_LASTNOTIFICATION << getCurrentTime()) <<
                            "$inc" << BSON(CASUB_COUNT << 1));

  collectionUpdate(getSubscribeContextAvailabilityCollectionName(tenant), query, update, false, err);

  return SccOk;
}



/* ****************************************************************************
*
* processAvailabilitySubscription -
*
* This function is called from two places:
*
* 1) initial processing of subscribeContextAvailability (and updateContextAvailabilitySubscription),
*   so an "initial" notification for all matching context registrations is sent
* 2) registerContext processing logic when the new (or updated) context registration
*   matches an availability subscription
*
* The enV arguments is set with all the entities included in the subscription (case 1) or
* with only the triggering entities (case 2).
*
* This method returns true if the notification was actually send. Otherwise, false
* is returned.
*/
bool processAvailabilitySubscription
(
  const EntityIdVector& enV,
  const StringList&     attrL,
  const std::string&    subId,
  const std::string&    notifyUrl,
  RenderFormat          renderFormat,
  const std::string&    tenant,
  const std::string&    fiwareCorrelator
)
{
  std::string                       err;
  NotifyContextAvailabilityRequest  ncar;
  std::vector<std::string>          servicePathV;  // FIXME P5: servicePath for NGSI9 Subscriptions
  servicePathV.push_back("");                      // While this gets implemented, "" default is used.

  if (!registrationsQuery(enV, attrL, &ncar.contextRegistrationResponseVector, &err, tenant, servicePathV))
  {
    ncar.contextRegistrationResponseVector.release();
    return false;
  }

  if (ncar.contextRegistrationResponseVector.size() > 0)
  {
    /* Complete the fields in NotifyContextRequest */
    ncar.subscriptionId.set(subId);

    getNotifier()->sendNotifyContextAvailabilityRequest(&ncar, notifyUrl, tenant, fiwareCorrelator, renderFormat);
    ncar.contextRegistrationResponseVector.release();

    /* Update database fields due to new notification */
    if (mongoUpdateCasubNewNotification(subId, &err, tenant) != SccOk)
    {
      return false;
    }

    return true;
  }

  ncar.contextRegistrationResponseVector.release();
  return false;
}



/* ****************************************************************************
*
* slashEscape - escape all slashes in 'from' into 'to'
*
* If the 'to' buffer is not big enough, slashEscape returns with to's content set to 'ERROR'.
*/
void slashEscape(const char* from, char* to, unsigned int toLen)
{
  unsigned int ix      = 0;
  unsigned int slashes = 0;

  // 1. count number of slashes, to help to decide whether to return ERROR or not
  while (from[ix] != 0)
  {
    if (from[ix] == '/')
    {
      ++slashes;
    }

    ++ix;
  }

  // 2. If the escaped version of 'from' doesn't fit inside 'to', return ERROR as string
  if ((strlen(from) + slashes + 1) > toLen)
  {
    strncpy(to, "ERROR", toLen);
    return;
  }


  // 3. Copy 'in' to 'from', including escapes for '/'
  ix = 0;
  while (*from != 0)
  {
    if (ix >= toLen - 2)
    {
      return;
    }

    if (*from == '/')
    {
      to[ix++] = '\\';
    }

    to[ix++] = *from;
    ++from;
    to[ix] = 0;
  }
}



/* ****************************************************************************
*
* releaseTriggeredSubscriptions -
*/
void releaseTriggeredSubscriptions(std::map<std::string, TriggeredSubscription*>* subsP)
{
  for (std::map<std::string, TriggeredSubscription*>::iterator it = subsP->begin(); it != subsP->end(); ++it)
  {
    delete it->second;
  }

  subsP->clear();
}



/* ****************************************************************************
*
* fillContextProviders -
*
* This functions is very similar the one with the same name in mongoQueryContext.cpp
* but acting on a single CER instead that on a complete vector of them.
*
* Looks in the CER passed as argument, searching for a suitable CPr in the CRR
* vector passed as argument. If a suitable CPr is found, it is set in the CER (and the 'found' field
* is changed to true)
*/
void fillContextProviders(ContextElementResponse* cer, const ContextRegistrationResponseVector& crrV)
{
  for (unsigned int ix = 0; ix < cer->entity.attributeVector.size(); ++ix)
  {
    ContextAttribute* ca = cer->entity.attributeVector[ix];

    if (ca->found)
    {
      continue;
    }

    /* Search for some CPr in crrV */
    std::string     perEntPa;
    std::string     perAttrPa;
    MimeType        perEntPaMimeType  = NOMIMETYPE;
    MimeType        perAttrPaMimeType = NOMIMETYPE;
    ProviderFormat  providerFormat;

    cprLookupByAttribute(cer->entity,
                         ca->name,
                         crrV,
                         &perEntPa,
                         &perEntPaMimeType,
                         &perAttrPa,
                         &perAttrPaMimeType,
                         &providerFormat);

    /* Looking results after crrV processing */
    ca->providingApplication.set(perAttrPa == "" ? perEntPa : perAttrPa);
    ca->providingApplication.setProviderFormat(providerFormat);
    ca->found = (ca->providingApplication.get() != "");
  }
}



/* ****************************************************************************
*
* someContextElementNotFound -
*
* This functions is very similar the one with the same name in mongoQueryContext.cpp
* but acting on a single CER instead that on a complete vector of them.
*
* Returns true if some attribute with 'found' set to 'false' is found in the CER passed
* as argument
*/
bool someContextElementNotFound(const ContextElementResponse& cer)
{
  for (unsigned int ix = 0; ix < cer.entity.attributeVector.size(); ++ix)
  {
    if (!cer.entity.attributeVector[ix]->found)
    {
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* cprLookupByAttribute -
*
* Search for the CPr, given the entity/attribute as argument. Actually, two CPrs can be returned -
* the "general" one at entity level or the "specific" one at attribute level
*/
void cprLookupByAttribute
(
  const Entity&                             en,
  const std::string&                        attrName,
  const ContextRegistrationResponseVector&  crrV,
  std::string*                              perEntPa,
  MimeType*                                 perEntPaMimeType,
  std::string*                              perAttrPa,
  MimeType*                                 perAttrPaMimeType,
  ProviderFormat*                           providerFormatP
)
{
  *perEntPa  = "";
  *perAttrPa = "";

  for (unsigned int crrIx = 0; crrIx < crrV.size(); ++crrIx)
  {
    ContextRegistrationResponse* crr = crrV[crrIx];

    /* Is there a matching entity in the CRR? */
    for (unsigned enIx = 0; enIx < crr->contextRegistration.entityIdVector.size(); ++enIx)
    {
      EntityId* regEn = crr->contextRegistration.entityIdVector[enIx];

      if (regEn->isPatternIsTrue() && regEn->id == ".*")
      {
        // By the moment the only supported pattern is .*. In this case matching is
        // based exclusively in type
        if (regEn->type != en.type && regEn->type != "")
        {
          /* No match (keep searching the CRR) */
          continue;
        }
      }
      else if (regEn->id != en.id || (regEn->type != en.type && regEn->type != ""))
      {
        /* No match (keep searching the CRR) */
        continue;
      }

      /* CRR without attributes (keep searching in other CRR) */
      if (crr->contextRegistration.contextRegistrationAttributeVector.size() == 0)
      {
        *perEntPa         = crr->contextRegistration.providingApplication.get();
        *providerFormatP  =  crr->contextRegistration.providingApplication.getProviderFormat();

        break;  /* enIx */
      }

      /* Is there a matching entity or the absence of attributes? */
      for (unsigned attrIx = 0; attrIx < crr->contextRegistration.contextRegistrationAttributeVector.size(); ++attrIx)
      {
        std::string regAttrName = crr->contextRegistration.contextRegistrationAttributeVector[attrIx]->name;
        if (regAttrName == attrName)
        {
          /* We cannot "improve" this result by keep searching the CRR vector, so we return */
          *perAttrPa         = crr->contextRegistration.providingApplication.get();
          *providerFormatP  =  crr->contextRegistration.providingApplication.getProviderFormat();

          return;
        }
      }
    }
  }
}
