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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include "gtest/gtest.h"
#include "testInit.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoRegisterContext.h"
#include "ngsi/StatusCode.h"
#include "ngsi/ContextRegistration.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextRegistrationAttribute.h"
#include "ngsi/Metadata.h"
#include "ngsi9/RegisterContextRequest.h"
#include "ngsi9/RegisterContextResponse.h"

#include "mongo/client/dbclient.h"

#include "commonMocks.h"

using ::testing::_;
using ::testing::Throw;
using ::testing::Return;

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
* In addition to these 24 tests, there are  4 tests that check cases when there are documents in the
* entities collection that match with the registration (prefixed with "pre")
*
* In addition, we include some tests to check of notify context availability triggering upon
* creation of context registration:
*
* - NotifyContextAvailability1
* - NotifyContextAvailability2
* - NotifyContextAvailability3
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
* - MongoDbCAsubsFindFail
*
* In addition, we include some tests related with associations metadata processing
*
* - AssociationsOk
* - AssociationsDbFail
*
* Note these tests are not "canonical" unit tests. Canon says that in this case we should have
* mocked MongoDB. Actually, we think is very much powerfull to check that everything is ok at
* MongoDB layer.
*
*/

/* ****************************************************************************
*
* prepareDatabase -
*/
static void prepareDatabase(void) {

    /* Clean database */
    setupDatabase();

    DBClientConnection* connection = getMongoConnection();

    /* 1879048191 corresponds to year 2029 so we avoid any expiration problem in the next 16 years :) */
    BSONObj sub1 = BSON("_id" << OID("51307b66f481db11bf860001") <<
                        "expiration" << 1879048191 <<
                        "reference" << "http://notify1.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E5" << "type" << "T5" << "isPattern" << "false")) <<
                        "attrs" << BSONArray());

    BSONObj sub2 = BSON("_id" << OID("51307b66f481db11bf860002") <<
                        "expiration" << 1879048191 <<
                        "reference" << "http://notify2.me" <<
                        "entities" << BSON_ARRAY(BSON("id" << "E5" << "type" << "T5" << "isPattern" << "false")) <<
                        "attrs" << BSON_ARRAY("A1"));

    connection->insert(SUBSCRIBECONTEXTAVAIL_COLL, sub1);
    connection->insert(SUBSCRIBECONTEXTAVAIL_COLL, sub2);
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

  /* Prepare mock */
  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
          .Times(0);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

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
  ms = mongoRegisterContext(&req, &res);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientConnection* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().str();
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

  /* entities collection: */
  ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));
  BSONObj ent = connection->findOne(ENTITIES_COLL, BSONObj());

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(0, ent.getField("attrs").Array().size());
  EXPECT_EQ(1360232700, ent.getIntField("creDate"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

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

  /* Prepare mock */
  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
          .Times(0);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

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
  ms = mongoRegisterContext(&req, &res);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientConnection* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().str();
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

  /* entities collection: */
  ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));
  BSONObj ent = connection->findOne(ENTITIES_COLL, BSONObj());

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
  EXPECT_EQ(0, ent.getField("attrs").Array().size());
  EXPECT_EQ(1360232700, ent.getIntField("creDate"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

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

  /* Prepare mock */
  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
          .Times(0);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistrationAttribute cra1("A1", "TA1", "false");
  ContextRegistrationAttribute cra2("A2", "TA2", "true");
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
  ms = mongoRegisterContext(&req, &res);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientConnection* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().str();
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
  EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
  EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
  EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));
  EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

  /* entities collection: */
  ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));
  BSONObj ent = connection->findOne(ENTITIES_COLL, BSONObj());

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(1360232700, ent.getIntField("creDate"));
  std::vector<BSONElement> entAttrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, entAttrs.size());
  BSONObj eattr0 = entAttrs[0].embeddedObject();
  BSONObj eattr1 = entAttrs[1].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
  EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
  EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
  EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

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

  /* Prepare mock */
  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
          .Times(0);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistrationAttribute cra1("A1", "", "false");
  ContextRegistrationAttribute cra2("A2", "", "true");
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
  ms = mongoRegisterContext(&req, &res);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientConnection* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().str();
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
  EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
  EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
  EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));
  EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

  /* entities collection: */
  ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));
  BSONObj ent = connection->findOne(ENTITIES_COLL, BSONObj());

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(1360232700, ent.getIntField("creDate"));
  std::vector<BSONElement> entAttrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, entAttrs.size());
  BSONObj eattr0 = entAttrs[0].embeddedObject();
  BSONObj eattr1 = entAttrs[1].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
  EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
  EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
  EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

}

/* ****************************************************************************
*
* ce1_En1nt_AtN_Ok -
*/
TEST(mongoRegisterContextRequest, ce1_En1nt_AtN_Ok)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  /* Prepare mock */
  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
          .Times(0);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "");
  ContextRegistrationAttribute cra1("A1", "TA1", "false");
  ContextRegistrationAttribute cra2("A2", "TA2", "true");
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
  ms = mongoRegisterContext(&req, &res);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientConnection* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().str();
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
  EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
  EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
  EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));
  EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

  /* db.csubs.findOne({_id: ObjectId("$SUB_ID")}, {_id: 0, count: 1}): */
  ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));
  BSONObj ent = connection->findOne(ENTITIES_COLL, BSONObj());

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
  EXPECT_EQ(1360232700, ent.getIntField("creDate"));
  std::vector<BSONElement> entAttrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, entAttrs.size());
  BSONObj eattr0 = entAttrs[0].embeddedObject();
  BSONObj eattr1 = entAttrs[1].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
  EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
  EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
  EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

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

  /* Prepare mock */
  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
          .Times(0);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "");
  ContextRegistrationAttribute cra1("A1", "", "false");
  ContextRegistrationAttribute cra2("A2", "", "true");
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
  ms = mongoRegisterContext(&req, &res);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientConnection* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().str();
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
  EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
  EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
  EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));
  EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

  /* entities collection: */
  ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));
  BSONObj ent = connection->findOne(ENTITIES_COLL, BSONObj());

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
  EXPECT_EQ(1360232700, ent.getIntField("creDate"));
  std::vector<BSONElement> entAttrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, entAttrs.size());
  BSONObj eattr0 = entAttrs[0].embeddedObject();
  BSONObj eattr1 = entAttrs[1].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
  EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
  EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
  EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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

    /* entities collection: */
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    BSONObj ent;
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(0, ent.getField("attrs").Array().size());
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(0, ent.getField("attrs").Array().size());
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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

    /* entities collection: */
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    BSONObj ent;
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(0, ent.getField("attrs").Array().size());
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(0, ent.getField("attrs").Array().size());
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationAttribute cra1("A1", "TA1", "false");
    ContextRegistrationAttribute cra2("A2", "TA2", "true");
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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

    /* entities collection: */
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    BSONObj ent, eattr0, eattr1;
    std::vector<BSONElement> entAttrs;
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationAttribute cra1("A1", "", "false");
    ContextRegistrationAttribute cra2("A2", "", "true");
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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

    /* entities collection: */
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    BSONObj ent, eattr0, eattr1;
    std::vector<BSONElement> entAttrs;
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationAttribute cra1("A1", "TA1", "false");
    ContextRegistrationAttribute cra2("A2", "TA2", "true");
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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

    /* entities collection: */
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    BSONObj ent, eattr0, eattr1;
    std::vector<BSONElement> entAttrs;
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationAttribute cra1("A1", "", "false");
    ContextRegistrationAttribute cra2("A2", "", "true");
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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

    /* entities collection: */
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    BSONObj ent, eattr0, eattr1;
    std::vector<BSONElement> entAttrs;
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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

    /* entities collection: */
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));
    BSONObj ent;
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    EXPECT_EQ(0, ent.getField("attrs").Array().size());
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    EXPECT_EQ(0, ent.getField("attrs").Array().size());

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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

    /* entities collection: */
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));
    BSONObj ent;
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    EXPECT_EQ(0, ent.getField("attrs").Array().size());
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    EXPECT_EQ(0, ent.getField("attrs").Array().size());

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationAttribute cra1("A1", "TA1", "false");
    ContextRegistrationAttribute cra2("A2", "TA2", "true");
    ContextRegistrationAttribute cra3("A3", "TA3", "false");
    ContextRegistrationAttribute cra4("A4", "TA4", "true");
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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

    /* entities collection: */
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));
    BSONObj ent, eattr0, eattr1;
    std::vector<BSONElement> entAttrs;
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(eattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    ContextRegistrationAttribute cra1("A1", "", "false");
    ContextRegistrationAttribute cra2("A2", "", "true");
    ContextRegistrationAttribute cra3("A3", "", "false");
    ContextRegistrationAttribute cra4("A4", "", "true");
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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

    /* entities collection: */
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));
    BSONObj ent, eattr0, eattr1;
    std::vector<BSONElement> entAttrs;
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationAttribute cra1("A1", "TA1", "false");
    ContextRegistrationAttribute cra2("A2", "TA2", "true");
    ContextRegistrationAttribute cra3("A3", "TA3", "false");
    ContextRegistrationAttribute cra4("A4", "TA4", "true");
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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

    /* entities collection: */
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));
    BSONObj ent, eattr0, eattr1;
    std::vector<BSONElement> entAttrs;
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(eattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    ContextRegistrationAttribute cra1("A1", "", "false");
    ContextRegistrationAttribute cra2("A2", "", "true");
    ContextRegistrationAttribute cra3("A3", "", "false");
    ContextRegistrationAttribute cra4("A4", "", "true");
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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

    /* entities collection: */
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));
    BSONObj ent, eattr0, eattr1;
    std::vector<BSONElement> entAttrs;
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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

    /* entities collection: */
    ASSERT_EQ(4, connection->count(ENTITIES_COLL, BSONObj()));
    BSONObj ent;
    std::vector<BSONElement> entAttrs;

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    EXPECT_EQ(0, entAttrs.size());

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    EXPECT_EQ(0, entAttrs.size());

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    EXPECT_EQ(0, entAttrs.size());

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E4" << "_id.type" << "T4"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T4", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    EXPECT_EQ(0, entAttrs.size());

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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

    /* entities collection: */
    ASSERT_EQ(4, connection->count(ENTITIES_COLL, BSONObj()));
    BSONObj ent;
    std::vector<BSONElement> entAttrs;

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    EXPECT_EQ(0, entAttrs.size());

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    EXPECT_EQ(0, entAttrs.size());

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    EXPECT_EQ(0, entAttrs.size());

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E4"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    EXPECT_EQ(0, entAttrs.size());

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    EntityId en3("E3", "T3");
    EntityId en4("E4", "T4");
    ContextRegistrationAttribute cra1("A1", "TA1", "false");
    ContextRegistrationAttribute cra2("A2", "TA2", "true");
    ContextRegistrationAttribute cra3("A3", "TA3", "false");
    ContextRegistrationAttribute cra4("A4", "TA4", "true");
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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

    /* entities collection: */
    ASSERT_EQ(4, connection->count(ENTITIES_COLL, BSONObj()));
    BSONObj ent, eattr0, eattr1;
    std::vector<BSONElement> entAttrs;

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E4" << "_id.type" << "T4"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T4", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(eattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    EntityId en3("E3", "T3");
    EntityId en4("E4", "T4");
    ContextRegistrationAttribute cra1("A1", "", "false");
    ContextRegistrationAttribute cra2("A2", "", "true");
    ContextRegistrationAttribute cra3("A3", "", "false");
    ContextRegistrationAttribute cra4("A4", "", "true");
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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

    /* entities collection: */
    ASSERT_EQ(4, connection->count(ENTITIES_COLL, BSONObj()));
    BSONObj ent, eattr0, eattr1;
    std::vector<BSONElement> entAttrs;

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E4" << "_id.type" << "T4"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T4", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    EntityId en3("E3", "");
    EntityId en4("E4", "");
    ContextRegistrationAttribute cra1("A1", "TA1", "false");
    ContextRegistrationAttribute cra2("A2", "TA2", "true");
    ContextRegistrationAttribute cra3("A3", "TA3", "false");
    ContextRegistrationAttribute cra4("A4", "TA4", "true");
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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

    /* entities collection: */
    ASSERT_EQ(4, connection->count(ENTITIES_COLL, BSONObj()));
    BSONObj ent, eattr0, eattr1;
    std::vector<BSONElement> entAttrs;

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E4"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(eattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

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

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "");
    EntityId en2("E2", "");
    EntityId en3("E3", "");
    EntityId en4("E4", "");
    ContextRegistrationAttribute cra1("A1", "", "false");
    ContextRegistrationAttribute cra2("A2", "", "true");
    ContextRegistrationAttribute cra3("A3", "", "false");
    ContextRegistrationAttribute cra4("A4", "", "true");
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
    ms = mongoRegisterContext(&req, &res);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* registrations collection: */
    ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
    BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
    std::string oid = reg.getField("_id").OID().str();
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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

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
    EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
    EXPECT_STREQ("A4", C_STR_FIELD(rattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(rattr1, "type"));
    EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

    /* entities collection: */
    ASSERT_EQ(4, connection->count(ENTITIES_COLL, BSONObj()));
    BSONObj ent, eattr0, eattr1;
    std::vector<BSONElement> entAttrs;

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E4"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    entAttrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, entAttrs.size());
    eattr0 = entAttrs[0].embeddedObject();
    eattr1 = entAttrs[1].embeddedObject();
    EXPECT_STREQ("A3", C_STR_FIELD(eattr0, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr0, "type"));
    EXPECT_STREQ("A4", C_STR_FIELD(eattr1, "name"));
    EXPECT_STREQ("", C_STR_FIELD(eattr1, "type"));

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_EQ(oid, res.registrationId.get());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;
    delete notifierMock;

}

/* ****************************************************************************
*
* preEqualRegistrationNoAttrs -
*/
TEST(mongoRegisterContextRequest, preEqualRegistrationNoAttrs)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  /* Prepare mock */
  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
          .Times(0);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.providingApplication.set("http://dummy.com");

  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Set initial state in database */
  DBClientConnection* connection = getMongoConnection();
  connection->insert(getEntitiesCollectionName(), BSON("_id" << BSON("id" << "E1" << "type" << "T1") << "attrs" << BSONArray()));

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().str();
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

  /* entities collection: */
  ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));
  BSONObj ent = connection->findOne(ENTITIES_COLL, BSONObj());

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(0, ent.getField("attrs").Array().size());

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

}

/* ****************************************************************************
*
* preLessAttributesInReg -
*/

TEST(mongoRegisterContextRequest, preLessAttributesInReg)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  /* Prepare mock */
  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
          .Times(0);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistrationAttribute cra("A1", "TA1", "false");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.contextRegistrationAttributeVector.push_back(&cra);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Set initial state in database */
  DBClientConnection* connection = getMongoConnection();
  BSONArray attrs = BSON_ARRAY(BSON("name" << "A1" << "type" << "TA1") << BSON("name" << "A2" << "type" << "TA2"));
  connection->insert(getEntitiesCollectionName(), BSON("_id" << BSON("id" << "E1" << "type" << "T1") << "attrs" << attrs));

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().str();
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
  ASSERT_EQ(1, regAttrs.size());
  BSONObj rattr0 = regAttrs[0].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
  EXPECT_STREQ("TA1", C_STR_FIELD(rattr0, "type"));
  EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));

  /* entities collection: */
  ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));
  BSONObj ent = connection->findOne(ENTITIES_COLL, BSONObj());

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  std::vector<BSONElement> entAttrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, entAttrs.size());
  BSONObj eattr0 = entAttrs[0].embeddedObject();
  BSONObj eattr1 = entAttrs[1].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
  EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
  EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
  EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

}

/* ****************************************************************************
*
* preMoreAttributesInReg -
*/

TEST(mongoRegisterContextRequest, preMoreAttributesInReg)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  /* Prepare mock */
  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
          .Times(0);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistrationAttribute cra1("A1", "TA1", "false");
  ContextRegistrationAttribute cra2("A2", "TA2", "true");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.contextRegistrationAttributeVector.push_back(&cra1);
  cr.contextRegistrationAttributeVector.push_back(&cra2);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Set initial state in database */
  DBClientConnection* connection = getMongoConnection();
  BSONArray attrs = BSON_ARRAY(BSON("name" << "A1" << "type" << "TA1"));
  connection->insert(getEntitiesCollectionName(), BSON("_id" << BSON("id" << "E1" << "type" << "T1") << "attrs" << attrs));

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().str();
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
  EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));
  EXPECT_STREQ("A2", C_STR_FIELD(rattr1, "name"));
  EXPECT_STREQ("TA2", C_STR_FIELD(rattr1, "type"));
  EXPECT_STREQ("true", C_STR_FIELD(rattr1, "isDomain"));

  /* entities collection: */
  ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));
  BSONObj ent = connection->findOne(ENTITIES_COLL, BSONObj());

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  std::vector<BSONElement> entAttrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, entAttrs.size());
  BSONObj eattr0 = entAttrs[0].embeddedObject();
  BSONObj eattr1 = entAttrs[1].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
  EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
  EXPECT_STREQ("A2", C_STR_FIELD(eattr1, "name"));
  EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

}

/* ****************************************************************************
*
* preSameNameDifferentType -
*/

TEST(mongoRegisterContextRequest, preMoreAttributesInRegSameNameDifferentType)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;  

  /* Prepare mock */
  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
          .Times(0);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistrationAttribute cra("A1", "TA2", "false");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.contextRegistrationAttributeVector.push_back(&cra);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Set initial state in database */
  DBClientConnection* connection = getMongoConnection();
  BSONArray attrs = BSON_ARRAY(BSON("name" << "A1" << "type" << "TA1"));
  connection->insert(getEntitiesCollectionName(), BSON("_id" << BSON("id" << "E1" << "type" << "T1") << "attrs" << attrs));

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().str();
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
  ASSERT_EQ(1, regAttrs.size());
  BSONObj rattr0 = regAttrs[0].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(rattr0, "name"));
  EXPECT_STREQ("TA2", C_STR_FIELD(rattr0, "type"));
  EXPECT_STREQ("false", C_STR_FIELD(rattr0, "isDomain"));

  /* entities collection: */
  ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));
  BSONObj ent = connection->findOne(ENTITIES_COLL, BSONObj());

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  std::vector<BSONElement> entAttrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, entAttrs.size());
  BSONObj eattr0 = entAttrs[0].embeddedObject();
  BSONObj eattr1 = entAttrs[1].embeddedObject();
  EXPECT_STREQ("A1", C_STR_FIELD(eattr0, "name"));
  EXPECT_STREQ("TA1", C_STR_FIELD(eattr0, "type"));
  EXPECT_STREQ("A1", C_STR_FIELD(eattr1, "name"));
  EXPECT_STREQ("TA2", C_STR_FIELD(eattr1, "type"));

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* Clean the collections used for the test */
  connection->remove(ENTITIES_COLL, BSONObj());
  connection->remove(REGISTRATIONS_COLL, BSONObj());

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

}

/* ****************************************************************************
*
* NotifyContextAvailability1 -
*/
TEST(mongoRegisterContextRequest, NotifyContextAvailability1)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  /* Prepare mock */
  NotifyContextAvailabilityRequest expectedNcar;
  EntityId mockEn1("E5", "T5", "false");
  ContextRegistrationResponse crr;
  crr.contextRegistration.entityIdVector.push_back(&mockEn1);
  crr.contextRegistration.providingApplication.set("http://dummy.com");  
  expectedNcar.contextRegistrationResponseVector.push_back(&crr);
  expectedNcar.subscriptionId.set("51307b66f481db11bf860001");

  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),"http://notify1.me", XML))
          .Times(1);
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,"http://notify2.me", XML))
          .Times(0);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E5", "T5", "false");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_FALSE(res.registrationId.isEmpty());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* The only collection affected by this operation is registrations, which has been extensively
   * testbed by other unit tests, so we don't include checking in the present unit test */

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

}

/* ****************************************************************************
*
* NotifyContextAvailability2 -
*/
TEST(mongoRegisterContextRequest, NotifyContextAvailability2)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  /* Prepare mock */
  NotifyContextAvailabilityRequest expectedNcar1, expectedNcar2;
  EntityId mockEn1("E5", "T5", "false");
  ContextRegistrationAttribute mockCra("A1", "TA1", "false");
  ContextRegistrationResponse crr;
  crr.contextRegistration.entityIdVector.push_back(&mockEn1);
  crr.contextRegistration.contextRegistrationAttributeVector.push_back(&mockCra);
  crr.contextRegistration.providingApplication.set("http://dummy.com");
  expectedNcar1.contextRegistrationResponseVector.push_back(&crr);
  expectedNcar1.subscriptionId.set("51307b66f481db11bf860001");

  expectedNcar2.contextRegistrationResponseVector.push_back(&crr);
  expectedNcar2.subscriptionId.set("51307b66f481db11bf860002");

  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar1),"http://notify1.me", XML))
          .Times(1);
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar2),"http://notify2.me", XML))
          .Times(1);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E5", "T5", "false");
  ContextRegistrationAttribute cra("A1", "TA1", "false");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.contextRegistrationAttributeVector.push_back(&cra);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_FALSE(res.registrationId.isEmpty());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* The only collection affected by this operation is registrations, which has been extensively
   * testbed by other unit tests, so we don't include checking in the present unit test */

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

}

/* ****************************************************************************
*
* NotifyContextAvailability3 -
*/
TEST(mongoRegisterContextRequest, NotifyContextAvailability3)
{
  HttpStatusCode           ms;
  RegisterContextRequest   req;
  RegisterContextResponse  res;

  /* Prepare mock */
  NotifyContextAvailabilityRequest expectedNcar;
  EntityId mockEn1("E5", "T5", "false");
  ContextRegistrationAttribute mockCra("A2", "TA2", "false");
  ContextRegistrationResponse crr;
  crr.contextRegistration.entityIdVector.push_back(&mockEn1);
  crr.contextRegistration.contextRegistrationAttributeVector.push_back(&mockCra);
  crr.contextRegistration.providingApplication.set("http://dummy.com");
  expectedNcar.contextRegistrationResponseVector.push_back(&crr);
  expectedNcar.subscriptionId.set("51307b66f481db11bf860001");

  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(MatchNcar(&expectedNcar),"http://notify1.me", XML))
          .Times(1);
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,"http://notify2.me", XML))
          .Times(0);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E5", "T5", "false");
  ContextRegistrationAttribute cra("A2", "TA2", "false");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.contextRegistrationAttributeVector.push_back(&cra);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);
  req.duration.set("PT1M");

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT1M", res.duration.get());
  EXPECT_FALSE(res.registrationId.isEmpty());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* The only collection affected by this operation is registrations, which has been extensively
   * testbed by other unit tests, so we don't include checking in the present unit test */

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

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

  /* Prepare mock */
  NotifierMock* notifierMock = new NotifierMock();
  EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
          .Times(0);
  setNotifier(notifierMock);

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
          .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  ContextRegistration cr;
  cr.entityIdVector.push_back(&en);
  cr.providingApplication.set("http://dummy.com");
  req.contextRegistrationVector.push_back(&cr);

  /* Prepare database */
  prepareDatabase();

  /* Invoke the function in mongoBackend library */
  ms = mongoRegisterContext(&req, &res);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientConnection* connection = getMongoConnection();

  /* registrations collection: */
  ASSERT_EQ(1, connection->count(REGISTRATIONS_COLL, BSONObj()));
  BSONObj reg = connection->findOne(REGISTRATIONS_COLL, BSONObj());
  std::string oid = reg.getField("_id").OID().str();
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

  /* entities collection: */
  ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));
  BSONObj ent = connection->findOne(ENTITIES_COLL, BSONObj());

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(0, ent.getField("attrs").Array().size());

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ("PT24H", res.duration.get());
  EXPECT_EQ(oid, res.registrationId.get());
  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  /* Release connection */
  mongoDisconnect();

  /* Delete mock */
  delete timerMock;
  delete notifierMock;

}

/* ****************************************************************************
*
* MongoDbCountFail -
*/
TEST(mongoRegisterContextRequest, MongoDbCountFail)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    /* Set database */
    setupDatabase();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, count(_,_,_,_,_))
            .WillByDefault(Throw(e));

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en);
    cr.providingApplication.set("http://dummy.com");    
    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Set MongoDB connection */
    mongoConnect(connectionMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.duration.isEmpty());
    EXPECT_TRUE(res.registrationId.isEmpty());
    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Database Error", res.errorCode.reasonPhrase);
    EXPECT_EQ("collection: unittest.entities "
              "- count(): { _id.id: \"E1\", _id.type: \"T1\" } "
              "- exception: boom!!", res.errorCode.details);

    /* Release mock */
    delete connectionMock;
    delete timerMock;
    delete notifierMock;

    /* Reconnect to database in not-mocked way */
    mongoConnect("localhost");

    /* check collection has not been touched */
    DBClientConnection* connection = getMongoConnection();
    EXPECT_EQ(0, connection->count(REGISTRATIONS_COLL, BSONObj()));
    EXPECT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

}

/* ****************************************************************************
*
* MongoDbFindOneFail -
*/
TEST(mongoRegisterContextRequest, MongoDbFindOneFail)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;  

    /* Set database */
    setupDatabase();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, count(_,_,_,_,_))
            .WillByDefault(Return(1));
    ON_CALL(*connectionMock, findOne("unittest.entities",_,_,_))
            .WillByDefault(Throw(e));

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en);
    cr.providingApplication.set("http://dummy.com");
    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Set MongoDB connection */
    mongoConnect(connectionMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.duration.isEmpty());
    EXPECT_TRUE(res.registrationId.isEmpty());
    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Database Error", res.errorCode.reasonPhrase);
    EXPECT_EQ("collection: unittest.entities "
              "- findOne(): { _id.id: \"E1\", _id.type: \"T1\" } "
              "- exception: boom!!", res.errorCode.details);

    /* Release mock */
    delete connectionMock;
    delete timerMock;
    delete notifierMock;

    /* Reconnect to database in not-mocked way */
    mongoConnect("localhost");

    /* check collection has not been touched */
    DBClientConnection* connection = getMongoConnection();
    EXPECT_EQ(0, connection->count(REGISTRATIONS_COLL, BSONObj()));
    EXPECT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

}

/* ****************************************************************************
*
* MongoDbUpdateEntityFail -
*/
TEST(mongoRegisterContextRequest, MongoDbUpdateEntityFail)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    /* Set database */
    setupDatabase();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    const DBException e = DBException("boom!!", 33);
    BSONObj fakeEntity = BSON("_id" << BSON("id" << "E1" << "type" << "T1"));
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, count(_,_,_,_,_))
            .WillByDefault(Return(1));
    ON_CALL(*connectionMock, findOne(_,_,_,_))
            .WillByDefault(Return(fakeEntity));
    ON_CALL(*connectionMock, update("unittest.entities",_,_,_,_))
            .WillByDefault(Throw(e));

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    ContextRegistrationAttribute cra("A1", "TA1", "false");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en);
    cr.contextRegistrationAttributeVector.push_back(&cra);
    cr.providingApplication.set("http://dummy.com");
    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Set MongoDB connection */
    mongoConnect(connectionMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.duration.isEmpty());
    EXPECT_TRUE(res.registrationId.isEmpty());
    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Database Error", res.errorCode.reasonPhrase);
    EXPECT_EQ("collection: unittest.entities "
              "- update() query: { _id.id: \"E1\", _id.type: \"T1\" } "
              "- update() doc: { $pushAll: { attrs: [ { name: \"A1\", type: \"TA1\" } ] } } "
              "- exception: boom!!", res.errorCode.details);

    /* Release mock */
    delete connectionMock;
    delete timerMock;
    delete notifierMock;

    /* Reconnect to database in not-mocked way */
    mongoConnect("localhost");

    /* check collection has not been touched */
    DBClientConnection* connection = getMongoConnection();
    EXPECT_EQ(0, connection->count(REGISTRATIONS_COLL, BSONObj()));
    EXPECT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

}

/* ****************************************************************************
*
* MongoDbInsertEntityFail -
*/
TEST(mongoRegisterContextRequest, MongoDbInsertEntityFail)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    /* Set database */
    setupDatabase();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, count(_,_,_,_,_))
            .WillByDefault(Return(0));
    ON_CALL(*connectionMock, insert("unittest.entities",_,_))
            .WillByDefault(Throw(e));

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en);
    cr.providingApplication.set("http://dummy.com");

    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Set MongoDB connection */
    mongoConnect(connectionMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.duration.isEmpty());
    EXPECT_TRUE(res.registrationId.isEmpty());
    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Database Error", res.errorCode.reasonPhrase);
    EXPECT_EQ("collection: unittest.entities "
              "- insert(): { _id: { id: \"E1\", type: \"T1\" }, attrs: {}, creDate: 1360232700 } "
              "- exception: boom!!", res.errorCode.details);

    /* Release mock */
    delete connectionMock;
    delete timerMock;
    delete notifierMock;

    /* Reconnect to database in not-mocked way */
    mongoConnect("localhost");

    /* check collection has not been touched */
    DBClientConnection* connection = getMongoConnection();
    EXPECT_EQ(0, connection->count(REGISTRATIONS_COLL, BSONObj()));
    EXPECT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

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

    /* Set database */
    setupDatabase();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    const DBException e = DBException("boom!!", 33);
    BSONObj fakeEntity = BSON("_id" << BSON("id" << "E1" << "type" << "T1"));
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    DBClientCursorMock* cursorMockCAsub = new DBClientCursorMock(connectionMock, "", 0, 0, 0);
    ON_CALL(*connectionMock, count(_,_,_,_,_))
            .WillByDefault(Return(1));
    ON_CALL(*connectionMock, findOne(_,_,_,_))
            .WillByDefault(Return(fakeEntity));
    ON_CALL(*connectionMock, update("unittest.registrations",_,_,_,_))
            .WillByDefault(Throw(e));    
    ON_CALL(*cursorMockCAsub, more())
            .WillByDefault(Return(false));
    ON_CALL(*connectionMock, _query("unittest.casubs",_,_,_,_,_,_))
            .WillByDefault(Return(cursorMockCAsub));

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en);
    cr.providingApplication.set("http://dummy.com");
    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Set MongoDB connection */
    mongoConnect(connectionMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.duration.isEmpty());
    EXPECT_TRUE(res.registrationId.isEmpty());
    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Database Error", res.errorCode.reasonPhrase);

    /* We split the response into to string, to ease comparison, given that the contect of ObjectId is
     * random. ObjectId is 24 characters long.*/
    /* FIXME: a better approach would be to pass a regex filter to relace ObjectId by a constant string,
     * but I don't know how to work with regex in C++ */
    std::string s1 = res.errorCode.details.substr(0, 71);
    std::string s2 = res.errorCode.details.substr(71+24, res.errorCode.details.size()-71-24);
    EXPECT_EQ("collection: unittest.registrations "
              "- upsert update(): { _id: ObjectId('",s1);
    EXPECT_EQ("'), expiration: 1360232760, contextRegistration: [ { entities: [ { id: \"E1\", type: \"T1\" } ], attrs: {}, providingApplication: \"http://dummy.com\" } ] } "
              "- exception: boom!!", s2);

    /* Release mock */
    delete connectionMock;
    delete timerMock;
    delete notifierMock;

    /* Reconnect to database in not-mocked way */
    mongoConnect("localhost");

    /* check collection has not been touched */
    DBClientConnection* connection = getMongoConnection();
    EXPECT_EQ(0, connection->count(REGISTRATIONS_COLL, BSONObj()));
    EXPECT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

}

/* ****************************************************************************
*
* MongoDbEntitiesInconsistency -
*/
TEST(mongoRegisterContextRequest, MongoDbEntitiesInconsistency)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;    

    /* Set database */
    setupDatabase();

    /* Prepare mock */
    NotifierMock* notifierMock = new NotifierMock();
    EXPECT_CALL(*notifierMock, sendNotifyContextAvailabilityRequest(_,_,_))
            .Times(0);
    setNotifier(notifierMock);

    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, count(_,_,_,_,_))
            .WillByDefault(Return(2));

    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    ContextRegistration cr;
    cr.entityIdVector.push_back(&en);
    cr.providingApplication.set("http://dummy.com");
    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Set MongoDB connection */
    mongoConnect(connectionMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.duration.isEmpty());
    EXPECT_TRUE(res.registrationId.isEmpty());
    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Database Error", res.errorCode.reasonPhrase);
    EXPECT_EQ("collection: unittest.entities "
              "- count(): { _id.id: \"E1\", _id.type: \"T1\" } "
              "- exception: entity unicity violation, the number of entities is: 2", res.errorCode.details);

    /* Release mock */
    delete connectionMock;
    delete timerMock;
    delete notifierMock;

    /* Reconnect to database in not-mocked way */
    mongoConnect("localhost");

    /* check collection has not been touched */
    DBClientConnection* connection = getMongoConnection();
    EXPECT_EQ(0, connection->count(REGISTRATIONS_COLL, BSONObj()));
    EXPECT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

}

/* ****************************************************************************
*
* MongoDbCAsubsFindFail -
*
* FIXME: test disabled by the moment. The same comment done in
* mongoUpdateContext_withOnchangeSubscriptions.MongoDbQueryFail applyes here
*/
TEST(mongoRegisterContextRequest, DISABLED_MongoDbCAsubsFindFail)
{
    //FIXME
    EXPECT_EQ(1, 2);
}

/* ****************************************************************************
*
* AssociationsOk -
*/
TEST(mongoRegisterContextRequest, AssociationsOk)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Forge the request (from "inside" to "outside") */
    ContextRegistration cr;
    AttributeAssociation aa1, aa2;
    aa1.source = "A1";
    aa1.target = "B1";
    aa2.source = "A2";
    aa2.target = "B2";
    Metadata md("assoc1", "Association");;
    md.association.entityAssociation.source = EntityId("E1", "T1", "false");
    md.association.entityAssociation.target = EntityId("E2", "T2", "false");
    md.association.attributeAssociationList.push_back(&aa1);
    md.association.attributeAssociationList.push_back(&aa2);
    cr.registrationMetadataVector.push_back(&md);
    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ("PT1M", res.duration.get());
    EXPECT_FALSE(res.registrationId.isEmpty());
    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    /* Check that associations collection at MongoDB is as expected. We don't check other collections
     * (other tests in this suite cover it extensively) */

    DBClientConnection* connection = getMongoConnection();

    ASSERT_EQ(1, connection->count(ASSOCIATIONS_COLL, BSONObj()));
    BSONObj asoc = connection->findOne(ASSOCIATIONS_COLL, BSONObj());
    EXPECT_STREQ("assoc1", C_STR_FIELD(asoc, "_id"));
    EXPECT_STREQ("E1", C_STR_FIELD(asoc.getObjectField("srcEnt"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(asoc.getObjectField("srcEnt"), "type"));
    EXPECT_STREQ("E2", C_STR_FIELD(asoc.getObjectField("tgtEnt"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(asoc.getObjectField("tgtEnt"), "type"));

    std::vector<BSONElement> attrAssocs = asoc.getField("attrs").Array();
    ASSERT_EQ(2, attrAssocs.size());
    BSONObj attrAssoc0 = attrAssocs[0].embeddedObject();
    BSONObj attrAssoc1 = attrAssocs[1].embeddedObject();
    EXPECT_STREQ("A1", C_STR_FIELD(attrAssoc0, "src"));
    EXPECT_STREQ("B1", C_STR_FIELD(attrAssoc0, "tgt"));
    EXPECT_STREQ("A2", C_STR_FIELD(attrAssoc1, "src"));
    EXPECT_STREQ("B2", C_STR_FIELD(attrAssoc1, "tgt"));

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;

}

/* ****************************************************************************
*
* AssociationsDbFail -
*/
TEST(mongoRegisterContextRequest, AssociationsDbFail)
{
    HttpStatusCode           ms;
    RegisterContextRequest   req;
    RegisterContextResponse  res;

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, insert("unittest.associations",_,_))
            .WillByDefault(Throw(e));

    /* Forge the request (from "inside" to "outside") */
    ContextRegistration cr;
    AttributeAssociation aa1, aa2;
    aa1.source = "A1";
    aa1.target = "B1";
    aa2.source = "A2";
    aa2.target = "B2";
    Metadata md("assoc1", "Association");;
    md.association.entityAssociation.source = EntityId("E1", "T1", "false");
    md.association.entityAssociation.target = EntityId("E2", "T2", "false");
    md.association.attributeAssociationList.push_back(&aa1);
    md.association.attributeAssociationList.push_back(&aa2);
    cr.registrationMetadataVector.push_back(&md);
    req.contextRegistrationVector.push_back(&cr);
    req.duration.set("PT1M");

    /* Prepare database */
    prepareDatabase();

    /* Set MongoDB connection */
    mongoConnect(connectionMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoRegisterContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_TRUE(res.duration.isEmpty());
    EXPECT_TRUE(res.registrationId.isEmpty());
    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Database Error", res.errorCode.reasonPhrase);
    EXPECT_EQ("boom!!", res.errorCode.details);

    /* Release mock */
    delete connectionMock;
    delete timerMock;

    /* Reconnect to database in not-mocked way */
    mongoConnect("localhost");

    /* check collections have not been touched */
    DBClientConnection* connection = getMongoConnection();
    EXPECT_EQ(0, connection->count(ASSOCIATIONS_COLL, BSONObj()));
    EXPECT_EQ(0, connection->count(REGISTRATIONS_COLL, BSONObj()));
}
