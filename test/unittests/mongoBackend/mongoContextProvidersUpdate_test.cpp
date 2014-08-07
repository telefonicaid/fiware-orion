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
* fermin at tid dot es
*
* Author: Fermin Galan
*/
#include "unittest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi/StatusCode.h"
#include "ngsi/EntityId.h"
#include "ngsi/Scope.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"

#include "mongo/client/dbclient.h"

/* ****************************************************************************
*
* Tests
*
* These test are based on the ones in the mongoDiscoverContextAvailability_test.cpp file.
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
* Note these tests are not "canonical" unit tests. Canon says that in this case we should have
* mocked MongoDB. Actually, we think is very much powerfull to check that everything is ok at
* MongoDB layer.
*
*/

#define NO_CODE 0

/* ****************************************************************************
*
* prepareDatabase -
*
* This function is called before every test, to populate some information in the
* registrations collection.
*/
static void prepareDatabase(void) {

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
   *     to make them equat to the ones in Reg1 and Reg2 and check that some tests are failing.
   * (**)same name but without type
   */

  BSONObj cr1 = BSON("providingApplication" << "http://cr1.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E1" << "type" << "T1") <<
                         BSON("id" << "E2" << "type" << "T2") <<
                         BSON("id" << "E3" << "type" << "T3")
                         ) <<
                     "attrs" << BSON_ARRAY(
                         BSON("name" << "A1" << "type" << "TA1" << "isDomain" << "true") <<
                         BSON("name" << "A2" << "type" << "TA2" << "isDomain" << "false") <<
                         BSON("name" << "A3" << "type" << "TA3" << "isDomain" << "true")
                         )
                     );
  BSONObj cr2 = BSON("providingApplication" << "http://cr2.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E1" << "type" << "T1")
                         ) <<
                     "attrs" << BSON_ARRAY(
                         BSON("name" << "A1" << "type" << "TA1" << "isDomain" << "true") <<
                         BSON("name" << "A4" << "type" << "TA4" << "isDomain" << "false")
                         )
                     );
  BSONObj cr3 = BSON("providingApplication" << "http://cr3.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E2" << "type" << "T2")
                         ) <<
                     "attrs" << BSON_ARRAY(
                         BSON("name" << "A2" << "type" << "TA2" << "isDomain" << "false") <<
                         BSON("name" << "A3" << "type" << "TA3" << "isDomain" << "true")
                         )
                     );

  BSONObj cr4 = BSON("providingApplication" << "http://cr4.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E1" << "type" << "T1bis")
                         ) <<
                     "attrs" << BSON_ARRAY(
                         BSON("name" << "A1" << "type" << "TA1bis" << "isDomain" << "false")
                         )
                     );

  BSONObj cr5 = BSON("providingApplication" << "http://cr5.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E1")
                         ) <<
                     "attrs" << BSON_ARRAY(
                         BSON("name" << "A1" << "type" << "TA1" << "isDomain" << "true")
                         )
                     );

  /* 1879048191 corresponds to year 2029 so we avoid any expiration problem in the next 16 years :) */
  BSONObj reg1 = BSON(
              "_id" << OID("51307b66f481db11bf860001") <<
              "expiration" << 1879048191 <<
              "contextRegistration" << BSON_ARRAY(cr1 << cr2)
              );

  BSONObj reg2 = BSON(
              "_id" << OID("51307b66f481db11bf860002") <<
              "expiration" << 1879048191 <<
              "contextRegistration" << BSON_ARRAY(cr3)
              );

  BSONObj reg3 = BSON(
              "_id" << OID("51307b66f481db11bf860003") <<
              "expiration" << 1879048191 <<
              "contextRegistration" << BSON_ARRAY(cr4)
              );

  BSONObj reg4 = BSON(
              "_id" << OID("51307b66f481db11bf860004") <<
              "expiration" << 1879048191 <<
              "contextRegistration" << BSON_ARRAY(cr5)
              );

  connection->insert(REGISTRATIONS_COLL, reg1);
  connection->insert(REGISTRATIONS_COLL, reg2);
  connection->insert(REGISTRATIONS_COLL, reg3);
  connection->insert(REGISTRATIONS_COLL, reg4);


}

#if 0
/* ****************************************************************************
*
* prepareDatabasePatternTrue -
*
* This is a variant of populateDatabase function in which all entities have the same type,
* to ease test for isPattern=true cases
*/
static void prepareDatabasePatternTrue(void) {

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
   *     to make them equat to the ones in Reg1 and Reg2 and check that some tests are failing.
   * (**)same name but without type
   */

  BSONObj cr1 = BSON("providingApplication" << "http://cr1.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E1" << "type" << "T") <<
                         BSON("id" << "E2" << "type" << "T") <<
                         BSON("id" << "E3" << "type" << "T")
                         ) <<
                     "attrs" << BSON_ARRAY(
                         BSON("name" << "A1" << "type" << "TA1" << "isDomain" << "true") <<
                         BSON("name" << "A2" << "type" << "TA2" << "isDomain" << "false") <<
                         BSON("name" << "A3" << "type" << "TA3" << "isDomain" << "true")
                         )
                     );
  BSONObj cr2 = BSON("providingApplication" << "http://cr2.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E1" << "type" << "T")
                         ) <<
                     "attrs" << BSON_ARRAY(
                         BSON("name" << "A1" << "type" << "TA1" << "isDomain" << "true") <<
                         BSON("name" << "A4" << "type" << "TA4" << "isDomain" << "false")
                         )
                     );
  BSONObj cr3 = BSON("providingApplication" << "http://cr3.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E2" << "type" << "T")
                         ) <<
                     "attrs" << BSON_ARRAY(
                         BSON("name" << "A2" << "type" << "TA2" << "isDomain" << "false") <<
                         BSON("name" << "A3" << "type" << "TA3" << "isDomain" << "true")
                         )
                     );

  BSONObj cr4 = BSON("providingApplication" << "http://cr4.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E2" << "type" << "Tbis")
                         ) <<
                     "attrs" << BSON_ARRAY(
                         BSON("name" << "A2" << "type" << "TA2bis" << "isDomain" << "false")
                         )
                     );

  BSONObj cr5 = BSON("providingApplication" << "http://cr5.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E3")
                         ) <<
                     "attrs" << BSON_ARRAY(
                         BSON("name" << "A2" << "type" << "TA2" << "isDomain" << "false")
                         )
                     );

  /* 1879048191 corresponds to year 2029 so we avoid any expiration problem in the next 16 years :) */
  BSONObj reg1 = BSON(
              "_id" << "ff37" <<
              "expiration" << 1879048191 <<
              "subscriptions" << BSONArray() <<
              "contextRegistration" << BSON_ARRAY(cr1 << cr2)
              );

  BSONObj reg2 = BSON(
              "_id" << "ff48" <<
              "expiration" << 1879048191 <<
              "subscriptions" << BSONArray() <<
              "contextRegistration" << BSON_ARRAY(cr3)
              );

  BSONObj reg3 = BSON(
              "_id" << "ff80" <<
              "expiration" << 1879048191 <<
              "subscriptions" << BSONArray() <<
              "contextRegistration" << BSON_ARRAY(cr4)
              );

  BSONObj reg4 = BSON(
              "_id" << "ff90" <<
              "expiration" << 1879048191 <<
              "subscriptions" << BSONArray() <<
              "contextRegistration" << BSON_ARRAY(cr5)
              );

  connection->insert(REGISTRATIONS_COLL, reg1);
  connection->insert(REGISTRATIONS_COLL, reg2);
  connection->insert(REGISTRATIONS_COLL, reg3);
  connection->insert(REGISTRATIONS_COLL, reg4);
}
#endif

/* ****************************************************************************
*
* noPatternAttrsAll -
*
* Discover:  E3 -no attrs
* Result:    E3 - (A1, A2, A3) - http://cr1.com
*/
TEST(mongoContextProvidersUpdateRequest, noPatternAttrsAll)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  utInit();
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  /* Note that although is a bit weird having a updateContext without attributes to update,
   * it is legal from the point of view of OMA spec */
  ContextElement ce;
  ce.entityId.fill("E3", "T3", "false");
  req.contextElementVector.push_back(&ce);
  req.updateActionType.set("UPDATE");

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(NO_CODE, res.errorCode.code);
  EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.errorCode.details.size());

  ASSERT_EQ(1,res.contextElementResponseVector.size());
  EXPECT_EQ("E3", RES_CER(0).entityId.id);
  EXPECT_EQ("T3", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(0, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ(SccFound, RES_CER_STATUS(0).code);
  EXPECT_EQ("Found", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("http://cr1.com", RES_CER_STATUS(0).details);

  /* Release connection */
  mongoDisconnect();

  utExit();
}

#if 0
/* ****************************************************************************
*
* noPatternAttrOneSingle -
*
* Discover:  E1 - A4
* Result:    E1 - A4 - http://cr2.com
*/
TEST(mongoContextProvidersUpdateRequest, noPatternAttrOneSingle)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr2.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, noPatternAttrOneMulti)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr1.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, noPatternAttrsSubset)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr1.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, noPatternSeveralCREs)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr1.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, noPatternSeveralRegistrations)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr1.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, noPatternNoEntity)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());
    EXPECT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, noPatternNoAttribute)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());
    EXPECT_EQ(0,res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, noPatternMultiEntity)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr1.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, noPatternMultiAttr)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr1.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, noPatternMultiEntityAttrs)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr1.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, noPatternNoType)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr1.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, pattern0Attr)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr1.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, pattern1AttrSingle)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr2.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, pattern1AttrMulti)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr1.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, patternNAttr)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr1.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, patternFail)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());
    EXPECT_EQ(0,res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, patternNoType)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr1.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

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
TEST(mongoContextProvidersUpdateRequest, mixPatternAndNotPattern)
{
    HttpStatusCode        ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccFound, res.errorCode.code);
    EXPECT_EQ("Found", res.errorCode.reasonPhrase);
    EXPECT_EQ("http://cr1.com", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

    /* Delete mock */
    delete timerMock;

}
#endif
