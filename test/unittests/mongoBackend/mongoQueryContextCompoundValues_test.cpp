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
* Author: Fermin Galan
*/
#include "gtest/gtest.h"
#include "unittest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/EntityId.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"

#include "mongo/client/dbclient.h"


/* ****************************************************************************
*
* Tests
*
* - CompoundValue1
* - CompoundValue2
* - CompoundValue1PlusSimpleValue
* - CompoundValue2PlusSimpleValue
*
* Compound 1 is based in: [ 22, {x: [x1, x2], y: 3}, [z1, z2] ]
*
* Compound 2 is based in: { x: {x1: a, x2: b}, y: [ y1, y2 ] }
*
*/

/* ****************************************************************************
*
* prepareDatabase -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabase(void) {

  /* Set database */
  setupDatabase();

  DBClientConnection* connection = getMongoConnection();

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A1" <<
                             "type" << "TA1" <<
                             "value" << BSON_ARRAY("22" <<
                                                   BSON("x" << BSON_ARRAY("x1" << "x2") <<
                                                        "y" << "3") <<
                                                   BSON_ARRAY("z1" << "z2")
                                                   )
                             ) <<
                        BSON("name" << "A2" << "type" << "TA2" << "value" << "val2")
                        )
                    );

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T2") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A2" <<
                             "type" << "TA2" <<
                             "value" << BSON("x" << BSON("x1" << "a" << "x2" << "b") <<
                                             "y" << BSON_ARRAY("y1" << "y2")
                                             )
                             ) <<
                        BSON("name" << "A3" << "type" << "TA3" << "value" << "val3")
                        )
                    );

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);

}

/* ****************************************************************************
*
* CompoundValue1 -
*/
TEST(mongoQueryContextCompoundValuesRequest, CompoundValue1)
{

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    //EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* CompoundValue2 -
*/
TEST(mongoQueryContextCompoundValuesRequest, CompoundValue2)
{
    EXPECT_EQ(1, 0) << "to be implemented";
}

/* ****************************************************************************
*
* CompoundValue1PlusSimpleValue -
*/
TEST(mongoQueryContextCompoundValuesRequest, CompoundValue1PlusSimpleValue)
{
    EXPECT_EQ(1, 0) << "to be implemented";
}

/* ****************************************************************************
*
* CompoundValue2PlusSimpleValue -
*/
TEST(mongoQueryContextCompoundValuesRequest, CompoundValue2PlusSimpleValue)
{
    EXPECT_EQ(1, 0) << "to be implemented";
}
