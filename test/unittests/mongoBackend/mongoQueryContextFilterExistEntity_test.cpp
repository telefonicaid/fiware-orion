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
* Author: Fermin Galan
*/
#include "unittests/unittest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "orionld/common/tenantList.h"     // tenant0

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/EntityId.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"
#include "ngsi/Scope.h"
#include "mongo/client/dbclient.h"



/* ****************************************************************************
*
* USING
*/
using mongo::DBClientBase;
using mongo::BSONObj;


extern void setMongoConnectionForUnitTest(DBClientBase* _connection);



/* ****************************************************************************
*
* Tests
*
* Note that these tests are similar in structure to the ones in DiscoverContextAvailability,
* due to both operations behaves quite similar regarding entities and attributes matching
*
* With pagination:
*
* - entityTypeWithoutFilter
* - entityTypeFilterExist
* - entityTypeFilterNotExists
*
* Note these tests are not "canonical" unit tests. Canon says that in this case we should have
* mocked MongoDB. Actually, we think is very much powerful to check that everything is ok at
* MongoDB layer.
*
*/

/* ****************************************************************************
*
* prepareDatabase -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabase(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "TA1" << "value" << "val1")));

  BSONObj en2 = BSON("_id" << BSON("id" << "E1") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "TA1" << "value" << "val1b")));


  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
}



/* ****************************************************************************
*
* entityTypeWithoutFilter -
*
*/
TEST(mongoQueryContextExistEntity, entityTypeWithoutFilter)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E1", RES_CER(1).entityId.id);
    EXPECT_EQ("", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val1b", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    utExit();
}


/* ****************************************************************************
*
* entityTypeFilterExist -
*
*/
TEST(mongoQueryContextExistEntity, entityTypeFilterExist)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "", "false");
  req.entityIdVector.push_back(&en);

  /* Define filter scope */
  Scope sc;
  sc.type  = SCOPE_FILTER_EXISTENCE;
  sc.value = SCOPE_VALUE_ENTITY_TYPE;
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}

/* ****************************************************************************
*
* entityTypeFilterNotExist -
*
*/
TEST(mongoQueryContextExistEntity, entityTypeFilterNotExist)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "", "false");
  req.entityIdVector.push_back(&en);

  /* Define filter scope */
  Scope sc;
  sc.type  = SCOPE_FILTER_EXISTENCE;
  sc.oper  = SCOPE_OPERATOR_NOT;
  sc.value = SCOPE_VALUE_ENTITY_TYPE;
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("val1b", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}
