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
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoConnectionPool.h"
#include "mongoBackend/mongoUnsubscribeContext.h"
#include "ngsi10/UnsubscribeContextRequest.h"
#include "ngsi10/UnsubscribeContextResponse.h"

#include "unittests/testInit.h"
#include "unittests/commonMocks.h"



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
using ::testing::_;
using ::testing::Throw;
using ::testing::Return;



extern void setMongoConnectionForUnitTest(DBClientBase _connection);



/* ****************************************************************************
*
* First set of test is related with updating thinks
*
* - subscriptionNotFound
* - unsubscribe
*
* Simulating fails in MongoDB connection.
*
* - MongoDbFindOneFail
* - MongoDbRemoveFail
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

    BSONObj sub1 = BSON("_id" << OID("51307b66f481db11bf860001") <<
                        "expiration" << 10000000 <<
                        "lastNotification" << 15000000 <<
                        "reference" << "http://notify1.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false")) <<
                        "attrs" << BSONArray() <<
                        "conditions" << BSON_ARRAY("AX1" << "AY1"));

    BSONObj sub2 = BSON("_id" << OID("51307b66f481db11bf860002") <<
                        "expiration" << 20000000 <<
                        "lastNotification" << 25000000 <<
                        "reference" << "http://notify2.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false")) <<
                        "attrs" << BSON_ARRAY("A1" << "A2") <<
                        "conditions" << BSON_ARRAY("AX2" << "AY2"));

    connection->insert(SUBSCRIBECONTEXT_COLL, sub1);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub2);
}

/* ****************************************************************************
*
* subscriptionNotFound -
*/
TEST(mongoUnsubscribeContext, subscriptionNotFound)
{
    HttpStatusCode             ms;
    UnsubscribeContextRequest  req;
    UnsubscribeContextResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_, _, _, _, _, _, _, _, _, _))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf869999");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUnsubscribeContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("51307b66f481db11bf869999", res.subscriptionId.get());
    EXPECT_EQ(SccContextElementNotFound, res.statusCode.code);
    EXPECT_EQ("No context element found", res.statusCode.reasonPhrase);
    EXPECT_EQ("subscriptionId: /51307b66f481db11bf869999/", res.statusCode.details);

    /* Check database (untouched) */
    DBClientBase* connection = getMongoConnection();
    ASSERT_EQ(2, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));

    /* Release mock */
    delete notifierMock;
}

/* ****************************************************************************
*
* unsubscribe -
*/
TEST(mongoUnsubscribeContext, unsubscribe)
{
    HttpStatusCode             ms;
    UnsubscribeContextRequest  req;
    UnsubscribeContextResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_, _, _, _, _, _, _, _, _, _))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUnsubscribeContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("51307b66f481db11bf860001", res.subscriptionId.get());
    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ(0, res.statusCode.details.size());

    /* Check database (one document, but not the deleted one) */
    DBClientBase* connection = getMongoConnection();
    ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());

    /* Release mock */
    delete notifierMock;
}


/* ****************************************************************************
*
* MongoDbFindOneFail -
*
*/
TEST(mongoUnsubscribeContext, MongoDbFindOneFail)
{
    HttpStatusCode             ms;
    UnsubscribeContextRequest  req;
    UnsubscribeContextResponse res;

    /* Prepare mocks */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, findOne("utest.csubs", _, _, _))
            .WillByDefault(Throw(e));

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_, _, _, _, _, _, _, _, _, _))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");

    /* Set MongoDB connection (prepare database first with the "actual" connection object).
     * The "actual" conneciton is preserved for later use */
    prepareDatabase();
    DBClientBase* connectionDb = getMongoConnection();
    setMongoConnectionForUnitTest(connectionMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoUnsubscribeContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("51307b66f481db11bf860001", res.subscriptionId.get());
    EXPECT_EQ(SccReceiverInternalError, res.statusCode.code);
    EXPECT_EQ("Internal Server Error", res.statusCode.reasonPhrase);
    EXPECT_EQ("Database Error (collection: utest.csubs "
              "- findOne(): { _id: ObjectId('51307b66f481db11bf860001') } "
              "- exception: boom!!)", res.statusCode.details);

    // Sleeping a little to "give mongod time to process its input".
    // Without this sleep, this tests fails around 10% of the times (in Ubuntu 13.04)
    usleep(1000);

    int count = connectionDb->count(SUBSCRIBECONTEXT_COLL, BSONObj());

    ASSERT_EQ(2, count);

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb);

    /* Release mocks */
    delete notifierMock;
    delete connectionMock;
}

/* ****************************************************************************
*
* MongoDbRemoveFail -
*
*/
TEST(mongoUnsubscribeContext, MongoDbRemoveFail)
{
    HttpStatusCode             ms;
    UnsubscribeContextRequest  req;
    UnsubscribeContextResponse res;

    /* Prepare mocks */
    const DBException e = DBException("boom!!", 33);

    BSONObj fakeSub = BSON("_id"              << OID("51307b66f481db11bf860001")            <<
                           "expiration"       << 10000000                                   <<
                           "lastNotification" << 15000000                                   <<
                           "reference"        << "http://notify1.me"                        <<
                           "entities"         << BSON_ARRAY(BSON("id"          << "E1" <<
                                                                 "type"        << "T1" <<
                                                                 "isPattern"   << "false")) <<
                           "attrs" << BSONArray()                                           <<
                           "conditions" << BSONArray());

    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, findOne("utest.csubs", _, _, _))
            .WillByDefault(Return(fakeSub));
    ON_CALL(*connectionMock, remove("utest.csubs", _, _, _))
            .WillByDefault(Throw(e));

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_, _, _, _, _, _, _, _, _, _))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");

    /* Set MongoDB connection (prepare database first with the "actual" connection object).
     * The "actual" conneciton is preserved for later use */
    prepareDatabase();
    DBClientBase* connectionDb = getMongoConnection();
    setMongoConnectionForUnitTest(connectionMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoUnsubscribeContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("51307b66f481db11bf860001", res.subscriptionId.get());
    EXPECT_EQ(SccReceiverInternalError, res.statusCode.code);
    EXPECT_EQ("Internal Server Error", res.statusCode.reasonPhrase);
    EXPECT_EQ("Database Error (collection: utest.csubs "
              "- remove(): { _id: ObjectId('51307b66f481db11bf860001') } "
              "- exception: boom!!)", res.statusCode.details);

    // Sleeping a little to "give mongod time to process its input".
    usleep(1000);

    ASSERT_EQ(2, connectionDb->count(SUBSCRIBECONTEXT_COLL, BSONObj()));

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb);

    /* Release mocks */
    delete notifierMock;
    delete connectionMock;
}

