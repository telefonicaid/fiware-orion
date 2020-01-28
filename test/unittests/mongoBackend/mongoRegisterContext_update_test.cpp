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
#include <vector>

#include "gtest/gtest.h"
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoRegisterContext.h"
#include "ngsi/ContextRegistration.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextRegistrationAttribute.h"
#include "ngsi/Metadata.h"
#include "ngsi/StatusCode.h"
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
using mongo::BSONObjBuilder;
using ::testing::_;
using ::testing::Throw;
using ::testing::Return;



extern void setMongoConnectionForUnitTest(DBClientBase* _connection);

/* ****************************************************************************
*
* Tests
*
* - updateCase1
* - udpateCase2
* - updateCaseNotFound
* - updateCaseWrongIdString
* - updateWrongIdNoHex
*
* - MongoDbFindOneFail
*
* - NotifyContextAvailability1
* - NotifyContextAvailability2
* - NotifyContextAvailability3
*
*/

/* ****************************************************************************
*
* prepareDatabase -
*
* This function is called before every test, to populate some information in the
* registrations collection.
*
*/
static void prepareDatabase(void)
{
  DBClientBase* connection = getMongoConnection();

  /* We create the following registrations:
   *
   * - Reg1: CR: E1 - (A1,A2,A3) - http://cr1.com
   * - Reg2: CR: E1 - A1 - http://cr1.com
   *         CR: E1 - A1 - http://cr3.com
   */

  BSONObj cr1 = BSON("providingApplication" << "http://cr1.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T1")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1") <<
                       BSON("name" << "A2" << "type" << "TA2") <<
                       BSON("name" << "A3" << "type" << "TA3")));

  BSONObj cr2 = BSON("providingApplication" << "http://cr1.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T1")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1")));

  BSONObj cr3 = BSON("providingApplication" << "http://cr2.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T1")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1")));

  BSONObjBuilder reg1;

  reg1.appendElements(BSON(
                        "_id" << OID("51307b66f481db11bf860001") <<
                        "expiration" << 10000000 <<
                        "contextRegistration" << BSON_ARRAY(cr1) <<
                        "servicePath" << "/"));

  BSONObjBuilder reg2;
  reg2.appendElements(BSON(
                        "_id" << OID("51307b66f481db11bf860002") <<
                        "expiration" << 20000000 <<
                        "contextRegistration" << BSON_ARRAY(cr2 << cr3) <<
                        "servicePath" << "/"));

  /* 1879048191 corresponds to year 2029 so we avoid any expiration problem in the next 16 years :) */
  BSONObj sub1 = BSON("_id" << OID("51307b66f481db11bf860010") <<
                      "expiration" << 1879048191 <<
                      "reference" << "http://notify1.me" <<
                      "entities" << BSON_ARRAY(BSON("id" << "E5" << "type" << "T5" << "isPattern" << "false")) <<
                      "attrs" << BSONArray());

  BSONObj sub2 = BSON("_id" << OID("51307b66f481db11bf860020") <<
                      "expiration" << 1879048191 <<
                      "reference" << "http://notify2.me" <<
                      "entities" << BSON_ARRAY(BSON("id" << "E5" << "type" << "T5" << "isPattern" << "false")) <<
                      "attrs" << BSON_ARRAY("A1"));

  connection->insert(REGISTRATIONS_COLL, reg1.obj());
  connection->insert(REGISTRATIONS_COLL, reg2.obj());

  connection->insert(SUBSCRIBECONTEXTAVAIL_COLL, sub1);
  connection->insert(SUBSCRIBECONTEXTAVAIL_COLL, sub2);
}



/* ****************************************************************************
*
* updateCase1 -
*/
TEST(mongoRegisterContext_update, updateCase1)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  utInit();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistrationAttribute cra("A1", "TA1");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.contextRegistrationAttributeVector.push_back(&cra);
  cr.providingApplication.set("http://newurl.com");
  req.contextRegistrationVector.push_back(&cr);
  req.registrationId.set("51307b66f481db11bf860001");
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res, uriParams, "", "/");
  EXPECT_EQ(200, ms);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(2, connection->count(REGISTRATIONS_COLL, BSONObj()));

  BSONObj reg, contextRegistration, ent, ent0, attr0;
  std::vector<BSONElement> contextRegistrationV, entities, attrs;

  /* reg #1 */
  reg = connection->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));
  EXPECT_EQ("51307b66f481db11bf860001", reg.getField("_id").OID().toString());
  EXPECT_EQ(1360232760, reg.getIntField("expiration"));

  contextRegistrationV = reg.getField("contextRegistration").Array();
  ASSERT_EQ(1, contextRegistrationV.size());
  contextRegistration = contextRegistrationV[0].embeddedObject();

  EXPECT_STREQ("http://newurl.com", C_STR_FIELD(contextRegistration, "providingApplication"));
  entities = contextRegistration.getField("entities").Array();
  ASSERT_EQ(1, entities.size());
  ent0 = entities[0].embeddedObject();
  EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

  attrs = contextRegistration.getField("attrs").Array();
  ASSERT_EQ(1, attrs.size());
  attr0 = attrs[0].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
  EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));

  /* reg #2 (untouched) */
  reg = connection->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
  EXPECT_EQ("51307b66f481db11bf860002", reg.getField("_id").OID().toString());
  EXPECT_EQ(20000000, reg.getIntField("expiration"));

  contextRegistrationV = reg.getField("contextRegistration").Array();
  ASSERT_EQ(2, contextRegistrationV.size());
  contextRegistration = contextRegistrationV[0].embeddedObject();

  EXPECT_STREQ("http://cr1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
  entities = contextRegistration.getField("entities").Array();
  ASSERT_EQ(1, entities.size());
  ent0 = entities[0].embeddedObject();
  EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

  attrs = contextRegistration.getField("attrs").Array();
  ASSERT_EQ(1, attrs.size());
  attr0 = attrs[0].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
  EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));

  contextRegistration = contextRegistrationV[1].embeddedObject();

  EXPECT_STREQ("http://cr2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
  entities = contextRegistration.getField("entities").Array();
  ASSERT_EQ(1, entities.size());
  ent0 = entities[0].embeddedObject();
  EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

  attrs = contextRegistration.getField("attrs").Array();
  ASSERT_EQ(1, attrs.size());
  attr0 = attrs[0].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
  EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ("51307b66f481db11bf860001", res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* updateCase2 -
*/
TEST(mongoRegisterContext_update, updateCase2)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    ContextRegistrationAttribute cra("A1", "TA1");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en);
    cr.contextRegistrationAttributeVector.push_back(&cra);
    cr.providingApplication.set("http://newurl.com");
    req.contextRegistrationVector.push_back(&cr);
    req.registrationId.set("51307b66f481db11bf860002");
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
    ASSERT_EQ(2, connection->count(REGISTRATIONS_COLL, BSONObj()));

    BSONObj reg, contextRegistration, ent, ent0, attr0, attr1, attr2;
    std::vector<BSONElement> contextRegistrationV, entities, attrs;

    /* reg #1 (untouched) */
    reg = connection->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));
    EXPECT_EQ("51307b66f481db11bf860001", reg.getField("_id").OID().toString());
    EXPECT_EQ(10000000, reg.getIntField("expiration"));

    contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(1, contextRegistrationV.size());
    contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://cr1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    attr0 = attrs[0].embeddedObject();
    attr1 = attrs[1].embeddedObject();
    attr2 = attrs[2].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(attr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(attr1, "type"));
    EXPECT_STREQ("A3", C_STR_FIELD(attr2, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(attr2, "type"));

    /* reg #2 */
    reg = connection->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_EQ("51307b66f481db11bf860002", reg.getField("_id").OID().toString());
    EXPECT_EQ(1360232760, reg.getIntField("expiration"));

    contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(1, contextRegistrationV.size());
    contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://newurl.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    attr0 = attrs[0].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ("51307b66f481db11bf860002", res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}

/* ****************************************************************************
*
* updateNotFound -
*/
TEST(mongoRegisterContext_update, updateNotFound)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    ContextRegistrationAttribute cra("A1", "TA1");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en);
    cr.contextRegistrationAttributeVector.push_back(&cra);
    cr.providingApplication.set("http://newurl.com");
    req.contextRegistrationVector.push_back(&cr);
    req.registrationId.set("51307b66f481db11bf860003");
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
    ASSERT_EQ(2, connection->count(REGISTRATIONS_COLL, BSONObj()));

    BSONObj reg, contextRegistration, ent0, attr0, attr1, attr2;
    std::vector<BSONElement> contextRegistrationV, entities, attrs;

    /* reg #1 (untouched) */
    reg = connection->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));
    EXPECT_EQ("51307b66f481db11bf860001", reg.getField("_id").OID().toString());
    EXPECT_EQ(10000000, reg.getIntField("expiration"));

    contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(1, contextRegistrationV.size());
    contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://cr1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    attr0 = attrs[0].embeddedObject();
    attr1 = attrs[1].embeddedObject();
    attr2 = attrs[2].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(attr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(attr1, "type"));
    EXPECT_STREQ("A3", C_STR_FIELD(attr2, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(attr2, "type"));

    /* reg #2 (untouched) */
    reg = connection->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_EQ("51307b66f481db11bf860002", reg.getField("_id").OID().toString());
    EXPECT_EQ(20000000, reg.getIntField("expiration"));

    contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());
    contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://cr1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    attr0 = attrs[0].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));

    contextRegistration = contextRegistrationV[1].embeddedObject();

    EXPECT_STREQ("http://cr2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    attr0 = attrs[0].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(0, res.duration.get().size());
    EXPECT_EQ("51307b66f481db11bf860003", res.registrationId.get());
    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ("registration id: /51307b66f481db11bf860003/", res.errorCode.details);

    utExit();
}

/* ****************************************************************************
*
* updateWrongIdString -
*/
TEST(mongoRegisterContext_update, updateWrongIdString)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    ContextRegistrationAttribute cra("A1", "TA1");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en);
    cr.contextRegistrationAttributeVector.push_back(&cra);
    cr.providingApplication.set("http://newurl.com");
    req.contextRegistrationVector.push_back(&cr);
    req.registrationId.set("51307b66f481db11bf861111");
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
    ASSERT_EQ(2, connection->count(REGISTRATIONS_COLL, BSONObj()));

    BSONObj reg, contextRegistration, ent0, attr0, attr1, attr2;
    std::vector<BSONElement> contextRegistrationV, entities, attrs;

    /* reg #1 (untouched) */
    reg = connection->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));
    EXPECT_EQ("51307b66f481db11bf860001", reg.getField("_id").OID().toString());
    EXPECT_EQ(10000000, reg.getIntField("expiration")) << "wrong expiration (reg #1)";

    contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(1, contextRegistrationV.size());
    contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://cr1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size()) << "wrong number of entities in registration (reg #1, cr #1)";
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    attr0 = attrs[0].embeddedObject();
    attr1 = attrs[1].embeddedObject();
    attr2 = attrs[2].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(attr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(attr1, "type"));
    EXPECT_STREQ("A3", C_STR_FIELD(attr2, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(attr2, "type"));

    /* reg #2 (untouched) */
    reg = connection->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_EQ("51307b66f481db11bf860002", reg.getField("_id").OID().toString());
    EXPECT_EQ(20000000, reg.getIntField("expiration"));

    contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());
    contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://cr1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    attr0 = attrs[0].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));

    contextRegistration = contextRegistrationV[1].embeddedObject();

    EXPECT_STREQ("http://cr2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    attr0 = attrs[0].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(0, res.duration.get().size());
    EXPECT_EQ("51307b66f481db11bf861111", res.registrationId.get());
    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ("registration id: /51307b66f481db11bf861111/", res.errorCode.details);

    utExit();
}

/* ****************************************************************************
*
* updateWrongIdNoHex -
*
* FIXME P3: check that we cover this case in the proper place, e.g. check() in the pre-mongoBackend layers, before permanent removal
*/
TEST(DISABLED_mongoRegisterContext_update, updateWrongIdNoHex)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    ContextRegistrationAttribute cra("A1", "TA1");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en);
    cr.contextRegistrationAttributeVector.push_back(&cra);
    cr.providingApplication.set("http://newurl.com");
    req.contextRegistrationVector.push_back(&cr);
    req.registrationId.set("51307b66f481db11bf8600XX");
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
    ASSERT_EQ(2, connection->count(REGISTRATIONS_COLL, BSONObj()));

    BSONObj reg, contextRegistration, ent0, attr0, attr1, attr2;
    std::vector<BSONElement> contextRegistrationV, entities, attrs;

    /* reg #1 (untouched) */
    reg = connection->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));
    EXPECT_EQ("51307b66f481db11bf860001", reg.getField("_id").OID().toString());
    EXPECT_EQ(10000000, reg.getIntField("expiration"));

    contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(1, contextRegistrationV.size());
    contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://cr1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    attr0 = attrs[0].embeddedObject();
    attr1 = attrs[1].embeddedObject();
    attr2 = attrs[2].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(attr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(attr1, "type"));
    EXPECT_STREQ("A3", C_STR_FIELD(attr2, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(attr2, "type"));

    /* reg #2 (untouched) */
    reg = connection->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_EQ("51307b66f481db11bf860002", reg.getField("_id").OID().toString());
    EXPECT_EQ(20000000, reg.getIntField("expiration"));

    contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());
    contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://cr1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    attr0 = attrs[0].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));

    contextRegistration = contextRegistrationV[1].embeddedObject();

    EXPECT_STREQ("http://cr2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    attr0 = attrs[0].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(0, res.duration.get().size());
    EXPECT_EQ("51307b66f481db11bf8600XX", res.registrationId.get());
    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("Registration Not Found", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    utExit();
}



/* ****************************************************************************
*
* MongoDbFindOneFail -
*/
TEST(mongoRegisterContext_update, MongoDbFindOneFail)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    utInit();

    /* Prepare mock */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, findOne("utest.registrations", _, _, _))
            .WillByDefault(Throw(e));

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    ContextRegistrationAttribute cra("A1", "TA1");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en);
    cr.contextRegistrationAttributeVector.push_back(&cra);
    cr.providingApplication.set("http://newurl.com");
    req.contextRegistrationVector.push_back(&cr);
    req.registrationId.set("51307b66f481db11bf860001");
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Set MongoDB connection mock (preserving "actual" connection for later use) */
    DBClientBase* connectionDb = getMongoConnection();
    setMongoConnectionForUnitTest(connectionMock, getMongoConnectionCxx());

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.duration.isEmpty());
    EXPECT_TRUE(res.registrationId.isEmpty());
    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Internal Server Error", res.errorCode.reasonPhrase);
    EXPECT_EQ("Database Error (collection: utest.registrations "
              "- findOne(): { _id: ObjectId('51307b66f481db11bf860001'), servicePath: \"/\" } "
              "- exception: boom!!)", res.errorCode.details);

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb, getMongoConnectionCxx());

    /* Release mock */
    delete connectionMock;

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    /* registrations collection: */
    ASSERT_EQ(2, connectionDb->count(REGISTRATIONS_COLL, BSONObj()));

    BSONObj reg, contextRegistration, ent0, attr0, attr1, attr2;
    std::vector<BSONElement> contextRegistrationV, entities, attrs;

    /* reg #1 (untouched) */
    reg = connectionDb->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));
    EXPECT_EQ("51307b66f481db11bf860001", reg.getField("_id").OID().toString());
    EXPECT_EQ(10000000, reg.getIntField("expiration"));

    contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(1, contextRegistrationV.size());
    contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://cr1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    attr0 = attrs[0].embeddedObject();
    attr1 = attrs[1].embeddedObject();
    attr2 = attrs[2].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(attr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(attr1, "type"));
    EXPECT_STREQ("A3", C_STR_FIELD(attr2, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(attr2, "type"));

    /* reg #2 (untouched) */
    reg = connectionDb->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_EQ("51307b66f481db11bf860002", reg.getField("_id").OID().toString());
    EXPECT_EQ(20000000, reg.getIntField("expiration"));

    contextRegistrationV = reg.getField("contextRegistration").Array();
    ASSERT_EQ(2, contextRegistrationV.size());
    contextRegistration = contextRegistrationV[0].embeddedObject();

    EXPECT_STREQ("http://cr1.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    attr0 = attrs[0].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));

    contextRegistration = contextRegistrationV[1].embeddedObject();

    EXPECT_STREQ("http://cr2.com", C_STR_FIELD(contextRegistration, "providingApplication"));
    entities = contextRegistration.getField("entities").Array();
    ASSERT_EQ(1, entities.size());
    ent0 = entities[0].embeddedObject();
    EXPECT_STREQ("E1", C_STR_FIELD(ent0, "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent0, "type"));

    attrs = contextRegistration.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    attr0 = attrs[0].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(attr0, "type"));

    utExit();
}

/* ****************************************************************************
*
* NotifyContextAvailability1 -
*/
TEST(mongoRegisterContext_update, NotifyContextAvailability1)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  utInit(false, true);  // TimerMock only - NOT NotifierMock

  /* Prepare mock */
  NotifyContextAvailabilityRequest  expectedNcar;
  EntityId                          mockEn1("E5", "T5", "false");
  ContextRegistrationResponse       crr;

  crr.contextRegistration.entityIdVector.push_back(&mockEn1);
  crr.contextRegistration.providingApplication.set("http://dummy.com");

  expectedNcar.contextRegistrationResponseVector.push_back(&crr);
  expectedNcar.subscriptionId.set("51307b66f481db11bf860010");

  NotifierMock* notifierMock = new NotifierMock();

  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                  "http://notify1.me",
                                                                  "",
                                                                  "no correlator",
                                                                  NGSI_V1_LEGACY)).Times(1);

  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,
                                                                  "http://notify2.me",
                                                                  "",
                                                                  "no correlator",
                                                                  NGSI_V1_LEGACY)).Times(0);
  setNotifier(notifierMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E5", "T5", "false");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.registrationId.set("51307b66f481db11bf860001");
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ("51307b66f481db11bf860001", res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* The only collection affected by this operation is registrations, which has been extensively
   * testbed by other unit tests, so we don't include checking in the present unit test */

  /* Delete mock */
  delete notifierMock;

  utExit();
}



/* ****************************************************************************
*
* NotifyContextAvailability2 -
*/
TEST(mongoRegisterContext_update, NotifyContextAvailability2)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  utInit(false, true);  // TimerMock only - NOT NotifierMock

  /* Prepare mock */
  NotifyContextAvailabilityRequest expectedNcar1, expectedNcar2;
  EntityId mockEn1("E5", "T5", "false");
  ContextRegistrationAttribute mockCra("A1", "TA1");
  ContextRegistrationResponse crr;
  crr.contextRegistration.entityIdVector.push_back(&mockEn1);
  crr.contextRegistration.contextRegistrationAttributeVector.push_back(&mockCra);
  crr.contextRegistration.providingApplication.set("http://dummy.com");
  expectedNcar1.contextRegistrationResponseVector.push_back(&crr);
  expectedNcar1.subscriptionId.set("51307b66f481db11bf860010");

  expectedNcar2.contextRegistrationResponseVector.push_back(&crr);
  expectedNcar2.subscriptionId.set("51307b66f481db11bf860020");

  NotifierMock* notifierMock = new NotifierMock();

  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar1),
                                                                  "http://notify1.me",
                                                                  "",
                                                                  "no correlator",
                                                                  NGSI_V1_LEGACY)).Times(1);

  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar2),
                                                                  "http://notify2.me",
                                                                  "",
                                                                  "no correlator",
                                                                  NGSI_V1_LEGACY)).Times(1);

  setNotifier(notifierMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E5", "T5", "false");
  ContextRegistrationAttribute cra("A1", "TA1");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.contextRegistrationAttributeVector.push_back(&cra);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.registrationId.set("51307b66f481db11bf860001");
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ("51307b66f481db11bf860001", res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* The only collection affected by this operation is registrations, which has been extensively
   * testbed by other unit tests, so we don't include checking in the present unit test */

  /* Delete mock */
  delete notifierMock;

  utExit();
}

/* ****************************************************************************
*
* NotifyContextAvailability3 -
*/
TEST(mongoRegisterContext_update, NotifyContextAvailability3)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  utInit(false, true);  // TimerMock only - NOT NotifierMock

  /* Prepare mock */
  NotifyContextAvailabilityRequest expectedNcar;
  EntityId mockEn1("E5", "T5", "false");
  ContextRegistrationAttribute mockCra("A2", "TA2");
  ContextRegistrationResponse crr;
  crr.contextRegistration.entityIdVector.push_back(&mockEn1);
  crr.contextRegistration.contextRegistrationAttributeVector.push_back(&mockCra);
  crr.contextRegistration.providingApplication.set("http://dummy.com");
  expectedNcar.contextRegistrationResponseVector.push_back(&crr);
  expectedNcar.subscriptionId.set("51307b66f481db11bf860010");

  NotifierMock* notifierMock = new NotifierMock();

  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),
                                                                  "http://notify1.me",
                                                                  "",
                                                                  "no correlator",
                                                                  NGSI_V1_LEGACY)).Times(1);

  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,
                                                                  "http://notify2.me",
                                                                  "",
                                                                  "no correlator",
                                                                  NGSI_V1_LEGACY)).Times(0);

  setNotifier(notifierMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E5", "T5", "false");
  ContextRegistrationAttribute cra("A2", "TA2");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.contextRegistrationAttributeVector.push_back(&cra);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.registrationId.set("51307b66f481db11bf860001");
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ("51307b66f481db11bf860001", res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* The only collection affected by this operation is registrations, which has been extensively
   * testbed by other unit tests, so we don't include checking in the present unit test */

  /* Delete mock */
  delete notifierMock;

  utExit();
}
