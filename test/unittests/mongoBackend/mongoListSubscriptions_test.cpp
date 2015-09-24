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
#include "gtest/gtest.h"
#include "testInit.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoListSubscriptions.h"

#include "mongo/client/dbclient.h"

#include "commonMocks.h"
#include "unittest.h"

using ::testing::_;
using ::testing::Throw;
using ::testing::Return;

using namespace ngsiv2;

extern void setMongoConnectionForUnitTest(DBClientBase*);

/* ****************************************************************************
*
* - getAllSubscriptionsV1Info
*
*/

/* ****************************************************************************
*
* prepareDatabaseV1Subs -
*/
static void prepareDatabaseV1Subs(void) {

    /* Set database */
    setupDatabase();

    DBClientBase* connection = getMongoConnection();

    BSONObj sub1 = BSON("_id" << OID("51307b66f481db11bf860001") <<
                        "expiration" << 10000000 <<
                        "lastNotification" << 15000000 <<
                        "reference" << "http://notify1.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1" << "isPattern" << "false")) <<
                        "attrs" << BSONArray() <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX1" << "AY1")
                                                       ))
                        );

    BSONObj sub2 = BSON("_id" << OID("51307b66f481db11bf860002") <<
                        "expiration" << 20000000 <<
                        "lastNotification" << 25000000 <<
                        "reference" << "http://notify2.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E.*" << "type" << "T2" << "isPattern" << "true")) <<
                        "attrs" << BSON_ARRAY("A1" << "A2") <<
                        "conditions" << BSON_ARRAY(BSON(
                                                       "type" << "ONCHANGE" <<
                                                       "value" << BSON_ARRAY("AX2" << "AY2")
                                                       )) <<
                        "throttling" << 5.0
                        );

    BSONObj sub3 = BSON("_id" << OID("51307b66f481db11bf860003") <<
                        "expiration" << 20000000 <<
                        "lastNotification" << 25000000 <<
                        "reference" << "http://notify2.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E.*" << "type" << "T2" << "isPattern" << "true")) <<
                        "attrs" << BSON_ARRAY("A1" << "A2") <<
                        "conditions" << BSON_ARRAY(BSON(
                                                     "type" << "ONTIMEINTERVAL" <<
                                                     "value" << 100
                                                     ))
                        );

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
  OrionError oe;

  /* Prepare database */
  prepareDatabaseV1Subs();

  /* Invoke the function in mongoBackend library */
  std::vector<Subscription> subs;
  oe = mongoListSubscriptions(subs, uriParams, "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, oe.code);
  EXPECT_EQ("OK", oe.reasonPhrase);
  EXPECT_EQ("", oe.details);

  ASSERT_EQ(2, subs.size());
  Subscription             s;
  std::vector<EntID>       ents;
  std::vector<std::string> attrs;

  /* Subscription #1 */
  s = subs[0];
  EXPECT_EQ("51307b66f481db11bf860001", s.id);
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
  EXPECT_EQ("http://notify1.me", s.notification.callback);
  EXPECT_TRUE(s.notification.throttling.isEmpty());
  EXPECT_EQ("FIXME", s.duration.get());

  /* Subscription #2 */
  s = subs[1];
  EXPECT_EQ("51307b66f481db11bf860002", s.id);
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
  EXPECT_EQ("http://notify2.me", s.notification.callback);
  EXPECT_EQ("FIXME", s.notification.throttling.get());
  EXPECT_EQ("FIXME", s.duration.get());

}
