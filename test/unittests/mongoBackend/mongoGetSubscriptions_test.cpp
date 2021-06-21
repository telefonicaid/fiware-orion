/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "mongoBackend/mongoConnectionPool.h"
#include "mongoBackend/mongoGetSubscriptions.h"

#include "unittests/testInit.h"
#include "unittests/commonMocks.h"
#include "unittests/unittest.h"



/* ****************************************************************************
*
* USING
*/
using mongo::DBClientBase;
using mongo::BSONObj;
using mongo::BSONArray;
using mongo::OID;
using ::testing::_;
using ::testing::Throw;
using ::testing::Return;
using ngsiv2::Subscription;
using ngsiv2::EntID;



extern void setMongoConnectionForUnitTest(orion::DBClientBase _connection);



/* ****************************************************************************
*
* - getAllSubscriptionsV1Info
*
*/


/* ****************************************************************************
 *  Subscrptions IDs
*/
static const std::string SUB_OID1 = "51307b66f481db11bf860001";
static const std::string SUB_OID2 = "51307b66f481db11bf860002";
static const std::string SUB_OID3 = "51307b66f481db11bf860003";


/* ****************************************************************************
*
* prepareDatabaseV1Subs -
*/
static void prepareDatabaseV1Subs(void)
{
    /* Set database */
    setupDatabase();

    // 10000000 -> Sun, 26 Apr 1970 17:46:40 GMT -> "1970-04-26T17:46:40Z"
    // 20000000 -> Thu, 20 Aug 1970 11:33:20 GMT -> "1970-08-20T11:33:20Z"
    // 25000000 -> Sat, 17 Oct 1970 08:26:40 GMT -> "1970-19-17T08:26:40Z"
    DBClientBase* connection = getMongoConnection();

    BSONObj sub1 = BSON("_id" << OID(SUB_OID1) <<
                        "expiration" << 10000000 <<
                        "reference" << "http://notify1.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false")) <<
                        "attrs" << BSONArray() <<
                        "conditions" << BSON_ARRAY("AX1" << "AY1"));

    BSONObj sub2 = BSON("_id" << OID(SUB_OID2) <<
                        "expiration" << 25000000 <<
                        "lastNotification" << 20000000 <<
                        "count" << 24 <<
                        "reference" << "http://notify2.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E.*" << "type" << "T2" << "isPattern" << "true")) <<
                        "attrs" << BSON_ARRAY("A1" << "A2") <<
                        "conditions" << BSON_ARRAY("AX2" << "AY2") <<
                        "throttling" << 5);

    BSONObj sub3 = BSON("_id" << OID(SUB_OID3) <<
                        "expiration" << 20000000 <<
                        "lastNotification" << 25000000 <<
                        "reference" << "http://notify2.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E.*" << "type" << "T2" << "isPattern" << "true")) <<
                        "attrs" << BSON_ARRAY("A1" << "A2") <<
                        "conditions" << BSON_ARRAY("ZZ2" << "WW2"));

    connection->insert(SUBSCRIBECONTEXT_COLL, sub1);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub2);
    connection->insert(SUBSCRIBECONTEXT_COLL, sub3);
}

/* ****************************************************************************
*
* getAllSubscriptionsV1Info -
*/
TEST(mongoListSubscriptions, getAllSubscriptionsV1Info)
{
  utInit();

  OrionError  oe;
  long long   count;

  /* Prepare database */
  prepareDatabaseV1Subs();

  /* Invoke the function in mongoBackend library */
  std::vector<Subscription> subs;
  mongoListSubscriptions(&subs, &oe, uriParams, "", "/#", 20, 0, &count);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, oe.code);
  EXPECT_EQ("OK", oe.reasonPhrase);
  EXPECT_EQ("", oe.details);

  ASSERT_EQ(3, subs.size());
  Subscription             s;
  std::vector<EntID>       ents;
  std::vector<std::string> attrs;

  /* Subscription #1 */
  s = subs[0];
  EXPECT_EQ(SUB_OID1, s.id);
  ents = s.subject.entities;
  ASSERT_EQ(1, ents.size());
  EXPECT_EQ("E1", ents[0].id);
  EXPECT_EQ("T1", ents[0].type);
  EXPECT_EQ("", ents[0].idPattern);
  attrs = s.subject.condition.attributes;
  ASSERT_EQ(2, attrs.size());
  EXPECT_EQ("AX1", attrs[0]);
  EXPECT_EQ("AY1", attrs[1]);
  EXPECT_EQ("", s.subject.condition.expression.q);
  EXPECT_EQ("", s.subject.condition.expression.geometry);
  EXPECT_EQ("", s.subject.condition.expression.coords);
  attrs = s.notification.attributes;
  ASSERT_EQ(0, attrs.size());
  EXPECT_EQ("http://notify1.me", s.notification.httpInfo.url);
  EXPECT_EQ(-1, s.notification.timesSent);
  EXPECT_EQ(-1, s.notification.lastNotification);
  EXPECT_EQ(-1, s.throttling);
  EXPECT_EQ(10000000, s.expires);

  /* Subscription #2 */
  s = subs[1];
  EXPECT_EQ(SUB_OID2, s.id);
  ents = s.subject.entities;
  ASSERT_EQ(1, ents.size());
  EXPECT_EQ("", ents[0].id);
  EXPECT_EQ("T2", ents[0].type);
  EXPECT_EQ("E.*", ents[0].idPattern);
  attrs = s.subject.condition.attributes;
  ASSERT_EQ(2, attrs.size());
  EXPECT_EQ("AX2", attrs[0]);
  EXPECT_EQ("AY2", attrs[1]);
  EXPECT_EQ("", s.subject.condition.expression.q);
  EXPECT_EQ("", s.subject.condition.expression.geometry);
  EXPECT_EQ("", s.subject.condition.expression.coords);
  attrs = s.notification.attributes;
  ASSERT_EQ(2, attrs.size());
  EXPECT_EQ("A1", attrs[0]);
  EXPECT_EQ("A2", attrs[1]);
  EXPECT_EQ("http://notify2.me", s.notification.httpInfo.url);
  EXPECT_EQ(24, s.notification.timesSent);
  EXPECT_EQ(20000000, s.notification.lastNotification);
  EXPECT_EQ(5, s.throttling);
  EXPECT_EQ(25000000, s.expires);

  /* Subscription #3 */
  s = subs[2];
  EXPECT_EQ(SUB_OID3, s.id);
  ents = s.subject.entities;
  ASSERT_EQ(1, ents.size());
  EXPECT_EQ("", ents[0].id);
  EXPECT_EQ("T2", ents[0].type);
  EXPECT_EQ("E.*", ents[0].idPattern);
  attrs = s.subject.condition.attributes;
  ASSERT_EQ(2, attrs.size());
  EXPECT_EQ("ZZ2", attrs[0]);
  EXPECT_EQ("WW2", attrs[1]);
  EXPECT_EQ("", s.subject.condition.expression.q);
  EXPECT_EQ("", s.subject.condition.expression.geometry);
  EXPECT_EQ("", s.subject.condition.expression.coords);
  attrs = s.notification.attributes;
  ASSERT_EQ(2, attrs.size());
  EXPECT_EQ("A1", attrs[0]);
  EXPECT_EQ("A2", attrs[1]);
  EXPECT_EQ("http://notify2.me", s.notification.httpInfo.url);
  EXPECT_EQ(-1, s.notification.timesSent);
  EXPECT_EQ(25000000, s.notification.lastNotification);
  EXPECT_EQ(-1, s.throttling);
  EXPECT_EQ(20000000, s.expires);

  utExit();
}

/* ****************************************************************************
*
* getSubscriptionsV1Info -
*/
TEST(mongoGetSubscription, getSubscription)
{
  utInit();

  OrionError oe;

  /* Prepare database */
  prepareDatabaseV1Subs();

  /* Invoke the function in mongoBackend library */
  Subscription empty, s;

  mongoGetSubscription(&s, &oe, SUB_OID1, uriParams, "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, oe.code);
  EXPECT_EQ("OK", oe.reasonPhrase);
  EXPECT_EQ("", oe.details);

  std::vector<EntID>       ents;
  std::vector<std::string> attrs;

  /* Subscription #1 */

  EXPECT_EQ(SUB_OID1, s.id);
  ents = s.subject.entities;
  ASSERT_EQ(1, ents.size());
  EXPECT_EQ("E1", ents[0].id);
  EXPECT_EQ("T1", ents[0].type);
  EXPECT_EQ("", ents[0].idPattern);
  attrs = s.subject.condition.attributes;
  ASSERT_EQ(2, attrs.size());
  EXPECT_EQ("AX1", attrs[0]);
  EXPECT_EQ("AY1", attrs[1]);
  EXPECT_EQ("", s.subject.condition.expression.q);
  EXPECT_EQ("", s.subject.condition.expression.geometry);
  EXPECT_EQ("", s.subject.condition.expression.coords);
  attrs = s.notification.attributes;
  ASSERT_EQ(0, attrs.size());
  EXPECT_EQ("http://notify1.me", s.notification.httpInfo.url);
  EXPECT_EQ(-1, s.notification.timesSent);
  EXPECT_EQ(-1, s.notification.lastNotification);
  EXPECT_EQ(-1, s.throttling);
  EXPECT_EQ(10000000, s.expires);

  /* Subscription #2 */

  // clear subscription
  s = empty;

  mongoGetSubscription(&s, &oe, SUB_OID2, uriParams, "");

  EXPECT_EQ(SUB_OID2, s.id);
  ents = s.subject.entities;
  ASSERT_EQ(1, ents.size());
  EXPECT_EQ("", ents[0].id);
  EXPECT_EQ("T2", ents[0].type);
  EXPECT_EQ("E.*", ents[0].idPattern);
  attrs = s.subject.condition.attributes;
  ASSERT_EQ(2, attrs.size());
  EXPECT_EQ("AX2", attrs[0]);
  EXPECT_EQ("AY2", attrs[1]);
  EXPECT_EQ("", s.subject.condition.expression.q);
  EXPECT_EQ("", s.subject.condition.expression.geometry);
  EXPECT_EQ("", s.subject.condition.expression.coords);
  attrs = s.notification.attributes;
  ASSERT_EQ(2, attrs.size());
  EXPECT_EQ("A1", attrs[0]);
  EXPECT_EQ("A2", attrs[1]);
  EXPECT_EQ("http://notify2.me", s.notification.httpInfo.url);
  EXPECT_EQ(24, s.notification.timesSent);
  EXPECT_EQ(20000000, s.notification.lastNotification);
  EXPECT_EQ(5, s.throttling);
  EXPECT_EQ(25000000, s.expires);

  utExit();
}
