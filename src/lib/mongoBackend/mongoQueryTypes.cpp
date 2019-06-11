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
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/dbFieldEncoding.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/mongoQueryTypes.h"



/* ****************************************************************************
*
* USING
*/
using mongo::BSONArrayBuilder;
using mongo::BSONObj;
using mongo::BSONObjBuilder;
using mongo::BSONArray;
using mongo::BSONElement;
using mongo::BSONNULL;
using mongo::DBClientCursor;
using mongo::DBClientBase;



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

  BSONObjBuilder bob;

  if (entityType == "")
  {
    std::string idType = std::string("_id.") + ENT_ENTITY_TYPE;
    bob.append("$or", BSON_ARRAY(BSON(idType << entityType) << BSON(idType << BSON("$exists" << false)) ));
  }
  if (servicePathFilterNeeded(servicePathV))
  {
    bob.appendElements(fillQueryServicePath("_id." ENT_SERVICE_PATH, servicePathV));
  }
  bob.append(ENT_ATTRNAMES, attrName);

  BSONObj query = bob.obj();

  std::auto_ptr<DBClientCursor>  cursor;
  std::string                    err;

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();

  if (!collectionQuery(connection, getEntitiesCollectionName(tenant), query, &cursor, &err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    return;
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  unsigned int  docs = 0;

  while (moreSafe(cursor))
  {
    BSONObj r;

    if (!nextSafeOrErrorF(cursor, &r, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s - query: %s)", err.c_str(), query.toString().c_str()));
      continue;
    }

    docs++;
    LM_T(LmtMongo, ("retrieved document [%d]: '%s'", docs, r.toString().c_str()));

    BSONObj attrs = getObjectFieldF(r, ENT_ATTRS);
    BSONObj attr  = getObjectFieldF(attrs, attrName);
    attrTypes->push_back(getStringFieldF(attr, ENT_ATTRS_TYPE));
  }

  releaseMongoConnection(connection);
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
  BSONObjBuilder bob;

  bob.append("_id." ENT_ENTITY_TYPE, entityType);
  if (servicePathFilterNeeded(servicePathV))
  {
    bob.appendElements(fillQueryServicePath("_id." ENT_SERVICE_PATH, servicePathV));
  }

  std::string         err;
  unsigned long long  c;

  if (!collectionCount(getEntitiesCollectionName(tenant), bob.obj(), &c, &err))
  {
    return -1;
  }

  return c;
}

/* ****************************************************************************
*
* countCmd -
*
*/
static unsigned int countCmd(const std::string& tenant, const BSONArray& pipelineForCount)
{
  BSONObj result;
  BSONObj cmd     = BSON("aggregate" << COL_ENTITIES <<
                         "cursor" << BSON("batchSize" << BATCH_SIZE) <<
                         "pipeline"  << pipelineForCount);

  std::string err;

  if (!runCollectionCommand(composeDatabaseName(tenant), cmd, &result, &err))
  {
    LM_E(("Runtime Error (executing: %s, error %s)", cmd.toString().c_str(), err.c_str()));
    return 0;
  }

  // Processing result to build response
  LM_T(LmtMongo, ("aggregation result: %s", result.toString().c_str()));

  std::vector<BSONElement> resultsArray = std::vector<BSONElement>();

  if (result.hasField("cursor"))
  {
    // abcense of "count" field in the "firtBatch" array means "zero result"
    resultsArray = getFieldF(getObjectFieldF(result, "cursor"), "firstBatch").Array();
    if ((resultsArray.size() > 0) && (resultsArray[0].embeddedObject().hasField("count")))
    {
      return getIntFieldF(resultsArray[0].embeddedObject(), "count");
    }
  }
  else
  {
    LM_E(("Runtime Error (executing: %s, result hasn't cursor field", cmd.toString().c_str()));
  }

  return 0;
}


/* ****************************************************************************
*
* mongoEntityTypesValues -
*
* "Simplified" version of mongoEntityTypes(), using a simpler aggregation command
* and the processing logic afterwards. Note that apiVersion is not included in this
* operation as it can be used only in NGSIv2
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
   * db.runCommand({aggregate: "entities",
   *                cursor: { batchSize: 1000 },
   *                pipeline: [ {$match: { "_id.servicePath": /.../ } },
   *                (1)         {$project: {_id.type: {$ifNull: ["$_id.type", null]}, "attrNames": 1 } },
   *                (2)         {$group: {_id: {$cond: [{$in: [$_id.type, [null, ""]]}, "", "$_id.type"]} },
   *                            {$sort: {_id: 1} },
   *                            {$skip: ... },
   *                            {$limit: ... }
   *                          ]
   *                })
   *
   * (1) The $ifNull causes that entities without _id.type have _id.type: null (used by (2))
   *
   * (2) The $cond used in this $group is to group together entities with empty type ("") and null type
   *
   * Up to Orion 2.0.0 we didn't use $skip and $limit. We got all the results in order to get the count
   * (to be used in fiware-total-count if the user request so). However, support to MongoDB 3.6 with the
   * old driver needs using cursor and bathSize so the old approach is no longer valid and we do now
   * the count in a separate operation. Note that with this approach in some cases the count could not
   * be accurate if some entity creation/deletion operation takes place in the middle. However, it is a
   * reasonable price to pay to support MongoDB 3.6.
   *
   * The new approach also assumes that the batch will not surpass 16MB. It is hardly improbable to reach
   * the 16MB limit given that each individual result is small (just a short list of attribute names).
   *
   * For more detail on this, check dicussion in the following links. Old implementation can be retrieved
   * in tag 2.0.0 in the case it could be uselful (for instance, due to a change in the mongo driver).
   *
   * https://github.com/telefonicaid/fiware-orion/issues/3070
   * https://github.com/telefonicaid/fiware-orion/pull/3251
   *
   */

  BSONArrayBuilder pipeline;
  BSONArrayBuilder pipelineForCount;

  // Common elements to both pipelines
  if (servicePathFilterNeeded(servicePathV))
  {
    BSONObj spQuery = fillQueryServicePath(C_ID_SERVICEPATH, servicePathV);
    pipeline.append(BSON("$match" << spQuery));
    pipelineForCount.append(BSON("$match" << spQuery));
  }
  BSONObj project = BSON("$project" << BSON(C_ID_ENTITY << BSON("$ifNull" << BSON_ARRAY(CS_ID_ENTITY << BSONNULL)) << ENT_ATTRNAMES << 1));
  BSONObj groupCond = BSON("$cond" << BSON_ARRAY(
                               BSON("$in" << BSON_ARRAY(CS_ID_ENTITY << BSON_ARRAY(BSONNULL << ""))) <<
                               "" <<
                               CS_ID_ENTITY));

  BSONObj group   = BSON("$group" << BSON("_id" << groupCond));
  pipeline.append(project);
  pipeline.append(group);
  pipelineForCount.append(project);
  pipelineForCount.append(group);

  // Specific elements for pipeline
  pipeline.append(BSON("$sort"  << BSON("_id" << 1)));
  pipeline.append(BSON("$skip" << offset));
  pipeline.append(BSON("$limit" << limit));

  // Specific elements for pipelineForCount
  pipelineForCount.append(BSON("$count" << "count"));

  BSONObj result;
  BSONObj cmd     = BSON("aggregate" << COL_ENTITIES <<
                         "cursor" << BSON("batchSize" << BATCH_SIZE) <<
                         "pipeline"  << pipeline.arr());

  std::string err;

  if (!runCollectionCommand(composeDatabaseName(tenant), cmd, &result, &err))
  {
    responseP->statusCode.fill(SccReceiverInternalError, err);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }

  // Get count if user requested (i.e. if totalTypesP is not NULL)
  if (totalTypesP != NULL)
  {
    *totalTypesP = countCmd(tenant, pipelineForCount.arr());
  }

  // Processing result to build response
  LM_T(LmtMongo, ("aggregation result: %s", result.toString().c_str()));

  std::vector<BSONElement> resultsArray = std::vector<BSONElement>();

  if (result.hasField("cursor"))
  {
    resultsArray = getFieldF(getObjectFieldF(result, "cursor"), "firstBatch").Array();
  }

  if (resultsArray.size() == 0)
  {
    responseP->statusCode.fill(SccOk);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }

  for (unsigned int ix = 0; ix < resultsArray.size(); ++ix)
  {
    BSONObj     resultItem = resultsArray[ix].embeddedObject();
    std::string type;

    LM_T(LmtMongo, ("result item[%d]: %s", ix, resultItem.toString().c_str()));

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

  responseP->statusCode.fill(SccOk);
  reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

  return SccOk;
}



/* ****************************************************************************
*
* mongoEntityTypes -
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
   * db.runCommand({aggregate: "entities",
   *                cursor: { batchSize: 1000 },
   *                pipeline: [ {$match: { "_id.servicePath": /.../ } },
   *                (1)         {$project: {_id.type: {$ifNull: ["$_id.type", null]}, "attrNames": 1 } },
   *                            {$project: { "attrNames"
   *                (2)               {$cond: [ {$eq: [ "$attrNames", [ ] ] }, [null], "$attrNames"] }
   *                               }
   *                            },
   *                            {$unwind: "$attrNames"},
   *                (3)         {$group: {_id: {$cond: [{$in: [$_id.type, [null, ""]]}, "", "$_id.type"]},
   *                                      attrs: {$addToSet: "$attrNames"}},
   *                            {$sort: {_id: 1} },
   *                            {$skip: ... },
   *                            {$limit: ... },
   *                          ]
   *                })
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
   *           db.runCommand({aggregate: "entities", pipeline: [ {$group: {_id: "$_id.type"} }]})
   *
   * Up to Orion 2.0.0 we didn't use $skip and $limit. We got all the results in order to get the count
   * (to be used in fiware-total-count if the user request so). However, support to MongoDB 3.6 with the
   * old driver needs using cursor and bathSize so the old approach is no longer valid and we do now
   * the count in a separate operation. Note that with this approach in some cases the count could not
   * be accurate if some entity creation/deletion operation takes place in the middle. However, it is a
   * reasonable price to pay to support MongoDB 3.6.
   *
   * The new approach also assumes that the batch will not surpass 16MB. It is hardly improbable to reach
   * the 16MB limit given that each individual result is small (just a short list of attribute names).
   *
   * For more detail on this, check dicussion in the following links. Old implementation can be retrieved
   * in tag 2.0.0 in the case it could be uselful (for instance, due to a change in the mongo driver).
   */

  BSONObj result;

  BSONArrayBuilder pipeline;
  BSONArrayBuilder pipelineForCount;

  // Common elements to both pipelines
  if (servicePathFilterNeeded(servicePathV))
  {
    BSONObj spQuery = fillQueryServicePath(C_ID_SERVICEPATH, servicePathV);
    pipeline.append(BSON("$match" << spQuery));
    pipelineForCount.append(BSON("$match" << spQuery));
  }

  BSONObj groupCond = BSON("$cond" << BSON_ARRAY(
                               BSON("$in" << BSON_ARRAY(CS_ID_ENTITY << BSON_ARRAY(BSONNULL << ""))) <<
                               "" <<
                               CS_ID_ENTITY));

  BSONObj project = BSON("$project" << BSON(C_ID_ENTITY << BSON("$ifNull" << BSON_ARRAY(CS_ID_ENTITY << BSONNULL)) << ENT_ATTRNAMES << 1));

  //
  // Building the projection part of the query that includes types that have no attributes
  // See bug: https://github.com/telefonicaid/fiware-orion/issues/686
  //

  // FIXME P3. We are using the $cond: [ .. ] and not the $cond: { .. } one, due to the former was
  // the only one valid in MongoDB 2.4. However, MongoDB 2.4 support was removed time ago, so we could
  // change the syntax
  BSONObj projection = BSON(
    "$project" << BSON(
      ENT_ATTRNAMES << BSON(
        "$cond" << BSON_ARRAY(
          BSON("$eq" << BSON_ARRAY(S_ATTRNAMES << BSONArray() )) <<
          BSON_ARRAY(BSONNULL) <<
          S_ATTRNAMES))));

  BSONObj unwind = BSON("$unwind" << S_ATTRNAMES);

  BSONObj group = BSON("$group" << BSON("_id"   << groupCond << "attrs" << BSON("$addToSet" << S_ATTRNAMES)));

  pipeline.append(project);
  pipeline.append(projection);
  pipeline.append(unwind);
  pipeline.append(group);

  pipelineForCount.append(project);
  pipelineForCount.append(projection);
  pipelineForCount.append(unwind);
  pipelineForCount.append(group);

  // Specific elements for pipeline
  pipeline.append(BSON("$sort" << BSON("_id" << 1)));
  pipeline.append(BSON("$skip" << offset));
  pipeline.append(BSON("$limit" << limit));

  // Specific elements for pipelineForCount
  pipelineForCount.append(BSON("$count" << "count"));

  BSONObj cmd = BSON("aggregate" << COL_ENTITIES <<
                     "cursor" << BSON("batchSize" << BATCH_SIZE) <<
                     "pipeline" << pipeline.arr());

  std::string err;

  if (!runCollectionCommand(composeDatabaseName(tenant), cmd, &result, &err))
  {
    responseP->statusCode.fill(SccReceiverInternalError, err);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }

  // Get count if user requested (i.e. if totalTypesP is not NULL)
  if (totalTypesP != NULL)
  {
    *totalTypesP = countCmd(tenant, pipelineForCount.arr());
  }

  // Processing result to build response
  LM_T(LmtMongo, ("aggregation result: %s", result.toString().c_str()));

  std::vector<BSONElement> resultsArray = std::vector<BSONElement>();

  if (result.hasField("cursor"))
  {
    resultsArray = getFieldF(getObjectFieldF(result, "cursor"), "firstBatch").Array();
  }

  // Early return if no element was found
  if (resultsArray.size() == 0)
  {
    if (totalTypesP != NULL)
    {
      char detailsMsg[256];
      snprintf(detailsMsg, sizeof(detailsMsg), "Number of types: %u. Offset is %u", *totalTypesP, offset);
      responseP->statusCode.fill(SccContextElementNotFound, detailsMsg);
    }
    else
    {
      responseP->statusCode.fill(SccContextElementNotFound);
    }

    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }

  for (unsigned int ix = 0; ix < resultsArray.size(); ++ix)
  {
    BSONObj                   resultItem  = resultsArray[ix].embeddedObject();
    std::vector<BSONElement>  attrsArray  = getFieldF(resultItem, "attrs").Array();
    EntityType*               entityType;

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

          getAttributeTypes(tenant, servicePathV, entityType->type , attrsArray[jx].str(), &attrTypes);

          for (unsigned int kx = 0; kx < attrTypes.size(); ++kx)
          {
            ContextAttribute* ca = new ContextAttribute(attrsArray[jx].str(), attrTypes[kx], "");

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
          ContextAttribute* caP = new ContextAttribute(attrsArray[jx].str(), "", "");
          entityType->contextAttributeVector.push_back(caP);
        }
      }
    }

    responseP->entityTypeVector.push_back(entityType);
  }

  if (totalTypesP != NULL)
  {
    char detailsMsg[256];
    snprintf(detailsMsg, sizeof(detailsMsg), "Count: %u", *totalTypesP);
    responseP->statusCode.fill(SccOk, detailsMsg);
  }
  else
  {
    responseP->statusCode.fill(SccOk);
  }

  reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

  return SccOk;
}



/* ****************************************************************************
*
* mongoAttributesForEntityType -
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
   * db.runCommand({aggregate: "entities",
   *                cursor: { batchSize: 1000 },
   *                pipeline: [ {$match: { "_id.type": "TYPE" , "_id.servicePath": /.../ } },
   *                (1)         {$project: {_id.type: {$ifNull: ["$_id.type", null]}, "attrNames": 1 } },
   *                            {$unwind: "$attrNames"},
   *                (2)         {$group: {_id: {$cond: [{$in: [$_id.type, [null, ""]]}, "", "$_id.type"]},
   *                                      attrs: {$addToSet: "$attrNames"}},
   *                            {$unwind: "$attrs"},
   *                            {$group: {_id: "$attrs" }},
   *                            {$sort: {_id: 1}}
   *                          ]
   *                })
   *
   * (1) The $ifNull causes that entities without _id.type have _id.type: null (used by (2))
   *
   * (2) The $cond used in this $group is to group together entities with empty type ("") and null type
   *
   */

  BSONObj groupCond = BSON("$cond" << BSON_ARRAY(
                               BSON("$in" << BSON_ARRAY(CS_ID_ENTITY << BSON_ARRAY(BSONNULL << ""))) <<
                               "" <<
                               CS_ID_ENTITY));

  BSONObjBuilder match;

  match.append(C_ID_ENTITY, entityType);
  if (servicePathFilterNeeded(servicePathV))
  {
    match.appendElements(fillQueryServicePath(C_ID_SERVICEPATH, servicePathV));
  }

  BSONObj result;
  BSONObj cmd =
    BSON("aggregate" << COL_ENTITIES <<
         "cursor" << BSON("batchSize" << BATCH_SIZE) <<
         "pipeline" << BSON_ARRAY(
           BSON("$match" << match.obj()) <<
           BSON("$project" << BSON(C_ID_ENTITY << BSON("$ifNull" << BSON_ARRAY(CS_ID_ENTITY << BSONNULL)) << ENT_ATTRNAMES << 1)) <<
           BSON("$unwind" << S_ATTRNAMES) <<
           BSON("$group" << BSON("_id" << groupCond << "attrs" << BSON("$addToSet" << S_ATTRNAMES))) <<
           BSON("$unwind" << "$attrs") <<
           BSON("$group" << BSON("_id" << "$attrs")) <<
           BSON("$sort" << BSON("_id" << 1))));

  std::string err;
  if (!runCollectionCommand(composeDatabaseName(tenant), cmd, &result, &err))
  {
    responseP->statusCode.fill(SccReceiverInternalError, err);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }

  /* Processing result to build response */
  LM_T(LmtMongo, ("aggregation result: %s", result.toString().c_str()));

  std::vector<BSONElement> resultsArray = std::vector<BSONElement>();

  if (result.hasField("cursor"))
  {
    resultsArray = getFieldF(getObjectFieldF(result, "cursor"), "firstBatch").Array();
  }

  responseP->entityType.count = countEntities(tenant, servicePathV, entityType);

  if (resultsArray.size() == 0)
  {
    responseP->statusCode.fill(SccContextElementNotFound);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }

  /* See comment above in the other method regarding this strategy to implement pagination */
  for (unsigned int ix = offset; ix < MIN(resultsArray.size(), offset + limit); ++ix)
  {
    BSONElement idField = getFieldF(resultsArray[ix].embeddedObject(), "_id");

    //
    // BSONElement::eoo returns true if 'not found', i.e. the field "_id" doesn't exist in 'sub'
    //
    // Now, if 'getFieldF(resultsArray[ix].embeddedObject(), "_id")' is not found, if we continue,
    // calling embeddedObject() on it, then we get an exception and the broker crashes.
    //
    if (idField.eoo() == true)
    {
      std::string details = std::string("error retrieving _id field in doc: '") +
                            resultsArray[ix].embeddedObject().toString() + "'";

      alarmMgr.dbError(details);
      continue;
    }

    alarmMgr.dbErrorReset();

    /* Note that we need and extra query() to the database (inside attributeType() function) to get each attribute type.
     * This could be unefficient, specially if the number of attributes is large */
    if (!noAttrDetail)
    {
      std::vector<std::string> attrTypes;

      getAttributeTypes(tenant, servicePathV, entityType , idField.str(), &attrTypes);

      for (unsigned int kx = 0; kx < attrTypes.size(); ++kx)
      {
        ContextAttribute*  ca = new ContextAttribute(idField.str(), attrTypes[kx], "");

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
      ContextAttribute* caP = new ContextAttribute(idField.str(), "", "");
      responseP->entityType.contextAttributeVector.push_back(caP);
    }
  }

  char detailsMsg[256];

  if (responseP->entityType.contextAttributeVector.size() > 0)
  {
    if (count)
    {
      snprintf(detailsMsg, sizeof(detailsMsg), "Count: %d", (int) resultsArray.size());
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
      snprintf(detailsMsg, sizeof(detailsMsg), "Number of attributes: %zu. Offset is %u", resultsArray.size(), offset);
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
