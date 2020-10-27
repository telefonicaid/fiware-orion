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
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoConnectionPool.h"
#include "mongoBackend/mongoDiscoverContextAvailability.h"
#include "ngsi/StatusCode.h"
#include "ngsi/EntityId.h"
#include "ngsi/Scope.h"
#include "ngsi9/DiscoverContextAvailabilityRequest.h"
#include "ngsi9/DiscoverContextAvailabilityResponse.h"

#include "unittests/unittest.h"



/* ****************************************************************************
*
* USING
*/
using mongo::DBClientBase;
using mongo::BSONObj;
using mongo::BSONArray;
using mongo::OID;
using mongo::DBException;
using ::testing::Return;
using ::testing::Throw;
using ::testing::_;



extern void setMongoConnectionForUnitTest(DBClientBase* _connection);



/* ****************************************************************************
*
* Tests
*
* With pagination:
*
* - paginationDetails
* - paginationAll
* - paginationOnlyFirst
* - paginationOnlySecond
* - paginationRange
* - paginationNonExisting
* - paginationNonExistingOverlap
* - paginationNonExistingDetails
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
* Simulating fails in MongoDB connection:
*
* - mongoDbQueryFail
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
                       BSON("id" << "E1" << "type" << "T1")  <<
                       BSON("id" << "E2" << "type" << "T2")  <<
                       BSON("id" << "E3" << "type" << "T3")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1") <<
                       BSON("name" << "A2" << "type" << "TA2") <<
                       BSON("name" << "A3" << "type" << "TA3")));

  BSONObj cr2 = BSON("providingApplication" << "http://cr2.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T1")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1") <<
                       BSON("name" << "A4" << "type" << "TA4")));

  BSONObj cr3 = BSON("providingApplication" << "http://cr3.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E2" << "type" << "T2")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "TA2") <<
                       BSON("name" << "A3" << "type" << "TA3")));

  BSONObj cr4 = BSON("providingApplication" << "http://cr4.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T1bis")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1bis")));

  BSONObj cr5 = BSON("providingApplication" << "http://cr5.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1")));

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
                       BSON("name" << "A1" << "type" << "TA1") <<
                       BSON("name" << "A2" << "type" << "TA2") <<
                       BSON("name" << "A3" << "type" << "TA3")));

  BSONObj cr2 = BSON("providingApplication" << "http://cr2.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A1" << "type" << "TA1") <<
                       BSON("name" << "A4" << "type" << "TA4")));

  BSONObj cr3 = BSON("providingApplication" << "http://cr3.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E2" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "TA2") <<
                       BSON("name" << "A3" << "type" << "TA3")));

  BSONObj cr4 = BSON("providingApplication" << "http://cr4.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E2" << "type" << "Tbis")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "TA2bis")));

  BSONObj cr5 = BSON("providingApplication" << "http://cr5.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E3")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "TA2")));

  /* 1879048191 corresponds to year 2029 so we avoid any expiration problem in the next 16 years :) */
  BSONObj reg1 = BSON(
              "_id" << OID("51307b66f481db11bf86ff37") <<
              "expiration" << 1879048191 <<
              "subscriptions" << BSONArray() <<
              "contextRegistration" << BSON_ARRAY(cr1 << cr2));

  BSONObj reg2 = BSON(
              "_id" << OID("51307b66f481db11bf86ff48") <<
              "expiration" << 1879048191 <<
              "subscriptions" << BSONArray() <<
              "contextRegistration" << BSON_ARRAY(cr3));

  BSONObj reg3 = BSON(
              "_id" << OID("51307b66f481db11bf86ff80") <<
              "expiration" << 1879048191 <<
              "subscriptions" << BSONArray() <<
              "contextRegistration" << BSON_ARRAY(cr4));

  BSONObj reg4 = BSON(
              "_id" << OID("51307b66f481db11bf86ff90") <<
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
* prepareDatabaseForPagination -
*/
static void prepareDatabaseForPagination(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following registrations:
   *
   * - Reg1: CR: (E1 for http://cr1.com)
   * - Reg2: CR: (E2 for http://cr2.com)
   * - Reg3: CR: (E3 for http://cr3.com)
   * - Reg4: CR: (E4 for http://cr4.com)
   * - Reg5: CR: (E5 for http://cr5.com)
   *
   */

  BSONObj cr1 = BSON("providingApplication" << "http://cr1.com" <<
                     "entities" << BSON_ARRAY(BSON("id" << "E1" << "type" << "T1")) <<
                     "attrs" << BSON_ARRAY(BSON("name" << "A1" << "type" << "TA1")));

  BSONObj cr2 = BSON("providingApplication" << "http://cr2.com" <<
                     "entities" << BSON_ARRAY(BSON("id" << "E2" << "type" << "T2")) <<
                     "attrs" << BSON_ARRAY(BSON("name" << "A1" << "type" << "TA1")));

  BSONObj cr3 = BSON("providingApplication" << "http://cr3.com" <<
                     "entities" << BSON_ARRAY(BSON("id" << "E3" << "type" << "T3")) <<
                     "attrs" << BSON_ARRAY(BSON("name" << "A1" << "type" << "TA1")));

  BSONObj cr4 = BSON("providingApplication" << "http://cr4.com" <<
                     "entities" << BSON_ARRAY(BSON("id" << "E4" << "type" << "T4")) <<
                     "attrs" << BSON_ARRAY(BSON("name" << "A1" << "type" << "TA1")));

  BSONObj cr5 = BSON("providingApplication" << "http://cr5.com" <<
                     "entities" << BSON_ARRAY(BSON("id" << "E5" << "type" << "T5")) <<
                     "attrs" << BSON_ARRAY(BSON("name" << "A1" << "type" << "TA1")));

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

  connection->insert(REGISTRATIONS_COLL, reg1);
  connection->insert(REGISTRATIONS_COLL, reg2);
  connection->insert(REGISTRATIONS_COLL, reg3);
  connection->insert(REGISTRATIONS_COLL, reg4);
  connection->insert(REGISTRATIONS_COLL, reg5);
}

/* ****************************************************************************
*
* paginationDetails -
*
*/
TEST(mongoDiscoverContextAvailabilityRequest, paginationDetails)
{
  HttpStatusCode                       ms;
  DiscoverContextAvailabilityRequest   req;
  DiscoverContextAvailabilityResponse  res;

  utInit();
  prepareDatabaseForPagination();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "", "true");
  req.entityIdVector.push_back(&en);
  uriParams[URI_PARAM_PAGINATION_DETAILS]  = "on";

  /* Invoke the function in mongoBackend library */
  ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ("Count: 5", res.errorCode.details);

  ASSERT_EQ(5, res.responseVector.size());
  /* Context registration element #1 */
  ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
  EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
  EXPECT_EQ("T1", RES_CNTX_REG(0).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
  EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

  /* Context registration element #2 */
  ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
  EXPECT_EQ("E2", RES_CNTX_REG(1).entityIdVector[0]->id);
  EXPECT_EQ("T2", RES_CNTX_REG(1).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(1, 0)->type);
  EXPECT_EQ("http://cr2.com", RES_CNTX_REG(1).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());

  /* Context registration element #3 */
  ASSERT_EQ(1, RES_CNTX_REG(2).entityIdVector.size());
  EXPECT_EQ("E3", RES_CNTX_REG(2).entityIdVector[0]->id);
  EXPECT_EQ("T3", RES_CNTX_REG(2).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(2).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(2).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(2, 0)->type);
  EXPECT_EQ("http://cr3.com", RES_CNTX_REG(2).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[2]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[2]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[2]->errorCode.details.size());

  /* Context registration element #4 */
  ASSERT_EQ(1, RES_CNTX_REG(3).entityIdVector.size());
  EXPECT_EQ("E4", RES_CNTX_REG(3).entityIdVector[0]->id);
  EXPECT_EQ("T4", RES_CNTX_REG(3).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(3).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(3).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(3, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(3, 0)->type);
  EXPECT_EQ("http://cr4.com", RES_CNTX_REG(3).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[3]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[3]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[3]->errorCode.details.size());

  /* Context registration element #5 */
  ASSERT_EQ(1, RES_CNTX_REG(4).entityIdVector.size());
  EXPECT_EQ("E5", RES_CNTX_REG(4).entityIdVector[0]->id);
  EXPECT_EQ("T5", RES_CNTX_REG(4).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(4).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(4).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(4, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(4, 0)->type);
  EXPECT_EQ("http://cr5.com", RES_CNTX_REG(4).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[4]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[4]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[4]->errorCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* paginationAll -
*
*/
TEST(mongoDiscoverContextAvailabilityRequest, paginationAll)
{
  HttpStatusCode                       ms;
  DiscoverContextAvailabilityRequest   req;
  DiscoverContextAvailabilityResponse  res;

  utInit();
  prepareDatabaseForPagination();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "", "true");
  req.entityIdVector.push_back(&en);
  uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";

  /* Invoke the function in mongoBackend library */
  ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(5, res.responseVector.size());
  /* Context registration element #1 */
  ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
  EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
  EXPECT_EQ("T1", RES_CNTX_REG(0).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
  EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

  /* Context registration element #2 */
  ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
  EXPECT_EQ("E2", RES_CNTX_REG(1).entityIdVector[0]->id);
  EXPECT_EQ("T2", RES_CNTX_REG(1).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(1, 0)->type);
  EXPECT_EQ("http://cr2.com", RES_CNTX_REG(1).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());

  /* Context registration element #3 */
  ASSERT_EQ(1, RES_CNTX_REG(2).entityIdVector.size());
  EXPECT_EQ("E3", RES_CNTX_REG(2).entityIdVector[0]->id);
  EXPECT_EQ("T3", RES_CNTX_REG(2).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(2).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(2).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(2, 0)->type);
  EXPECT_EQ("http://cr3.com", RES_CNTX_REG(2).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[2]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[2]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[2]->errorCode.details.size());

  /* Context registration element #4 */
  ASSERT_EQ(1, RES_CNTX_REG(3).entityIdVector.size());
  EXPECT_EQ("E4", RES_CNTX_REG(3).entityIdVector[0]->id);
  EXPECT_EQ("T4", RES_CNTX_REG(3).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(3).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(3).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(3, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(3, 0)->type);
  EXPECT_EQ("http://cr4.com", RES_CNTX_REG(3).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[3]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[3]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[3]->errorCode.details.size());

  /* Context registration element #5 */
  ASSERT_EQ(1, RES_CNTX_REG(4).entityIdVector.size());
  EXPECT_EQ("E5", RES_CNTX_REG(4).entityIdVector[0]->id);
  EXPECT_EQ("T5", RES_CNTX_REG(4).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(4).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(4).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(4, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(4, 0)->type);
  EXPECT_EQ("http://cr5.com", RES_CNTX_REG(4).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[4]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[4]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[4]->errorCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* paginationOnlyFirst -
*
*/
TEST(mongoDiscoverContextAvailabilityRequest, paginationOnlyFirst)
{
  HttpStatusCode                       ms;
  DiscoverContextAvailabilityRequest   req;
  DiscoverContextAvailabilityResponse  res;

  utInit();
  prepareDatabaseForPagination();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "", "true");
  req.entityIdVector.push_back(&en);
  uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
  uriParams[URI_PARAM_PAGINATION_OFFSET] = "0";
  uriParams[URI_PARAM_PAGINATION_LIMIT]  = "1";

  /* Invoke the function in mongoBackend library */
  ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.responseVector.size());
  /* Context registration element #1 */
  ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
  EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
  EXPECT_EQ("T1", RES_CNTX_REG(0).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
  EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* paginationOnlySecond -
*
*/
TEST(mongoDiscoverContextAvailabilityRequest, paginationOnlySecond)
{
  HttpStatusCode                       ms;
  DiscoverContextAvailabilityRequest   req;
  DiscoverContextAvailabilityResponse  res;

  utInit();
  prepareDatabaseForPagination();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "", "true");
  req.entityIdVector.push_back(&en);
  uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
  uriParams[URI_PARAM_PAGINATION_OFFSET] = "1";
  uriParams[URI_PARAM_PAGINATION_LIMIT]  = "1";

  /* Invoke the function in mongoBackend library */
  ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.responseVector.size());
  /* Context registration element #1 */
  ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
  EXPECT_EQ("E2", RES_CNTX_REG(0).entityIdVector[0]->id);
  EXPECT_EQ("T2", RES_CNTX_REG(0).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
  EXPECT_EQ("http://cr2.com", RES_CNTX_REG(0).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* paginationRange -
*
*/
TEST(mongoDiscoverContextAvailabilityRequest, paginationRange)
{
  HttpStatusCode                       ms;
  DiscoverContextAvailabilityRequest   req;
  DiscoverContextAvailabilityResponse  res;

  utInit();
  prepareDatabaseForPagination();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "", "true");
  req.entityIdVector.push_back(&en);
  uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
  uriParams[URI_PARAM_PAGINATION_OFFSET] = "2";
  uriParams[URI_PARAM_PAGINATION_LIMIT]  = "3";

  /* Invoke the function in mongoBackend library */
  ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(3, res.responseVector.size());
  /* Context registration element #1 */
  ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
  EXPECT_EQ("E3", RES_CNTX_REG(0).entityIdVector[0]->id);
  EXPECT_EQ("T3", RES_CNTX_REG(0).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
  EXPECT_EQ("http://cr3.com", RES_CNTX_REG(0).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

  /* Context registration element #2 */
  ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
  EXPECT_EQ("E4", RES_CNTX_REG(1).entityIdVector[0]->id);
  EXPECT_EQ("T4", RES_CNTX_REG(1).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(1, 0)->type);
  EXPECT_EQ("http://cr4.com", RES_CNTX_REG(1).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());

  /* Context registration element #3 */
  ASSERT_EQ(1, RES_CNTX_REG(2).entityIdVector.size());
  EXPECT_EQ("E5", RES_CNTX_REG(2).entityIdVector[0]->id);
  EXPECT_EQ("T5", RES_CNTX_REG(2).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(2).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(2).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(2, 0)->type);
  EXPECT_EQ("http://cr5.com", RES_CNTX_REG(2).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[2]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[2]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[2]->errorCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* paginationNonExisting -
*
*/
TEST(mongoDiscoverContextAvailabilityRequest, paginationNonExisting)
{
  HttpStatusCode                       ms;
  DiscoverContextAvailabilityRequest   req;
  DiscoverContextAvailabilityResponse  res;

  utInit();
  prepareDatabaseForPagination();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "", "true");
  req.entityIdVector.push_back(&en);
  uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
  uriParams[URI_PARAM_PAGINATION_OFFSET] = "6";
  uriParams[URI_PARAM_PAGINATION_LIMIT]  = "2";

  /* Invoke the function in mongoBackend library */
  ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
  EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(0, res.responseVector.size());

  utExit();
}

/* ****************************************************************************
*
* paginationNonExistingOverlap -
*
*/
TEST(mongoDiscoverContextAvailabilityRequest, paginationNonExistingOverlap)
{
  HttpStatusCode                       ms;
  DiscoverContextAvailabilityRequest   req;
  DiscoverContextAvailabilityResponse  res;

  utInit();
  prepareDatabaseForPagination();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "", "true");
  req.entityIdVector.push_back(&en);
  uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
  uriParams[URI_PARAM_PAGINATION_OFFSET] = "4";
  uriParams[URI_PARAM_PAGINATION_LIMIT]  = "4";

  /* Invoke the function in mongoBackend library */
  ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.responseVector.size());
  /* Context registration element #1 */
  ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
  EXPECT_EQ("E5", RES_CNTX_REG(0).entityIdVector[0]->id);
  EXPECT_EQ("T5", RES_CNTX_REG(0).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
  ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
  EXPECT_EQ("http://cr5.com", RES_CNTX_REG(0).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* paginationNonExistingDetails -
*
*/
TEST(mongoDiscoverContextAvailabilityRequest, paginationNonExistingDetails)
{
  HttpStatusCode                       ms;
  DiscoverContextAvailabilityRequest   req;
  DiscoverContextAvailabilityResponse  res;

  utInit();
  prepareDatabaseForPagination();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "", "true");
  req.entityIdVector.push_back(&en);
  uriParams[URI_PARAM_PAGINATION_DETAILS]  = "on";
  uriParams[URI_PARAM_PAGINATION_OFFSET]   = "6";
  uriParams[URI_PARAM_PAGINATION_LIMIT]    = "2";

  /* Invoke the function in mongoBackend library */
  ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
  EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
  EXPECT_EQ("Number of matching registrations: 5. Offset is 6", res.errorCode.details);

  ASSERT_EQ(0, res.responseVector.size());

  utExit();
}

/* ****************************************************************************
*
* noPatternAttrsAll -
*
* Discover:  E3 -no attrs
* Result:    E3 - (A1, A2, A3) - http://cr1.com
*/
TEST(mongoDiscoverContextAvailabilityRequest, noPatternAttrsAll)
{
  HttpStatusCode                       ms;
  DiscoverContextAvailabilityRequest   req;
  DiscoverContextAvailabilityResponse  res;

  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E3", "T3");
  req.entityIdVector.push_back(&en);

  /* Invoke the function in mongoBackend library */
  ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_STREQ("", res.errorCode.reasonPhrase.c_str());
  EXPECT_STREQ("", res.errorCode.details.c_str());

  ASSERT_EQ(1, res.responseVector.size());
  /* Context registration element #1 */
  ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
  EXPECT_EQ("E3", RES_CNTX_REG(0).entityIdVector[0]->id);
  EXPECT_EQ("T3", RES_CNTX_REG(0).entityIdVector[0]->type);
  EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
  ASSERT_EQ(3, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
  EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
  EXPECT_EQ("A2", RES_CNTX_REG_ATTR(0, 1)->name);
  EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(0, 1)->type);
  EXPECT_EQ("A3", RES_CNTX_REG_ATTR(0, 2)->name);
  EXPECT_EQ("TA3", RES_CNTX_REG_ATTR(0, 2)->type);
  EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
  EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
  EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

  utExit();
}

/* ****************************************************************************
*
* noPatternAttrOneSingle -
*
* Discover:  E1 - A4
* Result:    E1 - A4 - http://cr2.com
*/
TEST(mongoDiscoverContextAvailabilityRequest, noPatternAttrOneSingle)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A4");

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T1", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A4", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA4", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("http://cr2.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}



/* ****************************************************************************
*
* noPatternAttrOneMulti -
*
* Discover:  E1 - A1
* Result:    E1 - A1 - http://cr1.com
*            E1 - A1 - http://cr2.com
*
* This test also checks that discovering for type (E1) doesn't match with no-typed
* entities (E1** - cr5 is not returned)
*/
TEST(mongoDiscoverContextAvailabilityRequest, noPatternAttrOneMulti)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T1", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

    /* Context registration element #2 */
    ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(1).entityIdVector[0]->id);
    EXPECT_EQ("T1", RES_CNTX_REG(1).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(1, 0)->type);
    EXPECT_EQ("http://cr2.com", RES_CNTX_REG(1).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}


/* ****************************************************************************
*
* noPatternAttrsSubset -
*
* Discover:  E3 - (A1, A2)
* Result:    E3 - (A1, A2) - http://cr1.com
*/
TEST(mongoDiscoverContextAvailabilityRequest, noPatternAttrsSubset)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E3", "T3");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");
    req.attributeList.push_back("A2");

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E3", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T3", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    ASSERT_EQ(2, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(0, 1)->type);
    EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternSeveralCREs -
*
* Discover:  E1 - no attrs
* Result:    E1 - (A1, A2, A3) - http://cr1.com
*            E1 - (A1, A4)     - http://cr2.com
*/
TEST(mongoDiscoverContextAvailabilityRequest, noPatternSeveralCREs)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    req.entityIdVector.push_back(&en);

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T1", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    ASSERT_EQ(3, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(0, 1)->type);
    EXPECT_EQ("A3", RES_CNTX_REG_ATTR(0, 2)->name);
    EXPECT_EQ("TA3", RES_CNTX_REG_ATTR(0, 2)->type);
    EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());
    /* Context registration element #2 */
    ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(1).entityIdVector[0]->id);
    EXPECT_EQ("T1", RES_CNTX_REG(1).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
    ASSERT_EQ(2, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(1, 0)->type);
    EXPECT_EQ("A4", RES_CNTX_REG_ATTR(1, 1)->name);
    EXPECT_EQ("TA4", RES_CNTX_REG_ATTR(1, 1)->type);
    EXPECT_EQ("http://cr2.com", RES_CNTX_REG(1).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternSeveralRegistrations -
*
* Discover:  E2 - no attrs
* Result:    E2 - (A1, A2, A3) - http://cr1.com
*            E2 - (A2, A3)     - http://cr3.com
*/
TEST(mongoDiscoverContextAvailabilityRequest, noPatternSeveralRegistrations)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E2", "T2");
    req.entityIdVector.push_back(&en);

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E2", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T2", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    ASSERT_EQ(3, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(0, 1)->type);
    EXPECT_EQ("A3", RES_CNTX_REG_ATTR(0, 2)->name);
    EXPECT_EQ("TA3", RES_CNTX_REG_ATTR(0, 2)->type);
    EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());
    /* Context registration element #2 */
    ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
    EXPECT_EQ("E2", RES_CNTX_REG(1).entityIdVector[0]->id);
    EXPECT_EQ("T2", RES_CNTX_REG(1).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
    ASSERT_EQ(2, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(1, 0)->type);
    EXPECT_EQ("A3", RES_CNTX_REG_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CNTX_REG_ATTR(1, 1)->type);
    EXPECT_EQ("http://cr3.com", RES_CNTX_REG(1).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternNoEntity -
*
* Discover:  E4 - no attrs
* Result:    none
*/
TEST(mongoDiscoverContextAvailabilityRequest, noPatternNoEntity)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E4", "T4");
    req.entityIdVector.push_back(&en);

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());
    EXPECT_EQ(0, res.responseVector.size());

    /* Delete mock */
    delete timerMock;
}


/* ****************************************************************************
*
* noPatternNoAttribute -
*
* Discover:  E1 - A5
* Result:    none
*/
TEST(mongoDiscoverContextAvailabilityRequest, noPatternNoAttribute)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A5");

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());
    EXPECT_EQ(0, res.responseVector.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternMultiEntity -
*
* Discover:  (E1, E2) - no attrs
* Result:    (E1, E2) - (A1, A2, A3) - http://cr1.com
*            E1       - (A1, A4)     - http://cr2.com
*            E2       - (A2, A3)     - http://cr3.com
*/
TEST(mongoDiscoverContextAvailabilityRequest, noPatternMultiEntity)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(2, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T1", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    EXPECT_EQ("E2", RES_CNTX_REG(0).entityIdVector[1]->id);
    EXPECT_EQ("T2", RES_CNTX_REG(0).entityIdVector[1]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[1]->isPattern);
    ASSERT_EQ(3, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(0, 1)->type);
    EXPECT_EQ("A3", RES_CNTX_REG_ATTR(0, 2)->name);
    EXPECT_EQ("TA3", RES_CNTX_REG_ATTR(0, 2)->type);
    EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());
    /* Context registration element #2 */
    ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(1).entityIdVector[0]->id);
    EXPECT_EQ("T1", RES_CNTX_REG(1).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
    ASSERT_EQ(2, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(1, 0)->type);
    EXPECT_EQ("A4", RES_CNTX_REG_ATTR(1, 1)->name);
    EXPECT_EQ("TA4", RES_CNTX_REG_ATTR(1, 1)->type);
    EXPECT_EQ("http://cr2.com", RES_CNTX_REG(1).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());
    /* Context registration element #3 */
    ASSERT_EQ(1, RES_CNTX_REG(2).entityIdVector.size());
    EXPECT_EQ("E2", RES_CNTX_REG(2).entityIdVector[0]->id);
    EXPECT_EQ("T2", RES_CNTX_REG(2).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(2).entityIdVector[0]->isPattern);
    ASSERT_EQ(2, RES_CNTX_REG(2).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(2, 0)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(2, 0)->type);
    EXPECT_EQ("A3", RES_CNTX_REG_ATTR(2, 1)->name);
    EXPECT_EQ("TA3", RES_CNTX_REG_ATTR(2, 1)->type);
    EXPECT_EQ("http://cr3.com", RES_CNTX_REG(2).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[2]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[2]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[2]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternMultiAttr -
*
* Discover:  E1 - (A3, A4, A5)
* Result:    E1 - A3 - http://cr1.com
*            E1 - A4 - http://cr2.com
*/
TEST(mongoDiscoverContextAvailabilityRequest, noPatternMultiAttr)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A3");
    req.attributeList.push_back("A4");
    req.attributeList.push_back("A5");

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T1", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A3", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA3", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());
    /* Context registration element #2 */
    ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(1).entityIdVector[0]->id);
    EXPECT_EQ("T1", RES_CNTX_REG(1).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A4", RES_CNTX_REG_ATTR(1, 0)->name);
    EXPECT_EQ("TA4", RES_CNTX_REG_ATTR(1, 0)->type);
    EXPECT_EQ("http://cr2.com", RES_CNTX_REG(1).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternMultiEntityAttrs -
*
* Discover:  (E1, E2) - (A3, A4, A5)
* Result:    (E1, E2) - A3 - http://cr1.com
*            E1       - A4 - http://cr2.com
*            E2       - A3 - http://cr3.com
*/
TEST(mongoDiscoverContextAvailabilityRequest, noPatternMultiEntityAttrs)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1");
    EntityId en2("E2", "T2");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.attributeList.push_back("A3");
    req.attributeList.push_back("A4");
    req.attributeList.push_back("A5");

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(2, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T1", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    EXPECT_EQ("E2", RES_CNTX_REG(0).entityIdVector[1]->id);
    EXPECT_EQ("T2", RES_CNTX_REG(0).entityIdVector[1]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[1]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A3", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA3", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());
    /* Context registration element #2 */
    ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(1).entityIdVector[0]->id);
    EXPECT_EQ("T1", RES_CNTX_REG(1).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A4", RES_CNTX_REG_ATTR(1, 0)->name);
    EXPECT_EQ("TA4", RES_CNTX_REG_ATTR(1, 0)->type);
    EXPECT_EQ("http://cr2.com", RES_CNTX_REG(1).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());
    /* Context registration element #3 */
    ASSERT_EQ(1, RES_CNTX_REG(2).entityIdVector.size());
    EXPECT_EQ("E2", RES_CNTX_REG(2).entityIdVector[0]->id);
    EXPECT_EQ("T2", RES_CNTX_REG(2).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(2).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(2).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A3", RES_CNTX_REG_ATTR(2, 0)->name);
    EXPECT_EQ("TA3", RES_CNTX_REG_ATTR(2, 0)->type);
    EXPECT_EQ("http://cr3.com", RES_CNTX_REG(2).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[2]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[2]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[2]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* noPatternNoType -
*
* Discover:  E1** - A1
* Result:    E1   - A1 - http://cr1.com
*            E1   - A1 - http://cr2.com
*            E1*  - A1*- http://cr4.com
*            E1** - A1 - http://cr5.com
*
* Note that this case checks matching of no-type in the discover for both the case in
* which the returned CR has type (cr1, cr2, cr4) and the case in which it has no type (cr5).
*
*/
TEST(mongoDiscoverContextAvailabilityRequest, noPatternNoType)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(4, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T1", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

    /* Context registration element #2 */
    ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(1).entityIdVector[0]->id);
    EXPECT_EQ("T1", RES_CNTX_REG(1).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(1, 0)->type);
    EXPECT_EQ("http://cr2.com", RES_CNTX_REG(1).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());

    /* Context registration element #3 */
    ASSERT_EQ(1, RES_CNTX_REG(2).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(2).entityIdVector[0]->id);
    EXPECT_EQ("T1bis", RES_CNTX_REG(2).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(2).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(2).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(2, 0)->name);
    EXPECT_EQ("TA1bis", RES_CNTX_REG_ATTR(2, 0)->type);
    EXPECT_EQ("http://cr4.com", RES_CNTX_REG(2).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[2]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[2]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[2]->errorCode.details.size());

    /* Context registration element #4 */
    ASSERT_EQ(1, RES_CNTX_REG(3).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(3).entityIdVector[0]->id);
    EXPECT_EQ(0, RES_CNTX_REG(3).entityIdVector[0]->type.size());
    EXPECT_EQ("false", RES_CNTX_REG(3).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(3).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(3, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(3, 0)->type);
    EXPECT_EQ("http://cr5.com", RES_CNTX_REG(3).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[3]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[3]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[3]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* pattern0Attr -
*
* Discover:  E[2-3] - none
* Result:    (E2, E3) - (A1, A2, A3) - http://cr1.com
*            E2       - (A2, A3)     - http://cr3.com
*
* This test also checks that discovering for type (E[2-3]) doesn't match with no-typed
* entities (E3** - cr5 is not returned)
*
*/
TEST(mongoDiscoverContextAvailabilityRequest, pattern0Attr)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[2-3]", "T", "true");
    req.entityIdVector.push_back(&en);

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(2, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E2", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    EXPECT_EQ("E3", RES_CNTX_REG(0).entityIdVector[1]->id);
    EXPECT_EQ("T", RES_CNTX_REG(0).entityIdVector[1]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[1]->isPattern);
    ASSERT_EQ(3, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(0, 1)->type);
    EXPECT_EQ("A3", RES_CNTX_REG_ATTR(0, 2)->name);
    EXPECT_EQ("TA3", RES_CNTX_REG_ATTR(0, 2)->type);
    EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_STREQ("", res.responseVector[0]->errorCode.reasonPhrase.c_str());
    EXPECT_STREQ("", res.responseVector[0]->errorCode.details.c_str());

    /* Context registration element #2 */
    ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
    EXPECT_EQ("E2", RES_CNTX_REG(1).entityIdVector[0]->id);
    EXPECT_EQ("T", RES_CNTX_REG(1).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
    ASSERT_EQ(2, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(1, 0)->type);
    EXPECT_EQ("A3", RES_CNTX_REG_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CNTX_REG_ATTR(1, 1)->type);
    EXPECT_EQ("http://cr3.com", RES_CNTX_REG(1).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* pattern1AttrSingle -
*
* Discover:  E[1-3] - A4
* Result:    E1 - A4 - http://cr2.com
*/
TEST(mongoDiscoverContextAvailabilityRequest, pattern1AttrSingle)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-3]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A4");

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(1, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A4", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA4", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("http://cr2.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* pattern1AttrMulti -
*
* Discover:  E[1-2] - A1
* Result:    (E1, E2) - A1 - http://cr1.com
*            E1       - A1 - http://cr2.com
*/
TEST(mongoDiscoverContextAvailabilityRequest, pattern1AttrMulti)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(2, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    EXPECT_EQ("E2", RES_CNTX_REG(0).entityIdVector[1]->id);
    EXPECT_EQ("T", RES_CNTX_REG(0).entityIdVector[1]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[1]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

    /* Context registration element #2 */
    ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(1).entityIdVector[0]->id);
    EXPECT_EQ("T", RES_CNTX_REG(1).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(1, 0)->type);
    EXPECT_EQ("http://cr2.com", RES_CNTX_REG(1).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* patternNAttr -
*
* Discover:  E[1-2] - (A1, A2)
* Result:    (E1. E2) - (A1, A2) - http://cr1.com
*            E1      - A1        - http://cr2.com
*            E2      - A2        - http://cr3.com
*/
TEST(mongoDiscoverContextAvailabilityRequest, patternNAttr)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");
    req.attributeList.push_back("A2");

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(2, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    EXPECT_EQ("E2", RES_CNTX_REG(0).entityIdVector[1]->id);
    EXPECT_EQ("T", RES_CNTX_REG(0).entityIdVector[1]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[1]->isPattern);
    ASSERT_EQ(2, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(0, 1)->type);
    EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

    /* Context registration element #2 */
    ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(1).entityIdVector[0]->id);
    EXPECT_EQ("T", RES_CNTX_REG(1).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(1, 0)->type);
    EXPECT_EQ("http://cr2.com", RES_CNTX_REG(1).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());

    /* Context registration element #3 */
    ASSERT_EQ(1, RES_CNTX_REG(2).entityIdVector.size());
    EXPECT_EQ("E2", RES_CNTX_REG(2).entityIdVector[0]->id);
    EXPECT_EQ("T", RES_CNTX_REG(2).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(2).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(2).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(2, 0)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(2, 0)->type);
    EXPECT_EQ("http://cr3.com", RES_CNTX_REG(2).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[2]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[2]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[2]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* patternFail -
*
* Discover:  R.* - none
* Result:    none
*/
TEST(mongoDiscoverContextAvailabilityRequest, patternFail)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("R.*", "T", "true");
    req.entityIdVector.push_back(&en);

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());
    EXPECT_EQ(0, res.responseVector.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* patternNoType -
*
* Discover:  E[2-3]** - A2
* Result:    E2, E3 - A2  - http://cr1.com
*            E2     - A2  - http://cr3.com
*            E2*    - A2* - http://cr4.com
*            E3**   - A2  - http://cr5.com
*
* Note that this case checks matching of no-type in the discover for both the case in
* which the returned CR has type (cr1, cr3, cr4) and the case in which it has no type (cr5).
*
*/
TEST(mongoDiscoverContextAvailabilityRequest, patternNoType)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[2-3]", "", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A2");

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(4, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(2, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E2", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    EXPECT_EQ("E3", RES_CNTX_REG(0).entityIdVector[1]->id);
    EXPECT_EQ("T", RES_CNTX_REG(0).entityIdVector[1]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[1]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

    /* Context registration element #2 */
    ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
    EXPECT_EQ("E2", RES_CNTX_REG(1).entityIdVector[0]->id);
    EXPECT_EQ("T", RES_CNTX_REG(1).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(1, 0)->type);
    EXPECT_EQ("http://cr3.com", RES_CNTX_REG(1).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());

    /* Context registration element #3 */
    ASSERT_EQ(1, RES_CNTX_REG(2).entityIdVector.size());
    EXPECT_EQ("E2", RES_CNTX_REG(2).entityIdVector[0]->id);
    EXPECT_EQ("Tbis", RES_CNTX_REG(2).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(2).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(2).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(2, 0)->name);
    EXPECT_EQ("TA2bis", RES_CNTX_REG_ATTR(2, 0)->type);
    EXPECT_EQ("http://cr4.com", RES_CNTX_REG(2).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[2]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[2]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[2]->errorCode.details.size());

    /* Context registration element #4 */
    ASSERT_EQ(1, RES_CNTX_REG(3).entityIdVector.size());
    EXPECT_EQ("E3", RES_CNTX_REG(3).entityIdVector[0]->id);
    EXPECT_EQ(0, RES_CNTX_REG(3).entityIdVector[0]->type.size());
    EXPECT_EQ("false", RES_CNTX_REG(3).entityIdVector[0]->isPattern);
    ASSERT_EQ(1, RES_CNTX_REG(3).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(3, 0)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(3, 0)->type);
    EXPECT_EQ("http://cr5.com", RES_CNTX_REG(3).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[3]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[3]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[3]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}

/* ****************************************************************************
*
* mixPatternAndNotPattern -
*
* Discover:  (E[2-3]. E1) - none
* Result:    (E1, E2, E3) - (A1, A2, A3) - http://cr1.com
*            E1           - (A1 ,A4) - http://cr2.com
*            E2           - (A2, A3) - http://cr3.com
*/
TEST(mongoDiscoverContextAvailabilityRequest, mixPatternAndNotPattern)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;


    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E[2-3]", "T", "true");
    EntityId en2("E1", "T");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.responseVector.size());
    /* Context registration element #1 */
    ASSERT_EQ(3, RES_CNTX_REG(0).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(0).entityIdVector[0]->id);
    EXPECT_EQ("T", RES_CNTX_REG(0).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[0]->isPattern);
    EXPECT_EQ("E2", RES_CNTX_REG(0).entityIdVector[1]->id);
    EXPECT_EQ("T", RES_CNTX_REG(0).entityIdVector[1]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[1]->isPattern);
    EXPECT_EQ("E3", RES_CNTX_REG(0).entityIdVector[2]->id);
    EXPECT_EQ("T", RES_CNTX_REG(0).entityIdVector[2]->type);
    EXPECT_EQ("false", RES_CNTX_REG(0).entityIdVector[2]->isPattern);
    ASSERT_EQ(3, RES_CNTX_REG(0).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(0, 0)->type);
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(0, 1)->type);
    EXPECT_EQ("A3", RES_CNTX_REG_ATTR(0, 2)->name);
    EXPECT_EQ("TA3", RES_CNTX_REG_ATTR(0, 2)->type);
    EXPECT_EQ("http://cr1.com", RES_CNTX_REG(0).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[0]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[0]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[0]->errorCode.details.size());

    /* Context registration element #2 */
    ASSERT_EQ(1, RES_CNTX_REG(1).entityIdVector.size());
    EXPECT_EQ("E1", RES_CNTX_REG(1).entityIdVector[0]->id);
    EXPECT_EQ("T", RES_CNTX_REG(1).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(1).entityIdVector[0]->isPattern);
    ASSERT_EQ(2, RES_CNTX_REG(1).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A1", RES_CNTX_REG_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CNTX_REG_ATTR(1, 0)->type);
    EXPECT_EQ("A4", RES_CNTX_REG_ATTR(1, 1)->name);
    EXPECT_EQ("TA4", RES_CNTX_REG_ATTR(1, 1)->type);
    EXPECT_EQ("http://cr2.com", RES_CNTX_REG(1).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[1]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[1]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[1]->errorCode.details.size());

    /* Context registration element #3 */
    ASSERT_EQ(1, RES_CNTX_REG(2).entityIdVector.size());
    EXPECT_EQ("E2", RES_CNTX_REG(2).entityIdVector[0]->id);
    EXPECT_EQ("T", RES_CNTX_REG(2).entityIdVector[0]->type);
    EXPECT_EQ("false", RES_CNTX_REG(2).entityIdVector[0]->isPattern);
    ASSERT_EQ(2, RES_CNTX_REG(2).contextRegistrationAttributeVector.size());
    EXPECT_EQ("A2", RES_CNTX_REG_ATTR(2, 0)->name);
    EXPECT_EQ("TA2", RES_CNTX_REG_ATTR(2, 0)->type);
    EXPECT_EQ("A3", RES_CNTX_REG_ATTR(2, 1)->name);
    EXPECT_EQ("TA3", RES_CNTX_REG_ATTR(2, 1)->type);
    EXPECT_EQ("http://cr3.com", RES_CNTX_REG(2).providingApplication.get());
    EXPECT_EQ(SccNone, res.responseVector[2]->errorCode.code);
    EXPECT_EQ(0, res.responseVector[2]->errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.responseVector[2]->errorCode.details.size());

    /* Delete mock */
    delete timerMock;
}


/* ****************************************************************************
*
* mongoDbQueryFail -
*
*/
TEST(mongoDiscoverContextAvailabilityRequest, mongoDbQueryFail)
{
    HttpStatusCode                       ms;
    DiscoverContextAvailabilityRequest   req;
    DiscoverContextAvailabilityResponse  res;

    /* Prepare mock */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, _query(_, _, _, _, _, _, _))
            .WillByDefault(Throw(e));

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Set MongoDB connection */
    DBClientBase* connectionDb = getMongoConnection();
    setMongoConnectionForUnitTest(connectionMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E3", "T3");

    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoDiscoverContextAvailability(&req, &res, "", uriParams, servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);
    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Internal Server Error", res.errorCode.reasonPhrase);

    EXPECT_EQ("Database Error (collection: utest.registrations "
              "- query(): { query: "
              "{ $or: [ { contextRegistration.entities.id: \"E3\", contextRegistration.entities.type: \"T3\" }, "
                       "{ contextRegistration.entities.id: \".*\", contextRegistration.entities.isPattern: \"true\", contextRegistration.entities.type: { $in: [ \"T3\" ] } }, "
                       "{ contextRegistration.entities.id: \".*\", contextRegistration.entities.isPattern: \"true\", contextRegistration.entities.type: { $exists: false } } ], "
              "expiration: { $gt: 1360232700 } }, "
              "orderby: { _id: 1 } } - exception: boom!!)", res.errorCode.details);
    EXPECT_EQ(0, res.responseVector.size());

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb);

    /* Release mock */
    delete connectionMock;
    delete timerMock;
}
