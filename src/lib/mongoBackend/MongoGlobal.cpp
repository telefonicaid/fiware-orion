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
#include "ngsiNotify/Notifier.h"
#include "rest/StringFilter.h"
#include "apiTypesV2/Subscription.h"

#include "mongoBackend/dbConstants.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/compoundResponses.h"
#include "mongoBackend/MongoGlobal.h"

#include "mongoDriver/mongoConnectionPool.h"
#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/safeMongo.h"
#include "mongoDriver/BSONArray.h"
#include "mongoDriver/BSONArrayBuilder.h"
#include "mongoDriver/BSONElement.h"


/* ****************************************************************************
*
* USING
*/
using ngsiv2::HttpInfo;



/* ****************************************************************************
*
* Globals
*/
static std::string          dbPrefix;
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
)
{
  // Set the global multitenant variable
  multitenant = mtenant;

  if (orion::mongoConnectionPoolInit(dbURI,
                                     dbName.c_str(),
                                     pwd,
                                     mtenant,
                                     writeConcern,
                                     dbPoolSize,
                                     mutexTimeStat) != 0)
  {
    LM_X(1, ("Fatal Error (MongoDB error)"));
  }

  setDbPrefix(dbName);

  //
  // Note that index creation operation is idempotent.
  // From http://docs.mongodb.org/manual/reference/method/db.collection.ensureIndex:
  // "If you call multiple ensureIndex() methods with the same index specification at the same time,
  // only the first operation will succeed, all other operations will have no effect."
  //
  if (!mtenant)
  {
    // To avoid creating and empty 'orion' database when -multiservice is in use
    ensureLocationIndex("");
    ensureDateExpirationIndex("");
  }
  else
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
  orion::BSONObj  result;
  std::string     err;

  orion::BSONObjBuilder bob;
  bob.append("listDatabases", 1);
  bob.append("nameOnly", true);

  if (!orion::runDatabaseCommand("admin", bob.obj(), &result, &err))
  {
    return false;
  }

  if (!result.hasField("databases"))
  {
    LM_E(("Runtime Error (no 'databases' field in %s)", result.toString().c_str()));
    return false;
  }

  std::vector<orion::BSONElement> databases = getFieldF(result, "databases").Array();
  for (std::vector<orion::BSONElement>::iterator i = databases.begin(); i != databases.end(); ++i)
  {
    orion::BSONObj  db      = (*i).embeddedObject();
    std::string     dbName  = getStringFieldF(db, "name");
    std::string     prefix  = dbPrefix + "-";

    if (strncmp(prefix.c_str(), dbName.c_str(), strlen(prefix.c_str())) == 0)
    {
      // Check for size of database name
      if (strlen(dbName.c_str()) <= DB_AND_SERVICE_NAME_MAX_LEN)
      {
        LM_T(LmtMongo, ("Orion database found: %s", dbName.c_str()));
        dbsP->push_back(dbName);
        LM_T(LmtBug, ("Pushed back db name '%s'", dbName.c_str()));
      }
      else
      {
        LM_E(("Runtime Error (database name size should be smaller than %d characters: %s)", DB_AND_SERVICE_NAME_MAX_LEN, dbName.c_str()));
      }
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
* composeDatabaseName -
*
* Common helper function for composing database names
*/
std::string composeDatabaseName(const std::string& tenant)
{
  std::string result;

  if (!multitenant || (tenant.empty()))
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
* mongoLocationCapable -
*/
bool mongoLocationCapable(void)
{
  int mayor;
  int minor;

  /* Geo location based on 2dsphere indexes was introduced in MongoDB 2.4 */
  orion::mongoVersionGet(&mayor, &minor);

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
  orion::mongoVersionGet(&mayor, &minor);

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

    orion::BSONObjBuilder bobIndex;
    bobIndex.append(index, "2dsphere");

    std::string indexName = index + "_2dsphere";

    orion::collectionCreateIndex(composeDatabaseName(tenant), COL_ENTITIES, indexName, bobIndex.obj(), false, &err);
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

    orion::BSONObjBuilder bobIndex;
    bobIndex.append(index, 1);

    std::string indexName = index + "_1";

    orion::collectionCreateIndex(composeDatabaseName(tenant), COL_ENTITIES, indexName, bobIndex.obj(), true, &err);
    LM_T(LmtMongo, ("ensuring TTL date expiration index on %s (tenant <%s>)", index.c_str(), tenant.c_str()));
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
bool matchEntity(const EntityId* en1, const EntityId& en2)
{
  bool idMatch;

  if (!en2.idPattern.empty())
  {
    regex_t regex;

    idMatch = false;
    if (!regComp(&regex, en2.idPattern.c_str(), REG_EXTENDED))
    {
      std::string details = std::string("error compiling regex for id: '") + en2.id + "'";
      alarmMgr.badInput(clientIp, details);
    }
    else
    {
      idMatch = (regexec(&regex, en1->id.c_str(), 0, NULL, 0) == 0);

      regfree(&regex);  // If regcomp fails it frees up itself (see glibc sources for details)
    }
  }
  else  /* is not a pattern */
  {
    idMatch = (en2.id == en1->id);
  }

  // Note that type.empty() is like a * wildcard for type
  return idMatch && (en1->type.empty() || en2.type.empty() || en2.type == en1->type);
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
static void fillQueryEntity(orion::BSONObjBuilder* bobP, const EntityId* enP)
{
  const std::string  idString    = "_id." ENT_ENTITY_ID;
  const std::string  typeString  = "_id." ENT_ENTITY_TYPE;

  if (!enP->idPattern.empty())
  {
    // In the case of "universal pattern" we can avoid adding anything (simpler query)
    if (enP->idPattern != ".*")
    {
      bobP->appendRegex(idString, enP->idPattern);
    }
  }
  else
  {
    bobP->append(idString, enP->id);
  }


  if (!enP->typePattern.empty())
  {
    // In the case of "universal pattern" we can avoid adding anything (simpler query)
    if (enP->typePattern != ".*")
    {
      bobP->appendRegex(typeString, enP->typePattern);
    }
  }
  else
  {
    if (!enP->type.empty())
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
  if (servicePath[0].empty())
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
static void addServicePathInTokens(orion::BSONArrayBuilder* servicePathIn, const std::string& sp)
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
orion::BSONObj fillQueryServicePath(const std::string& spKey, const std::vector<std::string>& servicePath)
{
  orion::BSONArrayBuilder servicePathIn;
  orion::BSONObjBuilder   bob;

  // Special case (although the most common one): only one service path
  if (servicePath.size() == 1)
  {
    std::string sp = servicePath[0];
    if (sp.at(sp.length() - 1) == '#')
    {
      addServicePathInTokens(&servicePathIn, sp);
      orion::BSONObjBuilder bobIn;
      bobIn.append("$in", servicePathIn.arr());
      bob.append(spKey, bobIn.obj());
    }
    else
    {
      // Use simple matching. Avoid "$in: [ ... ]" pattern. This is the most common case
      bob.append(spKey, servicePath[0]);
    }
    return bob.obj();
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

  orion::BSONObjBuilder bobIn;
  bobIn.append("$in", servicePathIn.arr());
  bob.append(spKey, bobIn.obj());
  return bob.obj();
}



/* *****************************************************************************
*
* addFilterScope -
*/
static void addFilterScope( const Scope* scoP, std::vector<orion::BSONObj>* filtersP)
{
  if ((scoP->type == SCOPE_FILTER_EXISTENCE) && (scoP->value == SCOPE_VALUE_ENTITY_TYPE))
  {
    // Early return to avoid _id.type: {$exits: true}
    return;
  }

  std::string entityTypeString = std::string("_id.") + ENT_ENTITY_TYPE;

  if (scoP->type == SCOPE_FILTER_EXISTENCE)
  {
    // Entity type existence filter only makes sense in NGSIv1
    if (scoP->value == SCOPE_VALUE_ENTITY_TYPE)
    {
      bool existValue = scoP->oper == SCOPE_OPERATOR_NOT ? false : true;

      orion::BSONObjBuilder bobInner;
      bobInner.append("$exists", existValue);

      orion::BSONObjBuilder bobOuter;
      bobOuter.append(entityTypeString, bobInner.obj());

      filtersP->push_back(bobOuter.obj());
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

  if (sortToken == SERVICE_PATH)
  {
    return std::string("_id.") + ENT_SERVICE_PATH;
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
* processAreaScope -
*
* Returns true if queryP/countQueryP were filled, false otherwise
*
* Most of the cases queryP and countQueryP are filled the same way. However, in the case of near georel they
* are different. This is due to:
*
* - we need to use $near in the query used to search (queryP), due to the alternative using $geoWithin
*   (described in https://stackoverflow.com/a/76416103/1485926) doesn't sort result by center proximity, so
*   ?orderBy=distance will break
* - we cannot use $near in the query used to count (countQueryP) as it doesn't work with modern count functions
*   (see the aforementioned link).
*
*/
bool processAreaScope(const Scope* scoP, orion::BSONObjBuilder* queryP, orion::BSONObjBuilder* countQueryP)
{
  // FIXME #3774: previously this part was based in streamming instead of append()

  std::string  keyLoc  = ENT_LOCATION "." ENT_LOCATION_COORDS;

  if (!mongoLocationCapable())
  {
    std::string details = std::string("location scope was found but your MongoDB version doesn't support it. ") +
      "Please upgrade MongoDB server to 2.4 or newer)";

    alarmMgr.badInput(clientIp, details);
    return false;
  }

  // Fill BSON corresponding to geometry
  orion::BSONObj geometry;
  if (scoP->areaType == orion::PointType)
  {
    orion::BSONArrayBuilder bab;
    bab.append(scoP->point.longitude());
    bab.append(scoP->point.latitude());
    orion::BSONObjBuilder bob;
    bob.append("type", "Point");
    bob.append("coordinates", bab.arr());
    geometry = bob.obj();
  }
  else if (scoP->areaType == orion::LineType)
  {
    // Arbitrary number of points
    orion::BSONArrayBuilder ps;

    for (unsigned int ix = 0; ix < scoP->line.pointList.size(); ++ix)
    {
      orion::Point* p = scoP->line.pointList[ix];
      orion::BSONArrayBuilder bab;
      bab.append(p->longitude());
      bab.append(p->latitude());
      ps.append(bab.arr());
    }
    orion::BSONObjBuilder bob;
    bob.append("type", "LineString");
    bob.append("coordinates", ps.arr());
    geometry = bob.obj();
  }
  else if (scoP->areaType == orion::BoxType)
  {
    orion::BSONArrayBuilder ps;

    orion::BSONArrayBuilder ba1, ba2, ba3, ba4, ba5;
    ba1.append(scoP->box.lowerLeft.longitude());
    ba1.append(scoP->box.lowerLeft.latitude());
    ps.append(ba1.arr());
    ba2.append(scoP->box.upperRight.longitude());
    ba2.append(scoP->box.lowerLeft.latitude());
    ps.append(ba2.arr());
    ba3.append(scoP->box.upperRight.longitude());
    ba3.append(scoP->box.upperRight.latitude());
    ps.append(ba3.arr());
    ba4.append(scoP->box.lowerLeft.longitude());
    ba4.append(scoP->box.upperRight.latitude());
    ps.append(ba4.arr());
    ba5.append(scoP->box.lowerLeft.longitude());
    ba5.append(scoP->box.lowerLeft.latitude());
    ps.append(ba5.arr());

    orion::BSONObjBuilder bob;
    orion::BSONArrayBuilder baCoords;
    baCoords.append(ps.arr());

    bob.append("type", "Polygon");
    bob.append("coordinates", baCoords.arr());
    geometry = bob.obj();
  }
  else if (scoP->areaType == orion::PolygonType)
  {
    // Arbitrary number of points
    orion::BSONArrayBuilder ps;

    for (unsigned int ix = 0; ix < scoP->polygon.vertexList.size(); ++ix)
    {
      orion::Point* p = scoP->polygon.vertexList[ix];

      orion::BSONArrayBuilder bab;
      bab.append(p->longitude());
      bab.append(p->latitude());

      ps.append(bab.arr());
    }

    orion::BSONObjBuilder bob;
    orion::BSONArrayBuilder baCoords;
    baCoords.append(ps.arr());

    bob.append("type", "Polygon");
    bob.append("coordinates", baCoords.arr());
    geometry = bob.obj();
  }
  else
  {
    LM_E(("Runtime Error (unknown area type: %d)", scoP->areaType));
    return false;
  }

  orion::BSONObjBuilder bobArea;
  if (scoP->georel.type == "near")
  {
    // 1. query used for count (without $near)

    // This works as far as we don't have any other $and field in the query, in which
    // case the array for $and here needs to be combined with other items. Currently,
    // users of processAreaScopesV2() doesn't do add any new $and clause.
    //
    // Note that and empty query $and: [ ] could not happen, as the parsing logic check that
    // at least minDistance or maxDistance are there. Check functional tests
    //
    // * 1177_geometry_and_coords/geometry_and_coords_errors.test, step 7
    // * 1677_geo_location_for_api_v2/geo_location_for_api_v2_error_cases.test, step 6

    orion::BSONArrayBuilder andArray;

    orion::BSONArrayBuilder coordsBuilder;
    coordsBuilder.append(scoP->point.longitude());
    coordsBuilder.append(scoP->point.latitude());
    orion::BSONArray coords = coordsBuilder.arr();

    if (scoP->georel.maxDistance >= 0)
    {
      orion::BSONObjBuilder andToken;

      orion::BSONObjBuilder geoWithinMax;
      orion::BSONArrayBuilder centerMax;
      centerMax.append(coords);
      centerMax.append(scoP->georel.maxDistance / EARTH_RADIUS_METERS);
      geoWithinMax.append("$centerSphere", centerMax.arr());

      orion::BSONObjBuilder loc;
      loc.append("$geoWithin", geoWithinMax.obj());

      andToken.append(keyLoc, loc.obj());

      andArray.append(andToken.obj());
    }

    if (scoP->georel.minDistance == 0)
    {
      // This is somehow a degenerate case, as any point in the space is
      // more than 0 distance than any other point :). In this case we only
      // check for existance of the location
      orion::BSONObjBuilder existTrue;
      existTrue.append("$exists", true);

      orion::BSONObjBuilder andToken;
      andToken.append(keyLoc, existTrue.obj());

      andArray.append(andToken.obj());
    }
    else if (scoP->georel.minDistance > 0)
    {
      orion::BSONObjBuilder andToken;

      orion::BSONObjBuilder geoWithinMin;
      orion::BSONArrayBuilder centerMin;
      centerMin.append(coords);
      centerMin.append(scoP->georel.minDistance / EARTH_RADIUS_METERS);
      geoWithinMin.append("$centerSphere", centerMin.arr());

      orion::BSONObjBuilder loc;
      loc.append("$geoWithin", geoWithinMin.obj());

      orion::BSONObjBuilder notx;
      notx.append("$not", loc.obj());

      andToken.append(keyLoc, notx.obj());

      andArray.append(andToken.obj());
    }

    countQueryP->append("$and", andArray.arr());

    // 2. Query used for count (with $near)

    orion::BSONObjBuilder near;

    near.append("$geometry", geometry);

    if (scoP->georel.maxDistance >= 0)
    {
      near.append("$maxDistance", scoP->georel.maxDistance);
    }

    if (scoP->georel.minDistance >= 0)
    {
      near.append("$minDistance", scoP->georel.minDistance);
    }

    bobArea.append("$near", near.obj());
    queryP->append(keyLoc, bobArea.obj());
  }
  else if (scoP->georel.type == "coveredBy")
  {
    orion::BSONObjBuilder bobGeom;
    bobGeom.append("$geometry", geometry);
    bobArea.append("$geoWithin", bobGeom.obj());

    queryP->append(keyLoc, bobArea.obj());
    countQueryP->append(keyLoc, bobArea.obj());
  }
  else if (scoP->georel.type == "intersects")
  {
    orion::BSONObjBuilder bobGeom;
    bobGeom.append("$geometry", geometry);
    bobArea.append("$geoIntersects", bobGeom.obj());

    queryP->append(keyLoc, bobArea.obj());
    countQueryP->append(keyLoc, bobArea.obj());
  }
  else if (scoP->georel.type == "disjoint")
  {
    orion::BSONObjBuilder bobGeom;
    orion::BSONObjBuilder bobNot;
    bobGeom.append("$geometry", geometry);
    bobNot.append("$geoIntersects", bobGeom.obj());
    bobArea.append("$exists", true);
    bobArea.append("$not", bobNot.obj());

    queryP->append(keyLoc, bobArea.obj());
    countQueryP->append(keyLoc, bobArea.obj());
  }
  else if (scoP->georel.type == "equals")
  {
    queryP->append(keyLoc, geometry);
    countQueryP->append(keyLoc, geometry);
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
* addIfNotPresentAttr (for numbers) -
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
* addIfNotPresentAttr (for strings) -
*
* If the attribute doesn't exist in the entity, then add it (shadowed, render will depend on filter)
*/
static void addIfNotPresentAttr
(
  Entity*             eP,
  const std::string&  name,
  const std::string&  type,
  const std::string&  value
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
* addIfNotPresentMetadata (double version) -
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

  Metadata* mdP = NULL;

  if (caP->previousValue == NULL)
  {
    // in this case previousValue is filled with caP value
    if (caP->compoundValueP == NULL)
    {
      switch (caP->valueType)
      {
      case orion::ValueTypeString:
        mdP = new Metadata(NGSI_MD_PREVIOUSVALUE, caP->type, caP->stringValue);
        break;

      case orion::ValueTypeBoolean:
        mdP = new Metadata(NGSI_MD_PREVIOUSVALUE, caP->type, caP->boolValue);
        break;

      case orion::ValueTypeNumber:
        mdP = new Metadata(NGSI_MD_PREVIOUSVALUE, caP->type, caP->numberValue);
        break;

      case orion::ValueTypeNull:
        mdP = new Metadata(NGSI_MD_PREVIOUSVALUE, caP->type, "");
        mdP->valueType = orion::ValueTypeNull;
        break;

      case orion::ValueTypeNotGiven:
        LM_E(("Runtime Error (value not given for metadata)"));
        return;

      default:
        LM_E(("Runtime Error (unknown value type: %d)", caP->valueType));
        return;
      }
    }
    else
    {
      mdP            = new Metadata(NGSI_MD_PREVIOUSVALUE, caP->type, "");
      mdP->valueType = caP->valueType;

      // Different with the case when previousValue is not NULL, we cannot
      // steal the value here, as the caP->compoundValueP has its own memory
      // independent memory lifecyel
      mdP->compoundValueP = caP->compoundValueP->clone();
    }
  }
  else
  {
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
* - alterationType
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
void addBuiltins(ContextElementResponse* cerP, const std::string& alterationType)
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

  // alterationType
  if (!alterationType.empty())
  {
    addIfNotPresentAttr(&cerP->entity, ALTERATION_TYPE, DEFAULT_ATTR_STRING_TYPE, alterationType);
  }

  // servicePath
  if (!cerP->entity.servicePath.empty())
  {
    addIfNotPresentAttr(&cerP->entity, SERVICE_PATH, DEFAULT_ATTR_STRING_TYPE, cerP->entity.servicePath);
  }

  for (unsigned int ix = 0; ix < cerP->entity.attributeVector.size(); ix++)
  {
    ContextAttribute* caP = cerP->entity.attributeVector[ix];

    // dateCreated metadata
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
    if (!caP->actionType.empty())
    {
      addIfNotPresentMetadata(caP, NGSI_MD_ACTIONTYPE, DEFAULT_ATTR_STRING_TYPE, caP->actionType);
    }

    // previousValue (except in entityCreate case)
    if (alterationType != "entityCreate")
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
* This method is used by queryContext. It takes a vector with entities and a vector
* with attributes as input and returns the corresponding ContextElementResponseVector or error.
*
*/
bool entitiesQuery
(
  const EntityIdVector&            enV,
  const StringList&                attrL,
  const ScopeVector&               spV,
  ContextElementResponseVector*    cerV,
  OrionError*                      oeP,
  const std::string&               tenant,
  const std::vector<std::string>&  servicePath,
  const std::vector<std::string>&  sortOrderV,
  int                              offset,
  int                              limit,
  bool*                            limitReached,
  long long*                       countP
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

  std::string err;

  orion::BSONObjBuilder    finalQuery;
  orion::BSONObjBuilder    finalCountQuery;
  orion::BSONArrayBuilder  orEnt;

  /* Part 1: entities - avoid $or in the case of a single element */
  if (enV.size() == 1)
  {
    orion::BSONObjBuilder bob;
    fillQueryEntity(&bob, enV[0]);
    orion::BSONObj entObj = bob.obj();
    finalQuery.appendElements(entObj);
    finalCountQuery.appendElements(entObj);

    LM_T(LmtMongo, ("Entity single query token: '%s'", entObj.toString().c_str()));
  }
  else
  {
    for (unsigned int ix = 0; ix < enV.size(); ++ix)
    {
      orion::BSONObjBuilder bob;
      fillQueryEntity(&bob, enV[ix]);
      orion::BSONObj entObj = bob.obj();
      orEnt.append(entObj);

      LM_T(LmtMongo, ("Entity query token: '%s'", entObj.toString().c_str()));
    }
    finalQuery.append("$or", orEnt.arr());
    finalCountQuery.append("$or", orEnt.arr());
  }

  /* Part 2: service path */
  if (servicePathFilterNeeded(servicePath))
  {
    finalQuery.appendElements(fillQueryServicePath("_id." ENT_SERVICE_PATH, servicePath));
    finalCountQuery.appendElements(fillQueryServicePath("_id." ENT_SERVICE_PATH, servicePath));
  }

  /* Part 3: attributes */
  orion::BSONArrayBuilder attrs;

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
    orion::BSONObjBuilder bob;
    bob.append("$in", attrs.arr());
    finalQuery.append(ENT_ATTRNAMES, bob.obj());
    finalCountQuery.append(ENT_ATTRNAMES, bob.obj());
  }

  /* Part 5: scopes */
  std::vector<orion::BSONObj>  filters;
  unsigned int                 geoScopes = 0;

  for (unsigned int ix = 0; ix < spV.size(); ++ix)
  {
    const Scope* scopeP = spV[ix];

    if (scopeP->type.find(SCOPE_FILTER) == 0)
    {
      addFilterScope(scopeP, &filters);
    }
    else if (scopeP->type == FIWARE_LOCATION_V2)
    {
      geoScopes++;
      if (geoScopes > 1)
      {
        alarmMgr.badInput(clientIp, "current version supports only one area scope, extra geoScope is ignored");
      }
      else
      {
        processAreaScope(scopeP, &finalQuery, &finalCountQuery);
      }
    }
    else if (scopeP->type == SCOPE_TYPE_SIMPLE_QUERY)
    {
      if (scopeP->stringFilterP)
      {
        for (unsigned int ix = 0; ix < scopeP->stringFilterP->mongoFilters.size(); ++ix)
        {
          finalQuery.appendElements(scopeP->stringFilterP->mongoFilters[ix]);
          finalCountQuery.appendElements(scopeP->stringFilterP->mongoFilters[ix]);
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
          finalCountQuery.appendElements(scopeP->mdStringFilterP->mongoFilters[ix]);
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
    finalCountQuery.appendElements(filters[ix]);
  }

  LM_T(LmtPagination, ("Offset: %d, Limit: %d, countP: %p", offset, limit, countP));

  /* Do the query on MongoDB */
  orion::DBCursor  cursor;

  orion::BSONObj query = finalQuery.obj();
  orion::BSONObj countQuery = finalCountQuery.obj();
  orion::BSONObj sort;

  if (sortOrderV.size() == 0)
  {
    orion::BSONObjBuilder bobSort;
    bobSort.append(ENT_CREATION_DATE, 1);
    sort = bobSort.obj();
  }
  else if ((sortOrderV[0] == ORDER_BY_PROXIMITY))
  {
    // In this case the solution is not setting any query.sort(), as the $near operator will do the
    // sorting itself. Of course, using orderBy=geo:distance without using georel=near will return
    // unexpected ordering, but this is already warned in the documentation.
  }
  else
  {
    orion::BSONObjBuilder     sortOrder;

    for (unsigned int ix = 0; ix < sortOrderV.size(); ix++)
    {
      std::string  sortToken;
      int          sortDirection;

      if (sortOrderV[ix][0] == '!')
      {
        // reverse
        sortToken     = sortOrderV[ix].substr(1);
        sortDirection = -1;
      }
      else
      {
        sortToken     = sortOrderV[ix];
        sortDirection = 1;
      }

      sortOrder.append(sortCriteria(sortToken), sortDirection);
    }

    sort = sortOrder.obj();
  }

  TIME_STAT_MONGO_READ_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();

  if (!orion::collectionRangedQuery(connection, composeDatabaseName(tenant), COL_ENTITIES, query, countQuery, sort, limit, offset, &cursor, countP, &err))
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    return false;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  unsigned int docs = 0;

  orion::BSONObj  r;
  int             errType = ON_NEXT_NO_ERROR;
  std::string     nextErr;

  /* Note limit != 0 will cause skipping the while loop in case request didn't actually ask for any result */
  while ((limit != 0) && (cursor.next(&r, &errType, &nextErr)))
  {
    alarmMgr.dbErrorReset();

    // Build CER from BSON retrieved from DB
    docs++;
    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, r.toString().c_str()));
    ContextElementResponse*  cer = new ContextElementResponse(r, attrL);

    // Add builtin attributes and metadata
    addBuiltins(cer, "");

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

    cerV->push_back(cer);
  }

  orion::releaseMongoConnection(connection);

  // Get check errType after the while on next(). When an error occurs, next()
  // returns false, as there aren't more results to process, and the program
  // flow directly ends here, to check errType

  if (errType == ON_NEXT_MANAGED_ERROR)
  {
    alarmMgr.dbError(nextErr);

    oeP->fill(SccReceiverInternalError, nextErr);
    return false;
  }
  else if (errType == ON_NEXT_UNMANAGED_ERROR)
  {
    LM_E(("Runtime Error (exception in next(): %s - query: %s)", nextErr.c_str(), query.toString().c_str()));
    oeP->fill(SccReceiverInternalError, nextErr);
    return false;
  }


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
    if (enV[ix]->idPattern.empty())
    {
      bool needToAdd = true;

      for (unsigned int jx = 0; jx < cerV->size(); ++jx)
      {
        if (((*cerV)[jx]->entity.entityId.id == enV[ix]->id) && ((*cerV)[jx]->entity.entityId.type == enV[ix]->type))
        {
          needToAdd = false;
          break;  /* jx */
        }
      }

      if (needToAdd)
      {
        ContextElementResponse* cerP = new ContextElementResponse();

        cerP->entity.entityId = enV[ix];

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

        cerP->error.fill(SccOk);

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
* We filter attributes with CPr not included in a list of attributes
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
  const StringList&                    attrsV,
  const ContextElementResponseVector&  oldCerV,
  ContextElementResponseVector*        newCerVP
)
{
  for (unsigned int ix = 0; ix < oldCerV.size(); ++ix)
  {
    ContextElementResponse* cerP    = oldCerV[ix];
    ContextElementResponse* newCerP = new ContextElementResponse();

    newCerP->entity.fill(cerP->entity.entityId,
                         cerP->entity.servicePath,
                         cerP->entity.creDate,
                         cerP->entity.modDate);

    // FIXME P10: not sure if this is the right way to do it, maybe we need a fill() method for this
    newCerP->entity.providerList = cerP->entity.providerList;
    newCerP->entity.providerRegIdList = cerP->entity.providerRegIdList;
    newCerP->error.fill(&cerP->error);

    bool pruneEntity = cerP->prune;

    for (unsigned int jx = 0; jx < cerP->entity.attributeVector.size(); ++jx)
    {
      ContextAttribute* caP = cerP->entity.attributeVector[jx];

      // To be included, it need to be found and one of the following:
      // - The attributes filter list is empty
      // - (Not empty attributes filter) The attribute is included in the filter list (taking into account wildcard)
      if ((caP->found) && ((attrsV.size() == 0) || (attrsV.lookup(caP->name, ALL_ATTRS))))
      {
        ContextAttribute* newCaP = new ContextAttribute(caP);
        newCerP->entity.attributeVector.push_back(newCaP);
      }
    }

    /* If after pruning the entity has no attribute and no CPr information, then it is not included
     * in the output vector, except if "prune" is set to false */
    if (pruneEntity &&
        (newCerP->entity.attributeVector.size() == 0) &&
        (newCerP->entity.providerList.size()    == 0))
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



/* ****************************************************************************
*
* registrationsQuery -
*/
bool registrationsQuery
(
  const EntityIdVector&               enV,
  const StringList&                   attrL,
  const ngsiv2::ForwardingMode        forwardingMode,
  std::vector<ngsiv2::Registration>*  regV,
  std::string*                        err,
  const std::string&                  tenant,
  const std::vector<std::string>&     servicePathV
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
  //          { cr.entities.id: ".*", cr.entities.isPattern: true (*), crs.entities.type: {$in: [T1, T3, ...} },
  //          { cr.entities.id: ".*", cr.entities.isPattern: true (*), crs.entities.type: {$exists: false } },
  //   ],
  //   cr.attrs.name : { $in: [ A1, ... ] },  (only if attrs > 0)
  //   servicePath: ... ,
  //   expiration: { $gt: ... },
  //   fwdMode: ...                           (only when forwardingMode is update or query)
  // }
  //
  // Note that by construction the $or array always has at least two elements (the two ones corresponding to the
  // universal pattern) so we cannot avoid to use this operator.
  //
  // (*) Not actually this way. See comment in the code.
  //

  /* Build query based on arguments */
  std::string       crEntitiesId      = REG_CONTEXT_REGISTRATION "." REG_ENTITIES "." REG_ENTITY_ID;
  std::string       crEntitiesType    = REG_CONTEXT_REGISTRATION "." REG_ENTITIES "." REG_ENTITY_TYPE;
  std::string       crEntitiesPattern = REG_CONTEXT_REGISTRATION "." REG_ENTITIES "." REG_ENTITY_ISPATTERN;
  std::string       crAttrsNames      = REG_CONTEXT_REGISTRATION "." REG_ATTRS    "." REG_ATTRS_NAME;
  orion::BSONArrayBuilder  entityOr;
  orion::BSONArrayBuilder  types;

  for (unsigned int ix = 0; ix < enV.size(); ++ix)
  {
    const EntityId* en = enV[ix];
    orion::BSONObjBuilder b;

    if (!en->idPattern.empty())
    {
      b.appendRegex(crEntitiesId, en->idPattern);
    }
    else  /* not a pattern */
    {
      b.append(crEntitiesId, en->id);
    }

    if (!en->type.empty())
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
  orion::BSONObjBuilder bobArrayItem1;
  orion::BSONObjBuilder bobArrayItem2;

  orion::BSONObjBuilder bobIn;
  bobIn.append("$in", types.arr());

  orion::BSONObjBuilder bobExistsFalse;
  bobExistsFalse.append("$exists", false);

  // Note that previous to Orion 3.4.0 csub were using strings for this, so need to use
  // {$in: [true, "true"]} instead of just true
  orion::BSONArrayBuilder bInArr;
  orion::BSONObjBuilder bin;
  bInArr.append(true);
  bInArr.append("true");
  bin.append("$in", bInArr.arr());
  orion::BSONObj binObj = bin.obj();

  bobArrayItem1.append(crEntitiesId, ".*");
  bobArrayItem1.append(crEntitiesPattern, binObj);
  bobArrayItem1.append(crEntitiesType, bobIn.obj());

  bobArrayItem2.append(crEntitiesId, ".*");
  bobArrayItem2.append(crEntitiesPattern, binObj);
  bobArrayItem2.append(crEntitiesType, bobExistsFalse.obj());

  entityOr.append(bobArrayItem1.obj());
  entityOr.append(bobArrayItem2.obj());

  orion::BSONArrayBuilder attrs;

  for (unsigned int ix = 0; ix < attrL.size(); ++ix)
  {
    std::string attrName = attrL[ix];

    attrs.append(attrName);
    LM_T(LmtMongo, ("Attribute discovery: '%s'", attrName.c_str()));
  }

  orion::BSONObjBuilder queryBuilder;

  orion::BSONObjBuilder bobGtCurrentTime;
  bobGtCurrentTime.append("$gt", (long long) getCurrentTime());

  queryBuilder.append("$or", entityOr.arr());
  queryBuilder.append(REG_EXPIRATION, bobGtCurrentTime.obj());

  if (attrs.arrSize() > 0)
  {
    /* If we don't do this check, the {$in: [] } of the attribute name part makes the query fail */
    orion::BSONObjBuilder bobIn;
    bobIn.append("$in", attrs.arr());
    queryBuilder.append(crAttrsNames, bobIn.obj());
  }

  //
  // 'And-in' the service path
  //
  if (servicePathFilterNeeded(servicePathV))
  {
    queryBuilder.appendElements(fillQueryServicePath(REG_SERVICE_PATH, servicePathV));
  }

  // Forwarding mode filter (only when forwardingMode is update or query)
  // Note that "all" and null (omission of the field) is always included in the filter
  if ((forwardingMode == ngsiv2::ForwardQuery) || (forwardingMode == ngsiv2::ForwardUpdate))
  {
    orion::BSONObjBuilder    bob;
    orion::BSONArrayBuilder  bab;
    if (forwardingMode == ngsiv2::ForwardUpdate)
    {
      bab.append("update");
    }
    else
    {
      // forwardingMode == ngsiv2::ForwardQuery
      bab.append("query");
    }
    bab.append("all");
    bab.appendNull();
    bob.append("$in", bab.arr());
    queryBuilder.append(REG_FORWARDING_MODE, bob.obj());
  }

  //
  // Do the query in MongoDB
  // FIXME P2: Use field selector to include the only relevant field:
  //           contextRegistration array (e.g. "expiration" is not needed)
  //
  orion::DBCursor cursor;

  orion::BSONObj query = queryBuilder.obj();

  TIME_STAT_MONGO_READ_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();

  orion::BSONObjBuilder bobSort;
  bobSort.append("_id", 1);

  if (!orion::collectionRangedQuery(connection, composeDatabaseName(tenant), COL_REGISTRATIONS, query, query, bobSort.obj(), 0, 0, &cursor, NULL, err))
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    return false;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  /* Process query result */
  unsigned int docs = 0;

  orion::BSONObj r;
  while (cursor.next(&r))
  {
    docs++;

    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, r.toString().c_str()));

    ngsiv2::Registration reg;
    if (reg.fromBson(r))
    {
      regV->push_back(reg);
    }
    else
    {
      // FIXME #4611: this else branch will be no longer needed after fixing the issue
      LM_E(("Runtime Error (registrations with more than one CR are considered runtime errors since Orion 4.1.0, please fix reg %s at DB)", reg.id.c_str()));
    }

    /* FIXME: note that given the response doesn't distinguish from which registration ID the
     * response comes, it could have that we have same context registration elements, belong to different
     * registrations ID at DB level, thus causing a duplicated context element response. Moreover,
     * NGSI doesn't forbid to registry exactly twice the same context registration element in the
     * same registration ID. Thus, it could be interesting to post-process the response vector, to
     * "compact" removing duplicated responses.*/
  }
  orion::releaseMongoConnection(connection);

  return true;
}



/* ****************************************************************************
*
* condValueAttrMatch -
*/
bool condValueAttrMatch(const orion::BSONObj& sub, const std::vector<std::string>& modifiedAttrs)
{
  std::vector<orion::BSONElement>  conds = getFieldF(sub, CSUB_CONDITIONS).Array();

  // If the list of attributes to check is empty, then no match
  if (modifiedAttrs.size() == 0)
  {
    return false;
  }

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
* getCommonAttributes -
*/
static void getCommonAttributes
(
  const std::vector<std::string>&   fVector,
  const std::vector<std::string>&   sVector,
  std::vector<std::string>&         resultVector
)
{
  for (unsigned int avOc = 0; avOc < sVector.size(); ++avOc)
  {
    // some builtin attributes are always include (even when onlyChangedAttrs is true)
    if ((sVector[avOc] == ALTERATION_TYPE) || (sVector[avOc] == DATE_CREATED) || (sVector[avOc] == DATE_MODIFIED))
    {
      resultVector.push_back(sVector[avOc]);
    }
    else 
    {
      for (unsigned int cavOc = 0; cavOc < fVector.size(); ++cavOc)
      {      
        if (fVector[cavOc] == sVector[avOc])
        {
          resultVector.push_back(sVector[avOc]);
          break;          
        }
      }
    }
  }
}


/* ****************************************************************************
*
* getDifferenceAttributes -
*/
static void getDifferenceAttributes
(
  const std::vector<std::string>&   fVector,
  const std::vector<std::string>&   sVector,
  std::vector<std::string>&         resultVector
)
{
  for (unsigned int cavOc = 0; cavOc < fVector.size(); ++cavOc)
  {
    bool found = false;
    for (unsigned int avOc = 0; avOc < sVector.size(); ++avOc)
    {
      if (fVector[cavOc] == sVector[avOc])
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      resultVector.push_back(fVector[cavOc]);
    }
  }
}


/* ****************************************************************************
*
* subToNotifyList -
*
*/
void subToNotifyList
(
  const std::vector<std::string>&  notificationVector,
  const std::vector<std::string>&  entityAttrsVector,
  StringList&                      attrL,
  const bool&                      blacklist
)
{
    std::vector<std::string>  notifyAttrs;

    if (blacklist)
    {
      // By definition, notificationVector cannot be empty in blacklist case
      // (checked at parsing time)
      getDifferenceAttributes(entityAttrsVector, notificationVector, notifyAttrs);
      attrL.fill(notifyAttrs);
    }
    else
    {
      if (notificationVector.size() == 0)
      {
        // This means "all attributes"
        attrL.fill(entityAttrsVector);
      }
      else
      {
        getCommonAttributes(entityAttrsVector, notificationVector, notifyAttrs);
        attrL.fill(notifyAttrs);
      }
    }
}



/* ****************************************************************************
*
* subToAttributeList -
*
* Extract the attribute list from a BSON document (in the format of the csubs
* collection)
*/
StringList subToAttributeList
(
  const orion::BSONObj&           sub,
  const bool&                     blacklist,
  const std::vector<std::string>  attributes
)
{
  StringList                       attrL;
  std::vector<orion::BSONElement>  subAttrs = getFieldF(sub, CSUB_ATTRS).Array();
  std::vector<std::string>         notificationAttrs;
  for (unsigned int ix = 0; ix < subAttrs.size() ; ++ix)
  {
    std::string subAttr = subAttrs[ix].String();
    notificationAttrs.push_back(subAttr);
  }
  subToNotifyList(notificationAttrs, attributes, attrL, blacklist);
  return attrL;
}



/* ****************************************************************************
*
* subToAttributeList -
*
* Extract the attribute list from a BSON document (in the format of the csubs
* collection)
*/
StringList subToAttributeList(const orion::BSONObj& sub)
{
  StringList                       attrL;
  std::vector<orion::BSONElement>  subAttrs = getFieldF(sub, CSUB_ATTRS).Array();

  for (unsigned int ix = 0; ix < subAttrs.size() ; ++ix)
  {
    std::string subAttr = subAttrs[ix].String();

    attrL.push_back(subAttr);
  }

  return attrL;
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
* Looks in the CER passed as argument, searching for a suitable CPr in the Registrations
* vector passed as argument. If a suitable CPr is found, it is set in the CER (and the 'found' field
* is changed to true)
*/
void fillContextProviders(ContextElementResponse* cer, const std::vector<ngsiv2::Registration>& regV)
{
  for (unsigned int ix = 0; ix < cer->entity.attributeVector.size(); ++ix)
  {
    ContextAttribute* ca = cer->entity.attributeVector[ix];

    if (ca->found)
    {
      continue;
    }

    /* Search for some CPr in Registrations vector */
    std::string     perEntProvider;
    std::string     perAttrProvider;
    bool            legacyProviderFormat;
    std::string     regId;

    cprLookupByAttribute(cer->entity,
                         ca->name,
                         regV,
                         &perEntProvider,
                         &perAttrProvider,
                         &legacyProviderFormat,
                         &regId);

    /* Looking results after Registrations vector processing */
    ca->provider.http.url = perAttrProvider.empty() ? perEntProvider : perAttrProvider;
    ca->provider.legacyForwardingMode = legacyProviderFormat;
    ca->providerRegId = regId;

    ca->found = (!ca->provider.http.url.empty());
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
  const std::vector<ngsiv2::Registration>&  regV,
  std::string*                              perEntProviderP,
  std::string*                              perAttrProviderP,
  bool*                                     legacyPproviderFormatP,
  std::string*                              regId
)
{
  *perEntProviderP  = "";
  *perAttrProviderP = "";
  *regId = "";

  for (unsigned int regIx = 0; regIx < regV.size(); ++regIx)
  {
    ngsiv2::Registration reg = regV[regIx];

    /* Is there a matching entity in the Registration? */
    for (unsigned enIx = 0; enIx < reg.dataProvided.entities.size(); ++enIx)
    {
      EntityId regEn = reg.dataProvided.entities[enIx];

      if (regEn.idPattern == ".*")
      {
        // By the moment the only supported pattern is .*. In this case matching is
        // based exclusively in type
        if (regEn.type != en.entityId.type && !regEn.type.empty())
        {
          /* No match (keep searching the Registration) */
          continue;
        }
      }
      else if (regEn.id != en.entityId.id || (regEn.type != en.entityId.type && !regEn.type.empty()))
      {
        /* No match (keep searching the Registration) */
        continue;
      }

      /* Registration without attributes (keep searching in other Registration) */
      if (reg.dataProvided.attributes.size() == 0)
      {
        *perEntProviderP         = reg.provider.http.url;
        *legacyPproviderFormatP  = reg.provider.legacyForwardingMode;
        *regId                   = reg.id;

        break;  /* enIx */
      }

      /* Is there a matching entity or the absence of attributes? */
      for (unsigned attrIx = 0; attrIx < reg.dataProvided.attributes.size(); ++attrIx)
      {
        std::string regAttrName = reg.dataProvided.attributes[attrIx];
        if (regAttrName == attrName)
        {
          /* We cannot "improve" this result by keep searching the Registrations vector, so we return */
          *perAttrProviderP        = reg.provider.http.url;
          *legacyPproviderFormatP  = reg.provider.legacyForwardingMode;
          *regId                   = reg.id;

          return;
        }
      }
    }
  }
}
