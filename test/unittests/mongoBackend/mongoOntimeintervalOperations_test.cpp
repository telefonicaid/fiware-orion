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
#include "gtest/gtest.h"
#include "testInit.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoOntimeintervalOperations.h"

#include "mongo/client/dbclient.h"

#include "commonMocks.h"

using ::testing::_;
using ::testing::MatchesRegex;
using ::testing::Throw;
using ::testing::Return;

extern void setMongoConnectionForUnitTest(DBClientBase*);

/* ****************************************************************************
*
* For mongoGetContextSubscriptionInfo:
*
* - mongoGetContextSubscriptionInfo_ok
* - mongoGetContextSubscriptionInfo_fail
* - mongoGetContextSubscriptionInfo_dbfail
*
* For mongoGetContextElementResponses:
*
* - mongoGetContextElementResponses_ok
* - mongoGetContextElementResponses_pattern
* - mongoGetContextElementResponses_fail
* - mongoGetContextElementResponses_dbfail
*
* For mongoUpdateCsubNewNotification:
*
* - mongoUpdateCsubNewNotification_ok
* - mongoUpdateCsubNewNotification_fail
* - mongoUpdateCsubNewNotification_dbfail
*
*/

/* ****************************************************************************
*
* prepareDatabase -
*/
static void prepareDatabase(void) {

    /* Set database */
    setupDatabase();

    DBClientBase* connection = getMongoConnection();

    /* We create the following entities:
     *
     * - E1:
     *     A1:  X
     *     A2:  Z
     *     A3:  W
     *     A7:  R
     * - E2
     *     A1:  S
     *     A4:  T
     *     A6:  U
     * - E3
     *     A1:  noise
     *     A1*: noise
     *     A2:  noise
     *     A3:  noise
     *     A4:  noise
     *     A6:  noise
     *     A7:  noise
     *
     * We create the following subscriptions:
     *
     * - Sub1:
     *     Entities: E1, E2
     *     Attribute: A1, A3, A4
     *     NotifyCond: ONCHANGE on [A1, A2, A4]
     *
     * - Sub2:
     *     Entities: E2
     *     Attribute: A1, A3, A4
     *     NotifyCond: ONCHANGE on A1
     *                 ONCHANGE on A2
     *                 ONCHANGE on A4
     *
     */

    BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T") <<
                       "attrNames" << BSON_ARRAY("A1" << "A2" << "A3" << "A7") <<
                       "attrs" << BSON(
                          "A1" << BSON("type" << "TA1" << "value" << "X") <<
                          "A2" << BSON("type" << "TA2" << "value" << "Z") <<
                          "A3" << BSON("type" << "TA3" << "value" << "W") <<
                          "A7" << BSON("type" << "TA7" << "value" << "R")
                          )
                      );

    BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T") <<
                       "attrNames" << BSON_ARRAY("A1" << "A4" << "A6") <<
                       "attrs" << BSON(
                          "A1" << BSON("type" << "TA1" << "value" << "S") <<
                          "A4" << BSON("type" << "TA4" << "value" << "T") <<
                          "A6" << BSON("type" << "TA6" << "value" << "U")
                          )
                      );

    BSONObj en3 = BSON("_id" << BSON("id" << "E3" << "type" << "T") <<
                       "attrNames" << BSON_ARRAY("A1" << "A2" << "A3" << "A4" << "A6" << "A7") <<
                       "attrs" << BSON(
                           "A1" << BSON("type" << "TA1" << "value" << "noise") <<
                           "A2" << BSON("type" << "TA2" << "value" << "noise") <<
                           "A3" << BSON("type" << "TA3" << "value" << "noise") <<
                           "A4" << BSON("type" << "TA4" << "value" << "noise") <<
                           "A6" << BSON("type" << "TA6" << "value" << "noise") <<
                           "A7" << BSON("type" << "TA7" << "value" << "noise")
                          )
                      );

    BSONObj sub1 = BSON("_id" << OID("51307b66f481db11bf860001") <<
                        "expiration" << 10000000 <<
                        "lastNotification" << 20000000 <<
                        "throttling" << 10 <<
                        "reference" << "http://notify1.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T" << "isPattern" << "false") <<
                                                 BSON("id" << "E2" << "type" << "T" << "isPattern" << "false")
                                                 ) <<
                        "attrs" << BSON_ARRAY("A1" << "A3" << "A4") <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("A1" << "A2" << "A4" << "A5")
                                                       )) <<
                        "count" << 20 <<
                        "format" << "XML"
                        );

    BSONObj sub2 = BSON("_id" << OID("51307b66f481db11bf860002") <<
                        "expiration" << 20000000 <<
                        "lastNotification" << 30000000 <<
                        "throttling" << 20 <<
                        "reference" << "http://notify2.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E2" << "type" << "T" << "isPattern" << "false")) <<
                        "attrs" << BSON_ARRAY("A1" << "A3" << "A4") <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("A1")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("A2")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("A4")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("A5")
                                                       )) <<
                        "count" << 30
                        );

    connection->insert(ENTITIES_COLL, en1);
    connection->insert(ENTITIES_COLL, en2);
    connection->insert(ENTITIES_COLL, en3);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub1);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub2);


}

/* ****************************************************************************
*
* mongoGetContextSubscriptionInfo_ok -
*/
TEST(mongoOntimeintervalOperations, mongoGetContextSubscriptionInfo_ok)
{
    HttpStatusCode ms;

    /* Forge the parameters */
    std::string subId = "51307b66f481db11bf860001";
    ContextSubscriptionInfo csi;
    std::string err;

    /* Prepare database */
    prepareDatabase();

    /* Do operation */
    ms = mongoGetContextSubscriptionInfo(subId, &csi, &err);

    /* Check results */
    EXPECT_EQ(SccOk, ms);
    ASSERT_EQ(2, csi.entityIdVector.size());
    EXPECT_EQ("E1", csi.entityIdVector.get(0)->id);
    EXPECT_EQ("T", csi.entityIdVector.get(0)->type);
    EXPECT_EQ("false", csi.entityIdVector.get(0)->isPattern);
    EXPECT_EQ("E2", csi.entityIdVector.get(1)->id);
    EXPECT_EQ("T", csi.entityIdVector.get(1)->type);
    EXPECT_EQ("false", csi.entityIdVector.get(1)->isPattern);
    ASSERT_EQ(3, csi.attributeList.size());
    EXPECT_EQ("A1", csi.attributeList.get(0));
    EXPECT_EQ("A3", csi.attributeList.get(1));
    EXPECT_EQ("A4", csi.attributeList.get(2));
    EXPECT_EQ(10000000, csi.expiration);
    EXPECT_EQ(20000000, csi.lastNotification);
    EXPECT_EQ("http://notify1.me", csi.url);
    EXPECT_EQ(10, csi.throttling);
    EXPECT_EQ(XML, csi.format);
    EXPECT_EQ(0, err.length());

}

/* ****************************************************************************
*
* mongoGetContextSubscriptionInfo_fail -
*/
TEST(mongoOntimeintervalOperations, mongoGetContextSubscriptionInfo_fail)
{
    HttpStatusCode ms;

    /* Forge the parameters */
    std::string subId = "51307b66f481db11bf869999";
    ContextSubscriptionInfo csi;
    std::string err;

    /* Prepare database */
    prepareDatabase();

    /* Do operation */
    ms = mongoGetContextSubscriptionInfo(subId, &csi, &err);

    /* Check results */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(0, err.length());

}

/* ****************************************************************************
*
* mongoGetContextSubscriptionInfo_dbfail -
*/
TEST(mongoOntimeintervalOperations, mongoGetContextSubscriptionInfo_dbfail)
{
    HttpStatusCode ms;

    /* Prepare mock */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, findOne("utest.csubs",_,_,_))
            .WillByDefault(Throw(e));

    /* Forge the parameters */
    std::string subId = "51307b66f481db11bf860001";
    ContextSubscriptionInfo csi;
    std::string err;

    /* Set MongoDB connection (prepare database first with the "actual" connection object) */
    prepareDatabase();
    DBClientBase* connectionDb = getMongoConnection();
    setMongoConnectionForUnitTest(connectionMock);

    /* Do operation */
    ms = mongoGetContextSubscriptionInfo(subId, &csi, &err);

    /* Check results */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("Database Error (collection: utest.csubs "
              "- findOne(): { _id: ObjectId('51307b66f481db11bf860001') } "
              "- exception: boom!!)", err);

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb);

    /* Release mocks */
    delete connectionMock;
}

/* ****************************************************************************
*
* mongoGetContextElementResponses_ok -
*/
TEST(mongoOntimeintervalOperations, mongoGetContextElementResponses_ok)
{
    HttpStatusCode ms;

    /* Forge the parameters */
    EntityIdVector enV;
    EntityId en1("E1", "T", "false");
    EntityId en2("E2", "T", "false");
    enV.push_back(&en1);
    enV.push_back(&en2);
    AttributeList attrL;
    attrL.push_back("A1");
    attrL.push_back("A2");
    attrL.push_back("A3");
    attrL.push_back("A4");
    ContextElementResponseVector cerV;
    std::string err;

    /* Prepare database */
    prepareDatabase();

    /* Do operation */
    ms = mongoGetContextElementResponses(enV, attrL, &cerV, &err);

    /* Check results */
    EXPECT_EQ(SccOk, ms);
    ASSERT_EQ(2, cerV.size());
    ContextElementResponse cer0 = *cerV.get(0);
    ContextElementResponse cer1 = *cerV.get(1);
    ContextAttribute ca0, ca1, ca2, ca3;

    /* Context Element Response #1 */
    EXPECT_EQ(SccOk, cer0.statusCode.code);
    EXPECT_EQ("OK", cer0.statusCode.reasonPhrase);
    EXPECT_EQ(0, cer0.statusCode.details.length());
    EXPECT_EQ("E1", cer0.contextElement.entityId.id);
    EXPECT_EQ("T", cer0.contextElement.entityId.type);
    EXPECT_EQ("false", cer0.contextElement.entityId.isPattern);
    ASSERT_EQ(3, cer0.contextElement.contextAttributeVector.size());
    ca0 = *cer0.contextElement.contextAttributeVector.get(0);
    ca1 = *cer0.contextElement.contextAttributeVector.get(1);
    ca2 = *cer0.contextElement.contextAttributeVector.get(2);    
    EXPECT_EQ("A1", ca0.name);
    EXPECT_EQ("TA1", ca0.type);
    EXPECT_EQ("X", ca0.stringValue);
    EXPECT_EQ("A2", ca1.name);
    EXPECT_EQ("TA2", ca1.type);
    EXPECT_EQ("Z", ca1.stringValue);
    EXPECT_EQ("A3", ca2.name);
    EXPECT_EQ("TA3", ca2.type);
    EXPECT_EQ("W", ca2.stringValue);

    /* Context Element Response #2 */
    EXPECT_EQ(SccOk, cer1.statusCode.code);
    EXPECT_EQ("OK", cer1.statusCode.reasonPhrase);
    EXPECT_EQ(0, cer1.statusCode.details.length());
    EXPECT_EQ("E2", cer1.contextElement.entityId.id);
    EXPECT_EQ("T", cer1.contextElement.entityId.type);
    EXPECT_EQ("false", cer1.contextElement.entityId.isPattern);
    ASSERT_EQ(2, cer1.contextElement.contextAttributeVector.size());
    ca0 = *cer1.contextElement.contextAttributeVector.get(0);
    ca1 = *cer1.contextElement.contextAttributeVector.get(1);
    EXPECT_EQ("A1", ca0.name);
    EXPECT_EQ("TA1", ca0.type);
    EXPECT_EQ("S", ca0.stringValue);
    EXPECT_EQ("A4", ca1.name);
    EXPECT_EQ("TA4", ca1.type);
    EXPECT_EQ("T", ca1.stringValue);

    EXPECT_EQ(0, err.length());
}

/* ****************************************************************************
*
* mongoGetContextElementResponses_pattern -
*/
TEST(mongoOntimeintervalOperations, mongoGetContextElementResponses_pattern)
{
    HttpStatusCode ms;

    /* Forge the parameters */
    EntityIdVector enV;
    EntityId en("E[1-2]", "T", "true");
    enV.push_back(&en);
    AttributeList attrL;
    attrL.push_back("A1");
    attrL.push_back("A2");
    attrL.push_back("A3");
    attrL.push_back("A4");
    ContextElementResponseVector cerV;
    std::string err;

    /* Prepare database */
    prepareDatabase();

    /* Do operation */
    ms = mongoGetContextElementResponses(enV, attrL, &cerV, &err);

    /* Check results */
    EXPECT_EQ(SccOk, ms);
    ASSERT_EQ(2, cerV.size());
    ContextElementResponse cer0 = *cerV.get(0);
    ContextElementResponse cer1 = *cerV.get(1);
    ContextAttribute ca0, ca1, ca2, ca3;

    /* Context Element Response #1 */
    EXPECT_EQ(SccOk, cer0.statusCode.code);
    EXPECT_EQ("OK", cer0.statusCode.reasonPhrase);
    EXPECT_EQ(0, cer0.statusCode.details.length());
    EXPECT_EQ("E1", cer0.contextElement.entityId.id);
    EXPECT_EQ("T", cer0.contextElement.entityId.type);
    EXPECT_EQ("false", cer0.contextElement.entityId.isPattern);
    ASSERT_EQ(3, cer0.contextElement.contextAttributeVector.size());
    ca0 = *cer0.contextElement.contextAttributeVector.get(0);
    ca1 = *cer0.contextElement.contextAttributeVector.get(1);
    ca2 = *cer0.contextElement.contextAttributeVector.get(2);    
    EXPECT_EQ("A1", ca0.name);
    EXPECT_EQ("TA1", ca0.type);
    EXPECT_EQ("X", ca0.stringValue);
    EXPECT_EQ("A2", ca1.name);
    EXPECT_EQ("TA2", ca1.type);
    EXPECT_EQ("Z", ca1.stringValue);
    EXPECT_EQ("A3", ca2.name);
    EXPECT_EQ("TA3", ca2.type);
    EXPECT_EQ("W", ca2.stringValue);

    /* Context Element Response #2 */
    EXPECT_EQ(SccOk, cer1.statusCode.code);
    EXPECT_EQ("OK", cer1.statusCode.reasonPhrase);
    EXPECT_EQ(0, cer1.statusCode.details.length());
    EXPECT_EQ("E2", cer1.contextElement.entityId.id);
    EXPECT_EQ("T", cer1.contextElement.entityId.type);
    EXPECT_EQ("false", cer1.contextElement.entityId.isPattern);
    ASSERT_EQ(2, cer1.contextElement.contextAttributeVector.size());
    ca0 = *cer1.contextElement.contextAttributeVector.get(0);
    ca1 = *cer1.contextElement.contextAttributeVector.get(1);
    EXPECT_EQ("A1", ca0.name);
    EXPECT_EQ("TA1", ca0.type);
    EXPECT_EQ("S", ca0.stringValue);
    EXPECT_EQ("A4", ca1.name);
    EXPECT_EQ("TA4", ca1.type);
    EXPECT_EQ("T", ca1.stringValue);

    EXPECT_EQ(0, err.length());
}

/* ****************************************************************************
*
* mongoGetContextElementResponses_fail -
*/
TEST(mongoOntimeintervalOperations, mongoGetContextElementResponses_fail)
{
    HttpStatusCode ms;

    /* Forge the parameters */
    EntityIdVector enV;
    EntityId en("E5", "T", "false");
    enV.push_back(&en);
    AttributeList attrL;
    attrL.push_back("A1");
    attrL.push_back("A2");
    attrL.push_back("A3");
    attrL.push_back("A4");
    ContextElementResponseVector cerV;
    std::string err;

    /* Prepare database */
    prepareDatabase();

    /* Do operation */
    ms = mongoGetContextElementResponses(enV, attrL, &cerV, &err);

    /* Check results */
    EXPECT_EQ(SccOk, ms);
    ASSERT_EQ(0, cerV.size());
}

/* ****************************************************************************
*
* mongoGetContextElementResponses_dbfail -
*
* FIXME: not sure if this test should exist... it should be include in unit testing
* for entitiesQuery()
*/
TEST(mongoOntimeintervalOperations, mongoGetContextElementResponses_dbfail)
{
    HttpStatusCode ms;

    /* Prepare mock */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock,_query("utest.entities",_,_,_,_,_,_))
            .WillByDefault(Throw(e));

    /* Forge the parameters */
    EntityIdVector enV;
    EntityId en1("E1", "T", "false");
    EntityId en2("E2", "T", "false");
    enV.push_back(&en1);
    enV.push_back(&en2);
    AttributeList attrL;
    attrL.push_back("A1");
    attrL.push_back("A2");
    attrL.push_back("A3");
    attrL.push_back("A4");
    ContextElementResponseVector cerV;
    std::string err;

    /* Set MongoDB connection (prepare database first with the "actual" connection object) */
    prepareDatabase();
    DBClientBase* connectionDb = getMongoConnection();
    setMongoConnectionForUnitTest(connectionMock);

    /* Do operation */
    ms = mongoGetContextElementResponses(enV, attrL, &cerV, &err);    

    /* Check results */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("Database Error (collection: utest.entities - "
              "query(): { query: { $or: [ { _id.id: \"E1\", _id.type: \"T\" }, { _id.id: \"E2\", _id.type: \"T\" } ], _id.servicePath: { $in: [ /^/.*/, null ] }, "
              "attrNames: { $in: [ \"A1\", \"A2\", \"A3\", \"A4\" ] } }, orderby: { creDate: 1 } } - "
              "exception: boom!!)", err);

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb);

    /* Release mocks */
    delete connectionMock;
}

/* ****************************************************************************
*
* mongoUpdateCsubNewNotification_ok -
*/
TEST(mongoOntimeintervalOperations, mongoUpdateCsubNewNotification_ok)
{
    HttpStatusCode ms;

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the parameters */
    std::string subId = "51307b66f481db11bf860001";
    std::string err;

    /* Prepare database */
    prepareDatabase();

    /* Do operation */
    ms = mongoUpdateCsubNewNotification(subId, &err);

    /* Check results */
    EXPECT_EQ(SccOk, ms);

    /* Check that database is as expected */
    DBClientBase* connection = getMongoConnection();
    BSONObj sub1 = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));
    BSONObj sub2 = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_EQ(1360232700, sub1.getIntField("lastNotification"));
    EXPECT_EQ(21, sub1.getIntField("count"));
    EXPECT_EQ(30000000, sub2.getIntField("lastNotification"));    
    EXPECT_EQ(30, sub2.getIntField("count"));

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* mongoUpdateCsubNewNotification_fail -
*/
TEST(mongoOntimeintervalOperations, mongoUpdateCsubNewNotification_fail)
{
    HttpStatusCode ms;

    /* Prepare mocks */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the parameters */
    std::string subId = "51307b66f481db11bf869999";
    std::string err;

    /* Prepare database */
    prepareDatabase();

    /* Do operation */
    ms = mongoUpdateCsubNewNotification(subId, &err);

    /* Check results */
    EXPECT_EQ(SccOk, ms);

    /* Check that database is as expected (untouched) */
    DBClientBase* connection = getMongoConnection();
    BSONObj sub1 = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));
    BSONObj sub2 = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_EQ(20000000, sub1.getIntField("lastNotification"));
    EXPECT_EQ(20, sub1.getIntField("count"));
    EXPECT_EQ(30000000, sub2.getIntField("lastNotification"));
    EXPECT_EQ(30, sub2.getIntField("count"));

    /* Release mocks */
    delete timerMock;
}

/* ****************************************************************************
*
* mongoUpdateCsubNewNotification_dbfail -
*/
TEST(mongoOntimeintervalOperations, mongoUpdateCsubNewNotification_dbfail)
{
    HttpStatusCode ms;

    /* Prepare mock */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, update("utest.csubs",_,_,_,_,_))
            .WillByDefault(Throw(e));

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the parameters */
    std::string subId = "51307b66f481db11bf860001";
    std::string err;

    /* Set MongoDB connection (prepare database first with the "actual" connection object)
     * Note that we preserve the "actual" connection for future database checking */
    prepareDatabase();
    DBClientBase* connectionDb = getMongoConnection();
    setMongoConnectionForUnitTest(connectionMock);

    /* Do operation */
    ms = mongoUpdateCsubNewNotification(subId, &err);

    /* Check results */
    EXPECT_EQ(SccReceiverInternalError, ms);
    EXPECT_EQ("Database Error (collection: utest.csubs "
              "- update(): <{ _id: ObjectId('51307b66f481db11bf860001') },{ $set: { lastNotification: 1360232700 }, $inc: { count: 1 } }> "
              "- exception: boom!!)", err);

    /* Check that database is as expected (untouched) */   

    // Sleeping a little to "give mongod time to process its input".
    usleep(1000);

    BSONObj sub1 = connectionDb->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));
    BSONObj sub2 = connectionDb->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_EQ(20000000, sub1.getIntField("lastNotification"));
    EXPECT_EQ(20, sub1.getIntField("count"));
    EXPECT_EQ(30000000, sub2.getIntField("lastNotification"));
    EXPECT_EQ(30, sub2.getIntField("count"));

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb);

    /* Release mocks */    
    delete timerMock;
    delete connectionMock;
}
