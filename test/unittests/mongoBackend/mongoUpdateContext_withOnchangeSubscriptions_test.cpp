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

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "mongoBackend/mongoConnectionPool.h"
#include "cache/subCache.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"

#include "unittests/unittest.h"



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
using mongo::BSONNULL;
using ::testing::Throw;
using ::testing::_;



extern void setMongoConnectionForUnitTest(orion::DBClientBase _connection);



/* ****************************************************************************
*
* Tests
*
* - Cond1_updateMatch
* - Cond1_appendMatch
* - Cond1_deleteMatch
* - Cond1_updateMatch_no_type
* - Cond1_appendMatch_no_type
* - Cond1_deleteMatch_no_type
* - Cond1_updateMatch_pattern
* - Cond1_appendMatch_pattern
* - Cond1_deleteMatch_pattern
* - Cond1_updateMatch_pattern_noType
* - Cond1_appendMatch_pattern_noType
* - Cond1_deleteMatch_pattern_noType
* - Cond1_updateMatchDisjoint
* - Cond1_appendMatchDisjoint
* - Cond1_deleteMatchDisjoint
* - Cond1_updateNoMatch
* - Cond1_appendNoMatch
* - Cond1_deleteNoMatch
* - Cond1_updateMatchWithoutChange
* - Cond1_updateMixMatchNoMatch
* - Cond1_appendMixMatchNoMatch
* - Cond1_deleteMixMatchNoMatch
* - Cond1_update2Matches1Notification
* - Cond1_append2Matches1Notification
* - Cond1_delete2Matches1Notification
* - CondN_updateMatch
* - CondN_appendMatch
* - CondN_deleteMatch
* - CondN_updateMatchDisjoint
* - CondN_appendMatchDisjoint
* - CondN_deleteMatchDisjoint
* - CondN_updateNoMatch
* - CondN_appendNoMatch
* - CondN_deleteNoMatch
* - CondN_updateMatchWithoutChange
* - CondN_updateMixMatchNoMatch
* - CondN_appendMixMatchNoMatch
* - CondN_deleteMixMatchNoMatch
* - CondN_update2Matches1Notification
* - CondN_append2Matches1Notification
* - CondN_delete2Matches1Notification
*
* Notes:
*
* - "Cond1" means that all the condValues are in the same notifyCondition element,
*   "CondN" means that they each condValue is in a separate notifyCondition. The
*   behaviour should be exactly the same.
* - "Disjoint" means cases in which the triggering attribute is not included in the
*   ones that are to be reported in the NotifyContextRequest
* - "MixMatchNoMatch" are test that mix an update/append/delete on an attribute that
*   matches the ONCHANGE condition and other that doesn't match it
* - "2Matches1Notification" are tests that matches twice, used to check that we are
*   sending just one notification (not two ones).
*
* General comment that applyes to tests above: our focus in the test module is
* ONCHANGE notifications. Thus, we won't pay attention to response and database
* status (both are being checked in another test module for updateContextSubscription,
* see mongoUpdateContext_test.cpp).
*
* - FIXME: tests with a matching subscription but on entities/attributes that doesn't
*   exists, so notification is not sent
*
* Simulating fails in MongoDB connection:
*
* - MongoDbQueryFail
* - MongoDbUpdateFail
* - MongoDbFindOneFail
*
*/

/* ****************************************************************************
*
* prepareDatabase -
*
* This function is called before every test, to populate some information in the
* entities and csbus collections.
*/
static void prepareDatabase(bool useSubCache = true)
{
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
   * - E2:
   *     A1:  X
   *     A2:  Z
   *     A3:  W
   *     A7:  R
   * - E1*:
   *     A1:  X
   *     A2:  Z
   *     A3:  W
   *     A7:  R
   * - E2*:
   *     A1:  X
   *     A2:  Z
   *     A3:  W
   *     A7:  R
   * - E1**:
   *     A1:  X
   *     A2:  Z
   *     A3:  W
   *     A7:  R
   *
   * (*) Means that entity/type is using same name but different type. This is included to check that type
   *     is taken into account.
   *
   * (**)same name but without type
   *
   * We create the following subscriptions:
   *
   * - Sub1:
   *     Entity: E1
   *     Attribute: A1, A3, A4
   *     NotifyCond: ONCHANGE on [A1, A2, A4]
   *
   * - Sub2:
   *     Entity: E2
   *     Attribute: A1, A3, A4
   *     NotifyCond: ONCHANGE on A1
   *                 ONCHANGE on A2
   *                 ONCHANGE on A4
   * - Sub3:
   *     Entity: E[1-2]*
   *     Attribute: A1, A3, A4
   *     NotifyCond: ONCHANGE on [A1, A2, A4]
   *
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrNames" << BSON_ARRAY("A1" << "A2" << "A3" << "A7") <<
                     "attrs" << BSON(
                        "A1" << BSON("type" << "TA1" << "value" << "X") <<
                        "A2" << BSON("type" << "TA2" << "value" << "Z") <<
                        "A3" << BSON("type" << "TA3" << "value" << "W") <<
                        "A7" << BSON("type" << "TA7" << "value" << "W")));

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T2") <<
                     "attrNames" << BSON_ARRAY("A1" << "A2" << "A3" << "A7") <<
                     "attrs" << BSON(
                        "A1" << BSON("type" << "TA1" << "value" << "X") <<
                        "A2" << BSON("type" << "TA2" << "value" << "Z") <<
                        "A3" << BSON("type" << "TA3" << "value" << "W") <<
                        "A7" << BSON("type" << "TA7" << "value" << "W")));

  BSONObj en3 = BSON("_id" << BSON("id" << "E1" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A1" << "A2" << "A3" << "A7") <<
                     "attrs" << BSON(
                        "A1" << BSON("type" << "TA1" << "value" << "X") <<
                        "A2" << BSON("type" << "TA2" << "value" << "Z") <<
                        "A3" << BSON("type" << "TA3" << "value" << "W") <<
                        "A7" << BSON("type" << "TA7" << "value" << "W")));

  BSONObj en4 = BSON("_id" << BSON("id" << "E2" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A1" << "A2" << "A3" << "A7") <<
                     "attrs" << BSON(
                        "A1" << BSON("type" << "TA1" << "value" << "X") <<
                        "A2" << BSON("type" << "TA2" << "value" << "Z") <<
                        "A3" << BSON("type" << "TA3" << "value" << "W") <<
                        "A7" << BSON("type" << "TA7" << "value" << "W")));

  BSONObj en5 = BSON("_id" << BSON("id" << "E1") <<
                     "attrNames" << BSON_ARRAY("A1" << "A2" << "A3" << "A7") <<
                     "attrs" << BSON(
                        "A1" << BSON("type" << "TA1" << "value" << "X") <<
                        "A2" << BSON("type" << "TA2" << "value" << "Z") <<
                        "A3" << BSON("type" << "TA3" << "value" << "W") <<
                        "A7" << BSON("type" << "TA7" << "value" << "W")));

  BSONObj sub1 = BSON("_id" << OID("51307b66f481db11bf860001") <<
                      "expiration" << 1500000000 <<
                      "lastNotification" << 20000000 <<
                      "reference" << "http://notify1.me" <<
                      "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false")) <<
                      "attrs" << BSON_ARRAY("A1" << "A3" << "A4") <<
                      "conditions" << BSON_ARRAY("A1" << "A2" << "A4" << "A5"));

  // After the changes to simplify "condition" field (issue #1851) sub2 has become equal to sub1 and sub3
  BSONObj sub2 = BSON("_id" << OID("51307b66f481db11bf860002") <<
                      "expiration" << 2000000000 <<
                      "lastNotification" << 30000000 <<
                      "reference" << "http://notify2.me" <<
                      "entities" << BSON_ARRAY(BSON("id" << "E2" << "type" << "T2" << "isPattern" << "false")) <<
                      "attrs" << BSON_ARRAY("A1" << "A3" << "A4") <<
                      "conditions" << BSON_ARRAY("A1" << "A2" << "A4" << "A5"));

  BSONObj sub3 = BSON("_id" << OID("51307b66f481db11bf860003") <<
                      "expiration" << 1500000000 <<
                      "lastNotification" << 20000000 <<
                      "reference" << "http://notify3.me" <<
                      "entities" << BSON_ARRAY(BSON("id" << "E[1-2]" << "type" << "T" << "isPattern" << "true")) <<
                      "attrs" << BSON_ARRAY("A1" << "A3" << "A4") <<
                      "conditions" << BSON_ARRAY("A1" << "A2" << "A4" << "A5"));

  LM_M(("Creating 5 entities"));
  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en3);
  connection->insert(ENTITIES_COLL, en4);
  connection->insert(ENTITIES_COLL, en5);

  LM_M(("Creating 5 subscriptions"));
  connection->insert(SUBSCRIBECONTEXT_COLL, sub1);
  connection->insert(SUBSCRIBECONTEXT_COLL, sub2);
  connection->insert(SUBSCRIBECONTEXT_COLL, sub3);

  /* Given that preparation including csubs, we have to init cache */
  if (useSubCache == true)
  {
    noCache = false;
    subCacheInit();
    subCacheRefresh();
  }
}



/* ****************************************************************************
*
* prepareDatabaseWithNoTypeSubscriptions -
*
* This function relies on prepareDatabaseSubscription, adding two additional subscriptions for
* no type cases
*
*/
static void prepareDatabaseWithNoTypeSubscriptions(void)
{
    prepareDatabase(false);

    DBClientBase* connection = getMongoConnection();

    /* We create the following entities:
     *
     * - E3:
     *     A1:  X
     *     A2:  Z
     *     A3:  W
     *     A7:  R
     *
     * We create the following subscriptions:
     *
     * - Sub4:
     *     Entity: E1**
     *     Attribute: A1, A3, A4
     *     NotifyCond: ONCHANGE on [A1, A2, A4]
     *
     * - Sub5:
     *     Entity: E[2-3]**
     *     Attribute: A1, A3, A4
     *     NotifyCond: ONCHANGE on [A1, A2, A4]*
     *
     */

    BSONObj en = BSON("_id" << BSON("id" << "E3" << "type" << "T3") <<
                      "attrNames" << BSON_ARRAY("A1" << "A2" << "A3" << "A7") <<
                       "attrs" << BSON(
                          "A1" << BSON("type" << "TA1" << "value" << "X") <<
                          "A2" << BSON("type" << "TA2" << "value" << "Z") <<
                          "A3" << BSON("type" << "TA3" << "value" << "W") <<
                          "A7" << BSON("type" << "TA7" << "value" << "W")));

    BSONObj sub4 = BSON("_id" << OID("51307b66f481db11bf860004") <<
                        "expiration" << 1500000000 <<
                        "lastNotification" << 20000000 <<
                        "reference" << "http://notify4.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "isPattern" << "false")) <<
                        "attrs" << BSON_ARRAY("A1" << "A3" << "A4") <<
                        "conditions" << BSON_ARRAY("A1" << "A2" << "A4" << "A5"));

    BSONObj sub5 = BSON("_id" << OID("51307b66f481db11bf860005") <<
                        "expiration" << 1500000000 <<
                        "lastNotification" << 20000000 <<
                        "reference" << "http://notify5.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E[2-3]" << "isPattern" << "true")) <<
                        "attrs" << BSON_ARRAY("A1" << "A3" << "A4") <<
                        "conditions" << BSON_ARRAY("A1" << "A2" << "A4" << "A5"));


    connection->insert(ENTITIES_COLL, en);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub4);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub5);

    /* Given that preparation including csubs, we have to initialize the subscription cache */
    subCacheInit();
    subCacheRefresh();
}

/* ****************************************************************************
*
* Cond1_updateMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_updateMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "new_val");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860001");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");
    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_appendMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_appendMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    ContextAttribute* ca4P = new ContextAttribute("A4", "TA4", "new_val");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    cerP->entity.attributeVector.push_back(ca4P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860001");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A4", "TA4", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeAppend;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");
    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_deleteMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_deleteMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca2P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca2P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860001");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeDelete;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");
    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_updateMatch_noType -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_updateMatch_noType)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr1, expectedNcr2;
    expectedNcr1.originator.set("localhost");
    expectedNcr2.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "new_val");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    expectedNcr1.contextElementResponseVector.push_back(cerP);
    expectedNcr2.contextElementResponseVector.push_back(cerP);
    expectedNcr1.subscriptionId.set("51307b66f481db11bf860001");
    expectedNcr2.subscriptionId.set("51307b66f481db11bf860004");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");
    ngsiv2::HttpInfo             httpInfo2("http://notify4.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr1),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr2),
                                                        MatchHttpInfo(&httpInfo2),
                                                        "",
                                                        "",
                                                        "",
                                                        2,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabaseWithNoTypeSubscriptions();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* sub1P = subCacheItemLookup("", "51307b66f481db11bf860001");
    CachedSubscription* sub2P = subCacheItemLookup("", "51307b66f481db11bf860004");

    ASSERT_TRUE(sub1P != NULL);
    ASSERT_TRUE(sub2P != NULL);

    EXPECT_EQ(1360232700, sub1P->lastNotificationTime);
    EXPECT_EQ(1360232700, sub2P->lastNotificationTime);


    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_appendMatch_noType -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_appendMatch_noType)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr1, expectedNcr2;
    expectedNcr1.originator.set("localhost");
    expectedNcr2.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    ContextAttribute* ca4P = new ContextAttribute("A4", "TA4", "new_val");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    cerP->entity.attributeVector.push_back(ca4P);
    expectedNcr1.contextElementResponseVector.push_back(cerP);
    expectedNcr2.contextElementResponseVector.push_back(cerP);
    expectedNcr1.subscriptionId.set("51307b66f481db11bf860001");
    expectedNcr2.subscriptionId.set("51307b66f481db11bf860004");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");
    ngsiv2::HttpInfo             httpInfo2("http://notify4.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr1),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr2),
                                                        MatchHttpInfo(&httpInfo2),
                                                        "",
                                                        "",
                                                        "",
                                                        2,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A4", "TA4", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeAppend;

    /* Prepare database */
    prepareDatabaseWithNoTypeSubscriptions();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* sub1P = subCacheItemLookup("", "51307b66f481db11bf860001");
    CachedSubscription* sub2P = subCacheItemLookup("", "51307b66f481db11bf860004");

    ASSERT_TRUE(sub1P != NULL);
    ASSERT_TRUE(sub2P != NULL);

    EXPECT_EQ(1360232700, sub1P->lastNotificationTime);
    EXPECT_EQ(1360232700, sub2P->lastNotificationTime);


    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_deleteMatch_noType -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_deleteMatch_noType)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr1, expectedNcr2;
    expectedNcr1.originator.set("localhost");
    expectedNcr2.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca2P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca2P);
    expectedNcr1.contextElementResponseVector.push_back(cerP);
    expectedNcr2.contextElementResponseVector.push_back(cerP);
    expectedNcr1.subscriptionId.set("51307b66f481db11bf860001");
    expectedNcr2.subscriptionId.set("51307b66f481db11bf860004");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");
    ngsiv2::HttpInfo             httpInfo2("http://notify4.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr1),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr2),
                                                        MatchHttpInfo(&httpInfo2),
                                                        "",
                                                        "",
                                                        "",
                                                        2,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);

    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeDelete;

    /* Prepare database */
    prepareDatabaseWithNoTypeSubscriptions();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* sub1P = subCacheItemLookup("", "51307b66f481db11bf860001");
    CachedSubscription* sub2P = subCacheItemLookup("", "51307b66f481db11bf860004");

    ASSERT_TRUE(sub1P != NULL);
    ASSERT_TRUE(sub2P != NULL);

    EXPECT_EQ(1360232700, sub1P->lastNotificationTime);
    EXPECT_EQ(1360232700, sub2P->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_updateMatch_pattern -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_updateMatch_pattern)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cer1P = new ContextElementResponse();
    cer1P->entity.fill("E1", "T", "false");
    ContextAttribute* ca11P = new ContextAttribute("A1", "TA1", "new_val");
    ContextAttribute* ca13P = new ContextAttribute("A3", "TA3", "W");
    cer1P->entity.attributeVector.push_back(ca11P);
    cer1P->entity.attributeVector.push_back(ca13P);
    expectedNcr.contextElementResponseVector.push_back(cer1P);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860003");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify3.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);

    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860003");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);


    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_appendMatch_pattern -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_appendMatch_pattern)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cer1P = new ContextElementResponse();
    cer1P->entity.fill("E1", "T", "false");
    ContextAttribute* ca11P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca13P = new ContextAttribute("A3", "TA3", "W");
    ContextAttribute* ca14P = new ContextAttribute("A4", "TA4", "new_val");
    cer1P->entity.attributeVector.push_back(ca11P);
    cer1P->entity.attributeVector.push_back(ca13P);
    cer1P->entity.attributeVector.push_back(ca14P);
    expectedNcr.contextElementResponseVector.push_back(cer1P);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860003");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify3.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T", "false");
    ContextAttribute* caP = new ContextAttribute("A4", "TA4", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeAppend;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860003");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_deleteMatch_pattern -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_deleteMatch_pattern)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cer1P = new ContextElementResponse();
    cer1P->entity.fill("E1", "T", "false");
    ContextAttribute* ca12P = new ContextAttribute("A3", "TA3", "W");
    cer1P->entity.attributeVector.push_back(ca12P);
    expectedNcr.contextElementResponseVector.push_back(cer1P);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860003");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify3.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeDelete;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860003");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_updateMatch_pattern_noType -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_updateMatch_pattern_noType)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cer1P = new ContextElementResponse();
    cer1P->entity.fill("E3", "T3", "false");
    ContextAttribute* ca11P = new ContextAttribute("A1", "TA1", "new_val");
    ContextAttribute* ca13P = new ContextAttribute("A3", "TA3", "W");
    cer1P->entity.attributeVector.push_back(ca11P);
    cer1P->entity.attributeVector.push_back(ca13P);
    expectedNcr.contextElementResponseVector.push_back(cer1P);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860005");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify5.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E3", "T3", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabaseWithNoTypeSubscriptions();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860005");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_appendMatch_pattern_noType -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_appendMatch_pattern_noType)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cer1P = new ContextElementResponse();
    cer1P->entity.fill("E3", "T3", "false");
    ContextAttribute* ca11P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca13P = new ContextAttribute("A3", "TA3", "W");
    ContextAttribute* ca14P = new ContextAttribute("A4", "TA4", "new_val");
    cer1P->entity.attributeVector.push_back(ca11P);
    cer1P->entity.attributeVector.push_back(ca13P);
    cer1P->entity.attributeVector.push_back(ca14P);
    expectedNcr.contextElementResponseVector.push_back(cer1P);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860005");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify5.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E3", "T3", "false");
    ContextAttribute* caP = new ContextAttribute("A4", "TA4", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeAppend;

    /* Prepare database */
    prepareDatabaseWithNoTypeSubscriptions();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860005");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_deleteMatch_pattern_noType -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_deleteMatch_pattern_noType)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cer1P = new ContextElementResponse();
    cer1P->entity.fill("E3", "T3", "false");
    ContextAttribute* ca12P = new ContextAttribute("A3", "TA3", "W");
    cer1P->entity.attributeVector.push_back(ca12P);
    expectedNcr.contextElementResponseVector.push_back(cer1P);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860005");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify5.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E3", "T3", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeDelete;

    /* Prepare database */
    prepareDatabaseWithNoTypeSubscriptions();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860005");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_updateMatchDisjoint -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_updateMatchDisjoint)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860001");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A2", "TA2", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_appendMatchDisjoint -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_appendMatchDisjoint)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860001");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A5", "TA5", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeAppend;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_deleteMatchDisjoint -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_deleteMatchDisjoint)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860001");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A2", "TA2", "");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeDelete;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_updateNoMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_updateNoMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        _,
                                                        _,
                                                        _)).Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A3", "TA3", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(20000000, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_appendNoMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_appendNoMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        _,
                                                        _,
                                                        _)).Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A6", "TA6", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeAppend;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    DBClientBase* connection = getMongoConnection();
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));
    EXPECT_EQ(20000000, sub.getIntField("lastNotification"));

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_deleteNoMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_deleteNoMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        _,
                                                        _,
                                                        _)).Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A3", "TA3", "");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeDelete;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    DBClientBase* connection = getMongoConnection();
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));
    EXPECT_EQ(20000000, sub.getIntField("lastNotification"));

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_updateMatchWithoutChange -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_updateMatchWithoutChange)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        emptyV,
                                                        false,
                                                        emptyV)).Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "X");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    DBClientBase* connection = getMongoConnection();
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));
    EXPECT_EQ(20000000, sub.getIntField("lastNotification"));

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
*  Cond1_updateMixMatchNoMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_updateMixMatchNoMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "new_val1");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860001");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caa1P = new ContextAttribute("A1", "TA1", "new_val1");   // match
    ContextAttribute* caa2P = new ContextAttribute("A7", "TA7", "new_val7");   // no match
    eP->attributeVector.push_back(caa1P);
    eP->attributeVector.push_back(caa2P);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_appendMixMatchNoMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_appendMixMatchNoMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    ContextAttribute* ca4P = new ContextAttribute("A4", "TA4", "new_val4");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    cerP->entity.attributeVector.push_back(ca4P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860001");

    NotifierMock* notifierMock = new NotifierMock();
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        _,
                                                        _,
                                                        _)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caa1P = new ContextAttribute("A4", "TA4", "new_val4");   // match
    ContextAttribute* caa2P = new ContextAttribute("A6", "TA6", "new_val6");   // no match
    eP->attributeVector.push_back(caa1P);
    eP->attributeVector.push_back(caa2P);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeAppend;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_deleteMixMatchNoMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_deleteMixMatchNoMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    cerP->entity.attributeVector.push_back(ca1P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860001");

    NotifierMock* notifierMock = new NotifierMock();
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        _,
                                                        _,
                                                        _)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caa1P = new ContextAttribute("A3", "TA3", "");    // no match
    ContextAttribute* caa2P = new ContextAttribute("A2", "TA2", "");    // match
    eP->attributeVector.push_back(caa1P);
    eP->attributeVector.push_back(caa2P);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeDelete;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);


    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_update2Matches1Notification -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_update2Matches1Notification)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "new_val1");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860001");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caa1P = new ContextAttribute("A1", "TA1", "new_val1");
    ContextAttribute* caa2P = new ContextAttribute("A2", "TA2", "new_val2");
    eP->attributeVector.push_back(caa1P);
    eP->attributeVector.push_back(caa2P);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);
    EXPECT_EQ(1, subP->count);


    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_append2Matches1Notification -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_append2Matches1Notification)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    ContextAttribute* ca4P = new ContextAttribute("A4", "TA4", "new_val4");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    cerP->entity.attributeVector.push_back(ca4P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860001");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caa1P = new ContextAttribute("A4", "TA4", "new_val4");
    ContextAttribute* caa2P = new ContextAttribute("A5", "TA5", "new_val5");
    eP->attributeVector.push_back(caa1P);
    eP->attributeVector.push_back(caa2P);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeAppend;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);
    EXPECT_EQ(1, subP->count);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* Cond1_delete2Matches1Notification -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, Cond1_delete2Matches1Notification)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(caP);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860001");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify1.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caa1P = new ContextAttribute("A1", "TA1", "");
    eP->attributeVector.push_back(caa1P);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeDelete;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_updateMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_updateMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E2", "T2", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "new_val");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860002");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");

    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860002");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_appendMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_appendMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E2", "T2", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    ContextAttribute* ca4P = new ContextAttribute("A4", "TA4", "new_val");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    cerP->entity.attributeVector.push_back(ca4P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860002");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A4", "TA4", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeAppend;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860002");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_deleteMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_deleteMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E2", "T2", "false");
    ContextAttribute* ca2P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca2P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860002");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeDelete;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860002");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_updateMatchDisjoint -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_updateMatchDisjoint)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E2", "T2", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860002");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A2", "TA2", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860002");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);


    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_appendMatchDisjoint -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_appendMatchDisjoint)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E2", "T2", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860002");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A5", "TA5", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeAppend;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860002");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_deleteMatchDisjoint -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_deleteMatchDisjoint)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E2", "T2", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860002");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A2", "TA2", "");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeDelete;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860002");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_updateNoMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_updateNoMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifierMock*                notifierMock = new NotifierMock();
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        _,
                                                        _,
                                                        _)).Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A3", "TA3", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    DBClientBase* connection = getMongoConnection();
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_EQ(30000000, sub.getIntField("lastNotification"));

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_appendNoMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_appendNoMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        _,
                                                        _,
                                                        _)).Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A6", "TA6", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeAppend;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    DBClientBase* connection = getMongoConnection();
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_EQ(30000000, sub.getIntField("lastNotification"));

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_deleteNoMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_deleteNoMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        _,
                                                        _,
                                                        _)).Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A3", "TA3", "");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeDelete;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    DBClientBase* connection = getMongoConnection();
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_EQ(30000000, sub.getIntField("lastNotification"));

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_updateMatchWithoutChange -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_updateMatchWithoutChange)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_,
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        emptyV,
                                                        false,
                                                        emptyV)).Times(0);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "X");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    DBClientBase* connection = getMongoConnection();
    BSONObj sub = connection->findOne(SUBSCRIBECONTEXT_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_EQ(30000000, sub.getIntField("lastNotification"));

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
*  CondN_updateMixMatchNoMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_updateMixMatchNoMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E2", "T2", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "new_val1");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860002");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caa1P = new ContextAttribute("A1", "TA1", "new_val1");   // match
    ContextAttribute* caa2P = new ContextAttribute("A7", "TA7", "new_val7");   // no match
    eP->attributeVector.push_back(caa1P);
    eP->attributeVector.push_back(caa2P);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860002");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_appendMixMatchNoMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_appendMixMatchNoMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E2", "T2", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    ContextAttribute* ca4P = new ContextAttribute("A4", "TA4", "new_val4");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    cerP->entity.attributeVector.push_back(ca4P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860002");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caa1P = new ContextAttribute("A4", "TA4", "new_val4");   // match
    ContextAttribute* caa2P = new ContextAttribute("A6", "TA6", "new_val6");    // no match
    eP->attributeVector.push_back(caa1P);
    eP->attributeVector.push_back(caa2P);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeAppend;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860002");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_deleteMixMatchNoMatch -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_deleteMixMatchNoMatch)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E2", "T2", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    cerP->entity.attributeVector.push_back(ca1P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860002");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caa1P = new ContextAttribute("A3", "TA3", "");   // no match
    ContextAttribute* caa2P = new ContextAttribute("A2", "TA2", "");   // match
    eP->attributeVector.push_back(caa1P);
    eP->attributeVector.push_back(caa2P);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeDelete;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860002");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_update2Matches1Notification -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_update2Matches1Notification)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E2", "T2", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "new_val1");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860002");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caa1P = new ContextAttribute("A1", "TA1", "new_val1");
    ContextAttribute* caa2P = new ContextAttribute("A2", "TA2", "new_val2");
    eP->attributeVector.push_back(caa1P);
    eP->attributeVector.push_back(caa2P);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860002");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);
    EXPECT_EQ(1, subP->count);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_append2Matches1Notification -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_append2Matches1Notification)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E2", "T2", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "X");
    ContextAttribute* ca3P = new ContextAttribute("A3", "TA3", "W");
    ContextAttribute* ca4P = new ContextAttribute("A4", "TA4", "new_val4");
    cerP->entity.attributeVector.push_back(ca1P);
    cerP->entity.attributeVector.push_back(ca3P);
    cerP->entity.attributeVector.push_back(ca4P);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860002");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caa1P = new ContextAttribute("A4", "TA4", "new_val4");
    ContextAttribute* caa2P = new ContextAttribute("A5", "TA5", "new_val5");
    eP->attributeVector.push_back(caa1P);
    eP->attributeVector.push_back(caa2P);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeAppend;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860002");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);
    EXPECT_EQ(1, subP->count);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* CondN_delete2Matches1Notification -
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, CondN_delete2Matches1Notification)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    NotifyContextRequest expectedNcr;
    expectedNcr.originator.set("localhost");
    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->entity.fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A3", "TA3", "W");
    cerP->entity.attributeVector.push_back(caP);
    expectedNcr.contextElementResponseVector.push_back(cerP);
    expectedNcr.subscriptionId.set("51307b66f481db11bf860002");

    NotifierMock* notifierMock = new NotifierMock();
    std::vector<std::string>     emptyV;
    std::vector<std::string>     attrsFilter;
    ngsiv2::HttpInfo             httpInfo("http://notify2.me");

    attrsFilter.push_back("A1");
    attrsFilter.push_back("A3");
    attrsFilter.push_back("A4");

    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(MatchNcr(&expectedNcr),
                                                        MatchHttpInfo(&httpInfo),
                                                        "",
                                                        "",
                                                        "",
                                                        1,
                                                        NGSI_V1_LEGACY,
                                                        attrsFilter,
                                                        false,
                                                        emptyV)).Times(1);
    setNotifier(notifierMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E2", "T2", "false");
    ContextAttribute* caa1P = new ContextAttribute("A1", "TA1", "");
    eP->attributeVector.push_back(caa1P);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeDelete;

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860002");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(1360232700, subP->lastNotificationTime);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* MongoDbQueryFail -
*
* FIXME: given that we can no "bypass" calls to the mocked function in which we
* are not interested given the problem described in the mock constructor above
* (in particular, I will need to bypass queries to "utest.entities), I will leave
* finalization of this tests (current version is not working) upon resolution of
* old issue #87
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, DISABLED_MongoDbQueryFail)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Prepare mocks */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, _query("utest.csubs", _, _, _, _, _, _))
            .WillByDefault(Throw(e));

    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextRequest(_, _, _, _, _, _, _, _, _, _))
            .Times(0);
    setNotifier(notifierMock);

    /* Set MongoDB connection */
    DBClientBase* connectionDb = getMongoConnection();
    setMongoConnectionForUnitTest(connectionMock);

    /* Forge the request (from "inside" to "outside") */
    Entity* eP = new Entity();
    eP->fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
    eP->attributeVector.push_back(caP);
    req.entityVector.push_back(eP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    EXPECT_EQ(0, RES_CER(0).attributeVector.size());
    EXPECT_EQ(SccReceiverInternalError, RES_CER_STATUS(0).code);
    EXPECT_EQ("Database Error", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check lastNotification */
    CachedSubscription* subP = subCacheItemLookup("", "51307b66f481db11bf860001");

    ASSERT_TRUE(subP != NULL);
    EXPECT_EQ(20000000, subP->lastNotificationTime);

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb);

    /* Release mock */
    delete notifierMock;

    utExit();
}

/* ****************************************************************************
*
* MongoDbUpdateFail -
*
* FIXME: waiting to solve old issues #87
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, DISABLED_MongoDbUpdateFail)
{
}

/* ****************************************************************************
*
* MongoDbFindOneFail -
*
* FIXME: The necessity of this test depends on issue #371.
*
* If we are able to solve the problem, it is unneded. However, if the conclusion of that issue
* is that we need the dobule-query on csubs, then we need this tests. By the moment, we leave the
* stub, marked with DISABLED.
*/
TEST(mongoUpdateContext_withOnchangeSubscriptions, DISABLED_MongoDbFindOneFail)
{
}
