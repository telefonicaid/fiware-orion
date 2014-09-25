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
* - queryAllTypeBasic (*)
* - queryAllpaginationDetails (*)
* - queryAllpaginationAll (*)
* - queryAllpaginationOnlyFirst (*)
* - queryAllpaginationOnlySecond (*)
* - queryAllpaginationRange (*)
* - queryAllpaginationNonExisting (*)
* - queryAllpaginationNonExistingOverlap (*)
* - queryAllpaginationNonExistingDetails (*)
*
* - queryGivenType
*
* - queryAllDbException
* - queryAllDbExcpetionGeneric
* - queryGivenTypeDbFail
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

/* ****************************************************************************
*
* queryAllpaginationDetails -
*
*/
TEST(mongoQueryTypes, queryAllpaginationDetails)
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
/* ****************************************************************************
*
* queryAllpaginationAll -
*
*/
TEST(mongoQueryTypes, queryAllpaginationAll)
{
    HttpStatusCode         ms;
    EntityTypesResponse     res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    /* Using default offset/limit */
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
/* ****************************************************************************
*
* queryAllpaginationOnlyFirst -
*
*/
TEST(mongoQueryTypes, queryAllpaginationOnlyFirst)
{
    HttpStatusCode         ms;
    EntityTypesResponse     res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_LIMIT] = "1";
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(1, res.typeEntityVector.size());

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

    /* Release connection */
    mongoDisconnect();

    utExit();
}
/* ****************************************************************************
*
* queryAllpaginationOnlySecond -
*
*/
TEST(mongoQueryTypes, queryAllpaginationOnlySecond)
{
    HttpStatusCode         ms;
    EntityTypesResponse     res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "1";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "1";
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(1, res.typeEntityVector.size());

    /* Type # 2 */
    EXPECT_EQ("Lamp", res.typeEntityVector.get(0)->type);
    ASSERT_EQ(2, res.typeEntityVector.get(0)->contextAttributeVector.size());

    EXPECT_EQ("status", res.typeEntityVector.get(0)->contextAttributeVector.get(0)->name);
    EXPECT_EQ("status_T", res.typeEntityVector.get(0)->contextAttributeVector.get(0)->type);
    EXPECT_EQ("", res.typeEntityVector.get(0)->contextAttributeVector.get(0)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(0)->contextAttributeVector.get(0)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(0)->contextAttributeVector.get(0)->metadataVector.size());

    EXPECT_EQ("battery", res.typeEntityVector.get(0)->contextAttributeVector.get(1)->name);
    EXPECT_EQ("battery_T", res.typeEntityVector.get(0)->contextAttributeVector.get(1)->type);
    EXPECT_EQ("", res.typeEntityVector.get(0)->contextAttributeVector.get(1)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(0)->contextAttributeVector.get(1)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(0)->contextAttributeVector.get(1)->metadataVector.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}
/* ****************************************************************************
*
* queryAllpaginationRange -
*
*/
TEST(mongoQueryTypes, queryAllpaginationRange)
{
    HttpStatusCode         ms;
    EntityTypesResponse     res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "1";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "2";
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(2, res.typeEntityVector.size());

    /* Type # 1 */
    EXPECT_EQ("Lamp", res.typeEntityVector.get(0)->type);
    ASSERT_EQ(2, res.typeEntityVector.get(0)->contextAttributeVector.size());

    EXPECT_EQ("status", res.typeEntityVector.get(0)->contextAttributeVector.get(0)->name);
    EXPECT_EQ("status_T", res.typeEntityVector.get(0)->contextAttributeVector.get(0)->type);
    EXPECT_EQ("", res.typeEntityVector.get(0)->contextAttributeVector.get(0)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(0)->contextAttributeVector.get(0)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(0)->contextAttributeVector.get(0)->metadataVector.size());

    EXPECT_EQ("battery", res.typeEntityVector.get(0)->contextAttributeVector.get(1)->name);
    EXPECT_EQ("battery_T", res.typeEntityVector.get(0)->contextAttributeVector.get(1)->type);
    EXPECT_EQ("", res.typeEntityVector.get(0)->contextAttributeVector.get(1)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(0)->contextAttributeVector.get(1)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(0)->contextAttributeVector.get(1)->metadataVector.size());

    /* Type # 2 */
    EXPECT_EQ("Room", res.typeEntityVector.get(1)->type);
    ASSERT_EQ(3, res.typeEntityVector.get(1)->contextAttributeVector.size());

    EXPECT_EQ("humidity", res.typeEntityVector.get(1)->contextAttributeVector.get(0)->name);
    EXPECT_EQ("humidity_T", res.typeEntityVector.get(1)->contextAttributeVector.get(0)->type);
    EXPECT_EQ("", res.typeEntityVector.get(1)->contextAttributeVector.get(0)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(1)->contextAttributeVector.get(0)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(1)->contextAttributeVector.get(0)->metadataVector.size());

    EXPECT_EQ("temp", res.typeEntityVector.get(1)->contextAttributeVector.get(1)->name);
    EXPECT_EQ("temp_T", res.typeEntityVector.get(1)->contextAttributeVector.get(1)->type);
    EXPECT_EQ("", res.typeEntityVector.get(1)->contextAttributeVector.get(1)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(1)->contextAttributeVector.get(1)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(1)->contextAttributeVector.get(1)->metadataVector.size());

    EXPECT_EQ("pos", res.typeEntityVector.get(1)->contextAttributeVector.get(2)->name);
    EXPECT_EQ("pos_T", res.typeEntityVector.get(1)->contextAttributeVector.get(2)->type);
    EXPECT_EQ("", res.typeEntityVector.get(1)->contextAttributeVector.get(2)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(1)->contextAttributeVector.get(2)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(1)->contextAttributeVector.get(2)->metadataVector.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}
/* ****************************************************************************
*
* queryAllpaginationNonExisting -
*
*/
TEST(mongoQueryTypes, queryAllpaginationNonExisting)
{
    HttpStatusCode         ms;
    EntityTypesResponse     res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "7";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "3";
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.statusCode.code);
    EXPECT_EQ("No context element found", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(0, res.typeEntityVector.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}
/* ****************************************************************************
*
* queryAllpaginationNonExistingOverlap -
*
*/
TEST(mongoQueryTypes, queryAllpaginationNonExistingOverlap)
{
    HttpStatusCode         ms;
    EntityTypesResponse     res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "2";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "4";

    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(1, res.typeEntityVector.size());

    /* Type # 1 */
    EXPECT_EQ("Room", res.typeEntityVector.get(0)->type);
    ASSERT_EQ(3, res.typeEntityVector.get(0)->contextAttributeVector.size());

    EXPECT_EQ("humidity", res.typeEntityVector.get(0)->contextAttributeVector.get(0)->name);
    EXPECT_EQ("humidity_T", res.typeEntityVector.get(0)->contextAttributeVector.get(0)->type);
    EXPECT_EQ("", res.typeEntityVector.get(0)->contextAttributeVector.get(0)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(0)->contextAttributeVector.get(0)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(0)->contextAttributeVector.get(0)->metadataVector.size());

    EXPECT_EQ("temp", res.typeEntityVector.get(0)->contextAttributeVector.get(1)->name);
    EXPECT_EQ("temp_T", res.typeEntityVector.get(0)->contextAttributeVector.get(1)->type);
    EXPECT_EQ("", res.typeEntityVector.get(0)->contextAttributeVector.get(1)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(0)->contextAttributeVector.get(1)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(0)->contextAttributeVector.get(1)->metadataVector.size());

    EXPECT_EQ("pos", res.typeEntityVector.get(0)->contextAttributeVector.get(2)->name);
    EXPECT_EQ("pos_T", res.typeEntityVector.get(0)->contextAttributeVector.get(2)->type);
    EXPECT_EQ("", res.typeEntityVector.get(0)->contextAttributeVector.get(2)->value);
    EXPECT_EQ(NULL, res.typeEntityVector.get(0)->contextAttributeVector.get(2)->compoundValueP);
    EXPECT_EQ(0, res.typeEntityVector.get(0)->contextAttributeVector.get(2)->metadataVector.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}
/* ****************************************************************************
*
* queryAllpaginationNonExistingDetails -
*
*/
TEST(mongoQueryTypes, queryAllpaginationNonExistingDetails)
{
    HttpStatusCode         ms;
    EntityTypesResponse     res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "on";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "7";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "3";
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.statusCode.code);
    EXPECT_EQ("No context element found", res.statusCode.reasonPhrase);
    EXPECT_EQ("Number of types: 3. Offset is 7", res.statusCode.details);

    ASSERT_EQ(0, res.typeEntityVector.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* queryGivenType -
*
*/
TEST(mongoQueryTypes, queryGivenType)
{
    HttpStatusCode                ms;
    EntityTypesAttributesResponse res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoAttributesForEntityType("Room", &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(6, res.entityType.contextAttributeVector.size());

    /* Type # 1 */
    EXPECT_EQ("colour", res.entityType.contextAttributeVector.get(0)->name);
    EXPECT_EQ("colour_T", res.entityType.contextAttributeVector.get(0)->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector.get(0)->value);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector.get(0)->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector.get(0)->metadataVector.size());

    /* Type 2 */
    EXPECT_EQ("fuel", res.entityType.contextAttributeVector.get(1)->name);
    EXPECT_EQ("fuel_T", res.entityType.contextAttributeVector.get(1)->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector.get(1)->value);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector.get(1)->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector.get(1)->metadataVector.size());

    /* Type 3 */
    EXPECT_EQ("plate", res.entityType.contextAttributeVector.get(2)->name);
    EXPECT_EQ("plate_T", res.entityType.contextAttributeVector.get(2)->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector.get(2)->value);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector.get(2)->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector.get(2)->metadataVector.size());

    /* Type 4 */
    EXPECT_EQ("plate", res.entityType.contextAttributeVector.get(3)->name);
    EXPECT_EQ("plate_T2", res.entityType.contextAttributeVector.get(3)->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector.get(3)->value);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector.get(3)->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector.get(3)->metadataVector.size());

    /* Type 5 */
    EXPECT_EQ("pos", res.entityType.contextAttributeVector.get(4)->name);
    EXPECT_EQ("pos_T", res.entityType.contextAttributeVector.get(4)->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector.get(4)->value);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector.get(4)->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector.get(4)->metadataVector.size());

    /* Type 6 */
    EXPECT_EQ("temp", res.entityType.contextAttributeVector.get(5)->name);
    EXPECT_EQ("temp_T", res.entityType.contextAttributeVector.get(5)->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector.get(5)->value);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector.get(5)->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector.get(5)->metadataVector.size());

    /* Release connection */
    mongoDisconnect();

    utExit();
}

#if 0

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
