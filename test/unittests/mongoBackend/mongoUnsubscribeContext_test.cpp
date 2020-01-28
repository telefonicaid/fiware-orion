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

#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/client.hpp>

#include "gtest/gtest.h"
#include "mongo/client/dbclient.h"  // FIXME OLD-DR: not actually needed but required
                                    // by the moment by setMongoConnectionForUnitTest()
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUnsubscribeContext.h"
#include "ngsi10/UnsubscribeContextRequest.h"
#include "ngsi10/UnsubscribeContextResponse.h"

#include "unittests/testInit.h"
#include "unittests/commonMocks.h"



/* ****************************************************************************
*
* USING
*/
using mongo::DBClientBase; // FIXME OLD-DR: needed by the setMongoConnectionForUnitTest
using mongo::BSONObj;      // FIXME OLD-DR: needed by the tests that use DB mocks
using mongo::BSONArray;    // FIXME OLD-DR: needed by the tests that use DB mocks
using mongo::OID;          // FIXME OLD-DR: needed by the tests that use DB mocks
using mongo::DBException;  // FIXME OLD-DR: needed by the tests that use DB mocks

using ::testing::_;
using ::testing::Throw;
using ::testing::Return;

using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::finalize;

extern void setMongoConnectionForUnitTest(mongo::DBClientBase* _connection, mongocxx::client* _connectionCxx);



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

    mongocxx::client* connectionCxx = getMongoConnectionCxx();

    auto builder = bsoncxx::builder::stream::document{};

    bsoncxx::document::value sub1 = builder
      << "_id" << bsoncxx::oid("51307b66f481db11bf860001")
      << "expiration" << 10000000
      << "lastNotification" << 15000000
      << "reference" << "http://notify1.me"
      << "entities" << open_array
        << open_document << "id" << "E1" << "type" << "T1" << "isPattern" << "false" << close_document
      << close_array
      << "attrs" << open_array << close_array
      << "contidions" << open_array << "AX1" << "AY1" << close_array
      << finalize;

    bsoncxx::document::value sub2 = builder
      << "_id" << bsoncxx::oid("51307b66f481db11bf860002")
      << "expiration" << 20000000
      << "lastNotification" << 25000000
      << "reference" << "http://notify2.me"
      << "entities" << open_array
        << open_document << "id" << "E1" << "type" << "T1" << "isPattern" << "false" << close_document
      << close_array
      << "attrs" << open_array << "A1" << "A2" << close_array
      << "contidions" << open_array << "AX2" << "AY2" << close_array
      << finalize;

    // FIXME OLD-DR: connection must include the database itself
    // FIXME OLD-DR: we should use SUBSCRIBECONTEXT_COLL but it includes "utest." prefix

    (*connectionCxx)["utest"]["csubs"].insert_one(sub1.view());
    (*connectionCxx)["utest"]["csubs"].insert_one(sub2.view());
}



/* ****************************************************************************
*
* getDocId -
*/
inline std::string getDocId(const bsoncxx::document::value& sub)
{
  return sub.view()["_id"].get_oid().value.to_string();
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
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_, _, _, _, _, _, _, _, _))
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
    mongocxx::client* connectionCxx = getMongoConnectionCxx();
    ASSERT_EQ(2, (*connectionCxx)["utest"]["csubs"].count_documents(bsoncxx::document::view()));

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
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_, _, _, _, _, _, _, _, _))
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
    mongocxx::client* connectionCxx = getMongoConnectionCxx();
    ASSERT_EQ(1, (*connectionCxx)["utest"]["csubs"].count_documents(bsoncxx::document::view()));

    auto builder = bsoncxx::builder::stream::document{};
    bsoncxx::document::value query = builder << "_id" << bsoncxx::oid("51307b66f481db11bf860002") << finalize;
    bsoncxx::stdx::optional<bsoncxx::document::value> sub = (*connectionCxx)["utest"]["csubs"].find_one(query.view());
    ASSERT_TRUE((bool)sub);  // ensure find_one actually found a document
    EXPECT_EQ("51307b66f481db11bf860002", getDocId(*sub));

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
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_, _, _, _, _, _, _, _, _))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");

    /* Set MongoDB connection (prepare database first with the "actual" connection object).
     * The "actual" conneciton is preserved for later use */
    prepareDatabase();
    DBClientBase* connectionDb = getMongoConnection();
    mongocxx::client* connectionCxx = getMongoConnectionCxx();
    setMongoConnectionForUnitTest(connectionMock, connectionCxx);

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

    int count = (*connectionCxx)["utest"]["csubs"].count_documents(bsoncxx::document::view());
    ASSERT_EQ(2, count);

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb, connectionCxx);

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
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_, _, _, _, _, _, _, _, _))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");

    /* Set MongoDB connection (prepare database first with the "actual" connection object).
     * The "actual" conneciton is preserved for later use */
    prepareDatabase();
    DBClientBase* connectionDb = getMongoConnection();
    mongocxx::client* connectionCxx = getMongoConnectionCxx();
    setMongoConnectionForUnitTest(connectionMock, connectionCxx);

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

    int count = (*connectionCxx)["utest"]["csubs"].count_documents(bsoncxx::document::view());
    ASSERT_EQ(2, count);

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb, connectionCxx);

    /* Release mocks */
    delete notifierMock;
    delete connectionMock;
}
