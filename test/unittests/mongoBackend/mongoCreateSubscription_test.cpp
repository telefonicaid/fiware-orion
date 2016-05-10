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
#include "gtest/gtest.h"
#include "testInit.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoCreateSubscription.h"

#include "mongo/client/dbclient.h"

#include "commonMocks.h"
#include "unittest.h"

using namespace ngsiv2;

/* ****************************************************************************
 *  Subscrptions IDs
*/
static const std::string SUB_OID1 = "51307b66f481db11bf860001";


/* ****************************************************************************
*
* x -
*/
TEST(mongoCreateSubscriptions, x)
{
  OrionError  oe;

  /* Forge input subscription */
  Subscription sub;
  sub.id = SUB_OID1;

  /* Invoke the function in mongoBackend library */
  bool result = mongoCreateSubscription(sub, &oe, uriParams, "");

  /* Check response is as expected */
  EXPECT_TRUE(result);

#if 0
  EXPECT_EQ(SccOk, oe.code);
  EXPECT_EQ("OK", oe.reasonPhrase);
  EXPECT_EQ("", oe.details);

  // I keep this by the moment, as a reminder on how to check DB

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
  EXPECT_EQ(24, s.notification.timesSent);;
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
  EXPECT_EQ(-1, s.notification.timesSent);;
  EXPECT_EQ(25000000, s.notification.lastNotification);
  EXPECT_EQ(-1, s.throttling);
  EXPECT_EQ(20000000, s.expires);
#endif
}
