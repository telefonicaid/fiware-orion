/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <utility>

#include "gtest/gtest.h"
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoConnectionPool.h"
#include "mongoBackend/mongoCreateSubscription.h"

#include "unittests/testInit.h"
#include "unittests/commonMocks.h"
#include "unittests/unittest.h"



/* ****************************************************************************
*
* USING
*/
using mongo::DBClientBase;
using mongo::BSONElement;
using mongo::BSONObj;
using mongo::BSONArray;
using mongo::OID;
using ngsiv2::Subscription;
using ngsiv2::EntID;



/* ****************************************************************************
*
* createSubscriptionNotCustomOK -
*/
TEST(mongoCreateSubscriptions, createSubscriptionNotCustomOK)
{
  OrionError  oe;

  utInit();

  /* Forge input subscription */
  Subscription sub;
  sub.description = "this is the sub";
  sub.expires     = 1360236300;
  sub.status      = "active";
  sub.throttling  = 5;
  sub.attrsFormat = NGSI_V2_NORMALIZED;

  EntID en1("E1", "", "T1", "");
  EntID en2("", "E.*", "T2", "");
  sub.subject.entities.push_back(en1);
  sub.subject.entities.push_back(en2);
  sub.subject.condition.attributes.push_back("A");
  sub.subject.condition.attributes.push_back("B");

  sub.subject.condition.expression.q        = "temperature<=20";
  sub.subject.condition.expression.coords   = "-40.4,-3.5;0,0";
  sub.subject.condition.expression.georel   = "coveredBy";
  sub.subject.condition.expression.geometry = "box";

  sub.notification.attributes.push_back("C");
  sub.notification.attributes.push_back("D");
  sub.notification.httpInfo.url      = "http://foo.bar";
  sub.notification.httpInfo.custom   = false;

  /* Invoke the function in mongoBackend library */
  std::string result = mongoCreateSubscription(sub, &oe, "", servicePathVector, "", "", false, V2);

  /* Check response is as expected */
  EXPECT_EQ(SccNone, oe.code);
  EXPECT_EQ("", oe.reasonPhrase);
  EXPECT_EQ("", oe.details);

  DBClientBase* connection = getMongoConnection();

  ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
  BSONObj doc = connection->findOne(SUBSCRIBECONTEXT_COLL, BSONObj());

  EXPECT_EQ(result, doc.getField("_id").OID().toString());
  EXPECT_EQ(1360236300, doc.getIntField("expiration"));
  EXPECT_FALSE(doc.hasField("lastNotification"));
  EXPECT_EQ(5, doc.getIntField("throttling"));
  EXPECT_STREQ("http://foo.bar", C_STR_FIELD(doc, "reference"));
  EXPECT_STREQ("normalized", C_STR_FIELD(doc, "format"));

  EXPECT_STREQ("this is the sub", C_STR_FIELD(doc, "description"));
  EXPECT_STREQ("active", C_STR_FIELD(doc, "status"));
  EXPECT_FALSE(doc.getBoolField("custom"));

  BSONObj expression = doc.getField("expression").embeddedObject();
  EXPECT_STREQ("temperature<=20", C_STR_FIELD(expression, "q"));
  EXPECT_STREQ("-40.4,-3.5;0,0", C_STR_FIELD(expression, "coords"));
  EXPECT_STREQ("coveredBy", C_STR_FIELD(expression, "georel"));
  EXPECT_STREQ("box", C_STR_FIELD(expression, "geometry"));

  std::vector<BSONElement> entities = doc.getField("entities").Array();
  ASSERT_EQ(2, entities.size());
  BSONObj ent0 = entities[0].embeddedObject();
  EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
  EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));
  BSONObj ent1 = entities[1].embeddedObject();
  EXPECT_STREQ("E.*", C_STR_FIELD(ent1, "id"));
  EXPECT_STREQ("T2", C_STR_FIELD(ent1, "type"));
  EXPECT_STREQ("true", C_STR_FIELD(ent1, "isPattern"));

  std::vector<BSONElement> attrs = doc.getField("attrs").Array();
  EXPECT_EQ(2, attrs.size());
  EXPECT_EQ("C", attrs[0].String());
  EXPECT_EQ("D", attrs[1].String());

  std::vector<BSONElement> conds = doc.getField("conditions").Array();
  ASSERT_EQ(2, conds.size());
  EXPECT_EQ("A", conds[0].String());
  EXPECT_EQ("B", conds[1].String());

  utExit();
}



/* ****************************************************************************
*
* createSubscriptionCustomOK -
*/
TEST(mongoCreateSubscriptions, createSubscriptionCustomOK)
{
  OrionError  oe;

  utInit();

  /* Forge input subscription */
  Subscription sub;
  sub.description = "this is the sub";
  sub.expires     = 1360236300;
  sub.status      = "active";
  sub.throttling  = 5;
  sub.attrsFormat = NGSI_V2_NORMALIZED;

  EntID en1("E1", "", "T1", "");
  EntID en2("", "E.*", "T2", "");
  sub.subject.entities.push_back(en1);
  sub.subject.entities.push_back(en2);
  sub.subject.condition.attributes.push_back("A");
  sub.subject.condition.attributes.push_back("B");

  sub.subject.condition.expression.q        = "temperature<=20";
  sub.subject.condition.expression.coords   = "-40.4,-3.5;0,0";
  sub.subject.condition.expression.georel   = "coveredBy";
  sub.subject.condition.expression.geometry = "box";

  sub.notification.attributes.push_back("C");
  sub.notification.attributes.push_back("D");
  sub.notification.httpInfo.url      = "http://foo.bar";
  sub.notification.httpInfo.custom   = true;
  sub.notification.httpInfo.verb     = PUT;
  sub.notification.httpInfo.headers.insert(std::pair<std::string, std::string>("X-My-Header", "foo"));
  sub.notification.httpInfo.headers.insert(std::pair<std::string, std::string>("Content-Type", "text/plain"));
  sub.notification.httpInfo.qs.insert(std::pair<std::string, std::string>("p1", "param1"));
  sub.notification.httpInfo.qs.insert(std::pair<std::string, std::string>("p2", "param2"));
  sub.notification.httpInfo.payload = "Hey!";

  /* Invoke the function in mongoBackend library */
  std::string result = mongoCreateSubscription(sub, &oe, "", servicePathVector, "", "", false, V2);

  /* Check response is as expected */
  EXPECT_EQ(SccNone, oe.code);
  EXPECT_EQ("", oe.reasonPhrase);
  EXPECT_EQ("", oe.details);

  DBClientBase* connection = getMongoConnection();

  ASSERT_EQ(1, connection->count(SUBSCRIBECONTEXT_COLL, BSONObj()));
  BSONObj doc = connection->findOne(SUBSCRIBECONTEXT_COLL, BSONObj());

  EXPECT_EQ(result, doc.getField("_id").OID().toString());
  EXPECT_EQ(1360236300, doc.getIntField("expiration"));
  EXPECT_FALSE(doc.hasField("lastNotification"));
  EXPECT_EQ(5, doc.getIntField("throttling"));
  EXPECT_STREQ("http://foo.bar", C_STR_FIELD(doc, "reference"));
  EXPECT_STREQ("normalized", C_STR_FIELD(doc, "format"));

  EXPECT_STREQ("this is the sub", C_STR_FIELD(doc, "description"));
  EXPECT_STREQ("active", C_STR_FIELD(doc, "status"));
  EXPECT_TRUE(doc.getBoolField("custom"));

  EXPECT_STREQ("PUT", C_STR_FIELD(doc, "method"));
  EXPECT_STREQ("Hey!", C_STR_FIELD(doc, "payload"));

  BSONObj headers = doc.getField("headers").embeddedObject();
  ASSERT_TRUE(headers.hasField("X-My-Header"));
  EXPECT_STREQ("foo", C_STR_FIELD(headers, "X-My-Header"));
  ASSERT_TRUE(headers.hasField("Content-Type"));
  EXPECT_STREQ("text/plain", C_STR_FIELD(headers, "Content-Type"));

  BSONObj qs = doc.getField("qs").embeddedObject();
  ASSERT_TRUE(qs.hasField("p1"));
  EXPECT_STREQ("param1", C_STR_FIELD(qs, "p1"));
  ASSERT_TRUE(qs.hasField("p2"));
  EXPECT_STREQ("param2", C_STR_FIELD(qs, "p2"));

  BSONObj expression = doc.getField("expression").embeddedObject();
  EXPECT_STREQ("temperature<=20", C_STR_FIELD(expression, "q"));
  EXPECT_STREQ("-40.4,-3.5;0,0", C_STR_FIELD(expression, "coords"));
  EXPECT_STREQ("coveredBy", C_STR_FIELD(expression, "georel"));
  EXPECT_STREQ("box", C_STR_FIELD(expression, "geometry"));

  std::vector<BSONElement> entities = doc.getField("entities").Array();
  ASSERT_EQ(2, entities.size());
  BSONObj ent0 = entities[0].embeddedObject();
  EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
  EXPECT_STREQ("false", C_STR_FIELD(ent0, "isPattern"));
  BSONObj ent1 = entities[1].embeddedObject();
  EXPECT_STREQ("E.*", C_STR_FIELD(ent1, "id"));
  EXPECT_STREQ("T2", C_STR_FIELD(ent1, "type"));
  EXPECT_STREQ("true", C_STR_FIELD(ent1, "isPattern"));

  std::vector<BSONElement> attrs = doc.getField("attrs").Array();
  EXPECT_EQ(2, attrs.size());
  EXPECT_EQ("C", attrs[0].String());
  EXPECT_EQ("D", attrs[1].String());

  std::vector<BSONElement> conds = doc.getField("conditions").Array();
  ASSERT_EQ(2, conds.size());
  EXPECT_EQ("A", conds[0].String());
  EXPECT_EQ("B", conds[1].String());

  utExit();
}
