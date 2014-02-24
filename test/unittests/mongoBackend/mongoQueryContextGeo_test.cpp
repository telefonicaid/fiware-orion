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
#include "gtest/gtest.h"
#include "unittest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/EntityId.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"

#include "mongo/client/dbclient.h"

#include "commonMocks.h"

using ::testing::_;
using ::testing::Throw;

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
static void prepareDatabase(void) {

  /* Set database */
  setupDatabase();

  DBClientConnection* connection = getMongoConnection();

  connection->ensureIndex("tutorial.persons", BSON("location.coords" << "2dsphere" ));

  BSONObj A = BSON("_id" << BSON("id" << "A" << "type" << "Point") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "pos" << "type" << "location" << "value" << "3, 2") <<
                        BSON("name" << "foo" << "type" << "string" << "value" << "attr_A")
                        ) <<
                     "location" << BSON("attrName" << "pos" << "coords" << BSON_ARRAY(3 << 2))
                    );

  BSONObj B = BSON("_id" << BSON("id" << "B" << "type" << "Point") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "pos" << "type" << "location" << "value" << "5, 5") <<
                        BSON("name" << "foo" << "type" << "string" << "value" << "attr_B")
                        ) <<
                     "location" << BSON("attrName" << "pos" << "coords" << BSON_ARRAY(5 << 5))
                    );

  BSONObj C = BSON("_id" << BSON("id" << "C" << "type" << "Point") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "pos" << "type" << "location" << "value" << "7, 4") <<
                        BSON("name" << "foo" << "type" << "string" << "value" << "attr_C")
                        ) <<
                     "location" << BSON("attrName" << "pos" << "coords" << BSON_ARRAY(7 << 4))
                    );

  // Entity D hasn't a location attribute (i.e. no location field). This entity will be never returned
  // when a geoscope is defined
  BSONObj D = BSON("_id" << BSON("id" << "D" << "type" << "Point") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "pos" << "type" << "location" << "value" << "7, 4") <<
                        BSON("name" << "foo" << "type" << "string" << "value" << "attr_C")
                        )
                    );

  BSONObj city1 = BSON("_id" << BSON("id" << "Madrid" << "type" << "City") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "pos" << "type" << "location" << "value" << "40.418889, -3.691944") <<
                        BSON("name" << "foo" << "type" << "string" << "value" << "attr_Mad")
                        ) <<
                     "location" << BSON("attrName" << "pos" << "coords" << BSON_ARRAY(40.418889 << -3.691944))
                    );

  BSONObj city2 = BSON("_id" << BSON("id" << "Alcobendas" << "type" << "City") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "pos" << "type" << "location" << "value" << "40.533333, -3.633333") <<
                        BSON("name" << "foo" << "type" << "string" << "value" << "attr_Alc")
                        ) <<
                     "location" << BSON("attrName" << "pos" << "coords" << BSON_ARRAY(40.533333 << -3.633333))
                    );

  BSONObj city3 = BSON("_id" << BSON("id" << "Leganes" << "type" << "City") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "pos" << "type" << "location" << "value" << "40.316667, -3.75") <<
                        BSON("name" << "foo" << "type" << "string" << "value" << "attr_Leg")
                        ) <<
                     "location" << BSON("attrName" << "pos" << "coords" << BSON_ARRAY(40.316667 << -3.75))
                    );

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
    sc.type = "FIWARE_Location";
    sc.scopeType = ScopeAreaCircle;
    sc.circle.origin.latitude = 40.418889;
    sc.circle.origin.longitude = -3.691944;
    sc.circle.radius  = 14000;
    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("Madrid", RES_CER(0).entityId.id);
    EXPECT_EQ("City", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("40.418889, -3.691944", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("attr_Mad", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("Leganes", RES_CER(1).entityId.id);
    EXPECT_EQ("City", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("40.316667, -3.75", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("attr_Leg", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();

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
    sc.type = "FIWARE_Location";
    sc.scopeType = ScopeAreaCircle;
    sc.circle.origin.latitude = 40.418889;
    sc.circle.origin.longitude = -3.691944;
    sc.circle.radius  = 15000;
    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("Madrid", RES_CER(0).entityId.id);
    EXPECT_EQ("City", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("40.418889, -3.691944", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("attr_Mad", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("Alcobendas", RES_CER(1).entityId.id);
    EXPECT_EQ("City", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("40.533333, -3.633333", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("attr_Alc", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("Leganes", RES_CER(2).entityId.id);
    EXPECT_EQ("City", RES_CER(2).entityId.type);
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ("40.316667, -3.75", RES_CER_ATTR(2, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(2, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(2, 1)->type);
    EXPECT_EQ("attr_Leg", RES_CER_ATTR(2, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();

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
    sc.type = "FIWARE_Location";
    sc.scopeType = ScopeAreaCircle;
    sc.circle.origin.latitude = 40.418889;
    sc.circle.origin.longitude = -3.691944;
    sc.circle.radius  = 14000;
    // TBD: define out area
    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("Leganes", RES_CER(0).entityId.id);
    EXPECT_EQ("City", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("40.316667, -3.75", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("attr_Leg", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();

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
    sc.type = "FIWARE_Location";
    sc.scopeType = ScopeAreaPolygon;
    ScopePoint p1, p2, p3, p4;
    p1.latitude = 0; p1.longitude = 0; sc.polygon.vertexList.push_back(&p1);
    p2.latitude = 0; p2.longitude = 6; sc.polygon.vertexList.push_back(&p2);
    p3.latitude = 6; p3.longitude = 6; sc.polygon.vertexList.push_back(&p3);
    p4.latitude = 6; p4.longitude = 0; sc.polygon.vertexList.push_back(&p4);
    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("A", RES_CER(0).entityId.id);
    EXPECT_EQ("Point", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("3, 2", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("attr_A", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("B", RES_CER(1).entityId.id);
    EXPECT_EQ("Point", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("5, 5", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("attr_B", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();

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
    sc.type = "FIWARE_Location";
    sc.scopeType = ScopeAreaPolygon;
    ScopePoint p1, p2, p3, p4;
    p1.latitude = 3; p1.longitude = 8; sc.polygon.vertexList.push_back(&p1);
    p2.latitude = 11; p2.longitude = 8; sc.polygon.vertexList.push_back(&p2);
    p3.latitude = 11; p3.longitude = 3; sc.polygon.vertexList.push_back(&p3);
    p4.latitude = 3; p4.longitude = 3; sc.polygon.vertexList.push_back(&p4);
    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("B", RES_CER(0).entityId.id);
    EXPECT_EQ("Point", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("5, 5", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("attr_B", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("C", RES_CER(1).entityId.id);
    EXPECT_EQ("Point", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("7, 4", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("attr_C", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();

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
    sc.type = "FIWARE_Location";
    sc.scopeType = ScopeAreaPolygon;
    ScopePoint p1, p2, p3;
    p1.latitude = 0; p1.longitude = 0; sc.polygon.vertexList.push_back(&p1);
    p2.latitude = 0; p2.longitude = 6; sc.polygon.vertexList.push_back(&p2);
    p3.latitude = 6; p3.longitude = 0; sc.polygon.vertexList.push_back(&p3);
    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("A", RES_CER(0).entityId.id);
    EXPECT_EQ("Point", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("3, 2", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("attr_A", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();

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
    sc.type = "FIWARE_Location";
    sc.scopeType = ScopeAreaPolygon;
    ScopePoint p1, p2, p3, p4;
    p1.latitude = 3; p1.longitude = 8; sc.polygon.vertexList.push_back(&p1);
    p2.latitude = 11; p2.longitude = 8; sc.polygon.vertexList.push_back(&p2);
    p3.latitude = 11; p3.longitude = 3; sc.polygon.vertexList.push_back(&p3);
    p4.latitude = 3; p4.longitude = 3; sc.polygon.vertexList.push_back(&p4);
    // TBD: define out area
    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("A", RES_CER(0).entityId.id);
    EXPECT_EQ("Point", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("3, 2", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("attr_A", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();

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
    sc.type = "FIWARE_Location";
    sc.scopeType = ScopeAreaPolygon;
    ScopePoint p1, p2, p3;
    p1.latitude = 0; p1.longitude = 0; sc.polygon.vertexList.push_back(&p1);
    p2.latitude = 0; p2.longitude = 6; sc.polygon.vertexList.push_back(&p2);
    p3.latitude = 6; p3.longitude = 0; sc.polygon.vertexList.push_back(&p3);
    // TBD: define out area
    req.restriction.scopeVector.push_back(&sc);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("B", RES_CER(0).entityId.id);
    EXPECT_EQ("Point", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("5, 5", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("attr_B", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("C", RES_CER(1).entityId.id);
    EXPECT_EQ("Point", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("pos", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("location", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("7, 4", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("foo", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("string", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("attr_C", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();

    utExit();
}
