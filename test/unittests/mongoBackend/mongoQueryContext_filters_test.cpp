/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "unittests/unittest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "orionTypes/OrionValueType.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoConnectionPool.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/EntityId.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"

#include "mongo/client/dbclient.h"



/* ****************************************************************************
*
* USING
*/
using mongo::DBClientBase;
using mongo::BSONObj;



extern void setMongoConnectionForUnitTest(DBClientBase* _connection);



/* ****************************************************************************
*
* Basic test cases
*
* - equalToOne_s
* - equalToOne_n
* - equalToOne_d
* - equalToMulti_s
* - equalToMulti_n
* - equalToMulti_d
* - unequalToOne_s
* - unequalToOne_n
* - unequalToOne_d
* - unequalToMany_s
* - unequalToMany_n
* - unequalToMany_d
* - greaterThan_n
* - greaterThan_d
* - greaterThanOrEqual_n
* - greaterThanOrEqual_d
* - lessThan_n
* - lessThan_d
* - lessThanOrEqual_n
* - lessThanOrEqual_d
* - insideRange_n
* - insideRange_d
* - outsideRange_n
* - outsideRange_d
* - withAttribute
* - withoutAttribute
*
* Special test cases
*
* - stringsWithCommas
* - cobingingSeveralFilters
* - repeatSameFilter
* - rangeWithDecimals
* - numberAsString
* - dateAsString
*
* The prefix _s, _n and _d means string, number or date (in the cases distinguishing makes sense)
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
* entities collection.
*/
static void prepareDatabase(bool extraEntities = false)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following entities:
   *
   * - E1
   *     S: "running"
   *     N: 26.5
   *     D: date 2019-07-25T10:00:00Z
   * - E2
   *     S: "running"
   *     N: 27
   *     D: date 2019-07-25T11:00:00Z
   * - E3
   *     S: "shutdown"
   *     N: 31
   *     D: date 2019-07-25T12:00:00Z
   * - E4
   *     S: "error"
   *     N: 17.8
   *     D: date 2019-07-25T13:00:00Z
   * - E5
   *     S: "shutdown"
   *     N: 24
   *     D: date 2019-07-25T14:00:00Z
   * - C1
   *     colour=black,white
   * - C2
   *     colour=red,blue
   * - C3
   *     colour=red, blue
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("S" << "N" << "D") <<
                     "attrs" << BSON(
                       "S" << BSON("type" << "T" << "value" << "running") <<
                       "N" << BSON("type" << "T" << "value" << 26.5) <<
                       "D" << BSON("type" << "T" << "value" << 1564048800)));  // 2019-07-25T10:00:00Z

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("S" << "N" << "D") <<
                     "attrs" << BSON(
                       "S" << BSON("type" << "T" << "value" << "running") <<
                       "N" << BSON("type" << "T" << "value" << 27) <<
                       "D" << BSON("type" << "T" << "value" << 1564052400)));  // 2019-07-25T11:00:00Z

  BSONObj en3 = BSON("_id" << BSON("id" << "E3" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("S" << "N" << "D") <<
                     "attrs" << BSON(
                       "S" << BSON("type" << "T" << "value" << "shutdown") <<
                       "N" << BSON("type" << "T" << "value" << 31) <<
                       "D" << BSON("type" << "T" << "value" << 1564056000)));  // 2019-07-25T12:00:00Z

  BSONObj en4 = BSON("_id" << BSON("id" << "E4" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("S" << "N" << "D") <<
                     "attrs" << BSON(
                       "S" << BSON("type" << "T" << "value" << "error") <<
                       "N" << BSON("type" << "T" << "value" << 17.8) <<
                       "D" << BSON("type" << "T" << "value" << 1564059600)));  // 2019-07-25T13:00:00Z

  BSONObj en5 = BSON("_id" << BSON("id" << "E5" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("S" << "N" << "D") <<
                     "attrs" << BSON(
                       "S" << BSON("type" << "T" << "value" << "shutdown") <<
                       "N" << BSON("type" << "T" << "value" << 24) <<
                       "D" << BSON("type" << "T" << "value" << 1564063200)));  // 2019-07-25T14:00:00Z

  BSONObj c1 = BSON("_id" << BSON("id" << "C1" << "type" << "T") <<
                    "attrNames" << BSON_ARRAY("colour") <<
                    "attrs" << BSON(
                      "colour" << BSON("type" << "T" << "value" << "black,white")));

  BSONObj c2 = BSON("_id" << BSON("id" << "C2" << "type" << "T") <<
                    "attrNames" << BSON_ARRAY("colour") <<
                    "attrs" << BSON(
                      "colour" << BSON("type" << "T" << "value" << "red,blue")));

  BSONObj c3 = BSON("_id" << BSON("id" << "C3" << "type" << "T") <<
                    "attrNames" << BSON_ARRAY("colour") <<
                    "attrs" << BSON(
                      "colour" << BSON("type" << "T" << "value" << "black, white")));

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en3);
  connection->insert(ENTITIES_COLL, en4);
  connection->insert(ENTITIES_COLL, en5);
  connection->insert(ENTITIES_COLL, c1);
  connection->insert(ENTITIES_COLL, c2);
  connection->insert(ENTITIES_COLL, c3);

  if (extraEntities)
  {
    /* Adding additional entities, used in some tests
     * - E6
     *     N: 26.5
     * - E7 (no type, "")
     *     N: 27
     * - E8 (no type, <none>)
     *     N: 27
     */

    BSONObj en6 = BSON("_id" << BSON("id" << "E6" << "type" << "T") <<
                       "attrNames" << BSON_ARRAY("N") <<
                       "attrs" << BSON(
                         "N" << BSON("type" << "T" << "value" << 26.5)));

    BSONObj en7 = BSON("_id" << BSON("id" << "E7" << "type" << "") <<
                       "attrNames" << BSON_ARRAY("N") <<
                       "attrs" << BSON(
                         "N" << BSON("type" << "T" << "value" << 27)));

    BSONObj en8 = BSON("_id" << BSON("id" << "E8") <<
                       "attrNames" << BSON_ARRAY("N") <<
                       "attrs" << BSON(
                         "N" << BSON("type" << "T" << "value" << 27)));

    connection->insert(ENTITIES_COLL, en6);
    connection->insert(ENTITIES_COLL, en7);
    connection->insert(ENTITIES_COLL, en8);
  }
}

/* ****************************************************************************
*
* prepareDatabaseAsString -
*
* Similar to prepareDatabase, to be used by the "date/number as string" tests
*/
static void prepareDatabaseAsString(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following entities:
   *
   * - E1
   *     N: "26.5"
   *     D: "2019-07-25T10:00:00Z"
   * - E2
   *     N: 26.5
   *     D: date 2019-07-25T10:00:00Z
   * - E3
   *     N: "31"
   *     D: "2019-07-25T12:00:00Z"
   * - E4
   *     N: 31
   *     D: date 2019-07-25T12:00:00Z
   * - C1
   *     colour=black,white
   * - C2
   *     colour=red,blue
   * - C3
   *     colour=red, blue
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("N" << "D") <<
                     "attrs" << BSON(
                       "N" << BSON("type" << "T" << "value" << "26.5") <<
                       "D" << BSON("type" << "T" << "value" << "2019-07-25T10:00:00Z")));

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("N" << "D") <<
                     "attrs" << BSON(
                       "N" << BSON("type" << "T" << "value" << 26.5) <<
                       "D" << BSON("type" << "T" << "value" << 1564048800)));  // 2019-07-25T10:00:00Z

  BSONObj en3 = BSON("_id" << BSON("id" << "E3" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("N" << "D") <<
                     "attrs" << BSON(
                       "N" << BSON("type" << "T" << "value" << "31") <<
                       "D" << BSON("type" << "T" << "value" << "2019-07-25T12:00:00Z")));  // 2019-07-25T12:00:00Z

  BSONObj en4 = BSON("_id" << BSON("id" << "E4" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("N" << "D") <<
                     "attrs" << BSON(
                       "N" << BSON("type" << "T" << "value" << 31) <<
                       "D" << BSON("type" << "T" << "value" << 1564056000)));  // 2019-07-25T12:00:00Z

  BSONObj c1 = BSON("_id" << BSON("id" << "C1" << "type" << "T") <<
                    "attrNames" << BSON_ARRAY("colour") <<
                    "attrs" << BSON(
                      "colour" << BSON("type" << "T" << "value" << "black,white")));

  BSONObj c2 = BSON("_id" << BSON("id" << "C2" << "type" << "T") <<
                    "attrNames" << BSON_ARRAY("colour") <<
                    "attrs" << BSON(
                      "colour" << BSON("type" << "T" << "value" << "red,blue")));

  BSONObj c3 = BSON("_id" << BSON("id" << "C3" << "type" << "T") <<
                    "attrNames" << BSON_ARRAY("colour") <<
                    "attrs" << BSON(
                      "colour" << BSON("type" << "T" << "value" << "black, white")));

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en3);
  connection->insert(ENTITIES_COLL, en4);
  connection->insert(ENTITIES_COLL, c1);
  connection->insert(ENTITIES_COLL, c2);
  connection->insert(ENTITIES_COLL, c3);
}



/* ****************************************************************************
*
* equalToOne_s -
*
*/
TEST(mongoQueryContextRequest_filters, equalToOne_s)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId     en(".*", "T", "true");
    Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "S==running");
    std::string  errorString;
    bool         b;

    sc.stringFilterP = new StringFilter(SftQ);
    b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
    EXPECT_EQ("", errorString);
    EXPECT_EQ(true, b);

    req.entityIdVector.push_back(&en);
    req.restriction.scopeVector.push_back(&sc);
    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
    ASSERT_EQ(2, res.contextElementResponseVector.size());
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("E2", RES_CER(1).id);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* equalToOne_n -
*
*/
TEST(mongoQueryContextRequest_filters, equalToOne_n)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "N==27");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(1, res.contextElementResponseVector.size());
  EXPECT_EQ("E2", RES_CER(0).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* equalToOne_d -
*
*/
TEST(mongoQueryContextRequest_filters, equalToOne_d)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "D==2019-07-25T12:00:00Z");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(1, res.contextElementResponseVector.size());
  EXPECT_EQ("E3", RES_CER(0).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* equalToMulti_s -
*
*/
TEST(mongoQueryContextRequest_filters, equalToMulti_s)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "S==running,error");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("E2", RES_CER(1).id);
  EXPECT_EQ("E4", RES_CER(2).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* equalToMulti_n -
*
*/
TEST(mongoQueryContextRequest_filters, equalToMulti_n)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "N==31,17.8,22");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E3", RES_CER(0).id);
  EXPECT_EQ("E4", RES_CER(1).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* equalToMulti_d -
*
*/
TEST(mongoQueryContextRequest_filters, equalToMulti_d)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "D==2019-07-25T11:00:00Z,2019-07-25T13:00:00Z");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E2", RES_CER(0).id);
  EXPECT_EQ("E4", RES_CER(1).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* unequalToOne_s -
*
*/
TEST(mongoQueryContextRequest_filters, unequalToOne_s)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "S!=running");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E3", RES_CER(0).id);
  EXPECT_EQ("E4", RES_CER(1).id);
  EXPECT_EQ("E5", RES_CER(2).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* unequalToOne_n -
*
*/
TEST(mongoQueryContextRequest_filters, unequalToOne_n)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "N!=31");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(4, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("E2", RES_CER(1).id);
  EXPECT_EQ("E4", RES_CER(2).id);
  EXPECT_EQ("E5", RES_CER(3).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* unequalToOne_d -
*
*/
TEST(mongoQueryContextRequest_filters, unequalToOne_d)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "D!=2019-07-25T12:00:00Z");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(4, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("E2", RES_CER(1).id);
  EXPECT_EQ("E4", RES_CER(2).id);
  EXPECT_EQ("E5", RES_CER(3).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* unequalToMany_s -
*
*/
TEST(mongoQueryContextRequest_filters, unequalToMany_s)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "S!=running,error");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E3", RES_CER(0).id);
  EXPECT_EQ("E5", RES_CER(1).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* unequalToMany_n -
*
*/
TEST(mongoQueryContextRequest_filters, unequalToMany_n)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "N!=24,26.5,28");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E2", RES_CER(0).id);
  EXPECT_EQ("E3", RES_CER(1).id);
  EXPECT_EQ("E4", RES_CER(2).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* unequalToMany_d -
*
*/
TEST(mongoQueryContextRequest_filters, unequalToMany_d)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "D!=2019-07-25T10:00:00Z,2019-07-25T14:00:00Z");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E2", RES_CER(0).id);
  EXPECT_EQ("E3", RES_CER(1).id);
  EXPECT_EQ("E4", RES_CER(2).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* greaterThan_n -
*
*/
TEST(mongoQueryContextRequest_filters, greaterThan_n)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "N>26");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("E2", RES_CER(1).id);
  EXPECT_EQ("E3", RES_CER(2).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* greaterThan_d -
*
*/
TEST(mongoQueryContextRequest_filters, greaterThan_d)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "D>2019-07-25T11:00:00Z");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E3", RES_CER(0).id);
  EXPECT_EQ("E4", RES_CER(1).id);
  EXPECT_EQ("E5", RES_CER(2).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* greaterThanOrEqual_n -
*
*/
TEST(mongoQueryContextRequest_filters, greaterThanOrEqual_n)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "N>=27");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E2", RES_CER(0).id);
  EXPECT_EQ("E3", RES_CER(1).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* greaterThanOrEqual_d -
*
*/
TEST(mongoQueryContextRequest_filters, greaterThanOrEqual_d)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "D>=2019-07-25T13:00:00Z");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E4", RES_CER(0).id);
  EXPECT_EQ("E5", RES_CER(1).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* lessThan_n -
*
*/
TEST(mongoQueryContextRequest_filters, lessThan_n)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "N<27");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("E4", RES_CER(1).id);
  EXPECT_EQ("E5", RES_CER(2).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* lessThan_d-
*
*/
TEST(mongoQueryContextRequest_filters, lessThan_d)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "D<2019-07-25T13:00:00Z");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("E2", RES_CER(1).id);
  EXPECT_EQ("E3", RES_CER(2).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* lessThanOrEqual_n -
*
*/
TEST(mongoQueryContextRequest_filters, lessThanOrEqual_n)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "N<=24");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E4", RES_CER(0).id);
  EXPECT_EQ("E5", RES_CER(1).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* lessThanOrEqual_d -
*
*/
TEST(mongoQueryContextRequest_filters, lessThanOrEqual_d)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "D<=2019-07-25T11:00:00Z");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("E2", RES_CER(1).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* insideRange_n -
*
*/
TEST(mongoQueryContextRequest_filters, insideRange_n)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "N==17..24");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E4", RES_CER(0).id);
  EXPECT_EQ("E5", RES_CER(1).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* insideRange_d -
*
*/
TEST(mongoQueryContextRequest_filters, insideRange_d)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "D==2019-07-25T11:00:00Z..2019-07-25T12:00:00Z");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E2", RES_CER(0).id);
  EXPECT_EQ("E3", RES_CER(1).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* outsideRange_n -
*
*/
TEST(mongoQueryContextRequest_filters, outsideRange_n)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "N!=17..24");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("E2", RES_CER(1).id);
  EXPECT_EQ("E3", RES_CER(2).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* outsideRange_d -
*
*/
TEST(mongoQueryContextRequest_filters, outsideRange_d)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "D!=2019-07-25T11:00:00Z..2019-07-25T12:00:00Z");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("E4", RES_CER(1).id);
  EXPECT_EQ("E5", RES_CER(2).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* withAttribute -
*
*/
TEST(mongoQueryContextRequest_filters, withAttribute)
{
  utInit();

  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  prepareDatabase(true);

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "S");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(5, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("E2", RES_CER(1).id);
  EXPECT_EQ("E3", RES_CER(2).id);
  EXPECT_EQ("E4", RES_CER(3).id);
  EXPECT_EQ("E5", RES_CER(4).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* withoutAttribute -
*
*/
TEST(mongoQueryContextRequest_filters, withoutAttribute)
{
  utInit();

  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  prepareDatabase(true);

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "!S");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(6, res.contextElementResponseVector.size());
  EXPECT_EQ("C1", RES_CER(0).id);
  EXPECT_EQ("C2", RES_CER(1).id);
  EXPECT_EQ("C3", RES_CER(2).id);
  EXPECT_EQ("E6", RES_CER(3).id);
  EXPECT_EQ("E7", RES_CER(4).id);
  EXPECT_EQ("E8", RES_CER(5).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}



/* ****************************************************************************
*
* stringsWithCommas -
*/
TEST(mongoQueryContextRequest_filters, stringsWithCommas)
{
  utInit();

  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "colour=='black,white','red,blue'");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("C1", RES_CER(0).id);
  EXPECT_EQ("C2", RES_CER(1).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}



/* ****************************************************************************
*
* combiningSeveralFilters -
*
*/
TEST(mongoQueryContextRequest_filters, combiningSeveralFilters)
{
  utInit();

  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "S==running;N<27");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(1, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* repeatSameFilter -
*
*/
TEST(mongoQueryContextRequest_filters, repeatSameFilter)
{
  utInit();

  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "N>=17;N<=24");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E4", RES_CER(0).id);
  EXPECT_EQ("E5", RES_CER(1).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* rangeWithDecimals -
*
*/
TEST(mongoQueryContextRequest_filters, rangeWithDecimals)
{
  utInit();

  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "N==16.99..24.1");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E4", RES_CER(0).id);
  EXPECT_EQ("E5", RES_CER(1).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* numberAsString -
*
*/
TEST(mongoQueryContextRequest_filters, numberAsString)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabaseAsString();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "N=='26.5'");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);
  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(1, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

/* ****************************************************************************
*
* numberAsString -
*
*/
TEST(mongoQueryContextRequest_filters, dateAsString)
{
  HttpStatusCode         ms;
  QueryContextRequest   req;
  QueryContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabaseAsString();

  /* Forge the request (from "inside" to "outside") */
  EntityId     en(".*", "T", "true");
  Scope        sc(SCOPE_TYPE_SIMPLE_QUERY, "D=='2019-07-25T12:00:00Z'");
  std::string  errorString;
  bool         b;

  sc.stringFilterP = new StringFilter(SftQ);
  b = sc.stringFilterP->parse(sc.value.c_str(), &errorString);
  EXPECT_EQ("", errorString);
  EXPECT_EQ(true, b);

  req.entityIdVector.push_back(&en);
  req.restriction.scopeVector.push_back(&sc);
  /* Invoke the function in mongoBackend library */
  servicePathVector.clear();
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(1, res.contextElementResponseVector.size());
  EXPECT_EQ("E3", RES_CER(0).id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}
