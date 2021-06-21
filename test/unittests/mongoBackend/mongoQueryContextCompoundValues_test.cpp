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
#include "mongo/client/dbclient.h"
#include "gtest/gtest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "orionTypes/OrionValueType.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoConnectionPool.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/EntityId.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"

#include "unittests/unittest.h"



/* ****************************************************************************
*
* USING
*/
using mongo::DBClientBase;
using mongo::BSONObj;
using mongo::BSONNULL;
using mongo::OID;



/* ****************************************************************************
*
* Tests
*
* - CompoundValue1
* - CompoundValue2
* - CompoundValue1PlusSimpleValue
* - CompoundValue2PlusSimpleValue
*
* Compound 1 is based on: [ 22, {x: [x1, x2], y: 3}, [z1, z2] ]
* Compound 2 is based on: { x: {x1: a, x2: b}, y: [ y1, y2 ] }
*
* - CompoundValue1Native
* - CompoundValue2Native
* - CompoundValue1PlusSimpleValueNative
* - CompoundValue2PlusSimpleValueNative
*
* Compound 1 is based on: [ 22, {x: [x1, x2], y: 3, z: null}, [z1, false, null] ]
* Compound 2 is based on: { x: {x1: a, x2: true}, y: [ y1, y2 ], z: null }
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
                       "A1" << BSON("type" << "TA1" <<
                                    "value" << BSON_ARRAY("22" <<
                                                          BSON("x" << BSON_ARRAY("x1" << "x2") <<
                                                               "y" << "3") <<
                                                          BSON_ARRAY("z1" << "z2")))));

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T2") <<
                     "attrNames" << BSON_ARRAY("A2") <<
                     "attrs" << BSON(
                       "A2" << BSON("type" << "TA2" <<
                                    "value" << BSON("x" << BSON("x1" << "a" << "x2" << "b") <<
                                                    "y" << BSON_ARRAY("y1" << "y2")))));

  BSONObj en3 = BSON("_id" << BSON("id" << "E3" << "type" << "T3") <<
                     "attrNames" << BSON_ARRAY("A3" << "A3bis") <<
                     "attrs" << BSON(
                       "A3" << BSON("type" << "TA3" <<
                                    "value" << BSON_ARRAY("22" <<
                                                          BSON("x" << BSON_ARRAY("x1" << "x2") <<
                                                               "y" << "3") <<
                                                          BSON_ARRAY("z1" << "z2"))) <<
                       "A3bis" << BSON("type" << "TA3" << "value" << "val3")));

  BSONObj en4 = BSON("_id" << BSON("id" << "E4" << "type" << "T4") <<
                     "attrNames" << BSON_ARRAY("A4" << "A4bis") <<
                     "attrs" << BSON(
                       "A4" << BSON("type" << "TA4" <<
                                    "value" << BSON("x" << BSON("x1" << "a" << "x2" << "b") <<
                                                    "y" << BSON_ARRAY("y1" << "y2"))) <<
                       "A4bis" << BSON("type" << "TA4" << "value" << "val4")));

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en3);
  connection->insert(ENTITIES_COLL, en4);
}



/* ****************************************************************************
*
* prepareDatabaseNative -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabaseNative(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "TA1" <<
                                    "value" << BSON_ARRAY(22.0 <<
                                                          BSON("x" << BSON_ARRAY("x1" << "x2") <<
                                                               "y" << 3.0 <<
                                                               "z" << BSONNULL) <<
                                                          BSON_ARRAY("z1" << false << BSONNULL)))));

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T2") <<
                     "attrNames" << BSON_ARRAY("A2") <<
                     "attrs" << BSON(
                       "A2" << BSON("type" << "TA2" <<
                                    "value" << BSON("x" << BSON("x1" << "a" << "x2" << true) <<
                                                    "y" << BSON_ARRAY("y1" << "y2" << BSONNULL) <<
                                                    "z" << BSONNULL))));

  BSONObj en3 = BSON("_id" << BSON("id" << "E3" << "type" << "T3") <<
                     "attrNames" << BSON_ARRAY("A3" << "A3bis") <<
                     "attrs" << BSON(
                       "A3" << BSON("type" << "TA3" <<
                                    "value" << BSON_ARRAY(22.0 <<
                                                          BSON("x" << BSON_ARRAY("x1" << "x2") <<
                                                               "y" << 3.0 <<
                                                               "z" << BSONNULL) <<
                                                          BSON_ARRAY("z1" << false << BSONNULL))) <<
                       "A3bis" << BSON("type" << "TA3" << "value" << "val3")));

  BSONObj en4 = BSON("_id" << BSON("id" << "E4" << "type" << "T4") <<
                     "attrNames" << BSON_ARRAY("A4" << "A4bis") <<
                     "attrs" << BSON(
                       "A4" << BSON("type" << "TA4" <<
                                    "value" << BSON("x" << BSON("x1" << "a" << "x2" << true) <<
                                                    "y" << BSON_ARRAY("y1" << "y2") <<
                                                    "z" << BSONNULL)) <<
                       "A4bis" << BSON("type" << "TA4" << "value" << "val4")));

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en3);
  connection->insert(ENTITIES_COLL, en4);
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
    servicePathVector.clear();
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->valueType);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->valueType);
    EXPECT_EQ("22", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->valueType);
    EXPECT_EQ("x", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->name);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[0]->valueType);
    EXPECT_EQ("x1", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[1]->valueType);
    EXPECT_EQ("x2", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[1]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->valueType);
    EXPECT_EQ("y", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->name);
    EXPECT_EQ("3", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->stringValue);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->valueType);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[0]->valueType);
    EXPECT_EQ("z1", RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[1]->valueType);
    EXPECT_EQ("z2", RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[1]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

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
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E2", "T2", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E2", RES_CER(0).id);
    EXPECT_EQ("T2", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(orion::ValueTypeObject, RES_CER_ATTR(0, 0)->compoundValueP->valueType);
    EXPECT_EQ(orion::ValueTypeObject, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->valueType);
    EXPECT_EQ("x", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->name);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[0]->valueType);
    EXPECT_EQ("x1", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[0]->name);
    EXPECT_EQ("a", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[1]->valueType);
    EXPECT_EQ("x2", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[1]->name);
    EXPECT_EQ("b", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[1]->stringValue);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->valueType);
    EXPECT_EQ("y", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->name);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->valueType);
    EXPECT_EQ("y1", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->valueType);
    EXPECT_EQ("y2", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* CompoundValue1PlusSimpleValue -
*/
TEST(mongoQueryContextCompoundValuesRequest, CompoundValue1PlusSimpleValue)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E3", "T3", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).id);
    EXPECT_EQ("T3", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A3", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->valueType);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->valueType);
    EXPECT_EQ("22", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->valueType);
    EXPECT_EQ("x", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->name);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[0]->valueType);
    EXPECT_EQ("x1", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[1]->valueType);
    EXPECT_EQ("x2", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[1]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->valueType);
    EXPECT_EQ("y", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->name);
    EXPECT_EQ("3", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->stringValue);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->valueType);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[0]->valueType);
    EXPECT_EQ("z1", RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[1]->valueType);
    EXPECT_EQ("z2", RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[1]->stringValue);
    EXPECT_EQ("A3bis", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(0, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* CompoundValue2PlusSimpleValue -
*/
TEST(mongoQueryContextCompoundValuesRequest, CompoundValue2PlusSimpleValue)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E4", "T4", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E4", RES_CER(0).id);
    EXPECT_EQ("T4", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A4", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA4", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(orion::ValueTypeObject, RES_CER_ATTR(0, 0)->compoundValueP->valueType);
    EXPECT_EQ(orion::ValueTypeObject, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->valueType);
    EXPECT_EQ("x", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->name);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->valueType);
    EXPECT_EQ("x1", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[0]->name);
    EXPECT_EQ("a", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->valueType);
    EXPECT_EQ("x2", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[1]->name);
    EXPECT_EQ("b", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[1]->stringValue);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->valueType);
    EXPECT_EQ("y", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->name);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->valueType);
    EXPECT_EQ("y1", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->valueType);
    EXPECT_EQ("y2", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->stringValue);
    EXPECT_EQ("A4bis", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA4", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val4", RES_CER_ATTR(0, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* CompoundValue1Native -
*/
TEST(mongoQueryContextCompoundValuesRequest, CompoundValue1Native)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseNative();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->valueType);
    EXPECT_EQ(orion::ValueTypeNumber, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->valueType);
    EXPECT_EQ(22.0, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->numberValue);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->valueType);
    EXPECT_EQ("x", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->name);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[0]->valueType);
    EXPECT_EQ("x1", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[1]->valueType);
    EXPECT_EQ("x2", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[1]->stringValue);
    EXPECT_EQ(orion::ValueTypeNumber, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->valueType);
    EXPECT_EQ("y", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->name);
    EXPECT_EQ(3.0, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->numberValue);
    EXPECT_EQ(orion::ValueTypeNull, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[2]->valueType);
    EXPECT_EQ("z", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[2]->name);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->valueType);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[0]->valueType);
    EXPECT_EQ("z1", RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeBoolean, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[1]->valueType);
    EXPECT_FALSE(RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[1]->boolValue);
    EXPECT_EQ(orion::ValueTypeNull, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[2]->valueType);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* CompoundValue2Native -
*/
TEST(mongoQueryContextCompoundValuesRequest, CompoundValue2Native)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseNative();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E2", "T2", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E2", RES_CER(0).id);
    EXPECT_EQ("T2", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(orion::ValueTypeObject, RES_CER_ATTR(0, 0)->compoundValueP->valueType);
    EXPECT_EQ(orion::ValueTypeObject, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->valueType);
    EXPECT_EQ("x", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->name);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[0]->valueType);
    EXPECT_EQ("x1", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[0]->name);
    EXPECT_EQ("a", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeBoolean, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[1]->valueType);
    EXPECT_EQ("x2", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[1]->name);
    EXPECT_TRUE(RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[1]->boolValue);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->valueType);
    EXPECT_EQ("y", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->name);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->valueType);
    EXPECT_EQ("y1", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->valueType);
    EXPECT_EQ("y2", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->stringValue);
    EXPECT_EQ(orion::ValueTypeNull, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->valueType);
    EXPECT_EQ("z", RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->name);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* CompoundValue1PlusSimpleValueNative -
*/
TEST(mongoQueryContextCompoundValuesRequest, CompoundValue1PlusSimpleValueNative)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseNative();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E3", "T3", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).id);
    EXPECT_EQ("T3", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A3", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->valueType);
    EXPECT_EQ(orion::ValueTypeNumber, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->valueType);
    EXPECT_EQ(22.0, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->numberValue);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->valueType);
    EXPECT_EQ("x", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->name);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[0]->valueType);
    EXPECT_EQ("x1", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[1]->valueType);
    EXPECT_EQ("x2", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->childV[1]->stringValue);
    EXPECT_EQ(orion::ValueTypeNumber, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->valueType);
    EXPECT_EQ("y", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->name);
    EXPECT_EQ(3.0, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->numberValue);
    EXPECT_EQ(orion::ValueTypeNull, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[2]->valueType);
    EXPECT_EQ("z", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[2]->name);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->valueType);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[0]->valueType);
    EXPECT_EQ("z1", RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeBoolean, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[1]->valueType);
    EXPECT_FALSE(RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[1]->boolValue);
    EXPECT_EQ(orion::ValueTypeNull, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->childV[2]->valueType);
    EXPECT_EQ("A3bis", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(0, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* CompoundValue2PlusSimpleValueNative -
*/
TEST(mongoQueryContextCompoundValuesRequest, CompoundValue2PlusSimpleValueNative)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseNative();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E4", "T4", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E4", RES_CER(0).id);
    EXPECT_EQ("T4", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A4", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA4", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(orion::ValueTypeObject, RES_CER_ATTR(0, 0)->compoundValueP->valueType);
    EXPECT_EQ(orion::ValueTypeObject, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->valueType);
    EXPECT_EQ("x", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->name);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[0]->valueType);
    EXPECT_EQ("x1", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[0]->name);
    EXPECT_EQ("a", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeBoolean, RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[1]->valueType);
    EXPECT_EQ("x2", RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[1]->name);
    EXPECT_TRUE(RES_CER_ATTR(0, 0)->compoundValueP->childV[0]->childV[1]->boolValue);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->valueType);
    EXPECT_EQ("y", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->name);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->valueType);
    EXPECT_EQ("y1", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->valueType);
    EXPECT_EQ("y2", RES_CER_ATTR(0, 0)->compoundValueP->childV[1]->childV[1]->stringValue);
    EXPECT_EQ(orion::ValueTypeNull, RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->valueType);
    EXPECT_EQ("z", RES_CER_ATTR(0, 0)->compoundValueP->childV[2]->name);
    EXPECT_EQ("A4bis", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA4", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val4", RES_CER_ATTR(0, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}
