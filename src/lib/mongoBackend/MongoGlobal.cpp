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
#include <algorithm>  // std::replace

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/sem.h"
#include "common/string.h"

#include "mongoBackend/mongoOntimeintervalOperations.h"
#include "mongoBackend/mongoConnectionPool.h"
#include "mongoBackend/MongoGlobal.h"

#include "ngsi/EntityIdVector.h"
#include "ngsi/AttributeList.h"
#include "ngsi/ContextElementResponseVector.h"
#include "ngsi/Duration.h"
#include "ngsi/Restriction.h"
#include "ngsiNotify/Notifier.h"
#include "parse/CompoundValueNode.h"

using namespace mongo;
using std::auto_ptr;


/* ****************************************************************************
*
* OtisTreatFunction - callback signature for treatOntimeintervalSubscriptions
*/
typedef void (*OtisTreatFunction)(std::string tenant, BSONObj& bobjP);


/* ****************************************************************************
*
* Globals
*/
static std::string          dbPrefix;
static std::string          entitiesCollectionName;
static std::string          registrationsCollectionName;
static std::string          subscribeContextCollectionName;
static std::string          subscribeContextAvailabilityCollectionName;
static std::string          assocationsCollectionName;
static Notifier*            notifier;
static bool                 multitenant;


/* ****************************************************************************
*
* Forward declarations
*/
static void compoundVectorResponse(orion::CompoundValueNode* cvP, const BSONElement& be);
static void compoundObjectResponse(orion::CompoundValueNode* cvP, const BSONElement& be);


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
* mongoConnect -
*
* This method is intended for unit testing, that needs the DBClientConnection
* object to be mocked.
*
*/
bool mongoConnect(DBClientConnection* c)
{
  connection = c;

  return true;
}


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
*/
void releaseMongoConnection(DBClientBase* connection)
{
  mongoPoolConnectionRelease(connection);
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
extern void setDbPrefix(std::string _dbPrefix)
{
  dbPrefix = _dbPrefix;
  LM_T(LmtBug, ("Set dbPrefix to '%s'", dbPrefix.c_str()));
}


/*****************************************************************************
*
* getOrionDatabases -
*/
extern void getOrionDatabases(std::vector<std::string>& dbs)
{
  BSONObj       result;
  DBClientBase* connection = getMongoConnection();

  connection->runCommand("admin", BSON("listDatabases" << 1), result);
  releaseMongoConnection(connection);

  std::vector<BSONElement> databases = result.getField("databases").Array();

  for (std::vector<BSONElement>::iterator i = databases.begin(); i != databases.end(); ++i)
  {
    BSONObj      db      = (*i).Obj();
    std::string  dbName  = STR_FIELD(db, "name");
    std::string  prefix  = dbPrefix + "-";

    if (strncmp(prefix.c_str(), dbName.c_str(), strlen(prefix.c_str())) == 0)
    {
      LM_T(LmtMongo, ("Orion database found: %s", dbName.c_str()));
      dbs.push_back(dbName);
      LM_T(LmtBug, ("Pushed back db name '%s'", dbName.c_str()));
    }
  }
}


/*****************************************************************************
*
* setEntitiesCollectionName -
*/
void setEntitiesCollectionName(std::string name)
{
  entitiesCollectionName = name;
}


/*****************************************************************************
*
* setRegistrationsCollectionName -
*/
void setRegistrationsCollectionName(std::string name)
{
  registrationsCollectionName = name;
}


/*****************************************************************************
*
* setSubscribeContextCollectionName -
*/
void setSubscribeContextCollectionName(std::string name)
{
  subscribeContextCollectionName = name;
}


/*****************************************************************************
*
* setSubscribeContextAvailabilityCollectionName -
*/
void setSubscribeContextAvailabilityCollectionName(std::string name)
{
  subscribeContextAvailabilityCollectionName = name;
}


/*****************************************************************************
*
* setAssociationsCollectionName -
*/
void setAssociationsCollectionName(std::string name)
{
  assocationsCollectionName = name;
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
std::string composeDatabaseName(std::string tenant)
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
std::string getEntitiesCollectionName(std::string tenant)
{
  return composeCollectionName(tenant, entitiesCollectionName);
}


/*****************************************************************************
*
* getRegistrationsCollectionName -
*/
std::string getRegistrationsCollectionName(std::string tenant)
{
  return composeCollectionName(tenant, registrationsCollectionName);
}


/*****************************************************************************
*
* getSubscribeContextCollectionName -
*/
std::string getSubscribeContextCollectionName(std::string tenant)
{
  return composeCollectionName(tenant, subscribeContextCollectionName);
}


/*****************************************************************************
*
* getSubscribeContextAvailabilityCollectionName -
*/
std::string getSubscribeContextAvailabilityCollectionName(std::string tenant)
{
  return composeCollectionName(tenant, subscribeContextAvailabilityCollectionName);
}


/*****************************************************************************
*
* getAssociationsCollectionName -
*/
std::string getAssociationsCollectionName(std::string tenant)
{
  return composeCollectionName(tenant, assocationsCollectionName);
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
void ensureLocationIndex(std::string tenant)
{
  /* Ensure index for entity locations, in the case of using 2.4 */
  if (mongoLocationCapable())
  {
    std::string   index      = ENT_LOCATION "." ENT_LOCATION_COORDS;
    DBClientBase* connection = getMongoConnection();

    connection->createIndex(getEntitiesCollectionName(tenant).c_str(), BSON(index << "2dsphere"));
    releaseMongoConnection(connection);
    LM_T(LmtMongo, ("ensuring 2dsphere index on %s (tenant %s)", index.c_str(), tenant.c_str()));
  }
}


/* ****************************************************************************
*
* treatOnTimeIntervalSubscriptions -
*
* Look for ONTIMEINTERVAL subscriptions in the database
*/
static void treatOnTimeIntervalSubscriptions(std::string tenant, OtisTreatFunction treatFunction)
{
  std::string               condType   = CSUB_CONDITIONS "." CSUB_CONDITIONS_TYPE;
  BSONObj                   query      = BSON(condType << ON_TIMEINTERVAL_CONDITION);
  DBClientBase*             connection = getMongoConnection();
  auto_ptr<DBClientCursor>  cursor;

  LM_T(LmtMongo, ("query() in '%s' collection: '%s'",
                  getSubscribeContextCollectionName(tenant).c_str(),
                  query.toString().c_str()));

  try
  {
    cursor = connection->query(getSubscribeContextCollectionName(tenant).c_str(), query);

    /*
     * We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
     * raising an exception, the query() method sets the cursor to NULL. In this case, we raise the
     * exception ourselves
     */
    if (cursor.get() == NULL)
    {
      throw DBException("Null cursor from mongo (details on this is found in the source code)", 0);
    }
    releaseMongoConnection(connection);  // KZ: OK to release the connection up here?

    LM_I(("Database Operation Successful (%s)", query.toString().c_str()));
  }
  catch (const DBException &e)
  {
    releaseMongoConnection(connection);
    LM_E(("Database Error (DBException: %s)", e.what()));
    return;
  }
  catch (...)
  {
    releaseMongoConnection(connection);
    LM_E(("Database Error (generic exception)"));
    return;
  }

  // Call the treat function for each subscription
  while (cursor->more())
  {
    BSONObj sub = cursor->next();

    treatFunction(tenant, sub);
  }
}


/* ****************************************************************************
*
* recoverOnTimeIntervalThread -
*/
static void recoverOnTimeIntervalThread(std::string tenant, BSONObj& sub)
{
  BSONElement  idField = sub.getField("_id");

  // Paranoia check:  _id exists?
  if (idField.eoo() == true)
  {
    LM_E(("Database Error (error retrieving _id field in doc: '%s')", sub.toString().c_str()));
    return;
  }

  std::string  subId   = idField.OID().toString();

  // Paranoia check II:  'conditions' exists?
  BSONElement conditionsField = sub.getField(CSUB_CONDITIONS);
  if (conditionsField.eoo() == true)
  {
    LM_E(("Database Error (error retrieving 'conditions' field) for subscription '%s'", subId.c_str()));
    return;
  }

  std::vector<BSONElement> condV = sub.getField(CSUB_CONDITIONS).Array();
  for (unsigned int ix = 0; ix < condV.size(); ++ix)
  {
    BSONObj condition = condV[ix].embeddedObject();

    if (strcmp(STR_FIELD(condition, CSUB_CONDITIONS_TYPE).c_str(), ON_TIMEINTERVAL_CONDITION) == 0)
    {
      int interval = condition.getIntField(CSUB_CONDITIONS_VALUE);

      LM_T(LmtNotifier, ("creating ONTIMEINTERVAL thread for subscription '%s' with interval %d (tenant '%s')",
                         subId.c_str(),
                         interval,
                         tenant.c_str()));
      processOntimeIntervalCondition(subId, interval, tenant);
    }
  }
}


/* ****************************************************************************
*
* recoverOntimeIntervalThreads -
*/
void recoverOntimeIntervalThreads(std::string tenant)
{
  treatOnTimeIntervalSubscriptions(tenant, recoverOnTimeIntervalThread);
}


/* ****************************************************************************
*
* destroyOnTimeIntervalThread -
*/
static void destroyOnTimeIntervalThread(std::string tenant, BSONObj& sub)
{
  BSONElement  idField = sub.getField("_id");

  if (idField.eoo() == true)
  {
    LM_E(("Database Error (error retrieving _id field in doc: '%s')", sub.toString().c_str()));
    return;
  }

  std::string  subId  = idField.OID().toString();

  notifier->destroyOntimeIntervalThreads(subId);
}


/* ****************************************************************************
*
 destroyAllOntimeIntervalThreads -
*
* This function is only to be used under harakiri mode, not for real use
*/
void destroyAllOntimeIntervalThreads(std::string tenant)
{
  treatOnTimeIntervalSubscriptions(tenant, destroyOnTimeIntervalThread);
}


/* ****************************************************************************
*
* matchEntity -
*
* Taking into account null types and id patterns. Note this function is not
* commutative: en1 is interpreted as the entity to match *in* en2 (i.e.
* it is assumed that the pattern is in en2)
*/
bool matchEntity(EntityId* en1, EntityId* en2)
{
  bool idMatch;

  if (isTrue(en2->isPattern))
  {
    regex_t regex;

    idMatch = false;
    if (regcomp(&regex, en2->id.c_str(), 0) != 0)
    {
      LM_W(("Bad Input (error compiling regex: '%s')", en2->id.c_str()));
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
bool includedEntity(EntityId en, EntityIdVector& entityIdV)
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
bool includedAttribute(ContextRegistrationAttribute attr, AttributeList* attrsV)
{
  // This i's the case in which the discoverAvailabilityRequest doesn't include attributes,
  // so all the attributes are included in the response
  //
  if (attrsV->size() == 0)
  {
    return true;
  }

  for (unsigned int ix = 0; ix < attrsV->size(); ++ix)
  {
    if (attrsV->get(ix) == attr.name)
    {
      return true;
    }
  }

  return false;
}


/* ****************************************************************************
*
* includedAttribute -
*
* FIXME: note that in the current implementation, in which we only use 'name' to
* compare, this function is equal to the one for ContextRegistrationAttrribute.
* However, we keep them separated, as isDomain (present in ContextRegistrationAttribute
* but not in ContextRegistration could mean a difference). To review once domain attributes
* get implemented.
*
*/
bool includedAttribute(ContextAttribute attr, AttributeList* attrsV)
{
  //
  // This is the case in which the queryContextRequest doesn't include attributes,
  // so all the attributes are included in the response
  //
  if (attrsV->size() == 0)
  {
    return true;
  }

  for (unsigned int ix = 0; ix < attrsV->size(); ++ix)
  {
    if (attrsV->get(ix) == attr.name)
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
static void fillQueryEntity(BSONArrayBuilder& ba, EntityId* enP)
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

  std::string servicePathValue = "";

  if (servicePath.size() > 0)
  {
    bool nullAdded = false;

    servicePathValue += "{ $in: [ ";

    for (unsigned int ix = 0 ; ix < servicePath.size(); ++ix)
    {
      LM_T(LmtServicePath, ("Service Path: '%s'", servicePath[ix].c_str()));

      //
      // Add "null" in the following service path cases: / or /#. In order to avoid adding null
      // several times, the nullAdded flag is used
      //
      if (!nullAdded && ((servicePath[ix] == "/") || (servicePath[ix] == "/#")))
      {
        servicePathValue += "null, ";
        nullAdded = true;
      }

      char path[MAX_SERVICE_NAME_LEN];
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
  }
  else
  {
    /* In this case, servicePath match any path, including the case of "null" */
    servicePathValue = "{$in: [ /^\\/.*/, null] }";
  }

  return fromjson(servicePathValue);
}


/* ****************************************************************************
*
* addCompoundNode -
*
*/
static void addCompoundNode(orion::CompoundValueNode* cvP, const BSONElement& e)
{
  if ((e.type() != String) && (e.type() != Object) && (e.type() != Array))
  {
    LM_T(LmtSoftError, ("unknown BSON type"));
    return;
  }

  orion::CompoundValueNode* child = new orion::CompoundValueNode(orion::CompoundValueNode::Object);
  child->name = e.fieldName();

  switch (e.type())
  {
  case String:
    child->type  = orion::CompoundValueNode::String;
    child->value = e.String();
    break;

  case Object:
    compoundObjectResponse(child, e);
    break;

  case Array:
    compoundVectorResponse(child, e);
    break;

  default:
    //
    // We need the default clause to avoid 'enumeration value X not handled in switch' errors
    // due to -Werror=switch at compilation time
    //
    break;
  }

  cvP->add(child);
}


/* ****************************************************************************
*
* compoundObjectResponse -
*/
static void compoundObjectResponse(orion::CompoundValueNode* cvP, const BSONElement& be)
{
  BSONObj obj = be.embeddedObject();

  cvP->type = orion::CompoundValueNode::Object;
  for (BSONObj::iterator i = obj.begin(); i.more();)
  {
    BSONElement e = i.next();
    addCompoundNode(cvP, e);
  }
}


/* ****************************************************************************
*
* compoundVectorResponse -
*/
static void compoundVectorResponse(orion::CompoundValueNode* cvP, const BSONElement& be)
{
  std::vector<BSONElement> vec = be.Array();

  cvP->type = orion::CompoundValueNode::Vector;
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    BSONElement e = vec[ix];
    addCompoundNode(cvP, e);
  }
}


/* *****************************************************************************
*
* processAreaScope -
*
* Returns true if areaQuery was filled, false otherwise
*
*/
static bool processAreaScope(Scope* scoP, BSONObj &areaQuery)
{
  if (!mongoLocationCapable())
  {
    LM_W(("Bad Input (location scope was found but your MongoDB version doesn't support it."
          " Please upgrade MongoDB server to 2.4 or newer)"));

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
  else if (scoP->areaType== orion::PolygonType)
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
    LM_W(("Bad Input (unknown area type)"));
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
* addFilterScopes -
*
*/
static void addFilterScope(Scope* scoP, std::vector<BSONObj> &filters)
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
      LM_W(("Bad Input (unknown value for %s filter: '%s'", SCOPE_FILTER_EXISTENCE, scoP->value.c_str()));
    }
  }
  else
  {
    LM_W(("Bad Input (unknown filter type '%s'", scoP->type.c_str()));
  }
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
  EntityIdVector                   enV,
  AttributeList                    attrL,
  Restriction                      res,
  ContextElementResponseVector*    cerV,
  std::string*                     err,
  bool                             includeEmpty,
  std::string                      tenant,
  const std::vector<std::string>&  servicePath,
  int                              offset,
  int                              limit,
  long long*                       countP
)
{
  DBClientBase* connection = NULL;

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
    fillQueryEntity(orEnt, enV.get(ix));
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
    std::string attrName = attrL.get(ix);

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
    Scope* sco = res.scopeVector.get(ix);

    if (sco->type.find(SCOPE_FILTER) == 0)
    {
      addFilterScope(sco, filters);
    }
    else if (sco->type == FIWARE_LOCATION || sco->type == FIWARE_LOCATION_DEPRECATED)
    {
      geoScopes++;
      if (geoScopes > 1)
      {
        LM_W(("Bad Input (current version supports only one area scope, extra geoScope is ignored"));
      }
      else
      {
        BSONObj areaQuery;

        if (processAreaScope(sco, areaQuery))
        {
          std::string locCoords = ENT_LOCATION "." ENT_LOCATION_COORDS;

          finalQuery.append(locCoords, areaQuery);
        }
      }
    }
    else
    {
      LM_W(("Bad Input (unknown scope type '%s', ignoring)", sco->type.c_str()));
    }
  }

  for (unsigned int ix = 0; ix < filters.size(); ++ix)
  {
    finalQuery.appendElements(filters[ix]);
  }

  LM_T(LmtPagination, ("Offset: %d, Limit: %d, countP: %p", offset, limit, countP));

  /* Do the query on MongoDB */
  auto_ptr<DBClientCursor>  cursor;
  BSONObj                   bquery = finalQuery.obj();
  Query                     query(bquery);
  Query                     sortCriteria  = query.sort(BSON(ENT_CREATION_DATE << 1));

  LM_T(LmtMongo, ("query() in '%s' collection: '%s'",
                  getEntitiesCollectionName(tenant).c_str(),
                  query.toString().c_str()));

  connection = getMongoConnection();
  try
  {
    if (countP != NULL)
    {
      *countP = connection->count(getEntitiesCollectionName(tenant).c_str(), bquery);
    }

    cursor = connection->query(getEntitiesCollectionName(tenant).c_str(), query, limit, offset);

    //
    // We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
    // raising an exception, the query() method sets the cursor to NULL. In this case, we raise the
    // exception ourselves
    //
    if (cursor.get() == NULL)
    {
      throw DBException("Null cursor from mongo (details on this is found in the source code)", 0);
    }

    releaseMongoConnection(connection);
    LM_I(("Database Operation Successful (%s)", query.toString().c_str()));
  }
  catch (const DBException& e)
  {
    releaseMongoConnection(connection);
    *err = std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
      " - query(): " + query.toString() +
      " - exception: " + e.what();

    LM_E(("Database Error (%s)", err->c_str()));
    return false;
  }
  catch (...)
  {
    releaseMongoConnection(connection);
    *err = std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
      " - query(): " + query.toString() +
      " - exception: " + "generic";

    LM_E(("Database Error (%s)", err->c_str()));
    return false;
  }

  /* Process query result */
  while (cursor->more())
  {
    BSONObj                 r    = cursor->next();
    std::string             err  = r.getStringField("$err");
    ContextElementResponse* cer  = new ContextElementResponse();

    if (err != "")
    {
      //
      // We can't return the error 'as is', as it may contain forbidden characters.
      // So, we can just match the error and send a less descriptive text.
      //
      const char* invalidPolygon      = "Exterior shell of polygon is invalid";
      const char* defaultErrorString  = "Error at querying MongoDB";

      LM_W(("Database Error (%s)", err.c_str()));

      if (strncmp(err.c_str(), invalidPolygon, strlen(invalidPolygon)) == 0)
      {
        err = invalidPolygon;
      }
      else
      {
        err = defaultErrorString;
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

      cer->statusCode.fill(SccReceiverInternalError, err);
      cerV->push_back(cer);
      return true;
    }

    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));
    cer->statusCode.fill(SccOk);

    /* Entity part */

    BSONObj queryEntity = r.getObjectField("_id");

    cer->contextElement.entityId.id          = STR_FIELD(queryEntity, ENT_ENTITY_ID);
    cer->contextElement.entityId.type        = STR_FIELD(queryEntity, ENT_ENTITY_TYPE);
    cer->contextElement.entityId.servicePath = STR_FIELD(queryEntity, ENT_SERVICE_PATH);
    cer->contextElement.entityId.isPattern   = "false";

    /* Get the location attribute (if it exists) */
    std::string locAttr;
    if (r.hasElement(ENT_LOCATION))
    {
      locAttr = r.getObjectField(ENT_LOCATION).getStringField(ENT_LOCATION_ATTRNAME);
    }

    /* Attributes part */
    BSONObj queryAttrs;

    //
    // This try/catch should not be necessary as all document in the entities collection have an attrs embedded document
    // from creation time. However, it adds an extra protection, just in case.
    // Somebody *could* manipulate the mongo database and if so, the broker would crash here.
    // Better to be on the safe side ...
    //
    try
    {
      queryAttrs = r.getField(ENT_ATTRS).embeddedObject();
    }
    catch (...)
    {
      LM_E(("Database Error (no attrs array in document of entities collection)"));
      cer->statusCode.fill(SccReceiverInternalError, "attrs field missing in entity document");
      cerV->push_back(cer);

      return true;
    }

    std::set<std::string> attrNames;

    queryAttrs.getFieldNames(attrNames);
    for (std::set<std::string>::iterator i = attrNames.begin(); i != attrNames.end(); ++i)
    {
      std::string       attrName   = *i;
      BSONObj           queryAttr  = queryAttrs.getField(attrName).embeddedObject();
      ContextAttribute  ca;

      ca.name = dbDotDecode(basePart(attrName));
      std::string mdId = idPart(attrName);
      ca.type = STR_FIELD(queryAttr, ENT_ATTRS_TYPE);

      /* Note that includedAttribute decision is based on name and type. Value is set only if
       * decision is positive
       */
      if (includedAttribute(ca, &attrL))
      {
        ContextAttribute* caP;

        if (queryAttr.getField(ENT_ATTRS_VALUE).type() == String)
        {
          ca.value = STR_FIELD(queryAttr, ENT_ATTRS_VALUE);
          if (!includeEmpty && ca.value.length() == 0)
          {
            continue;
          }
          caP = new ContextAttribute(ca.name, ca.type, ca.value);
        }
        else if (queryAttr.getField(ENT_ATTRS_VALUE).type() == Object)
        {
          caP = new ContextAttribute(ca.name, ca.type);
          caP->compoundValueP = new orion::CompoundValueNode(orion::CompoundValueNode::Object);
          compoundObjectResponse(caP->compoundValueP, queryAttr.getField(ENT_ATTRS_VALUE));
        }
        else if (queryAttr.getField(ENT_ATTRS_VALUE).type() == Array)
        {
          caP = new ContextAttribute(ca.name, ca.type);
          caP->compoundValueP = new orion::CompoundValueNode(orion::CompoundValueNode::Vector);
          compoundVectorResponse(caP->compoundValueP, queryAttr.getField(ENT_ATTRS_VALUE));
        }
        else
        {
          LM_T(LmtSoftError, ("unknown BSON type"));
          continue;
        }

        /* Setting ID (if found) */
        if (mdId != "")
        {
          Metadata* md = new Metadata(NGSI_MD_ID, "string", mdId);

          caP->metadataVector.push_back(md);
        }

        if (locAttr == ca.name)
        {
          Metadata* md = new Metadata(NGSI_MD_LOCATION, "string", LOCATION_WGS84);

          caP->metadataVector.push_back(md);
        }

        /* Setting custom metadata (if any) */
        if (queryAttr.hasField(ENT_ATTRS_MD))
        {
          std::vector<BSONElement> metadataV = queryAttr.getField(ENT_ATTRS_MD).Array();

          for (unsigned int ix = 0; ix < metadataV.size(); ++ix)
          {
            BSONObj    metadata = metadataV[ix].embeddedObject();
            Metadata*  md;

            if (metadata.hasField(ENT_ATTRS_MD_TYPE))
            {
              md = new Metadata(STR_FIELD(metadata, ENT_ATTRS_MD_NAME),
                                STR_FIELD(metadata, ENT_ATTRS_MD_TYPE),
                                STR_FIELD(metadata, ENT_ATTRS_MD_VALUE));
            }
            else
            {
              md = new Metadata(STR_FIELD(metadata, ENT_ATTRS_MD_NAME),
                                "",
                                STR_FIELD(metadata, ENT_ATTRS_MD_VALUE));
            }

            caP->metadataVector.push_back(md);
          }
        }

        cer->contextElement.contextAttributeVector.push_back(caP);
      }
    }

    /* All the attributes existing in the request but not found in the response are added with 'found' set to false */
    for (unsigned int ix = 0; ix < attrL.size(); ++ix)
    {
      bool         found     = false;
      std::string  attrName  = attrL.get(ix);

      for (unsigned int jx = 0; jx < cer->contextElement.contextAttributeVector.size(); ++jx)
      {
        if (attrName == cer->contextElement.contextAttributeVector.get(jx)->name)
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

  /* All the not-patterned entities in the request not in the response are added (without attributes), as they are
   * used before pruning in the CPr calculation logic */
  for (unsigned int ix = 0; ix < enV.size(); ++ix)
  {
    if (enV.get(ix)->isPattern != "true")
    {
      bool needToAdd = true;

      for (unsigned int jx = 0; jx < cerV->size(); ++jx)
      {
        EntityId* eP = &cerV->get(jx)->contextElement.entityId;

        if ((eP->id == enV.get(ix)->id) && (eP->type == enV.get(ix)->type))
        {
          needToAdd = false;
          break;  /* jx */
        }
      }

      if (needToAdd)
      {
        ContextElementResponse* cerP = new ContextElementResponse();

        cerP->contextElement.entityId.id = enV.get(ix)->id;
        cerP->contextElement.entityId.type = enV.get(ix)->type;
        cerP->contextElement.entityId.isPattern = "false";

        //
        // This entity has to be pruned if after CPr searching no attribute is "added" to it.
        // The prune attribute distinguish this kind of entities from the "real ones" that are
        // without attributes in the Orion local database (a rare case, but may happen)
        //
        cerP->prune = true;

        for (unsigned int jx = 0; jx < attrL.size(); ++jx)
        {
          ContextAttribute* caP = new ContextAttribute(attrL.get(jx), "", "", false);

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
    ContextElementResponse* cerP = oldCerV.get(ix);
    ContextElementResponse* newCerP = new ContextElementResponse();

    /* Note we cannot use the ContextElement::fill() method, given that it also copies the ContextAttributeVector. The side-effect
     * of this is that attributeDomainName and domainMetadataVector are not being copied, but it should not be a problem, given that
     * domain attributes are not implemented */
    newCerP->contextElement.entityId.fill(&cerP->contextElement.entityId);

    // FIXME P10: not sure if this is the right way of doing, maybe we need a fill() method for this
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
static void processEntity(ContextRegistrationResponse* crr, EntityIdVector enV, BSONObj entity)
{
  EntityId en;

  en.id = STR_FIELD(entity, REG_ENTITY_ID);
  en.type = STR_FIELD(entity, REG_ENTITY_TYPE);

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
static void processAttribute(ContextRegistrationResponse* crr, AttributeList attrL, BSONObj attribute)
{
  ContextRegistrationAttribute attr(
    STR_FIELD(attribute, REG_ATTRS_NAME),
    STR_FIELD(attribute, REG_ATTRS_TYPE),
    STR_FIELD(attribute, REG_ATTRS_ISDOMAIN));

  // FIXME: we don't take metadata into account at the moment
  // attr.metadataV = ..

  if (includedAttribute(attr, &attrL))
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
  EntityIdVector                      enV,
  AttributeList                       attrL,
  ContextRegistrationResponseVector*  crrV,
  Format                              format
)
{
  ContextRegistrationResponse crr;

  crr.contextRegistration.providingApplication.set(STR_FIELD(cr, REG_PROVIDING_APPLICATION));
  crr.contextRegistration.providingApplication.setFormat(format);

  std::vector<BSONElement> queryEntityV = cr.getField(REG_ENTITIES).Array();

  for (unsigned int ix = 0; ix < queryEntityV.size(); ++ix)
  {
    processEntity(&crr, enV, queryEntityV[ix].embeddedObject());
  }

  /* Note that attributes can be included only if at least one entity has been found */
  if (crr.contextRegistration.entityIdVector.size() > 0)
  {
    if (cr.hasField(REG_ATTRS)) /* To prevent registration in the E-<null> style */
    {
      std::vector<BSONElement> queryAttrV = cr.getField(REG_ATTRS).Array();

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
  EntityIdVector                      enV,
  AttributeList                       attrL,
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
  DBClientBase* connection = NULL;

  /* Build query based on arguments */
  // FIXME P2: this implementation need to be refactored for cleanup
  std::string       contextRegistrationEntities     = REG_CONTEXT_REGISTRATION "." REG_ENTITIES;
  std::string       contextRegistrationEntitiesId   = REG_CONTEXT_REGISTRATION "." REG_ENTITIES "." REG_ENTITY_ID;
  std::string       contextRegistrationEntitiesType = REG_CONTEXT_REGISTRATION "." REG_ENTITIES "." REG_ENTITY_TYPE;
  std::string       contextRegistrationAttrsNames   = REG_CONTEXT_REGISTRATION "." REG_ATTRS    "." REG_ATTRS_NAME;
  BSONArrayBuilder  entityOr;
  BSONArrayBuilder  entitiesWithType;
  BSONArrayBuilder  entitiesWithoutType;

  for (unsigned int ix = 0; ix < enV.size(); ++ix)
  {
    EntityId* en = enV.get(ix);

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
    std::string attrName = attrL.get(ix);

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
  BSONObj                   bquery = queryBuilder.obj();
  Query                     query(bquery);
  Query                     sortCriteria  = query.sort(BSON("_id" << 1));

  LM_T(LmtMongo, ("query() in '%s' collection: '%s'",
                  getRegistrationsCollectionName(tenant).c_str(),
                  query.toString().c_str()));

  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));

  connection = getMongoConnection();
  try
  {
    if ((details == true) && (countP != NULL))
    {
      *countP = connection->count(getRegistrationsCollectionName(tenant).c_str(), bquery);
    }

    cursor = connection->query(getRegistrationsCollectionName(tenant).c_str(), query, limit, offset);
    releaseMongoConnection(connection);
    LM_I(("Database Operation Successful (%s)", query.toString().c_str()));
  }
  catch (const DBException& e)
  {
    releaseMongoConnection(connection);
    *err = std::string("collection: ") + getRegistrationsCollectionName(tenant).c_str() +
      " - query(): " + query.toString() +
      " - exception: " + e.what();

    LM_E(("Database Error (%s)", err->c_str()));
    return false;
  }
  catch (...)
  {
    releaseMongoConnection(connection);
    *err = std::string("collection: ") + getRegistrationsCollectionName(tenant).c_str() +
      " - query(): " + query.toString() +
      " - exception: " + "generic";

    LM_E(("Database Error (%s)", err->c_str()));
    return false;
  }

  /* Process query result */
  while (cursor->more())
  {
    BSONObj r = cursor->next();

    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

    //
    // Default format is XML, in the case the field is not found in the
    // registrations document (for pre-0.21.0 versions)
    //
    Format                    format = r.hasField(REG_FORMAT)? stringToFormat(STR_FIELD(r, REG_FORMAT)) : XML;
    std::vector<BSONElement>  queryContextRegistrationV = r.getField(REG_CONTEXT_REGISTRATION).Array();

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

  return true;
}


/* ****************************************************************************
*
* isCondValueInContextElementResponse -
*/
bool isCondValueInContextElementResponse(ConditionValueList* condValues, ContextElementResponseVector* cerV)
{
  for (unsigned int cvlx = 0; cvlx < condValues->size(); ++cvlx)
  {
    for (unsigned int aclx = 0; aclx < cerV->size(); ++aclx)
    {
      ContextAttributeVector caV = cerV->get(aclx)->contextElement.contextAttributeVector;

      for (unsigned int kx = 0; kx < caV.size(); ++kx)
      {
        if (caV.get(kx)->name == condValues->get(cvlx))
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
EntityIdVector subToEntityIdVector(BSONObj sub)
{
  EntityIdVector            enV;
  std::vector<BSONElement>  subEnts = sub.getField(CSUB_ENTITIES).Array();

  for (unsigned int ix = 0; ix < subEnts.size() ; ++ix)
  {
    BSONObj    subEnt = subEnts[ix].embeddedObject();
    EntityId*  en     = new EntityId(STR_FIELD(subEnt, CSUB_ENTITY_ID),
                                     STR_FIELD(subEnt, CSUB_ENTITY_TYPE),
                                     STR_FIELD(subEnt, CSUB_ENTITY_ISPATTERN));
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
AttributeList subToAttributeList(BSONObj sub)
{
  AttributeList             attrL;
  std::vector<BSONElement>  subAttrs = sub.getField(CSUB_ATTRS).Array();

  for (unsigned int ix = 0; ix < subAttrs.size() ; ++ix)
  {
    std::string subAttr = subAttrs[ix].String();

    attrL.push_back(subAttr);
  }

  return attrL;
}


/* ****************************************************************************
*
* processOnChangeCondition -
*
* This function is called from two places:
*
* 1) initial processing of an ONCHANGE condition in processConditionVector (used from
*   subscribeContext and updateContextSubscription), so an "initial"
*   notification for all the entites/attributes included in the entity in the case
*   that some of them are within the ones in the condValues.
* 2) updateContext processing logic when attributes under an ONCHANGE condition are
*   updated
*
* The argument enV is the entities and attributes in the subscribeContext
* request (case 1) or the "triggering" entity (case 2). The argument attrL is the
* attributes in the subscribeContext request. Note that is condValues is NULL, the checking
* on condValues is omitted (this is the case when this function is called from updateContext,
* where the previous query on csubs ensures that that condition is true)
*
* This method returns true if the notification was actually send. Otherwise, false
* is returned. This is used in the caller to know if lastNotification field in the
* subscription document in csubs collection has to be modified or not.
*/
bool processOnChangeCondition
(
  EntityIdVector                   enV,
  AttributeList                    attrL,
  ConditionValueList*              condValues,
  std::string                      subId,
  std::string                      notifyUrl,
  Format                           format,
  std::string                      tenant,
  const std::string&               xauthToken,
  const std::vector<std::string>&  servicePathV
)
{
  // FIXME P10: we are using dummy scope at the moment, until subscription scopes get implemented
  std::string                   err;
  NotifyContextRequest          ncr;
  Restriction                   res;
  ContextElementResponseVector  rawCerV;

  if (!entitiesQuery(enV, attrL, res, &rawCerV, &err, false, tenant, servicePathV))
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

      // FIXME P10: we are using dummy scope by the moment, until subscription scopes get implemented
      // FIXME P10: we are using an empty service path vector until serive paths get implemented for subscriptions
      if (!entitiesQuery(enV, emptyList, res, &rawCerV, &err, false, tenant, servicePathV))
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
        getNotifier()->sendNotifyContextRequest(&ncr, notifyUrl, tenant, xauthToken, format);
        allCerV.release();
        ncr.contextElementResponseVector.release();

        return true;
      }

      allCerV.release();
    }
    else
    {
      getNotifier()->sendNotifyContextRequest(&ncr, notifyUrl, tenant, xauthToken, format);
      ncr.contextElementResponseVector.release();
      return true;
    }
  }

  ncr.contextElementResponseVector.release();
  return false;
}


/* ****************************************************************************
*
* processOntimeIntervalCondition -
*/
void processOntimeIntervalCondition(std::string subId, int interval, std::string tenant)
{
  getNotifier()->createIntervalThread(subId, interval, tenant);
}


/* ****************************************************************************
*
* processConditionVector -
*
*/
BSONArray processConditionVector
(
  NotifyConditionVector*           ncvP,
  EntityIdVector                   enV,
  AttributeList                    attrL,
  std::string                      subId,
  std::string                      url,
  bool*                            notificationDone,
  Format                           format,
  std::string                      tenant,
  const std::string&               xauthToken,
  const std::vector<std::string>&  servicePathV
)
{
  BSONArrayBuilder conds;

  *notificationDone = false;

  for (unsigned int ix = 0; ix < ncvP->size(); ++ix)
  {
    NotifyCondition* nc = ncvP->get(ix);

    if (nc->type == ON_TIMEINTERVAL_CONDITION)
    {
      Duration interval;

      interval.set(nc->condValueList.get(0));
      interval.parse();

      conds.append(BSON(CSUB_CONDITIONS_TYPE << ON_TIMEINTERVAL_CONDITION <<
                        CSUB_CONDITIONS_VALUE << (long long) interval.seconds));

      processOntimeIntervalCondition(subId, interval.seconds, tenant);
    }
    else if (nc->type == ON_CHANGE_CONDITION)
    {
      /* Create an array holding the list of condValues */
      BSONArrayBuilder condValues;

      for (unsigned int jx = 0; jx < nc->condValueList.size(); ++jx)
      {
        condValues.append(nc->condValueList.get(jx));
      }

      conds.append(BSON(CSUB_CONDITIONS_TYPE << ON_CHANGE_CONDITION <<
                        CSUB_CONDITIONS_VALUE << condValues.arr()));

      if (processOnChangeCondition(enV,
                                   attrL,
                                   &(nc->condValueList),
                                   subId,
                                   url,
                                   format,
                                   tenant,
                                   xauthToken,
                                   servicePathV))
      {
        *notificationDone = true;
      }
    }
    else  // ON_VALUE_CONDITION
    {
      // FIXME: not implemented
    }
  }

  return conds.arr();
}


/* ****************************************************************************
*
* mongoUpdateCasubNewNotification -
*
* This method is pretty similar to the mongoUpdateCsubNewNotification in mongoOntimeintervalOperations module.
* However, it doesn't take semaphore
*
*/
static HttpStatusCode mongoUpdateCasubNewNotification(std::string subId, std::string* err, std::string tenant)
{
  LM_T(LmtMongo, ("Update NGSI9 Subscription New Notification"));

  DBClientBase* connection = NULL;

  /* Update the document */
  BSONObj query  = BSON("_id" << OID(subId));
  BSONObj update = BSON("$set" << BSON(CASUB_LASTNOTIFICATION << getCurrentTime()) << "$inc" << BSON(CASUB_COUNT << 1));

  LM_T(LmtMongo, ("update() in '%s' collection: (%s,%s)", getSubscribeContextAvailabilityCollectionName(tenant).c_str(),
                  query.toString().c_str(),
                  update.toString().c_str()));

  connection = getMongoConnection();
  try
  {
    connection->update(getSubscribeContextAvailabilityCollectionName(tenant).c_str(), query, update);
    releaseMongoConnection(connection);
    LM_I(("Database Operation Successful (%s)", query.toString().c_str()));
  }
  catch (const DBException &e)
  {
    releaseMongoConnection(connection);
    *err = e.what();

    LM_E(("Database Error ('update[%s:%s] in %s', '%s')",
          query.toString().c_str(),
          update.toString().c_str(),
          getSubscribeContextAvailabilityCollectionName(tenant).c_str(),
          e.what()));

    return SccOk;
  }
  catch (...)
  {
    releaseMongoConnection(connection);
    *err = "Database error - exception thrown";

    LM_E(("Database Error ('update[%s:%s] in %s', '%s')",
          query.toString().c_str(),
          update.toString().c_str(),
          getSubscribeContextAvailabilityCollectionName(tenant).c_str(),
          "generic exception"));

    return SccOk;
  }

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
  EntityIdVector  enV,
  AttributeList   attrL,
  std::string     subId,
  std::string     notifyUrl,
  Format          format,
  std::string     tenant
)
{
  std::string                       err;
  NotifyContextAvailabilityRequest  ncar;
  const std::vector<std::string>    servicePathV;  // FIXME P5: servicePath for NGSI9 Subscriptions

  if (!registrationsQuery(enV, attrL, &ncar.contextRegistrationResponseVector, &err, tenant, servicePathV))
  {
    ncar.contextRegistrationResponseVector.release();
    return false;
  }

  if (ncar.contextRegistrationResponseVector.size() > 0)
  {
    /* Complete the fields in NotifyContextRequest */
    ncar.subscriptionId.set(subId);

    getNotifier()->sendNotifyContextAvailabilityRequest(&ncar, notifyUrl, tenant, format);
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
    ContextAttribute* ca = cer->contextElement.contextAttributeVector.get(ix);

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
    ContextRegistrationResponse* crr = crrV.get(crrIx);

    /* Is there a matching entity in the CRR? */
    for (unsigned enIx = 0; enIx < crr->contextRegistration.entityIdVector.size(); ++enIx)
    {
      EntityId* regEn = crr->contextRegistration.entityIdVector.get(enIx);

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
        std::string regAttrName = crr->contextRegistration.contextRegistrationAttributeVector.get(attrIx)->name;
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


/* ****************************************************************************
*
* Characters for attribute value encoding
*/
#define ESCAPE_1_DECODED  '.'
#define ESCAPE_1_ENCODED  '='


/* ****************************************************************************
*
* dbDotEncode -
*
* Replace:
*   . => =
*/
std::string dbDotEncode(std::string s)
{
  replace(s.begin(), s.end(), ESCAPE_1_DECODED, ESCAPE_1_ENCODED);
  return s;
}


/* ****************************************************************************
*
* dbDotDecode -
*
* Replace:
*   = => .
*/
std::string dbDotDecode(std::string s)
{
  std::replace(s.begin(), s.end(), ESCAPE_1_ENCODED, ESCAPE_1_DECODED);
  return s;
}
