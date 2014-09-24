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
#include "mongoBackend/mongoQueryTypes.h"
#include "utility/EntityTypesResponse.h"

#include "mongo/client/dbclient.h"

/* ****************************************************************************
*
* Tests
*
* - queryAllType (*)
* - queryGivenType
* - monboDbQueryFail
*
* (*) FIXME: currently mongoBackend doesn't interprets collapse parameter (considering
* that the "collapse prunning" is done at render layer). However, if in the future
* we change this operation, a new case has to be added to test that mongoBackend honour
* the collapse parameter.
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
static void prepareDatabase(void) {

  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following entities (value is not meaniful)
   *
   * - Type Car:
   *     Car1: pos, temp, plate
   *     Car2: pos, plate(*), fuel
   *     Car3: pos, colour
   * - Type Room:
   *     Room1: pos, temp
   *     Room2: pos, humidity
   * - Type Lamp:
   *     Lamp1: battery, status
   *
   * (*) Diferent type in the attribute
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "Car1" << "type" << "Car") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "pos" << "type" << "pos_T" << "value" << "1") <<
                        BSON("name" << "temp" << "type" << "temp_T" << "value" << "2") <<
                        BSON("name" << "plate" << "type" << "plate_T" << "value" << "3")
                        )
                    );

  BSONObj en2 = BSON("_id" << BSON("id" << "Car2" << "type" << "Car") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "pos" << "type" << "pos_T" << "value" << "4") <<
                        BSON("name" << "plate" << "type" << "plate_T2" << "value" << "5") <<
                        BSON("name" << "fuel" << "type" << "fuel_T" << "value" << "6")
                        )
                    );

  BSONObj en3 = BSON("_id" << BSON("id" << "Car3" << "type" << "Car") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "pos" << "type" << "pos_T" << "value" << "7") <<
                        BSON("name" << "colour" << "type" << "colour_T" << "value" << "8")
                        )
                    );

  BSONObj en4 = BSON("_id" << BSON("id" << "Room1" << "type" << "Room") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "pos" << "type" << "pos_T" << "value" << "9") <<
                        BSON("name" << "temp" << "type" << "temp_T" << "value" << "10")
                        )
                    );

  BSONObj en5 = BSON("_id" << BSON("id" << "Room2" << "type" << "Room") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "pos" << "type" << "pos_T" << "value" << "11") <<
                        BSON("name" << "humidity" << "type" << "humidity_T" << "value" << "12")
                        )
                    );

  BSONObj en6 = BSON("_id" << BSON("id" << "Lamp1" << "type" << "Lamp") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "battery" << "type" << "battery_T" << "value" << "13") <<
                        BSON("name" << "status" << "type" << "status_T" << "value" << "14")
                        )
                    );


  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en3);
  connection->insert(ENTITIES_COLL, en4);
  connection->insert(ENTITIES_COLL, en5);
  connection->insert(ENTITIES_COLL, en6);

}

/* ****************************************************************************
*
* queryAllTypes -
*
*/
TEST(mongoQueryTypes, queryAllType)
{
    HttpStatusCode         ms;
    EntityTypesResponse     res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(3, res.typeEntityVector.size());

    /* Type # 1 */
    EXPECT_EQ("Car", res.typeEntityVector.get(0)->type);
    ASSERT_EQ(6, res.typeEntityVector.get(0)->contextAttributeVector.size());

    EXPECT_EQ("fuel", res.typeEntityVector.get(0)->contextAttributeVector.get(0)->name);
    EXPECT_EQ("fuel_T", res.typeEntityVector.get(0)->contextAttributeVector.get(0)->type);
    EXPECT_EQ("", res.typeEntityVector.get(0)->contextAttributeVector.get(0)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(0)->contextAttributeVector.get(0)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(0)->contextAttributeVector.get(0)->metadataVector.size());

    EXPECT_EQ("plate", res.typeEntityVector.get(0)->contextAttributeVector.get(1)->name);
    EXPECT_EQ("plate_T", res.typeEntityVector.get(0)->contextAttributeVector.get(1)->type);
    EXPECT_EQ("", res.typeEntityVector.get(0)->contextAttributeVector.get(1)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(0)->contextAttributeVector.get(1)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(0)->contextAttributeVector.get(1)->metadataVector.size());

    EXPECT_EQ("temp", res.typeEntityVector.get(0)->contextAttributeVector.get(2)->name);
    EXPECT_EQ("temp_T", res.typeEntityVector.get(0)->contextAttributeVector.get(2)->type);
    EXPECT_EQ("", res.typeEntityVector.get(0)->contextAttributeVector.get(2)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(0)->contextAttributeVector.get(2)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(0)->contextAttributeVector.get(2)->metadataVector.size());

    EXPECT_EQ("colour", res.typeEntityVector.get(0)->contextAttributeVector.get(3)->name);
    EXPECT_EQ("colour_T", res.typeEntityVector.get(0)->contextAttributeVector.get(3)->type);
    EXPECT_EQ("", res.typeEntityVector.get(0)->contextAttributeVector.get(3)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(0)->contextAttributeVector.get(3)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(0)->contextAttributeVector.get(3)->metadataVector.size());

    EXPECT_EQ("plate", res.typeEntityVector.get(0)->contextAttributeVector.get(4)->name);
    EXPECT_EQ("plate_T2", res.typeEntityVector.get(0)->contextAttributeVector.get(4)->type);
    EXPECT_EQ("", res.typeEntityVector.get(0)->contextAttributeVector.get(4)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(0)->contextAttributeVector.get(4)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(0)->contextAttributeVector.get(4)->metadataVector.size());

    EXPECT_EQ("pos", res.typeEntityVector.get(0)->contextAttributeVector.get(5)->name);
    EXPECT_EQ("pos_T", res.typeEntityVector.get(0)->contextAttributeVector.get(5)->type);
    EXPECT_EQ("", res.typeEntityVector.get(0)->contextAttributeVector.get(5)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(0)->contextAttributeVector.get(5)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(0)->contextAttributeVector.get(5)->metadataVector.size());

    /* Type # 2 */
    EXPECT_EQ("Lamp", res.typeEntityVector.get(1)->type);
    ASSERT_EQ(2, res.typeEntityVector.get(1)->contextAttributeVector.size());

    EXPECT_EQ("status", res.typeEntityVector.get(1)->contextAttributeVector.get(0)->name);
    EXPECT_EQ("status_T", res.typeEntityVector.get(1)->contextAttributeVector.get(0)->type);
    EXPECT_EQ("", res.typeEntityVector.get(1)->contextAttributeVector.get(0)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(1)->contextAttributeVector.get(0)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(1)->contextAttributeVector.get(0)->metadataVector.size());

    EXPECT_EQ("battery", res.typeEntityVector.get(1)->contextAttributeVector.get(1)->name);
    EXPECT_EQ("battery_T", res.typeEntityVector.get(1)->contextAttributeVector.get(1)->type);
    EXPECT_EQ("", res.typeEntityVector.get(1)->contextAttributeVector.get(1)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(1)->contextAttributeVector.get(1)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(1)->contextAttributeVector.get(1)->metadataVector.size());

    /* Type # 3 */
    EXPECT_EQ("Room", res.typeEntityVector.get(2)->type);
    ASSERT_EQ(3, res.typeEntityVector.get(2)->contextAttributeVector.size());

    EXPECT_EQ("humidity", res.typeEntityVector.get(2)->contextAttributeVector.get(0)->name);
    EXPECT_EQ("humidity_T", res.typeEntityVector.get(2)->contextAttributeVector.get(0)->type);
    EXPECT_EQ("", res.typeEntityVector.get(2)->contextAttributeVector.get(0)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(2)->contextAttributeVector.get(0)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(2)->contextAttributeVector.get(0)->metadataVector.size());

    EXPECT_EQ("temp", res.typeEntityVector.get(2)->contextAttributeVector.get(1)->name);
    EXPECT_EQ("temp_T", res.typeEntityVector.get(2)->contextAttributeVector.get(1)->type);
    EXPECT_EQ("", res.typeEntityVector.get(2)->contextAttributeVector.get(1)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(2)->contextAttributeVector.get(1)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(2)->contextAttributeVector.get(1)->metadataVector.size());

    EXPECT_EQ("pos", res.typeEntityVector.get(2)->contextAttributeVector.get(2)->name);
    EXPECT_EQ("pos_T", res.typeEntityVector.get(2)->contextAttributeVector.get(2)->type);
    EXPECT_EQ("", res.typeEntityVector.get(2)->contextAttributeVector.get(2)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(2)->contextAttributeVector.get(2)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(2)->contextAttributeVector.get(2)->metadataVector.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}

#if 0

/* ****************************************************************************
*
* paginationAll -
*
*/
TEST(mongoQueryTypes, paginationAll)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseForPagination();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E.*", "T", "true");
    req.entityIdVector.push_back(&en);
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    /* Using default offset/limit */

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector , uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(6, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("a1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("a2", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E3", RES_CER(2).entityId.id);
    EXPECT_EQ("T", RES_CER(2).entityId.type);
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ("a3", RES_CER_ATTR(2, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Context Element response # 4 */
    EXPECT_EQ("E4", RES_CER(3).entityId.id);
    EXPECT_EQ("T", RES_CER(3).entityId.type);
    EXPECT_EQ("false", RES_CER(3).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(3).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(3, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(3, 0)->type);
    EXPECT_EQ("a4", RES_CER_ATTR(3, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
    EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

    /* Context Element response # 5 */
    EXPECT_EQ("E5", RES_CER(4).entityId.id);
    EXPECT_EQ("T", RES_CER(4).entityId.type);
    EXPECT_EQ("false", RES_CER(4).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(4).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(4, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(4, 0)->type);
    EXPECT_EQ("a5", RES_CER_ATTR(4, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(4).code);
    EXPECT_EQ("OK", RES_CER_STATUS(4).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(4).details.size());

    /* Context Element response # 6 */
    EXPECT_EQ("E6", RES_CER(5).entityId.id);
    EXPECT_EQ("T", RES_CER(5).entityId.type);
    EXPECT_EQ("false", RES_CER(5).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(5).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(5, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(5, 0)->type);
    EXPECT_EQ("a6", RES_CER_ATTR(5, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(5).code);
    EXPECT_EQ("OK", RES_CER_STATUS(5).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(5).details.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* paginationOnlyFirst -
*
*/
TEST(mongoQueryTypes, paginationOnlyFirst)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseForPagination();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E.*", "T", "true");
    req.entityIdVector.push_back(&en);
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_LIMIT] = "1";

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector , uriParams);

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
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("a1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* paginationOnlySecond -
*
*/
TEST(mongoQueryTypes, paginationOnlySecond)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseForPagination();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E.*", "T", "true");
    req.entityIdVector.push_back(&en);
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "1";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "1";

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector , uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E2", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("a2", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* paginationRange -
*
*/
TEST(mongoQueryTypes, paginationRange)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseForPagination();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E.*", "T", "true");
    req.entityIdVector.push_back(&en);
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "2";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "3";

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector , uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("a3", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E4", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("a4", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E5", RES_CER(2).entityId.id);
    EXPECT_EQ("T", RES_CER(2).entityId.type);
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ("a5", RES_CER_ATTR(2, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* paginationNonExisting -
*
*/
TEST(mongoQueryTypes, paginationNonExisting)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseForPagination();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E.*", "T", "true");
    req.entityIdVector.push_back(&en);
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "7";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "3";

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector , uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* paginationNonExistingOverlap -
*
*/
TEST(mongoQueryTypes, paginationNonExistingOverlap)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseForPagination();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E.*", "T", "true");
    req.entityIdVector.push_back(&en);
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "5";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "4";

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector , uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E6", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("a6", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* paginationNonExistingDetails -
*
*/
TEST(mongoQueryTypes, paginationNonExistingDetails)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseForPagination();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E.*", "T", "true");
    req.entityIdVector.push_back(&en);
    uriParams[URI_PARAM_PAGINATION_OFFSET]   = "7";
    uriParams[URI_PARAM_PAGINATION_LIMIT]    = "3";
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "on";

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector , uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ("Number of matching entities: 6. Offset is 7", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntPatternType_2levels -
*
*/
TEST(mongoQueryTypes, queryWithServicePathEntPatternType_2levels)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("patternType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "T", "true");
  req.entityIdVector.push_back(&en);
  servicePathVector.push_back("/home/kz");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(3, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E2", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a2", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  /* Context Element response # 2 */
  EXPECT_EQ("E4", RES_CER(1).entityId.id);
  EXPECT_EQ("T", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a4", RES_CER_ATTR(1, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Context Element response # 3 */
  EXPECT_EQ("E5", RES_CER(2).entityId.id);
  EXPECT_EQ("T", RES_CER(2).entityId.type);
  EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("a5", RES_CER_ATTR(2, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntPatternType_1level -
*
*/
TEST(mongoQueryTypes, queryWithServicePathEntPatternType_1level)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("patternType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "T", "true");
  req.entityIdVector.push_back(&en);
  servicePathVector.push_back("/home");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(6, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a1", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  /* Context Element response # 2 */
  EXPECT_EQ("E2", RES_CER(1).entityId.id);
  EXPECT_EQ("T", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a2", RES_CER_ATTR(1, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Context Element response # 3 */
  EXPECT_EQ("E3", RES_CER(2).entityId.id);
  EXPECT_EQ("T", RES_CER(2).entityId.type);
  EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("a3", RES_CER_ATTR(2, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

  /* Context Element response # 4 */
  EXPECT_EQ("E4", RES_CER(3).entityId.id);
  EXPECT_EQ("T", RES_CER(3).entityId.type);
  EXPECT_EQ("false", RES_CER(3).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(3).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(3, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(3, 0)->type);
  EXPECT_EQ("a4", RES_CER_ATTR(3, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
  EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

  /* Context Element response # 5 */
  EXPECT_EQ("E5", RES_CER(4).entityId.id);
  EXPECT_EQ("T", RES_CER(4).entityId.type);
  EXPECT_EQ("false", RES_CER(4).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(4).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(4, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(4, 0)->type);
  EXPECT_EQ("a5", RES_CER_ATTR(4, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(4).code);
  EXPECT_EQ("OK", RES_CER_STATUS(4).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(4).details.size());

  /* Context Element response # 6 */
  EXPECT_EQ("E6", RES_CER(5).entityId.id);
  EXPECT_EQ("T", RES_CER(5).entityId.type);
  EXPECT_EQ("false", RES_CER(5).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(5).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(5, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(5, 0)->type);
  EXPECT_EQ("a6", RES_CER_ATTR(5, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(5).code);
  EXPECT_EQ("OK", RES_CER_STATUS(5).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(5).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntPatternType_0levels -
*
*/
TEST(mongoQueryTypes, queryWithServicePathEntPatternType_0levels)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("patternType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "T", "true");
  req.entityIdVector.push_back(&en);
  servicePathVector.clear();

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);


  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E10", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a10", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntPatternType_1levelbis -
*
*/
TEST(mongoQueryTypes, queryWithServicePathEntPatternType_1levelbis)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("patternType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "T", "true");
  req.entityIdVector.push_back(&en);
  servicePathVector.push_back("/home2");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E7", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a7", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  /* Context Element response # 2 */
  EXPECT_EQ("E8", RES_CER(1).entityId.id);
  EXPECT_EQ("T", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a8", RES_CER_ATTR(1, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithIdenticalEntitiesButDifferentServicePaths_case1 -
*
*/
TEST(mongoQueryTypes, queryWithIdenticalEntitiesButDifferentServicePaths_case1)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("IdenticalEntitiesButDifferentServicePaths");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("IE", "T", "false");
  req.entityIdVector.push_back(&en);

  // Test that three items are found for Service path /home/fg
  servicePathVector.clear();
  servicePathVector.push_back("/home/fg");

  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(3, res.contextElementResponseVector.size());

  EXPECT_EQ("IE", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("ie_01", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  EXPECT_EQ("IE", RES_CER(1).entityId.id);
  EXPECT_EQ("T", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("ie_02", RES_CER_ATTR(1, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  EXPECT_EQ("IE", RES_CER(2).entityId.id);
  EXPECT_EQ("T", RES_CER(2).entityId.type);
  EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("ie_03", RES_CER_ATTR(2, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithIdenticalEntitiesButDifferentServicePaths_case2 -
*
*/
TEST(mongoQueryTypes, queryWithIdenticalEntitiesButDifferentServicePaths_case2)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("IdenticalEntitiesButDifferentServicePaths");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("IE", "T", "false");
  req.entityIdVector.push_back(&en);

  // Test that only ONE item AND the right one is found for Service paths /home/fg/01, /home/fg/02, and /home/fg/03
  servicePathVector.clear();
  servicePathVector.push_back("/home/fg/01");

  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  EXPECT_EQ("IE", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("ie_01", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithIdenticalEntitiesButDifferentServicePaths_case3 -
*
*/
TEST(mongoQueryTypes, queryWithIdenticalEntitiesButDifferentServicePaths_case3)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("IdenticalEntitiesButDifferentServicePaths");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("IE", "T", "false");
  req.entityIdVector.push_back(&en);

  // Same test for /home/fg/02
  servicePathVector.clear();
  servicePathVector.push_back("/home/fg/02");

  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  EXPECT_EQ("IE", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("ie_02", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithIdenticalEntitiesButDifferentServicePaths_case4 -
*
*/
TEST(mongoQueryTypes, queryWithIdenticalEntitiesButDifferentServicePaths_case4)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("IdenticalEntitiesButDifferentServicePaths");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("IE", "T", "false");
  req.entityIdVector.push_back(&en);

  // Same test for /home/fg/03

  servicePathVector.clear();
  servicePathVector.push_back("/home/fg/03");

  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  EXPECT_EQ("IE", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("ie_03", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntPatternNoType_2levels -
*
*/
TEST(mongoQueryTypes, queryWithServicePathEntPatternNoType_2levels)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("patternNoType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "", "true");
  req.entityIdVector.push_back(&en);
  servicePathVector.push_back("/home/kz");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

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
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a2", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  /* Context Element response # 2 */
  EXPECT_EQ("E4", RES_CER(1).entityId.id);
  EXPECT_EQ("T", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a4", RES_CER_ATTR(1, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Context Element response # 3 */
  EXPECT_EQ("E5", RES_CER(2).entityId.id);
  EXPECT_EQ("T", RES_CER(2).entityId.type);
  EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("a5", RES_CER_ATTR(2, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

  /* Context Element response # 4 */
  EXPECT_EQ("E", RES_CER(3).entityId.id);
  EXPECT_EQ("OOO", RES_CER(3).entityId.type);
  EXPECT_EQ("false", RES_CER(3).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(3).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(3, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(3, 0)->type);
  EXPECT_EQ("ae_1", RES_CER_ATTR(3, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
  EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntPatternNoType_1level -
*
*/
TEST(mongoQueryTypes, queryWithServicePathEntPatternNoType_1level)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("patternNoType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "", "true");
  req.entityIdVector.push_back(&en);
  servicePathVector.push_back("/home");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(7, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a1", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  /* Context Element response # 2 */
  EXPECT_EQ("E2", RES_CER(1).entityId.id);
  EXPECT_EQ("T", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a2", RES_CER_ATTR(1, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Context Element response # 3 */
  EXPECT_EQ("E3", RES_CER(2).entityId.id);
  EXPECT_EQ("T", RES_CER(2).entityId.type);
  EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("a3", RES_CER_ATTR(2, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

  /* Context Element response # 4 */
  EXPECT_EQ("E4", RES_CER(3).entityId.id);
  EXPECT_EQ("T", RES_CER(3).entityId.type);
  EXPECT_EQ("false", RES_CER(3).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(3).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(3, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(3, 0)->type);
  EXPECT_EQ("a4", RES_CER_ATTR(3, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
  EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

  /* Context Element response # 5 */
  EXPECT_EQ("E5", RES_CER(4).entityId.id);
  EXPECT_EQ("T", RES_CER(4).entityId.type);
  EXPECT_EQ("false", RES_CER(4).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(4).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(4, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(4, 0)->type);
  EXPECT_EQ("a5", RES_CER_ATTR(4, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(4).code);
  EXPECT_EQ("OK", RES_CER_STATUS(4).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(4).details.size());

  /* Context Element response # 6 */
  EXPECT_EQ("E6", RES_CER(5).entityId.id);
  EXPECT_EQ("T", RES_CER(5).entityId.type);
  EXPECT_EQ("false", RES_CER(5).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(5).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(5, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(5, 0)->type);
  EXPECT_EQ("a6", RES_CER_ATTR(5, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(5).code);
  EXPECT_EQ("OK", RES_CER_STATUS(5).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(5).details.size());

  /* Context Element response # 7 */
  EXPECT_EQ("E", RES_CER(6).entityId.id);
  EXPECT_EQ("OOO", RES_CER(6).entityId.type);
  EXPECT_EQ("false", RES_CER(6).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(6).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(6, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(6, 0)->type);
  EXPECT_EQ("ae_1", RES_CER_ATTR(6, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(6).code);
  EXPECT_EQ("OK", RES_CER_STATUS(6).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(6).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntPatternNoType_0levels -
*
* FIXME P4: Fermin to inspect whether the function has enough EXPECTs
*/
TEST(mongoQueryTypes, queryWithServicePathEntPatternNoType_0levels)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("patternNoType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "", "true");
  req.entityIdVector.push_back(&en);
  servicePathVector.clear();

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E10", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a10", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntPatternNoType_1levelbis -
*
*/
TEST(mongoQueryTypes, queryWithServicePathEntPatternNoType_1levelbis)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("patternNoType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "", "true");
  req.entityIdVector.push_back(&en);
  servicePathVector.push_back("/home2");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E7", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a7", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  /* Context Element response # 2 */
  EXPECT_EQ("E8", RES_CER(1).entityId.id);
  EXPECT_EQ("T", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a8", RES_CER_ATTR(1, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntNoPatternTypeFail -
*
*/
TEST(mongoQueryTypes, queryWithServicePathEntNoPatternTypeFail)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("noPatternType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E3", "T", "false");
  req.entityIdVector.push_back(&en);
  servicePathVector.push_back("/home/kz");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
  EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  EXPECT_EQ(0, res.contextElementResponseVector.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntNoPatternTypeOk -
*
*/
TEST(mongoQueryTypes, queryWithServicePathEntNoPatternTypeOk)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("noPatternType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E3", "T", "false");
  req.entityIdVector.push_back(&en);
  servicePathVector.push_back("/home/fg");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E3", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a3", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntNoPatternNoType -
*
*/
TEST(mongoQueryTypes, queryWithServicePathEntNoPatternNoType)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("noPatternNoType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E3", "", "false");
  req.entityIdVector.push_back(&en);
  servicePathVector.push_back("/home/fg");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E3", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a3", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  /* Context Element response # 2 */
  EXPECT_EQ("E3", RES_CER(1).entityId.id);
  EXPECT_EQ("OOO", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("ae_2", RES_CER_ATTR(1, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithSeveralServicePaths -
*
*/
TEST(mongoQueryTypes, queryWithSeveralServicePaths)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("patternType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E.*", "T", "true");
  req.entityIdVector.push_back(&en);
  servicePathVector.clear();
  servicePathVector.push_back("/home/kz/e4");
  servicePathVector.push_back("/home3/e12");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(0 , res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E4", RES_CER(0).entityId.id);
  EXPECT_EQ("T", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a4", RES_CER_ATTR(0, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  /* Context Element response # 2 */
  EXPECT_EQ("E12", RES_CER(1).entityId.id);
  EXPECT_EQ("T", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a12", RES_CER_ATTR(1, 0)->value);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  utExit();
}

/* ****************************************************************************
*
* query1Ent0Attr -
*
* Discover:  E1 - no attrs
* Result:    E1 - (A1, A2)
*
* This test also checks that querying for type (E1) doesn't match with no-typed
* entities (E1** is not returned)
*
*/
TEST(mongoQueryTypes, query1Ent0Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

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
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* query1Ent1Attr -
*
* Discover:  E1 - A1
* Result:    E1 - A1
*/
TEST(mongoQueryTypes, query1Ent1Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector , uriParams);

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
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* query1Ent1AttrSameName -
*
* Discover:  E1* - A1
* Result:    E1* - (A1, A1*)  [same name but different type]
*/
TEST(mongoQueryTypes, query1Ent1AttrSameName)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1bis", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1bis", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1bis", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA1bis", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val1bis2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNEnt0Attr -
*
* Discover:  (E1, E2) -no attrs
* Result:    E1 - (A1, A2)
*            E2 - (A3, A4)
*/
TEST(mongoQueryTypes, queryNEnt0Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E2", "T2", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

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
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNEnt1AttrSingle -
*
* Discover:  (E1, E2) - A1
* Result:    E1 - A1
*/
TEST(mongoQueryTypes, queryNEnt1AttrSingle)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E2", "T2", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

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
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNEnt1AttrMulti -
*
* Discover:  (E1, E2) - A2
* Result:    E1 - A2
*            E2 - A2
*/
TEST(mongoQueryTypes, queryNEnt1AttrMulti)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E2", "T2", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.attributeList.push_back("A2");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

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
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNEntNAttr -
*
* Discover:  (E1, E2) - (A1, A3)
* Result:    E1 - A1
*            E2 - A3
*/
TEST(mongoQueryTypes, queryNEntNAttr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E2", "T2", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.attributeList.push_back("A1");
    req.attributeList.push_back("A3");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

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
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A3", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* query1Ent0AttrFail -
*
* Discover:  E3 - no attrs
* Result:    none
*/
TEST(mongoQueryTypes, query1Ent0AttrFail)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E3", "T3", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);
    EXPECT_EQ(0,res.contextElementResponseVector.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* query1Ent1AttrFail -
*
* Discover:  E1 - A3
* Result:    none
*/
TEST(mongoQueryTypes, query1Ent1AttrFail)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A3");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);
    EXPECT_EQ(0,res.contextElementResponseVector.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}


/* ****************************************************************************
*
* query1EntWA0AttrFail -
*
* Discover:  E4 - A1
* Result:    none
*/
TEST(mongoQueryTypes, query1EntWA0AttrFail)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E4", "T4", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);
    EXPECT_EQ(0,res.contextElementResponseVector.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* query1EntWA1Attr -
*
* Discover:  E4 - none
* Result:    E4 - none
*/
TEST(mongoQueryTypes, query1EntWA1Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E4", "T4", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E4", RES_CER(0).entityId.id);
    EXPECT_EQ("T4", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    EXPECT_EQ(0, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNEntWA0Attr -
*
* Discover:  (E1, E4) - none
* Result:    E1 - A1, A2
*            E4 - none
*/
TEST(mongoQueryTypes, queryNEntWA0Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E4", "T4", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

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
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E4", RES_CER(1).entityId.id);
    EXPECT_EQ("T4", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    EXPECT_EQ(0, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNEntWA1Attr -
*
* Discover:  (E1, E4) - A1
* Result:    E1 - A1
*/
TEST(mongoQueryTypes, queryNEntWA1Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E1", "T1", "false");
    EntityId en2("E4", "T4", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

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
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNoType -
*
* Discover:  E1
* Result:    E1   - A1
*            E1*  - A1, A1*
*            E1** - A1
*
* Note that this case checks matching of no-type in the query for both the case in
* which the returned entity has type (E1 and E1*) and the case in which it has no type (E1**)
*
*/
TEST(mongoQueryTypes, queryNoType)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response #2 */
    EXPECT_EQ("E1", RES_CER(1).entityId.id);
    EXPECT_EQ("T1bis", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val1bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA1bis", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val1bis2", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response #3 */
    EXPECT_EQ("E1", RES_CER(2).entityId.id);
    EXPECT_EQ(0, RES_CER(2).entityId.type.size());
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ("val1bis1", RES_CER_ATTR(2, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryIdMetadata -
*
*/
TEST(mongoQueryTypes, queryIdMetadata)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E10", "T", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(3, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("A", RES_CER_ATTR(0, 0)->value);
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("B", RES_CER_ATTR(0, 1)->value);
    ASSERT_EQ(1, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 1)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->metadataVector.get(0)->type);
    EXPECT_EQ("ID2", RES_CER_ATTR(0, 1)->metadataVector.get(0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 2)->name);
    EXPECT_EQ("TA11", RES_CER_ATTR(0, 2)->type);
    EXPECT_EQ("C", RES_CER_ATTR(0, 2)->value);
    ASSERT_EQ(0, RES_CER_ATTR(0, 2)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryCustomMetadata -
*
*/
TEST(mongoQueryTypes, queryCustomMetadata)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabaseWithCustomMetadata();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E10", "T", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("A", RES_CER_ATTR(0, 0)->value);
    ASSERT_EQ(2, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->type);
    EXPECT_EQ("2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA11", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("B", RES_CER_ATTR(0, 1)->value);
    ASSERT_EQ(2, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 1)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 1)->metadataVector.get(0)->type);
    EXPECT_EQ("3", RES_CER_ATTR(0, 1)->metadataVector.get(0)->value);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 1)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 1)->metadataVector.get(1)->type);
    EXPECT_EQ("4", RES_CER_ATTR(0, 1)->metadataVector.get(1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}


/* ****************************************************************************
*
* queryPattern0Attr -
*
* Discover:  E[1-2] - none
* Result:    E1 - (A1, A2)
*            E2 - (A2, A3)
*
* This test also checks that querying for type (E[1-2]) doesn't match with no-typed
* entities (E1** is not returned)
*
*/
TEST(mongoQueryTypes, queryPattern0Attr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "T", "true");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

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
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryPattern1AttrSingle -
*
* Discover:  E[1-2] - A1
* Result:    E1 - A1
*/
TEST(mongoQueryTypes, queryPattern1AttrSingle)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

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
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryPattern1AttrMulti -
*
* Discover:  E[1-2] - A2
* Result:    E1 - A2
*            E2 - A2
*/
TEST(mongoQueryTypes, queryPattern1AttrMulti)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A2");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

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
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryPatternNAttr -
*
* Discover:  E[1-2] - (A1, A2)
* Result:    E1 - (A1, A2)
*            E2 - A2
*/
TEST(mongoQueryTypes, queryPatternNAttr)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");
    req.attributeList.push_back("A2");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

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
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryPatternFail -
*
* Discover:  R.* - none
* Result:    none
*/
TEST(mongoQueryTypes, queryPatternFail)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("R.*", "T", "true");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);
    EXPECT_EQ(0,res.contextElementResponseVector.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryMixPatternAndNotPattern -
*
* Discover:  (E[1-2]. E4) - none
* Result:    E1 - (A1, A2)
*            E2 - (A2 ,A3)
*            E4 - none
*/
TEST(mongoQueryTypes, queryMixPatternAndNotPattern)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en1("E[1-2]", "T", "true");
    EntityId en2("E4", "T", "false");
    req.entityIdVector.push_back(&en1);
    req.entityIdVector.push_back(&en2);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

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
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E4", RES_CER(2).entityId.id);
    EXPECT_EQ("T", RES_CER(2).entityId.type);
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    EXPECT_EQ(0, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryNoTypePattern -
*
* Discover:  E[1-2]
* Result:    E1   - A1, A2
*            E2   - A2 ,A3
*            E1*  - A4, A5
*            E2** - A2
*
* Note that this case checks matching of no-type in the query for both the case in
* which the returned entity has type (E1, E2 and E1*) and the case in which it has no type (E2**)
*
*/
TEST(mongoQueryTypes, queryNoTypePattern)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "", "true");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(4, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->value);
    EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E1", RES_CER(2).entityId.id);
    EXPECT_EQ("Tbis", RES_CER(2).entityId.type);
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ("A4", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA4", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ("val4", RES_CER_ATTR(2, 0)->value);
    EXPECT_EQ("A5", RES_CER_ATTR(2, 1)->name);
    EXPECT_EQ("TA5", RES_CER_ATTR(2, 1)->type);
    EXPECT_EQ("val5", RES_CER_ATTR(2, 1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Context Element response # 4 */
    EXPECT_EQ("E2", RES_CER(3).entityId.id);
    EXPECT_EQ(0, RES_CER(3).entityId.type.size());
    EXPECT_EQ("false", RES_CER(3).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(3).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(3, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(3, 0)->type);
    EXPECT_EQ("val2bis1", RES_CER_ATTR(3, 0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
    EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryIdMetadataPattern -
*
*/
TEST(mongoQueryTypes, queryIdMetadataPattern)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1[0-1]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(3, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("A", RES_CER_ATTR(0, 0)->value);
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("B", RES_CER_ATTR(0, 1)->value);
    ASSERT_EQ(1, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 1)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->metadataVector.get(0)->type);
    EXPECT_EQ("ID2", RES_CER_ATTR(0, 1)->metadataVector.get(0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 2)->name);
    EXPECT_EQ("TA11", RES_CER_ATTR(0, 2)->type);
    EXPECT_EQ("C", RES_CER_ATTR(0, 2)->value);
    ASSERT_EQ(0, RES_CER_ATTR(0, 2)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E11", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(3, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("E", RES_CER_ATTR(1, 0)->value);
    ASSERT_EQ(1, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(1, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(1, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(1, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("F", RES_CER_ATTR(1, 1)->value);
    ASSERT_EQ(1, RES_CER_ATTR(1, 1)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(1, 1)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(1, 1)->metadataVector.get(0)->type);
    EXPECT_EQ("ID2", RES_CER_ATTR(1, 1)->metadataVector.get(0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(1, 2)->name);
    EXPECT_EQ("TA11", RES_CER_ATTR(1, 2)->type);
    EXPECT_EQ("G", RES_CER_ATTR(1, 2)->value);
    ASSERT_EQ(0, RES_CER_ATTR(1, 2)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* queryCustomMetadataPattern -
*
*/
TEST(mongoQueryTypes, queryCustomMetadataPattern)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabaseWithCustomMetadata();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1[0-1]", "T", "true");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("A", RES_CER_ATTR(0, 0)->value);
    ASSERT_EQ(2, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->type);
    EXPECT_EQ("2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA11", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("B", RES_CER_ATTR(0, 1)->value);
    ASSERT_EQ(2, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 1)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 1)->metadataVector.get(0)->type);
    EXPECT_EQ("3", RES_CER_ATTR(0, 1)->metadataVector.get(0)->value);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 1)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 1)->metadataVector.get(1)->type);
    EXPECT_EQ("4", RES_CER_ATTR(0, 1)->metadataVector.get(1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E11", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("D", RES_CER_ATTR(1, 0)->value);
    ASSERT_EQ(2, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(1, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(1, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("7", RES_CER_ATTR(1, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("MD2", RES_CER_ATTR(1, 0)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(1, 0)->metadataVector.get(1)->type);
    EXPECT_EQ("8", RES_CER_ATTR(1, 0)->metadataVector.get(1)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA11", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("E", RES_CER_ATTR(1, 1)->value);
    ASSERT_EQ(2, RES_CER_ATTR(1, 1)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(1, 1)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(1, 1)->metadataVector.get(0)->type);
    EXPECT_EQ("9", RES_CER_ATTR(1, 1)->metadataVector.get(0)->value);
    EXPECT_EQ("MD2", RES_CER_ATTR(1, 1)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(1, 1)->metadataVector.get(1)->type);
    EXPECT_EQ("10", RES_CER_ATTR(1, 1)->metadataVector.get(1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Release connection */
    mongoDisconnect();
}

/* ****************************************************************************
*
* mongoDbQueryFail -
*
*/
TEST(mongoQueryTypes, mongoDbQueryFail)
{
    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare mock */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, _query(_,_,_,_,_,_,_))
            .WillByDefault(Throw(e));

    /* Set MongoDB connection */
    mongoConnect(connectionMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Internal Server Error", res.errorCode.reasonPhrase);
    EXPECT_EQ("collection: unittest.entities - "
              "query(): { query: { $or: [ { _id.id: \"E1\", _id.type: \"T1\" } ], _id.servicePath: { $exists: false } }, orderby: { creDate: 1 } } - "
              "exception: boom!!", res.errorCode.details);
    EXPECT_EQ(0,res.contextElementResponseVector.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Release mock */
    delete connectionMock;
}

#endif
