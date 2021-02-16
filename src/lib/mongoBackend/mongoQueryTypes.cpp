/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan Marquez
*/
#include <string>
#include <vector>
#include <map>
#include <set>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/sem.h"
#include "common/statistics.h"
#include "alarmMgr/alarmMgr.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/mongoQueryTypes.h"

#include "mongoDriver/safeMongo.h"
#include "mongoDriver/connectionOperations.h"
#include "mongoDriver/mongoConnectionPool.h"
#include "mongoDriver/BSONObjBuilder.h"
#include "mongoDriver/BSONArrayBuilder.h"


// FIXME #3774: previously this file done a heavy use of BSON streamming instead of append() to
// build aggregation stages


/* ****************************************************************************
*
* BATCH_SIZE constant
*
*/
#define BATCH_SIZE  1000


/* ****************************************************************************
*
* getAttributeTypes -
*
*/
static void getAttributeTypes
(
  const std::string&               tenant,
  const std::vector<std::string>&  servicePathV,
  const std::string&               entityType,
  const std::string&               attrName,
  std::vector<std::string>*        attrTypes
)
{
  orion::BSONObjBuilder bob;

  if (entityType.empty())
  {
    std::string idType = std::string("_id.") + ENT_ENTITY_TYPE;

    orion::BSONObjBuilder bobType1;
    bobType1.append(idType, entityType);

    orion::BSONObjBuilder bobExists;
    bobExists.append("$exists", false);
    orion::BSONObjBuilder bobType2;
    bobType2.append(idType, bobExists.obj());

    orion::BSONArrayBuilder baOr;
    baOr.append(bobType1.obj());
    baOr.append(bobType2.obj());

    bob.append("$or", baOr.arr());
  }
  if (servicePathFilterNeeded(servicePathV))
  {
    bob.appendElements(fillQueryServicePath("_id." ENT_SERVICE_PATH, servicePathV));
  }
  bob.append(ENT_ATTRNAMES, attrName);

  orion::BSONObj query = bob.obj();

  orion::DBCursor  cursor;
  std::string      err;

  TIME_STAT_MONGO_COMMAND_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();

  if (!orion::collectionQuery(connection, composeDatabaseName(tenant), COL_ENTITIES, query, &cursor, &err))
  {
    orion::releaseMongoConnection(connection);
    TIME_STAT_MONGO_COMMAND_WAIT_STOP();
    return;
  }
  TIME_STAT_MONGO_COMMAND_WAIT_STOP();

  unsigned int  docs = 0;

  orion::BSONObj r;
  while (cursor.next(&r))
  {
    docs++;
    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, r.toString().c_str()));

    orion::BSONObj attrs = getObjectFieldF(r, ENT_ATTRS);
    orion::BSONObj attr  = getObjectFieldF(attrs, attrName);
    attrTypes->push_back(getStringFieldF(attr, ENT_ATTRS_TYPE));
  }

  orion::releaseMongoConnection(connection);
}



/* ****************************************************************************
*
* countEntities -
*
*/
static long long countEntities
(
  const std::string&               tenant,
  const std::vector<std::string>&  servicePathV,
  const std::string&               entityType
)
{
  orion::BSONObjBuilder bob;

  bob.append("_id." ENT_ENTITY_TYPE, entityType);
  if (servicePathFilterNeeded(servicePathV))
  {
    bob.appendElements(fillQueryServicePath("_id." ENT_SERVICE_PATH, servicePathV));
  }

  std::string         err;
  unsigned long long  c;

  if (!orion::collectionCount(composeDatabaseName(tenant), COL_ENTITIES, bob.obj(), &c, &err))
  {
    return -1;
  }

  return c;
}



/* ****************************************************************************
*
* projectStage -
*
* This function adds to the pipeline array this token:
*
* {$project: {_id.type: {$ifNull: ["$_id.type", null]}, "attrNames": 1 } }
*
*/
static void projectStage(orion::BSONArrayBuilder* pipeline)
{
  orion::BSONArrayBuilder baIfNull;
  baIfNull.append(CS_ID_ENTITY);
  baIfNull.appendNull();

  orion::BSONObjBuilder bobIfNull;
  bobIfNull.append("$ifNull", baIfNull.arr());

  orion::BSONObjBuilder bobProjectContent;
  bobProjectContent.append(C_ID_ENTITY, bobIfNull.obj());
  bobProjectContent.append(ENT_ATTRNAMES, 1);

  orion::BSONObjBuilder bobProject;
  bobProject.append("$project", bobProjectContent.obj());
  orion::BSONObj project = bobProject.obj();

  pipeline->append(project);
}



/* ****************************************************************************
*
* groupStage -
*
* This function adds to the pipeline array this token:
*
* {$group: {_id: {$cond: [{$in: [$_id.type, [null, ""]]}, "", "$_id.type"]},
*                                   attrs: {$addToSet: "$attrNames"}}
*
*/
static void groupStage(orion::BSONArrayBuilder* pipeline)
{
  // sub-element: $cond
  orion::BSONArrayBuilder innerArray;
  innerArray.appendNull();
  innerArray.append("");

  orion::BSONArrayBuilder inArray;
  inArray.append(CS_ID_ENTITY);
  inArray.append(innerArray.arr());

  orion::BSONObjBuilder inObj;
  inObj.append("$in", inArray.arr());

  orion::BSONArrayBuilder condArray;
  condArray.append(inObj.obj());
  condArray.append("");
  condArray.append(CS_ID_ENTITY);

  orion::BSONObjBuilder condObj;
  condObj.append("$cond", condArray.arr());

  // $group itself
  orion::BSONObjBuilder atsObj;
  atsObj.append("$addToSet", S_ATTRNAMES);

  orion::BSONObjBuilder groupObj;
  groupObj.append("_id", condObj.obj());
  groupObj.append("attrs", atsObj.obj());

  orion::BSONObjBuilder group;
  group.append("$group", groupObj.obj());

  pipeline->append(group.obj());
}



/* ****************************************************************************
*
* sortStage -
*
* This function adds to the pipeline array this token:
*
* {$sort: {_id: 1} }
*
*/
static void sortStage(orion::BSONArrayBuilder* pipeline)
{
  orion::BSONObjBuilder idObj;
  idObj.append("_id", 1);
  orion::BSONObjBuilder sortObj;
  sortObj.append("$sort", idObj.obj());

  pipeline->append(sortObj.obj());
}



/* ****************************************************************************
*
* mongoEntityTypesValues -
*
* "Simplified" version of mongoEntityTypes(), using a simpler aggregation command
* and the processing logic afterwards. Note that apiVersion is not included in this
* operation as it can be used only in NGSIv2
*
*/
HttpStatusCode mongoEntityTypesValues
(
  EntityTypeVectorResponse*            responseP,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV,
  std::map<std::string, std::string>&  uriParams,
  unsigned int*                        totalTypesP
)
{
  unsigned int   offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
  unsigned int   limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());
  bool           reqSemTaken    = false;

  LM_T(LmtMongo, ("Query Entity Types"));
  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Count: %s", offset, limit, (totalTypesP != NULL)? "true" : "false"));

  reqSemTake(__FUNCTION__, "query types request", SemReadOp, &reqSemTaken);

  /* Compose query based on this aggregation command:
   *
   * db.entities.aggregate([{$match: { "_id.servicePath": /.../ } },
   *                (1)      {$project: {_id.type: {$ifNull: ["$_id.type", null]}, "attrNames": 1 } },
   *                (2)      {$group: {_id: {$cond: [{$in: [$_id.type, [null, ""]]}, "", "$_id.type"]} },
   *                         {$sort: {_id: 1} }
   *                       ])
   *
   * (1) The $ifNull causes that entities without _id.type have _id.type: null (used by (2))
   *
   * (2) The $cond used in this $group is to group together entities with empty type ("") and null type
   *
   * Previous version of this code (see for instance Orion 2.5.0) use also skip and limit stages.
   * Current code follows the original approach in Orion 2.0.0 and before. The skip and limit stages
   * were introduced in Orion 2.1.0 due to limitations in legacy driver with MongodDB >=3.6.
   * Current approach needs a cursor with all the data (drawdback) but avoids the need of a separate
   * aggregation operation to count, so is simpler and more precise (advantage).
   *
   */

  orion::BSONArrayBuilder pipeline;

  // $match
  if (servicePathFilterNeeded(servicePathV))
  {
    orion::BSONObj spQuery = fillQueryServicePath(C_ID_SERVICEPATH, servicePathV);

    orion::BSONObjBuilder bobMatch;
    bobMatch.append("$match", spQuery);
    orion::BSONObj match = bobMatch.obj();

    pipeline.append(match);
  }

  // $project
  projectStage(&pipeline);

  // $cond
  orion::BSONArrayBuilder baNullEmpty;
  baNullEmpty.appendNull();
  baNullEmpty.append("");

  orion::BSONArrayBuilder baIn;
  baIn.append(CS_ID_ENTITY);
  baIn.append(baNullEmpty.arr());

  orion::BSONObjBuilder bobIn;
  bobIn.append("$in", baIn.arr());

  orion::BSONArrayBuilder baCond;
  baCond.append(bobIn.obj());
  baCond.append("");
  baCond.append(CS_ID_ENTITY);

  orion::BSONObjBuilder bobCond;
  bobCond.append("$cond", baCond.arr());

  // $group
  orion::BSONObjBuilder bobGroupContent;
  bobGroupContent.append("_id", bobCond.obj());

  orion::BSONObjBuilder bobGroup;
  bobGroup.append("$group", bobGroupContent.obj());
  orion::BSONObj group = bobGroup.obj();

  pipeline.append(group);

  // $sort
  sortStage(&pipeline);

  std::string err;

  orion::DBCursor  cursor;

  TIME_STAT_MONGO_COMMAND_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();

  if (!orion::collectionAggregate(connection, composeDatabaseName(tenant), COL_ENTITIES, pipeline.arr(), BATCH_SIZE, &cursor, &err))
  {
    TIME_STAT_MONGO_COMMAND_WAIT_STOP();
    orion::releaseMongoConnection(connection);
    responseP->statusCode.fill(SccReceiverInternalError, err);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }
  TIME_STAT_MONGO_COMMAND_WAIT_STOP();

  // Processing result to build response
  orion::BSONObj resultItem;
  unsigned int docs = 0;
  while (cursor.next(&resultItem))
  {
    if ((docs < offset) || (docs > offset + limit - 1))
    {
      docs++;
      continue;
    }

    docs++;
    LM_T(LmtMongo, ("aggregation result [%d]: %s", docs, resultItem.toString().c_str()));

    std::string     type;

    if (getFieldF(resultItem, "_id").isNull())
    {
      type = "";
    }
    else
    {
      type = getStringFieldF(resultItem, "_id");
    }

    responseP->entityTypeVector.push_back(new EntityType(type));
  }
  orion::releaseMongoConnection(connection);

  // Get count if user requested (i.e. if totalTypesP is not NULL)
  if (totalTypesP != NULL)
  {
    *totalTypesP = docs;
  }

  responseP->statusCode.fill(SccOk);
  reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

  return SccOk;
}



/* ****************************************************************************
*
* mongoEntityTypes -
*
*/
HttpStatusCode mongoEntityTypes
(
  EntityTypeVectorResponse*            responseP,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV,
  std::map<std::string, std::string>&  uriParams,
  ApiVersion                           apiVersion,
  unsigned int*                        totalTypesP,
  bool                                 noAttrDetail
)
{
  unsigned int   offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
  unsigned int   limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());
  bool           reqSemTaken    = false;

  LM_T(LmtMongo, ("Query Entity Types"));
  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Count: %s", offset, limit, (totalTypesP != NULL)? "true" : "false"));

  reqSemTake(__FUNCTION__, "query types request", SemReadOp, &reqSemTaken);

  /* Compose query based on this aggregation command:
   *
   * db.entities.aggregate([ {$match: { "_id.servicePath": /.../ } },
   *                (1)      {$project: {_id.type: {$ifNull: ["$_id.type", null]}, "attrNames": 1 } },
   *                         {$project: { "attrNames"
   *                (2)            {$cond: [ {$eq: [ "$attrNames", [ ] ] }, [null], "$attrNames"] }
   *                            }
   *                         },
   *                         {$unwind: "$attrNames"},
   *                (3)      {$group: {_id: {$cond: [{$in: [$_id.type, [null, ""]]}, "", "$_id.type"]},
   *                                   attrs: {$addToSet: "$attrNames"}},
   *                         {$sort: {_id: 1} }
   *                       ])
   *
   * (1) The $ifNull causes that entities without _id.type have _id.type: null (used by (3))
   *
   * (2) This $cond part is hard... more information at
   *     http://stackoverflow.com/questions/27510143/empty-array-prevents-document-to-appear-in-query
   *     As a consequence, some "null" values may appear in the resulting attrs vector,
   *     which are pruned by the result processing logic.
   *
   * (3) The $cond used in this $group is to group together entities with empty type ("") and null type
   *
   * FIXME P6: in the future, we can interpret the collapse parameter at this layer.
   *           If collapse=true so we don't need attributes, the
   *           following command can be used:
   *
   *           db.entities.aggregate({[ {$group: {_id: "$_id.type"} }]})
   *
   * Previous version of this code (see for instance Orion 2.5.0) use also skip and limit stages.
   * Current code follows the original approach in Orion 2.0.0 and before. The skip and limit stages
   * were introduced in Orion 2.1.0 due to limitations in legacy driver with MongodDB >=3.6.
   * Current approach needs a cursor with all the data (drawdback) but avoids the need of a separate
   * aggregation operation to count, so is simpler and more precise (advantage).
   *
   */

  orion::BSONArrayBuilder pipeline;

  // $match
  if (servicePathFilterNeeded(servicePathV))
  {
    orion::BSONObj spQuery = fillQueryServicePath(C_ID_SERVICEPATH, servicePathV);
    orion::BSONObjBuilder bobMatch;
    bobMatch.append("$match", spQuery);
    orion::BSONObj match = bobMatch.obj();

    pipeline.append(match);
  }

  // $project (first)
  projectStage(&pipeline);

  // $project (second)
  //
  // Building the projection part of the query that includes types that have no attributes
  // See bug: https://github.com/telefonicaid/fiware-orion/issues/686

  // FIXME P3. We are using the $cond: [ .. ] and not the $cond: { .. } one, due to the former was
  // the only one valid in MongoDB 2.4. However, MongoDB 2.4 support was removed time ago, so we could
  // change the syntax

  orion::BSONArrayBuilder emptyArray;

  orion::BSONArrayBuilder innerArray;
  innerArray.append(S_ATTRNAMES);
  innerArray.append(emptyArray.arr());

  orion::BSONObjBuilder eqObj;
  eqObj.append("$eq", innerArray.arr());

  orion::BSONArrayBuilder nullArray;
  nullArray.appendNull();

  orion::BSONArrayBuilder condArray;
  condArray.append(eqObj.obj());
  condArray.append(nullArray.arr());
  condArray.append(S_ATTRNAMES);

  orion::BSONObjBuilder condObj;
  condObj.append("$cond", condArray.arr());

  orion::BSONObjBuilder attrNamesObj;
  attrNamesObj.append(ENT_ATTRNAMES, condObj.obj());

  orion::BSONObjBuilder projectObj;
  projectObj.append("$project", attrNamesObj.obj());

  pipeline.append(projectObj.obj());


  // $unwind
  orion::BSONObjBuilder bobUnwind;
  bobUnwind.append("$unwind", S_ATTRNAMES);
  orion::BSONObj unwind = bobUnwind.obj();
  pipeline.append(unwind);

  // $group
  groupStage(&pipeline);

  // $sort
  sortStage(&pipeline);

  std::string err;

  orion::DBCursor  cursor;

  TIME_STAT_MONGO_COMMAND_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();

  if (!orion::collectionAggregate(connection, composeDatabaseName(tenant), COL_ENTITIES, pipeline.arr(), BATCH_SIZE, &cursor, &err))
  {
    TIME_STAT_MONGO_COMMAND_WAIT_STOP();
    orion::releaseMongoConnection(connection);
    responseP->statusCode.fill(SccReceiverInternalError, err);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }
  TIME_STAT_MONGO_COMMAND_WAIT_STOP();

  // Processing result to build response
  orion::BSONObj resultItem;
  unsigned int docs = 0;
  while (cursor.next(&resultItem))
  {
    if ((docs < offset) || (docs > offset + limit - 1))
    {
      docs++;
      continue;
    }

    docs++;
    LM_T(LmtMongo, ("aggregation result [%d]: %s", docs, resultItem.toString().c_str()));

    std::vector<orion::BSONElement>  attrsArray  = getFieldF(resultItem, "attrs").Array();
    EntityType*                      entityType;

    entityType = new EntityType(getStringFieldF(resultItem, "_id"));

    entityType->count = countEntities(tenant, servicePathV, entityType->type);

    if (!attrsArray[0].isNull())
    {
      for (unsigned int jx = 0; jx < attrsArray.size(); ++jx)
      {
        /* This is where NULL-elements in the resulting attrs vector are pruned */
        if (attrsArray[jx].isNull())
        {
          continue;
        }

        /* Note that we need and extra query() in the database (inside attributeType() function) to get each attribute type.
         * This could be inefficient, especially if the number of attributes is large */
        if (!noAttrDetail)
        {
          std::vector<std::string> attrTypes;

          getAttributeTypes(tenant, servicePathV, entityType->type , attrsArray[jx].String(), &attrTypes);

          for (unsigned int kx = 0; kx < attrTypes.size(); ++kx)
          {
            ContextAttribute* ca = new ContextAttribute(attrsArray[jx].String(), attrTypes[kx], "");

            entityType->contextAttributeVector.push_back(ca);

            // For backward compability, NGSIv1 only accepts one element
            if (apiVersion == V1)
            {
              break;
            }
          }
        }
        else
        {
          //
          // NOTE: here we add a ContextAttribute with empty type, as a marker for
          //       this special condition of 'No Attribute Detail'
          //
          ContextAttribute* caP = new ContextAttribute(attrsArray[jx].String(), "", "");
          entityType->contextAttributeVector.push_back(caP);
        }
      }
    }
    responseP->entityTypeVector.push_back(entityType);
  }
  orion::releaseMongoConnection(connection);

  // Get count if user requested (i.e. if totalTypesP is not NULL)
  // Special processing if result array is 0
  if (docs == 0)
  {
    if (totalTypesP != NULL)
    {
      *totalTypesP = docs;
      char detailsMsg[256];
      snprintf(detailsMsg, sizeof(detailsMsg), "Number of types: %u. Offset is %u", *totalTypesP, offset);
      responseP->statusCode.fill(SccContextElementNotFound, detailsMsg);
    }
    else
    {
      responseP->statusCode.fill(SccContextElementNotFound);
    }
  }
  else
  {
    if (totalTypesP != NULL)
    {
      *totalTypesP = docs;
      char detailsMsg[256];
      snprintf(detailsMsg, sizeof(detailsMsg), "Count: %u", *totalTypesP);
      responseP->statusCode.fill(SccOk, detailsMsg);
    }
    else
    {
      responseP->statusCode.fill(SccOk);
    }
  }

  reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

  return SccOk;
}



/* ****************************************************************************
*
* mongoAttributesForEntityType -
*
*/
HttpStatusCode mongoAttributesForEntityType
(
  const std::string&                    entityType,
  EntityTypeResponse*                   responseP,
  const std::string&                    tenant,
  const std::vector<std::string>&       servicePathV,
  std::map<std::string, std::string>&   uriParams,
  bool                                  noAttrDetail,
  ApiVersion                            apiVersion
)
{
  unsigned int   offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
  unsigned int   limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());
  bool           reqSemTaken    = false;
  bool           count          = false;

  // Count only makes sense for this operation in the case of NGSIv1
  if (apiVersion == V1)
  {
    std::string  detailsString  = uriParams[URI_PARAM_PAGINATION_DETAILS];

    count = (strcasecmp("on", detailsString.c_str()) == 0)? true : false;
  }

  // Setting the name of the entity type for the response
  responseP->entityType.type = entityType;

  LM_T(LmtMongo, ("Query Types Attribute for <%s>", entityType.c_str()));
  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Count: %s", offset, limit, (count == true)? "true" : "false"));

  reqSemTake(__FUNCTION__, "query types attributes request", SemReadOp, &reqSemTaken);


  /* Compose query based on this aggregation command:
   *
   * db.entities.aggreagte([ {$match: { "_id.type": "TYPE" , "_id.servicePath": /.../ } },
   *                (1)      {$project: {_id.type: {$ifNull: ["$_id.type", null]}, "attrNames": 1 } },
   *                         {$unwind: "$attrNames"},
   *                (2)      {$group: {_id: {$cond: [{$in: [$_id.type, [null, ""]]}, "", "$_id.type"]},
   *                                   attrs: {$addToSet: "$attrNames"}},
   *                         {$unwind: "$attrs"},
   *                         {$group: {_id: "$attrs" }},
   *                         {$sort: {_id: 1}}
   *                       ])
   *
   * (1) The $ifNull causes that entities without _id.type have _id.type: null (used by (2))
   *
   * (2) The $cond used in this $group is to group together entities with empty type ("") and null type
   *
   */

  orion::BSONArrayBuilder pipeline;

  // $match
  orion::BSONObjBuilder bobMatchContent;

  bobMatchContent.append(C_ID_ENTITY, entityType);
  if (servicePathFilterNeeded(servicePathV))
  {
    bobMatchContent.appendElements(fillQueryServicePath(C_ID_SERVICEPATH, servicePathV));
  }

  orion::BSONObjBuilder bobMatch;
  bobMatch.append("$match", bobMatchContent.obj());
  pipeline.append(bobMatch.obj());

  // $project
  projectStage(&pipeline);

  // $unwind (first)
  orion::BSONObjBuilder bobUnwind1;
  bobUnwind1.append("$unwind", S_ATTRNAMES);
  pipeline.append(bobUnwind1.obj());

  // $group (first)
  groupStage(&pipeline);

  // $unwind (second)
  orion::BSONObjBuilder bobUnwind2;
  bobUnwind2.append("$unwind", "$attrs");
  pipeline.append(bobUnwind2.obj());

  // $group (second)
  orion::BSONObjBuilder bobGroupContent2;
  bobGroupContent2.append("_id", "$attrs");

  orion::BSONObjBuilder bobGroup2;
  bobGroup2.append("$group", bobGroupContent2.obj());
  pipeline.append(bobGroup2.obj());

  // $sort
  sortStage(&pipeline);

  std::string err;

  TIME_STAT_MONGO_COMMAND_WAIT_START();
  orion::DBConnection connection = orion::getMongoConnection();
  orion::DBCursor cursor;

  if (!orion::collectionAggregate(connection, composeDatabaseName(tenant), COL_ENTITIES, pipeline.arr(), BATCH_SIZE, &cursor, &err))
  {
    TIME_STAT_MONGO_COMMAND_WAIT_STOP();
    orion::releaseMongoConnection(connection);
    responseP->statusCode.fill(SccReceiverInternalError, err);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }
  TIME_STAT_MONGO_COMMAND_WAIT_STOP();

  /* Processing result to build response */
  orion::BSONObj resultItem;
  unsigned int docs = 0;
  while (cursor.next(&resultItem))
  {
    if ((docs < offset) || (docs > offset + limit - 1))
    {
      docs++;
      continue;
    }

    docs++;
    LM_T(LmtMongo, ("aggregation result [%d]: %s", docs, resultItem.toString().c_str()));

    orion::BSONElement idField = getFieldF(resultItem, "_id");

    //
    // BSONElement::eoo returns true if 'not found', i.e. the field "_id" doesn't exist in 'sub'
    //
    // Now, if 'getFieldF(resultItem.embeddedObject(), "_id")' is not found, if we continue,
    // calling embeddedObject() on it, then we get an exception and the broker crashes.
    //
    if (idField.eoo() == true)
    {
      std::string details = std::string("error retrieving _id field in doc: '") +
                            resultItem.toString() + "'";

      alarmMgr.dbError(details);
      continue;
    }

    alarmMgr.dbErrorReset();

    /* Note that we need and extra query() to the database (inside attributeType() function) to get each attribute type.
     * This could be unefficient, specially if the number of attributes is large */
    if (!noAttrDetail)
    {
      std::vector<std::string> attrTypes;

      getAttributeTypes(tenant, servicePathV, entityType , idField.String(), &attrTypes);

      for (unsigned int kx = 0; kx < attrTypes.size(); ++kx)
      {
        ContextAttribute*  ca = new ContextAttribute(idField.String(), attrTypes[kx], "");

        responseP->entityType.contextAttributeVector.push_back(ca);

        // For backward compability, NGSIv1 only accepts one element
        if (apiVersion == V1)
        {
          break;
        }
      }
    }
    else
    {
      //
      // NOTE: here we add a ContextAttribute with empty type, as a marker for
      //       this special condition of 'No Attribute Detail'
      //
      ContextAttribute* caP = new ContextAttribute(idField.String(), "", "");
      responseP->entityType.contextAttributeVector.push_back(caP);
    }
  }
  orion::releaseMongoConnection(connection);

  responseP->entityType.count = countEntities(tenant, servicePathV, entityType);

  if (docs == 0)
  {
    responseP->statusCode.fill(SccContextElementNotFound);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }

  char detailsMsg[256];

  if (responseP->entityType.contextAttributeVector.size() > 0)
  {
    if (count)
    {
      snprintf(detailsMsg, sizeof(detailsMsg), "Count: %d", (int) docs);
      responseP->statusCode.fill(SccOk, detailsMsg);
    }
    else
    {
      responseP->statusCode.fill(SccOk);
    }
  }
  else
  {
    if (count)
    {
      snprintf(detailsMsg, sizeof(detailsMsg), "Number of attributes: %d. Offset is %u", docs, offset);
      responseP->statusCode.fill(SccContextElementNotFound, detailsMsg);
    }
    else
    {
      responseP->statusCode.fill(SccContextElementNotFound);
    }
  }

  reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

  return SccOk;
}
