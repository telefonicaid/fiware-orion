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
* fermin at tid dot es
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
   * db.runCommand({aggregate: "entities",
   *                pipeline: [ {$project: {_id: 1, "attrs.name": 1, "attrs.type": 1} },
   *                            {$unwind: "$attrs"},
   *                            {$group: {_id: "$_id.type", attrs: {$addToSet: "$attrs"}} },
   *                            {$sort: {_id: 1} }
   *                          ]
   *                })
   *
   * FIXME: in the future, we can interpret the collapse parameter at this layer. If collapse=true so we don't need attributes, the
   * following command can be used:
   *
   * db.runCommand({aggregate: "entities", pipeline: [ {$group: {_id: "$_id.type"} }]})
   *
   */

  // FIXME: unhardwire literals that depend on collection names and fields
  BSONObj result;
  BSONObj cmd = BSON("aggregate" << "entities" <<
                     "pipeline" << BSON_ARRAY(
                                              BSON("$project" << BSON("_id" << 1 << "attrs.name" << 1 << "attrs.type" << 1)) <<
                                              BSON("$unwind" << "$attrs") <<
                                              BSON("$group" << BSON("_id" << "$_id.type" << "attrs" << BSON("$addToSet" << "$attrs"))) <<
                                              BSON("$sort" << BSON("_id" << 1))
                                             )
                    );

  mongoSemTake(__FUNCTION__, "aggregation command");

  LM_T(LmtMongo, ("runCommand() in '%s' database: '%s'", composeDatabaseName(tenant).c_str(), cmd.toString().c_str()));
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
      reqSemGive(__FUNCTION__, "query types request");
      return SccOk;
  }

  /* Processing result to build response*/
  LM_T(LmtMongo, ("aggregation result: %s", result.toString().c_str()));

  std::vector<BSONElement> resultsArray = result.getField("result").Array();

  for (unsigned int ix = 0; ix < resultsArray.size(); ++ix)
  {
    /* Another strategy to implement pagination is to use the $skip and $limit operators in the
     * aggregation framework. However, doing so, we don't know the total number of results, which can
     * be needed in the case of details=on (using that approach, we need to do two queries: one to get
     * the cound and other to get the actual results with $skip and $limit, in the same "transaction" to
     * avoid incoherence between boths if some entity type is created or deleted in the process).
     *
     * However, considering that the number of types will be small compared with the number of entities,
     * the current approach seems to be ok
     */
    if (ix < offset)
    {
      continue;
    }
    if (ix >= offset + limit)
    {
      break;
    }

    BSONObj resultItem = resultsArray[ix].embeddedObject();
    TypeEntity* type = new TypeEntity(resultItem.getStringField("_id"));
    std::vector<BSONElement> attrsArray = resultItem.getField("attrs").Array();

    for (unsigned int jx = 0; jx < attrsArray.size(); ++jx)
    {
      BSONObj jAttr = attrsArray[jx].embeddedObject();
      ContextAttribute* ca = new ContextAttribute(jAttr.getStringField(ENT_ATTRS_NAME), jAttr.getStringField(ENT_ATTRS_TYPE));
      type->contextAttributeVector.push_back(ca);
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
  EntityTypesAttributesResponse*        responseP,
  const std::string&                    tenant,
  const std::vector<std::string>&       servicePathV,
  std::map<std::string, std::string>&   uriParams
)
{
  unsigned int offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
  unsigned int limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());
  std::string  detailsString  = uriParams[URI_PARAM_PAGINATION_DETAILS];
  bool         details        = (strcasecmp("on", detailsString.c_str()) == 0)? true : false;

  LM_T(LmtMongo, ("Query Types Attribute for <%s>", entityType.c_str()));
  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));

  reqSemTake(__FUNCTION__, "query types attributes request");

  DBClientBase* connection = getMongoConnection();

  /* Compose query based on this aggregation command:
   *
   * db.runCommand({aggregate: "entities",
   *                pipeline: [ {$match: { "_id.type": "TYPE" } },
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

  // FIXME: unhardwire literals that depend on collection names and fields
  BSONObj result;
  BSONObj cmd = BSON("aggregate" << "entities" <<
                     "pipeline" << BSON_ARRAY(
                                              BSON("$match" << BSON("_id.type" << entityType)) <<
                                              BSON("$project" << BSON("_id" << 1 << "attrs.name" << 1 << "attrs.type" << 1)) <<
                                              BSON("$unwind" << "$attrs") <<
                                              BSON("$group" << BSON("_id" << "$_id.type" << "attrs" << BSON("$addToSet" << "$attrs"))) <<
                                              BSON("$unwind" << "$attrs") <<
                                              BSON("$group" << BSON("_id" << "$attrs")) <<
                                              BSON("$sort" << BSON("_id.name" << 1 << "_id.type" << 1))
                                             )
                    );

  mongoSemTake(__FUNCTION__, "aggregation command");

  LM_T(LmtMongo, ("runCommand() in '%s' database: '%s'", composeDatabaseName(tenant).c_str(), cmd.toString().c_str()));
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
      reqSemGive(__FUNCTION__, "query types request");
      return SccOk;
  }

  /* Processing result to build response*/
  LM_T(LmtMongo, ("aggregation result: %s", result.toString().c_str()));

  std::vector<BSONElement> resultsArray = result.getField("result").Array();

  for (unsigned int ix = 0; ix < resultsArray.size(); ++ix)
  {

    /* See comment above in the other method regarding this strategy to implement pagination */
    if (ix < offset)
    {
      continue;
    }
    if (ix >= offset + limit)
    {
      break;
    }

    BSONObj resultItem = resultsArray[ix].embeddedObject().getField("_id").embeddedObject();
    ContextAttribute* ca = new ContextAttribute(resultItem.getStringField(ENT_ATTRS_NAME), resultItem.getStringField(ENT_ATTRS_TYPE));
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


