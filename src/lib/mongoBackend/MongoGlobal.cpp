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
#include "alarmMgr/alarmMgr.h"

#include "orionTypes/OrionValueType.h"

#include "mongoBackend/mongoConnectionPool.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/compoundResponses.h"

#include "ngsi/EntityIdVector.h"
#include "ngsi/AttributeList.h"
#include "ngsi/ContextElementResponseVector.h"
#include "ngsi/Duration.h"
#include "ngsi/Restriction.h"
#include "ngsiNotify/Notifier.h"
#include "rest/StringFilter.h"

using namespace mongo;
using std::auto_ptr;



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
* mongoMultitenant - 
*/
bool mongoMultitenant(void)
{
  return multitenant;
}



/* ****************************************************************************
*
* shutdownClient -
*/
static void shutdownClient(void)
{
  mongo::Status status = mongo::client::shutdown();
  if (!status.isOK())
  {
    LM_E(("Database Shutdown Error %s (cannot shutdown mongo client)", status.toString().c_str()));
  }
}


/* ****************************************************************************
*
* mongoStart -
*
* This function must be called just once, because of the intialization of mongo::client
* and the creation of the connection pool.
*/
bool mongoStart
(
  const char*  host,
  const char*  db,
  const char*  rplSet,
  const char*  username,
  const char*  passwd,
  bool         _multitenant,
  double       timeout,
  int          writeConcern,
  int          poolSize,
  bool         semTimeStat
)
{
  static bool alreadyDone = false;

  if (alreadyDone == true)
  {
    LM_E(("Runtime Error (mongoStart already called - can only be called once)"));
    return false;
  }
  alreadyDone = true;

  multitenant = _multitenant;

  mongo::Status status = mongo::client::initialize();
  if (!status.isOK())
  {
    LM_E(("Database Startup Error %s (cannot initialize mongo client)", status.toString().c_str()));
    return false;
  }
  atexit(shutdownClient);

  if (mongoConnectionPoolInit(host,
                              db,
                              rplSet,
                              username,
                              passwd,
                              _multitenant,
                              timeout,
                              writeConcern,
                              poolSize,
                              semTimeStat) != 0)
  {
    LM_E(("Database Startup Error (cannot initialize mongo connection pool)"));
    return false;
  }

  return true;
}





#ifdef UNIT_TEST

static DBClientBase* connection = NULL;


/* ****************************************************************************
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
*
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
*
*/
void releaseMongoConnection(DBClientBase* connection)
{
#ifdef UNIT_TEST
  return;
#else
  return mongoPoolConnectionRelease(connection);
#endif // UNIT_TEST
}


/*****************************************************************************
*
* getNotifier -
*/
Notifier* getNotifier()
{
  return notifier;
}


/*****************************************************************************
*
* setNotifier -
*/
void setNotifier(Notifier* n)
{
  notifier = n;
}


/*****************************************************************************
*
* setDbPrefix -
*/
void setDbPrefix(const std::string& _dbPrefix)
{
  dbPrefix = _dbPrefix;
  LM_T(LmtBug, ("Set dbPrefix to '%s'", dbPrefix.c_str()));
}


/*****************************************************************************
*
* getDbPrefix -
*/
const std::string& getDbPrefix(void)
{
  return dbPrefix;
}



/*****************************************************************************
*
* getOrionDatabases -
*/
extern bool getOrionDatabases(std::vector<std::string>& dbs)
{
  BSONObj       result;
  std::string   err;
  if (!runCollectionCommand("admin", BSON("listDatabases" << 1), &result, &err))
  {
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
      dbs.push_back(dbName);
      LM_T(LmtBug, ("Pushed back db name '%s'", dbName.c_str()));
    }
  }

  return true;

}

/*****************************************************************************
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


/*****************************************************************************
*
* setEntitiesCollectionName -
*/
void setEntitiesCollectionName(const std::string& name)
{
  entitiesCollectionName = name;
}


/*****************************************************************************
*
* setRegistrationsCollectionName -
*/
void setRegistrationsCollectionName(const std::string& name)
{
  registrationsCollectionName = name;
}


/*****************************************************************************
*
* setSubscribeContextCollectionName -
*/
void setSubscribeContextCollectionName(const std::string& name)
{
  subscribeContextCollectionName = name;
}


/*****************************************************************************
*
* setSubscribeContextAvailabilityCollectionName -
*/
void setSubscribeContextAvailabilityCollectionName(const std::string& name)
{
  subscribeContextAvailabilityCollectionName = name;
}


/*****************************************************************************
*
* composeCollectionName -
*
* Common helper function for composing collection names
*/
static std::string composeCollectionName(std::string tenant, std::string colName)
{
  return composeDatabaseName(tenant) + "." + colName;
}


/*****************************************************************************
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


/*****************************************************************************
*
* getEntitiesCollectionName -
*/
std::string getEntitiesCollectionName(const std::string& tenant)
{
  return composeCollectionName(tenant, entitiesCollectionName);
}


/*****************************************************************************
*
* getRegistrationsCollectionName -
*/
std::string getRegistrationsCollectionName(const std::string& tenant)
{
  return composeCollectionName(tenant, registrationsCollectionName);
}


/*****************************************************************************
*
* getSubscribeContextCollectionName -
*/
std::string getSubscribeContextCollectionName(const std::string& tenant)
{
  return composeCollectionName(tenant, subscribeContextCollectionName);
}


/*****************************************************************************
*
* getSubscribeContextAvailabilityCollectionName -
*/
std::string getSubscribeContextAvailabilityCollectionName(const std::string& tenant)
{
  return composeCollectionName(tenant, subscribeContextAvailabilityCollectionName);
}


/*****************************************************************************
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


/*****************************************************************************
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
    collectionCreateIndex(getEntitiesCollectionName(tenant), BSON(index << "2dsphere"), &err);
    LM_T(LmtMongo, ("ensuring 2dsphere index on %s (tenant %s)", index.c_str(), tenant.c_str()));
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
    if (regcomp(&regex, en2->id.c_str(), 0) != 0)
    {
      std::string details = std::string("error compiling regex for id: '") + en2->id + "'";
      alarmMgr.badInput(clientIp, details);
    }
    else
    {
      idMatch = (regexec(&regex, en1->id.c_str(), 0, NULL, 0) == 0);
    }

    regfree(&regex);
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
    if (matchEntity(&en, entityIdV[ix]))
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
bool includedAttribute(const ContextRegistrationAttribute& attr, const AttributeList& attrsV)
{
  // This i's the case in which the discoverAvailabilityRequest doesn't include attributes,
  // so all the attributes are included in the response
  //
  if (attrsV.size() == 0)
  {
    return true;
  }

  for (unsigned int ix = 0; ix < attrsV.size(); ++ix)
  {
    if (attrsV[ix] == attr.name)
    {
      return true;
    }
  }

  return false;
}


/* ****************************************************************************
*
* fillQueryEntity -
*
*/
static void fillQueryEntity(BSONArrayBuilder& ba, const EntityId* enP)
{
  BSONObjBuilder     ent;
  const std::string  idString          = "_id." ENT_ENTITY_ID;
  const std::string  typeString        = "_id." ENT_ENTITY_TYPE;

  if (enP->isPattern == "true")
  {
    ent.appendRegex(idString, enP->id);
  }
  else
  {
    ent.append(idString, enP->id);
  }

  if (enP->type != "")
  {
    ent.append(typeString, enP->type);
  }

  BSONObj entObj = ent.obj();
  ba.append(entObj);

  LM_T(LmtMongo, ("Entity query token: '%s'", entObj.toString().c_str()));
}


/* ****************************************************************************
*
* fillQueryServicePath -
*
* The regular expression for servicePath.
*
* If the servicePath is empty, then we return all entities, no matter their servicePath. This
* can be seen as a query on "/#" considering that entities without servicePath are implicitly
* assigned to "/" service path.
*/
BSONObj fillQueryServicePath(const std::vector<std::string>& servicePath)
{
  /* Due to limitations in the BSONArrayBuilder class (that hopefully will be solved in legacy-1.0.0
   * MongoDB driver) we need to compose the JSON string, then apply fromjson() function. Current
   * implementation uses an array element for each regex tokens, but we could use only
   * one element, concatenating all regex tokens using "|" (not sure what is more efficient from a
   * computational point of view). However, note that we need the $in array as "null" has to be added
   * as element in any case.
   *
   * More information on: http://stackoverflow.com/questions/24243276/include-regex-elements-in-bsonarraybuilder
   *
   */  

  // Note that by construction servicePath vector must have at least one element. Note that the
  // case in which first element is "" is special, it means that the SP were not provided and
  // we have to apply the default
  if (servicePath[0] == "")
  {
    LM_T(LmtServicePath, ("Service Path JSON string: '{$in: [ /^\\/.*/, null] }'"));
    return fromjson("{$in: [ /^\\/.*/, null] }");
  }

  std::string servicePathValue = "{ $in: [ ";
  bool nullAdded = false;

  for (unsigned int ix = 0 ; ix < servicePath.size(); ++ix)
  {

    //
    // Add "null" in the following service path cases: / or /#. In order to avoid adding null
    // several times, the nullAdded flag is used
    //
    if (!nullAdded && ((servicePath[ix] == "/") || (servicePath[ix] == "/#")))
    {
      servicePathValue += "null, ";
      nullAdded = true;
    }

    char path[SERVICE_PATH_MAX_TOTAL * 2];
    slashEscape(servicePath[ix].c_str(), path, sizeof(path));

    if (path[strlen(path) - 1] == '#')
    {
      /* Remove '\/#' trailing part of the string */
      int l = strlen(path);

      path[l - 3] = 0;
      servicePathValue += std::string("/^") + path + "$/, " + std::string("/^") + path + "\\/.*/";
    }
    else
    {
      servicePathValue += std::string("/^") + path + "$/";
    }

    /* Prepare concatenation for next token in regex */
    if (ix < servicePath.size() - 1)
    {
      servicePathValue += std::string(", ");
    }
  }

  servicePathValue += " ] }";
  LM_T(LmtServicePath, ("Service Path JSON string: '%s'", servicePathValue.c_str()));

  return fromjson(servicePathValue);

}

/* *****************************************************************************
*
* processAreaScope -
*
* Returns true if areaQuery was filled, false otherwise
*
*/
static bool processAreaScope(const Scope* scoP, BSONObj &areaQuery)
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
    areaQuery = BSON("$exists" << true << "$not" << BSON("$geoWithin" << geoWithin));
  }
  else
  {
    areaQuery = BSON("$geoWithin" << geoWithin);
  }

  return true;
}


/* *****************************************************************************
*
* processAreaScopeV2 -
*
* Returns true if areaQuery was filled, false otherwise
*
*/
static bool processAreaScopeV2(const Scope* scoP, BSONObj &areaQuery)
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
    geometry = BSON("type" << "Point" << "coordinates" << BSON_ARRAY(scoP->point.longitude() << scoP->point.latitude()));
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
    areaQuery = BSON("$near" << near.obj());
  }
  else if (scoP->georel.type == "coveredBy")
  {
    areaQuery = BSON("$geoWithin" << BSON("$geometry" << geometry));
  }
  else if (scoP->georel.type == "intersects")
  {
    areaQuery = BSON("$geoIntersects" << BSON("$geometry" << geometry));
  }
  else if (scoP->georel.type == "disjoint")
  {
    areaQuery = BSON("$exists" << true << "$not" << BSON("$geoIntersects" << BSON("$geometry" << geometry)));
  }
  else if (scoP->georel.type == "equals")
  {
    areaQuery = geometry;
  }
  else
  {
    LM_E(("Runtime Error (unknown georel type: '%s')", scoP->georel.type.c_str()));
    return false;
  }

  return true;
}


/* *****************************************************************************
*
* addFilterScope -
*/
static void addFilterScope(const Scope* scoP, std::vector<BSONObj> &filters)
{
  std::string entityTypeString = std::string("_id.") + ENT_ENTITY_TYPE;

  if (scoP->type == SCOPE_FILTER_EXISTENCE)
  {
    if (scoP->value == SCOPE_VALUE_ENTITY_TYPE)
    {
      BSONObj b = scoP->oper == SCOPE_OPERATOR_NOT ?
            BSON(entityTypeString << BSON("$exists" << false)) :
            BSON(entityTypeString << BSON("$exists" << true));
      filters.push_back(b);
    }
    else
    {
      std::string details = std::string("unknown value for '") + SCOPE_FILTER_EXISTENCE + "' filter: '" + scoP->value + "'";
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
*
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

  return std::string(ENT_ATTRS) + "." + sortToken + "." + ENT_ATTRS_VALUE;
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
  const AttributeList&             attrL,
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
  bool*                            badInputP,
  const std::string&               sortOrderList,
  bool                             includeCreDate,
  bool                             includeModDate,
  const std::string&               apiVersion
)
{
  /* Query structure is as follows
   *
   * {
   *    "$or": [ ... ],            (always)
   *    "_id.servicePath: { ... }  (always, in some cases using {$exists: false})
   *    "attrNames": { ... },      (only if attributes are used in the query)
   *    "location.coords": { ... } (only in the case of geo-queries)
   *  }
   *
   */

  BSONObjBuilder    finalQuery;
  BSONArrayBuilder  orEnt;

  /* Part 1: entities */

  for (unsigned int ix = 0; ix < enV.size(); ++ix)
  {
    fillQueryEntity(orEnt, enV[ix]);
  }

  /* The result of orEnt is appended to the final query */
  finalQuery.append("$or", orEnt.arr());

  /* Part 2: service path */
  const std::string  servicePathString = "_id." ENT_SERVICE_PATH;
  finalQuery.append(servicePathString, fillQueryServicePath(servicePath));

  /* Part 3: attributes */
  BSONArrayBuilder attrs;
  for (unsigned int ix = 0; ix < attrL.size(); ++ix)
  {
    std::string attrName = attrL[ix];

    attrs.append(attrName);
    LM_T(LmtMongo, ("Attribute query token: '%s'", attrName.c_str()));
  }

  if (attrs.arrSize() > 0)
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
      // FIXME P5: NGSI "v1" filter, probably to be removed in the future
      addFilterScope(scopeP, filters);
    }
    else if (scopeP->type == FIWARE_LOCATION || scopeP->type == FIWARE_LOCATION_DEPRECATED || scopeP->type == FIWARE_LOCATION_V2)
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
          result = processAreaScopeV2(scopeP, areaQuery);
        }
        else // FIWARE Location NGSIv1 (legacy)
        {
          result = processAreaScope(scopeP, areaQuery);
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
      for (unsigned int ix = 0; ix < scopeP->stringFilter.mongoFilters.size(); ++ix)
      {
        finalQuery.appendElements(scopeP->stringFilter.mongoFilters[ix]);
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
  auto_ptr<DBClientCursor>  cursor;
  Query                     query(finalQuery.obj());

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
    int components = stringSplit(sortOrderList, ',', sortedV);
    BSONObjBuilder sortOrder;
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
      const char* defaultErrorString  = "Error at querying MongoDB";

      alarmMgr.dbError(exErr);

      if (strncmp(exErr.c_str(), invalidPolygon, strlen(invalidPolygon)) == 0)
      {
        exErr = invalidPolygon;
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
        cer->contextElement.entityId.fill(enV[0]);
      }
      else
      {
        cer->contextElement.entityId.fill("", "", "");
      }

      cer->statusCode.fill(SccReceiverInternalError, exErr);
      cerV->push_back(cer);
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
    ContextElementResponse*  cer = new ContextElementResponse(r, attrL, includeEmpty, includeCreDate, includeModDate, apiVersion);
    cer->statusCode.fill(SccOk);

    /* All the attributes existing in the request but not found in the response are added with 'found' set to false */
    for (unsigned int ix = 0; ix < attrL.size(); ++ix)
    {
      bool         found     = false;
      std::string  attrName  = attrL[ix];

      for (unsigned int jx = 0; jx < cer->contextElement.contextAttributeVector.size(); ++jx)
      {
        if (attrName == cer->contextElement.contextAttributeVector[jx]->name)
        {
          found = true;
          break;
        }
      }

      if (!found)
      {
        ContextAttribute* caP = new ContextAttribute(attrName, "", "", false);
        cer->contextElement.contextAttributeVector.push_back(caP);
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
        EntityId eP = (*cerV)[jx]->contextElement.entityId;

        if ((eP.id == enV[ix]->id) && (eP.type == enV[ix]->type))
        {
          needToAdd = false;
          break;  /* jx */
        }
      }

      if (needToAdd)
      {
        ContextElementResponse* cerP = new ContextElementResponse();

        cerP->contextElement.entityId.id = enV[ix]->id;
        cerP->contextElement.entityId.type = enV[ix]->type;
        cerP->contextElement.entityId.isPattern = "false";

        //
        // This entity has to be pruned if after CPr searching no attribute is "added" to it.
        // The prune attribute distinguish this kind of entities from the "real ones" that are
        // without attributes in the Orion local database (a rare case, but may happen)
        //
        cerP->prune = true;

        for (unsigned int jx = 0; jx < attrL.size(); ++jx)
        {
          ContextAttribute* caP = new ContextAttribute(attrL[jx], "", "", false);

          cerP->contextElement.contextAttributeVector.push_back(caP);
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
*/
void pruneContextElements(ContextElementResponseVector& oldCerV, ContextElementResponseVector* newCerVP)
{
  for (unsigned int ix = 0; ix < oldCerV.size(); ++ix)
  {
    ContextElementResponse* cerP = oldCerV[ix];
    ContextElementResponse* newCerP = new ContextElementResponse();

    /* Note we cannot use the ContextElement::fill() method, given that it also copies the ContextAttributeVector. The side-effect
     * of this is that attributeDomainName and domainMetadataVector are not being copied, but it should not be a problem, given that
     * domain attributes are not implemented */
    newCerP->contextElement.entityId.fill(&cerP->contextElement.entityId);

    // FIXME P10: not sure if this is the right way to do it, maybe we need a fill() method for this
    newCerP->contextElement.providingApplicationList = cerP->contextElement.providingApplicationList;
    newCerP->statusCode.fill(&cerP->statusCode);

    bool pruneEntity = cerP->prune;

    for (unsigned int jx = 0; jx < cerP->contextElement.contextAttributeVector.size(); ++jx)
    {
      ContextAttribute* caP = cerP->contextElement.contextAttributeVector[jx];

      if (caP->found)
      {
        ContextAttribute* newCaP = new ContextAttribute(caP);
        newCerP->contextElement.contextAttributeVector.push_back(newCaP);
      }
    }

    /* If after pruning the entity has no attribute and no CPr information, then it is not included
     * in the output vector, except if "prune" is set to false */
    if (pruneEntity &&
        (newCerP->contextElement.contextAttributeVector.size()   == 0) &&
        (newCerP->contextElement.providingApplicationList.size() == 0))
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


/*****************************************************************************
*
* processEntity -
*/
static void processEntity(ContextRegistrationResponse* crr, const EntityIdVector& enV, BSONObj entity)
{
  EntityId en;

  en.id = getStringFieldF(entity, REG_ENTITY_ID);
  en.type = entity.hasField(REG_ENTITY_TYPE) ? getStringFieldF(entity, REG_ENTITY_TYPE) : "";

  /* isPattern = true is not allowed in registrations so it is not in the
   * document retrieved with the query; however we will set it to be formally correct
   * with NGSI spec
   */
  en.isPattern = std::string("false");

  if (includedEntity(en, enV))
  {
    EntityId* enP = new EntityId(en.id, en.type, en.isPattern);

    crr->contextRegistration.entityIdVector.push_back(enP);
  }
}


/*****************************************************************************
*
* processAttribute -
*/
static void processAttribute(ContextRegistrationResponse* crr, const AttributeList& attrL, const BSONObj& attribute)
{
  ContextRegistrationAttribute attr(
    getStringFieldF(attribute, REG_ATTRS_NAME),
    getStringFieldF(attribute, REG_ATTRS_TYPE),
    getStringFieldF(attribute, REG_ATTRS_ISDOMAIN));

  // FIXME: we don't take metadata into account at the moment
  // attr.metadataV = ..

  if (includedAttribute(attr, attrL))
  {
    ContextRegistrationAttribute* attrP = new ContextRegistrationAttribute(attr.name, attr.type, attr.isDomain);
    crr->contextRegistration.contextRegistrationAttributeVector.push_back(attrP);
  }
}


/*****************************************************************************
*
* processContextRegistrationElement -
*/
static void processContextRegistrationElement
(
  BSONObj                             cr,
  const EntityIdVector&               enV,
  const AttributeList&                attrL,
  ContextRegistrationResponseVector*  crrV,
  Format                              format
)
{
  ContextRegistrationResponse crr;

  crr.contextRegistration.providingApplication.set(getStringFieldF(cr, REG_PROVIDING_APPLICATION));
  crr.contextRegistration.providingApplication.setFormat(format);

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
*
*/
bool registrationsQuery
(
  const EntityIdVector&               enV,
  const AttributeList&                attrL,
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

  /* Build query based on arguments */
  // FIXME P2: this implementation needs to be refactored for cleanup
  std::string       contextRegistrationEntities     = REG_CONTEXT_REGISTRATION "." REG_ENTITIES;
  std::string       contextRegistrationEntitiesId   = REG_CONTEXT_REGISTRATION "." REG_ENTITIES "." REG_ENTITY_ID;
  std::string       contextRegistrationEntitiesType = REG_CONTEXT_REGISTRATION "." REG_ENTITIES "." REG_ENTITY_TYPE;
  std::string       contextRegistrationAttrsNames   = REG_CONTEXT_REGISTRATION "." REG_ATTRS    "." REG_ATTRS_NAME;
  BSONArrayBuilder  entityOr;
  BSONArrayBuilder  entitiesWithType;
  BSONArrayBuilder  entitiesWithoutType;

  for (unsigned int ix = 0; ix < enV.size(); ++ix)
  {
    const EntityId* en = enV[ix];

    if (isTrue(en->isPattern))
    {
      BSONObjBuilder b;

      b.appendRegex(contextRegistrationEntitiesId, en->id);
      if (en->type != "")
      {
        b.append(contextRegistrationEntitiesType, en->type);
      }
      entityOr.append(b.obj());
    }
    else  /* isPattern = false */
    {
      if (en->type == "")
      {
        entitiesWithoutType.append(en->id);
        LM_T(LmtMongo, ("Entity discovery without type: id '%s'", en->id.c_str()));
      }
      else
      {
        /* We have detected that sometimes mongo stores { id: ..., type ...} and others { type: ..., id: ...},
           so we have to take both them into account
        */
        entitiesWithType.append(BSON(REG_ENTITY_ID << en->id << REG_ENTITY_TYPE << en->type));
        entitiesWithType.append(BSON(REG_ENTITY_TYPE << en->type << REG_ENTITY_ID << en->id));
        LM_T(LmtMongo, ("Entity discovery: {id: %s, type: %s}", en->id.c_str(), en->type.c_str()));
      }
    }
  }

  BSONArrayBuilder attrs;

  for (unsigned int ix = 0; ix < attrL.size(); ++ix)
  {
    std::string attrName = attrL[ix];

    attrs.append(attrName);
    LM_T(LmtMongo, ("Attribute discovery: '%s'", attrName.c_str()));
  }

  entityOr.append(BSON(contextRegistrationEntities << BSON("$in" << entitiesWithType.arr())));
  entityOr.append(BSON(contextRegistrationEntitiesId << BSON("$in" <<entitiesWithoutType.arr())));

  BSONObjBuilder queryBuilder;

  /* The $or clause could be omitted if it contains only one element, but we can assume that
   * it has no impact on MongoDB query optimizer
   */
  queryBuilder.append("$or", entityOr.arr());
  queryBuilder.append(REG_EXPIRATION, BSON("$gt" << (long long) getCurrentTime()));

  if (attrs.arrSize() > 0)
  {
    /* If we don't do this checking, the {$in: [] } in the attribute name part will
     * make the query fail
     */
    queryBuilder.append(contextRegistrationAttrsNames, BSON("$in" << attrs.arr()));
  }

  //
  // 'And-in' the service path
  //
  const std::string  servicePathString = REG_SERVICE_PATH;

  queryBuilder.append(servicePathString, fillQueryServicePath(servicePathV));


  //
  // Do the query on MongoDB
  // FIXME P2: Use field selector to include the only relevant field:
  //           contextRegistration array (e.g. "expiration" is not needed)
  //
  auto_ptr<DBClientCursor>  cursor;
  Query                     query(queryBuilder.obj());

  query.sort(BSON("_id" << 1));

  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();
  if (!collectionRangedQuery(connection, getRegistrationsCollectionName(tenant), query, limit, offset, &cursor, countP, err))
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

    Format                    format = JSON;
    std::vector<BSONElement>  queryContextRegistrationV = getFieldF(r, REG_CONTEXT_REGISTRATION).Array();

    for (unsigned int ix = 0 ; ix < queryContextRegistrationV.size(); ++ix)
    {
      processContextRegistrationElement(queryContextRegistrationV[ix].embeddedObject(), enV, attrL, crrV, format);
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
      ContextAttributeVector caV = (*cerV)[aclx]->contextElement.contextAttributeVector;

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
* someEmptyCondValue -
*
* This logic would be MUCH simpler in the case conditions was a single field instead of a vector.
*/
bool someEmptyCondValue(const BSONObj& sub)
{
  std::vector<BSONElement>  conds = getFieldF(sub, CSUB_CONDITIONS).Array();

  for (unsigned int ix = 0; ix < conds.size() ; ++ix)
  {
    BSONObj cond = conds[ix].embeddedObject();
    if (getFieldF(cond, CSUB_CONDITIONS_VALUE).Array().size() == 0)
    {
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* condValueAttrMatch -
*
* This logic would be MUCH simpler in the case conditions was a single field instead of a vector.
*/
bool condValueAttrMatch(const BSONObj& sub, const std::vector<std::string>& modifiedAttrs)
{
  std::vector<BSONElement>  conds = getFieldF(sub, CSUB_CONDITIONS).Array();

  for (unsigned int ix = 0; ix < conds.size() ; ++ix)
  {
    BSONObj cond = conds[ix].embeddedObject();
    std::vector<BSONElement>  condValues = getFieldF(cond, CSUB_CONDITIONS_VALUE).Array();
    for (unsigned int jx = 0; jx < condValues.size() ; ++jx)
    {
      std::string condValue = condValues[jx].String();
      for (unsigned int kx = 0; kx < modifiedAttrs.size(); ++kx)
      {
        if (condValue == modifiedAttrs[kx])
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
* subToEntityIdVector -
*
* Extract the entity ID vector from a BSON document (in the format of the csubs/casub
* collection)
*
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
* subToAttributeList -
*
* Extract the attribute list from a BSON document (in the format of the csubs/casub
* collection)
*
*/
AttributeList subToAttributeList(const BSONObj& sub)
{
  AttributeList             attrL;
  std::vector<BSONElement>  subAttrs = getFieldF(sub, CSUB_ATTRS).Array();

  for (unsigned int ix = 0; ix < subAttrs.size() ; ++ix)
  {
    std::string subAttr = subAttrs[ix].String();

    attrL.push_back(subAttr);
  }

  return attrL;
}


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
bool processOnChangeConditionForSubscription
(
  const EntityIdVector&            enV,
  const AttributeList&             attrL,
  ConditionValueList*              condValues,
  const std::string&               subId,
  const std::string&               notifyUrl,
  Format                           format,
  const std::string&               tenant,
  const std::string&               xauthToken,
  const std::vector<std::string>&  servicePathV,
  Restriction*                     resP,
  const std::string&               fiwareCorrelator
)
{
  std::string                   err;
  NotifyContextRequest          ncr;
  ContextElementResponseVector  rawCerV;

  if (!entitiesQuery(enV, attrL, *resP, &rawCerV, &err, true, tenant, servicePathV))
  {
    ncr.contextElementResponseVector.release();
    rawCerV.release();

    return false;
  }

  /* Prune "not found" CERs */
  pruneContextElements(rawCerV, &ncr.contextElementResponseVector);
  rawCerV.release();

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
      AttributeList                 emptyList;

      if (!entitiesQuery(enV, emptyList, *resP, &rawCerV, &err, false, tenant, servicePathV))
      {
        rawCerV.release();
        ncr.contextElementResponseVector.release();

        return false;
      }

      /* Prune "not found" CERs */
      pruneContextElements(rawCerV, &allCerV);
      rawCerV.release();

      if (isCondValueInContextElementResponse(condValues, &allCerV))
      {
        /* Send notification */
        getNotifier()->sendNotifyContextRequest(&ncr, notifyUrl, tenant, xauthToken, fiwareCorrelator, format);
        allCerV.release();
        ncr.contextElementResponseVector.release();

        return true;
      }

      allCerV.release();
    }
    else
    {
      getNotifier()->sendNotifyContextRequest(&ncr, notifyUrl, tenant, xauthToken, fiwareCorrelator, format);
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
*
*/
BSONArray processConditionVector
(
  NotifyConditionVector*           ncvP,
  const EntityIdVector&            enV,
  const AttributeList&             attrL,
  const std::string&               subId,
  const std::string&               url,
  bool*                            notificationDone,
  Format                           format,
  const std::string&               tenant,
  const std::string&               xauthToken,
  const std::vector<std::string>&  servicePathV,
  Restriction*                     resP,
  const std::string&               status,
  const std::string&               fiwareCorrelator
)
{
  BSONArrayBuilder conds;

  *notificationDone = false;

  for (unsigned int ix = 0; ix < ncvP->size(); ++ix)
  {
    NotifyCondition* nc = (*ncvP)[ix];

    if (nc->type == ON_CHANGE_CONDITION)
    {
      /* Create an array holding the list of condValues */
      BSONArrayBuilder condValues;

      for (unsigned int jx = 0; jx < nc->condValueList.size(); ++jx)
      {
        condValues.append(nc->condValueList[jx]);
      }

      conds.append(BSON(CSUB_CONDITIONS_TYPE << ON_CHANGE_CONDITION <<
                        CSUB_CONDITIONS_VALUE << condValues.arr()
                        ));

      if ((status == STATUS_ACTIVE) &&
          (processOnChangeConditionForSubscription(enV,
                                                   attrL,
                                                   &(nc->condValueList),
                                                   subId,
                                                   url,
                                                   format,
                                                   tenant,
                                                   xauthToken,
                                                   servicePathV,
                                                   resP,
                                                   fiwareCorrelator)))
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
* mongoUpdateCasubNewNotification -
*
*/
static HttpStatusCode mongoUpdateCasubNewNotification(std::string subId, std::string* err, std::string tenant)
{
  LM_T(LmtMongo, ("Update NGSI9 Subscription New Notification"));

  /* Update the document */
  BSONObj     query  = BSON("_id" << OID(subId));
  BSONObj     update = BSON("$set" << BSON(CASUB_LASTNOTIFICATION << getCurrentTime()) << "$inc" << BSON(CASUB_COUNT << 1));
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
  const AttributeList&  attrL,
  const std::string&    subId,
  const std::string&    notifyUrl,
  Format                format,
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

    getNotifier()->sendNotifyContextAvailabilityRequest(&ncar, notifyUrl, tenant, fiwareCorrelator, format);
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
*
*/
void releaseTriggeredSubscriptions(std::map<std::string, TriggeredSubscription*>& subs)
{
  for (std::map<std::string, TriggeredSubscription*>::iterator it = subs.begin(); it != subs.end(); ++it)
  {
    delete it->second;
  }

  subs.clear();
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
void fillContextProviders(ContextElementResponse* cer, ContextRegistrationResponseVector& crrV)
{
  for (unsigned int ix = 0; ix < cer->contextElement.contextAttributeVector.size(); ++ix)
  {
    ContextAttribute* ca = cer->contextElement.contextAttributeVector[ix];

    if (ca->found)
    {
      continue;
    }

    /* Search for some CPr in crrV */
    std::string  perEntPa;
    std::string  perAttrPa;
    Format       perEntPaFormat = NOFORMAT;
    Format       perAttrPaFormat= NOFORMAT;

    cprLookupByAttribute(cer->contextElement.entityId,
                         ca->name,
                         crrV,
                         &perEntPa,
                         &perEntPaFormat,
                         &perAttrPa,
                         &perAttrPaFormat);

    /* Looking results after crrV processing */
    ca->providingApplication.set(perAttrPa == "" ? perEntPa : perAttrPa);
    ca->providingApplication.setFormat(perAttrPa == "" ? perEntPaFormat : perAttrPaFormat);
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
*
*/
bool someContextElementNotFound(ContextElementResponse& cer)
{
  for (unsigned int ix = 0; ix < cer.contextElement.contextAttributeVector.size(); ++ix)
  {
    if (!cer.contextElement.contextAttributeVector[ix]->found)
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
* Search for the CPr, given the entity/attribute as argument. Actually, two CPrs can be returned
* the "general" one at entity level or the "specific" one at attribute level
*
*/
void cprLookupByAttribute
(
  EntityId&                           en,
  const std::string&                  attrName,
  ContextRegistrationResponseVector&  crrV,
  std::string*                        perEntPa,
  Format*                             perEntPaFormat,
  std::string*                        perAttrPa,
  Format*                             perAttrPaFormat
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

      if (regEn->id != en.id || (regEn->type != en.type && regEn->type != ""))
      {
        /* No match (keep searching the CRR) */
        continue;
      }

      /* CRR without attributes (keep searching in other CRR) */
      if (crr->contextRegistration.contextRegistrationAttributeVector.size() == 0)
      {
        *perEntPa       = crr->contextRegistration.providingApplication.get();
        *perEntPaFormat = crr->contextRegistration.providingApplication.getFormat();

        break; /* enIx */
      }

      /* Is there a matching entity or the absence of attributes? */
      for (unsigned attrIx = 0; attrIx < crr->contextRegistration.contextRegistrationAttributeVector.size(); ++attrIx)
      {
        std::string regAttrName = crr->contextRegistration.contextRegistrationAttributeVector[attrIx]->name;
        if (regAttrName == attrName)
        {
          /* We cannot "improve" this result keep searching in CRR vector, so we return */
          *perAttrPa       = crr->contextRegistration.providingApplication.get();
          *perAttrPaFormat = crr->contextRegistration.providingApplication.getFormat();

          return;
        }
      }
    }
  }
}
