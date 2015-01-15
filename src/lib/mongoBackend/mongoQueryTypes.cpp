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

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoQueryTypes.h"

/* ****************************************************************************
*
* mongoEntityTypes -
*/
HttpStatusCode mongoEntityTypes
(
  EntityTypesResponse*                  responseP,
  const std::string&                    tenant,
  const std::vector<std::string>&       servicePathV,
  std::map<std::string, std::string>&   uriParams
)
{
  unsigned int offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
  unsigned int limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());
  std::string  detailsString  = uriParams[URI_PARAM_PAGINATION_DETAILS];
  bool         details        = (strcasecmp("on", detailsString.c_str()) == 0)? true : false;
  LM_T(LmtMongo, ("Query Entity Types"));
  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));

  reqSemTake(__FUNCTION__, "query types request");

  DBClientBase* connection = getMongoConnection();

  /* Compose query based on this aggregation command:
   *
   * FIXME P9: taking into account that type is no longer used as part of the attribute "key", not sure if the
   * aggregation query below is fully correct
   *
   * db.runCommand({aggregate: "entities",
   *                pipeline: [ {$match: { "_id.servicePath": /.../ } },
   *                            {$project: {_id: 1, "attrs.name": 1, "attrs.type": 1} },
   *                            {$unwind: "$attrs"},
   *                            {$group: {_id: "$_id.type", attrs: {$addToSet: "$attrs"}} },
   *                            {$sort: {_id: 1} }
   *                          ]
   *                })
   *
   * FIXME P6: in the future, we can interpret the collapse parameter at this layer. If collapse=true so we don't need attributes, the
   * following command can be used:
   *
   * db.runCommand({aggregate: "entities", pipeline: [ {$group: {_id: "$_id.type"} }]})
   *
   */

  BSONObj result;

  // Building the projection part of the query that includes types that have no attributes
  // See bug: https://github.com/telefonicaid/fiware-orion/issues/686
  BSONArrayBuilder emptyArrayBuilder;
  BSONArrayBuilder nulledArrayBuilder;
  nulledArrayBuilder.appendNull();

  // We are using the $cond: [ .. ] and not the $cond: { .. } one, as the former is the only one valid in MongoDB 2.4
  BSONObj projection = BSON(
    "$project" << BSON(
      "attrs" << BSON(
        "$cond" << BSON_ARRAY(
          BSON("$eq" << BSON_ARRAY(S_ATTRS << emptyArrayBuilder.arr()) ) <<
          nulledArrayBuilder.arr() <<
          S_ATTRS
        )
      )
    )
  );

  BSONObj cmd = BSON("aggregate" << COL_ENTITIES <<
                     "pipeline" << BSON_ARRAY(
                                              BSON("$match" << BSON(C_ID_SERVICEPATH << fillQueryServicePath(servicePathV))) <<
                                              BSON("$project" << BSON("_id" << 1 << C_ATTR_NAME << 1 << C_ATTR_TYPE << 1)) <<
                                              projection <<
                                              BSON("$unwind" << S_ATTRS) <<
                                              BSON("$group" << BSON("_id" << CS_ID_ENTITY << "attrs" << BSON("$addToSet" << S_ATTRS))) <<
                                              BSON("$sort" << BSON("_id" << 1))
                                             )
                     );

  LM_T(LmtMongo, ("runCommand() in '%s' database: '%s'", composeDatabaseName(tenant).c_str(), cmd.toString().c_str()));

  mongoSemTake(__FUNCTION__, "aggregation command");  
  try
  {

    connection->runCommand(composeDatabaseName(tenant).c_str(), cmd, result);
    mongoSemGive(__FUNCTION__, "aggregation command");
    LM_I(("Database Operation Successful (%s)", cmd.toString().c_str()));
  }
  catch (const DBException& e)
  {
      mongoSemGive(__FUNCTION__, "aggregation command");
      std::string err = std::string("database: ") + composeDatabaseName(tenant).c_str() +
              " - command: " + cmd.toString() +
              " - exception: " + e.what();

      LM_E(("Database Error (%s)", err.c_str()));
      responseP->statusCode.fill(SccReceiverInternalError, err);
      reqSemGive(__FUNCTION__, "query types request");
      return SccOk;
  }
  catch (...)
  {
      mongoSemGive(__FUNCTION__, "aggregation command");
      std::string err = std::string("database: ") + composeDatabaseName(tenant).c_str() +
              " - command: " + cmd.toString() +
              " - exception: " + "generic";

      LM_E(("Database Error (%s)", err.c_str()));
      responseP->statusCode.fill(SccReceiverInternalError, err);
      reqSemGive(__FUNCTION__, "query types request");
      return SccOk;
  }

  /* Processing result to build response*/
  LM_T(LmtMongo, ("aggregation result: %s", result.toString().c_str()));

  std::vector<BSONElement> resultsArray = result.getField("result").Array();

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

    BSONObj                  resultItem = resultsArray[ix].embeddedObject();
    TypeEntity*              type       = new TypeEntity(resultItem.getStringField("_id"));
    std::vector<BSONElement> attrsArray = resultItem.getField("attrs").Array();

    if (!attrsArray[0].isNull())
    {
      for (unsigned int jx = 0; jx < attrsArray.size(); ++jx)
      {
        BSONObj jAttr = attrsArray[jx].embeddedObject();
        ContextAttribute* ca = new ContextAttribute(jAttr.getStringField(ENT_ATTRS_NAME), jAttr.getStringField(ENT_ATTRS_TYPE));
        type->contextAttributeVector.push_back(ca);
      }
    }

    responseP->typeEntityVector.push_back(type);

  }

  char detailsMsg[256];
  if (responseP->typeEntityVector.size() > 0)
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
      snprintf(detailsMsg, sizeof(detailsMsg), "Number of types: %d. Offset is %d", (int) resultsArray.size(), offset);
      responseP->statusCode.fill(SccContextElementNotFound, detailsMsg);
    }
    else
    {
      responseP->statusCode.fill(SccContextElementNotFound);
    }
  }

  reqSemGive(__FUNCTION__, "query types request");

  return SccOk;

}

/* ****************************************************************************
*
* mongoAttributesForEntityType -
*/
HttpStatusCode mongoAttributesForEntityType
(
  std::string                           entityType,
  EntityTypeAttributesResponse*         responseP,
  const std::string&                    tenant,
  const std::vector<std::string>&       servicePathV,
  std::map<std::string, std::string>&   uriParams
)
{
  unsigned int offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
  unsigned int limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());
  std::string  detailsString  = uriParams[URI_PARAM_PAGINATION_DETAILS];
  bool         details        = (strcasecmp("on", detailsString.c_str()) == 0)? true : false;

  // Setting the name of the entity type for the response
  responseP->entityType.type = entityType;

  LM_T(LmtMongo, ("Query Types Attribute for <%s>", entityType.c_str()));
  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));

  reqSemTake(__FUNCTION__, "query types attributes request");

  DBClientBase* connection = getMongoConnection();

  /* Compose query based on this aggregation command:
   *
   * FIXME P9: taking into account that type is no longer used as part of the attribute "key", not sure if the
   * aggregation query below is fully correct
   *
   * db.runCommand({aggregate: "entities",
   *                pipeline: [ {$match: { "_id.type": "TYPE" , "_id.servicePath": /.../ } },
   *                            {$project: {_id: 1, "attrs.name": 1, "attrs.type": 1} },
   *                            {$unwind: "$attrs"},
   *                            {$group: {_id: "$_id.type", attrs: {$addToSet: "$attrs"}} },
   *                            {$unwind: "$attrs"},
   *                            {$group: {_id: "$attrs" }},
   *                            {$sort: {_id.name: 1, _id.type: 1} }
   *                          ]
   *                })
   *
   */

  BSONObj result;
  BSONObj cmd = BSON("aggregate" << COL_ENTITIES <<
                     "pipeline" << BSON_ARRAY(
                                              BSON("$match" << BSON(C_ID_ENTITY << entityType << C_ID_SERVICEPATH << fillQueryServicePath(servicePathV))) <<
                                              BSON("$project" << BSON("_id" << 1 << C_ATTR_NAME << 1 << C_ATTR_TYPE << 1)) <<
                                              BSON("$unwind" << S_ATTRS) <<
                                              BSON("$group" << BSON("_id" << CS_ID_ENTITY << "attrs" << BSON("$addToSet" << S_ATTRS))) <<
                                              BSON("$unwind" << S_ATTRS) <<
                                              BSON("$group" << BSON("_id" << S_ATTRS)) <<
                                              BSON("$sort" << BSON(C_ID_NAME << 1 << C_ID_TYPE << 1))
                                             )
                    );

  LM_T(LmtMongo, ("runCommand() in '%s' database: '%s'", composeDatabaseName(tenant).c_str(), cmd.toString().c_str()));

  mongoSemTake(__FUNCTION__, "aggregation command"); 
  try
  {

    connection->runCommand(composeDatabaseName(tenant).c_str(), cmd, result);
    mongoSemGive(__FUNCTION__, "aggregation command");
    LM_I(("Database Operation Successful (%s)", cmd.toString().c_str()));
  }
  catch (const DBException& e)
  {
      mongoSemGive(__FUNCTION__, "aggregation command");
      std::string err = std::string("database: ") + composeDatabaseName(tenant).c_str() +
              " - command: " + cmd.toString() +
              " - exception: " + e.what();

      LM_E(("Database Error (%s)", err.c_str()));
      responseP->statusCode.fill(SccReceiverInternalError, err);
      reqSemGive(__FUNCTION__, "query types request");
      return SccOk;
  }
  catch (...)
  {
      mongoSemGive(__FUNCTION__, "aggregation command");
      std::string err = std::string("database: ") + composeDatabaseName(tenant).c_str() +
              " - command: " + cmd.toString() +
              " - exception: " + "generic";

      LM_E(("Database Error (%s)", err.c_str()));
      responseP->statusCode.fill(SccReceiverInternalError, err);
      reqSemGive(__FUNCTION__, "query types request");
      return SccOk;
  }

  /* Processing result to build response*/
  LM_T(LmtMongo, ("aggregation result: %s", result.toString().c_str()));

  std::vector<BSONElement> resultsArray = result.getField("result").Array();

  /* See comment above in the other method regarding this strategy to implement pagination */
  for (unsigned int ix = offset; ix < MIN(resultsArray.size(), offset + limit); ++ix)
  {
    BSONElement        idField    = resultsArray[ix].embeddedObject().getField("_id");

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

    BSONObj            resultItem = idField.embeddedObject();
    ContextAttribute*  ca         = new ContextAttribute(resultItem.getStringField(ENT_ATTRS_NAME), resultItem.getStringField(ENT_ATTRS_TYPE));
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
      snprintf(detailsMsg, sizeof(detailsMsg), "Number of attributes: %d. Offset is %d", (int) resultsArray.size(), offset);
      responseP->statusCode.fill(SccContextElementNotFound, detailsMsg);
    }
    else
    {
      responseP->statusCode.fill(SccContextElementNotFound);
    }
  }

  reqSemGive(__FUNCTION__, "query types request");

  return SccOk;
}


