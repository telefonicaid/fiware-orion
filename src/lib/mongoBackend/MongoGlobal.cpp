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

#include <regex.h>
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/sem.h"
#include "common/string.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoOntimeintervalOperations.h"

#include "ngsi/EntityIdVector.h"
#include "ngsi/AttributeList.h"
#include "ngsi/ContextElementResponseVector.h"
#include "ngsi/Duration.h"
#include "parse/CompoundValueNode.h"
#include "ngsi/Restriction.h"
#include "ngsiNotify/Notifier.h"

using namespace mongo;
using std::auto_ptr;


/* ****************************************************************************
*
* RECONNECT_RETRIES - number of retries after connect
* RECONNECT_DELAY   - number of millisecs to sleep between retries
*/
#define RECONNECT_RETRIES 100
#define RECONNECT_DELAY   1000  // One second



/* ****************************************************************************
*
* Globals
*/
static DBClientBase*        connection;
static int                  mongoVersionMayor = -1;
static int                  mongoVersionMinor = -1;
static std::string          dbPrefix;
static std::string          entitiesCollectionName;
static std::string          registrationsCollectionName;
static std::string          subscribeContextCollectionName;
static std::string          subscribeContextAvailabilityCollectionName;
static std::string          assocationsCollectionName;
static Notifier*            notifier;

/* ****************************************************************************
*
* Forward declarations
*/
static void compoundVectorResponse(orion::CompoundValueNode* cvP, const BSONElement& be);
static void compoundObjectResponse(orion::CompoundValueNode* cvP, const BSONElement& be);

/* ****************************************************************************
*
* mongoConnect -
*/
bool mongoConnect(const char* host, const char* db, const char* rplSet, const char* username, const char* passwd, bool multitenant) {

    std::string err;

    LM_T(LmtBug, ("dbName: '%s'", db));

    mongoSemTake(__FUNCTION__, "connecting to mongo");

    bool connected     = false;
    int  retries       = RECONNECT_RETRIES;

    if (strlen(rplSet) == 0)
    {
      /* The first argument to true is to use autoreconnect */
      connection = new DBClientConnection(true);

      /* Not sure of to generalize the following code, given that DBClientBase class hasn't a common connect() method (surprisingly) */
      for (int tryNo = 0; tryNo < retries; ++tryNo)
      {
          if ( ((DBClientConnection*)connection)->connect(host, err))
        {
          connected = true;
          break;
        }

        if (tryNo == 0)
          LM_E(("Database Startup Error (cannot connect to mongo - doing %d retries with a %d microsecond interval)", retries, RECONNECT_DELAY));
        else
          LM_T(LmtMongo, ("Try %d connecting to mongo failed", tryNo));

        usleep(RECONNECT_DELAY * 1000); // usleep accepts microseconds
      }

    }
    else
    {
      LM_T(LmtMongo, ("Using replica set %s", rplSet));
      // autoReconnect is always on for DBClientReplicaSet connections.
      std::vector<std::string>  hostTokens;
      int components = stringSplit(host, ',', hostTokens);

      std::vector<HostAndPort> rplSetHosts;
      for (int ix = 0; ix < components; ix++)
      {
          LM_T(LmtMongo, ("rplSet host <%s>", hostTokens[ix].c_str()));
          rplSetHosts.push_back(HostAndPort(hostTokens[ix]));
      }

      connection = new DBClientReplicaSet(rplSet, rplSetHosts, 0);

      /* Not sure of to generalize the following code, given that DBClientBase class hasn't a common connect() method (surprisingly) */
      for (int tryNo = 0; tryNo < retries; ++tryNo)
      {
          if ( ((DBClientReplicaSet*)connection)->connect())
        {
          connected = true;
          break;
        }

        if (tryNo == 0)
          LM_E(("Database Startup Error (cannot connect to mongo - doing %d retries with a %d microsecond interval)", retries, RECONNECT_DELAY));
        else
          LM_T(LmtMongo, ("Try %d connecting to mongo failed", tryNo));

        usleep(RECONNECT_DELAY * 1000); // usleep accepts microseconds
      }


    }

    if (connected == false)
    {
      mongoSemGive(__FUNCTION__, "connecting to mongo failed");
      LM_E(("Database Error (connection failed, after %d retries: '%s')", retries, err.c_str()));
      return false;
    }

    LM_I(("Successful connection to database"));

    /* Authentication is different depending if multiservice is used or not. In the case of not
     * using multiservice, we authenticate in the single-service database. In the case of using
     * multiservice, it isn't a default database that we know at contextBroker start time (when
     * this connection function is invoked) so we authenticate on the admin database, which provides
     * access to any database */
    if (multitenant) {
        if (strlen(username) != 0 && strlen(passwd) != 0) {
            if (!connection->auth("admin", std::string(username), std::string(passwd), err))
            {
                mongoSemGive(__FUNCTION__, "connecting to mongo failed during authentication");
                LM_E(("Database Startup Error (authentication: db='admin', username='%s', password='*****': %s)", username, err.c_str()));
                return false;
            }
        }
    }
    else {
        if (strlen(db) != 0 && strlen(username) != 0 && strlen(passwd) != 0) {
            if (!connection->auth(std::string(db), std::string(username), std::string(passwd), err))
            {
                mongoSemGive(__FUNCTION__, "connecting to mongo failed during authentication");
                LM_E(("Database Startup Error (authentication: db='%s', username='%s', password='*****': %s)", db, username, err.c_str()));
                return false;
            }
        }
    }

    /* Get mongo version with the 'buildinfo' command */
    BSONObj result;
    std::string extra;
    connection->runCommand("admin", BSON("buildinfo" << 1), result);
    std::string versionString = std::string(result.getStringField("version"));
    if (!versionParse(versionString, mongoVersionMayor, mongoVersionMinor, extra))
    {
        mongoSemGive(__FUNCTION__, "wrong mongo version format");
        LM_E(("Database Startup Error (invalid version format: %s)", versionString.c_str()));
        return false;
    }
    LM_T(LmtMongo, ("mongo version server: %s (mayor: %d, minor: %d, extra: %s)", versionString.c_str(), mongoVersionMayor, mongoVersionMinor, extra.c_str()));

    mongoSemGive(__FUNCTION__, "connecting to mongo");
    return true;
}

/* ****************************************************************************
*
* mongoConnect -
*
* Version of the functions that doesn't uses authentication parameters
*
*/
bool mongoConnect(const char* host) {

    return mongoConnect(host, "", "", "", "", false);
}

/* ****************************************************************************
*
* mongoDisconnect -
*
* This method is intended for unit testing, that needs the DBClientConnection
* object to be mocked.
*
*/
void mongoDisconnect() {
    /* Safety check of null before releasing */
    if (connection != NULL) {
        delete connection;
    }
    connection = NULL;
}

/* ****************************************************************************
*
* mongoConnect -
*
* This method is intended for unit testing, that needs the DBClientConnection
* object to be mocked.
*
*/
#ifdef UNIT_TEST
bool mongoConnect(DBClientConnection* c) {

    connection = c;

    return true;
}
#endif

/*****************************************************************************
*
* getNotifier -
*/
Notifier* getNotifier() {
    return notifier;
}

/*****************************************************************************
*
* setNotifier -
*/
void setNotifier(Notifier* n) {
    notifier = n;
}

/* ****************************************************************************
*
* getMongoConnection -
*
* I would prefer to have per-collection methods, to have a better encapsulation, but
* the Mongo C++ API doesn't seem to work that way
*/
DBClientBase* getMongoConnection(void)
{
    return connection;
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
*
*/
extern void getOrionDatabases(std::vector<std::string>& dbs) {

    BSONObj result;
    mongoSemTake(__FUNCTION__, "get Orion databases");
    connection->runCommand("admin", BSON("listDatabases" << 1), result);
    mongoSemGive(__FUNCTION__, "get Orion databases");

    std::vector<BSONElement> databases = result.getField("databases").Array();

    for (std::vector<BSONElement>::iterator i = databases.begin(); i != databases.end(); ++i) {
        BSONObj db = (*i).Obj();
        std::string dbName = STR_FIELD(db, "name");
        std::string prefix = dbPrefix + "-";
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
void setEntitiesCollectionName(std::string name) {
    entitiesCollectionName = name;
}

/*****************************************************************************
*
* setRegistrationsCollectionName -
*/
void setRegistrationsCollectionName(std::string name) {
    registrationsCollectionName = name;
}

/*****************************************************************************
*
* setSubscribeContextCollectionName -
*/
void setSubscribeContextCollectionName(std::string name) {
    subscribeContextCollectionName = name;
}

/*****************************************************************************
*
* setSubscribeContextAvailabilityCollectionName -
*/
void setSubscribeContextAvailabilityCollectionName(std::string name) {
    subscribeContextAvailabilityCollectionName = name;
}

/*****************************************************************************
*
* setAssociationsCollectionName -
*/
void setAssociationsCollectionName(std::string name) {
    assocationsCollectionName = name;
}

/*****************************************************************************
*
* composeCollectionName -
*
* Common helper function for composing collection names
*/
static std::string composeCollectionName(std::string tenant, std::string colName) {
  return composeDatabaseName(tenant) + "." + colName;
}

/*****************************************************************************
*
* composeDatabaseName -
*
* Common helper function for composing database names
*/
std::string composeDatabaseName(std::string tenant) {
    std::string result;
    if (tenant == "")
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
std::string getEntitiesCollectionName(std::string tenant) {
    return composeCollectionName(tenant, entitiesCollectionName);
}

/*****************************************************************************
*
* getRegistrationsCollectionName -
*/
std::string getRegistrationsCollectionName(std::string tenant) {
    return composeCollectionName(tenant, registrationsCollectionName);
}

/*****************************************************************************
*
* getSubscribeContextCollectionName -
*/
std::string getSubscribeContextCollectionName(std::string tenant) {
    return composeCollectionName(tenant, subscribeContextCollectionName);
}

/*****************************************************************************
*
* getSubscribeContextAvailabilityCollectionName -
*/
std::string getSubscribeContextAvailabilityCollectionName(std::string tenant) {
    return composeCollectionName(tenant, subscribeContextAvailabilityCollectionName);
}

/*****************************************************************************
*
* getAssociationsCollectionName -
*/
std::string getAssociationsCollectionName(std::string tenant) {
    return composeCollectionName(tenant, assocationsCollectionName);
}

/*****************************************************************************
*
* mongoLocationCapable -
*/
bool mongoLocationCapable(void) {
    /* Geo location based in 2dsphere indexes was introduced in MongoDB 2.4 */
    return ((mongoVersionMayor == 2) && (mongoVersionMinor >= 4)) || (mongoVersionMayor > 2);
}

/*****************************************************************************
*
* ensureLocationIndex -
*/
void ensureLocationIndex(std::string tenant) {
    /* Ensure index for entity locations, in the case of using 2.4 */
    if (mongoLocationCapable()) {
        std::string index = ENT_LOCATION "." ENT_LOCATION_COORDS;
        connection->ensureIndex(getEntitiesCollectionName(tenant).c_str(), BSON(index << "2dsphere" ));
        LM_T(LmtMongo, ("ensuring 2dsphere index on %s (tenant %s)", index.c_str(), tenant.c_str()));
    }
}

/* ****************************************************************************
*
* getOntimeIntervalsThreads -
*/
static void getOntimeIntervalsThreads(std::string tenant, std::vector<BSONObj>& subs)
{
  /* Look for ONTIMEINTERVAL subscriptions in database */
  std::string condType= CSUB_CONDITIONS "." CSUB_CONDITIONS_TYPE;
  BSONObj query = BSON(condType << ON_TIMEINTERVAL_CONDITION);

  DBClientBase* connection = getMongoConnection();
  auto_ptr<DBClientCursor> cursor;
  try
  {
    LM_T(LmtMongo, ("query() in '%s' collection: '%s'", getSubscribeContextCollectionName(tenant).c_str(), query.toString().c_str()));
    mongoSemTake(__FUNCTION__, "query in SubscribeContextCollection");
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
    mongoSemGive(__FUNCTION__, "query in SubscribeContextCollection");
    LM_I(("Database Operation Successful (%s)", query.toString().c_str()));
  }
  catch (const DBException &e)
  {
    mongoSemGive(__FUNCTION__, "query in SubscribeContextCollection (mongo db exception)");
    LM_E(("Database Error (DBException: %s)", e.what()));
    return;
  }
  catch (...)
  {
    mongoSemGive(__FUNCTION__, "query in SubscribeContextCollection (mongo generic exception)");
    LM_E(("Database Error (generic exception)"));
    return;
  }

  /* Store all subs in returning vector */
  while (cursor->more())
  {
    subs.push_back(cursor->next());
  }
}

/* ****************************************************************************
*
* recoverOntimeIntervalThreads -
*/
void recoverOntimeIntervalThreads(std::string tenant)
{

  /* Look for all ONTIMEINTERVAL ids on the given tenant */
  std::vector<BSONObj> ids;
  getOntimeIntervalsThreads(tenant, ids);

  /* For each one of the subscriptions found, create threads */
  for (unsigned int ix = 0; ix < ids.size(); ++ix)
  {

    BSONObj     sub     = ids[ix];
    BSONElement idField = sub.getField("_id");

    //
    // BSONElement::eoo returns true if 'not found', i.e. the field "_id" doesn't exist in 'sub'
    //
    // Now, if 'sub.getField("_id")' is not found, if we continue, calling OID() on it, then we get 
    // an exception and the broker crashes. 
    //
    if (idField.eoo() == true)
    {
      LM_E(("Database Error (error retrieving _id field in doc: %s)", sub.toString().c_str()));
      continue;
    }

    std::string subId = idField.OID().str();

    std::vector<BSONElement> condV = sub.getField(CSUB_CONDITIONS).Array();
    for (unsigned int ix = 0; ix < condV.size(); ++ix) {
      BSONObj condition = condV[ix].embeddedObject();
      if (strcmp(STR_FIELD(condition, CSUB_CONDITIONS_TYPE).c_str(), ON_TIMEINTERVAL_CONDITION) == 0) {
        int interval = condition.getIntField(CSUB_CONDITIONS_VALUE);
        LM_T(LmtNotifier, ("creating ONTIMEINTERVAL for subscription %s with interval %d (tenant %s)", subId.c_str(), interval, tenant.c_str()));
        processOntimeIntervalCondition(subId, interval, tenant);
      }
    }
  }
}

/* ****************************************************************************
*
* destroyAllOntimeIntervalThreads -
*
* This function is only to be used under harakiri mode, not for real use
*/
extern void destroyAllOntimeIntervalThreads(std::string tenant)
{
  /* Look for all ONTIMEINTERVAL ids on the given tenant */
  std::vector<BSONObj> ids;
  getOntimeIntervalsThreads(tenant, ids);

  /* Iterate destroying thread by thread */
  for (unsigned int ix = 0; ix < ids.size(); ++ix)
  {
    BSONObj      sub     = ids[ix];
    BSONElement  idField = sub.getField("_id");

    //
    // BSONElement::eoo returns true if 'not found', i.e. the field "_id" doesn't exist in 'sub'
    //
    // Now, if 'sub.getField("_id")' is not found, if we continue, calling OID() on it, then we get 
    // an exception and the broker crashes. 
    //
    if (idField.eoo() == true)
    {
      LM_E(("Database Error (error retrieving _id field in doc: %s)", sub.toString().c_str()));
      continue;
    }

    std::string  subId = idField.OID().str();

    notifier->destroyOntimeIntervalThreads(subId);
  }

}

/* ****************************************************************************
*
* includedEntity -
*/
bool includedEntity(EntityId en, EntityIdVector* entityIdV) {

    for (unsigned int ix = 0; ix < entityIdV->size(); ++ix) {
        EntityId* en2 = entityIdV->get(ix);

        bool idMatch;
        if (isTrue(en2->isPattern)) {
            regex_t regex;
            if (regcomp(&regex, en2->id.c_str(), 0) != 0) {
                LM_W(("Bad Input (error compiling regex: '%s')", en2->id.c_str()));
                continue;
            }
            if (regexec(&regex, en.id.c_str(), 0, NULL, 0) == 0) {
                idMatch = true;
            }
            else {
                idMatch = false;
            }
            regfree(&regex);
        }
        else {  /* isPattern=false */
            idMatch = (en2->id == en.id);
        }

        /* Note that type == "" is like a * wildcard */
        if (idMatch && (en.type == "" || en2->type == "" || en2->type == en.type)) {
            return true;
        }
    }
    return false;
}

/* ****************************************************************************
*
* includedAttribute -
*/
bool includedAttribute(ContextRegistrationAttribute attr, AttributeList* attrsV) {

    /* That's the case in which the discoverAvailabilityRequest doesn't include attributes, so all the
       attributes are included in the response*/
    if (attrsV->size() == 0) {
        return true;
    }

    for (unsigned int ix = 0; ix < attrsV->size(); ++ix) {
        if (attrsV->get(ix) == attr.name) {
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
bool includedAttribute(ContextAttribute attr, AttributeList* attrsV) {

    /* That's the case in which the queryContextRequest doesn't include attributes, so all the
       attributes are included in the response*/
    if (attrsV->size() == 0) {
        return true;
    }

    for (unsigned int ix = 0; ix < attrsV->size(); ++ix) {
        if (attrsV->get(ix) == attr.name) {
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
    ent.appendRegex(idString, enP->id);
  else
    ent.append(idString, enP->id);

  if (enP->type != "")
    ent.append(typeString, enP->type);

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
static BSONObj fillQueryServicePath(const std::vector<std::string>& servicePath)
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
    for (unsigned int ix = 0 ; ix < servicePath.size(); ++ix) {
      LM_T(LmtServicePath, ("Service Path: '%s'", servicePath[ix].c_str()));

      /* Add "null" in the following service path cases: / or /#. In order to avoid adding null
             * several times, the nullAdded flag is used */
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
    /* We need the default clause to avoid 'enumeration value X not handled in switch' errors due to -Werror=switch at compilation time */
    break;
  }

  cvP->add(child);
}

/* ****************************************************************************
*
* compoundObjectResponse -
*
*/
static void compoundObjectResponse(orion::CompoundValueNode* cvP, const BSONElement& be) {
    BSONObj obj = be.embeddedObject();
    cvP->type = orion::CompoundValueNode::Object;
    for( BSONObj::iterator i = obj.begin(); i.more(); ) {
        BSONElement e = i.next();
        addCompoundNode(cvP, e);
    }

}

/* ****************************************************************************
*
* compoundVectorResponse -
*/
static void compoundVectorResponse(orion::CompoundValueNode* cvP, const BSONElement& be) {
    std::vector<BSONElement> vec = be.Array();
    cvP->type = orion::CompoundValueNode::Vector;
    for( unsigned int ix = 0; ix < vec.size(); ++ix) {
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
static bool processAreaScope(Scope* scoP, BSONObj &areaQuery) {

  if (!mongoLocationCapable())
  {
    LM_W(("Bad Input (location scope was found but your MongoDB version doesn't support it. Please upgrade MongoDB server to 2.4 or newer)"));
    return false;
  }

  bool inverted = false;

  BSONObj geoWithin;
  if (scoP->areaType == orion::CircleType)
  {
    double radians = scoP->circle.radius() / EARTH_RADIUS_METERS;
    geoWithin = BSON("$centerSphere" << BSON_ARRAY(BSON_ARRAY(scoP->circle.center.longitude() << scoP->circle.center.latitude()) << radians ));
    inverted = scoP->circle.inverted();
  }
  else if (scoP->areaType== orion::PolygonType)
  {
    BSONArrayBuilder vertex;
    double lat0 = 0;
    double lon0 = 0;

    for (unsigned int jx = 0; jx < scoP->polygon.vertexList.size() ; ++jx)
    {
      double lat = scoP->polygon.vertexList[jx]->latitude();
      double lon = scoP->polygon.vertexList[jx]->longitude();

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

    inverted = scoP->polygon.inverted();
  }
  else
  {
    LM_W(("Bad Input (unknown area type)"));
    return false;
  }

  if (inverted) {
    areaQuery = BSON("$not" << BSON("$geoWithin" << geoWithin));
  }
  else {
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
*
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
  bool                             details,
  long long*                       countP
)
{
    DBClientBase* connection = getMongoConnection();

    /* Query structure is as follows
     *
     * {
     *    "$or": [ ... ],            (always)
     *    "_id.servicePath: { ... }   (always, in some cases using {$exists: false})
     *    "attrs.name": { ... },     (only if attributes are used in the query)
     *    "location.coords": { ... } (only in the case of geo-queries)
     *  }
     *
     */

    BSONObjBuilder finalQuery;

    /* Part 1: entities */
    BSONArrayBuilder orEnt;

    for (unsigned int ix = 0; ix < enV.size(); ++ix)
      fillQueryEntity(orEnt, enV.get(ix));

    /* The result of orEnt is appended to the final query */
    finalQuery.append("$or", orEnt.arr());

    /* Part 2: service path */
    const std::string  servicePathString = "_id." ENT_SERVICE_PATH;
    finalQuery.append(servicePathString, fillQueryServicePath(servicePath));

    /* Part 3: attributes */
    BSONArrayBuilder attrs;
    for (unsigned int ix = 0; ix < attrL.size(); ++ix) {
        std::string attrName = attrL.get(ix);
        attrs.append(attrName);
        LM_T(LmtMongo, ("Attribute query token: '%s'", attrName.c_str()));
    }
    std::string attrNames = ENT_ATTRS "." ENT_ATTRS_NAME;
    if (attrs.arrSize() > 0) {
        /* If we don't do this checking, the {$in: [] } in the attribute name part will
         * make the query fail*/
        finalQuery.append(attrNames, BSON("$in" << attrs.arr()));
    }

    /* Part 5: scopes */
    std::vector<BSONObj> filters;
    unsigned int geoScopes = 0;
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

    LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));

    /* Do the query on MongoDB */
    auto_ptr<DBClientCursor>  cursor;
    BSONObj                   bquery = finalQuery.obj();
    Query                     query(bquery);
    Query                     sortCriteria  = query.sort(BSON(ENT_CREATION_DATE << 1));

    LM_T(LmtMongo, ("query() in '%s' collection: '%s'", getEntitiesCollectionName(tenant).c_str(), query.toString().c_str()));
    mongoSemTake(__FUNCTION__, "query in EntitiesCollection");

    try
    {
        if ((details == true) && (countP != NULL))
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

        mongoSemGive(__FUNCTION__, "query in EntitiesCollection");
        LM_I(("Database Operation Successful (%s)", query.toString().c_str()));
    }
    catch (const DBException& e)
    {
        mongoSemGive(__FUNCTION__, "query in EntitiesCollection (mongo db exception)");
        *err = std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
                " - query(): " + query.toString() +
                " - exception: " + e.what();

        LM_E(("Database Error (%s)", err->c_str()));
        return false;
    }
    catch (...)
    {
        mongoSemGive(__FUNCTION__, "query in EntitiesCollection (mongo generic exception)");
        *err = std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
                " - query(): " + query.toString() +
                " - exception: " + "generic";

        LM_E(("Database Error (%s)", err->c_str()));
        return false;
    }

    /* Process query result */
    while (cursor->more()) {

        BSONObj r = cursor->next();
        LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));
        ContextElementResponse* cer = new ContextElementResponse();
        cer->statusCode.fill(SccOk);

        /* Entity part */

        BSONObj queryEntity = r.getObjectField("_id");

        cer->contextElement.entityId.id          = STR_FIELD(queryEntity, ENT_ENTITY_ID);
        cer->contextElement.entityId.type        = STR_FIELD(queryEntity, ENT_ENTITY_TYPE);
        cer->contextElement.entityId.servicePath = STR_FIELD(queryEntity, ENT_SERVICE_PATH);
        cer->contextElement.entityId.isPattern   = "false";

        /* Get the location attribute (if it exists) */
        std::string locAttr;
        if (r.hasElement(ENT_LOCATION)) {
            locAttr = r.getObjectField(ENT_LOCATION).getStringField(ENT_LOCATION_ATTRNAME);
        }

        /* Attributes part */

        std::vector<BSONElement> queryAttrV = r.getField(ENT_ATTRS).Array();
        for (unsigned int ix = 0; ix < queryAttrV.size(); ++ix) {

            ContextAttribute ca;

            BSONObj queryAttr = queryAttrV[ix].embeddedObject();

            ca.name = STR_FIELD(queryAttr, ENT_ATTRS_NAME);
            ca.type = STR_FIELD(queryAttr, ENT_ATTRS_TYPE);

            /* Note that includedAttribute decision is based on name and type. Value is set only if
             * decision is positive */
            if (includedAttribute(ca, &attrL)) {

                ContextAttribute* caP;
                if (queryAttr.getField(ENT_ATTRS_VALUE).type() == String) {
                    ca.value = STR_FIELD(queryAttr, ENT_ATTRS_VALUE);
                    if (!includeEmpty && ca.value.length() == 0) {
                        continue;
                    }
                    caP = new ContextAttribute(ca.name, ca.type, ca.value);
                }
                else if (queryAttr.getField(ENT_ATTRS_VALUE).type() == Object) {
                    caP = new ContextAttribute(ca.name, ca.type);
                    caP->compoundValueP = new orion::CompoundValueNode(orion::CompoundValueNode::Object);
                    compoundObjectResponse(caP->compoundValueP, queryAttr.getField(ENT_ATTRS_VALUE));
                }
                else if (queryAttr.getField(ENT_ATTRS_VALUE).type() == Array) {
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
                if (STR_FIELD(queryAttr, ENT_ATTRS_ID) != "") {
                    Metadata* md = new Metadata(NGSI_MD_ID, "string", STR_FIELD(queryAttr, ENT_ATTRS_ID));
                    caP->metadataVector.push_back(md);
                }
                if (locAttr == ca.name) {
                    Metadata* md = new Metadata(NGSI_MD_LOCATION, "string", LOCATION_WGS84);
                    caP->metadataVector.push_back(md);
                }

                /* Setting custom metadata (if any) */
                if (queryAttr.hasField(ENT_ATTRS_MD)) {
                    std::vector<BSONElement> metadataV = queryAttr.getField(ENT_ATTRS_MD).Array();
                    for (unsigned int ix = 0; ix < metadataV.size(); ++ix) {

                        BSONObj metadata = metadataV[ix].embeddedObject();
                        Metadata* md;

                        if (metadata.hasField(ENT_ATTRS_MD_TYPE)) {
                            md = new Metadata(STR_FIELD(metadata, ENT_ATTRS_MD_NAME), STR_FIELD(metadata, ENT_ATTRS_MD_TYPE), STR_FIELD(metadata, ENT_ATTRS_MD_VALUE));
                        }
                        else {
                            md = new Metadata(STR_FIELD(metadata, ENT_ATTRS_MD_NAME), "", STR_FIELD(metadata, ENT_ATTRS_MD_VALUE));
                        }

                        caP->metadataVector.push_back(md);
                    }
                }

                cer->contextElement.contextAttributeVector.push_back(caP);
            }
        }

        cer->statusCode.fill(SccOk);

        cerV->push_back(cer);
    }

    return true;
}

/*****************************************************************************
*
* processEntity -
*/
static void processEntity(ContextRegistrationResponse* crr, EntityIdVector enV, BSONObj entity) {

    EntityId en;

    en.id = STR_FIELD(entity, REG_ENTITY_ID);
    en.type = STR_FIELD(entity, REG_ENTITY_TYPE);
    /* isPattern = true is not allowed in registrations so it is not in the
     * document retrieved with the query; however we will set it to be formally correct
     * with NGSI spec */
    en.isPattern = std::string("false");

    if (includedEntity(en, &enV)) {       
        EntityId* enP = new EntityId(en.id, en.type, en.isPattern);
        crr->contextRegistration.entityIdVector.push_back(enP);
    }
}

/*****************************************************************************
*
* processAttribute -
*/
static void processAttribute(ContextRegistrationResponse* crr, AttributeList attrL, BSONObj attribute) {

    ContextRegistrationAttribute attr(
                STR_FIELD(attribute, REG_ATTRS_NAME),
                STR_FIELD(attribute, REG_ATTRS_TYPE),
                STR_FIELD(attribute, REG_ATTRS_ISDOMAIN));

   // FIXME: we don't take metadata into account at the moment
   //attr.metadataV = ..

   if (includedAttribute(attr, &attrL)) {

      ContextRegistrationAttribute* attrP = new ContextRegistrationAttribute(attr.name, attr.type, attr.isDomain);
      crr->contextRegistration.contextRegistrationAttributeVector.push_back(attrP);
   }

}

/*****************************************************************************
*
* processContextRegistrationElement -
*/
static void processContextRegistrationElement (BSONObj cr, EntityIdVector enV, AttributeList attrL, ContextRegistrationResponseVector* crrV) {

    ContextRegistrationResponse crr;

    crr.contextRegistration.providingApplication.set(STR_FIELD(cr, REG_PROVIDING_APPLICATION));

    std::vector<BSONElement> queryEntityV = cr.getField(REG_ENTITIES).Array();
    for (unsigned int ix = 0; ix < queryEntityV.size(); ++ix) {
        processEntity(&crr, enV, queryEntityV[ix].embeddedObject());
    }

    /* Note that attributes can be included only if at least one entity has been found */
    if (crr.contextRegistration.entityIdVector.size() > 0) {
        std::vector<BSONElement> queryAttrV = cr.getField(REG_ATTRS).Array();
        for (unsigned int ix = 0; ix < queryAttrV.size(); ++ix) {
            processAttribute(&crr, attrL, queryAttrV[ix].embeddedObject());
        }
    }

    // FIXME: we don't take metadata into account at the moment
    //crr.contextRegistration.registrationMetadataV = ..

    /* Note that the context registration element is only included in one of the following cases:
    * - The number of entities and attributes included are both greater than 0
    * - The number of entities is greater than 0, the number of attributes is 0 but the discover
    *   doesn't use attributes in the request
    */
    if (crr.contextRegistration.entityIdVector.size() == 0) {
        return;
    }

    if ( crr.contextRegistration.contextRegistrationAttributeVector.size() > 0  ||
        (crr.contextRegistration.contextRegistrationAttributeVector.size() == 0 && attrL.size() == 0)) {

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
  int                                 offset,
  int                                 limit,
  bool                                details,
  long long*                          countP
)
{
    DBClientBase* connection = getMongoConnection();

    /* Build query based on arguments */
    // FIXME P2: this implementation need to be refactored for cleanup
    std::string contextRegistrationEntities     = REG_CONTEXT_REGISTRATION "." REG_ENTITIES;
    std::string contextRegistrationEntitiesId   = REG_CONTEXT_REGISTRATION "." REG_ENTITIES "." REG_ENTITY_ID;
    std::string contextRegistrationEntitiesType = REG_CONTEXT_REGISTRATION "." REG_ENTITIES "." REG_ENTITY_TYPE;
    std::string contextRegistrationAttrsNames   = REG_CONTEXT_REGISTRATION "." REG_ATTRS    "." REG_ATTRS_NAME;

    BSONArrayBuilder entityOr;
    BSONArrayBuilder entitiesWithType;
    BSONArrayBuilder entitiesWithoutType;    

    for (unsigned int ix = 0; ix < enV.size(); ++ix) {        
        EntityId* en = enV.get(ix);
        if (isTrue(en->isPattern)) {
            BSONObjBuilder b;
            b.appendRegex(contextRegistrationEntitiesId, en->id);
            if (en->type != "") {
                b.append(contextRegistrationEntitiesType, en->type);
            }
            entityOr.append(b.obj());
        }
        else { /* isPattern = false */
            if (en->type == "") {
                entitiesWithoutType.append(en->id);
                LM_T(LmtMongo, ("Entity discovery without type: id '%s'", en->id.c_str()));
            }
            else {
                /* We have detected that sometimes mongo stores { id: ..., type ...} and others { type: ..., id: ...},
                   so we have to take both them into account */
                entitiesWithType.append(BSON(REG_ENTITY_ID << en->id << REG_ENTITY_TYPE << en->type));
                entitiesWithType.append(BSON(REG_ENTITY_TYPE << en->type << REG_ENTITY_ID << en->id));
                LM_T(LmtMongo, ("Entity discovery: {id: %s, type: %s}", en->id.c_str(), en->type.c_str()));
            }
        }
    }
    BSONArrayBuilder attrs;
    for (unsigned int ix = 0; ix < attrL.size(); ++ix) {
        std::string attrName = attrL.get(ix);
        attrs.append(attrName);
        LM_T(LmtMongo, ("Attribute discovery: '%s'", attrName.c_str()));
    }

    entityOr.append(BSON(contextRegistrationEntities << BSON("$in" << entitiesWithType.arr())));
    entityOr.append(BSON(contextRegistrationEntitiesId << BSON("$in" <<entitiesWithoutType.arr())));

    BSONObjBuilder queryBuilder;
    /* The $or clause could be omitted if it contains only one element, but we can assume that
     * it has no impact on MongoDB query optimizer */
    queryBuilder.append("$or", entityOr.arr());
    queryBuilder.append(REG_EXPIRATION, BSON("$gt" << (long long) getCurrentTime()));
    if (attrs.arrSize() > 0) {
        /* If we don't do this checking, the {$in: [] } in the attribute name part will
         * make the query fail*/
        queryBuilder.append(contextRegistrationAttrsNames, BSON("$in" << attrs.arr()));
    }

    /* Do the query on MongoDB */
    // FIXME P2: use field selector to include the only relevant field: contextRegistration array (e.g. "expiration" is not needed)
    auto_ptr<DBClientCursor>  cursor;
    BSONObj                   bquery = queryBuilder.obj();
    Query                     query(bquery);
    Query                     sortCriteria  = query.sort(BSON("_id" << 1));

    LM_T(LmtMongo, ("query() in '%s' collection: '%s'", getRegistrationsCollectionName(tenant).c_str(), query.toString().c_str()));
    LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));
    mongoSemTake(__FUNCTION__, "query in RegistrationsCollection");

    try
    {
        if ((details == true) && (countP != NULL))
        {
          *countP = connection->count(getRegistrationsCollectionName(tenant).c_str(), bquery);
        }

        cursor = connection->query(getRegistrationsCollectionName(tenant).c_str(), query, limit, offset);
        mongoSemGive(__FUNCTION__, "query in RegistrationsCollection");
        LM_I(("Database Operation Successful (%s)", query.toString().c_str()));
    }
    catch (const DBException& e)
    {
        mongoSemGive(__FUNCTION__, "query in RegistrationsCollection (mongo db exception)");
        *err = std::string("collection: ") + getRegistrationsCollectionName(tenant).c_str() +
                " - query(): " + query.toString() +
                " - exception: " + e.what();

        LM_E(("Database Error (%s)", err->c_str()));
        return false;
    }
    catch (...)
    {
        mongoSemGive(__FUNCTION__, "query in RegistrationsCollection (mongo generic exception)");
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

        std::vector<BSONElement> queryContextRegistrationV = r.getField(REG_CONTEXT_REGISTRATION).Array();
        for (unsigned int ix = 0 ; ix < queryContextRegistrationV.size(); ++ix)
        {
            processContextRegistrationElement(queryContextRegistrationV[ix].embeddedObject(), enV, attrL, crrV);
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
bool isCondValueInContextElementResponse(ConditionValueList* condValues, ContextElementResponseVector* cerV) {

    for (unsigned int cvlx = 0; cvlx < condValues->size(); ++cvlx) {
        for (unsigned int aclx = 0; aclx < cerV->size(); ++aclx) {
            ContextAttributeVector caV = cerV->get(aclx)->contextElement.contextAttributeVector;
            for (unsigned int kx = 0; kx < caV.size(); ++kx) {
                if (caV.get(kx)->name == condValues->get(cvlx)) {
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
EntityIdVector subToEntityIdVector(BSONObj sub) {
    EntityIdVector enV;
    std::vector<BSONElement> subEnts = sub.getField(CSUB_ENTITIES).Array();
    for (unsigned int ix = 0; ix < subEnts.size() ; ++ix) {
        BSONObj subEnt = subEnts[ix].embeddedObject();
        EntityId* en = new EntityId(STR_FIELD(subEnt, CSUB_ENTITY_ID),
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
AttributeList subToAttributeList(BSONObj sub) {
    AttributeList attrL;
    std::vector<BSONElement> subAttrs = sub.getField(CSUB_ATTRS).Array();
    for (unsigned int ix = 0; ix < subAttrs.size() ; ++ix) {
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
  EntityIdVector      enV,
  AttributeList       attrL,
  ConditionValueList* condValues,
  std::string         subId,
  std::string         notifyUrl,
  Format              format,
  std::string         tenant,
  const std::string&  xauthToken
)
{
  std::string          err;
  NotifyContextRequest ncr;

    // FIXME P10: we are using dummy scope at the moment, until subscription scopes get implemented
    // FIXME P10: we are using an empty service path vector until service paths get implemented for subscriptions
    std::vector<std::string> servicePathV;
    Restriction res;    
    if (!entitiesQuery(enV, attrL, res, &ncr.contextElementResponseVector, &err, false, tenant, servicePathV))
    {
        ncr.contextElementResponseVector.release();
        return false;
    }

    if (ncr.contextElementResponseVector.size() > 0)
    {
        /* Complete the fields in NotifyContextRequest */
        ncr.subscriptionId.set(subId);
        //FIXME: we use a proper origin name
        ncr.originator.set("localhost");

        if (condValues != NULL) {
            /* Check if some of the attributes in the NotifyCondition values list are in the entity.
             * Note that in this case we do a query for all the attributes, not restricted to attrV */
            ContextElementResponseVector allCerV;
            AttributeList emptyList;
            // FIXME P10: we are using dummy scope by the moment, until subscription scopes get implemented
            // FIXME P10: we are using an empty service path vector until serive paths get implemented for subscriptions
            if (!entitiesQuery(enV, emptyList, res, &allCerV, &err, false, tenant, servicePathV))
            {
                allCerV.release();
                ncr.contextElementResponseVector.release();
                return false;
            }

            if (isCondValueInContextElementResponse(condValues, &allCerV)) {
                /* Send notification */
                getNotifier()->sendNotifyContextRequest(&ncr, notifyUrl, tenant, xauthToken, format);
                allCerV.release();
                ncr.contextElementResponseVector.release();
                return true;
            }

            allCerV.release();
        }
        else {
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
void processOntimeIntervalCondition(std::string subId, int interval, std::string tenant) {

    getNotifier()->createIntervalThread(subId, interval, tenant);
}

/* ****************************************************************************
*
* processConditionVector -
*
*/
BSONArray processConditionVector(NotifyConditionVector* ncvP, EntityIdVector enV, AttributeList attrL, std::string subId, std::string url, bool* notificationDone, Format format, std::string tenant, const std::string& xauthToken)
{
    BSONArrayBuilder conds;
    *notificationDone = false;

    for (unsigned int ix = 0; ix < ncvP->size(); ++ix) {
        NotifyCondition* nc = ncvP->get(ix);
        if (nc->type == ON_TIMEINTERVAL_CONDITION) {

            Duration interval;
            interval.set(nc->condValueList.get(0));
            interval.parse();

            conds.append(BSON(CSUB_CONDITIONS_TYPE << ON_TIMEINTERVAL_CONDITION <<
                              CSUB_CONDITIONS_VALUE << (long long) interval.seconds));

            processOntimeIntervalCondition(subId, interval.seconds, tenant);
        }
        else if (nc->type == ON_CHANGE_CONDITION) {

            /* Create an array holding the list of condValues */
            BSONArrayBuilder condValues;
            for (unsigned int jx = 0; jx < nc->condValueList.size(); ++jx) {
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
                                         xauthToken))
            {
              *notificationDone = true;
            }

        }
        else {  // ON_VALUE_CONDITION
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
static HttpStatusCode mongoUpdateCasubNewNotification(std::string subId, std::string* err, std::string tenant) {

    LM_T(LmtMongo, ("Update NGSI9 Subscription New Notification"));

    DBClientBase* connection = getMongoConnection();

    /* Update the document */
    BSONObj query = BSON("_id" << OID(subId));
    BSONObj update = BSON("$set" << BSON(CASUB_LASTNOTIFICATION << getCurrentTime()) << "$inc" << BSON(CASUB_COUNT << 1));
    LM_T(LmtMongo, ("update() in '%s' collection: (%s,%s)", getSubscribeContextAvailabilityCollectionName(tenant).c_str(),
                    query.toString().c_str(),
                    update.toString().c_str()));

    mongoSemTake(__FUNCTION__, "update in SubscribeContextAvailabilityCollection");

    try
    {
        connection->update(getSubscribeContextAvailabilityCollectionName(tenant).c_str(), query, update);
        mongoSemGive(__FUNCTION__, "update in SubscribeContextAvailabilityCollection");
        LM_I(("Database Operation Successful (%s)", query.toString().c_str()));
    }
    catch (const DBException &e)
    {
        mongoSemGive(__FUNCTION__, "update in SubscribeContextAvailabilityCollection (mongo db exception)");
        *err = e.what();
        LM_E(("Database Error ('update[%s:%s] in %s', '%s')", query.toString().c_str(), update.toString().c_str(), getSubscribeContextAvailabilityCollectionName(tenant).c_str(), e.what()));
        return SccOk;
    }
    catch (...)
    {
        mongoSemGive(__FUNCTION__, "update in SubscribeContextAvailabilityCollection (mongo generic exception)");
        *err = "Database error - exception thrown";
        LM_E(("Database Error ('update[%s:%s] in %s', '%s')", query.toString().c_str(), update.toString().c_str(), getSubscribeContextAvailabilityCollectionName(tenant).c_str(), "generic exception"));
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
bool processAvailabilitySubscription(EntityIdVector enV, AttributeList attrL, std::string subId, std::string notifyUrl, Format format, std::string tenant) {

    std::string err;
    NotifyContextAvailabilityRequest ncar;

    if (!registrationsQuery(enV, attrL, &ncar.contextRegistrationResponseVector, &err, tenant))
    {
       ncar.contextRegistrationResponseVector.release();
       return false;
    }

    if (ncar.contextRegistrationResponseVector.size() > 0) {

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
      ++slashes;

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
      return;

    if (*from == '/')
      to[ix++] = '\\';
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
  for (std::map<string, TriggeredSubscription*>::iterator it = subs.begin(); it != subs.end(); ++it)
  {
      delete it->second;
  }
  subs.clear();
}
