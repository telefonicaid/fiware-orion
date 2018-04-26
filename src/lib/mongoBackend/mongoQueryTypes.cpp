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
using mongo::BSONElement;
using mongo::DBClientCursor;
using mongo::DBClientBase;



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
  std::string  idType         = std::string("_id.")    + ENT_ENTITY_TYPE;
  std::string  idServicePath  = std::string("_id.")    + ENT_SERVICE_PATH;
  BSONObj      query;

  if (entityType == "")
  {
    query = BSON("$or"         << BSON_ARRAY(BSON(idType << entityType) << BSON(idType << BSON("$exists" << false)) ) <<
                 idServicePath << fillQueryServicePath(servicePathV) <<
                 ENT_ATTRNAMES << attrName);
  }
  else
  {
    query = BSON(idType        << entityType <<
                 idServicePath << fillQueryServicePath(servicePathV) <<
                 ENT_ATTRNAMES << attrName);
  }

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

    /* Previous versions of this function used a simpler approach:
     *
     *   BSONObj attrs = getObjectFieldF(r, ENT_ATTRS);
     *   BSONObj attr  = getObjectFieldF(attrs, attrName);
     *   attrTypes->push_back(getStringFieldF(attr, ENT_ATTRS_TYPE));
     *
     * However, it doesn't work when the attribute uses metadata ID
     *
     * FIXME PR: mabye the simpler approach should be recovered?
     *
     */

    BSONObj                attrs = getObjectFieldF(r, ENT_ATTRS);
    std::set<std::string>  attrsSet;

    attrs.getFieldNames(attrsSet);

    for (std::set<std::string>::iterator i = attrsSet.begin(); i != attrsSet.end(); ++i)
    {
      std::string currentAttr = *i;

      if (currentAttr == attrName)
      {
        BSONObj attr = getObjectFieldF(attrs, currentAttr);
        attrTypes->push_back(getStringFieldF(attr, ENT_ATTRS_TYPE));
      }
    }
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
  std::string    idType        = std::string("_id.") + ENT_ENTITY_TYPE;
  std::string    idServicePath = std::string("_id.") + ENT_SERVICE_PATH;

  BSONObj query = BSON(idType        << entityType <<
                       idServicePath << fillQueryServicePath(servicePathV));

  std::string         err;
  unsigned long long  c;

  if (!collectionCount(getEntitiesCollectionName(tenant), query, &c, &err))
  {
    return -1;
  }

  return c;
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
   *                pipeline: [ {$match: { "_id.servicePath": /.../ } },
   *                            {$group: {_id: "$_id.type"} },
   *                            {$sort: {_id: 1} }
   *                          ]
   *                })
   *
   */

  BSONObj result;
  BSONObj spQuery = fillQueryServicePath(servicePathV);
  BSONObj cmd     = BSON("aggregate" << COL_ENTITIES <<
                         "pipeline"  << BSON_ARRAY(
                           BSON("$match" << BSON(C_ID_SERVICEPATH << spQuery)) <<
                           BSON("$group" << BSON("_id" << CS_ID_ENTITY)) <<
                           BSON("$sort"  << BSON("_id" << 1))));

  std::string err;

  if (!runCollectionCommand(composeDatabaseName(tenant), cmd, &result, &err))
  {
    responseP->statusCode.fill(SccReceiverInternalError, err);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }

  // Processing result to build response
  LM_T(LmtMongo, ("aggregation result: %s", result.toString().c_str()));

  std::vector<BSONElement> resultsArray = getFieldF(result, "result").Array();

  if (resultsArray.size() == 0)
  {
    responseP->statusCode.fill(SccOk);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }

  /* Null and "" (which can appear only in the case of creating entities using NGSIv1) are
   * collapsed to the same type "". Due to sorting, they appear at the beginning. In case
   * both of them appear, one has to be removed or the pagination logic would break.
   */
  if ((resultsArray.size() > 1) &&
      (getFieldF(resultsArray[0].embeddedObject(), "_id").isNull()) &&
      (getStringFieldF(resultsArray[1].embeddedObject(), "_id") == ""))
  {
    resultsArray.erase(resultsArray.begin());
  }


  /* Another strategy to implement pagination is to use the $skip and $limit operators in the
   * aggregation framework. However, doing so, we don't know the total number of results, which can
   * be needed in the case of count request (using that approach, we need to do two queries: one to get
   * the count and other to get the actual results with $skip and $limit, in the same "transaction" to
   * avoid incoherence between both if some entity type is created or deleted in the process).
   *
   * However, considering that the number of types will be small compared with the number of entities,
   * the current approach seems to be ok
   */
  if (totalTypesP != NULL)
  {
    *totalTypesP = resultsArray.size();
  }

  for (unsigned int ix = offset; ix < MIN(resultsArray.size(), offset + limit); ++ix)
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
   * The $cond part is hard... more information at
   *   http://stackoverflow.com/questions/27510143/empty-array-prevents-document-to-appear-in-query
   *
   * As a consequence, some "null" values may appear in the resulting attrs vector,
   * which are pruned by the result processing logic.
   *
   * FIXME P6: in the future, we can interpret the collapse parameter at this layer.
   *           If collapse=true so we don't need attributes, the
   *           following command can be used:
   *
   *           db.runCommand({aggregate: "entities", pipeline: [ {$group: {_id: "$_id.type"} }]})
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
          S_ATTRNAMES))));

  BSONObj cmd = BSON("aggregate" << COL_ENTITIES <<
                     "pipeline" << BSON_ARRAY(
                                              BSON("$match" << BSON(C_ID_SERVICEPATH << fillQueryServicePath(servicePathV))) <<
                                              BSON("$project" << BSON("_id" << 1 << ENT_ATTRNAMES << 1)) <<
                                              projection << BSON("$unwind" << S_ATTRNAMES) <<
                                              BSON("$group" << BSON("_id"   << CS_ID_ENTITY <<
                                                                    "attrs" << BSON("$addToSet" << S_ATTRNAMES))) <<
                                              BSON("$sort" << BSON("_id" << 1))));

  std::string err;

  if (!runCollectionCommand(composeDatabaseName(tenant), cmd, &result, &err))
  {
    responseP->statusCode.fill(SccReceiverInternalError, err);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }

  // Processing result to build response
  LM_T(LmtMongo, ("aggregation result: %s", result.toString().c_str()));

  std::vector<BSONElement> resultsArray = getFieldF(result, "result").Array();

  if (resultsArray.size() == 0)
  {
    responseP->statusCode.fill(SccContextElementNotFound);
    reqSemGive(__FUNCTION__, "query types request", reqSemTaken);

    return SccOk;
  }

  /* Another strategy to implement pagination is to use the $skip and $limit operators in the
   * aggregation framework. However, doing so, we don't know the total number of results, which can
   * be needed in the case of count request (using that approach, we need to do two queries: one to get
   * the count and other to get the actual results with $skip and $limit, in the same "transaction" to
   * avoid incoherence between both if some entity type is created or deleted in the process).
   *
   * However, considering that the number of types will be small compared with the number of entities,
   * the current approach seems to be ok
   *
   * emptyEntityType is special: it must aggregate results for entity type "" and for entities without type.
   * Is pre-created before starting processing results and destroyed if at the end it has not been used
   * (i.e. pushed back into the vector)
   *
   */

  EntityType* emptyEntityType     = new EntityType("");
  bool        emptyEntityTypeUsed = false;

  if (totalTypesP != NULL)
  {
    *totalTypesP = resultsArray.size();
  }

  for (unsigned int ix = offset; ix < MIN(resultsArray.size(), offset + limit); ++ix)
  {
    BSONObj                   resultItem  = resultsArray[ix].embeddedObject();
    std::vector<BSONElement>  attrsArray  = getFieldF(resultItem, "attrs").Array();
    EntityType*               entityType;

    //
    // nullId true means that the "cumulative" entityType for both no-type and type "" has to be used. This happens
    // when the results item has the field "" and at the same time the value of that field is JSON null or when
    // the value of the field "_id" is ""
    //
    bool nullId = ((resultItem.hasField("")) && (getFieldF(resultItem, "").isNull())) ||
                  getFieldF(resultItem, "_id").isNull()                               ||
                  (getStringFieldF(resultItem, "_id") == "");

    if (nullId)
    {
      entityType           = emptyEntityType;
      emptyEntityTypeUsed  = true;
    }
    else
    {
      entityType = new EntityType(getStringFieldF(resultItem, "_id"));
    }

    /* Note we use += due to emptyEntityType accumulates */
    entityType->count += countEntities(tenant, servicePathV, entityType->type);

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
    // entityType corresponding to nullId case is skipped, as it is (eventually) added outside the for loop
    if (!nullId)
    {
      responseP->entityTypeVector.push_back(entityType);
    }
  }

  if (emptyEntityTypeUsed)
  {
    responseP->entityTypeVector.push_back(emptyEntityType);
  }
  else
  {
    delete emptyEntityType;
  }

  char detailsMsg[256];

  if (responseP->entityTypeVector.size() > 0)
  {
    if (totalTypesP != NULL)
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
    if (totalTypesP != NULL)
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
  BSONObj cmd =
    BSON("aggregate" << COL_ENTITIES <<
         "pipeline" << BSON_ARRAY(
           BSON("$match" << BSON(C_ID_ENTITY << entityType <<
                                 C_ID_SERVICEPATH << fillQueryServicePath(servicePathV))) <<
           BSON("$project" << BSON("_id" << 1 << ENT_ATTRNAMES << 1)) <<
           BSON("$unwind" << S_ATTRNAMES) <<
           BSON("$group" << BSON("_id" << CS_ID_ENTITY << "attrs" << BSON("$addToSet" << S_ATTRNAMES))) <<
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

  std::vector<BSONElement> resultsArray = getFieldF(result, "result").Array();

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
