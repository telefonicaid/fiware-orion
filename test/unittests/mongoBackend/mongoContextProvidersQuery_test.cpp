/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"

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



extern void setMongoConnectionForUnitTest(DBClientBase* _connection);



/* ****************************************************************************
*
* Tests
*
* These test are based on the ones in the mongoDiscoverContextAvailability_test.cpp file.
* In other words, the test in this file are queryContext operations that involve a
* query on the registration collection equal to the one in the test with the same name
* in mongoDiscoverContextAvailability_test.cpp. Some test in mongoDiscoverContextAvailability_test.cpp
* are not used here (e.g. the ones related with pagination).
*
* With isPattern=false:
*
* - noPatternAttrsAll - discover all the attributes of an entity in the same registration
* - noPatternAttrOneSingle - discover one attribute (single result)
* - noPatternAttrOneMulti -  discover one attribute (multiple result)
* - noPatternAttrsSubset - discover a subset of the attributes of an entity in the same
*   registration
* - noPatternSeveralCREs - discover registration information covering two
*   contextRegistrationElements in the same registration
* - noPatternSeveralRegistrations - discover registration information covering two different
*   registrations
* - noPatternNoEntity - discover with not existing entity
* - noPatternNoAttribute - discover with existing entity but not attribute
* - noPatternMultiEntity - several entities (and no attributes) in the discover
* - noPatternMultiAttr - single entity and several attributes in the discover
* - noPatternMultiEntityAttrs - several entities and attributes in the discover
* - noPatternNoType - discover entities without specifying type
*
* With isPattern=true:
*
* - pattern0Attr
* - pattern1AttrSingle
* - pattern1AttrMulti
* - patternNAttr
* - patternFail
* - patternNoType
* - mixPatternAndNotPattern
*
* Cases involving more than one CPR:
*
* - severalCprs1
* - severalCprs2
* - severalCprs3
*
* Note these tests are not "canonical" unit tests. Canon says that in this case we should have
* mocked MongoDB. Actually, we think is very much powerful to check that everything is ok at
* MongoDB layer.
*
*/

/* ****************************************************************************
*
* prepareDatabase -
*
* This function is called before every test, to populate some information in the
* registrations collection.
*/
static void prepareDatabase(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following registrations:
   *
   * - Reg1: CR: (E1,E2,E3) (A1,A2,A3)
   *         CR: (E1)       (A1,A4)
   * - Reg2: CR: (E2)       (A2, A3)
   * - Reg3: CR: (E1*)      (A1*)
   * - Reg4: CR: (E1**)     (A1)
   *
   * (*) same name but different types. This is included to check that type is taken into account,
   *     so Reg3 is not returned never (except noPatternNoType). You can try to change types in Reg3
   *     to make them equal to the ones in Reg1 and Reg2 and check that some tests are failing.
   * (**)same name but without type
   */

  BSONObj cr1 = BSON("providingApplication" << "http://cr1.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T1") <<
                       BSON("id" << "E2" << "type" << "T2") <<
                       BSON("id" << "E3" << "type" << "T3")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1" << "isDomain" << "true") <<
                       BSON("name" << "A2" << "type" << "TA2" << "isDomain" << "false") <<
                       BSON("name" << "A3" << "type" << "TA3" << "isDomain" << "true")));

  BSONObj cr2 = BSON("providingApplication" << "http://cr2.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T1")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1" << "isDomain" << "true") <<
                       BSON("name" << "A4" << "type" << "TA4" << "isDomain" << "false")));

  BSONObj cr3 = BSON("providingApplication" << "http://cr3.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E2" << "type" << "T2")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "TA2" << "isDomain" << "false") <<
                       BSON("name" << "A3" << "type" << "TA3" << "isDomain" << "true")));

  BSONObj cr4 = BSON("providingApplication" << "http://cr4.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T1bis")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1bis" << "isDomain" << "false")));

  BSONObj cr5 = BSON("providingApplication" << "http://cr5.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1" << "isDomain" << "true")));

  /* 1879048191 corresponds to year 2029 so we avoid any expiration problem in the next 16 years :) */
  BSONObj reg1 = BSON("_id" << OID("51307b66f481db11bf860001") <<
                      "expiration" << 1879048191 <<
                      "contextRegistration" << BSON_ARRAY(cr1 << cr2));

  BSONObj reg2 = BSON("_id" << OID("51307b66f481db11bf860002") <<
                      "expiration" << 1879048191 <<
                      "contextRegistration" << BSON_ARRAY(cr3));

  BSONObj reg3 = BSON("_id" << OID("51307b66f481db11bf860003") <<
                      "expiration" << 1879048191 <<
                      "contextRegistration" << BSON_ARRAY(cr4));

  BSONObj reg4 = BSON("_id" << OID("51307b66f481db11bf860004") <<
                      "expiration" << 1879048191 <<
                      "contextRegistration" << BSON_ARRAY(cr5));

  connection->insert(REGISTRATIONS_COLL, reg1);
  connection->insert(REGISTRATIONS_COLL, reg2);
  connection->insert(REGISTRATIONS_COLL, reg3);
  connection->insert(REGISTRATIONS_COLL, reg4);
}



/* ****************************************************************************
*
* prepareDatabasePatternTrue -
*
* This is a variant of populateDatabase function in which all entities have the same type,
* to ease test for isPattern=true cases
*/
static void prepareDatabasePatternTrue(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following registrations:
   *
   * - Reg1: CR: (E1,E2,E3) (A1,A2,A3)
   *         CR: (E1)       (A1,A4)
   * - Reg2: CR: (E2)       (A2, A3)
   * - Reg3: CR: (E2*)      (A2*)
   * - Reg4: CR: (E3**)     (A2)
   *
   * (*) same name but different types. This is included to check that type is taken into account,
   *     so Reg3 is not returned never (except patternNoType). You can try to change types in Reg3
   *     to make them equal to the ones in Reg1 and Reg2 and check that some tests are failing.
   * (**)same name but without type
   */

  BSONObj cr1 = BSON("providingApplication" << "http://cr1.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T") <<
                       BSON("id" << "E2" << "type" << "T") <<
                       BSON("id" << "E3" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1" << "isDomain" << "true") <<
                       BSON("name" << "A2" << "type" << "TA2" << "isDomain" << "false") <<
                       BSON("name" << "A3" << "type" << "TA3" << "isDomain" << "true")));

  BSONObj cr2 = BSON("providingApplication" << "http://cr2.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1" << "isDomain" << "true") <<
                       BSON("name" << "A4" << "type" << "TA4" << "isDomain" << "false")));

  BSONObj cr3 = BSON("providingApplication" << "http://cr3.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E2" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "TA2" << "isDomain" << "false") <<
                       BSON("name" << "A3" << "type" << "TA3" << "isDomain" << "true")));

  BSONObj cr4 = BSON("providingApplication" << "http://cr4.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E2" << "type" << "Tbis")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "TA2bis" << "isDomain" << "false")));

  BSONObj cr5 = BSON("providingApplication" << "http://cr5.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E3")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "TA2" << "isDomain" << "false")));

  /* 1879048191 corresponds to year 2029 so we avoid any expiration problem in the next 16 years :) */
  BSONObj reg1 = BSON("_id" << "ff37" <<
                      "expiration" << 1879048191 <<
                      "subscriptions" << BSONArray() <<
                      "contextRegistration" << BSON_ARRAY(cr1 << cr2));

  BSONObj reg2 = BSON("_id" << "ff48" <<
                      "expiration" << 1879048191 <<
                      "subscriptions" << BSONArray() <<
                      "contextRegistration" << BSON_ARRAY(cr3));

  BSONObj reg3 = BSON("_id" << "ff80" <<
                      "expiration" << 1879048191 <<
                      "subscriptions" << BSONArray() <<
                      "contextRegistration" << BSON_ARRAY(cr4));

  BSONObj reg4 = BSON("_id" << "ff90" <<
                      "expiration" << 1879048191 <<
                      "subscriptions" << BSONArray() <<
                      "contextRegistration" << BSON_ARRAY(cr5));

  connection->insert(REGISTRATIONS_COLL, reg1);
  connection->insert(REGISTRATIONS_COLL, reg2);
  connection->insert(REGISTRATIONS_COLL, reg3);
  connection->insert(REGISTRATIONS_COLL, reg4);
}



/* ****************************************************************************
*
* prepareDatabaseSeveralCprs1 -
*
*/
static void prepareDatabaseSeveralCprs1(bool addGenericRegistry)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following entities:
   *
   *   E1 - A1 - 1
   *
   * We create the following registries
   *
   * - Reg1: CR: E1 - A2 - CPR1
   * - Reg2: CR: E1 - A3 - CPR2
   * - Reg3: CR: E1 - A4 - CPR1
   * - Reg4: CR: E1 - A5 - CPR2
   * - Reg5: CR: E1 - <null> - CPR3  (if addGenericRegistry = true)
   *
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "T" << "value" << "1")));

  BSONObj cr1 = BSON("providingApplication" << "http://cpr1.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "T" << "isDomain" << "false")));

  BSONObj cr2 = BSON("providingApplication" << "http://cpr2.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A3" << "type" << "T" << "isDomain" << "false")));

  BSONObj cr3 = BSON("providingApplication" << "http://cpr1.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A4" << "type" << "T" << "isDomain" << "false")));

  BSONObj cr4 = BSON("providingApplication" << "http://cpr2.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A5" << "type" << "T" << "isDomain" << "false")));

  BSONObj cr5 = BSON("providingApplication" << "http://cpr3.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T")) /* << "attrs" << BSON_ARRAY*/);

  /* 1879048191 corresponds to year 2029 so we avoid any expiration problem in the next 16 years :) */
  BSONObj reg1 = BSON("_id" << OID("51307b66f481db11bf860001") <<
                      "expiration" << 1879048191 <<
                      "contextRegistration" << BSON_ARRAY(cr1));

  BSONObj reg2 = BSON("_id" << OID("51307b66f481db11bf860002") <<
                      "expiration" << 1879048191 <<
                      "contextRegistration" << BSON_ARRAY(cr2));

  BSONObj reg3 = BSON("_id" << OID("51307b66f481db11bf860003") <<
                      "expiration" << 1879048191 <<
                      "contextRegistration" << BSON_ARRAY(cr3));

  BSONObj reg4 = BSON("_id" << OID("51307b66f481db11bf860004") <<
                      "expiration" << 1879048191 <<
                      "contextRegistration" << BSON_ARRAY(cr4));

  BSONObj reg5 = BSON("_id" << OID("51307b66f481db11bf860005") <<
                      "expiration" << 1879048191 <<
                      "contextRegistration" << BSON_ARRAY(cr5));

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(REGISTRATIONS_COLL, reg1);
  connection->insert(REGISTRATIONS_COLL, reg2);
  connection->insert(REGISTRATIONS_COLL, reg3);
  connection->insert(REGISTRATIONS_COLL, reg4);

  if (addGenericRegistry)
  {
    connection->insert(REGISTRATIONS_COLL, reg5);
  }
}



/* ****************************************************************************
*
* prepareDatabaseSeveralCprs2 -
*
*/
static void prepareDatabaseSeveralCprs2(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following entities:
   *
   *   E1 - A1 - 1
   *
   * We create the following registries
   *
   * - Reg1: CR: E1 - A1 - CPR1
   * - Reg2: CR: E1 - A2 - CPR1
   * - Reg3: CR: E1 - A3 - CPR2
   * - Reg4: CR: E1 - <null> - CPR2
   * - Reg5: CR: E1 - <null> - CPR3
   *
   */


  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "T" << "value" << "1")));

  BSONObj cr1 = BSON("providingApplication" << "http://cpr1.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "T" << "isDomain" << "false")));

  BSONObj cr2 = BSON("providingApplication" << "http://cpr1.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "T" << "isDomain" << "false")));

  BSONObj cr3 = BSON("providingApplication" << "http://cpr2.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A3" << "type" << "T" << "isDomain" << "false")));

  BSONObj cr4 = BSON("providingApplication" << "http://cpr2.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T")) <<
                     "attrs" << BSONArray());

  BSONObj cr5 = BSON("providingApplication" << "http://cpr3.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T")) /* << "attrs" << BSON_ARRAY()*/ );

  /* 1879048191 corresponds to year 2029 so we avoid any expiration problem in the next 16 years :) */
  /*
  BSONObjBuilder reg1 = BSON("_id" << OID("51307b66f481db11bf860001") <<
                               "expiration" << 1879048191 <<
                               "contextRegistration" << BSON_ARRAY(cr1)); */

  BSONObjBuilder reg1;
  reg1.appendElements(BSON("_id" << OID("51307b66f481db11bf860001") <<
                           "expiration" << 1879048191 <<
                           "contextRegistration" << BSON_ARRAY(cr1)));

  BSONObjBuilder reg2;
  reg2.appendElements(BSON("_id" << OID("51307b66f481db11bf860002") <<
                           "expiration" << 1879048191 <<
                           "contextRegistration" << BSON_ARRAY(cr2)));

  BSONObjBuilder reg3;
  reg3.appendElements(BSON("_id" << OID("51307b66f481db11bf860003") <<
                           "expiration" << 1879048191 <<
                           "contextRegistration" << BSON_ARRAY(cr3)));

  BSONObjBuilder reg4;
  reg4.appendElements(BSON("_id" << OID("51307b66f481db11bf860004") <<
                           "expiration" << 1879048191 <<
                           "contextRegistration" << BSON_ARRAY(cr4)));

  BSONObjBuilder reg5;
  reg5.appendElements(BSON("_id" << OID("51307b66f481db11bf860005") <<
                           "expiration" << 1879048191 <<
                           "contextRegistration" << BSON_ARRAY(cr5)));

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(REGISTRATIONS_COLL, reg1.obj());
  connection->insert(REGISTRATIONS_COLL, reg2.obj());
  connection->insert(REGISTRATIONS_COLL, reg3.obj());
  connection->insert(REGISTRATIONS_COLL, reg4.obj());
  connection->insert(REGISTRATIONS_COLL, reg5.obj());
}



/* ****************************************************************************
*
* noPatternAttrsAll -
*
* Query:  E3 - <null>
* Result: E3 - (A1, A2, A3) - http://cr1.com
*/
TEST(mongoContextProvidersQueryRequest, noPatternAttrsAll)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E3", "T3");
  req.entityIdVector.push_back(&en);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E3", RES_CER(0).entityId.id);
  EXPECT_EQ("T3", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(3, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 2)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 2)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 2)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}


/* ****************************************************************************
*
* noPatternAttrOneSingle -
*
* Query:  E1 - A4
* Result: E1 - A4 - http://cr2.com
*/
TEST(mongoContextProvidersQueryRequest, noPatternAttrOneSingle)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  req.entityIdVector.push_back(&en);
  req.attributeList.push_back("A4");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A4", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr2.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* noPatternAttrOneMulti -
*
* Query:  E1 - A1
* Result: E1 - A1 - http://cr1.com
*         E1 - A1 - http://cr2.com
*
* In this case, htpt://cr2.com also matches, but current implementation of mongoBackend
* returns only one of them (which one is not specified).
*
* This test also checks that querying for type (E1) doesn't match with no-typed
* entities (E1** - cr5 is not returned)
*/
TEST(mongoContextProvidersQueryRequest, noPatternAttrOneMulti)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  req.entityIdVector.push_back(&en);
  req.attributeList.push_back("A1");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}


/* ****************************************************************************
*
* noPatternAttrsSubset -
*
* Query:  E3 - (A1, A2)
* Result: E3 - (A1, A2) - http://cr1.com
*/
TEST(mongoContextProvidersQueryRequest, noPatternAttrsSubset)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E3", "T3");
  req.entityIdVector.push_back(&en);
  req.attributeList.push_back("A1");
  req.attributeList.push_back("A2");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E3", RES_CER(0).entityId.id);
  EXPECT_EQ("T3", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* noPatternSeveralCREs -
*
* Query:  E1 - no attrs
* Result: E1 - A1, A2, A3 - http://cr1.com
*              A4         - http://cr2.com
*
* For A1 two CPrs overlap and mongoBacken takes http://cr1.com
*
*/
TEST(mongoContextProvidersQueryRequest, noPatternSeveralCREs)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  req.entityIdVector.push_back(&en);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(4, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 2)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 2)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 2)->providingApplication.getMimeType());

  EXPECT_EQ("A4", RES_CER_ATTR(0, 3)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 3)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 3)->stringValue);
  EXPECT_EQ("http://cr2.com",   RES_CER_ATTR(0, 3)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 3)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* noPatternSeveralRegistrations -
*
* Query:  E2 - no attrs
* Result: E2 - (A1, A2, A3) - http://cr1.com
*         E2 - (A2, A3)     - http://cr3.com
*
* For A2 and A3 is there overlap with http:;//cr3.com, but mongoBackend sets http://cr1.com
*
*/
TEST(mongoContextProvidersQueryRequest, noPatternSeveralRegistrations)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E2", "T2");
  req.entityIdVector.push_back(&en);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E2", RES_CER(0).entityId.id);
  EXPECT_EQ("T2", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(3, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 2)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 2)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 2)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* noPatternNoEntity -
*
* Query:  E4 - no attrs
* Result: none
*/
TEST(mongoContextProvidersQueryRequest, noPatternNoEntity)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E4", "T4");
  req.entityIdVector.push_back(&en);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
  EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(0, res.contextElementResponseVector.size());

  utExit();
}


/* ****************************************************************************
*
* noPatternNoAttribute -
*
* Query:  E1 - A5
* Result: none
*/
TEST(mongoContextProvidersQueryRequest, noPatternNoAttribute)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");

  req.entityIdVector.push_back(&en);
  req.attributeList.push_back("A5");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
  EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(0, res.contextElementResponseVector.size());

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* noPatternMultiEntity -
*
* Query:  (E1, E2) - no attrs
* Result: E1 - A1, A2, A3 - http://cr1.com
*              A4         - http://cr2.com
*         E2 - A1, A2, A3 - http://cr1.com
*
* Overlaps:
*    E1-A1: cr1 and cr2
*    E2-A2: cr1 and cr3
*    E2-A3: cr1 and cr3
*/
TEST(mongoContextProvidersQueryRequest, noPatternMultiEntity)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en1("E1", "T1");
  EntityId en2("E2", "T2");
  req.entityIdVector.push_back(&en1);
  req.entityIdVector.push_back(&en2);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(4, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 2)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 2)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 2)->providingApplication.getMimeType());

  EXPECT_EQ("A4", RES_CER_ATTR(0, 3)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 3)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 3)->stringValue);
  EXPECT_EQ("http://cr2.com",   RES_CER_ATTR(0, 3)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 3)->providingApplication.getMimeType());

  /* Context Element response # 1 */
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  EXPECT_EQ("E2", RES_CER(1).entityId.id);
  EXPECT_EQ("T2", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(1).providingApplicationList.size());
  ASSERT_EQ(3, RES_CER(1).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(1, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(1, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(1, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 1)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ("A3", RES_CER_ATTR(1, 2)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 2)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 2)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(1, 2)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(1, 2)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* noPatternMultiAttr -
*
* Query:  E1 - (A3, A4, A5)
* Result: E1 - A3 - http://cr1.com
*              A4 - http://cr2.com
*/
TEST(mongoContextProvidersQueryRequest, noPatternMultiAttr)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T1");
  req.entityIdVector.push_back(&en);
  req.attributeList.push_back("A3");
  req.attributeList.push_back("A4");
  req.attributeList.push_back("A5");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A4", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->stringValue);
  EXPECT_EQ("http://cr2.com",   RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* noPatternMultiEntityAttrs -
*
* Query:  (E1, E2) - (A3, A4, A5)
* Result: E1 - A3 - http://cr1.com
*              A4 - http://cr2.com
*         E2 - A3 - http://cr1.com
*
* E2-A3 is also provided by http://cr3.com, but mongoBackend choses cr1
*
*/
TEST(mongoContextProvidersQueryRequest, noPatternMultiEntityAttrs)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en1("E1", "T1");
  EntityId en2("E2", "T2");
  req.entityIdVector.push_back(&en1);
  req.entityIdVector.push_back(&en2);
  req.attributeList.push_back("A3");
  req.attributeList.push_back("A4");
  req.attributeList.push_back("A5");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A4", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->stringValue);
  EXPECT_EQ("http://cr2.com",   RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E2", RES_CER(1).entityId.id);
  EXPECT_EQ("T2", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(1).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());

  EXPECT_EQ("A3", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(1, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(1, 0)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* noPatternNoType -
*
* Query:  E1** - A1
* Result: E1   - A1 - http://cr1.com
*         E1*  - A1*- http://cr4.com
*         E1** - A1 - http://cr5.com
*
* Note that this case checks matching of no-type in the query for both the case in
* which the returned CR has type (cr1, cr4) and the case in which it has no type (cr5).
*
* Note also that for E1-A1 there are two possibilities in the registrations database (cr1 and cr2)
* and mongoBackend choses cr1.
*
*/
TEST(mongoContextProvidersQueryRequest, noPatternNoType)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "", "false");
  req.entityIdVector.push_back(&en);
  req.attributeList.push_back("A1");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(3, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr5.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E1", RES_CER(1).entityId.id);
  EXPECT_EQ("T1", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(1).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(1, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(1, 0)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Context Element response # 3 */
  EXPECT_EQ("E1", RES_CER(2).entityId.id);
  EXPECT_EQ("T1bis", RES_CER(2).entityId.type);
  EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(2).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(2, 0)->stringValue);
  EXPECT_EQ("http://cr4.com", RES_CER_ATTR(2, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(2, 0)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* pattern0Attr -
*
* Query:  E[2-3] - none
* Result: E2 - (A1, A2, A3) - http://cr1.com
*         E2 - (A1, A2, A3) - http://cr1.com
*
* There is overlap in some CPrs (e.g. E1-A1,A4-cr2) but monboBacken only returns one.
*
* This test also checks that querying for type (E[2-3]) doesn't match with no-typed
* entities (E3** - cr5 is not returned)
*
*/
TEST(mongoContextProvidersQueryRequest, pattern0Attr)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E[2-3]", "T", "true");
  req.entityIdVector.push_back(&en);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E2", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(3, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 2)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 2)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 2)->providingApplication.getMimeType());

  /* Context Element response # 2 */
  EXPECT_EQ("E3", RES_CER(1).entityId.id);
  EXPECT_EQ("T", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(1).providingApplicationList.size());
  ASSERT_EQ(3, RES_CER(1).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(1, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(1, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(1, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 1)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(1, 1)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(1, 1)->providingApplication.getMimeType());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 2)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(1, 2)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(1, 2)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* pattern1AttrSingle -
*
* Query:  E[1-3] - A4
* Result: E1 - A4 - http://cr2.com
*/
TEST(mongoContextProvidersQueryRequest, pattern1AttrSingle)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E[1-3]", "T", "true");
  req.entityIdVector.push_back(&en);
  req.attributeList.push_back("A4");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A4", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr2.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* pattern1AttrMulti -
*
* Query:  E[1-2] - A1
* Result: E1     - A1 - http://cr1.com
*         E2     - A2 - http://cr1.com
*
* There is overlap for E1-A1 (cr1 and cr2) than mongoBackend resolves to cr1
*
*/
TEST(mongoContextProvidersQueryRequest, pattern1AttrMulti)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E[1-2]", "T", "true");
  req.entityIdVector.push_back(&en);
  req.attributeList.push_back("A1");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E2", RES_CER(1).entityId.id);
  EXPECT_EQ("T", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(1).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(1, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(1, 0)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* patternNAttr -
*
* Query:  E[1-2] - (A1, A2)
* Result: E1     - (A1, A2) - http://cr1.com
*         E2     - (A1, A2) - http://cr1.com
*
* There is overlap in some CPrs (e.g. E2-A2-cr3) but monboBacken only returns one.
*
*/
TEST(mongoContextProvidersQueryRequest, patternNAttr)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E[1-2]", "T", "true");
  req.entityIdVector.push_back(&en);
  req.attributeList.push_back("A1");
  req.attributeList.push_back("A2");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E2", RES_CER(1).entityId.id);
  EXPECT_EQ("T", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(1).providingApplicationList.size());
  ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(1, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(1, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(1, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 1)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(1, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(1, 1)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* patternFail -
*
* Query:  R.* - none
* Result: none
*/
TEST(mongoContextProvidersQueryRequest, patternFail)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("R.*", "T", "true");
  req.entityIdVector.push_back(&en);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
  EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(0, res.contextElementResponseVector.size());

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* patternNoType -
*
* Query:  E[2-3]** - A2
* Result: E2       - A2  - http://cr1.com
*         E3       - A2  - http://cr1.com
*         E2*      - A2* - http://cr4.com
*         E3**     - A2  - http://cr5.com
*
* Overlaps: E2-A2-cr3
*
*/
TEST(mongoContextProvidersQueryRequest, patternNoType)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E[2-3]", "", "true");
  req.entityIdVector.push_back(&en);
  req.attributeList.push_back("A2");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(4, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E2", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E3", RES_CER(1).entityId.id);
  EXPECT_EQ("T", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(1).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());

  EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(1, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(1, 0)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Context Element response # 3 */
  EXPECT_EQ("E2", RES_CER(2).entityId.id);
  EXPECT_EQ("Tbis", RES_CER(2).entityId.type);
  EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(2).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());

  EXPECT_EQ("A2", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(2, 0)->stringValue);
  EXPECT_EQ("http://cr4.com", RES_CER_ATTR(2, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(2, 0)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

  /* Context Element response # 4 */
  EXPECT_EQ("E3", RES_CER(3).entityId.id);
  EXPECT_EQ("", RES_CER(3).entityId.type);
  EXPECT_EQ("false", RES_CER(3).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(3).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(3).contextAttributeVector.size());

  EXPECT_EQ("A2", RES_CER_ATTR(3, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(3, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(3, 0)->stringValue);
  EXPECT_EQ("http://cr5.com", RES_CER_ATTR(3, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(3, 0)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
  EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}

/* ****************************************************************************
*
* mixPatternAndNotPattern -
*
* Query:  (E[2-3]. E1) - none
* Result: E1           - (A1, A2, A3) - http://cr1.com
*                         A4          - http://cr2.com
*         E2           - (A1 ,A2, A3) - http://cr1.com
*         E3           - (A1, A2, A3) - http://cr1.com
*
* Overlaps: E1           - A1 - http://cr2.com
*           E2           - (A2, A3) - http://cr3.com
*/
TEST(mongoContextProvidersQueryRequest, mixPatternAndNotPattern)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  EntityId en1("E[2-3]", "T", "true");
  EntityId en2("E1", "T");
  req.entityIdVector.push_back(&en1);
  req.entityIdVector.push_back(&en2);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(3, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(4, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 2)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(0, 2)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 2)->providingApplication.getMimeType());

  EXPECT_EQ("A4", RES_CER_ATTR(0, 3)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 3)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 3)->stringValue);
  EXPECT_EQ("http://cr2.com",   RES_CER_ATTR(0, 3)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 3)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E2", RES_CER(1).entityId.id);
  EXPECT_EQ("T", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(1).providingApplicationList.size());
  ASSERT_EQ(3, RES_CER(1).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(1, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(1, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(1, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 1)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(1, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(1, 1)->providingApplication.getMimeType());

  EXPECT_EQ("A3", RES_CER_ATTR(1, 2)->name);
  EXPECT_EQ("", RES_CER_ATTR(1, 2)->type);
  EXPECT_EQ("", RES_CER_ATTR(1, 2)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(1, 2)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(1, 2)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Context Element response # 3 */
  EXPECT_EQ("E3", RES_CER(2).entityId.id);
  EXPECT_EQ("T", RES_CER(2).entityId.type);
  EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(2).providingApplicationList.size());
  ASSERT_EQ(3, RES_CER(2).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("", RES_CER_ATTR(2, 0)->stringValue);
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(2, 0)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(2, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(2, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(2, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(2, 1)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(2, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(2, 1)->providingApplication.getMimeType());

  EXPECT_EQ("A3", RES_CER_ATTR(2, 2)->name);
  EXPECT_EQ("", RES_CER_ATTR(2, 2)->type);
  EXPECT_EQ("", RES_CER_ATTR(2, 2)->stringValue);
  EXPECT_EQ("http://cr1.com",   RES_CER_ATTR(2, 2)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(2, 2)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

  /* Check entities collection hasn't been touched */
  DBClientBase* connection = getMongoConnection();
  ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

  utExit();
}


/* ****************************************************************************
*
* severalCprs1 -
*
* Query:  E1 - (A1, A2, A3, A4, A5, A6)
* Result: E1 - A1 - 1
*              A2 - fwd CPR1
*              A3 - fwd CPR2
*              A4 - fwd CPR1
*              A5 - fwd CPR2
*              A6 - fwd CPR3
*
*         CPR Vector: [ ]
*
*/
TEST(mongoContextProvidersQueryRequest, severalCprs1)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabaseSeveralCprs1(true);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T", "false");
  req.entityIdVector.push_back(&en);
  req.attributeList.push_back("A1");
  req.attributeList.push_back("A2");
  req.attributeList.push_back("A3");
  req.attributeList.push_back("A4");
  req.attributeList.push_back("A5");
  req.attributeList.push_back("A6");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(6, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("T", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("1", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(NOMIMETYPE, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->stringValue);
  EXPECT_EQ("http://cpr1.com",   RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 2)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->stringValue);
  EXPECT_EQ("http://cpr2.com",   RES_CER_ATTR(0, 2)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 2)->providingApplication.getMimeType());

  EXPECT_EQ("A4", RES_CER_ATTR(0, 3)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 3)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 3)->stringValue);
  EXPECT_EQ("http://cpr1.com",   RES_CER_ATTR(0, 3)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 3)->providingApplication.getMimeType());

  EXPECT_EQ("A5", RES_CER_ATTR(0, 4)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 4)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 4)->stringValue);
  EXPECT_EQ("http://cpr2.com", RES_CER_ATTR(0, 4)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 4)->providingApplication.getMimeType());

  EXPECT_EQ("A6", RES_CER_ATTR(0, 5)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 5)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 5)->stringValue);
  EXPECT_EQ("http://cpr3.com", RES_CER_ATTR(0, 5)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 5)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}

/* ****************************************************************************
*
* severalCprs2 -
*
* Query:  E1 - (A1, A2, A3, A4, A5, A6)
* Result: E1 - A1 - 1
*              A2 - fwd CPR1
*              A3 - fwd CPR2
*              A4 - fwd CPR1
*              A5 - fwd CPR2
*              A6 - Not found (actually, don't returned by mongoBackend)
*
*         CPR Vector: [ ]
*
*/
TEST(mongoContextProvidersQueryRequest, severalCprs2)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabaseSeveralCprs1(false);

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T", "false");
  req.entityIdVector.push_back(&en);
  req.attributeList.push_back("A1");
  req.attributeList.push_back("A2");
  req.attributeList.push_back("A3");
  req.attributeList.push_back("A4");
  req.attributeList.push_back("A5");
  req.attributeList.push_back("A6");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(5, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("T", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("1", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(NOMIMETYPE, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->stringValue);
  EXPECT_EQ("http://cpr1.com",   RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 2)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->stringValue);
  EXPECT_EQ("http://cpr2.com",   RES_CER_ATTR(0, 2)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 2)->providingApplication.getMimeType());

  EXPECT_EQ("A4", RES_CER_ATTR(0, 3)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 3)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 3)->stringValue);
  EXPECT_EQ("http://cpr1.com",   RES_CER_ATTR(0, 3)->providingApplication.get());
  EXPECT_EQ(JSON,   RES_CER_ATTR(0, 3)->providingApplication.getMimeType());

  EXPECT_EQ("A5", RES_CER_ATTR(0, 4)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 4)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 4)->stringValue);
  EXPECT_EQ("http://cpr2.com", RES_CER_ATTR(0, 4)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 4)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}

/* ****************************************************************************
*
* severalCprs3 -
*
* Query:  E1 - <null>
* Result: E1 - A1 - 1
*              A2 - fwd CPR1
*              A3 - fwd CPR2
*
*         CPR vector: [CPR2, CPR3]
*/
TEST(mongoContextProvidersQueryRequest, severalCprs3)
{
  HttpStatusCode        ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabaseSeveralCprs2();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1", "T", "false");
  req.entityIdVector.push_back(&en);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(2, RES_CER(0).providingApplicationList.size());
  EXPECT_EQ("http://cpr2.com", RES_CER(0).providingApplicationList[0].get());
  EXPECT_EQ(JSON, RES_CER(0).providingApplicationList[0].getMimeType());
  EXPECT_EQ("http://cpr3.com", RES_CER(0).providingApplicationList[1].get());
  EXPECT_EQ(JSON, RES_CER(0).providingApplicationList[1].getMimeType());

  ASSERT_EQ(3, RES_CER(0).contextAttributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("T", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("1", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_EQ(NOMIMETYPE, RES_CER_ATTR(0, 0)->providingApplication.getMimeType());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 1)->stringValue);
  EXPECT_EQ("http://cpr1.com", RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 1)->providingApplication.getMimeType());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 2)->name);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->type);
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->stringValue);
  EXPECT_EQ("http://cpr2.com", RES_CER_ATTR(0, 2)->providingApplication.get());
  EXPECT_EQ(JSON, RES_CER_ATTR(0, 2)->providingApplication.getMimeType());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  utExit();
}

