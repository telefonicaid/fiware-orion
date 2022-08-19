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
* Author: Ken Zangelin
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
#include "mongoBackend/mongoRegisterContext.h"
#include "ngsi/StatusCode.h"
#include "ngsi/ContextRegistration.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextRegistrationAttribute.h"
#include "ngsi/Metadata.h"
#include "ngsi9/RegisterContextRequest.h"
#include "ngsi9/RegisterContextResponse.h"

#include "unittests/testInit.h"
#include "unittests/unittest.h"
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
using ::testing::_;
using ::testing::Throw;
using ::testing::Return;



extern void setMongoConnectionForUnitTest(orion::DBClientBase _connection);



/* ****************************************************************************
*
* Tests
*
* For the ok test, we will use tests that combine the following possibilities of request,
* using the following naming: ce<X>_En<Y>[nt]_At[nt]<Z>_Ok
*
* - ContextRegistrationElements: 1 or N
* - Entities: 1 or N, the "nt" sufix mean that the entity/entities has/have no type
* - Attributes: 0 or N, the "nt" sufix mean that the attribute/attributes has/have no type
* - ContextRegistrationAttributes: 0 or N
*
* (Without lost of generality in our tests N=2)
*
* In addition, we include some tests to check of notify context availability triggering upon
* creation of context registration:
*
* In addition, we include a test to check default duration
*
* - defaultDuration
*
* In addition, we test some case in which we simulate fails in MongoDB connection:
*
* - MongoDbCountFail
* - MongoDbFindOneFail
* - MongoDbUpdateEntityFail
* - MongoDbInsertEntityFail
* - MongoDbUpsertRegistrationFail
* - MongoDbEntitiesInconsistency
*
* Note these tests are not "canonical" unit tests. Canon says that in this case we should have
* mocked MongoDB. Actually, we think is very much powerful to check that everything is ok at
* MongoDB layer.
*
*/



/* ****************************************************************************
*
* prepareDatabase -
*/
static void prepareDatabase(void)
{
    /* Clean database */
    setupDatabase();
}



/* ****************************************************************************
*
* ce1_En1_At0_Ok -
*/
TEST(mongoRegisterContextRequest, ce1_En1_At0_Ok)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  utExit();
}



/* ****************************************************************************
*
* ce1_En1_At0_Ok_JSON -
*/
TEST(mongoRegisterContextRequest, ce1_En1_At0_Ok_JSON)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360232760, reg.getIntField("expiration"));
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
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* ce1_En1nt_At0_Ok -
*/
TEST(mongoRegisterContextRequest, ce1_En1nt_At0_Ok)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* ce1_En1_AtN_Ok -
*/
TEST(mongoRegisterContextRequest, ce1_En1_AtN_Ok)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistrationAttribute cra1("A1", "TA1");
  ContextRegistrationAttribute cra2("A2", "TA2");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.contextRegistrationAttributeVector.push_back(&cra1);
  cr.contextRegistrationAttributeVector.push_back(&cra2);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* ce1_En1_AtNnt_Ok -
*/
TEST(mongoRegisterContextRequest, ce1_En1_AtNnt_Ok)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistrationAttribute cra1("A1", "");
  ContextRegistrationAttribute cra2("A2", "");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.contextRegistrationAttributeVector.push_back(&cra1);
  cr.contextRegistrationAttributeVector.push_back(&cra2);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  utExit();
}



/* ****************************************************************************
*
* ce1_En1nt_AtN_Ok -
*/
TEST(mongoRegisterContextRequest, ce1_En1nt_AtN_Ok)
{
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "");
  ContextRegistrationAttribute cra1("A1", "TA1");
  ContextRegistrationAttribute cra2("A2", "TA2");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.contextRegistrationAttributeVector.push_back(&cra1);
  cr.contextRegistrationAttributeVector.push_back(&cra2);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  mongoRegisterContext(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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

  utExit();
}

/* ****************************************************************************
*
* ce1_En1nt_AtNnt_Ok -
*/
TEST(mongoRegisterContextRequest, ce1_En1nt_AtNnt_Ok)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "");
  ContextRegistrationAttribute cra1("A1", "");
  ContextRegistrationAttribute cra2("A2", "");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.contextRegistrationAttributeVector.push_back(&cra1);
  cr.contextRegistrationAttributeVector.push_back(&cra2);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res, uriParams);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().toString();
  EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* ce1_EnN_At0_Ok -
*/
TEST(mongoRegisterContextRequest, ce1_EnN_At0_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en1);
    cr.entityIdVector.push_back(&en2);
    cr.providingApplication.set("http://dummy.com");
    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ce1_EnNnt_At0_Ok -
*/
TEST(mongoRegisterContextRequest, ce1_EnNnt_At0_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en1);
    cr.entityIdVector.push_back(&en2);
    cr.providingApplication.set("http://dummy.com");
    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ce1_EnN_AtN_Ok -
*/
TEST(mongoRegisterContextRequest, ce1_EnN_AtN_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en1);
    cr.entityIdVector.push_back(&en2);
    cr.contextRegistrationAttributeVector.push_back(&cra1);
    cr.contextRegistrationAttributeVector.push_back(&cra2);
    cr.providingApplication.set("http://dummy.com");

    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ce1_EnN_AtNnt_Ok -
*/
TEST(mongoRegisterContextRequest, ce1_EnN_AtNnt_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationAttribute cra1("A1", "");
    ContextRegistrationAttribute cra2("A2", "");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en1);
    cr.entityIdVector.push_back(&en2);
    cr.contextRegistrationAttributeVector.push_back(&cra1);
    cr.contextRegistrationAttributeVector.push_back(&cra2);
    cr.providingApplication.set("http://dummy.com");

    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ce1_EnNnt_AtN_Ok -
*/
TEST(mongoRegisterContextRequest, ce1_EnNnt_AtN_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en1);
    cr.entityIdVector.push_back(&en2);
    cr.contextRegistrationAttributeVector.push_back(&cra1);
    cr.contextRegistrationAttributeVector.push_back(&cra2);
    cr.providingApplication.set("http://dummy.com");

    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ce1_EnNnt_AtNnt_Ok -
*/
TEST(mongoRegisterContextRequest, ce1_EnNnt_AtNnt_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationAttribute cra1("A1", "");
    ContextRegistrationAttribute cra2("A2", "");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en1);
    cr.entityIdVector.push_back(&en2);
    cr.contextRegistrationAttributeVector.push_back(&cra1);
    cr.contextRegistrationAttributeVector.push_back(&cra2);
    cr.providingApplication.set("http://dummy.com");

    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_En1_At0_Ok -
*/
TEST(mongoRegisterContextRequest, ceN_En1_At0_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistration cr1, cr2;
    cr1.entityIdVector.push_back(&en1);
    cr1.providingApplication.set("http://dummy1.com");
    cr2.entityIdVector.push_back(&en2);
    cr2.providingApplication.set("http://dummy2.com");
    req.contextRegistrationVector.push_back(&cr1);
    req.contextRegistrationVector.push_back(&cr2);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_En1nt_At0_Ok -
*/
TEST(mongoRegisterContextRequest, ceN_En1nt_At0_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistration cr1, cr2;
    cr1.entityIdVector.push_back(&en1);
    cr1.providingApplication.set("http://dummy1.com");
    cr2.entityIdVector.push_back(&en2);
    cr2.providingApplication.set("http://dummy2.com");
    req.contextRegistrationVector.push_back(&cr1);
    req.contextRegistrationVector.push_back(&cr2);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_En1_AtN_Ok -
*/
TEST(mongoRegisterContextRequest, ceN_En1_AtN_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationAttribute cra3("A3", "TA3");
    ContextRegistrationAttribute cra4("A4", "TA4");
    ContextRegistration cr1, cr2;
    cr1.entityIdVector.push_back(&en1);
    cr1.contextRegistrationAttributeVector.push_back(&cra1);
    cr1.contextRegistrationAttributeVector.push_back(&cra2);
    cr1.providingApplication.set("http://dummy1.com");
    cr2.entityIdVector.push_back(&en2);
    cr2.contextRegistrationAttributeVector.push_back(&cra3);
    cr2.contextRegistrationAttributeVector.push_back(&cra4);
    cr2.providingApplication.set("http://dummy2.com");
    req.contextRegistrationVector.push_back(&cr1);
    req.contextRegistrationVector.push_back(&cr2);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_En1_AtNnt_Ok -
*/
TEST(mongoRegisterContextRequest, ceN_En1_AtNnt_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationAttribute cra1("A1", "");
    ContextRegistrationAttribute cra2("A2", "");
    ContextRegistrationAttribute cra3("A3", "");
    ContextRegistrationAttribute cra4("A4", "");
    ContextRegistration cr1, cr2;
    cr1.entityIdVector.push_back(&en1);
    cr1.contextRegistrationAttributeVector.push_back(&cra1);
    cr1.contextRegistrationAttributeVector.push_back(&cra2);
    cr1.providingApplication.set("http://dummy1.com");
    cr2.entityIdVector.push_back(&en2);
    cr2.contextRegistrationAttributeVector.push_back(&cra3);
    cr2.contextRegistrationAttributeVector.push_back(&cra4);
    cr2.providingApplication.set("http://dummy2.com");
    req.contextRegistrationVector.push_back(&cr1);
    req.contextRegistrationVector.push_back(&cr2);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_En1nt_AtN_Ok -
*/
TEST(mongoRegisterContextRequest, ceN_En1nt_AtN_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationAttribute cra1("A1", "TA1");
    ContextRegistrationAttribute cra2("A2", "TA2");
    ContextRegistrationAttribute cra3("A3", "TA3");
    ContextRegistrationAttribute cra4("A4", "TA4");
    ContextRegistration cr1, cr2;
    cr1.entityIdVector.push_back(&en1);
    cr1.contextRegistrationAttributeVector.push_back(&cra1);
    cr1.contextRegistrationAttributeVector.push_back(&cra2);
    cr1.providingApplication.set("http://dummy1.com");
    cr2.entityIdVector.push_back(&en2);
    cr2.contextRegistrationAttributeVector.push_back(&cra3);
    cr2.contextRegistrationAttributeVector.push_back(&cra4);
    cr2.providingApplication.set("http://dummy2.com");
    req.contextRegistrationVector.push_back(&cr1);
    req.contextRegistrationVector.push_back(&cr2);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_En1nt_AtNnt_Ok -
*/
TEST(mongoRegisterContextRequest, ceN_En1nt_AtNnt_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationAttribute cra1("A1", "");
    ContextRegistrationAttribute cra2("A2", "");
    ContextRegistrationAttribute cra3("A3", "");
    ContextRegistrationAttribute cra4("A4", "");
    ContextRegistration cr1, cr2;
    cr1.entityIdVector.push_back(&en1);
    cr1.contextRegistrationAttributeVector.push_back(&cra1);
    cr1.contextRegistrationAttributeVector.push_back(&cra2);
    cr1.providingApplication.set("http://dummy1.com");
    cr2.entityIdVector.push_back(&en2);
    cr2.contextRegistrationAttributeVector.push_back(&cra3);
    cr2.contextRegistrationAttributeVector.push_back(&cra4);
    cr2.providingApplication.set("http://dummy2.com");
    req.contextRegistrationVector.push_back(&cr1);
    req.contextRegistrationVector.push_back(&cr2);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_EnN_At0_Ok -
*/
TEST(mongoRegisterContextRequest, ceN_EnN_At0_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    EntityId en3("E3", "T3");
    EntityId en4("E4", "T4");
    ContextRegistration cr1, cr2;
    cr1.entityIdVector.push_back(&en1);
    cr1.entityIdVector.push_back(&en2);
    cr1.providingApplication.set("http://dummy1.com");
    cr2.entityIdVector.push_back(&en3);
    cr2.entityIdVector.push_back(&en4);
    cr2.providingApplication.set("http://dummy2.com");
    req.contextRegistrationVector.push_back(&cr1);
    req.contextRegistrationVector.push_back(&cr2);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_EnNnt_At0_Ok -
*/
TEST(mongoRegisterContextRequest, ceN_EnNnt_At0_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    EntityId en3("E3", "");
    EntityId en4("E4", "");
    ContextRegistration cr1, cr2;
    cr1.entityIdVector.push_back(&en1);
    cr1.entityIdVector.push_back(&en2);
    cr1.providingApplication.set("http://dummy1.com");
    cr2.entityIdVector.push_back(&en3);
    cr2.entityIdVector.push_back(&en4);
    cr2.providingApplication.set("http://dummy2.com");
    req.contextRegistrationVector.push_back(&cr1);
    req.contextRegistrationVector.push_back(&cr2);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_EnN_AtN_Ok -
*/
TEST(mongoRegisterContextRequest, ceN_EnN_AtN_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

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
    ContextRegistration cr1, cr2;
    cr1.entityIdVector.push_back(&en1);
    cr1.entityIdVector.push_back(&en2);
    cr1.contextRegistrationAttributeVector.push_back(&cra1);
    cr1.contextRegistrationAttributeVector.push_back(&cra2);
    cr1.providingApplication.set("http://dummy1.com");
    cr2.entityIdVector.push_back(&en3);
    cr2.entityIdVector.push_back(&en4);
    cr2.contextRegistrationAttributeVector.push_back(&cra3);
    cr2.contextRegistrationAttributeVector.push_back(&cra4);
    cr2.providingApplication.set("http://dummy2.com");
    req.contextRegistrationVector.push_back(&cr1);
    req.contextRegistrationVector.push_back(&cr2);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_EnN_AtNnt_Ok -
*/
TEST(mongoRegisterContextRequest, ceN_EnN_AtNnt_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

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
    ContextRegistration cr1, cr2;
    cr1.entityIdVector.push_back(&en1);
    cr1.entityIdVector.push_back(&en2);
    cr1.contextRegistrationAttributeVector.push_back(&cra1);
    cr1.contextRegistrationAttributeVector.push_back(&cra2);
    cr1.providingApplication.set("http://dummy1.com");
    cr2.entityIdVector.push_back(&en3);
    cr2.entityIdVector.push_back(&en4);
    cr2.contextRegistrationAttributeVector.push_back(&cra3);
    cr2.contextRegistrationAttributeVector.push_back(&cra4);
    cr2.providingApplication.set("http://dummy2.com");
    req.contextRegistrationVector.push_back(&cr1);
    req.contextRegistrationVector.push_back(&cr2);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_EnNnt_AtN_Ok -
*/
TEST(mongoRegisterContextRequest, ceN_EnNnt_AtN_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

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
    ContextRegistration cr1, cr2;
    cr1.entityIdVector.push_back(&en1);
    cr1.entityIdVector.push_back(&en2);
    cr1.contextRegistrationAttributeVector.push_back(&cra1);
    cr1.contextRegistrationAttributeVector.push_back(&cra2);
    cr1.providingApplication.set("http://dummy1.com");
    cr2.entityIdVector.push_back(&en3);
    cr2.entityIdVector.push_back(&en4);
    cr2.contextRegistrationAttributeVector.push_back(&cra3);
    cr2.contextRegistrationAttributeVector.push_back(&cra4);
    cr2.providingApplication.set("http://dummy2.com");
    req.contextRegistrationVector.push_back(&cr1);
    req.contextRegistrationVector.push_back(&cr2);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* ceN_EnNnt_AtNnt_Ok -
*/
TEST(mongoRegisterContextRequest, ceN_EnNnt_AtNnt_Ok)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

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
    ContextRegistration cr1, cr2;
    cr1.entityIdVector.push_back(&en1);
    cr1.entityIdVector.push_back(&en2);
    cr1.contextRegistrationAttributeVector.push_back(&cra1);
    cr1.contextRegistrationAttributeVector.push_back(&cra2);
    cr1.providingApplication.set("http://dummy1.com");
    cr2.entityIdVector.push_back(&en3);
    cr2.entityIdVector.push_back(&en4);
    cr2.contextRegistrationAttributeVector.push_back(&cra3);
    cr2.contextRegistrationAttributeVector.push_back(&cra4);
    cr2.providingApplication.set("http://dummy2.com");
    req.contextRegistrationVector.push_back(&cr1);
    req.contextRegistrationVector.push_back(&cr2);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().toString();
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

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
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}



/* ****************************************************************************
*
* defaultDuration -
*/
TEST(mongoRegisterContextRequest, defaultDuration)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res, uriParams);

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
  EXPECT_EQ("PT24H", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* MongoDbUpsertRegistrationFail -
*
*/
TEST(mongoRegisterContextRequest, MongoDbUpsertRegistrationFail)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Prepare mock */
    const DBException e = DBException("boom!!", 33);
    BSONObj fakeEntity = BSON("_id" << BSON("id" << "E1" << "type" << "T1"));
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, count(_, _, _, _, _))
            .WillByDefault(Return(1));
    ON_CALL(*connectionMock, findOne(_, _, _, _))
            .WillByDefault(Return(fakeEntity));
    ON_CALL(*connectionMock, update("utest.registrations", _, _, _, _, _))
            .WillByDefault(Throw(e));

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en);
    cr.providingApplication.set("http://dummy.com");
    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Set MongoDB connection mock (preserving the "actual" connection for later use) */
    DBClientBase* connectionDb = getMongoConnection();
    setMongoConnectionForUnitTest(connectionMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.duration.isEmpty());
    EXPECT_TRUE(res.registrationId.isEmpty());
    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Internal Server Error", res.errorCode.reasonPhrase);

    /* We split the response into to string, to ease comparison, given that the contect of ObjectId is
     * random. ObjectId is 24 characters long.*/
    /* FIXME: a better approach would be to pass a regex filter to relace ObjectId by a constant string,
     * but I don't know how to work with regex in C++ */
    std::string s1 = res.errorCode.details.substr(0, 78);
    std::string s2 = res.errorCode.details.substr(78+24, 22);
    std::string s3 = res.errorCode.details.substr(78+24+22+24, res.errorCode.details.size()-78-24-22-24);
    EXPECT_EQ("Database Error (collection: utest.registrations "
              "- update(): <{ _id: ObjectId('", s1);
    EXPECT_EQ("') },{ _id: ObjectId('", s2);

    EXPECT_EQ("'), expiration: 1360232760, "
              "servicePath: \"/\", "
              "format: \"JSON\", "
              "fwdMode: \"all\", "
              "contextRegistration: [ "
              "{ entities: [ { id: \"E1\", type: \"T1\" } ], "
              "attrs: [], "
              "providingApplication: \"http://dummy.com\" } ] }> "
              "- exception: boom!!)", s3);

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb);

    /* Release mock */
    delete connectionMock;

    /* check collection has not been touched */
    EXPECT_EQ(0, connectionDb->count(REGISTRATIONS_COLL, BSONObj()));
    EXPECT_EQ(0, connectionDb->count(ENTITIES_COLL, BSONObj()));

    utExit();
}


