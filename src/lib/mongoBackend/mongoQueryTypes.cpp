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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"
#include "common/statistics.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/connectionOperations.h"
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/mongoQueryTypes.h"


/* ****************************************************************************
*
* attributeType -
*
*/
static std::string attributeType
(
  const std::string&                    tenant,
  const std::vector<std::string>&       servicePathV,
  const std::string&                    entityType,
  const std::string&                    attrName
)
{
  std::string  idType         = std::string("_id.")    + ENT_ENTITY_TYPE;
  std::string  idServicePath  = std::string("_id.")    + ENT_SERVICE_PATH;
  std::string  attributeName  = std::string(ENT_ATTRS) + "." + attrName;

  BSONObj query = BSON(idType        << entityType <<
                       idServicePath << fillQueryServicePath(servicePathV) <<
                       attributeName << BSON("$exists" << true));

  std::auto_ptr<DBClientCursor> cursor;
  std::string                   err;

  TIME_STAT_MONGO_READ_WAIT_START();
  DBClientBase* connection = getMongoConnection();
  if (!collectionQuery(connection, getEntitiesCollectionName(tenant), query, &cursor, &err))
  {
    releaseMongoConnection(connection);
    TIME_STAT_MONGO_READ_WAIT_STOP();
    return "";
  }
  TIME_STAT_MONGO_READ_WAIT_STOP();

  std::string ret = "";
  while (moreSafe(cursor))
  {
    BSONObj r;
    if (!nextSafeOrError(cursor, &r, &err))
    {
      LM_E(("Runtime Error (exception in nextSafe(): %s", err.c_str()));
      continue;
    }
    LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

    /* It could happen that different entities within the same entity type may have attributes with the same name
     * but different types. In that case, one type (at random) is returned. A list could be returned but the
     * NGSIv2 operations only allow to set one type */
    BSONObj attrs = getField(r, ENT_ATTRS).embeddedObject();
    BSONObj attr = getField(attrs, attrName).embeddedObject();
    ret = getStringField(attr, ENT_ATTRS_TYPE);
    break;
  }
  releaseMongoConnection(connection);

  return ret;
}


/* ****************************************************************************
*
* countEntities -
*
*/
static long long countEntities(const std::string& tenant, const std::vector<std::string>& servicePathV,std::string entityType)
{  
  std::string    idType        = std::string("_id.") + ENT_ENTITY_TYPE;
  std::string    idServicePath = std::string("_id.") + ENT_SERVICE_PATH;

  BSONObj query = BSON(idType        << entityType <<
                       idServicePath << fillQueryServicePath(servicePathV));

  std::string        err;
  unsigned long long c;
  if (!collectionCount(getEntitiesCollectionName(tenant), query, &c, &err))
  {
    return -1;
  }
  return c;

}


/* ****************************************************************************
*
* mongoEntityTypes -
*/
HttpStatusCode mongoEntityTypes
(
  EntityTypeVectorResponse*                  responseP,
  const std::string&                    tenant,
  const std::vector<std::string>&       servicePathV,
  std::map<std::string, std::string>&   uriParams
)
{
  unsigned int   offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
  unsigned int   limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());
  std::string    detailsString  = uriParams[URI_PARAM_PAGINATION_DETAILS];
  bool           details        = (strcasecmp("on", detailsString.c_str()) == 0)? true : false;  
  bool           reqSemTaken    = false;

  LM_T(LmtMongo, ("Query Entity Types"));
  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));

  reqSemTake(__FUNCTION__, "query types request", SemReadOp, &reqSemTaken);

  /* Compose query based on this aggregation command:  
   *
   * db.runCommand({aggregate: "entities",
   *                pipeline: [ {$match: { "_id.servicePath": /.../ } },
   *                            {$project: {_id: 1, "attrNames": 1} },
   *                            {$project: { "attrNames"
   *                                  {$cond: [ {$eq: [ "$attrNames", [ ] ] }, [null], "$attrNames"] }
   *                               }
   *                            },
   *                            {$unwind: "$attrNames"},
   *                            {$group: {_id: "$_id.type", attrs: {$addToSet: "$attrNames"}} },
   *                            {$sort: {_id: 1} }
   *                          ]
   *                })   
   *
   * The $cond part is hard... more information at http://stackoverflow.com/questions/27510143/empty-array-prevents-document-to-appear-in-query
   * As a consequence, some "null" values may appear in the resulting attrs vector, which are pruned by the result processing logic.
   *
   * FIXME P6: in the future, we can interpret the collapse parameter at this layer. If collapse=true so we don't need attributes, the
   * following command can be used:
   *
   * db.runCommand({aggregate: "entities", pipeline: [ {$group: {_id: "$_id.type"} }]})
   *
   *
   *
   */

  BSONObj result;

  //
  // Building the projection part of the query that includes types that have no attributes
  // See bug: https://github.com/telefonicaid/fiware-orion/issues/686
  //
  BSONArrayBuilder  emptyArrayBuilder;
  BSONArrayBuilder  nulledArrayBuilder;

  nulledArrayBuilder.appendNull();

  // We are using the $cond: [ .. ] and not the $cond: { .. } one, as the former is the only one valid in MongoDB 2.4
  BSONObj projection = BSON(
    "$project" << BSON(
      ENT_ATTRNAMES << BSON(
        "$cond" << BSON_ARRAY(
          BSON("$eq" << BSON_ARRAY(S_ATTRNAMES << emptyArrayBuilder.arr()) ) <<
          nulledArrayBuilder.arr() <<
          S_ATTRNAMES
        )
      )
    )
  );

  BSONObj cmd = BSON("aggregate" << COL_ENTITIES <<
                     "pipeline" << BSON_ARRAY(
                                              BSON("$match" << BSON(C_ID_SERVICEPATH << fillQueryServicePath(servicePathV))) <<
                                              BSON("$project" << BSON("_id" << 1 << ENT_ATTRNAMES << 1)) <<
                                              projection <<
                                              BSON("$unwind" << S_ATTRNAMES) <<
                                              BSON("$group" << BSON("_id" << CS_ID_ENTITY << "attrs" << BSON("$addToSet" << S_ATTRNAMES))) <<
                                              BSON("$sort" << BSON("_id" << 1))
                                             )
                     );

  std::string err;
  if (!runCollectionCommand(composeDatabaseName(tenant), cmd, &result, &err))
  {
    responseP->statusCode.fill(SccReceiverInternalError, err);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);
    return SccOk;
  }

  // Processing result to build response
  LM_T(LmtMongo, ("aggregation result: %s", result.toString().c_str()));

  std::vector<BSONElement> resultsArray = getField(result, "result").Array();

  if (resultsArray.size() == 0)
  {
    responseP->statusCode.fill(SccContextElementNotFound);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }

  /* Another strategy to implement pagination is to use the $skip and $limit operators in the
   * aggregation framework. However, doing so, we don't know the total number of results, which can
   * be needed in the case of details=on (using that approach, we need to do two queries: one to get
   * the count and other to get the actual results with $skip and $limit, in the same "transaction" to
   * avoid incoherence between both if some entity type is created or deleted in the process).
   *
   * However, considering that the number of types will be small compared with the number of entities,
   * the current approach seems to be ok
   */
  for (unsigned int ix = offset; ix < MIN(resultsArray.size(), offset + limit); ++ix)
  {
    BSONObj                   resultItem  = resultsArray[ix].embeddedObject();
    EntityType*               entityType  = new EntityType(getStringField(resultItem, "_id"));
    std::vector<BSONElement>  attrsArray  = getField(resultItem, "attrs").Array();

    entityType->count = countEntities(tenant, servicePathV, entityType->type);

    if (!attrsArray[0].isNull())
    {
      for (unsigned int jx = 0; jx < attrsArray.size(); ++jx)
      {
        /* This is where NULL elements in the resulting attrs vector are pruned */
        if (attrsArray[jx].isNull())
        {
          continue;
        }

        /* Note that we need and extra query() to the database (inside attributeType() function) to get each attribute type.
         * This could be unefficient, specially if the number of attributes is large */
        std::string attrType = attributeType(tenant, servicePathV, entityType->type , attrsArray[jx].str());

        ContextAttribute* ca = new ContextAttribute(attrsArray[jx].str(), attrType, "");
        entityType->contextAttributeVector.push_back(ca);
      }
    }

    responseP->entityTypeVector.push_back(entityType);
  }

  char detailsMsg[256];
  if (responseP->entityTypeVector.size() > 0)
  {
    if (details)
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
    if (details)
    {      
      snprintf(detailsMsg, sizeof(detailsMsg), "Number of types: %zu. Offset is %u", resultsArray.size(), offset);
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


/* ****************************************************************************
*
* mongoAttributesForEntityType -
*/
HttpStatusCode mongoAttributesForEntityType
(
  std::string                           entityType,
  EntityTypeResponse*         responseP,
  const std::string&                    tenant,
  const std::vector<std::string>&       servicePathV,
  std::map<std::string, std::string>&   uriParams
)
{  
  unsigned int   offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
  unsigned int   limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());
  std::string    detailsString  = uriParams[URI_PARAM_PAGINATION_DETAILS];
  bool           details        = (strcasecmp("on", detailsString.c_str()) == 0)? true : false;
  bool           reqSemTaken    = false;

  // Setting the name of the entity type for the response
  responseP->entityType.type = entityType;

  LM_T(LmtMongo, ("Query Types Attribute for <%s>", entityType.c_str()));
  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));

  reqSemTake(__FUNCTION__, "query types attributes request", SemReadOp, &reqSemTaken);


  /* Compose query based on this aggregation command:   
   *
   * db.runCommand({aggregate: "entities",
   *                pipeline: [ {$match: { "_id.type": "TYPE" , "_id.servicePath": /.../ } },
   *                            {$project: {_id: 1, "attrNames": 1} },
   *                            {$unwind: "$attrNames"},
   *                            {$group: {_id: "$_id.type", attrs: {$addToSet: "$attrNames"}} },
   *                            {$unwind: "$attrs"},
   *                            {$group: {_id: "$attrs" }},
   *                            {$sort: {_id: 1}}
   *                          ]
   *                })
   *
   */

  BSONObj result;
  BSONObj cmd = BSON("aggregate" << COL_ENTITIES <<
                     "pipeline" << BSON_ARRAY(
                                              BSON("$match" << BSON(C_ID_ENTITY << entityType << C_ID_SERVICEPATH << fillQueryServicePath(servicePathV))) <<
                                              BSON("$project" << BSON("_id" << 1 << ENT_ATTRNAMES << 1)) <<
                                              BSON("$unwind" << S_ATTRNAMES) <<
                                              BSON("$group" << BSON("_id" << CS_ID_ENTITY << "attrs" << BSON("$addToSet" << S_ATTRNAMES))) <<
                                              BSON("$unwind" << "$attrs") <<
                                              BSON("$group" << BSON("_id" << "$attrs")) <<
                                              BSON("$sort" << BSON("_id" << 1))
                                             )
                    );

  std::string err;
  if (!runCollectionCommand(composeDatabaseName(tenant), cmd, &result, &err))
  {
    responseP->statusCode.fill(SccReceiverInternalError, err);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);
    return SccOk;
  }

  /* Processing result to build response */
  LM_T(LmtMongo, ("aggregation result: %s", result.toString().c_str()));

  std::vector<BSONElement> resultsArray = getField(result, "result").Array();

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
    BSONElement  idField    = resultsArray[ix].embeddedObject().getField("_id");

    //
    // BSONElement::eoo returns true if 'not found', i.e. the field "_id" doesn't exist in 'sub'
    //
    // Now, if 'resultsArray[ix].embeddedObject().getField("_id")' is not found, if we continue,
    // calling embeddedObject() on it, then we get an exception and the broker crashes.
    //
    if (idField.eoo() == true)
    {
      LM_E(("Database Error (error retrieving _id field in doc: %s)", resultsArray[ix].embeddedObject().toString().c_str()));
      continue;
    }

    /* Note that we need and extra query() to the database (inside attributeType() function) to get each attribute type.
     * This could be unefficient, specially if the number of attributes is large */
    std::string attrType = attributeType(tenant, servicePathV, entityType , idField.str());

    ContextAttribute*  ca = new ContextAttribute(idField.str(), attrType, "");
    responseP->entityType.contextAttributeVector.push_back(ca);
  }

  char detailsMsg[256];
  if (responseP->entityType.contextAttributeVector.size() > 0)
  {
    if (details)
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
    if (details)
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
