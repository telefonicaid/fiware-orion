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
* Author: Fermín Galán
*/
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "mongo/client/dbclient.h"


#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoNotifyContextAvailability.h"

#include "unittests/commonMocks.h"
#include "unittests/testInit.h"
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
using ::testing::_;
using ::testing::Throw;
using ::testing::Return;



extern void setMongoConnectionForUnitTest(DBClientBase* _connection);



/* ****************************************************************************
*
* Tests
*
* For the ok test, we will use tests that combine the following possibilities of request,
* using the following naming: ce<X>_En<Y>[nt]_At[nt]<Z>_Ok
*
* - ContextRegistraitonElements: 1 or N
* - Entities: 1 or N, the "nt" sufix mean that the entity/entities has/have no type
* - Attributes: 0 or N, the "nt" sufix mean that the attribute/attributess has/have no type
* - ContextRegistrationAttributes: 0 or N
*
* (Without lost of generality in our tests N=2)
*
*- FIXME P6: we can not provide a complete set of unit test right now, due to the rush
*  for Campus Party. This fixme mesage is a mark to get these tests completed in the
*  future. Look to registerContext test to get ideas of what is missing here
*
*/

/* ****************************************************************************
*
* ce1_En1_At0_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ce1_En1_At0_Ok)
{
  HttpStatusCode           ms;
  NotifyContextAvailabilityRequest   req;
  NotifyContextAvailabilityResponse  res;

  utInit();

  /* Forge the request */
  req.subscriptionId.set("51307b66f481db11bf860001");
  EntityId en("E1", "T1");
  ContextRegistrationResponse crr;
  crr.contextRegistration.entityIdVector.push_back(&en);
  crr.contextRegistration.providingApplication.set("http://dummy.com");
  crr.errorCode.fill(SccOk);
  req.contextRegistrationResponseVector.push_back(&crr);

  /* Invoke the function in mongoBackend library */
  ms = mongoNotifyContextAvailability(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360319100, reg.getIntField("expiration"));

  std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
  ASSERT_EQ(1, contextRegistrationV.size());
  BSONObj contextRegistration = contextRegistrationV[0].embeddedObject();

  EXPECT_STREQ("http://dummy.com", C_STR_FIELD(contextRegistration, "providingApplication"));
  std::vector<BSONElement> entities = contextRegistration.getField("entities").Array();
  ASSERT_EQ(1, entities.size());
  BSONObj ent0 = entities[0].embeddedObject();
  EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

  std::vector<BSONElement> attrs = contextRegistration.getField("attrs").Array();
  EXPECT_EQ(0, attrs.size());

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ(SccOk, res.responseCode.code);
  EXPECT_EQ("OK", res.responseCode.reasonPhrase);
  EXPECT_EQ(0, res.responseCode.details.size());

  utExit();
}



/* ****************************************************************************
*
* ce1_En1_At0_Ok_JSON -
*/
TEST(mongoNotifyContextAvailabilityRequest, ce1_En1_At0_Ok_JSON)
{
  HttpStatusCode           ms;
  NotifyContextAvailabilityRequest   req;
  NotifyContextAvailabilityResponse  res;

  utInit();

  /* Forge the request */
  req.subscriptionId.set("51307b66f481db11bf860001");
  EntityId en("E1", "T1");
  ContextRegistrationResponse crr;
  crr.contextRegistration.entityIdVector.push_back(&en);
  crr.contextRegistration.providingApplication.set("http://dummy.com");
  crr.errorCode.fill(SccOk);
  req.contextRegistrationResponseVector.push_back(&crr);

  /* Invoke the function in mongoBackend library */
  ms = mongoNotifyContextAvailability(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360319100, reg.getIntField("expiration"));
  EXPECT_STREQ("JSON", reg.getStringField("format"));

  std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
  ASSERT_EQ(1, contextRegistrationV.size());
  BSONObj contextRegistration = contextRegistrationV[0].embeddedObject();

  EXPECT_STREQ("http://dummy.com", C_STR_FIELD(contextRegistration, "providingApplication"));
  std::vector<BSONElement> entities = contextRegistration.getField("entities").Array();
  ASSERT_EQ(1, entities.size());
  BSONObj ent0 = entities[0].embeddedObject();
  EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

  std::vector<BSONElement> attrs = contextRegistration.getField("attrs").Array();
  EXPECT_EQ(0, attrs.size());

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ(SccOk, res.responseCode.code);
  EXPECT_EQ("OK", res.responseCode.reasonPhrase);
  EXPECT_EQ(0, res.responseCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* ce1_En1nt_At0_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ce1_En1nt_At0_Ok)
{
  HttpStatusCode           ms;
  NotifyContextAvailabilityRequest   req;
  NotifyContextAvailabilityResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "");
  ContextRegistrationResponse crr;
  crr.contextRegistration.entityIdVector.push_back(&en);
  crr.contextRegistration.providingApplication.set("http://dummy.com");
  crr.errorCode.fill(SccOk);
  req.contextRegistrationResponseVector.push_back(&crr);

  /* Invoke the function in mongoBackend library */
  ms = mongoNotifyContextAvailability(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360319100, reg.getIntField("expiration"));

  std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
  ASSERT_EQ(1, contextRegistrationV.size());
  BSONObj contextRegistration = contextRegistrationV[0].embeddedObject();

  EXPECT_STREQ("http://dummy.com", C_STR_FIELD(contextRegistration, "providingApplication"));
  std::vector<BSONElement> entities = contextRegistration.getField("entities").Array();
  ASSERT_EQ(1, entities.size());
  BSONObj ent0 = entities[0].embeddedObject();
  EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
  EXPECT_FALSE(ent0.hasField("type"));

  std::vector<BSONElement> attrs = contextRegistration.getField("attrs").Array();
  EXPECT_EQ(0, attrs.size());

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ(SccOk, res.responseCode.code);
  EXPECT_EQ("OK", res.responseCode.reasonPhrase);
  EXPECT_EQ(0, res.responseCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* ce1_En1_AtN_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ce1_En1_AtN_Ok)
{
  HttpStatusCode           ms;
  NotifyContextAvailabilityRequest   req;
  NotifyContextAvailabilityResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistrationAttribute cra1("A1", "TA1");
  ContextRegistrationAttribute cra2("A2", "TA2");
  ContextRegistrationResponse crr;
  crr.contextRegistration.entityIdVector.push_back(&en);
  crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
  crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
  crr.contextRegistration.providingApplication.set("http://dummy.com");
  crr.errorCode.fill(SccOk);
  req.contextRegistrationResponseVector.push_back(&crr);

  /* Invoke the function in mongoBackend library */
  ms = mongoNotifyContextAvailability(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360319100, reg.getIntField("expiration"));

  std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
  ASSERT_EQ(1, contextRegistrationV.size());
  BSONObj contextRegistration = contextRegistrationV[0].embeddedObject();

  EXPECT_STREQ("http://dummy.com", C_STR_FIELD(contextRegistration, "providingApplication"));
  std::vector<BSONElement> entities = contextRegistration.getField("entities").Array();
  ASSERT_EQ(1, entities.size());
  BSONObj ent0 = entities[0].embeddedObject();
  EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

  std::vector<BSONElement> regAttrs = contextRegistration.getField("attrs").Array();
  ASSERT_EQ(2, regAttrs.size());
  BSONObj rattr0 = regAttrs[0].embeddedObject();
  BSONObj rattr1 = regAttrs[1].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
  EXPECT_STREQ("TA1", C_STR_FIELD(rattr0, "type"));
  EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
  EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ(SccOk, res.responseCode.code);
  EXPECT_EQ("OK", res.responseCode.reasonPhrase);
  EXPECT_EQ(0, res.responseCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* ce1_En1_AtNnt_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ce1_En1_AtNnt_Ok)
{
  HttpStatusCode           ms;
  NotifyContextAvailabilityRequest   req;
  NotifyContextAvailabilityResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistrationAttribute cra1("A1", "");
  ContextRegistrationAttribute cra2("A2", "");
  ContextRegistrationResponse crr;
  crr.contextRegistration.entityIdVector.push_back(&en);
  crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
  crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
  crr.contextRegistration.providingApplication.set("http://dummy.com");
  crr.errorCode.fill(SccOk);
  req.contextRegistrationResponseVector.push_back(&crr);

  /* Invoke the function in mongoBackend library */
  ms = mongoNotifyContextAvailability(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360319100, reg.getIntField("expiration"));

  std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
  ASSERT_EQ(1, contextRegistrationV.size());
  BSONObj contextRegistration = contextRegistrationV[0].embeddedObject();

  EXPECT_STREQ("http://dummy.com", C_STR_FIELD(contextRegistration, "providingApplication"));
  std::vector<BSONElement> entities = contextRegistration.getField("entities").Array();
  ASSERT_EQ(1, entities.size());
  BSONObj ent0 = entities[0].embeddedObject();
  EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

  std::vector<BSONElement> regAttrs = contextRegistration.getField("attrs").Array();
  ASSERT_EQ(2, regAttrs.size());
  BSONObj rattr0 = regAttrs[0].embeddedObject();
  BSONObj rattr1 = regAttrs[1].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
  EXPECT_STREQ("", C_STR_FIELD(rattr0, "type"));
  EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
  EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ(SccOk, res.responseCode.code);
  EXPECT_EQ("OK", res.responseCode.reasonPhrase);
  EXPECT_EQ(0, res.responseCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* ce1_En1nt_AtN_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ce1_En1nt_AtN_Ok)
{
  HttpStatusCode           ms;
  NotifyContextAvailabilityRequest   req;
  NotifyContextAvailabilityResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "");
  ContextRegistrationAttribute cra1("A1", "TA1");
  ContextRegistrationAttribute cra2("A2", "TA2");
  ContextRegistrationResponse crr;
  crr.contextRegistration.entityIdVector.push_back(&en);
  crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
  crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
  crr.contextRegistration.providingApplication.set("http://dummy.com");
  crr.errorCode.fill(SccOk);
  req.contextRegistrationResponseVector.push_back(&crr);

  /* Invoke the function in mongoBackend library */
  ms = mongoNotifyContextAvailability(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360319100, reg.getIntField("expiration"));

  std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
  ASSERT_EQ(1, contextRegistrationV.size());
  BSONObj contextRegistration = contextRegistrationV[0].embeddedObject();

  EXPECT_STREQ("http://dummy.com", C_STR_FIELD(contextRegistration, "providingApplication"));
  std::vector<BSONElement> entities = contextRegistration.getField("entities").Array();
  ASSERT_EQ(1, entities.size());
  BSONObj ent0 = entities[0].embeddedObject();
  EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
  EXPECT_FALSE(ent0.hasField("type"));

  std::vector<BSONElement> regAttrs = contextRegistration.getField("attrs").Array();
  ASSERT_EQ(2, regAttrs.size());
  BSONObj rattr0 = regAttrs[0].embeddedObject();
  BSONObj rattr1 = regAttrs[1].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
  EXPECT_STREQ("TA1", C_STR_FIELD(rattr0, "type"));
  EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
  EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ(SccOk, res.responseCode.code);
  EXPECT_EQ("OK", res.responseCode.reasonPhrase);
  EXPECT_EQ(0, res.responseCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* ce1_En1nt_AtNnt_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ce1_En1nt_AtNnt_Ok)
{
  HttpStatusCode           ms;
  NotifyContextAvailabilityRequest   req;
  NotifyContextAvailabilityResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "");
  ContextRegistrationAttribute cra1("A1", "");
  ContextRegistrationAttribute cra2("A2", "");
  ContextRegistrationResponse crr;
  crr.contextRegistration.entityIdVector.push_back(&en);
  crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
  crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
  crr.contextRegistration.providingApplication.set("http://dummy.com");
  crr.errorCode.fill(SccOk);
  req.contextRegistrationResponseVector.push_back(&crr);

  /* Invoke the function in mongoBackend library */
  ms = mongoNotifyContextAvailability(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360319100, reg.getIntField("expiration"));

  std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
  ASSERT_EQ(1, contextRegistrationV.size());
  BSONObj contextRegistration = contextRegistrationV[0].embeddedObject();

  EXPECT_STREQ("http://dummy.com", C_STR_FIELD(contextRegistration, "providingApplication"));
  std::vector<BSONElement> entities = contextRegistration.getField("entities").Array();
  ASSERT_EQ(1, entities.size());
  BSONObj ent0 = entities[0].embeddedObject();
  EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
  EXPECT_FALSE(ent0.hasField("type"));

  std::vector<BSONElement> regAttrs = contextRegistration.getField("attrs").Array();
  ASSERT_EQ(2, regAttrs.size());
  BSONObj rattr0 = regAttrs[0].embeddedObject();
  BSONObj rattr1 = regAttrs[1].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
  EXPECT_STREQ("", C_STR_FIELD(rattr0, "type"));
  EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
  EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ(SccOk, res.responseCode.code);
  EXPECT_EQ("OK", res.responseCode.reasonPhrase);
  EXPECT_EQ(0, res.responseCode.details.size());

  utExit();
}



/* ****************************************************************************
*
* ce1_EnN_At0_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ce1_EnN_At0_Ok)
{
    HttpStatusCode                     ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationResponse crr;
    crr.contextRegistration.entityIdVector.push_back(&en1);
    crr.contextRegistration.entityIdVector.push_back(&en2);
    crr.contextRegistration.providingApplication.set("http://dummy.com");
    crr.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(1, contextRegistrationV.size());
    BSONObj contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://dummy.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    std::vector<BSONElement> entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    BSONObj ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent1, "type"));

    std::vector<BSONElement> regAttrs = contextRegistration.getField("attrs").Array();
    EXPECT_EQ(0, regAttrs.size());

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ce1_EnNnt_At0_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ce1_EnNnt_At0_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationResponse crr;
    crr.contextRegistration.entityIdVector.push_back(&en1);
    crr.contextRegistration.entityIdVector.push_back(&en2);
    crr.contextRegistration.providingApplication.set("http://dummy.com");
    crr.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(1, contextRegistrationV.size());
    BSONObj contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://dummy.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    std::vector<BSONElement> entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    BSONObj ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_FALSE(ent1.hasField("type"));

    std::vector<BSONElement> regAttrs = contextRegistration.getField("attrs").Array();
    EXPECT_EQ(0, regAttrs.size());

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ce1_EnN_AtN_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ce1_EnN_AtN_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationResponse crr;
    crr.contextRegistration.entityIdVector.push_back(&en1);
    crr.contextRegistration.entityIdVector.push_back(&en2);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr.contextRegistration.providingApplication.set("http://dummy.com");
    crr.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(1, contextRegistrationV.size());
    BSONObj contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://dummy.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    std::vector<BSONElement> entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    BSONObj ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent1, "type"));

    std::vector<BSONElement> regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    BSONObj rattr0 = regAttrs[0].embeddedObject();
    BSONObj rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ce1_EnN_AtNnt_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ce1_EnN_AtNnt_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationAttribute cra1("A1", "");
    ContextRegistrationAttribute cra2("A2", "");
    ContextRegistrationResponse crr;
    crr.contextRegistration.entityIdVector.push_back(&en1);
    crr.contextRegistration.entityIdVector.push_back(&en2);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr.contextRegistration.providingApplication.set("http://dummy.com");
    crr.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(1, contextRegistrationV.size());
    BSONObj contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://dummy.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    std::vector<BSONElement> entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    BSONObj ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent1, "type"));

    std::vector<BSONElement> regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    BSONObj rattr0 = regAttrs[0].embeddedObject();
    BSONObj rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ce1_EnNnt_AtN_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ce1_EnNnt_AtN_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationResponse crr;
    crr.contextRegistration.entityIdVector.push_back(&en1);
    crr.contextRegistration.entityIdVector.push_back(&en2);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr.contextRegistration.providingApplication.set("http://dummy.com");
    crr.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(1, contextRegistrationV.size());
    BSONObj contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://dummy.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    std::vector<BSONElement> entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    BSONObj ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_FALSE(ent1.hasField("type"));

    std::vector<BSONElement> regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    BSONObj rattr0 = regAttrs[0].embeddedObject();
    BSONObj rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ce1_EnNnt_AtNnt_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ce1_EnNnt_AtNnt_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationAttribute cra1("A1", "");
    ContextRegistrationAttribute cra2("A2", "");
    ContextRegistrationResponse crr;
    crr.contextRegistration.entityIdVector.push_back(&en1);
    crr.contextRegistration.entityIdVector.push_back(&en2);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr.contextRegistration.providingApplication.set("http://dummy.com");
    crr.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(1, contextRegistrationV.size());
    BSONObj contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://dummy.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    std::vector<BSONElement> entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    BSONObj ent0 = entities[0].embeddedObject();
    BSONObj ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_FALSE(ent1.hasField("type"));

    std::vector<BSONElement> regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    BSONObj rattr0 = regAttrs[0].embeddedObject();
    BSONObj rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_En1_At0_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ceN_En1_At0_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&en1);
    crr1.contextRegistration.providingApplication.set("http://dummy1.com");
    crr2.contextRegistration.entityIdVector.push_back(&en2);
    crr2.contextRegistration.providingApplication.set("http://dummy2.com");
    crr1.errorCode.fill(SccOk);
    crr2.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr1);
    req.contextRegistrationResponseVector.push_back(&crr2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());

    BSONObj contextRegistration, ent0;
    std::vector<BSONElement> entities, attrs;
    contextRegistration = contextRegistrationV[0].embeddedObject();
    EXPECT_STREQ("http://dummy1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    attrs = contextRegistration.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());
    contextRegistration = contextRegistrationV[1].embeddedObject();
    EXPECT_STREQ("http://dummy2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E2", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent0, "type"));
    attrs = contextRegistration.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_En1nt_At0_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ceN_En1nt_At0_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&en1);
    crr1.contextRegistration.providingApplication.set("http://dummy1.com");
    crr2.contextRegistration.entityIdVector.push_back(&en2);
    crr2.contextRegistration.providingApplication.set("http://dummy2.com");
    crr1.errorCode.fill(SccOk);
    crr2.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr1);
    req.contextRegistrationResponseVector.push_back(&crr2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());

    BSONObj contextRegistration, ent0;
    std::vector<BSONElement> entities, attrs;
    contextRegistration = contextRegistrationV[0].embeddedObject();
    EXPECT_STREQ("http://dummy1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    attrs = contextRegistration.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());
    contextRegistration = contextRegistrationV[1].embeddedObject();
    EXPECT_STREQ("http://dummy2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E2", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    attrs = contextRegistration.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_En1_AtN_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ceN_En1_AtN_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationAttribute cra3("A3", "TA3");
    ContextRegistrationAttribute cra4("A4", "TA4");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&en1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.providingApplication.set("http://dummy1.com");
    crr2.contextRegistration.entityIdVector.push_back(&en2);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra4);
    crr2.contextRegistration.providingApplication.set("http://dummy2.com");
    crr1.errorCode.fill(SccOk);
    crr2.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr1);
    req.contextRegistrationResponseVector.push_back(&crr2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());

    BSONObj contextRegistration, ent0, rattr0, rattr1;
    std::vector<BSONElement> entities, regAttrs;
    contextRegistration = contextRegistrationV[0].embeddedObject();
    EXPECT_STREQ("http://dummy1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));

    contextRegistration = contextRegistrationV[1].embeddedObject();
    EXPECT_STREQ("http://dummy2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E2", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent0, "type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(rattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_En1_AtNnt_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ceN_En1_AtNnt_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationAttribute cra1("A1", "");
    ContextRegistrationAttribute cra2("A2", "");
    ContextRegistrationAttribute cra3("A3", "");
    ContextRegistrationAttribute cra4("A4", "");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&en1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.providingApplication.set("http://dummy1.com");
    crr2.contextRegistration.entityIdVector.push_back(&en2);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra4);
    crr2.contextRegistration.providingApplication.set("http://dummy2.com");
    crr1.errorCode.fill(SccOk);
    crr2.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr1);
    req.contextRegistrationResponseVector.push_back(&crr2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());

    BSONObj contextRegistration, ent0, rattr0, rattr1;
    std::vector<BSONElement> entities, regAttrs;
    contextRegistration = contextRegistrationV[0].embeddedObject();
    EXPECT_STREQ("http://dummy1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));

    contextRegistration = contextRegistrationV[1].embeddedObject();
    EXPECT_STREQ("http://dummy2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E2", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent0, "type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_En1nt_AtN_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ceN_En1nt_AtN_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationAttribute cra3("A3", "TA3");
    ContextRegistrationAttribute cra4("A4", "TA4");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&en1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.providingApplication.set("http://dummy1.com");
    crr2.contextRegistration.entityIdVector.push_back(&en2);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra4);
    crr2.contextRegistration.providingApplication.set("http://dummy2.com");
    crr1.errorCode.fill(SccOk);
    crr2.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr1);
    req.contextRegistrationResponseVector.push_back(&crr2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());

    BSONObj contextRegistration, ent0, rattr0, rattr1;
    std::vector<BSONElement> entities, regAttrs;
    contextRegistration = contextRegistrationV[0].embeddedObject();
    EXPECT_STREQ("http://dummy1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));

    contextRegistration = contextRegistrationV[1].embeddedObject();
    EXPECT_STREQ("http://dummy2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E2", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(rattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_En1nt_AtNnt_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ceN_En1nt_AtNnt_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationAttribute cra1("A1", "");
    ContextRegistrationAttribute cra2("A2", "");
    ContextRegistrationAttribute cra3("A3", "");
    ContextRegistrationAttribute cra4("A4", "");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&en1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.providingApplication.set("http://dummy1.com");
    crr2.contextRegistration.entityIdVector.push_back(&en2);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra4);
    crr2.contextRegistration.providingApplication.set("http://dummy2.com");
    crr1.errorCode.fill(SccOk);
    crr2.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr1);
    req.contextRegistrationResponseVector.push_back(&crr2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());

    BSONObj contextRegistration, ent0, rattr0, rattr1;
    std::vector<BSONElement> entities, regAttrs;
    contextRegistration = contextRegistrationV[0].embeddedObject();
    EXPECT_STREQ("http://dummy1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));

    contextRegistration = contextRegistrationV[1].embeddedObject();
    EXPECT_STREQ("http://dummy2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E2", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_EnN_At0_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ceN_EnN_At0_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    EntityId en3("E3", "T3");
    EntityId en4("E4", "T4");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&en1);
    crr1.contextRegistration.entityIdVector.push_back(&en2);
    crr1.contextRegistration.providingApplication.set("http://dummy1.com");
    crr2.contextRegistration.entityIdVector.push_back(&en3);
    crr2.contextRegistration.entityIdVector.push_back(&en4);
    crr2.contextRegistration.providingApplication.set("http://dummy2.com");
    crr1.errorCode.fill(SccOk);
    crr2.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr1);
    req.contextRegistrationResponseVector.push_back(&crr2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());

    BSONObj contextRegistration, ent0, ent1;
    std::vector<BSONElement> entities, regAttrs;
    contextRegistration = contextRegistrationV[0].embeddedObject();
    EXPECT_STREQ("http://dummy1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    ent0 = entities[0].embeddedObject();
    ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent1, "type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    EXPECT_EQ(0, regAttrs.size());

    contextRegistration = contextRegistrationV[1].embeddedObject();
    EXPECT_STREQ("http://dummy2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    ent0 = entities[0].embeddedObject();
    ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E3", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T4", C_STR_FIELD(ent1, "type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    EXPECT_EQ(0, regAttrs.size());

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_EnNnt_At0_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ceN_EnNnt_At0_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    EntityId en3("E3", "");
    EntityId en4("E4", "");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&en1);
    crr1.contextRegistration.entityIdVector.push_back(&en2);
    crr1.contextRegistration.providingApplication.set("http://dummy1.com");
    crr2.contextRegistration.entityIdVector.push_back(&en3);
    crr2.contextRegistration.entityIdVector.push_back(&en4);
    crr2.contextRegistration.providingApplication.set("http://dummy2.com");
    crr1.errorCode.fill(SccOk);
    crr2.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr1);
    req.contextRegistrationResponseVector.push_back(&crr2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());

    BSONObj contextRegistration, ent0, ent1;
    std::vector<BSONElement> entities, regAttrs;
    contextRegistration = contextRegistrationV[0].embeddedObject();
    EXPECT_STREQ("http://dummy1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    ent0 = entities[0].embeddedObject();
    ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_FALSE(ent1.hasField("type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    EXPECT_EQ(0, regAttrs.size());

    contextRegistration = contextRegistrationV[1].embeddedObject();
    EXPECT_STREQ("http://dummy2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    ent0 = entities[0].embeddedObject();
    ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E3", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent1, "id"));
    EXPECT_FALSE(ent1.hasField("type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    EXPECT_EQ(0, regAttrs.size());

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_EnN_AtN_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ceN_EnN_AtN_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    EntityId en3("E3", "T3");
    EntityId en4("E4", "T4");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationAttribute cra3("A3", "TA3");
    ContextRegistrationAttribute cra4("A4", "TA4");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&en1);
    crr1.contextRegistration.entityIdVector.push_back(&en2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.providingApplication.set("http://dummy1.com");
    crr2.contextRegistration.entityIdVector.push_back(&en3);
    crr2.contextRegistration.entityIdVector.push_back(&en4);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra4);
    crr2.contextRegistration.providingApplication.set("http://dummy2.com");
    crr1.errorCode.fill(SccOk);
    crr2.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr1);
    req.contextRegistrationResponseVector.push_back(&crr2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());

    BSONObj contextRegistration, ent0, ent1, rattr0, rattr1;
    std::vector<BSONElement> entities, regAttrs;
    contextRegistration = contextRegistrationV[0].embeddedObject();
    EXPECT_STREQ("http://dummy1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    ent0 = entities[0].embeddedObject();
    ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent1, "type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));

    contextRegistration = contextRegistrationV[1].embeddedObject();
    EXPECT_STREQ("http://dummy2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    ent0 = entities[0].embeddedObject();
    ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E3", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T4", C_STR_FIELD(ent1, "type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(rattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_EnN_AtNnt_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ceN_EnN_AtNnt_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    EntityId en3("E3", "T3");
    EntityId en4("E4", "T4");
    ContextRegistrationAttribute cra1("A1", "");
    ContextRegistrationAttribute cra2("A2", "");
    ContextRegistrationAttribute cra3("A3", "");
    ContextRegistrationAttribute cra4("A4", "");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&en1);
    crr1.contextRegistration.entityIdVector.push_back(&en2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.providingApplication.set("http://dummy1.com");
    crr2.contextRegistration.entityIdVector.push_back(&en3);
    crr2.contextRegistration.entityIdVector.push_back(&en4);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra4);
    crr2.contextRegistration.providingApplication.set("http://dummy2.com");
    crr1.errorCode.fill(SccOk);
    crr2.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr1);
    req.contextRegistrationResponseVector.push_back(&crr2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());

    BSONObj contextRegistration, ent0, ent1, rattr0, rattr1;
    std::vector<BSONElement> entities, regAttrs;
    contextRegistration = contextRegistrationV[0].embeddedObject();
    EXPECT_STREQ("http://dummy1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    ent0 = entities[0].embeddedObject();
    ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent1, "type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));

    contextRegistration = contextRegistrationV[1].embeddedObject();
    EXPECT_STREQ("http://dummy2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    ent0 = entities[0].embeddedObject();
    ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E3", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent0, "type"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent1, "id"));
    EXPECT_STREQ("T4", C_STR_FIELD(ent1, "type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_EnNnt_AtN_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ceN_EnNnt_AtN_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    EntityId en3("E3", "");
    EntityId en4("E4", "");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationAttribute cra3("A3", "TA3");
    ContextRegistrationAttribute cra4("A4", "TA4");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&en1);
    crr1.contextRegistration.entityIdVector.push_back(&en2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.providingApplication.set("http://dummy1.com");
    crr2.contextRegistration.entityIdVector.push_back(&en3);
    crr2.contextRegistration.entityIdVector.push_back(&en4);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra4);
    crr2.contextRegistration.providingApplication.set("http://dummy2.com");
    crr1.errorCode.fill(SccOk);
    crr2.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr1);
    req.contextRegistrationResponseVector.push_back(&crr2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());

    BSONObj contextRegistration, ent0, ent1, rattr0, rattr1;
    std::vector<BSONElement> entities, regAttrs;
    contextRegistration = contextRegistrationV[0].embeddedObject();
    EXPECT_STREQ("http://dummy1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    ent0 = entities[0].embeddedObject();
    ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_FALSE(ent1.hasField("type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));

    contextRegistration = contextRegistrationV[1].embeddedObject();
    EXPECT_STREQ("http://dummy2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    ent0 = entities[0].embeddedObject();
    ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E3", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent1, "id"));
    EXPECT_FALSE(ent1.hasField("type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(rattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_EnNnt_AtNnt_Ok -
*/
TEST(mongoNotifyContextAvailabilityRequest, ceN_EnNnt_AtNnt_Ok)
{
    HttpStatusCode           ms;
    NotifyContextAvailabilityRequest   req;
    NotifyContextAvailabilityResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    EntityId en3("E3", "");
    EntityId en4("E4", "");
    ContextRegistrationAttribute cra1("A1", "");
    ContextRegistrationAttribute cra2("A2", "");
    ContextRegistrationAttribute cra3("A3", "");
    ContextRegistrationAttribute cra4("A4", "");
    ContextRegistrationResponse crr1, crr2;
    crr1.contextRegistration.entityIdVector.push_back(&en1);
    crr1.contextRegistration.entityIdVector.push_back(&en2);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra1);
    crr1.contextRegistration.contextRegistrationAttributeVector.push_back(&cra2);
    crr1.contextRegistration.providingApplication.set("http://dummy1.com");
    crr2.contextRegistration.entityIdVector.push_back(&en3);
    crr2.contextRegistration.entityIdVector.push_back(&en4);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra3);
    crr2.contextRegistration.contextRegistrationAttributeVector.push_back(&cra4);
    crr2.contextRegistration.providingApplication.set("http://dummy2.com");
    crr1.errorCode.fill(SccOk);
    crr2.errorCode.fill(SccOk);
    req.contextRegistrationResponseVector.push_back(&crr1);
    req.contextRegistrationResponseVector.push_back(&crr2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContextAvailability(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360319100, reg.getIntField("expiration"));

    std::vector<BSONElement> contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());

    BSONObj contextRegistration, ent0, ent1, rattr0, rattr1;
    std::vector<BSONElement> entities, regAttrs;
    contextRegistration = contextRegistrationV[0].embeddedObject();
    EXPECT_STREQ("http://dummy1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    ent0 = entities[0].embeddedObject();
    ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent1, "id"));
    EXPECT_FALSE(ent1.hasField("type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));

    contextRegistration = contextRegistrationV[1].embeddedObject();
    EXPECT_STREQ("http://dummy2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(2, entities.size());
    ent0 = entities[0].embeddedObject();
    ent1 = entities[1].embeddedObject();
    EXPECT_STREQ("E3", C_STR_FIELD(ent0, "id"));
    EXPECT_FALSE(ent0.hasField("type"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent1, "id"));
    EXPECT_FALSE(ent1.hasField("type"));
    regAttrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(2, regAttrs.size());
    rattr0 = regAttrs[0].embeddedObject();
    rattr1 = regAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(rattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

    utExit();
}
