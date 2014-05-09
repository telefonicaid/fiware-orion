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
* fermin at tid dot es
*
* Author: Fermin Galan
*/
#include "gtest/gtest.h"
#include "testInit.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/EntityId.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"

#include "mongo/client/dbclient.h"

#include "commonMocks.h"

using ::testing::_;
using ::testing::Throw;

/* ****************************************************************************
*
* Tests
*
* Note that these tests are similar in structure to the ones in DiscoverContextAvailability,
* due to both operations behaves quite similar regarding entities and attributes matching
*
* With isPattern=false:
*
* - query1Ent0Attr
* - query1Ent1Attr
* - query1Ent1AttrSameName
* - queryNEnt0Attr
* - queryNEnt1AttrSingle
* - queryNEnt1AttrMulti
* - queryNEntNAttr
* - query1Ent0AttrFail
* - query1Ent1AttrFail
* - query1EntWA0AttrFail
* - query1EntWA1Attr
* - queryNEntWA0Attr
* - queryNEntWA1Attr
* - queryNoType
* - queryIdMetadata
* - queryCustomMetadata
*
* (N=2 without loss of generality)
* (WA = Without Attributes)
*
* With isPattern=true:
*
* - queryPattern0Attr
* - queryPattern1AttrSingle
* - queryPattern1AttrMulti
* - queryPatternNAttr
* - queryPatternFail
* - queryMixPatternAndNotPattern
* - queryNoTypePattern
* - queryIdMetadataPattern
* - queryCustomMetadataPattern
*
* Simulating fails in MongoDB connection:
*
* - mongoDbQueryFail
*
* Note these tests are not "canonical" unit tests. Canon says that in this case we should have
* mocked MongoDB. Actually, we think is very much powerfull to check that everything is ok at
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
static void prepareDatabase(void) {

  /* Set database */
  setupDatabase();

  DBClientConnection* connection = getMongoConnection();

  /* We create the following entities:
   *
   * - E1:
   *     A1: val1
   *     A2: val2
   * - E2
   *     A2: val2bis
   *     A3: val3
   * - E4
   *     (no attrs)
   * - E1*
   *     A1: val1bis
   *     A1*: val1bis2
   * - E1**
   *     A1: val1bis1
   *
   * (*) Means that entity/type is using same name but different type. This is included to check that type is
   *     taken into account.
   * (**)same name but without type
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A1" << "type" << "TA1" << "value" << "val1") <<
                        BSON("name" << "A2" << "type" << "TA2" << "value" << "val2")
                        )
                    );

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T2") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A2" << "type" << "TA2" << "value" << "val2bis") <<
                        BSON("name" << "A3" << "type" << "TA3" << "value" << "val3")
                        )
                    );

  BSONObj en4 = BSON("_id" << BSON("id" << "E4" << "type" << "T4") <<
                     "attrs" << BSONArray()
                    );

  BSONObj en5 = BSON("_id" << BSON("id" << "E1" << "type" << "T1bis") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A1" << "type" << "TA1" << "value" << "val1bis") <<
                        BSON("name" << "A1" << "type" << "TA1bis" << "value" << "val1bis2")
                        )
                    );

  BSONObj en6 = BSON("_id" << BSON("id" << "E1") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A1" << "type" << "TA1" << "value" << "val1bis1")
                        )
                    );

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en4);
  connection->insert(ENTITIES_COLL, en5);
  connection->insert(ENTITIES_COLL, en6);

}

/* ****************************************************************************
*
* prepareDatabasePatternTrue -
*
* This is a variant of populateDatabase function in which all entities have the same type,
* to ease test for isPattern=true cases
*/
static void prepareDatabasePatternTrue(void) {

  /* Set database */
  setupDatabase();

  DBClientConnection* connection = getMongoConnection();

  /* We create the following entities:
   *
   * - E1:
   *     A1: val1
   *     A2: val2
   * - E2
   *     A2: val2bis
   *     A3: val3
   * - E1*:
   *     A4: val4
   *     A5: val5
   * - E4
   *     (no attrs)
   * - E2**
   *     A2: val2bis1
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A1" << "type" << "TA1" << "value" << "val1") <<
                        BSON("name" << "A2" << "type" << "TA2" << "value" << "val2")
                        )
                    );

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A2" << "type" << "TA2" << "value" << "val2bis") <<
                        BSON("name" << "A3" << "type" << "TA3" << "value" << "val3")
                        )
                    );

  BSONObj en4 = BSON("_id" << BSON("id" << "E4" << "type" << "T") <<
                     "attrs" << BSONArray()
                    );

  BSONObj en5 = BSON("_id" << BSON("id" << "E1" << "type" << "Tbis") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A4" << "type" << "TA4" << "value" << "val4") <<
                        BSON("name" << "A5" << "type" << "TA5" << "value" << "val5")
                        )
                    );

  BSONObj en6 = BSON("_id" << BSON("id" << "E2") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A2" << "type" << "TA2" << "value" << "val2bis1")
                        )
                    );

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en4);
  connection->insert(ENTITIES_COLL, en5);
  connection->insert(ENTITIES_COLL, en6);

}

/* ****************************************************************************
*
* prepareDatabaseWithAttributeIds -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabaseWithAttributeIds(void) {

    /* Start with the base entities */
    prepareDatabase();

    /* Add some entities with metadata ID */

    DBClientConnection* connection = getMongoConnection();
    BSONObj en1 = BSON("_id" << BSON("id" << "E10" << "type" << "T") <<
                       "attrs" << BSON_ARRAY(
                          BSON("name" << "A1" << "type" << "TA1" << "value" << "A" << "id" << "ID1") <<
                          BSON("name" << "A1" << "type" << "TA1" << "value" << "B" << "id" << "ID2") <<
                          BSON("name" << "A1" << "type" << "TA11" << "value" << "C") <<
                          BSON("name" << "A2" << "type" << "TA2" << "value" << "D")
                          )
                      );

    BSONObj en2 = BSON("_id" << BSON("id" << "E11" << "type" << "T") <<
                       "attrs" << BSON_ARRAY(
                          BSON("name" << "A1" << "type" << "TA1" << "value" << "E" << "id" << "ID1") <<
                          BSON("name" << "A1" << "type" << "TA1" << "value" << "F" << "id" << "ID2") <<
                          BSON("name" << "A1" << "type" << "TA11" << "value" << "G") <<
                          BSON("name" << "A2" << "type" << "TA2" << "value" << "H")
                          )
                      );

    connection->insert(ENTITIES_COLL, en1);
    connection->insert(ENTITIES_COLL, en2);

}

/* ****************************************************************************
*
* prepareDatabaseWithAttributeCustomMetadta -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabaseWithCustomMetadata(void) {

    /* Start with the base entities */
    prepareDatabase();

    /* Add some entities with metadata ID */

    DBClientConnection* connection = getMongoConnection();
    BSONObj en1 = BSON("_id" << BSON("id" << "E10" << "type" << "T") <<
                       "attrs" << BSON_ARRAY(
                          BSON("name" << "A1" << "type" << "TA1" << "value" << "A" <<
                               "md" << BSON_ARRAY(BSON("name" << "MD1" << "type" << "TMD1" << "value" << "1") <<
                                                  BSON("name" << "MD2" << "type" << "TMD2" << "value" << "2")
                                                 )
                               ) <<
                          BSON("name" << "A1" << "type" << "TA11" << "value" << "B" <<
                               "md" << BSON_ARRAY(BSON("name" << "MD1" << "type" << "TMD1" << "value" << "3") <<
                                                  BSON("name" << "MD2" << "type" << "TMD2" << "value" << "4")
                                                 )
                               ) <<
                          BSON("name" << "A2" << "type" << "TA2" << "value" << "C" <<
                               "md" << BSON_ARRAY(BSON("name" << "MD1" << "type" << "TMD1" << "value" << "5") <<
                                                  BSON("name" << "MD2" << "type" << "TMD2" << "value" << "6")
                                                 )
                               )
                          )
                      );

    BSONObj en2 = BSON("_id" << BSON("id" << "E11" << "type" << "T") <<
                       "attrs" << BSON_ARRAY(
                           BSON("name" << "A1" << "type" << "TA1" << "value" << "D" <<
                                "md" << BSON_ARRAY(BSON("name" << "MD1" << "type" << "TMD1" << "value" << "7") <<
                                                   BSON("name" << "MD2" << "type" << "TMD2" << "value" << "8")
                                                  )
                                ) <<
                           BSON("name" << "A1" << "type" << "TA11" << "value" << "E" <<
                                "md" << BSON_ARRAY(BSON("name" << "MD1" << "type" << "TMD1" << "value" << "9") <<
                                                   BSON("name" << "MD2" << "type" << "TMD2" << "value" << "10")
                                                  )
                                ) <<
                           BSON("name" << "A2" << "type" << "TA2" << "value" << "F" <<
                                "md" << BSON_ARRAY(BSON("name" << "MD1" << "type" << "TMD1" << "value" << "11") <<
                                                   BSON("name" << "MD2" << "type" << "TMD2" << "value" << "12")
                                                  )
                                )
                          )
                      );

    connection->insert(ENTITIES_COLL, en1);
    connection->insert(ENTITIES_COLL, en2);

}

/* ****************************************************************************
*
* query1Ent0Attr -
*
* Discover:  E1 - no attrs
* Result:    E1 - (A1, A2)
*
* This test also checks that querying for type (E1) doesn't match with no-typed
* entities (E1** is not returned)
*
*/
TEST(mongoQueryContextRequest, query1Ent0Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* query1Ent1Attr -
*
* Discover:  E1 - A1
* Result:    E1 - A1
*/
TEST(mongoQueryContextRequest, query1Ent1Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

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
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* query1Ent1AttrSameName -
*
* Discover:  E1* - A1
* Result:    E1* - (A1, A1*)  [same name but different type]
*/
TEST(mongoQueryContextRequest, query1Ent1AttrSameName)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1bis", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

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
    EXPECT_EQ("T1bis", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1bis", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA1bis", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val1bis2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNEnt0Attr -
*
* Discover:  (E1, E2) -no attrs
* Result:    E1 - (A1, A2)
*            E2 - (A3, A4)
*/
TEST(mongoQueryContextRequest, queryNEnt0Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E2", "T2", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNEnt1AttrSingle -
*
* Discover:  (E1, E2) - A1
* Result:    E1 - A1
*/
TEST(mongoQueryContextRequest, queryNEnt1AttrSingle)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E2", "T2", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.attributeList.push_back("A1");

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
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNEnt1AttrMulti -
*
* Discover:  (E1, E2) - A2
* Result:    E1 - A2
*            E2 - A2
*/
TEST(mongoQueryContextRequest, queryNEnt1AttrMulti)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;  

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E2", "T2", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.attributeList.push_back("A2");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNEntNAttr -
*
* Discover:  (E1, E2) - (A1, A3)
* Result:    E1 - A1
*            E2 - A3
*/
TEST(mongoQueryContextRequest, queryNEntNAttr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E2", "T2", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.attributeList.push_back("A1");
    req.attributeList.push_back("A3");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A3", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* query1Ent0AttrFail -
*
* Discover:  E3 - no attrs
* Result:    none
*/
TEST(mongoQueryContextRequest, query1Ent0AttrFail)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E3", "T3", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());
    EXPECT_EQ(0,res.contextElementResponseVector.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* query1Ent1AttrFail -
*
* Discover:  E1 - A3
* Result:    none
*/
TEST(mongoQueryContextRequest, query1Ent1AttrFail)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A3");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());
    EXPECT_EQ(0,res.contextElementResponseVector.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}


/* ****************************************************************************
*
* query1EntWA0AttrFail -
*
* Discover:  E4 - A1
* Result:    none
*/
TEST(mongoQueryContextRequest, query1EntWA0AttrFail)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E4", "T4", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());
    EXPECT_EQ(0,res.contextElementResponseVector.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* query1EntWA1Attr -
*
* Discover:  E4 - none
* Result:    E4 - none
*/
TEST(mongoQueryContextRequest, query1EntWA1Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E4", "T4", "false");
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
    EXPECT_EQ("E4", RES_CER(0).entityId.id);
    EXPECT_EQ("T4", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    EXPECT_EQ(0, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNEntWA0Attr -
*
* Discover:  (E1, E4) - none
* Result:    E1 - A1, A2
*            E4 - none
*/
TEST(mongoQueryContextRequest, queryNEntWA0Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E4", "T4", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E4", RES_CER(1).entityId.id);
    EXPECT_EQ("T4", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    EXPECT_EQ(0, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNEntWA1Attr -
*
* Discover:  (E1, E4) - A1
* Result:    E1 - A1
*/
TEST(mongoQueryContextRequest, queryNEntWA1Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E4", "T4", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.attributeList.push_back("A1");

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
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNoType -
*
* Discover:  E1
* Result:    E1   - A1
*            E1*  - A1, A1*
*            E1** - A1
*
* Note that this case checks matching of no-type in the query for both the case in
* which the returned entity has type (E1 and E1*) and the case in which it has no type (E1**)
*
*/
TEST(mongoQueryContextRequest, queryNoType)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response #2 */
    EXPECT_EQ("E1", RES_CER(1).entityId.id);
    EXPECT_EQ("T1bis", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val1bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA1bis", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val1bis2", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response #3 */
    EXPECT_EQ("E1", RES_CER(2).entityId.id);
    EXPECT_EQ(0, RES_CER(2).entityId.type.size());
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ("val1bis1", RES_CER_ATTR(2, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryIdMetadata -
*
*/
TEST(mongoQueryContextRequest, queryIdMetadata)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E10", "T", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(3, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("A", RES_CER_ATTR(0, 0)->value);
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("B", RES_CER_ATTR(0, 1)->value);
    ASSERT_EQ(1, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 1)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->metadataVector.get(0)->type);
    EXPECT_EQ("ID2", RES_CER_ATTR(0, 1)->metadataVector.get(0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 2)->name);
    EXPECT_EQ("TA11", RES_CER_ATTR(0, 2)->type);
    EXPECT_EQ("C", RES_CER_ATTR(0, 2)->value);
    ASSERT_EQ(0, RES_CER_ATTR(0, 2)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryCustomMetadata -
*
*/
TEST(mongoQueryContextRequest, queryCustomMetadata)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabaseWithCustomMetadata();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E10", "T", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("A", RES_CER_ATTR(0, 0)->value);
    ASSERT_EQ(2, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->type);
    EXPECT_EQ("2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA11", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("B", RES_CER_ATTR(0, 1)->value);
    ASSERT_EQ(2, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 1)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 1)->metadataVector.get(0)->type);
    EXPECT_EQ("3", RES_CER_ATTR(0, 1)->metadataVector.get(0)->value);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 1)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 1)->metadataVector.get(1)->type);
    EXPECT_EQ("4", RES_CER_ATTR(0, 1)->metadataVector.get(1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}


/* ****************************************************************************
*
* queryPattern0Attr -
*
* Discover:  E[1-2] - none
* Result:    E1 - (A1, A2)
*            E2 - (A2, A3)
*
* This test also checks that querying for type (E[1-2]) doesn't match with no-typed
* entities (E1** is not returned)
*
*/
TEST(mongoQueryContextRequest, queryPattern0Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "T", "true");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryPattern1AttrSingle -
*
* Discover:  E[1-2] - A1
* Result:    E1 - A1
*/
TEST(mongoQueryContextRequest, queryPattern1AttrSingle)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

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
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryPattern1AttrMulti -
*
* Discover:  E[1-2] - A2
* Result:    E1 - A2
*            E2 - A2
*/
TEST(mongoQueryContextRequest, queryPattern1AttrMulti)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A2");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);    

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryPatternNAttr -
*
* Discover:  E[1-2] - (A1, A2)
* Result:    E1 - (A1, A2)
*            E2 - A2
*/
TEST(mongoQueryContextRequest, queryPatternNAttr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");
    req.attributeList.push_back("A2");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryPatternFail -
*
* Discover:  R.* - none
* Result:    none
*/
TEST(mongoQueryContextRequest, queryPatternFail)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("R.*", "T", "true");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());
    EXPECT_EQ(0,res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryMixPatternAndNotPattern -
*
* Discover:  (E[1-2]. E4) - none
* Result:    E1 - (A1, A2)
*            E2 - (A2 ,A3)
*            E4 - none
*/
TEST(mongoQueryContextRequest, queryMixPatternAndNotPattern)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E[1-2]", "T", "true");
    EntityId en2("E4", "T", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E4", RES_CER(2).entityId.id);
    EXPECT_EQ("T", RES_CER(2).entityId.type);
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    EXPECT_EQ(0, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNoTypePattern -
*
* Discover:  E[1-2]
* Result:    E1   - A1, A2
*            E2   - A2 ,A3
*            E1*  - A4, A5
*            E2** - A2
*
* Note that this case checks matching of no-type in the query for both the case in
* which the returned entity has type (E1, E2 and E1*) and the case in which it has no type (E2**)
*
*/
TEST(mongoQueryContextRequest, queryNoTypePattern)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "", "true");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(4, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E1", RES_CER(2).entityId.id);
    EXPECT_EQ("Tbis", RES_CER(2).entityId.type);
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ("A4", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA4", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ("val4", RES_CER_ATTR(2, 0)->value);
    EXPECT_EQ("A5", RES_CER_ATTR(2, 1)->name);
    EXPECT_EQ("TA5", RES_CER_ATTR(2, 1)->type);
    EXPECT_EQ("val5", RES_CER_ATTR(2, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Context Element response # 4 */
    EXPECT_EQ("E2", RES_CER(3).entityId.id);
    EXPECT_EQ(0, RES_CER(3).entityId.type.size());
    EXPECT_EQ("false", RES_CER(3).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(3).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(3, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(3, 0)->type);
    EXPECT_EQ("val2bis1", RES_CER_ATTR(3, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
    EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryIdMetadataPattern -
*
*/
TEST(mongoQueryContextRequest, queryIdMetadataPattern)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1[0-1]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(3, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("A", RES_CER_ATTR(0, 0)->value);
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("B", RES_CER_ATTR(0, 1)->value);
    ASSERT_EQ(1, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 1)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->metadataVector.get(0)->type);
    EXPECT_EQ("ID2", RES_CER_ATTR(0, 1)->metadataVector.get(0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 2)->name);
    EXPECT_EQ("TA11", RES_CER_ATTR(0, 2)->type);
    EXPECT_EQ("C", RES_CER_ATTR(0, 2)->value);
    ASSERT_EQ(0, RES_CER_ATTR(0, 2)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E11", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(3, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("E", RES_CER_ATTR(1, 0)->value);
    ASSERT_EQ(1, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(1, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(1, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(1, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("F", RES_CER_ATTR(1, 1)->value);
    ASSERT_EQ(1, RES_CER_ATTR(1, 1)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(1, 1)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(1, 1)->metadataVector.get(0)->type);
    EXPECT_EQ("ID2", RES_CER_ATTR(1, 1)->metadataVector.get(0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(1, 2)->name);
    EXPECT_EQ("TA11", RES_CER_ATTR(1, 2)->type);
    EXPECT_EQ("G", RES_CER_ATTR(1, 2)->value);
    ASSERT_EQ(0, RES_CER_ATTR(1, 2)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryCustomMetadataPattern -
*
*/
TEST(mongoQueryContextRequest, queryCustomMetadataPattern)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabaseWithCustomMetadata();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1[0-1]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("A", RES_CER_ATTR(0, 0)->value);
    ASSERT_EQ(2, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->type);
    EXPECT_EQ("2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA11", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("B", RES_CER_ATTR(0, 1)->value);
    ASSERT_EQ(2, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 1)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 1)->metadataVector.get(0)->type);
    EXPECT_EQ("3", RES_CER_ATTR(0, 1)->metadataVector.get(0)->value);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 1)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 1)->metadataVector.get(1)->type);
    EXPECT_EQ("4", RES_CER_ATTR(0, 1)->metadataVector.get(1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E11", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("D", RES_CER_ATTR(1, 0)->value);
    ASSERT_EQ(2, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(1, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(1, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("7", RES_CER_ATTR(1, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("MD2", RES_CER_ATTR(1, 0)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(1, 0)->metadataVector.get(1)->type);
    EXPECT_EQ("8", RES_CER_ATTR(1, 0)->metadataVector.get(1)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA11", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("E", RES_CER_ATTR(1, 1)->value);
    ASSERT_EQ(2, RES_CER_ATTR(1, 1)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(1, 1)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(1, 1)->metadataVector.get(0)->type);
    EXPECT_EQ("9", RES_CER_ATTR(1, 1)->metadataVector.get(0)->value);
    EXPECT_EQ("MD2", RES_CER_ATTR(1, 1)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(1, 1)->metadataVector.get(1)->type);
    EXPECT_EQ("10", RES_CER_ATTR(1, 1)->metadataVector.get(1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* mongoDbQueryFail -
*
*/
TEST(mongoQueryContextRequest, mongoDbQueryFail)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare mock */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, _query(_,_,_,_,_,_,_))
            .WillByDefault(Throw(e));

    /* Set MongoDB connection */
    mongoConnect(connectionMock);    

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Internal Server Error", res.errorCode.reasonPhrase);
    EXPECT_EQ("collection: unittest.entities - "
              "query(): { $or: [ { _id: { $in: [ { id: \"E1\", type: \"T1\" } ] } } ] } - "
              "exception: boom!!", res.errorCode.details);
    EXPECT_EQ(0,res.contextElementResponseVector.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();    

    /* Release mock */
    delete connectionMock;
}
