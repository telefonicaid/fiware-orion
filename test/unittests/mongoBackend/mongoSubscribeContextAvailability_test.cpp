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
* iot_support at tid dot es
*
* Author: Fermin Galan
*/
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoSubscribeContextAvailability.h"
#include "ngsi9/SubscribeContextAvailabilityRequest.h"
#include "ngsi9/SubscribeContextAvailabilityResponse.h"
#include "ngsi/EntityId.h"

#include "unittests/unittest.h"
#include "unittests/testInit.h"



/* ****************************************************************************
*
* USING
*/
using mongo::DBClientBase;
using mongo::BSONObj;
using mongo::BSONArray;
using mongo::BSONElement;
using mongo::OID;
using mongo::DBException;
using mongo::BSONObjBuilder;
using ::testing::Return;
using ::testing::Throw;
using ::testing::_;


extern void setMongoConnectionForUnitTest(DBClientBase* _connection);



/* ****************************************************************************
*
* The first set of tests is just to test correct inserting in collection. There
* is no match in entities/attributes.
*
* - Ent1_Attr0_noPattern
* - Ent1_Attr0_noPattern_JSON
* - Ent1_AttrN_noPattern
* - EntN_Attr0_noPattern
* - EntN_AttrN_noPattern
* - Ent1_Attr0_pattern
*
* Match with isPattern=false (very close to the same tests for discoverContextAvailability):
*
* - noPatternAttrsAll
* - noPatternAttrsAll_JSON
* - noPatternAttrOneSingle
* - noPatternAttrOneMulti
* - noPatternAttrsSubset
* - noPatternSeveralCREs
* - noPatternSeveralRegistrations
* - noPatternMultiEntity
* - noPatternMultiAttr
* - noPatternMultiEntityAttrs
* - noPatternNoType
*
* With isPattern=true (very close to the same tests for discoverContextAvailability):
*
* - pattern0Attr
* - pattern1AttrSingle
* - pattern1AttrMulti
* - patternNAttr
* - patternNoType
* - mixPatternAndNotPattern
*
* In addition, we include a test to check default duration
*
* - defaultDuration
*
* Simulating fails in MongoDB connection. Note that we only test direct use of MongoDB connection
* (i.e. connection-> uses in mongoSubscribeContextAvailability module), indirect uses in called functions
* outside the module are not tested here.
*
* - mongoDbInsertFail
*
* Note that test related with triggering notifications due to registerContext on entities/attributes
* covered by the subscription are not included in this test suite, with deals exclusively with
* subscribeContextAvailability processing·
*
*/

/* ****************************************************************************
*
* prepareDatabase -
*/
static void prepareDatabase(void)
{
    /* Set database */
    setupDatabase();

    DBClientBase* connection = getMongoConnection();

    /* We create the following registrations:
     *
     * - Reg1: CR: (E1,E2,E3) (A1,A2,A3)
     *         CR: (E1) (A1,A4)
     * - Reg2: CR: (E2) (A2, A3)
     * - Reg3: CR: (E1*) (A1*)
     * - Reg4: CR: (E1**) A1
     *
     * (*) same name but different types. This is included to check that type is taken into account,
     *     so Reg3 is not returned never. You can try to change types in Reg3 to make them equal
     *     to the ones in Reg1 and Reg2 and check that some tests are failing.
     * (**)same name but without type
     */

    BSONObj cr1 = BSON("providingApplication" << "http://cr1.com" <<
                       "entities" << BSON_ARRAY(
                         BSON("id" << "E1" << "type" << "T1") <<
                         BSON("id" << "E2" << "type" << "T2") <<
                         BSON("id" << "E3" << "type" << "T3")) <<
                       "attrs" << BSON_ARRAY(
                         BSON("name" << "A1" << "type" << "TA1") <<
                         BSON("name" << "A2" << "type" << "TA2") <<
                         BSON("name" << "A3" << "type" << "TA3")));

    BSONObj cr2 = BSON("providingApplication" << "http://cr2.com" <<
                       "entities" << BSON_ARRAY(
                         BSON("id" << "E1" << "type" << "T1")) <<
                       "attrs" << BSON_ARRAY(
                         BSON("name" << "A1" << "type" << "TA1") <<
                         BSON("name" << "A4" << "type" << "TA4")));

    BSONObj cr3 = BSON("providingApplication" << "http://cr3.com" <<
                       "entities" << BSON_ARRAY(
                         BSON("id" << "E2" << "type" << "T2")) <<
                       "attrs" << BSON_ARRAY(
                         BSON("name" << "A2" << "type" << "TA2") <<
                         BSON("name" << "A3" << "type" << "TA3")));

    BSONObj cr4 = BSON("providingApplication" << "http://cr4.com" <<
                       "entities" << BSON_ARRAY(
                         BSON("id" << "E1" << "type" << "T1bis")) <<
                       "attrs" << BSON_ARRAY(
                         BSON("name" << "A1" << "type" << "TA1bis")));

    BSONObj cr5 = BSON("providingApplication" << "http://cr5.com" <<
                       "entities" << BSON_ARRAY(
                         BSON("id" << "E1")) <<
                       "attrs" << BSON_ARRAY(
                         BSON("name" << "A1" << "type" << "TA1")));

    /* 1879048191 corresponds to year 2029 so we avoid any expiration problem in the next 16 years :) */
    BSONObj reg1 = BSON(
                "_id" << OID("51307b66f481db11bf860001") <<
                "expiration" << 1879048191 <<
                "contextRegistration" << BSON_ARRAY(cr1 << cr2));

    BSONObj reg2 = BSON(
                "_id" << OID("51307b66f481db11bf860002") <<
                "expiration" << 1879048191 <<
                "contextRegistration" << BSON_ARRAY(cr3));

    BSONObj reg3 = BSON(
                "_id" << OID("51307b66f481db11bf860003") <<
                "expiration" << 1879048191 <<
                "contextRegistration" << BSON_ARRAY(cr4));

    BSONObj reg4 = BSON(
                "_id" << OID("51307b66f481db11bf860004") <<
                "expiration" << 1879048191 <<
                "contextRegistration" << BSON_ARRAY(cr5));

    connection->insert(REGISTRATIONS_COLL, reg1);
    connection->insert(REGISTRATIONS_COLL, reg2);
    connection->insert(REGISTRATIONS_COLL, reg3);
    connection->insert(REGISTRATIONS_COLL, reg4);
}



/* ****************************************************************************
*
* prepareDatabasePatternTrue -
*
* This is a variant of populateDatabase function in which all entities have the same type,
* to ease test for isPattern=true cases
*
*/
static void prepareDatabasePatternTrue(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following registrations:
   *
   * - Reg1: CR: (E1,E2,E3) (A1,A2,A3)
   *         CR: (E1)       (A1,A4)
   * - Reg2: CR: (E2)       (A2, A3)
   * - Reg3: CR: (E2*)      (A2*)
   * - Reg4: CR: (E3**)     (A2)
   *
   * (*) same name but different types. This is included to check that type is taken into account,
   *     so Reg3 is not returned never (except patternNoType). You can try to change types in Reg3
   *     to make them equal to the ones in Reg1 and Reg2 and check that some tests are failing.
   * (**)same name but without type
   */

  BSONObj cr1 = BSON("providingApplication" << "http://cr1.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T") <<
                       BSON("id" << "E2" << "type" << "T") <<
                       BSON("id" << "E3" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1") <<
                       BSON("name" << "A2" << "type" << "TA2") <<
                       BSON("name" << "A3" << "type" << "TA3")));

  BSONObj cr2 = BSON("providingApplication" << "http://cr2.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E1" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                         BSON("name" << "A1" << "type" << "TA1") <<
                         BSON("name" << "A4" << "type" << "TA4")));

  BSONObj cr3 = BSON("providingApplication" << "http://cr3.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E2" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "TA2") <<
                       BSON("name" << "A3" << "type" << "TA3")));

  BSONObj cr4 = BSON("providingApplication" << "http://cr4.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E2" << "type" << "Tbis")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "TA2bis")));

  BSONObj cr5 = BSON("providingApplication" << "http://cr5.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E3")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "TA2")));

  /* 1879048191 corresponds to year 2029 so we avoid any expiration problem in the next 16 years :) */
  BSONObj reg1 = BSON(
              "_id" << "ff37" <<
              "expiration" << 1879048191 <<
              "subscriptions" << BSONArray() <<
              "contextRegistration" << BSON_ARRAY(cr1 << cr2));

  BSONObj reg2 = BSON(
              "_id" << "ee48" <<
              "expiration" << 1879048191 <<
              "subscriptions" << BSONArray() <<
              "contextRegistration" << BSON_ARRAY(cr3));

  BSONObj reg3 = BSON(
              "_id" << "ee00" <<
              "expiration" << 1879048191 <<
              "subscriptions" << BSONArray() <<
              "contextRegistration" << BSON_ARRAY(cr4));

  BSONObj reg4 = BSON(
              "_id" << "ff00" <<
              "expiration" << 1879048191 <<
              "subscriptions" << BSONArray() <<
              "contextRegistration" << BSON_ARRAY(cr5));

  connection->insert(REGISTRATIONS_COLL, reg1);
  connection->insert(REGISTRATIONS_COLL, reg2);
  connection->insert(REGISTRATIONS_COLL, reg3);
  connection->insert(REGISTRATIONS_COLL, reg4);
}



/* ****************************************************************************
*
* Ent1_Attr0_noPattern -
*/
TEST(mongoSubscribeContextAvailability, Ent1_Attr0_noPattern)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_, _, _, _, _)).Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E5", "T5", "false");
    req.entityIdVector.push_back(&en);
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E5", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T5", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* Ent1_Attr0_noPattern_JSON -
*/
TEST(mongoSubscribeContextAvailability, Ent1_Attr0_noPattern_JSON)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_, _, _, _, _)).Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E5", "T5", "false");
    req.entityIdVector.push_back(&en);
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E5", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T5", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* Ent1_AttrN_noPattern -
*/
TEST(mongoSubscribeContextAvailability, Ent1_AttrN_noPattern)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_, _, _, _, _)).Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E5", "T5", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");
    req.attributeList.push_back("A2");
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E5", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T5", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* EntN_Attr0_noPattern -
*/
TEST(mongoSubscribeContextAvailability, EntN_Attr0_noPattern)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_, _, _, _, _)).Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E5", "T5", "false");
    EntityId en2("E6", "T6", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    BSONObj ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E5", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T5", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));
    EXPECT_STREQ("E6", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T6", C_STR_FIELD(ent1, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent1, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* EntN_AttrN_noPattern -
*/
TEST(mongoSubscribeContextAvailability, EntN_AttrN_noPattern)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_, _, _, _, _)).Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E5", "T5", "false");
    EntityId en2("E6", "T6", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.attributeList.push_back("A1");
    req.attributeList.push_back("A2");
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    BSONObj ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E5", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T5", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));
    EXPECT_STREQ("E6", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T6", C_STR_FIELD(ent1, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent1, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* Ent1_Attr0_pattern -
*
* Discover:  R.* - none
* Result:    none
*/
TEST(mongoSubscribeContextAvailability, Ent1_Attr0_pattern)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_, _, _, _, _)).Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("R.*", "T", "true");
    req.entityIdVector.push_back(&en);
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("R.*", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternAttrsAll -
*
* Discover:  E3 -no attrs
* Result:    E3 - (A1, A2, A3) - http://cr1.com
*/
TEST(mongoSubscribeContextAvailability, noPatternAttrsAll)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn("E3", "T3", "false");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationAttribute cra3("A3", "TA3");
    ContextRegistrationResponse crr;
    crr.contextRegistration.entityIdVector.push_back(&mockEn);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr.contextRegistration.providingApplication.set("http://cr1.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E3", "T3", "false");
    req.entityIdVector.push_back(&en);
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E3", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternAttrsAll_JSON -
*
* Discover:  E3 -no attrs
* Result:    E3 - (A1, A2, A3) - http://cr1.com
*/
TEST(mongoSubscribeContextAvailability, noPatternAttrsAll_JSON)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn("E3", "T3", "false");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationAttribute cra3("A3", "TA3");
    ContextRegistrationResponse crr;
    crr.contextRegistration.entityIdVector.push_back(&mockEn);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr.contextRegistration.providingApplication.set("http://cr1.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E3", "T3", "false");
    req.entityIdVector.push_back(&en);
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E3", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternAttrOneSingle -
*
* Discover:  E1 - A4
* Result:    E1 - A4 - http://cr2.com
*/
TEST(mongoSubscribeContextAvailability, noPatternAttrOneSingle)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn("E1", "T1", "false");
    ContextRegistrationAttribute cra("A4", "TA4");
    ContextRegistrationResponse crr;
    crr.contextRegistration.entityIdVector.push_back(&mockEn);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra);
    crr.contextRegistration.providingApplication.set("http://cr2.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A4");
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    EXPECT_EQ("A4", attrs[0].String());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternAttrOneMulti -
*
* Discover:  E1 - A1
* Result:    E1 - A1 - http://cr1.com
*            E1 - A1 - http://cr2.com
*
* This test also checks that discovering for type (E1) doesn't match with no-typed
* entities (E1** - cr5 is not returned)
*
*/
TEST(mongoSubscribeContextAvailability, noPatternAttrOneMulti)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn("E1", "T1", "false");
    ContextRegistrationAttribute cra("A1", "TA1");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&mockEn);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra);
    crr1.contextRegistration.providingApplication.set("http://cr1.com");
    crr2.contextRegistration.entityIdVector.push_back(&mockEn);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra);
    crr2.contextRegistration.providingApplication.set("http://cr2.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr1);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}


/* ****************************************************************************
*
* noPatternAttrsSubset -
*
* Discover:  E3 - (A1, A2)
* Result:    E3 - (A1, A2) - http://cr1.com
*/
TEST(mongoSubscribeContextAvailability, noPatternAttrsSubset)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn("E3", "T3", "false");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationResponse crr;
    crr.contextRegistration.entityIdVector.push_back(&mockEn);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr.contextRegistration.providingApplication.set("http://cr1.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E3", "T3", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");
    req.attributeList.push_back("A2");
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E3", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternSeveralCREs -
*
* Discover:  E1 - no attrs
* Result:    E1 - (A1, A2, A3) - http://cr1.com
*            E1 - (A1, A4)     - http://cr2.com
*/
TEST(mongoSubscribeContextAvailability, noPatternSeveralCREs)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn("E1", "T1", "false");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationAttribute cra3("A3", "TA3");
    ContextRegistrationAttribute cra4("A4", "TA4");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&mockEn);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr1.contextRegistration.providingApplication.set("http://cr1.com");
    crr2.contextRegistration.entityIdVector.push_back(&mockEn);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra4);
    crr2.contextRegistration.providingApplication.set("http://cr2.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr1);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(0, attrs.size());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternSeveralRegistrations -
*
* Discover:  E2 - no attrs
* Result:    E2 - (A1, A2, A3) - http://cr1.com
*            E2 - (A2, A3)     - http://cr3.com
*/
TEST(mongoSubscribeContextAvailability, noPatternSeveralRegistrations)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn("E2", "T2", "false");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationAttribute cra3("A3", "TA3");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&mockEn);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr1.contextRegistration.providingApplication.set("http://cr1.com");
    crr2.contextRegistration.entityIdVector.push_back(&mockEn);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr2.contextRegistration.providingApplication.set("http://cr3.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr1);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E2", "T2", "false");
    req.entityIdVector.push_back(&en);
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E2", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(0, attrs.size());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternMultiEntity -
*
* Discover:  (E1, E2) - no attrs
* Result:    (E1, E2) - (A1, A2, A3) - http://cr1.com
*            E1       - (A1, A4)     - http://cr2.com
*            E2       - (A2, A3)     - http://cr3.com
*/
TEST(mongoSubscribeContextAvailability, noPatternMultiEntity)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn1("E1", "T1", "false");
    EntityId mockEn2("E2", "T2", "false");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationAttribute cra3("A3", "TA3");
    ContextRegistrationAttribute cra4("A4", "TA4");
    ContextRegistrationResponse crr1, crr2, crr3;
    crr1.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr1.contextRegistration.entityIdVector.push_back(&mockEn2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr1.contextRegistration.providingApplication.set("http://cr1.com");
    crr2.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra4);
    crr2.contextRegistration.providingApplication.set("http://cr2.com");
    crr3.contextRegistration.entityIdVector.push_back(&mockEn2);
    crr3.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr3.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr3.contextRegistration.providingApplication.set("http://cr3.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr1);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr2);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr3);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E2", "T2", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    BSONObj ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent1, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent1, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(0, attrs.size());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternMultiAttr -
*
* Discover:  E1 - (A3, A4, A5)
* Result:    E1 - A3 - http://cr1.com
*            E1 - A4 - http://cr2.com
*/
TEST(mongoSubscribeContextAvailability, noPatternMultiAttr)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn("E1", "T1", "false");
    ContextRegistrationAttribute cra1("A3", "TA3");
    ContextRegistrationAttribute cra2("A4", "TA4");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&mockEn);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.providingApplication.set("http://cr1.com");
    crr2.contextRegistration.entityIdVector.push_back(&mockEn);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr2.contextRegistration.providingApplication.set("http://cr2.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr1);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A3");
    req.attributeList.push_back("A4");
    req.attributeList.push_back("A5");
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    EXPECT_EQ("A3", attrs[0].String());
    EXPECT_EQ("A4", attrs[1].String());
    EXPECT_EQ("A5", attrs[2].String());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternMultiEntityAttrs -
*
* Discover:  (E1, E2) - (A3, A4, A5)
* Result:    (E1, E2) - A3 - http://cr1.com
*            E1       - A4 - http://cr2.com
*            E2       - A3 - http://cr3.com
*/
TEST(mongoSubscribeContextAvailability, noPatternMultiEntityAttrs)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn1("E1", "T1", "false");
    EntityId mockEn2("E2", "T2", "false");
    ContextRegistrationAttribute cra1("A3", "TA3");
    ContextRegistrationAttribute cra2("A4", "TA4");
    ContextRegistrationResponse crr1, crr2, crr3;
    crr1.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr1.contextRegistration.entityIdVector.push_back(&mockEn2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.providingApplication.set("http://cr1.com");
    crr2.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr2.contextRegistration.providingApplication.set("http://cr2.com");
    crr3.contextRegistration.entityIdVector.push_back(&mockEn2);
    crr3.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr3.contextRegistration.providingApplication.set("http://cr3.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr1);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr2);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr3);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E2", "T2", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.attributeList.push_back("A3");
    req.attributeList.push_back("A4");
    req.attributeList.push_back("A5");
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    BSONObj ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent1, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent1, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    EXPECT_EQ("A3", attrs[0].String());
    EXPECT_EQ("A4", attrs[1].String());
    EXPECT_EQ("A5", attrs[2].String());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternNoType -
*
* Discover:  E1** - A1
* Result:    E1   - A1 - http://cr1.com
*            E1   - A1 - http://cr2.com
*            E1*  - A1*- http://cr4.com
*            E1** - A1 - http://cr5.com
*
* Note that this case checks matching of no-type in the discover for both the case in
* which the returned CR has type (cr1, cr2, cr4) and the case in which it has no type (cr5)
*
*/
TEST(mongoSubscribeContextAvailability, noPatternNoType)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn1("E1", "T1", "false");
    EntityId mockEn2("E1", "T1bis", "false");
    EntityId mockEn3("E1", "", "false");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A1", "TA1bis");
    ContextRegistrationResponse crr1, crr2, crr3, crr4;
    crr1.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.providingApplication.set("http://cr1.com");
    crr2.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr2.contextRegistration.providingApplication.set("http://cr2.com");
    crr3.contextRegistration.entityIdVector.push_back(&mockEn2);
    crr3.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr3.contextRegistration.providingApplication.set("http://cr4.com");
    crr4.contextRegistration.entityIdVector.push_back(&mockEn3);
    crr4.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr4.contextRegistration.providingApplication.set("http://cr5.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr1);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr2);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr3);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr4);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* pattern0Attr -
*
* Discover:  E[2-3] - none
* Result:    (E2, E3) - (A1, A2, A3) - http://cr1.com
*            E2       - (A2, A3)     - http://cr3.com
*
* This test also checks that discovering for type (E[2-3]) doesn't match with no-typed
* entities (E3** - cr5 is not returned)
*
*/
TEST(mongoSubscribeContextAvailability, pattern0Attr)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn1("E2", "T", "false");
    EntityId mockEn2("E3", "T", "false");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationAttribute cra3("A3", "TA3");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr1.contextRegistration.entityIdVector.push_back(&mockEn2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr1.contextRegistration.providingApplication.set("http://cr1.com");
    crr2.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr2.contextRegistration.providingApplication.set("http://cr3.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr1);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[2-3]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E[2-3]", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* pattern1AttrSingle -
*
* Discover:  E[1-3] - A4
* Result:    E1 - A4 - http://cr2.com
*/
TEST(mongoSubscribeContextAvailability, pattern1AttrSingle)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn("E1", "T", "false");
    ContextRegistrationAttribute cra("A4", "TA4");
    ContextRegistrationResponse crr;
    crr.contextRegistration.entityIdVector.push_back(&mockEn);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra);
    crr.contextRegistration.providingApplication.set("http://cr2.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-3]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A4");
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E[1-3]", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    EXPECT_EQ("A4", attrs[0].String());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* pattern1AttrMulti -
*
* Discover:  E[1-2] - A1
* Result:    (E1, E2) - A1 - http://cr1.com
*            E1       - A1 - http://cr2.com
*/
TEST(mongoSubscribeContextAvailability, pattern1AttrMulti)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn1("E1", "T", "false");
    EntityId mockEn2("E2", "T", "false");
    ContextRegistrationAttribute cra("A1", "TA1");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr1.contextRegistration.entityIdVector.push_back(&mockEn2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra);
    crr1.contextRegistration.providingApplication.set("http://cr1.com");
    crr2.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra);
    crr2.contextRegistration.providingApplication.set("http://cr2.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr1);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E[1-2]", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* patternNAttr -
*
* Discover:  E[1-2] - (A1, A2)
* Result:    (E1. E2) - (A1, A2) - http://cr1.com
*            E1      - A1        - http://cr2.com
*            E2      - A2        - http://cr3.com
*/
TEST(mongoSubscribeContextAvailability, patternNAttr)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn1("E1", "T", "false");
    EntityId mockEn2("E2", "T", "false");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationResponse crr1, crr2, crr3;
    crr1.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr1.contextRegistration.entityIdVector.push_back(&mockEn2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.providingApplication.set("http://cr1.com");
    crr2.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr2.contextRegistration.providingApplication.set("http://cr2.com");
    crr3.contextRegistration.entityIdVector.push_back(&mockEn2);
    crr3.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr3.contextRegistration.providingApplication.set("http://cr3.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr1);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr2);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr3);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");
    req.attributeList.push_back("A2");
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E[1-2]", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* patternNoType -
*
* Discover:  E[2-3]** - A2
* Result:    E2, E3 - A2  - http://cr1.com
*            E2     - A2  - http://cr3.com
*            E2*    - A2* - http://cr4.com
*            E3**   - A2  - http://cr5.com
*
* Note that this case checks matching of no-type in the discover for both the case in
* which the returned CR has type (cr1, cr3, cr4) and the case in which it has no type (cr5).
*
*/
TEST(mongoSubscribeContextAvailability, patternNoType)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn1("E2", "T", "false");
    EntityId mockEn2("E3", "T", "false");
    EntityId mockEn3("E2", "Tbis", "false");
    EntityId mockEn4("E3", "", "false");
    ContextRegistrationAttribute cra1("A2", "TA2");
    ContextRegistrationAttribute cra2("A2", "TA2bis");
    ContextRegistrationResponse crr1, crr2, crr3, crr4;
    crr1.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr1.contextRegistration.entityIdVector.push_back(&mockEn2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.providingApplication.set("http://cr1.com");
    crr2.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr2.contextRegistration.providingApplication.set("http://cr3.com");
    crr3.contextRegistration.entityIdVector.push_back(&mockEn3);
    crr3.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr3.contextRegistration.providingApplication.set("http://cr4.com");
    crr4.contextRegistration.entityIdVector.push_back(&mockEn4);
    crr4.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr4.contextRegistration.providingApplication.set("http://cr5.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr1);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr2);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr3);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr4);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[2-3]", "", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A2");
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E[2-3]", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    EXPECT_STREQ("true", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    EXPECT_EQ("A2", attrs[0].String());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* mixPatternAndNotPattern -
*
* Discover:  (E[2-3]. E1) - none
* Result:    (E1, E2, E3) - (A1, A2, A3) - http://cr1.com
*            E1           - (A1 ,A4) - http://cr2.com
*            E2           - (A2, A3) - http://cr3.com
*/
TEST(mongoSubscribeContextAvailability, mixPatternAndNotPattern)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifyContextAvailabilityRequest expectedNcar;
    EntityId mockEn1("E1", "T", "false");
    EntityId mockEn2("E2", "T", "false");
    EntityId mockEn3("E3", "T", "false");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationAttribute cra3("A3", "TA3");
    ContextRegistrationAttribute cra4("A4", "TA4");
    ContextRegistrationResponse crr1, crr2, crr3;
    crr1.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr1.contextRegistration.entityIdVector.push_back(&mockEn2);
    crr1.contextRegistration.entityIdVector.push_back(&mockEn3);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr1.contextRegistration.providingApplication.set("http://cr1.com");
    crr2.contextRegistration.entityIdVector.push_back(&mockEn1);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra4);
    crr2.contextRegistration.providingApplication.set("http://cr2.com");
    crr3.contextRegistration.entityIdVector.push_back(&mockEn2);
    crr3.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr3.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr3.contextRegistration.providingApplication.set("http://cr3.com");
    expectedNcar.contextRegistrationResponseVector.push_back(&crr1);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr2);
    expectedNcar.contextRegistrationResponseVector.push_back(&crr3);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                    "http://notify.me",
                                                                    "",
                                                                    "no correlator",
                                                                    NGSI_V1_LEGACY)).Times(1);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E[2-3]", "T", "true");
    EntityId en2("E1", "T", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360236300, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    BSONObj ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E[2-3]", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(ent0, "isPattern"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T", C_STR_FIELD(ent1, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent1, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(0, attrs.size());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* defaultDuration -
*/
TEST(mongoSubscribeContextAvailability, defaultDuration)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_, _, _, _, _)).Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E5", "T5", "false");
    req.entityIdVector.push_back(&en);
    req.reference.set("http://notify.me");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT24H", res.duration.get());
    EXPECT_FALSE(res.subscriptionId.isEmpty());
    std::string id = res.subscriptionId.get();
    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj());

    EXPECT_EQ(id, sub.getField("_id").OID().toString());
    EXPECT_EQ(1360319100, sub.getIntField("expiration"));
    EXPECT_STREQ("http://notify.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E5", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T5", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* MongoDbInsertFail -
*/
TEST(mongoSubscribeContextAvailability, MongoDbInsertFail)
{
    HttpStatusCode                       ms;
    SubscribeContextAvailabilityRequest  req;
    SubscribeContextAvailabilityResponse res;

    /* Prepare mocks */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, insert("utest.casubs", _, _, _))
            .WillByDefault(Throw(e));

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_, _, _, _, _, _, _, _, _)).Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E5", "T5", "false");
    req.entityIdVector.push_back(&en);
    req.duration.set("PT1H");
    req.reference.set("http://notify.me");

    /* Set MongoDB connection (prepare database first with the "actual" connection object).
     * The "actual" conneciton is preserved for later use */
    prepareDatabase();
    DBClientBase* connectionDb = getMongoConnection();
    setMongoConnectionForUnitTest(connectionMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoSubscribeContextAvailability(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.duration.isEmpty());
    EXPECT_TRUE(res.subscriptionId.isEmpty());
    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Internal Server Error", res.errorCode.reasonPhrase);
    std::string s1 = res.errorCode.details.substr(0, 70);
    std::string s2 = res.errorCode.details.substr(70+24, res.errorCode.details.size()-70-24);
    EXPECT_EQ("Database Error (collection: utest.casubs "
              "- insert(): { _id: ObjectId('", s1);
    EXPECT_EQ("'), expiration: 1360236300, "
              "reference: \"http://notify.me\", "
              "entities: [ { id: \"E5\", type: \"T5\", isPattern: \"false\" } ], "
              "attrs: [], format: \"JSON\" } "
              "- exception: boom!!)", s2);

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb);

    /* Release mocks */
    delete notifierMock;
    delete connectionMock;
    delete timerMock;

    /* Check actual database has not been touched */
    ASSERT_EQ(0, connectionDb->count(SUBSCRIBECONTEXTAVAIL_COLL, BSONObj()));
}
