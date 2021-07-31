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

#include "orionld/common/tenantList.h"     // tenant0

#include "common/globals.h"
#include "orionTypes/OrionValueType.h"
#include "mongoBackend/MongoGlobal.h"
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
*
* FIXME 5: missing tests (see https://github.com/telefonicaid/fiware-orion/issues/1129)
* - Number as strings, eg. '23'
* - Dates as string
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
   *     D: ISODate("2017-06-17T07:12:25.823Z")
   * - E2
   *     S: "running"
   *     N: 27
   *     D: ISODate("2017-06-17T07:21:24.238Z")
   * - E3
   *     S: "shutdown"
   *     N: 31
   *     D: ISODate("2017-06-17T08:19:12.231Z")
   * - E4
   *     S: "error"
   *     N: 17.8
   *     D: ISODate("2017-06-17T07:22:43.112Z")
   * - E5
   *     S: "shutdown"
   *     N: 24
   *     D: ISODate("2017-06-17T07:10:12.328Z")
   * - C1
   *     colour=black,white
   * - C2
   *     colour=red,blue
   * - C3
   *     colour=red, blue
   */

  // FIXME: D will be set with dates once https://github.com/telefonicaid/fiware-orion/issues/1039 implementation
  // gets addressed

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("S" << "N" << "D") <<
                     "attrs" << BSON(
                       "S" << BSON("type" << "T" << "value" << "running") <<
                       "N" << BSON("type" << "T" << "value" << 26.5) <<
                       "D" << BSON("type" << "T" << "value" << "")));

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("S" << "N" << "D") <<
                     "attrs" << BSON(
                       "S" << BSON("type" << "T" << "value" << "running") <<
                       "N" << BSON("type" << "T" << "value" << 27) <<
                       "D" << BSON("type" << "T" << "value" << "")));

  BSONObj en3 = BSON("_id" << BSON("id" << "E3" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("S" << "N" << "D") <<
                     "attrs" << BSON(
                       "S" << BSON("type" << "T" << "value" << "shutdown") <<
                       "N" << BSON("type" << "T" << "value" << 31) <<
                       "D" << BSON("type" << "T" << "value" << "")));

  BSONObj en4 = BSON("_id" << BSON("id" << "E4" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("S" << "N" << "D") <<
                     "attrs" << BSON(
                       "S" << BSON("type" << "T" << "value" << "error") <<
                       "N" << BSON("type" << "T" << "value" << 17.8) <<
                       "D" << BSON("type" << "T" << "value" << "")));

  BSONObj en5 = BSON("_id" << BSON("id" << "E5" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("S" << "N" << "D") <<
                     "attrs" << BSON(
                       "S" << BSON("type" << "T" << "value" << "shutdown") <<
                       "N" << BSON("type" << "T" << "value" << 24) <<
                       "D" << BSON("type" << "T" << "value" << "")));

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

    // FIXME: D will be set with dates once https://github.com/telefonicaid/fiware-orion/issues/1039 implementation
    // gets addressed

    BSONObj en6 = BSON("_id" << BSON("id" << "E6" << "type" << "T") <<
                       "attrNames" << BSON_ARRAY("S" << "N") <<
                       "attrs" << BSON(
                         "N" << BSON("type" << "T" << "value" << 26.5)));

    BSONObj en7 = BSON("_id" << BSON("id" << "E7" << "type" << "") <<
                       "attrNames" << BSON_ARRAY("S" << "N") <<
                       "attrs" << BSON(
                         "N" << BSON("type" << "T" << "value" << 27)));

    BSONObj en8 = BSON("_id" << BSON("id" << "E8") <<
                       "attrNames" << BSON_ARRAY("S" << "N") <<
                       "attrs" << BSON(
                         "N" << BSON("type" << "T" << "value" << 27)));

    connection->insert(ENTITIES_COLL, en6);
    connection->insert(ENTITIES_COLL, en7);
    connection->insert(ENTITIES_COLL, en8);
  }
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
    ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
    ASSERT_EQ(2, res.contextElementResponseVector.size());
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("E2", RES_CER(1).entityId.id);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(1, res.contextElementResponseVector.size());
  EXPECT_EQ("E2", RES_CER(0).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();
}

/* ****************************************************************************
*
* equalToOne_d -
*
*/
TEST(mongoQueryContextRequest_filters, DISABLED_equalToOne_d)
{
  // FIXME to be completed during https://github.com/telefonicaid/fiware-orion/issues/1039 implementation
  EXPECT_EQ(1, 2);
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("E2", RES_CER(1).entityId.id);
  EXPECT_EQ("E4", RES_CER(2).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E3", RES_CER(0).entityId.id);
  EXPECT_EQ("E4", RES_CER(1).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();
}

/* ****************************************************************************
*
* equalToMulti_d -
*
*/
TEST(mongoQueryContextRequest_filters, DISABLED_equalToMulti_d)
{
  // FIXME to be completed during https://github.com/telefonicaid/fiware-orion/issues/1039 implementation
  EXPECT_EQ(1, 2);
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E3", RES_CER(0).entityId.id);
  EXPECT_EQ("E4", RES_CER(1).entityId.id);
  EXPECT_EQ("E5", RES_CER(2).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(4, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("E2", RES_CER(1).entityId.id);
  EXPECT_EQ("E4", RES_CER(2).entityId.id);
  EXPECT_EQ("E5", RES_CER(3).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();
}

/* ****************************************************************************
*
* unequalToOne_d -
*
*/
TEST(mongoQueryContextRequest_filters, DISABLED_unequalToOne_d)
{
  // FIXME to be completed during https://github.com/telefonicaid/fiware-orion/issues/1039 implementation
  EXPECT_EQ(1, 2);
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E3", RES_CER(0).entityId.id);
  EXPECT_EQ("E5", RES_CER(1).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E2", RES_CER(0).entityId.id);
  EXPECT_EQ("E3", RES_CER(1).entityId.id);
  EXPECT_EQ("E4", RES_CER(2).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();
}

/* ****************************************************************************
*
* unequalToMany_d -
*
*/
TEST(mongoQueryContextRequest_filters, DISABLED_unequalToMany_d)
{
  // FIXME to be completed during https://github.com/telefonicaid/fiware-orion/issues/1039 implementation
  EXPECT_EQ(1, 2);
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("E2", RES_CER(1).entityId.id);
  EXPECT_EQ("E3", RES_CER(2).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();
}

/* ****************************************************************************
*
* greaterThan_d -
*
*/
TEST(mongoQueryContextRequest_filters, DISABLED_greaterThan_d)
{
  // FIXME to be completed during https://github.com/telefonicaid/fiware-orion/issues/1039 implementation
  EXPECT_EQ(1, 2);
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E2", RES_CER(0).entityId.id);
  EXPECT_EQ("E3", RES_CER(1).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();
}

/* ****************************************************************************
*
* greaterThanOrEqual_d -
*
*/
TEST(mongoQueryContextRequest_filters, DISABLED_greaterThanOrEqual_d)
{
  // FIXME to be completed during https://github.com/telefonicaid/fiware-orion/issues/1039 implementation
  EXPECT_EQ(1, 2);
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("E4", RES_CER(1).entityId.id);
  EXPECT_EQ("E5", RES_CER(2).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();
}

/* ****************************************************************************
*
* lessThan_d-
*
*/
TEST(mongoQueryContextRequest_filters, DISABLED_lessThan_d)
{
  // FIXME to be completed during https://github.com/telefonicaid/fiware-orion/issues/1039 implementation
  EXPECT_EQ(1, 2);
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E4", RES_CER(0).entityId.id);
  EXPECT_EQ("E5", RES_CER(1).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();
}

/* ****************************************************************************
*
* lessThanOrEqual_d -
*
*/
TEST(mongoQueryContextRequest_filters, DISABLED_lessThanOrEqual_d)
{
  // FIXME to be completed during https://github.com/telefonicaid/fiware-orion/issues/1039 implementation
  EXPECT_EQ(1, 2);
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E4", RES_CER(0).entityId.id);
  EXPECT_EQ("E5", RES_CER(1).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();
}

/* ****************************************************************************
*
* insideRange_d -
*
*/
TEST(mongoQueryContextRequest_filters, DISABLED_insideRange_d)
{
  // FIXME to be completed during https://github.com/telefonicaid/fiware-orion/issues/1039 implementation
  EXPECT_EQ(1, 2);
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(3, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("E2", RES_CER(1).entityId.id);
  EXPECT_EQ("E3", RES_CER(2).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();
}

/* ****************************************************************************
*
* outsideRange_d -
*
*/
TEST(mongoQueryContextRequest_filters, DISABLED_outsideRange_d)
{
  // FIXME to be completed during https://github.com/telefonicaid/fiware-orion/issues/1039 implementation
  EXPECT_EQ(1, 2);
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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(5, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("E2", RES_CER(1).entityId.id);
  EXPECT_EQ("E3", RES_CER(2).entityId.id);
  EXPECT_EQ("E4", RES_CER(3).entityId.id);
  EXPECT_EQ("E5", RES_CER(4).entityId.id);

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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(6, res.contextElementResponseVector.size());
  EXPECT_EQ("C1", RES_CER(0).entityId.id);
  EXPECT_EQ("C2", RES_CER(1).entityId.id);
  EXPECT_EQ("C3", RES_CER(2).entityId.id);
  EXPECT_EQ("E6", RES_CER(3).entityId.id);
  EXPECT_EQ("E7", RES_CER(4).entityId.id);
  EXPECT_EQ("E8", RES_CER(5).entityId.id);

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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("C1", RES_CER(0).entityId.id);
  EXPECT_EQ("C2", RES_CER(1).entityId.id);

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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(1, res.contextElementResponseVector.size());
  EXPECT_EQ("E1", RES_CER(0).entityId.id);

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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E4", RES_CER(0).entityId.id);
  EXPECT_EQ("E5", RES_CER(1).entityId.id);

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
  ms = mongoQueryContext(&req, &res, &tenant0, servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  /* Only entity IDs are checked (we have a bunch of tests in other places to check the query response itself) */
  ASSERT_EQ(2, res.contextElementResponseVector.size());
  EXPECT_EQ("E4", RES_CER(0).entityId.id);
  EXPECT_EQ("E5", RES_CER(1).entityId.id);

  /* Release dynamic memory used by response (mongoBackend allocates it) */
  res.contextElementResponseVector.release();

  utExit();
}

