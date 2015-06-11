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
#include "unittest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUpdateContextSubscription.h"
#include "ngsi10/UpdateContextSubscriptionRequest.h"
#include "ngsi10/UpdateContextSubscriptionResponse.h"

#include "ngsi/EntityId.h"
#include "ngsi/NotifyCondition.h"

#include "mongo/client/dbclient.h"

#include "commonMocks.h"

using ::testing::_;
using ::testing::Throw;
using ::testing::Return;

extern void setMongoConnectionForUnitTest(DBClientBase*);

/* ****************************************************************************
*
* First set of test is related with updating thinks
*
* - subscriptionNotFound
* - updateDuration
* - updateRestriction
* - updateThrottling
* - clearThrottling
*
* The rest of the tests are for updates in NotifyConditions and mirror the ones used in
* mongoSubscribeContext_test.
*
* - Ent1_Attr0_T1_C0
* - Ent1_Attr0_T1_C0_JSON
* - Ent1_AttrN_T1_C0
* - Ent1_Attr0_TN_C0
* - Ent1_AttrN_TN_C0
* - Ent1_Attr0_T0_C1
* - Ent1_AttrN_T0_C1
* - Ent1_Attr0_T0_CN
* - Ent1_Attr0_T0_CNbis
* - Ent1_AttrN_T0_CN
* - Ent1_AttrN_T0_CNbis
* - Ent1_Attr0_TN_CN
* - Ent1_Attr0_TN_CNbis
* - Ent1_AttrN_TN_CN
* - Ent1_AttrN_TN_CNbis
* - EntN_Attr0_T1_C0
* - EntN_AttrN_T1_C0
* - EntN_Attr0_TN_C0
* - EntN_AttrN_TN_C0
* - EntN_Attr0_T0_C1
* - EntN_AttrN_T0_C1
* - EntN_Attr0_T0_CN
* - EntN_Attr0_T0_CNbis
* - EntN_AttrN_T0_CN
* - EntN_AttrN_T0_CNbis
* - EntN_AttrN_T0_CN
* - EntN_AttrN_T0_CNbis
* - EntN_AttrN_TN_CN
* - EntN_AttrN_TN_CNbis
*
* (T: ONTIMEINTERVAL)
* (C: ONCHANGE)
*
* Test matching entities/attributes (with 1 condition, both ONCHANGE and ONINTERVAL). Some
* of this tests take into account type. The _partial tests are variant of _CN in which
* only one of the condValues match the attributes in the entity. The _disjoint tests are
* valiants of _AttrN_ in which the attribute in the condValue belong to the entity, but
* it is not include in the N attributes to be returned.
*
* - matchEnt1_Attr0_T0_C1
* - matchEnt1_Attr0_T0_C1_JSON
* - matchEnt1_AttrN_T0_C1
* - matchEnt1_AttrN_T0_C1_disjoint
* - matchEnt1NoType_AttrN_T0_C1
* - matchEnt1NoType_AttrN_T0_C1_disjoint
* - matchEnt1Pattern_AttrN_T0_C1
* - matchEnt1Pattern_AttrN_T0_C1_disjoint
* - matchEnt1PatternNoType_AttrN_T0_C1
* - matchEnt1PatternNoType_AttrN_T0_C1_disjoint
* - matchEnt1_Attr0_T0_CN
* - matchEnt1_Attr0_T0_CN_partial
* - matchEnt1_Attr0_T0_CNbis
* - matchEnt1_AttrN_T0_CN_disjoint
* - matchEnt1_AttrN_T0_CN_partial
* - matchEnt1_AttrN_T0_CN_partial_disjoint
* - matchEnt1_AttrN_T0_CNbis
* - matchEnt1_Attr0_TN_CN
* - matchEnt1_Attr0_TN_CNbis
* - matchEnt1_AttrN_TN_CN
* - matchEnt1_AttrN_TN_CNbis
* - matchEntN_Attr0_T0_C1
* - matchEntN_AttrN_T0_C1
* - matchEntN_Attr0_T0_CN
* - matchEntN_Attr0_T0_CNbis
* - matchEntN_AttrN_T0_CN
* - matchEntN_AttrN_T0_CNbis
* - matchEntN_Attr0_TN_CN
* - matchEntN_Attr0_TN_CNbis
* - matchEntN_AttrN_TN_CN
* - matchEntN_AttrN_TN_CNbis
*
* Next we have tests that mix updates in several fields in the same request:
*
* - updateDurationAndNotifyConditions
*
* Simulating fails in MongoDB connection. Note that we only test direct use of MongoDB connection
* (i.e. connection-> uses in mongoUpdateContextSubscription module), indirect uses in called functions
* outside the module are not tested here.
*
* - MongoDbFindOneFail
* - MongoDbUpdateFail
*
* Note that test related with triggering notifications due to updateContext on attributes covered
* by ONCHANGE subscriptions are not included in this test suite, with deals exclusively with
* updateContextSubscription processing. See mongoUpdateContext_withOnChangeSubscriptions_test.cpp
*
* FIXME: test that are not included, but that we should detect in the parsing checking logic
* (remove this FIXME once we assure on that):
*
* - NotifyCondtions MUST not be 0 (OMA spec cardinallity allows this, but for us is an error)
* - Duration MUST be included (our policy is so)
* - ONTIMEINTERVAL condValues MUST be exactly 1
* - ONCHANGE condValues MUST be 1 or more
* - ONVALUE condValues MUST be 0
* - Restiction MUST be ommited in ONTIMEINTERVAL and ONCHANGE case
*
* FIXME: we are not taking into account ONVALUE by the moment
*
*/

/* ****************************************************************************
*
* emptyServicePathV -
*
* Empty vector of service paths, sent to mongoSubscribeContext during tests.
*/
static std::vector<std::string> emptyServicePathV;

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
     *     A1: X
     *     A2: Z
     *     A3: W
     * - E2
     *     A2: R
     *     A3: S
     * - E1*
     *     A1: T
     * - E1**
     *     A1: P
     *     A2: Q
     *
     * (*) Means that entity/type is using same name but different type. This is included to check that type is
     *     taken into account.
     * (**)same name but without type
     */

    BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                       "attrNames" << BSON_ARRAY("A1" << "A2" << "A3") <<
                       "attrs" << BSON(
                          "A1" << BSON("type" << "TA1" << "value" << "X") <<
                          "A2" << BSON("type" << "TA2" << "value" << "Z") <<
                          "A3" << BSON("type" << "TA3" << "value" << "W")
                          )
                      );

    BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T2") <<
                       "attrNames" << BSON_ARRAY("A2" << "A3") <<
                       "attrs" << BSON(
                          "A2" << BSON("type" << "TA2" << "value" << "R") <<
                          "A3" << BSON("type" << "TA3" << "value" << "S")
                          )
                      );

    BSONObj en3 = BSON("_id" << BSON("id" << "E1" << "type" << "T1bis") <<
                       "attrNames" << BSON_ARRAY("A1") <<
                       "attrs" << BSON(
                          "A1" << BSON("type" << "TA1" << "value" << "T")
                          )
                      );

    BSONObj en4 = BSON("_id" << BSON("id" << "E1") <<
                       "attrNames" << BSON_ARRAY("A1" << "A2") <<
                       "attrs" << BSON(
                          "A1" << BSON("type" << "TA1" << "value" << "P") <<
                          "A2" << BSON("type" << "TA2" << "value" << "Q")
                          )
                      );

    BSONObj sub1 = BSON("_id" << OID("51307b66f481db11bf860001") <<
                        "expiration" << 10000000 <<
                        "lastNotification" << 15000000 <<
                        "throttling" << 60 <<
                        "reference" << "http://notify1.me" <<
                        "format" << "XML" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false")) <<
                        "attrs" << BSONArray() <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX1" << "AY1")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONTIMEINTERVAL" <<
                                                       "value" << 100
                                                       ))
                        );

    BSONObj sub2 = BSON("_id" << OID("51307b66f481db11bf860002") <<
                        "expiration" << 20000000 <<
                        "lastNotification" << 25000000 <<
                        "reference" << "http://notify2.me" <<
                        "format" << "XML" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false")) <<
                        "attrs" << BSON_ARRAY("A1" << "A2") <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX2" << "AY2")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONTIMEINTERVAL" <<
                                                       "value" << 200
                                                       ))
                        );

    /* No type variant of sub2 */
    BSONObj sub2nt = BSON("_id" << OID("51307b66f481db11bf860022") <<
                        "expiration" << 20000000 <<
                        "lastNotification" << 25000000 <<
                        "reference" << "http://notify2.me" <<
                        "format" << "XML" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "isPattern" << "false")) <<
                        "attrs" << BSON_ARRAY("A1" << "A2") <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX2" << "AY2")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONTIMEINTERVAL" <<
                                                       "value" << 200
                                                       ))
                        );

    BSONObj sub3 = BSON("_id" << OID("51307b66f481db11bf860003") <<
                        "expiration" << 30000000 <<
                        "lastNotification" << 35000000 <<
                        "reference" << "http://notify3.me" <<
                        "format" << "XML" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false") <<
                                                 BSON("id" << "E2" << "type" << "T2" << "isPattern" << "false")) <<
                        "attrs" << BSONArray() <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX3" << "AY3")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONTIMEINTERVAL" <<
                                                       "value" << 300
                                                       ))
                        );

    BSONObj sub4 = BSON("_id" << OID("51307b66f481db11bf860004") <<
                        "expiration" << 40000000 <<
                        "lastNotification" << 45000000 <<
                        "reference" << "http://notify4.me" <<
                        "format" << "XML" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false") <<
                                                 BSON("id" << "E2" << "type" << "T2" << "isPattern" << "false")) <<
                        "attrs" << BSON_ARRAY("A1" << "A2") <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX4" << "AY4")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONTIMEINTERVAL" <<
                                                       "value" << 400
                                                       ))
                        );
    BSONObj sub5 = BSON("_id" << OID("51307b66f481db11bf860005") <<
                        "expiration" << 50000000 <<
                        "lastNotification" << 55000000 <<
                        "throttling" << 10 <<
                        "reference" << "http://notify5.me" <<
                        "format" << "XML" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false")) <<
                        "attrs" << BSONArray() <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX5" << "AY5")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONTIMEINTERVAL" <<
                                                       "value" << 500
                                                       ))
                        );

    connection->insert(ENTITIES_COLL, en1);
    connection->insert(ENTITIES_COLL, en2);
    connection->insert(ENTITIES_COLL, en3);
    connection->insert(ENTITIES_COLL, en4);

    connection->insert(SUBSCRIBECONTEXT_COLL, sub1);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub2);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub2nt);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub3);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub4);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub5);

}

/* ****************************************************************************
*
* prepareDatabasePatternTrue -
*
* This is a variant of populateDatabase function in which all entities have the same type,
* to ease test for isPattern=true cases
*
*/
static void prepareDatabasePatternTrue(void) {

    /* Set database */
    setupDatabase();

    DBClientBase* connection = getMongoConnection();

    /* We create the following entities:
     *
     * - E1:
     *     A1: X     
     *     A2: Z
     *     A3: W
     * - E2
     *     A2: R
     *     A3: S
     * - E2*
     *     A1: T     
     * - E1**
     *     A1: P
     *     A2: Q
     * - E3
     *     A1: noise
     *     A2: noise
     *
     * (*) Means that entity/type is using same name but different type. This is included to check that type is
     *     taken into account.
     * (**)same name but without type
     */

    BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T") <<
                       "attrNames" << BSON_ARRAY("A1" << "A2" << "A3") <<
                       "attrs" << BSON(
                          "A1" << BSON("type" << "TA1" << "value" << "X") <<
                          "A2" << BSON("type" << "TA2" << "value" << "Z") <<
                          "A3" << BSON("type" << "TA3" << "value" << "W")
                          )
                      );

    BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T") <<
                       "attrNames" << BSON_ARRAY("A2" << "A3") <<
                       "attrs" << BSON(
                          "A2" << BSON("type" << "TA2" << "value" << "R") <<
                          "A3" << BSON("type" << "TA3" << "value" << "S")
                          )
                      );

    BSONObj en3 = BSON("_id" << BSON("id" << "E2" << "type" << "Tbis") <<
                       "attrNames" << BSON_ARRAY("A1") <<
                       "attrs" << BSON(
                          "A1" << BSON("type" << "TA1" << "value" << "T")
                          )
                      );

    BSONObj en4 = BSON("_id" << BSON("id" << "E1") <<
                       "attrNames" << BSON_ARRAY("A1" << "A2") <<
                       "attrs" << BSON(
                          "A1" << BSON("type" << "TA1" << "value" << "P") <<
                          "A2" << BSON("type" << "TA2" << "value" << "Q")
                          )
                      );

    BSONObj en5 = BSON("_id" << BSON("id" << "E3") <<
                       "attrNames" << BSON_ARRAY("A1" << "A2") <<
                       "attrs" << BSON(
                          "A1" << BSON("type" << "TA1" << "value" << "noise") <<
                          "A2" << BSON("type" << "TA2" << "value" << "noise")
                          )
                      );

    BSONObj sub1 = BSON("_id" << OID("51307b66f481db11bf860001") <<
                        "expiration" << 10000000 <<
                        "lastNotification" << 15000000 <<
                        "throttling" << 60 <<
                        "reference" << "http://notify1.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false")) <<
                        "attrs" << BSONArray() <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX1" << "AY1")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONTIMEINTERVAL" <<
                                                       "value" << 100
                                                       ))
                        );

    BSONObj sub2 = BSON("_id" << OID("51307b66f481db11bf860002") <<
                        "expiration" << 20000000 <<
                        "lastNotification" << 25000000 <<
                        "reference" << "http://notify2.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E[1-2]" << "type" << "T" << "isPattern" << "true")) <<
                        "attrs" << BSON_ARRAY("A1" << "A2") <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX2" << "AY2")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONTIMEINTERVAL" <<
                                                       "value" << 200
                                                       ))
                        );

    /* No type variant of sub2 */
    BSONObj sub2nt = BSON("_id" << OID("51307b66f481db11bf860022") <<
                        "expiration" << 20000000 <<
                        "lastNotification" << 25000000 <<
                        "reference" << "http://notify2.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E[1-2]" << "isPattern" << "true")) <<
                        "attrs" << BSON_ARRAY("A1" << "A2") <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX2" << "AY2")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONTIMEINTERVAL" <<
                                                       "value" << 200
                                                       ))
                        );

    BSONObj sub3 = BSON("_id" << OID("51307b66f481db11bf860003") <<
                        "expiration" << 30000000 <<
                        "lastNotification" << 35000000 <<
                        "reference" << "http://notify3.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false") <<
                                                 BSON("id" << "E2" << "type" << "T2" << "isPattern" << "false")) <<
                        "attrs" << BSONArray() <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX3" << "AY3")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONTIMEINTERVAL" <<
                                                       "value" << 300
                                                       ))
                        );

    BSONObj sub4 = BSON("_id" << OID("51307b66f481db11bf860004") <<
                        "expiration" << 40000000 <<
                        "lastNotification" << 45000000 <<
                        "reference" << "http://notify4.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false") <<
                                                 BSON("id" << "E2" << "type" << "T2" << "isPattern" << "false")) <<
                        "attrs" << BSON_ARRAY("A1" << "A2") <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX4" << "AY4")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONTIMEINTERVAL" <<
                                                       "value" << 400
                                                       ))
                        );
    BSONObj sub5 = BSON("_id" << OID("51307b66f481db11bf860005") <<
                        "expiration" << 50000000 <<
                        "lastNotification" << 55000000 <<
                        "throttling" << 10 <<
                        "reference" << "http://notify5.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false")) <<
                        "attrs" << BSONArray() <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX5" << "AY5")
                                                       ) <<
                                                   BSON(
                                                       "type" << "ONTIMEINTERVAL" <<
                                                       "value" << 500
                                                       ))
                        );

    connection->insert(ENTITIES_COLL, en1);
    connection->insert(ENTITIES_COLL, en2);
    connection->insert(ENTITIES_COLL, en3);
    connection->insert(ENTITIES_COLL, en4);
    connection->insert(ENTITIES_COLL, en5);

    connection->insert(SUBSCRIBECONTEXT_COLL, sub1);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub2);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub2nt);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub3);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub4);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub5);

}

/* ****************************************************************************
*
* subscriptionNotFound -
*/
TEST(mongoUpdateContextSubscription, subscriptionNotFound)
{
    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads(_))
            .Times(0);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf869999");
    req.duration.set("PT5H");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.subscriptionId.isEmpty());
    EXPECT_EQ(SccContextElementNotFound, res.subscribeError.errorCode.code);
    EXPECT_EQ("No context element found", res.subscribeError.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;
}

/* ****************************************************************************
*
* updateDuration -
*/
TEST(mongoUpdateContextSubscription, updateDuration)
{
    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads(_))
            .Times(0);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    req.duration.set("PT5H");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT5H",res.subscribeResponse.duration.get());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(1360250700, sub.getIntField("expiration"));
    EXPECT_EQ(15000000, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();    
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("AX1", condValues[0].String());
    EXPECT_EQ("AY1", condValues[1].String());
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(100, cond1.getIntField("value"));

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;
    delete timerMock;
}

/* ****************************************************************************
*
* updateRestriction -
*/
TEST(mongoUpdateContextSubscription, DISABLED_updateRestriction)
{
    /* FIXME P5: restrictions not implemented so far. In fact, when we implement restriction we will not
     * only filling this unit test, but also creating test for all the other request in which Restriction
     * can be included
     */
}

/* ****************************************************************************
*
* updateThrottling -
*/
TEST(mongoUpdateContextSubscription, updateThrottling)
{
    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads(_))
            .Times(0);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);    

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860005");
    req.throttling.set("PT4S");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_EQ("PT4S",res.subscribeResponse.throttling.get());
    EXPECT_EQ("51307b66f481db11bf860005", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860005")));

    EXPECT_EQ("51307b66f481db11bf860005", sub.getField("_id").OID().toString());
    EXPECT_EQ(50000000, sub.getIntField("expiration"));
    EXPECT_EQ(55000000, sub.getIntField("lastNotification"));
    EXPECT_EQ(4, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify5.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();    
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("AX5", condValues[0].String());
    EXPECT_EQ("AY5", condValues[1].String());
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(500, cond1.getIntField("value"));

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;
}

/* ****************************************************************************
*
* clearThrottling -
*/
TEST(mongoUpdateContextSubscription, clearThrottling)
{
    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads(_))
            .Times(0);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860005");
    req.throttling.set("PT0S");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_EQ("PT0S",res.subscribeResponse.throttling.get());
    EXPECT_EQ("51307b66f481db11bf860005", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860005")));

    EXPECT_EQ("51307b66f481db11bf860005", sub.getField("_id").OID().toString());
    EXPECT_EQ(50000000, sub.getIntField("expiration"));
    EXPECT_EQ(55000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify5.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("AX5", condValues[0].String());
    EXPECT_EQ("AY5", condValues[1].String());
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(500, cond1.getIntField("value"));

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;
}

/* ****************************************************************************
*
* Ent1_Attr0_T1_C0 -
*/
TEST(mongoUpdateContextSubscription, Ent1_Attr0_T1_C0)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860001", 60, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc;
    nc.type = "ONTIMEINTERVAL";
    nc.condValueList.push_back("PT1M");
    req.notifyConditionVector.push_back(&nc);

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(15000000, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}

/* ****************************************************************************
*
* Ent1_Attr0_T1_C0_JSON -
*/
TEST(mongoUpdateContextSubscription, Ent1_Attr0_T1_C0_JSON)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860001", 60, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc;
    nc.type = "ONTIMEINTERVAL";
    nc.condValueList.push_back("PT1M");
    req.notifyConditionVector.push_back(&nc);

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, JSON, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(15000000, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}

/* ****************************************************************************
*
* Ent1_AttrN_T1_C0 -
*/
TEST(mongoUpdateContextSubscription, Ent1_AttrN_T1_C0)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860002", 60, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc;
    nc.type = "ONTIMEINTERVAL";
    nc.condValueList.push_back("PT1M");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(25000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}

/* ****************************************************************************
*
* Ent1_Attr0_TN_C0 -
*/
TEST(mongoUpdateContextSubscription, Ent1_Attr0_TN_C0)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860001", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860001", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc1, nc2;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(15000000, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}

/* ****************************************************************************
*
* Ent1_AttrN_TN_C0 -
*/
TEST(mongoUpdateContextSubscription, Ent1_AttrN_TN_C0)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860002", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860002", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc1, nc2;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(25000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}

/* ****************************************************************************
*
* Ent1_Attr0_T0_C1 -
*/
TEST(mongoUpdateContextSubscription, Ent1_Attr0_T0_C1)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A10");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(15000000, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}

/* ****************************************************************************
*
* Ent1_AttrN_T0_C1 -
*/
TEST(mongoUpdateContextSubscription, Ent1_AttrN_T0_C1)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A10");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(25000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
* Ent1_Attr0_T0_CN -
*/
TEST(mongoUpdateContextSubscription, Ent1_Attr0_T0_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc1, nc2;
    nc1.type = "ONCHANGE";
    nc1.condValueList.push_back("A10");
    nc2.type = "ONCHANGE";
    nc2.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(15000000, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond1.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A20", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
* Ent1_Attr0_T0_CNbis -
*/
TEST(mongoUpdateContextSubscription, Ent1_Attr0_T0_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A10");
    nc.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc);   

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(15000000, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_EQ("A20", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
*  Ent1_AttrN_T0_CN -
*/
TEST(mongoUpdateContextSubscription, Ent1_AttrN_T0_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc1;
    nc1.type = "ONCHANGE";
    nc1.condValueList.push_back("A10");
    NotifyCondition nc2;
    nc2.type = "ONCHANGE";
    nc2.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(25000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond1.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A20", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
* Ent1_AttrN_T0_CNbis -
*/
TEST(mongoUpdateContextSubscription, Ent1_AttrN_T0_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A10");
    nc.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc);

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(25000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_EQ("A20", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
*  Ent1_Attr0_TN_CN -
*/
TEST(mongoUpdateContextSubscription, Ent1_Attr0_TN_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860001", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860001", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc1, nc2, nc3, nc4;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A10");
    nc4.type = "ONCHANGE";
    nc4.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);
    req.notifyConditionVector.push_back(&nc4);   

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(15000000, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(4, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    BSONObj cond3 = conds[3].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    condValues = cond2.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond3, "type"));
    condValues = cond3.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A20", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
* Ent1_Attr0_TN_CNbis -
*/
TEST(mongoUpdateContextSubscription, Ent1_Attr0_TN_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860001", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860001", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc1, nc2, nc3;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A10");
    nc3.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(15000000, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(3, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    std::vector<BSONElement> condValues = cond2.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_EQ("A20", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
* Ent1_AttrN_TN_CN -
*/
TEST(mongoUpdateContextSubscription, Ent1_AttrN_TN_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860002", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860002", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc1, nc2, nc3, nc4;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A10");
    nc4.type = "ONCHANGE";
    nc4.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);
    req.notifyConditionVector.push_back(&nc4);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(25000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(4, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    BSONObj cond3 = conds[3].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    condValues = cond2.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond3, "type"));
    condValues = cond3.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A20", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
* Ent1_AttrN_TN_CNbis -
*/
TEST(mongoUpdateContextSubscription, Ent1_AttrN_TN_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860002", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860002", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc1, nc2, nc3;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A10");
    nc3.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(25000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(3, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    std::vector<BSONElement> condValues = cond2.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_EQ("A20", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}

/* ****************************************************************************
*
* EntN_Attr0_T1_C0 -
*/
TEST(mongoUpdateContextSubscription, EntN_Attr0_T1_C0)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860003"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860003", 60, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860003");
    NotifyCondition nc;
    nc.type = "ONTIMEINTERVAL";
    nc.condValueList.push_back("PT1M");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860003", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860003")));

    EXPECT_EQ("51307b66f481db11bf860003", sub.getField("_id").OID().toString());
    EXPECT_EQ(30000000, sub.getIntField("expiration"));
    EXPECT_EQ(35000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify3.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}

/* ****************************************************************************
*
* EntN_AttrN_T1_C0 -
*/
TEST(mongoUpdateContextSubscription, EntN_AttrN_T1_C0)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860004"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860004", 60, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860004");
    NotifyCondition nc;
    nc.type = "ONTIMEINTERVAL";
    nc.condValueList.push_back("PT1M");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860004", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860004")));

    EXPECT_EQ("51307b66f481db11bf860004", sub.getField("_id").OID().toString());
    EXPECT_EQ(40000000, sub.getIntField("expiration"));
    EXPECT_EQ(45000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify4.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}

/* ****************************************************************************
*
* EntN_Attr0_TN_C0 -
*/
TEST(mongoUpdateContextSubscription, EntN_Attr0_TN_C0)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860003"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860003", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860003", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860003");
    NotifyCondition nc1, nc2;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860003", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860003")));

    EXPECT_EQ("51307b66f481db11bf860003", sub.getField("_id").OID().toString());
    EXPECT_EQ(30000000, sub.getIntField("expiration"));
    EXPECT_EQ(35000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify3.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}

/* ****************************************************************************
*
* EntN_AttrN_TN_C0 -
*/
TEST(mongoUpdateContextSubscription, EntN_AttrN_TN_C0)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860004"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860004", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860004", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860004");
    NotifyCondition nc1, nc2;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860004", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860004")));

    EXPECT_EQ("51307b66f481db11bf860004", sub.getField("_id").OID().toString());
    EXPECT_EQ(40000000, sub.getIntField("expiration"));
    EXPECT_EQ(45000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify4.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}

/* ****************************************************************************
*
* EntN_Attr0_T0_C1 -
*/
TEST(mongoUpdateContextSubscription, EntN_Attr0_T0_C1)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860003"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860003");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A10");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860003", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860003")));

    EXPECT_EQ("51307b66f481db11bf860003", sub.getField("_id").OID().toString());
    EXPECT_EQ(30000000, sub.getIntField("expiration"));
    EXPECT_EQ(35000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify3.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}

/* ****************************************************************************
*
* EntN_AttrN_T0_C1 -
*/
TEST(mongoUpdateContextSubscription, EntN_AttrN_T0_C1)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860004"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860004");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A10");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860004", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860004")));

    EXPECT_EQ("51307b66f481db11bf860004", sub.getField("_id").OID().toString());
    EXPECT_EQ(40000000, sub.getIntField("expiration"));
    EXPECT_EQ(45000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify4.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
* EntN_Attr0_T0_CN -
*/
TEST(mongoUpdateContextSubscription, EntN_Attr0_T0_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860003"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860003");
    NotifyCondition nc1, nc2;
    nc1.type = "ONCHANGE";
    nc1.condValueList.push_back("A10");
    nc2.type = "ONCHANGE";
    nc2.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860003", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860003")));

    EXPECT_EQ("51307b66f481db11bf860003", sub.getField("_id").OID().toString());
    EXPECT_EQ(30000000, sub.getIntField("expiration"));
    EXPECT_EQ(35000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify3.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond1.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A20", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
* EntN_Attr0_T0_CNbis -
*/
TEST(mongoUpdateContextSubscription, EntN_Attr0_T0_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860003"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860003");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A10");
    nc.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860003", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860003")));

    EXPECT_EQ("51307b66f481db11bf860003", sub.getField("_id").OID().toString());
    EXPECT_EQ(30000000, sub.getIntField("expiration"));
    EXPECT_EQ(35000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify3.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_EQ("A20", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
* EntN_AttrN_T0_CN -
*/
TEST(mongoUpdateContextSubscription, EntN_AttrN_T0_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860004"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860004");
    NotifyCondition nc1, nc2;
    nc1.type = "ONCHANGE";
    nc1.condValueList.push_back("A10");
    nc2.type = "ONCHANGE";
    nc2.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860004", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860004")));

    EXPECT_EQ("51307b66f481db11bf860004", sub.getField("_id").OID().toString());
    EXPECT_EQ(40000000, sub.getIntField("expiration"));
    EXPECT_EQ(45000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify4.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond1.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A20", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
* EntN_AttrN_T0_CNbis -
*/
TEST(mongoUpdateContextSubscription, EntN_AttrN_T0_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860004"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860004");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A10");
    nc.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860004", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860004")));

    EXPECT_EQ("51307b66f481db11bf860004", sub.getField("_id").OID().toString());
    EXPECT_EQ(40000000, sub.getIntField("expiration"));
    EXPECT_EQ(45000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify4.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_EQ("A20", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
*  EntN_Attr0_TN_CN -
*/
TEST(mongoUpdateContextSubscription, EntN_Attr0_TN_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860003"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860003", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860003", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860003");
    NotifyCondition nc1, nc2, nc3, nc4;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A10");
    nc4.type = "ONCHANGE";
    nc4.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);
    req.notifyConditionVector.push_back(&nc4);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860003", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860003")));

    EXPECT_EQ("51307b66f481db11bf860003", sub.getField("_id").OID().toString());
    EXPECT_EQ(30000000, sub.getIntField("expiration"));
    EXPECT_EQ(35000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify3.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(4, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    BSONObj cond3 = conds[3].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    condValues = cond2.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond3, "type"));
    condValues = cond3.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A20", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
* EntN_Attr0_TN_CNbis -
*/
TEST(mongoUpdateContextSubscription, EntN_Attr0_TN_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860003"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860003", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860003", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860003");
    NotifyCondition nc1, nc2, nc3;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A10");
    nc3.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860003", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860003")));

    EXPECT_EQ("51307b66f481db11bf860003", sub.getField("_id").OID().toString());
    EXPECT_EQ(30000000, sub.getIntField("expiration"));
    EXPECT_EQ(35000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify3.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(3, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    std::vector<BSONElement> condValues = cond2.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_EQ("A20", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
* EntN_AttrN_TN_CN -
*/
TEST(mongoUpdateContextSubscription, EntN_AttrN_TN_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860004"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860004", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860004", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860004");
    NotifyCondition nc1, nc2, nc3, nc4;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A10");
    nc4.type = "ONCHANGE";
    nc4.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);
    req.notifyConditionVector.push_back(&nc4);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860004", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860004")));

    EXPECT_EQ("51307b66f481db11bf860004", sub.getField("_id").OID().toString());
    EXPECT_EQ(40000000, sub.getIntField("expiration"));
    EXPECT_EQ(45000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify4.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(4, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    BSONObj cond3 = conds[3].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    condValues = cond2.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond3, "type"));
    condValues = cond3.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A20", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

}
/* ****************************************************************************
*
* EntN_AttrN_TN_CNbis -
*/
TEST(mongoUpdateContextSubscription, EntN_AttrN_TN_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860004"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860004", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860004", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860004");
    NotifyCondition nc1, nc2, nc3;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A10");
    nc3.condValueList.push_back("A20");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860004", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860004")));

    EXPECT_EQ("51307b66f481db11bf860004", sub.getField("_id").OID().toString());
    EXPECT_EQ(40000000, sub.getIntField("expiration"));
    EXPECT_EQ(45000000, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify4.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(3, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    std::vector<BSONElement> condValues = cond2.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A10", condValues[0].String());
    EXPECT_EQ("A20", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

}



/* ****************************************************************************
*
* matchEnt1_Attr0_T0_C1 -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_Attr0_T0_C1)
{  
    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");    
    ContextAttribute ca2("A2", "TA2", "Z");
    ContextAttribute ca3("A3", "TA3", "W");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    cer.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify1.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A1");
    req.notifyConditionVector.push_back(&nc);

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEnt1_Attr0_T0_C1_JSON -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_Attr0_T0_C1_JSON)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    ContextAttribute ca3("A3", "TA3", "W");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    cer.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify1.me", "", "", JSON))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A1");
    req.notifyConditionVector.push_back(&nc);

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, JSON, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("JSON", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEnt1_AttrN_T0_C1 -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_AttrN_T0_C1)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");    
    ContextAttribute ca2("A2", "TA2", "Z");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A1");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEnt1_AttrN_T0_C1_disjoint -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_AttrN_T0_C1_disjoint)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A3");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A3", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEnt1NoType_AttrN_T0_C1 -
*/
TEST(mongoUpdateContextSubscription, matchEnt1NoType_AttrN_T0_C1)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2, cer3;

    cer1.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");    
    ContextAttribute ca2("A2", "TA2", "Z");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);

    cer2.contextElement.entityId.fill("E1", "T1bis", "false");
    ContextAttribute ca3("A1", "TA1", "T");
    cer2.contextElement.contextAttributeVector.push_back(&ca3);

    cer3.contextElement.entityId.fill("E1", "", "false");
    ContextAttribute ca4("A1", "TA1", "P");
    ContextAttribute ca5("A2", "TA2", "Q");
    cer3.contextElement.contextAttributeVector.push_back(&ca4);
    cer3.contextElement.contextAttributeVector.push_back(&ca5);

    expectedNcr.contextElementResponseVector.push_back(&cer1);
    expectedNcr.contextElementResponseVector.push_back(&cer2);
    expectedNcr.contextElementResponseVector.push_back(&cer3);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860022"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860022");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A1");
    req.notifyConditionVector.push_back(&nc);

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860022", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860022")));

    EXPECT_EQ("51307b66f481db11bf860022", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEnt1NoType_AttrN_T0_C1_disjoint -
*/
TEST(mongoUpdateContextSubscription, matchEnt1NoType_AttrN_T0_C1_disjoint)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2, cer3;

    cer1.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");   
    ContextAttribute ca2("A2", "TA2", "Z");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);

    cer2.contextElement.entityId.fill("E1", "T1bis", "false");
    ContextAttribute ca3("A1", "TA1", "T");
    cer2.contextElement.contextAttributeVector.push_back(&ca3);

    cer3.contextElement.entityId.fill("E1", "", "false");
    ContextAttribute ca4("A1", "TA1", "P");
    ContextAttribute ca5("A2", "TA2", "Q");
    cer3.contextElement.contextAttributeVector.push_back(&ca4);
    cer3.contextElement.contextAttributeVector.push_back(&ca5);

    expectedNcr.contextElementResponseVector.push_back(&cer1);
    expectedNcr.contextElementResponseVector.push_back(&cer2);
    expectedNcr.contextElementResponseVector.push_back(&cer3);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860022"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860022");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A3");
    req.notifyConditionVector.push_back(&nc);

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860022", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860022")));

    EXPECT_EQ("51307b66f481db11bf860022", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A3", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEnt1Pattern_AttrN_T0_C1 -
*/
TEST(mongoUpdateContextSubscription, matchEnt1Pattern_AttrN_T0_C1)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2;

    cer1.contextElement.entityId.fill("E1", "T", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);

    cer2.contextElement.entityId.fill("E2", "T", "false");
    ContextAttribute ca3("A2", "TA2", "R");
    cer2.contextElement.contextAttributeVector.push_back(&ca3);

    expectedNcr.contextElementResponseVector.push_back(&cer1);
    expectedNcr.contextElementResponseVector.push_back(&cer2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A1");
    req.notifyConditionVector.push_back(&nc);

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEnt1Pattern_AttrN_T0_C1_disjoint -
*/
TEST(mongoUpdateContextSubscription, matchEnt1Pattern_AttrN_T0_C1_disjoint)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2;

    cer1.contextElement.entityId.fill("E1", "T", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);

    cer2.contextElement.entityId.fill("E2", "T", "false");
    ContextAttribute ca3("A2", "TA2", "R");
    cer2.contextElement.contextAttributeVector.push_back(&ca3);

    expectedNcr.contextElementResponseVector.push_back(&cer1);
    expectedNcr.contextElementResponseVector.push_back(&cer2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A3");
    req.notifyConditionVector.push_back(&nc);

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A3", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEnt1PatternNoType_AttrN_T0_C1 -
*/
TEST(mongoUpdateContextSubscription, matchEnt1PatternNoType_AttrN_T0_C1)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2, cer3, cer4;

    cer1.contextElement.entityId.fill("E1", "T", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);

    cer2.contextElement.entityId.fill("E2", "T", "false");
    ContextAttribute ca3("A2", "TA2", "R");
    cer2.contextElement.contextAttributeVector.push_back(&ca3);

    cer3.contextElement.entityId.fill("E2", "Tbis", "false");
    ContextAttribute ca4("A1", "TA1", "T");
    cer3.contextElement.contextAttributeVector.push_back(&ca4);

    cer4.contextElement.entityId.fill("E1", "", "false");
    ContextAttribute ca5("A1", "TA1", "P");
    ContextAttribute ca6("A2", "TA2", "Q");
    cer4.contextElement.contextAttributeVector.push_back(&ca5);
    cer4.contextElement.contextAttributeVector.push_back(&ca6);

    expectedNcr.contextElementResponseVector.push_back(&cer1);
    expectedNcr.contextElementResponseVector.push_back(&cer2);
    expectedNcr.contextElementResponseVector.push_back(&cer3);
    expectedNcr.contextElementResponseVector.push_back(&cer4);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860022"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860022");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A1");
    req.notifyConditionVector.push_back(&nc);

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860022", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860022")));

    EXPECT_EQ("51307b66f481db11bf860022", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E[1-2]", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEnt1PatternNoType_AttrN_T0_C1_disjoint -
*/
TEST(mongoUpdateContextSubscription, matchEnt1PatternNoType_AttrN_T0_C1_disjoint)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2, cer3, cer4;

    cer1.contextElement.entityId.fill("E1", "T", "false");
    ContextAttribute ca1("A1", "TA1", "X");    
    ContextAttribute ca2("A2", "TA2", "Z");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);    

    cer2.contextElement.entityId.fill("E2", "T", "false");
    ContextAttribute ca3("A2", "TA2", "R");
    cer2.contextElement.contextAttributeVector.push_back(&ca3);

    cer3.contextElement.entityId.fill("E2", "Tbis", "false");
    ContextAttribute ca4("A1", "TA1", "T");
    cer3.contextElement.contextAttributeVector.push_back(&ca4);

    cer4.contextElement.entityId.fill("E1", "", "false");
    ContextAttribute ca5("A1", "TA1", "P");
    ContextAttribute ca6("A2", "TA2", "Q");
    cer4.contextElement.contextAttributeVector.push_back(&ca5);
    cer4.contextElement.contextAttributeVector.push_back(&ca6);

    expectedNcr.contextElementResponseVector.push_back(&cer1);
    expectedNcr.contextElementResponseVector.push_back(&cer2);
    expectedNcr.contextElementResponseVector.push_back(&cer3);
    expectedNcr.contextElementResponseVector.push_back(&cer4);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860022"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860022");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A3");
    req.notifyConditionVector.push_back(&nc);

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860022", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860022")));

    EXPECT_EQ("51307b66f481db11bf860022", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E[1-2]", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A3", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEnt1_Attr0_T0_CN -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_Attr0_T0_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    ContextAttribute ca3("A3", "TA3", "W");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    cer.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify1.me", "", "", XML))
            .Times(2);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc1, nc2;
    nc1.type = "ONCHANGE";
    nc1.condValueList.push_back("A1");
    nc2.type = "ONCHANGE";
    nc2.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond1.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A2", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEnt1_Attr0_T0_CN_partial -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_Attr0_T0_CN_partial)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");    
    ContextAttribute ca2("A2", "TA2", "Z");
    ContextAttribute ca3("A3", "TA3", "W");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    cer.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify1.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc1, nc2;
    nc1.type = "ONCHANGE";
    nc1.condValueList.push_back("A1");
    nc2.type = "ONCHANGE";
    nc2.condValueList.push_back("A5");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond1.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A5", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}


/* ****************************************************************************
*
* matchEnt1_Attr0_T0_CNbis -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_Attr0_T0_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    ContextAttribute ca3("A3", "TA3", "W");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    cer.contextElement.contextAttributeVector.push_back(&ca3);    
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify1.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A1");
    nc.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_EQ("A2", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
*  matchEnt1_AttrN_T0_CN_disjoint -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_AttrN_T0_CN_disjoint)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(2);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc1, nc2;
    nc1.type = "ONCHANGE";
    nc1.condValueList.push_back("A1");
    nc2.type = "ONCHANGE";
    nc2.condValueList.push_back("A3");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond1.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A3", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);


    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
*  matchEnt1_AttrN_T0_CN_partial -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_AttrN_T0_CN_partial)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc1, nc2;
    nc1.type = "ONCHANGE";
    nc1.condValueList.push_back("A1");
    nc2.type = "ONCHANGE";
    nc2.condValueList.push_back("A5");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond1.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A5", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
*  matchEnt1_AttrN_T0_CN_partial_disjoint -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_AttrN_T0_CN_partial_disjoint)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc1, nc2;
    nc1.type = "ONCHANGE";
    nc1.condValueList.push_back("A3");
    nc2.type = "ONCHANGE";
    nc2.condValueList.push_back("A5");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A3", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond1.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A5", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEnt1_AttrN_T0_CNbis -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_AttrN_T0_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A1");
    nc.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_EQ("A2", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}
/* ****************************************************************************
*
*  matchEnt1_Attr0_TN_CN -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_Attr0_TN_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");    
    ContextAttribute ca2("A2", "TA2", "Z");
    ContextAttribute ca3("A3", "TA3", "W");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    cer.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify1.me", "", "", XML))
            .Times(2);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860001", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860001", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc1, nc2, nc3, nc4;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A1");
    nc4.type = "ONCHANGE";
    nc4.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);
    req.notifyConditionVector.push_back(&nc4);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(4, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    BSONObj cond3 = conds[3].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    condValues = cond2.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond3, "type"));
    condValues = cond3.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A2", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}
/* ****************************************************************************
*
* matchEnt1_Attr0_TN_CNbis -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_Attr0_TN_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");    
    ContextAttribute ca2("A2", "TA2", "Z");
    ContextAttribute ca3("A3", "TA3", "W");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    cer.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify1.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860001", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860001", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc1, nc2, nc3;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A1");
    nc3.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(10000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(3, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    std::vector<BSONElement> condValues = cond2.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_EQ("A2", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}
/* ****************************************************************************
*
* matchEnt1_AttrN_TN_CN -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_AttrN_TN_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(2);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860002", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860002", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc1, nc2, nc3, nc4;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A1");
    nc4.type = "ONCHANGE";
    nc4.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);
    req.notifyConditionVector.push_back(&nc4);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();    
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(4, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    BSONObj cond3 = conds[3].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    condValues = cond2.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond3, "type"));
    condValues = cond3.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A2", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}
/* ****************************************************************************
*
* matchEnt1_AttrN_TN_CNbis -
*/
TEST(mongoUpdateContextSubscription, matchEnt1_AttrN_TN_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer;
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");    
    ContextAttribute ca2("A2", "TA2", "Z");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    expectedNcr.contextElementResponseVector.push_back(&cer);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860002"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify2.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860002", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860002", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860002");
    NotifyCondition nc1, nc2, nc3;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A1");
    nc3.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860002", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));

    EXPECT_EQ("51307b66f481db11bf860002", sub.getField("_id").OID().toString());
    EXPECT_EQ(20000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify2.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(3, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    std::vector<BSONElement> condValues = cond2.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_EQ("A2", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEntN_Attr0_T0_C1 -
*/
TEST(mongoUpdateContextSubscription, matchEntN_Attr0_T0_C1)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2;
    cer1.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");    
    ContextAttribute ca2("A2", "TA2", "Z");
    ContextAttribute ca3("A3", "TA3", "W");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);
    cer1.contextElement.contextAttributeVector.push_back(&ca3);    
    expectedNcr.contextElementResponseVector.push_back(&cer1);
    cer2.contextElement.entityId.id = "E2";
    cer2.contextElement.entityId.type = "T2";
    cer2.contextElement.entityId.isPattern = "false";
    ContextAttribute ca4("A2", "TA2", "R");
    ContextAttribute ca5("A3", "TA3", "S");
    cer2.contextElement.contextAttributeVector.push_back(&ca4);
    cer2.contextElement.contextAttributeVector.push_back(&ca5);
    expectedNcr.contextElementResponseVector.push_back(&cer2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860003"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify3.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860003");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A1");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860003", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860003")));

    EXPECT_EQ("51307b66f481db11bf860003", sub.getField("_id").OID().toString());
    EXPECT_EQ(30000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify3.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* matchEntN_AttrN_T0_C1 -
*/
TEST(mongoUpdateContextSubscription, matchEntN_AttrN_T0_C1)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2;
    cer1.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);
    expectedNcr.contextElementResponseVector.push_back(&cer1);
    cer2.contextElement.entityId.id = "E2";
    cer2.contextElement.entityId.type = "T2";
    cer2.contextElement.entityId.isPattern = "false";
    ContextAttribute ca3("A2", "TA2", "R");
    cer2.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860004"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify4.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860004");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A1");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860004", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860004")));

    EXPECT_EQ("51307b66f481db11bf860004", sub.getField("_id").OID().toString());
    EXPECT_EQ(40000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify4.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}
/* ****************************************************************************
*
* matchEntN_Attr0_T0_CN -
*/
TEST(mongoUpdateContextSubscription, matchEntN_Attr0_T0_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2;
    cer1.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    ContextAttribute ca3("A3", "TA3", "W");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);
    cer1.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer1);
    cer2.contextElement.entityId.id = "E2";
    cer2.contextElement.entityId.type = "T2";
    cer2.contextElement.entityId.isPattern = "false";
    ContextAttribute ca4("A2", "TA2", "R");
    ContextAttribute ca5("A3", "TA3", "S");
    cer2.contextElement.contextAttributeVector.push_back(&ca4);
    cer2.contextElement.contextAttributeVector.push_back(&ca5);
    expectedNcr.contextElementResponseVector.push_back(&cer2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860003"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify3.me", "", "", XML))
            .Times(2);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860003");
    NotifyCondition nc1, nc2;
    nc1.type = "ONCHANGE";
    nc1.condValueList.push_back("A1");
    nc2.type = "ONCHANGE";
    nc2.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860003", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860003")));

    EXPECT_EQ("51307b66f481db11bf860003", sub.getField("_id").OID().toString());
    EXPECT_EQ(30000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify3.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond1.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A2", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}
/* ****************************************************************************
*
* matchEntN_Attr0_T0_CNbis -
*/
TEST(mongoUpdateContextSubscription, matchEntN_Attr0_T0_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2;
    cer1.contextElement.entityId.fill ("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    ContextAttribute ca3("A3", "TA3", "W");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);
    cer1.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer1);
    cer2.contextElement.entityId.id = "E2";
    cer2.contextElement.entityId.type = "T2";
    cer2.contextElement.entityId.isPattern = "false";
    ContextAttribute ca4("A2", "TA2", "R");
    ContextAttribute ca5("A3", "TA3", "S");
    cer2.contextElement.contextAttributeVector.push_back(&ca4);
    cer2.contextElement.contextAttributeVector.push_back(&ca5);
    expectedNcr.contextElementResponseVector.push_back(&cer2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860003"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify3.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860003");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A1");
    nc.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860003", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860003")));

    EXPECT_EQ("51307b66f481db11bf860003", sub.getField("_id").OID().toString());
    EXPECT_EQ(30000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify3.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_EQ("A2", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}
/* ****************************************************************************
*
* matchEntN_AttrN_T0_CN -
*/
TEST(mongoUpdateContextSubscription, matchEntN_AttrN_T0_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2;
    cer1.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");    
    ContextAttribute ca2("A2", "TA2", "Z");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);
    expectedNcr.contextElementResponseVector.push_back(&cer1);
    cer2.contextElement.entityId.id = "E2";
    cer2.contextElement.entityId.type = "T2";
    cer2.contextElement.entityId.isPattern = "false";
    ContextAttribute ca3("A2", "TA2", "R");
    cer2.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860004"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify4.me", "", "", XML))
            .Times(2);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860004");
    NotifyCondition nc1, nc2;
    nc1.type = "ONCHANGE";
    nc1.condValueList.push_back("A1");
    nc2.type = "ONCHANGE";
    nc2.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860004", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860004")));

    EXPECT_EQ("51307b66f481db11bf860004", sub.getField("_id").OID().toString());
    EXPECT_EQ(40000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify4.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(2, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond0.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    condValues = cond1.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A2", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}
/* ****************************************************************************
*
* matchEntN_AttrN_T0_CNbis -
*/
TEST(mongoUpdateContextSubscription, matchEntN_AttrN_T0_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2;
    cer1.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);
    expectedNcr.contextElementResponseVector.push_back(&cer1);
    cer2.contextElement.entityId.fill("E2", "T2", "false");
    ContextAttribute ca3("A2", "TA2", "R");
    cer2.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860004"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify4.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860004");
    NotifyCondition nc;
    nc.type = "ONCHANGE";
    nc.condValueList.push_back("A1");
    nc.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860004", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860004")));

    EXPECT_EQ("51307b66f481db11bf860004", sub.getField("_id").OID().toString());
    EXPECT_EQ(40000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify4.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond0, "type"));
    std::vector<BSONElement> condValues = cond0.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_EQ("A2", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}
/* ****************************************************************************
*
*  matchEntN_Attr0_TN_CN -
*/
TEST(mongoUpdateContextSubscription, matchEntN_Attr0_TN_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2;
    cer1.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");    
    ContextAttribute ca2("A2", "TA2", "Z");
    ContextAttribute ca3("A3", "TA3", "W");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);
    cer1.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer1);
    cer2.contextElement.entityId.fill("E2", "T2", "false");
    ContextAttribute ca4("A2", "TA2", "R");
    ContextAttribute ca5("A3", "TA3", "S");
    cer2.contextElement.contextAttributeVector.push_back(&ca4);
    cer2.contextElement.contextAttributeVector.push_back(&ca5);
    expectedNcr.contextElementResponseVector.push_back(&cer2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860003"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify3.me", "", "", XML))
            .Times(2);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860003", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860003", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860003");
    NotifyCondition nc1, nc2, nc3, nc4;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A1");
    nc4.type = "ONCHANGE";
    nc4.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);
    req.notifyConditionVector.push_back(&nc4);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860003", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860003")));

    EXPECT_EQ("51307b66f481db11bf860003", sub.getField("_id").OID().toString());
    EXPECT_EQ(30000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify3.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(4, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    BSONObj cond3 = conds[3].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    condValues = cond2.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond3, "type"));
    condValues = cond3.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A2", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}
/* ****************************************************************************
*
* matchEntN_Attr0_TN_CNbis -
*/
TEST(mongoUpdateContextSubscription, matchEntN_Attr0_TN_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2;
    cer1.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");    
    ContextAttribute ca2("A2", "TA2", "Z");
    ContextAttribute ca3("A3", "TA3", "W");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);
    cer1.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer1);
    cer2.contextElement.entityId.fill("E2", "T2", "false");
    ContextAttribute ca4("A2", "TA2", "R");
    ContextAttribute ca5("A3", "TA3", "S");
    cer2.contextElement.contextAttributeVector.push_back(&ca4);
    cer2.contextElement.contextAttributeVector.push_back(&ca5);
    expectedNcr.contextElementResponseVector.push_back(&cer2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860003"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify3.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860003", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860003", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860003");
    NotifyCondition nc1, nc2, nc3;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A1");
    nc3.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860003", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860003")));

    EXPECT_EQ("51307b66f481db11bf860003", sub.getField("_id").OID().toString());
    EXPECT_EQ(30000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify3.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(3, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    std::vector<BSONElement> condValues = cond2.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_EQ("A2", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}
/* ****************************************************************************
*
* matchEntN_AttrN_TN_CN -
*/
TEST(mongoUpdateContextSubscription, matchEntN_AttrN_TN_CN)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2;
    cer1.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");
    ContextAttribute ca2("A2", "TA2", "Z");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);
    expectedNcr.contextElementResponseVector.push_back(&cer1);
    cer2.contextElement.entityId.fill("E2", "T2", "false");
    ContextAttribute ca3("A2", "TA2", "R");
    cer2.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860004"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify4.me", "", "", XML))
            .Times(2);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860004", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860004", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860004");
    NotifyCondition nc1, nc2, nc3, nc4;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A1");
    nc4.type = "ONCHANGE";
    nc4.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);
    req.notifyConditionVector.push_back(&nc4);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860004", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860004")));

    EXPECT_EQ("51307b66f481db11bf860004", sub.getField("_id").OID().toString());
    EXPECT_EQ(40000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify4.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    std::vector<BSONElement> condValues;
    ASSERT_EQ(4, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    BSONObj cond3 = conds[3].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    condValues = cond2.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond3, "type"));
    condValues = cond3.getField("value").Array();
    ASSERT_EQ(1, condValues.size());
    EXPECT_EQ("A2", condValues[0].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}
/* ****************************************************************************
*
* matcnEntN_AttrN_TN_CNbis -
*/
TEST(mongoUpdateContextSubscription, matchEntN_AttrN_TN_CNbis)
{

    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse cer1, cer2;
    cer1.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "X");    
    ContextAttribute ca2("A2", "TA2", "Z");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);    
    expectedNcr.contextElementResponseVector.push_back(&cer1);
    cer2.contextElement.entityId.fill("E2", "T2", "false");
    ContextAttribute ca3("A2", "TA2", "R");
    cer2.contextElement.contextAttributeVector.push_back(&ca3);
    expectedNcr.contextElementResponseVector.push_back(&cer2);

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860004"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),"http://notify4.me", "", "", XML))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860004", 60, ""))
            .Times(1);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860004", 120, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860004");
    NotifyCondition nc1, nc2, nc3;
    nc1.type = "ONTIMEINTERVAL";
    nc1.condValueList.push_back("PT1M");
    nc2.type = "ONTIMEINTERVAL";
    nc2.condValueList.push_back("PT2M");
    nc3.type = "ONCHANGE";
    nc3.condValueList.push_back("A1");
    nc3.condValueList.push_back("A2");
    req.notifyConditionVector.push_back(&nc1);
    req.notifyConditionVector.push_back(&nc2);
    req.notifyConditionVector.push_back(&nc3);    

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeResponse.duration.isEmpty());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860004", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860004")));

    EXPECT_EQ("51307b66f481db11bf860004", sub.getField("_id").OID().toString());
    EXPECT_EQ(40000000, sub.getIntField("expiration"));
    EXPECT_EQ(1360232700, sub.getIntField("lastNotification"));
    EXPECT_FALSE(sub.hasField("throttling"));
    EXPECT_STREQ("http://notify4.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

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
    ASSERT_EQ(2, attrs.size());
    EXPECT_EQ("A1", attrs[0].String());
    EXPECT_EQ("A2", attrs[1].String());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(3, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    BSONObj cond1 = conds[1].embeddedObject();
    BSONObj cond2 = conds[2].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond1, "type"));
    EXPECT_EQ(120, cond1.getIntField("value"));
    EXPECT_STREQ("ONCHANGE", C_STR_FIELD(cond2, "type"));
    std::vector<BSONElement> condValues = cond2.getField("value").Array();
    ASSERT_EQ(2, condValues.size());
    EXPECT_EQ("A1", condValues[0].String());
    EXPECT_EQ("A2", condValues[1].String());

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* updateDurationAndNotifyConditions -
*/
TEST(mongoUpdateContextSubscription, updateDurationAndNotifyConditions)
{
    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    utInit();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads("51307b66f481db11bf860001"))
            .Times(1);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread("51307b66f481db11bf860001", 60, ""))
            .Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    NotifyCondition nc;
    nc.type = "ONTIMEINTERVAL";
    nc.condValueList.push_back("PT1M");
    req.notifyConditionVector.push_back(&nc);
    req.duration.set("PT5H");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT5H",res.subscribeResponse.duration.get());
    EXPECT_TRUE(res.subscribeResponse.throttling.isEmpty());
    EXPECT_EQ("51307b66f481db11bf860001", res.subscribeResponse.subscriptionId.get());
    EXPECT_EQ(SccNone, res.subscribeError.errorCode.code);
    EXPECT_EQ(0, res.subscribeError.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.subscribeError.errorCode.details.size());

    /* Check database is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    ASSERT_EQ(6, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));

    EXPECT_EQ("51307b66f481db11bf860001", sub.getField("_id").OID().toString());
    EXPECT_EQ(1360250700, sub.getIntField("expiration"));
    EXPECT_EQ(15000000, sub.getIntField("lastNotification"));
    EXPECT_EQ(60, sub.getIntField("throttling"));
    EXPECT_STREQ("http://notify1.me", C_STR_FIELD(sub, "reference"));
    EXPECT_STREQ("XML", C_STR_FIELD(sub, "format"));

    std::vector<BSONElement> entities = sub.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));

    std::vector<BSONElement> attrs = sub.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    std::vector<BSONElement> conds = sub.getField("conditions").Array();
    ASSERT_EQ(1, conds.size());
    BSONObj cond0 = conds[0].embeddedObject();
    EXPECT_STREQ("ONTIMEINTERVAL", C_STR_FIELD(cond0, "type"));
    EXPECT_EQ(60, cond0.getIntField("value"));

    /* Release connection */
    setMongoConnectionForUnitTest(NULL);

    /* Release mock */
    delete notifierMock;

    utExit();

}

/* ****************************************************************************
*
* MongoDbFindOneFail -
*
*/
TEST(mongoUpdateContextSubscription, MongoDbFindOneFail)
{
    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mocks */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, findOne("unittest.csubs",_,_,_))
            .WillByDefault(Throw(e));

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads(_))
            .Times(0);
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads(_))
            .Times(0);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    req.duration.set("PT5H");

    /* Set MongoDB connection (prepare database first with the "actual" connection object) */
    prepareDatabase();
    setMongoConnectionForUnitTest(connectionMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeError.subscriptionId.isEmpty());
    EXPECT_EQ(SccReceiverInternalError, res.subscribeError.errorCode.code);
    EXPECT_EQ("Internal Server Error", res.subscribeError.errorCode.reasonPhrase);
    EXPECT_EQ("collection: unittest.csubs "
              "- findOne() _id: 51307b66f481db11bf860001 "
              "- exception: boom!!", res.subscribeError.errorCode.details);

    /* Release mocks */
    setMongoConnectionForUnitTest(NULL);
    delete notifierMock;
    delete connectionMock;

}

/* ****************************************************************************
*
* MongoDbUpdateFail -
*
*/
TEST(mongoUpdateContextSubscription, MongoDbUpdateFail)
{
    HttpStatusCode                    ms;
    UpdateContextSubscriptionRequest  req;
    UpdateContextSubscriptionResponse res;

    /* Prepare mocks */
    const DBException e = DBException("boom!!", 33);
    BSONObj fakeSub = BSON("_id" << OID("51307b66f481db11bf860001") <<
                                       "expiration" << 10000000 <<
                                       "lastNotification" << 15000000 <<
                                       "reference" << "http://notify1.me" <<
                                       "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false")) <<
                                       "attrs" << BSONArray() <<
                                       "conditions" << BSONArray());

    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, update("unittest.csubs",_,_,_,_,_))
            .WillByDefault(Throw(e));
    ON_CALL(*connectionMock, findOne("unittest.csubs",_,_,_))
            .WillByDefault(Return(fakeSub));

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads(_))
            .Times(0);
    EXPECT_CALL(*notifierMock, destroyOntimeIntervalThreads(_))
            .Times(0);
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,_,_,_,_))
            .Times(0);    
    EXPECT_CALL(*notifierMock, createIntervalThread(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    req.subscriptionId.set("51307b66f481db11bf860001");
    req.duration.set("PT5H");

    /* Set MongoDB connection (prepare database first with the "actual" connection object) */
    prepareDatabase();
    setMongoConnectionForUnitTest(connectionMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContextSubscription(&req, &res, XML, "", "", emptyServicePathV);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.subscribeError.subscriptionId.isEmpty());
    EXPECT_EQ(SccReceiverInternalError, res.subscribeError.errorCode.code);
    EXPECT_EQ("Internal Server Error", res.subscribeError.errorCode.reasonPhrase);
    EXPECT_EQ("collection: unittest.csubs "
              "- update() _id: 51307b66f481db11bf860001 "
              "- update() doc: { entities: [ { id: \"E1\", type: \"T1\", isPattern: \"false\" } ], attrs: [], reference: \"http://notify1.me\", expiration: 1360250700, conditions: [], lastNotification: 15000000, format: \"XML\" } "
              "- exception: boom!!", res.subscribeError.errorCode.details);

    /* Release mocks */
    setMongoConnectionForUnitTest(NULL);
    delete notifierMock;
    delete connectionMock;
    delete timerMock;
}
