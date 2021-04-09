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
#include <string>
#include <vector>

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoConnectionPool.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi/StatusCode.h"
#include "ngsi/EntityId.h"
#include "ngsi/Scope.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"

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



extern void setMongoConnectionForUnitTest(orion::DBClientBase _connection);



/* ****************************************************************************
*
* Tests
*
* These test are based on the ones in the mongoDiscoverContextAvailability_test.cpp file.
* In other words, the test in this file are updateContext operations that involve a
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
* - severalCprs
*
* Failing cases
*
* - notFoundUpdate
* - notFoundDelete
*
* Note these tests are not "canonical" unit tests. Canon says that in this case we should have
* mocked MongoDB. Actually, we think is very much powerful to check that everything is ok at
* MongoDB layer.
*
*/



/* ****************************************************************************
*
* findAttr -
*/
static bool findAttr(std::vector<BSONElement> attrs, std::string name)
{
  for (unsigned int ix = 0; ix < attrs.size(); ++ix)
  {
    if (attrs[ix].str() == name)
    {
      return true;
    }
  }

  return false;
}



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

  BSONObj reg1 = BSON("_id" << OID("51307b66f481db11bf86ff37") <<
                      "expiration" << 1879048191 <<
                      "subscriptions" << BSONArray() <<
                      "contextRegistration" << BSON_ARRAY(cr1 << cr2));

  BSONObj reg2 = BSON("_id" << OID("51307b66f481db11bf86ff48") <<
                      "expiration" << 1879048191 <<
                      "subscriptions" << BSONArray() <<
                      "contextRegistration" << BSON_ARRAY(cr3));

  BSONObj reg3 = BSON("_id" << OID("51307b66f481db11bf86ff80") <<
                      "expiration" << 1879048191 <<
                      "subscriptions" << BSONArray() <<
                      "contextRegistration" << BSON_ARRAY(cr4));

  BSONObj reg4 = BSON("_id" << OID("51307b66f481db11bf86ff90") <<
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
static void prepareDatabaseSeveralCprs(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following entities:
   *
   *   E1 - A1 - 1
   *   E2 - A3 - 3
   *
   * We create the following registries
   *
   * - Reg1: CR: E1 - A2 - CPR2
   * - Reg2: CR: E2 - A4 - CPR3
   * - Reg3: CR: E3 - A5 - CPR2
   * - Reg4: CR: E4 - <null> - CPR1
   *
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "T" << "value" << "1")));

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A3") <<
                     "attrs" << BSON(
                       "A3" << BSON("type" << "T" << "value" << "3")));

  BSONObj cr1 = BSON("providingApplication" << "http://cpr2.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E1" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A2" << "type" << "T")));

  BSONObj cr2 = BSON("providingApplication" << "http://cpr3.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E2" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A4" << "type" << "T")));

  BSONObj cr3 = BSON("providingApplication" << "http://cpr2.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E3" << "type" << "T")) <<
                     "attrs" << BSON_ARRAY(
                       BSON("name" << "A5" << "type" << "T")));

  BSONObj cr4 = BSON("providingApplication" << "http://cpr1.com" <<
                     "entities" << BSON_ARRAY(
                       BSON("id" << "E4" << "type" << "T")) <<
                     "attrs" << BSONArray());

  /* 1879048191 corresponds to year 2029 so we avoid any expiration problem in the next 16 years :) */
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

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(REGISTRATIONS_COLL, reg1.obj());
  connection->insert(REGISTRATIONS_COLL, reg2.obj());
  connection->insert(REGISTRATIONS_COLL, reg3.obj());
  connection->insert(REGISTRATIONS_COLL, reg4.obj());
}



/* ****************************************************************************
*
* noPatternAttrsAll -
*
* Discover:  E3 - no attrs
* Result:    E3 - no attrs
*
* Actually this test is a bit stupid :) but it correspond with a situation
* allowed in the NGSI syntax, so we have to include it
*
*/
TEST(mongoContextProvidersUpdateRequest, noPatternAttrsAll)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  /* Note that although it is a bit weird having an updateContext without attributes to update,
   * it is legal from the point of view of OMA spec */
  Entity* eP = new Entity();
  eP->fill("E3", "T3", "false");
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  EXPECT_EQ("E3", RES_CER(0).id);
  EXPECT_EQ("T3", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  EXPECT_EQ(SccContextElementNotFound, RES_CER_STATUS(0).code);
  EXPECT_EQ("No context element found", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);
  EXPECT_EQ(0, RES_CER(0).attributeVector.size());

  utExit();
}

/* ****************************************************************************
*
* noPatternAttrOneSingle -
*
* Discover:  E1 - A4
* Result:    E1 - A4 - http://cr2.com
*/
TEST(mongoContextProvidersUpdateRequest, noPatternAttrOneSingle)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  Entity* eP = new Entity();
  eP->fill("E1", "T1", "false");
  ContextAttribute* caP = new ContextAttribute("A4", "TA4", "new_val");
  eP->attributeVector.push_back(caP);
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("T1", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());

  EXPECT_EQ("A4", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA4", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
  EXPECT_EQ("http://cr2.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(0, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}

/* ****************************************************************************
*
* noPatternAttrOneMulti -
*
* Discover:  E1 - A1
* Result:    E1 - A1 - http://cr1.com
*
* Overlap:   E1 - A1 - http://cr2.com
*
* This test also checks that discovering for type (E1) doesn't match with no-typed
* entities (E1** - cr5 is not returned)
*/
TEST(mongoContextProvidersUpdateRequest, noPatternAttrOneMulti)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  Entity* eP = new Entity();
  eP->fill("E1", "T1", "false");
  ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
  eP->attributeVector.push_back(caP);
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("T1", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(0, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}


/* ****************************************************************************
*
* noPatternAttrsSubset -
*
* Discover:  E3 - (A1, A2)
* Result:    E3 - (A1, A2) - http://cr1.com
*/
TEST(mongoContextProvidersUpdateRequest, noPatternAttrsSubset)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  Entity* eP = new Entity();
  eP->fill("E3", "T3", "false");
  ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "new_val");
  ContextAttribute* ca2P = new ContextAttribute("A2", "TA2", "new_val");
  eP->attributeVector.push_back(ca1P);
  eP->attributeVector.push_back(ca2P);
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E3", RES_CER(0).id);
  EXPECT_EQ("T3", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(2, RES_CER(0).attributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(0, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(0, 1)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}


/* ****************************************************************************
*
* noPatternNoAttribute -
*
* Discover:  E1 - A5
* Result:    none
*/
TEST(mongoContextProvidersUpdateRequest, noPatternNoAttribute)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  Entity* eP = new Entity();
  eP->fill("E1", "T1", "false");
  ContextAttribute* caP = new ContextAttribute("A5", "TA5", "new_val");
  eP->attributeVector.push_back(caP);
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("T1", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());

  EXPECT_EQ("A5", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA5", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_FALSE(RES_CER_ATTR(0, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());

  EXPECT_EQ(SccContextElementNotFound, RES_CER_STATUS(0).code);
  EXPECT_EQ("No context element found", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}

/* ****************************************************************************
*
* noPatternMultiEntity -
*
* Discover:  (E1, E2) - no attrs
* Result:    E1 - none
*            E2 - none
*
* Actually this test is a bit stupid :) but it correspond with a situation
* allowed in the NGSI syntax, so we have to include it
*
*/
TEST(mongoContextProvidersUpdateRequest, noPatternMultiEntity)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  /* Note that although it is a bit weird having an updateContext without attributes to update,
     * it is legal from the point of view of OMA spec */
  Entity* e1P = new Entity();
  Entity* e2P = new Entity();
  e1P->fill("E1", "T1", "false");
  e2P->fill("E2", "T2", "false");
  req.entityVector.push_back(e1P);
  req.entityVector.push_back(e2P);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("T1", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  EXPECT_EQ(SccContextElementNotFound, RES_CER_STATUS(0).code);
  EXPECT_EQ("No context element found", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);
  EXPECT_EQ(0, RES_CER(0).attributeVector.size());

  EXPECT_EQ("E2", RES_CER(1).id);
  EXPECT_EQ("T2", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  EXPECT_EQ(0, RES_CER(1).providingApplicationList.size());
  EXPECT_EQ(SccContextElementNotFound, RES_CER_STATUS(1).code);
  EXPECT_EQ("No context element found", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(1).details);
  EXPECT_EQ(0, RES_CER(1).attributeVector.size());

  utExit();
}

/* ****************************************************************************
*
* noPatternMultiAttr -
*
* Discover:  E1 - (A3, A4, A5)
* Result:    E1 - A3 - http://cr1.com
*            E1 - A4 - http://cr2.com
*            E1 - A5 - not found
*/
TEST(mongoContextProvidersUpdateRequest, noPatternMultiAttr)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  Entity* eP = new Entity();
  eP->fill("E1", "T1", "false");
  ContextAttribute* ca1P = new ContextAttribute("A3", "TA3", "new_val");
  ContextAttribute* ca2P = new ContextAttribute("A4", "TA4", "new_val");
  ContextAttribute* ca3P = new ContextAttribute("A5", "TA5", "new_val");
  eP->attributeVector.push_back(ca1P);
  eP->attributeVector.push_back(ca2P);
  eP->attributeVector.push_back(ca3P);
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("T1", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(3, RES_CER(0).attributeVector.size());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA3", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(0, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());

  EXPECT_EQ("A4", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("TA4", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
  EXPECT_EQ("http://cr2.com", RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(0, 1)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());

  EXPECT_EQ("A5", RES_CER_ATTR(0, 2)->name);
  EXPECT_EQ("TA5", RES_CER_ATTR(0, 2)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 2)->stringValue.size());
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->providingApplication.get());
  EXPECT_FALSE(RES_CER_ATTR(0, 2)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 2)->metadataVector.size());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}

/* ****************************************************************************
*
* noPatternMultiEntityAttrs -
*
* Discover:  (E1, E2) - (A3, A4, A5)
* Result:    E1 - A3 - http://cr1.com
*                 A4 - http://cr2.com
*                 A5 - Not found
*            E2 - A3 - http://cr1.com
*                 A4 - Not found
*                 A5 - Not found
*
* Overlap: E1 - A4 - http://cr2.com
*          E2 - A3 - http://cr3.com
*
*/
TEST(mongoContextProvidersUpdateRequest, noPatternMultiEntityAttrs)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  Entity* e1P = new Entity();
  Entity* e2P = new Entity();
  e1P->fill("E1", "T1", "false");
  e2P->fill("E2", "T2", "false");
  ContextAttribute* ca1P = new ContextAttribute("A3", "TA3", "new_val");
  ContextAttribute* ca2P = new ContextAttribute("A4", "TA4", "new_val");
  ContextAttribute* ca3P = new ContextAttribute("A5", "TA5", "new_val");
  ContextAttribute* ca4P = new ContextAttribute("A3", "TA3", "new_val");
  ContextAttribute* ca5P = new ContextAttribute("A4", "TA4", "new_val");
  ContextAttribute* ca6P = new ContextAttribute("A5", "TA5", "new_val");
  e1P->attributeVector.push_back(ca1P);
  e1P->attributeVector.push_back(ca2P);
  e1P->attributeVector.push_back(ca3P);
  e2P->attributeVector.push_back(ca4P);
  e2P->attributeVector.push_back(ca5P);
  e2P->attributeVector.push_back(ca6P);
  req.entityVector.push_back(e1P);
  req.entityVector.push_back(e2P);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("T1", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(3, RES_CER(0).attributeVector.size());

  EXPECT_EQ("A3", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA3", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(1, 0)->stringValue.size());
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(0, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());

  EXPECT_EQ("A4", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("TA4", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
  EXPECT_EQ("http://cr2.com", RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(0, 1)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());

  EXPECT_EQ("A5", RES_CER_ATTR(0, 2)->name);
  EXPECT_EQ("TA5", RES_CER_ATTR(0, 2)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 2)->stringValue.size());
  EXPECT_EQ("", RES_CER_ATTR(0, 2)->providingApplication.get());
  EXPECT_FALSE(RES_CER_ATTR(0, 2)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 2)->metadataVector.size());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E2", RES_CER(1).id);
  EXPECT_EQ("T2", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  EXPECT_EQ(0, RES_CER(1).providingApplicationList.size());
  ASSERT_EQ(3, RES_CER(1).attributeVector.size());

  EXPECT_EQ("A3", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA3", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(1, 0)->stringValue.size());
  EXPECT_EQ("http://cr1.com", RES_CER_ATTR(1, 0)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(1, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());

  EXPECT_EQ("A4", RES_CER_ATTR(1, 1)->name);
  EXPECT_EQ("TA4", RES_CER_ATTR(1, 1)->type);
  EXPECT_EQ(0, RES_CER_ATTR(1, 1)->stringValue.size());
  EXPECT_EQ("", RES_CER_ATTR(1, 1)->providingApplication.get());
  EXPECT_FALSE(RES_CER_ATTR(1, 1)->found);
  EXPECT_EQ(0, RES_CER_ATTR(1, 1)->metadataVector.size());

  EXPECT_EQ("A5", RES_CER_ATTR(1, 2)->name);
  EXPECT_EQ("TA5", RES_CER_ATTR(1, 2)->type);
  EXPECT_EQ(0, RES_CER_ATTR(1, 2)->stringValue.size());
  EXPECT_EQ("", RES_CER_ATTR(1, 2)->providingApplication.get());
  EXPECT_FALSE(RES_CER_ATTR(1, 2)->found);
  EXPECT_EQ(0, RES_CER_ATTR(1, 2)->metadataVector.size());

  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(1).details);

  utExit();
}

/* ****************************************************************************
*
* noPatternNoType -
*
* Discover:  E1** - A1
* Result:    E1** - A1 - http://cr5.com
*
* Note that registration database has:
*            E1   - A1 - http://cr1.com
*            E1   - A1 - http://cr2.com
*            E1*  - A1*- http://cr4.com
*            E1** - A1 - http://cr5.com
*
* However, considering the following match condition in searchContextProviders()
*
*  (regEn->id != en.id || (regEn->type != en.type && regEn->type != ""))
*
* an registration without type match any type but the opposite is not true: an entity
* without type does *not* match any registration.
*
*/
TEST(mongoContextProvidersUpdateRequest, noPatternNoType)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  Entity* eP = new Entity();
  eP->fill("E1", "", "false");
  ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
  eP->attributeVector.push_back(caP);
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
  EXPECT_EQ("http://cr5.com", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(0, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());

  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}

/* ****************************************************************************
*
* pattern0Attr -
*
* Discover:  E[2-3] - none
* Result?:   (E2, E3) - (A1, A2, A3) - http://cr1.com
*            E2       - (A2, A3)     - http://cr3.com
*
* This test also checks that discovering for type (E[2-3]) doesn't match with no-typed
* entities (E3** - cr5 is not returned)
*
* isPattern=true is not currently supported in updateContext, so this test it disabled: enable it once
* this gets supported (need modifications to work)
*/
TEST(DISABLED_mongoContextProvidersUpdateRequest, pattern0Attr)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  /* Note that although it is a bit weird having an updateContext without attributes to update,
     * it is legal from the point of view of OMA spec */
  Entity* eP = new Entity();
  eP->fill("E[2-3]", "T", "true");
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  // TBD

  utExit();
}

/* ****************************************************************************
*
* pattern1AttrSingle -
*
* Discover:  E[1-3] - A4
* Result?:    E1 - A4 - http://cr2.com
*
* isPattern=true is not currently supported in updateContext, so this test it disabled: enable it once
* this gets supported (need extra modification to work)
*/
TEST(DISABLED_mongoContextProvidersUpdateRequest, pattern1AttrSingle)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  Entity* eP = new Entity();
  eP->fill("E[1-3]", "T", "true");
  ContextAttribute* caP = new ContextAttribute("A4", "TA4", "new_val");
  eP->attributeVector.push_back(caP);
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  // TBD

  utExit();
}

/* ****************************************************************************
*
* pattern1AttrMulti -
*
* Discover:  E[1-2] - A1
* Result?:    (E1, E2) - A1 - http://cr1.com
*            E1       - A1 - http://cr2.com
*
* isPattern=true is not currently supported in updateContext, so this test it disabled: enable it once
* this gets supported (need extra modification to work)
*
*/
TEST(DISABLED_mongoContextProvidersUpdateRequest, pattern1AttrMulti)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  Entity* eP = new Entity();
  eP->fill("E[1-2]", "T", "true");
  ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
  eP->attributeVector.push_back(caP);
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  // TBD

  utExit();
}

/* ****************************************************************************
*
* patternNAttr -
*
* Discover:  E[1-2] - (A1, A2)
* Result?:   (E1. E2) - (A1, A2) - http://cr1.com
*            E1      - A1        - http://cr2.com
*            E2      - A2        - http://cr3.com
*
* isPattern=true is not currently supported in updateContext, so this test it disabled: enable it once
* this gets supported (need extra modification to work)
*/
TEST(DISABLED_mongoContextProvidersUpdateRequest, patternNAttr)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  Entity* eP = new Entity();
  eP->fill("E[1-2]", "T", "true");
  ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "new_val");
  ContextAttribute* ca2P = new ContextAttribute("A2", "TA2", "new_val");
  eP->attributeVector.push_back(ca1P);
  eP->attributeVector.push_back(ca2P);
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  // TBD

  utExit();
}

/* ****************************************************************************
*
* patternFail -
*
* Discover:  R.* - none
* Result?:   none
*
* isPattern=true is not currently supported in updateContext, so this test it disabled: enable it once
* this gets supported (need extra modification to work)
*/
TEST(DISABLED_mongoContextProvidersUpdateRequest, patternFail)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  /* Note that although it is a bit weird having an updateContext without attributes to update,
     * it is legal from the point of view of OMA spec */
  Entity* eP = new Entity();
  eP->fill("R.*", "T", "true");
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  // TBD

  utExit();
}

/* ****************************************************************************
*
* patternNoType -
*
* Discover:  E[2-3]** - A2
* Result?:   E2, E3 - A2  - http://cr1.com
*            E2     - A2  - http://cr3.com
*            E2*    - A2* - http://cr4.com
*            E3**   - A2  - http://cr5.com
*
* Note that this case checks matching of no-type in the discover for both the case in
* which the returned CR has type (cr1, cr3, cr4) and the case in which it has no type (cr5).
*
* isPattern=true is not currently supported in updateContext, so this test it disabled: enable it once
* this gets supported (need extra modification to work)
*/
TEST(DISABLED_mongoContextProvidersUpdateRequest, patternNoType)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  Entity* eP = new Entity();
  eP->fill("E[2-3]", "", "true");
  ContextAttribute* caP = new ContextAttribute("A2", "TA2", "new_val");
  eP->attributeVector.push_back(caP);
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  // TBD

  utExit();
}

/* ****************************************************************************
*
* mixPatternAndNotPattern -
*
* Discover:  (E[2-3]. E1) - none
* Result?:   (E1, E2, E3) - (A1, A2, A3) - http://cr1.com
*            E1           - (A1 ,A4) - http://cr2.com
*            E2           - (A2, A3) - http://cr3.com
*
* isPattern=true is not currently supported in updateContext, so this test it disabled: enable it once
* this gets supported (need extra modification to work)
*
*/
TEST(DISABLED_mongoContextProvidersUpdateRequest, mixPatternAndNotPattern)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabasePatternTrue();

  /* Forge the request (from "inside" to "outside") */
  /* Note that although it is a bit weird having an updateContext without attributes to update,
     * it is legal from the point of view of OMA spec */
  Entity* e1P = new Entity();
  Entity* e2P = new Entity();
  e1P->fill("E[2-3]", "T", "true");
  e2P->fill("E1", "T", "false");
  req.entityVector.push_back(e1P);
  req.entityVector.push_back(e2P);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  // TBD

  utExit();
}

/* ****************************************************************************
*
* severalCprs -
*
* Update: E1 - (A1, A2)
*         E2 - (A3, A4)
*         E3 - (A5, A6)
*         E4 - A7
* Result: E1 - A1 - Up Ok
*              A2 - fwd CPR2
*         E2 - A3 - Up ok
*              A4 - fwd CPR3
*         E3 - A5 - fwd CPR2
*              A6 - Not found
*         E4 - A7 - fwd CPR1
*
*/
TEST(mongoContextProvidersUpdateRequest, severalCprs)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  /* Prepare database */
  utInit();
  prepareDatabaseSeveralCprs();

  /* Forge the request (from "inside" to "outside") */
  Entity* e1P = new Entity();
  Entity* e2P = new Entity();
  Entity* e3P = new Entity();
  Entity* e4P = new Entity();

  e1P->fill("E1", "T", "false");
  ContextAttribute* ca1P = new ContextAttribute("A1", "T", "10");
  ContextAttribute* ca2P = new ContextAttribute("A2", "T", "20");
  e1P->attributeVector.push_back(ca1P);
  e1P->attributeVector.push_back(ca2P);
  e2P->fill("E2", "T", "false");
  ContextAttribute* ca3P = new ContextAttribute("A4", "T", "40");
  ContextAttribute* ca4P = new ContextAttribute("A3", "T", "30");
  e2P->attributeVector.push_back(ca3P);
  e2P->attributeVector.push_back(ca4P);
  e3P->fill("E3", "T", "false");
  ContextAttribute* ca5P = new ContextAttribute("A5", "T", "50");
  ContextAttribute* ca6P = new ContextAttribute("A6", "T", "60");
  e3P->attributeVector.push_back(ca5P);
  e3P->attributeVector.push_back(ca6P);
  e4P->fill("E4", "T", "false");
  ContextAttribute* ca7P = new ContextAttribute("A7", "T", "70");
  e4P->attributeVector.push_back(ca7P);
  req.entityVector.push_back(e1P);
  req.entityVector.push_back(e2P);
  req.entityVector.push_back(e3P);
  req.entityVector.push_back(e4P);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(4, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(2, RES_CER(0).attributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("T", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(0, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());

  EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
  EXPECT_EQ("T", RES_CER_ATTR(0, 1)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
  EXPECT_EQ("http://cpr2.com", RES_CER_ATTR(0, 1)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(0, 1)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());

  EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
  EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("action: UPDATE - entity: [E1, T] - offending attribute: A2", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E2", RES_CER(1).id);
  EXPECT_EQ("T", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  EXPECT_EQ(0, RES_CER(1).providingApplicationList.size());
  ASSERT_EQ(2, RES_CER(1).attributeVector.size());

  EXPECT_EQ("A4", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("T", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(1, 0)->stringValue.size());
  EXPECT_EQ("http://cpr3.com", RES_CER_ATTR(1, 0)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(1, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());

  EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
  EXPECT_EQ("T", RES_CER_ATTR(1, 1)->type);
  EXPECT_EQ(0, RES_CER_ATTR(1, 1)->stringValue.size());
  EXPECT_EQ("", RES_CER_ATTR(1, 1)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(1, 1)->found);
  EXPECT_EQ(0, RES_CER_ATTR(1, 1)->metadataVector.size());

  EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(1).code);
  EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ("action: UPDATE - entity: [E2, T] - offending attribute: A4", RES_CER_STATUS(1).details);

  /* Context Element response # 3 */
  EXPECT_EQ("E3", RES_CER(2).id);
  EXPECT_EQ("T", RES_CER(2).type);
  EXPECT_EQ("false", RES_CER(2).isPattern);
  EXPECT_EQ(0, RES_CER(2).providingApplicationList.size());
  ASSERT_EQ(2, RES_CER(2).attributeVector.size());

  EXPECT_EQ("A5", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("T", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(2, 0)->stringValue.size());
  EXPECT_EQ("http://cpr2.com", RES_CER_ATTR(2, 0)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(2, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(2, 0)->metadataVector.size());

  EXPECT_EQ("A6", RES_CER_ATTR(2, 1)->name);
  EXPECT_EQ("T", RES_CER_ATTR(2, 1)->type);
  EXPECT_EQ(0, RES_CER_ATTR(2, 1)->stringValue.size());
  EXPECT_EQ("", RES_CER_ATTR(2, 1)->providingApplication.get());
  EXPECT_FALSE(RES_CER_ATTR(2, 1)->found);
  EXPECT_EQ(0, RES_CER_ATTR(2, 1)->metadataVector.size());

  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(2).details);

  /* Context Element response # 4 */
  EXPECT_EQ("E4", RES_CER(3).id);
  EXPECT_EQ("T", RES_CER(3).type);
  EXPECT_EQ("false", RES_CER(3).isPattern);
  EXPECT_EQ(0, RES_CER(3).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(3).attributeVector.size());

  EXPECT_EQ("A7", RES_CER_ATTR(3, 0)->name);
  EXPECT_EQ("T", RES_CER_ATTR(3, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(3, 0)->stringValue.size());
  EXPECT_EQ("http://cpr1.com", RES_CER_ATTR(3, 0)->providingApplication.get());
  EXPECT_TRUE(RES_CER_ATTR(3, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(3, 0)->metadataVector.size());

  EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
  EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(3).details);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* entities collection */
  BSONObj ent, attrs;
  std::vector<BSONElement> attrNames;
  ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(1360232700, ent.getIntField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(1, attrs.nFields());
  ASSERT_EQ(1, attrNames.size());
  BSONObj a1 = attrs.getField("A1").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_STREQ("T", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("10", C_STR_FIELD(a1, "value"));
  EXPECT_EQ(1360232700, a1.getIntField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T"));
  EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(1360232700, ent.getIntField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(1, attrs.nFields());
  ASSERT_EQ(1, attrNames.size());
  BSONObj a3 = attrs.getField("A3").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A3"));
  EXPECT_STREQ("T", C_STR_FIELD(a3, "type"));
  EXPECT_STREQ("30", C_STR_FIELD(a3, "value"));
  EXPECT_EQ(1360232700, a3.getIntField("modDate"));

  utExit();
}



/* ****************************************************************************
*
* notFoundUpdate -
*
*/
TEST(mongoContextProvidersUpdateRequest, notFoundUpdate)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  utInit();
  /* empty database, no preparation step */

  /* Forge the request (from "inside" to "outside") */
  Entity* eP = new Entity();
  eP->fill("E1", "T1", "false");
  ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
  eP->attributeVector.push_back(caP);
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("T1", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_FALSE(RES_CER_ATTR(0, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());

  EXPECT_EQ(SccContextElementNotFound, RES_CER_STATUS(0).code);
  EXPECT_EQ("No context element found", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}

/* ****************************************************************************
*
* notFoundDelete -
*
*/
TEST(mongoContextProvidersUpdateRequest, notFoundDelete)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  utInit();
  /* empty database, no preparation step */

  /* Forge the request (from "inside" to "outside") */
  Entity* eP = new Entity();
  eP->fill("E1", "T1", "false");
  ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
  eP->attributeVector.push_back(caP);
  req.entityVector.push_back(eP);
  req.updateActionType = ActionTypeDelete;

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("T1", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  EXPECT_EQ(0, RES_CER(0).providingApplicationList.size());
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());

  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
  EXPECT_EQ("", RES_CER_ATTR(0, 0)->providingApplication.get());
  EXPECT_FALSE(RES_CER_ATTR(0, 0)->found);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());

  EXPECT_EQ(SccContextElementNotFound, RES_CER_STATUS(0).code);
  EXPECT_EQ("No context element found", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}
