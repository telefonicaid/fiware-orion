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

#include "unittests/unittest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/EntityId.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"
#include "orionTypes/areas.h"

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
* Tests
*
*
* - queryGeoCircleIn1
* - queryGeoCircleIn2
* - queryGeoCircleOut
* - queryGeoPolygonIn1
* - queryGeoPolygonIn2
* - queryGeoPolygonIn3
* - queryGeoPolygonOut1
* - queryGeoPolygonOut2
*
*/

/* ****************************************************************************
*
* prepareDatabase -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabase(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  connection->createIndex("utest.entities", BSON("location.coords" << "2dsphere"));

  BSONObj A = BSON("_id" << BSON("id" << "A" << "type" << "Point") <<
                   "attrNames" << BSON_ARRAY("pos" << "foo") <<
                   "attrs" << BSON(
                     "pos" << BSON("type" << "location" << "value" << "2, 3") <<
                     "foo" << BSON("type" << "string" << "value" << "attr_A")) <<
                   "location" << BSON("attrName" << "pos" << "coords" << BSON_ARRAY(3.0 << 2.0)));

  BSONObj B = BSON("_id" << BSON("id" << "B" << "type" << "Point") <<
                   "attrNames" << BSON_ARRAY("pos" << "foo") <<
                   "attrs" << BSON(
                     "pos" << BSON("type" << "location" << "value" << "5, 5") <<
                     "foo" << BSON("type" << "string" << "value" << "attr_B")) <<
                   "location" << BSON("attrName" << "pos" << "coords" << BSON_ARRAY(5.0 << 5.0)));

  BSONObj C = BSON("_id" << BSON("id" << "C" << "type" << "Point") <<
                   "attrNames" << BSON_ARRAY("pos" << "foo") <<
                   "attrs" << BSON(
                     "pos" << BSON("type" << "location" << "value" << "4, 7") <<
                     "foo" << BSON("type" << "string" << "value" << "attr_C")) <<
                   "location" << BSON("attrName" << "pos" << "coords" << BSON_ARRAY(7.0 << 4.0)));

  // Entity D hasn't a location attribute (i.e. no location field). This entity will be never returned
  // when a geoscope is defined
  BSONObj D = BSON("_id" << BSON("id" << "D" << "type" << "Point") <<
                   "attrNames" << BSON_ARRAY("pos" << "foo") <<
                   "attrs" << BSON(
                     "pos" << BSON("type" << "location" << "value" << "4, 7") <<
                     "foo" << BSON("type" << "string" << "value" << "attr_C")));

  BSONObj city1 = BSON("_id" << BSON("id" << "Madrid" << "type" << "City") <<
                       "attrNames" << BSON_ARRAY("pos" << "foo") <<
                       "attrs" << BSON(
                         "pos" << BSON("type" << "location" << "value" << "40.418889, -3.691944") <<
                         "foo" << BSON("type" << "string" << "value" << "attr_Mad")) <<
                       "location" << BSON("attrName" << "pos" << "coords" << BSON_ARRAY(-3.691944 << 40.418889)));

  BSONObj city2 = BSON("_id" << BSON("id" << "Alcobendas" << "type" << "City") <<
                       "attrNames" << BSON_ARRAY("pos" << "foo") <<
                       "attrs" << BSON(
                         "pos" << BSON("type" << "location" << "value" << "40.533333, -3.633333") <<
                         "foo" << BSON("type" << "string" << "value" << "attr_Alc")) <<
                       "location" << BSON("attrName" << "pos" << "coords" << BSON_ARRAY(-3.633333 << 40.533333)));

  BSONObj city3 = BSON("_id" << BSON("id" << "Leganes" << "type" << "City") <<
                       "attrNames" << BSON_ARRAY("pos" << "foo") <<
                       "attrs" << BSON(
                         "pos" << BSON("type" << "location" << "value" << "40.316667, -3.75") <<
                         "foo" << BSON("type" << "string" << "value" << "attr_Leg")) <<
                       "location" << BSON("attrName" << "pos" << "coords" << BSON_ARRAY(-3.75 << 40.316667)));

  connection->insert(ENTITIES_COLL, A);
  connection->insert(ENTITIES_COLL, B);
  connection->insert(ENTITIES_COLL, C);
  connection->insert(ENTITIES_COLL, D);
  connection->insert(ENTITIES_COLL, city1);
  connection->insert(ENTITIES_COLL, city2);
  connection->insert(ENTITIES_COLL, city3);
}



/* ****************************************************************************
*
* getEntityIndex -
*
*/
int getEntityIndex(ContextElementResponseVector& v, const std::string& id)
{
  for (unsigned int ix = 0; ix < v.size(); ++ix)
  {
    if (v[ix]->entity.id == id)
    {
      return ix;
    }
  }

  return -1;
}



/* ****************************************************************************
*
* queryGeoCircleIn1 -
*
* Area:      14 km circle centered in Madrid
* Result:    Madrid, Alcobendas
*
*/
TEST(mongoQueryContextGeoRequest, queryGeoCircleIn1)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en(".*", "City", "true");
    req.entityIdVector.push_back(&en);

    /* Define area scope */
    Scope sc;
    sc.type = "FIWARE::Location";
    sc.areaType = orion::CircleType;
    sc.circle.center.latitudeSet("40.418889");
    sc.circle.center.longitudeSet("-3.691944");
    sc.circle.radiusSet("13600");
    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    int i;

    /* Context Element response # 1 */
    i = getEntityIndex(res.contextElementResponseVector, "Madrid");
    ASSERT_GE(i, 0);
    EXPECT_EQ("Madrid", RES_CER(i).id);
    EXPECT_EQ("City", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_Mad", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    EXPECT_EQ("40.418889, -3.691944", RES_CER_ATTR(i, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Context Element response # 2 */
    i = getEntityIndex(res.contextElementResponseVector, "Leganes");
    ASSERT_GE(i, 0);
    EXPECT_EQ("Leganes", RES_CER(i).id);
    EXPECT_EQ("City", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_Leg", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    EXPECT_EQ("40.316667, -3.75", RES_CER_ATTR(i, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryGeoCircleIn2 -
*
* Area:      15 km circle centered in Madrid
* Result:    Madrid, Alcobendas, Leganes
*
*/
TEST(mongoQueryContextGeoRequest, queryGeoCircleIn2)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en(".*", "City", "true");
    req.entityIdVector.push_back(&en);

    /* Define area scope */
    Scope sc;
    sc.type = "FIWARE::Location";
    sc.areaType = orion::CircleType;
    sc.circle.center.latitudeSet("40.418889");
    sc.circle.center.longitudeSet("-3.691944");
    sc.circle.radiusSet("15000");
    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    int i;
    /* Context Element response # 1 */

    i = getEntityIndex(res.contextElementResponseVector, "Madrid");
    ASSERT_GE(i, 0);
    EXPECT_EQ("Madrid", RES_CER(i).id);
    EXPECT_EQ("City", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_Mad", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    EXPECT_EQ("40.418889, -3.691944", RES_CER_ATTR(i, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Context Element response # 2 */
    i = getEntityIndex(res.contextElementResponseVector, "Alcobendas");
    ASSERT_GE(i, 0);
    EXPECT_EQ("Alcobendas", RES_CER(i).id);
    EXPECT_EQ("City", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_Alc", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    EXPECT_EQ("40.533333, -3.633333", RES_CER_ATTR(i, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Context Element response # 3 */
    i = getEntityIndex(res.contextElementResponseVector, "Leganes");
    ASSERT_GE(i, 0);
    EXPECT_EQ("Leganes", RES_CER(i).id);
    EXPECT_EQ("City", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_Leg", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    EXPECT_EQ("40.316667, -3.75", RES_CER_ATTR(i, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryGeoCircleOut -
*
* Area:      14 km circle centered in Madrid
* Result:    Leganes
*
*/
TEST(mongoQueryContextGeoRequest, queryGeoCircleOut)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en(".*", "City", "true");
    req.entityIdVector.push_back(&en);

    /* Define area scope */
    Scope sc;
    sc.type = "FIWARE::Location";
    sc.areaType = orion::CircleType;
    sc.circle.center.latitudeSet("40.418889");
    sc.circle.center.longitudeSet("-3.691944");
    sc.circle.radiusSet("13600");
    sc.circle.invertedSet("true");
    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_STREQ("", res.errorCode.reasonPhrase.c_str());
    EXPECT_STREQ("", res.errorCode.details.c_str());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    int i;

    /* Context Element response # 1 */
    i = getEntityIndex(res.contextElementResponseVector, "Alcobendas");
    ASSERT_GE(i, 0);
    EXPECT_EQ("Alcobendas", RES_CER(i).id);
    EXPECT_EQ("City", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_Alc", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    EXPECT_EQ("40.533333, -3.633333", RES_CER_ATTR(i, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryGeoPolygonIn1 -
*
* Area:      Square: [0,0], [0,6], [6,6], [6,0]
* Result:    A, B
*
*/
TEST(mongoQueryContextGeoRequest, queryGeoPolygonIn1)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en(".*", "Point", "true");
    req.entityIdVector.push_back(&en);

    /* Define area scope */
    Scope sc;
    sc.type = "FIWARE::Location";
    sc.areaType = orion::PolygonType;
    orion::Point p1, p2, p3, p4;

    p1.latitudeSet("0");
    p1.longitudeSet("0");
    sc.polygon.vertexList.push_back(&p1);

    p2.latitudeSet("0");
    p2.longitudeSet("6");
    sc.polygon.vertexList.push_back(&p2);

    p3.latitudeSet("6");
    p3.longitudeSet("6");
    sc.polygon.vertexList.push_back(&p3);

    p4.latitudeSet("6");
    p4.longitudeSet("0");
    sc.polygon.vertexList.push_back(&p4);

    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    int i;

    /* Context Element response # 1 */
    i = getEntityIndex(res.contextElementResponseVector, "A");
    ASSERT_GE(i, 0);
    EXPECT_EQ("A", RES_CER(i).id);
    EXPECT_EQ("Point", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_A", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    EXPECT_EQ("2, 3", RES_CER_ATTR(i, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Context Element response # 2 */
    i = getEntityIndex(res.contextElementResponseVector, "B");
    ASSERT_GE(i, 0);
    EXPECT_EQ("B", RES_CER(i).id);
    EXPECT_EQ("Point", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_B", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    EXPECT_EQ("5, 5", RES_CER_ATTR(i, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryGeoPolygonIn2 -
*
* Area:      Square: [3,8], [11,8], [11,3], [3,3]
* Result:    B, C
*
*/
TEST(mongoQueryContextGeoRequest, queryGeoPolygonIn2)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en(".*", "Point", "true");
    req.entityIdVector.push_back(&en);

    /* Define area scope */
    Scope sc;
    sc.type = "FIWARE::Location";
    sc.areaType = orion::PolygonType;
    orion::Point p1, p2, p3, p4;

    p1.latitudeSet("3");
    p1.longitudeSet("8");
    sc.polygon.vertexList.push_back(&p1);

    p2.latitudeSet("11");
    p2.longitudeSet("8");
    sc.polygon.vertexList.push_back(&p2);

    p3.latitudeSet("11");
    p3.longitudeSet("3");
    sc.polygon.vertexList.push_back(&p3);

    p4.latitudeSet("3");
    p4.longitudeSet("3");
    sc.polygon.vertexList.push_back(&p4);

    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    int i;

    /* Context Element response # 1 */
    i = getEntityIndex(res.contextElementResponseVector, "B");
    ASSERT_GE(i, 0);
    EXPECT_EQ("B", RES_CER(i).id);
    EXPECT_EQ("Point", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_B", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    EXPECT_EQ("5, 5", RES_CER_ATTR(i, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Context Element response # 2 */
    i = getEntityIndex(res.contextElementResponseVector, "C");
    ASSERT_GE(i, 0);
    EXPECT_EQ("C", RES_CER(i).id);
    EXPECT_EQ("Point", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_C", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ("4, 7", RES_CER_ATTR(i, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryGeoPolygonIn3 -
*
* Area:      Triangle: [0,0], [0,6], [6,0]
* Result:    A
*
*/
TEST(mongoQueryContextGeoRequest, queryGeoPolygonIn3)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en(".*", "Point", "true");
    req.entityIdVector.push_back(&en);

    /* Define area scope */
    Scope sc;
    sc.type = "FIWARE::Location";
    sc.areaType = orion::PolygonType;
    orion::Point p1, p2, p3;

    p1.latitudeSet("0");
    p1.longitudeSet("0");
    sc.polygon.vertexList.push_back(&p1);

    p2.latitudeSet("0");
    p2.longitudeSet("6");
    sc.polygon.vertexList.push_back(&p2);

    p3.latitudeSet("6");
    p3.longitudeSet("0");
    sc.polygon.vertexList.push_back(&p3);

    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    int i;

    /* Context Element response # 1 */
    i = getEntityIndex(res.contextElementResponseVector, "A");
    ASSERT_GE(i, 0);
    EXPECT_EQ("A", RES_CER(i).id);
    EXPECT_EQ("Point", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_A", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    EXPECT_EQ("2, 3", RES_CER_ATTR(i, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryGeoPolygonOut1 -
*
* Area:      Square: [3,8], [11,8], [11,3], [3,3]
* Result:    A
*
*/
TEST(mongoQueryContextGeoRequest, queryGeoPolygonOut1)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en(".*", "Point", "true");
    req.entityIdVector.push_back(&en);

    /* Define area scope */
    Scope sc;
    sc.type = "FIWARE::Location";
    sc.areaType = orion::PolygonType;
    orion::Point p1, p2, p3, p4;

    p1.latitudeSet("3");
    p1.longitudeSet("8");
    sc.polygon.vertexList.push_back(&p1);

    p2.latitudeSet("11");
    p2.longitudeSet("8");
    sc.polygon.vertexList.push_back(&p2);

    p3.latitudeSet("11");
    p3.longitudeSet("3");
    sc.polygon.vertexList.push_back(&p3);

    p4.latitudeSet("3");
    p4.longitudeSet("3");
    sc.polygon.vertexList.push_back(&p4);

    sc.polygon.invertedSet("true");
    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    int i;

    /* Context Element response # 1 */
    i = getEntityIndex(res.contextElementResponseVector, "A");
    ASSERT_GE(i, 0);
    EXPECT_EQ("A", RES_CER(i).id);
    EXPECT_EQ("Point", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_A", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    EXPECT_EQ("2, 3", RES_CER_ATTR(i, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryGeoPolygonOut2 -
*
* Area:      Triangle: [0,0], [0,6], [6,0]
* Result:    B, C
*
*/
TEST(mongoQueryContextGeoRequest, queryGeoPolygonOut2)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en(".*", "Point", "true");
    req.entityIdVector.push_back(&en);

    /* Define area scope */
    Scope sc;
    sc.type = "FIWARE::Location";
    sc.areaType = orion::PolygonType;
    orion::Point p1, p2, p3;

    p1.latitudeSet("0");
    p1.longitudeSet("0");
    sc.polygon.vertexList.push_back(&p1);

    p2.latitudeSet("0");
    p2.longitudeSet("6");
    sc.polygon.vertexList.push_back(&p2);

    p3.latitudeSet("6");
    p3.longitudeSet("0");
    sc.polygon.vertexList.push_back(&p3);

    sc.polygon.invertedSet("true");
    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    int i;

    /* Context Element response # 1 */
    i = getEntityIndex(res.contextElementResponseVector, "B");
    ASSERT_GE(i, 0);
    EXPECT_EQ("B", RES_CER(i).id);
    EXPECT_EQ("Point", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_B", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    EXPECT_EQ("5, 5", RES_CER_ATTR(i, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Context Element response # 2 */
    i = getEntityIndex(res.contextElementResponseVector, "C");
    ASSERT_GE(i, 0);
    EXPECT_EQ("C", RES_CER(i).id);
    EXPECT_EQ("Point", RES_CER(i).type);
    EXPECT_EQ("false", RES_CER(i).isPattern);
    ASSERT_EQ(2, RES_CER(i).attributeVector.size());
    EXPECT_EQ("foo", RES_CER_ATTR(i, 0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 0)->type);
    EXPECT_EQ("attr_C", RES_CER_ATTR(i, 0)->stringValue);
    EXPECT_EQ("pos", RES_CER_ATTR(i, 1)->name);
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->type);
    EXPECT_EQ("4, 7", RES_CER_ATTR(i, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(i, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(i, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(i, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(i, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(i).code);
    EXPECT_EQ("OK", RES_CER_STATUS(i).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(i).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}
