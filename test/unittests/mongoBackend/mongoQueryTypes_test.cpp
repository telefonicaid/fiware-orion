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

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoQueryTypes.h"
#include "orionTypes/EntityTypeVectorResponse.h"
#include "orionTypes/EntityTypeResponse.h"

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
using ::testing::Throw;
using ::testing::_;



extern void setMongoConnectionForUnitTest(DBClientBase* _connection);



/* ****************************************************************************
*
* Tests
*
* - queryAllTypeBasic (*)
* - queryAllPaginationDetails (*)
* - queryAllPaginationAll (*)
* - queryAllPaginationOnlyFirst (*)
* - queryAllPaginationOnlySecond (*)
* - queryAllPaginationRange (*)
* - queryAllPaginationNonExisting (*)
* - queryAllPaginationNonExistingOverlap (*)
* - queryAllPaginationNonExistingDetails (*)
*
* - queryGivenTypeBasic
* - queryGivenTypePaginationDetails
* - queryGivenTypePaginationAll
* - queryGivenTypePaginationOnlyFirst
* - queryGivenTypePaginationOnlySecond
* - queryGivenTypePaginationRange
* - queryGivenTypePaginationNonExisting
* - queryGivenTypePaginationNonExistingOverlap
* - queryGivenTypePaginationNonExistingDetails
*
* - queryAllDbException
* - queryAllGenericException
* - queryGivenTypeDbException
* - queryGivenTypeGenericException
*
* (*) FIXME: currently mongoBackend doesn't interprets collapse parameter (considering
* that the "collapse pruning" is done at render layer). However, if in the future
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
static void prepareDatabase(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following entities (value is not meaniful)
   *
   * - Type Car:
   *     Car1: pos, temp, plate
   *     Car2: pos, plate, fuel
   *     Car3: pos, colour
   * - Type Room:
   *     Room1: pos, temp
   *     Room2: pos, humidity
   * - Type Lamp:
   *     Lamp1: battery, status
   *
   * (*) Different type in the attribute
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "Car1" << "type" << "Car") <<
                     "attrNames" << BSON_ARRAY("pos" << "temp" << "plate") <<
                     "attrs" << BSON(
                       "pos"   << BSON("type" << "pos_T" << "value" << "1") <<
                       "temp"  << BSON("type" << "temp_T" << "value" << "2") <<
                       "plate" << BSON("type" << "plate_T" << "value" << "3")));

  BSONObj en2 = BSON("_id" << BSON("id" << "Car2" << "type" << "Car") <<
                     "attrNames" << BSON_ARRAY("pos" << "plate" << "fuel") <<
                     "attrs" << BSON(
                       "pos"   << BSON("type" << "pos_T" << "value" << "4") <<
                       "plate" << BSON("type" << "plate_T2" << "value" << "5") <<
                       "fuel"  << BSON("type" << "fuel_T" << "value" << "6")));

  BSONObj en3 = BSON("_id" << BSON("id" << "Car3" << "type" << "Car") <<
                     "attrNames" << BSON_ARRAY("pos" << "colour") <<
                     "attrs" << BSON(
                       "pos"    << BSON("type" << "pos_T" << "value" << "7") <<
                       "colour" << BSON("type" << "colour_T" << "value" << "8")));

  BSONObj en4 = BSON("_id" << BSON("id" << "Room1" << "type" << "Room") <<
                     "attrNames" << BSON_ARRAY("pos" << "temp") <<
                     "attrs" << BSON(
                       "pos"  << BSON("type" << "pos_T" << "value" << "9") <<
                       "temp" << BSON("type" << "temp_T" << "value" << "10")));

  BSONObj en5 = BSON("_id" << BSON("id" << "Room2" << "type" << "Room") <<
                     "attrNames" << BSON_ARRAY("pos" << "humidity") <<
                     "attrs" << BSON(
                       "pos"      << BSON("type" << "pos_T" << "value" << "11") <<
                       "humidity" << BSON("type" << "humidity_T" << "value" << "12")));

  BSONObj en6 = BSON("_id" << BSON("id" << "Lamp1" << "type" << "Lamp") <<
                     "attrNames" << BSON_ARRAY("battery" << "status") <<
                     "attrs" << BSON(
                       "battery" << BSON("type" << "battery_T" << "value" << "13") <<
                       "status"  << BSON("type" << "status_T" << "value" << "14")));


  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en3);
  connection->insert(ENTITIES_COLL, en4);
  connection->insert(ENTITIES_COLL, en5);
  connection->insert(ENTITIES_COLL, en6);
}



/* ****************************************************************************
*
* getAttr -
*/
ContextAttribute* getAttr(ContextAttributeVector& caV, const std::string& name, const std::string& type = "")
{
  for (unsigned int ix = 0; ix < caV.size() ; ix++)
  {
    if (caV[ix]->name == name)
    {
      if (type == "" || caV[ix]->type == type)
      {
        return caV[ix];
      }
    }
  }
  /* Usually (i.e. in no-test code) we should return NULL. However, it helps to debug to use
   * a ContextAttribute value with crazy values */
  ContextAttribute* ca = new ContextAttribute("error", "error_T", "error_V");
  return ca;
}

/* ****************************************************************************
*
* queryAllTypes -
*
*/
TEST(mongoQueryTypes, queryAllType)
{
    HttpStatusCode         ms;
    EntityTypeVectorResponse    res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams, V1, NULL, false);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(3, res.entityTypeVector.size());
    ContextAttribute* ca;

    /* Type # 1 */
    EXPECT_EQ("Car", res.entityTypeVector[0]->type);
    EXPECT_EQ(3, res.entityTypeVector[0]->count);
    ASSERT_EQ(5, res.entityTypeVector[0]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "fuel");
    EXPECT_EQ("fuel", ca->name);
    EXPECT_EQ("fuel_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "plate");
    EXPECT_EQ("plate", ca->name);
    EXPECT_EQ("plate_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "temp");
    EXPECT_EQ("temp", ca->name);
    EXPECT_EQ("temp_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "colour");
    EXPECT_EQ("colour", ca->name);
    EXPECT_EQ("colour_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "pos");
    EXPECT_EQ("pos", ca->name);
    EXPECT_EQ("pos_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    /* Type # 2 */
    EXPECT_EQ("Lamp", res.entityTypeVector[1]->type);
    EXPECT_EQ(1, res.entityTypeVector[1]->count);
    ASSERT_EQ(2, res.entityTypeVector[1]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[1]->contextAttributeVector, "status");
    EXPECT_EQ("status", ca->name);
    EXPECT_EQ("status_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[1]->contextAttributeVector, "battery");
    EXPECT_EQ("battery", ca->name);
    EXPECT_EQ("battery_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    /* Type # 3 */
    EXPECT_EQ("Room", res.entityTypeVector[2]->type);
    EXPECT_EQ(2, res.entityTypeVector[2]->count);
    ASSERT_EQ(3, res.entityTypeVector[2]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[2]->contextAttributeVector, "humidity");
    EXPECT_EQ("humidity", ca->name);
    EXPECT_EQ("humidity_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[2]->contextAttributeVector, "temp");
    EXPECT_EQ("temp", ca->name);
    EXPECT_EQ("temp_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[2]->contextAttributeVector, "pos");
    EXPECT_EQ("pos", ca->name);
    EXPECT_EQ("pos_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryAllPaginationDetails -
*
*/
TEST(mongoQueryTypes, queryAllPaginationDetails)
{
    HttpStatusCode         ms;
    EntityTypeVectorResponse    res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    /* Using default offset/limit */
    unsigned int totalTypes = 0;  // enables count details
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams, V1, &totalTypes, false);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("Count: 3", res.statusCode.details);

    ASSERT_EQ(3, res.entityTypeVector.size());
    ContextAttribute* ca;

    /* Type # 1 */
    EXPECT_EQ("Car", res.entityTypeVector[0]->type);
    EXPECT_EQ(3, res.entityTypeVector[0]->count);
    ASSERT_EQ(5, res.entityTypeVector[0]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "fuel");
    EXPECT_EQ("fuel", ca->name);
    EXPECT_EQ("fuel_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "plate");
    EXPECT_EQ("plate", ca->name);
    EXPECT_EQ("plate_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "temp");
    EXPECT_EQ("temp", ca->name);
    EXPECT_EQ("temp_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "colour");
    EXPECT_EQ("colour", ca->name);
    EXPECT_EQ("colour_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "pos");
    EXPECT_EQ("pos", ca->name);
    EXPECT_EQ("pos_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    /* Type # 2 */
    EXPECT_EQ("Lamp", res.entityTypeVector[1]->type);
    EXPECT_EQ(1, res.entityTypeVector[1]->count);
    ASSERT_EQ(2, res.entityTypeVector[1]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[1]->contextAttributeVector, "status");
    EXPECT_EQ("status", ca->name);
    EXPECT_EQ("status_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[1]->contextAttributeVector, "battery");
    EXPECT_EQ("battery", ca->name);
    EXPECT_EQ("battery_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    /* Type # 3 */
    EXPECT_EQ("Room", res.entityTypeVector[2]->type);
    EXPECT_EQ(2, res.entityTypeVector[2]->count);
    ASSERT_EQ(3, res.entityTypeVector[2]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[2]->contextAttributeVector, "humidity");
    EXPECT_EQ("humidity", ca->name);
    EXPECT_EQ("humidity_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[2]->contextAttributeVector, "temp");
    EXPECT_EQ("temp", ca->name);
    EXPECT_EQ("temp_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[2]->contextAttributeVector, "pos");
    EXPECT_EQ("pos", ca->name);
    EXPECT_EQ("pos_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryAllPaginationAll -
*
*/
TEST(mongoQueryTypes, queryAllPaginationAll)
{
    HttpStatusCode         ms;
    EntityTypeVectorResponse    res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    /* Using default offset/limit */
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams, V1, NULL, false);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(3, res.entityTypeVector.size());
    ContextAttribute* ca;

    /* Type # 1 */
    EXPECT_EQ("Car", res.entityTypeVector[0]->type);
    EXPECT_EQ(3, res.entityTypeVector[0]->count);
    ASSERT_EQ(5, res.entityTypeVector[0]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "fuel");
    EXPECT_EQ("fuel", ca->name);
    EXPECT_EQ("fuel_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "plate");
    EXPECT_EQ("plate", ca->name);
    EXPECT_EQ("plate_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "temp");
    EXPECT_EQ("temp", ca->name);
    EXPECT_EQ("temp_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "colour");
    EXPECT_EQ("colour", ca->name);
    EXPECT_EQ("colour_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "pos");
    EXPECT_EQ("pos", ca->name);
    EXPECT_EQ("pos_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    /* Type # 2 */
    EXPECT_EQ("Lamp", res.entityTypeVector[1]->type);
    EXPECT_EQ(1, res.entityTypeVector[1]->count);
    ASSERT_EQ(2, res.entityTypeVector[1]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[1]->contextAttributeVector, "status");
    EXPECT_EQ("status", ca->name);
    EXPECT_EQ("status_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[1]->contextAttributeVector, "battery");
    EXPECT_EQ("battery", ca->name);
    EXPECT_EQ("battery_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    /* Type # 3 */
    EXPECT_EQ("Room", res.entityTypeVector[2]->type);
    EXPECT_EQ(2, res.entityTypeVector[2]->count);
    ASSERT_EQ(3, res.entityTypeVector[2]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[2]->contextAttributeVector, "humidity");
    EXPECT_EQ("humidity", ca->name);
    EXPECT_EQ("humidity_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[2]->contextAttributeVector, "temp");
    EXPECT_EQ("temp", ca->name);
    EXPECT_EQ("temp_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[2]->contextAttributeVector, "pos");
    EXPECT_EQ("pos", ca->name);
    EXPECT_EQ("pos_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryAllPaginationOnlyFirst -
*
*/
TEST(mongoQueryTypes, queryAllPaginationOnlyFirst)
{
    HttpStatusCode         ms;
    EntityTypeVectorResponse    res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_LIMIT] = "1";
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams, V1, NULL, false);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(1, res.entityTypeVector.size());
    ContextAttribute* ca;

    /* Type # 1 */
    EXPECT_EQ("Car", res.entityTypeVector[0]->type);
    EXPECT_EQ(3, res.entityTypeVector[0]->count);
    ASSERT_EQ(5, res.entityTypeVector[0]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "fuel");
    EXPECT_EQ("fuel", ca->name);
    EXPECT_EQ("fuel_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "plate");
    EXPECT_EQ("plate", ca->name);
    EXPECT_EQ("plate_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "temp");
    EXPECT_EQ("temp", ca->name);
    EXPECT_EQ("temp_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "colour");
    EXPECT_EQ("colour", ca->name);
    EXPECT_EQ("colour_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "pos");
    EXPECT_EQ("pos", ca->name);
    EXPECT_EQ("pos_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryAllPaginationOnlySecond -
*
*/
TEST(mongoQueryTypes, queryAllPaginationOnlySecond)
{
    HttpStatusCode         ms;
    EntityTypeVectorResponse    res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "1";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "1";
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams, V1, NULL, false);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(1, res.entityTypeVector.size());
    ContextAttribute* ca;

    /* Type # 2 */
    EXPECT_EQ("Lamp", res.entityTypeVector[0]->type);
    EXPECT_EQ(1, res.entityTypeVector[0]->count);
    ASSERT_EQ(2, res.entityTypeVector[0]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "status");
    EXPECT_EQ("status", ca->name);
    EXPECT_EQ("status_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "battery");
    EXPECT_EQ("battery", ca->name);
    EXPECT_EQ("battery_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryAllPaginationRange -
*
*/
TEST(mongoQueryTypes, queryAllPaginationRange)
{
    HttpStatusCode         ms;
    EntityTypeVectorResponse    res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "1";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "2";
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams, V1, NULL, false);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(2, res.entityTypeVector.size());
    ContextAttribute* ca;

    /* Type # 1 */
    EXPECT_EQ("Lamp", res.entityTypeVector[0]->type);
    EXPECT_EQ(1, res.entityTypeVector[0]->count);
    ASSERT_EQ(2, res.entityTypeVector[0]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "status");
    EXPECT_EQ("status", ca->name);
    EXPECT_EQ("status_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "battery");
    EXPECT_EQ("battery", ca->name);
    EXPECT_EQ("battery_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    /* Type # 2 */
    EXPECT_EQ("Room", res.entityTypeVector[1]->type);
    EXPECT_EQ(2, res.entityTypeVector[1]->count);
    ASSERT_EQ(3, res.entityTypeVector[1]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[1]->contextAttributeVector, "humidity");
    EXPECT_EQ("humidity", ca->name);
    EXPECT_EQ("humidity_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[1]->contextAttributeVector, "temp");
    EXPECT_EQ("temp", ca->name);
    EXPECT_EQ("temp_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[1]->contextAttributeVector, "pos");
    EXPECT_EQ("pos", ca->name);
    EXPECT_EQ("pos_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryAllPaginationNonExisting -
*
*/
TEST(mongoQueryTypes, queryAllPaginationNonExisting)
{
    HttpStatusCode         ms;
    EntityTypeVectorResponse    res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "7";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "3";
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams, V1, NULL, false);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.statusCode.code);
    EXPECT_EQ("No context element found", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(0, res.entityTypeVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryAllPaginationNonExistingOverlap -
*
*/
TEST(mongoQueryTypes, queryAllPaginationNonExistingOverlap)
{
    HttpStatusCode         ms;
    EntityTypeVectorResponse    res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "2";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "4";
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams, V1, NULL, false);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(1, res.entityTypeVector.size());
    ContextAttribute* ca;

    /* Type # 1 */
    EXPECT_EQ("Room", res.entityTypeVector[0]->type);
    EXPECT_EQ(2, res.entityTypeVector[0]->count);
    ASSERT_EQ(3, res.entityTypeVector[0]->contextAttributeVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "humidity");
    EXPECT_EQ("humidity", ca->name);
    EXPECT_EQ("humidity_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "temp");
    EXPECT_EQ("temp", ca->name);
    EXPECT_EQ("temp_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    ca = getAttr(res.entityTypeVector[0]->contextAttributeVector, "pos");
    EXPECT_EQ("pos", ca->name);
    EXPECT_EQ("pos_T", ca->type);
    EXPECT_EQ("", ca->stringValue);
    EXPECT_EQ(NULL, ca->compoundValueP);
    EXPECT_EQ(0, ca->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryAllPaginationNonExistingDetails -
*
*/
TEST(mongoQueryTypes, queryAllPaginationNonExistingDetails)
{
    HttpStatusCode         ms;
    EntityTypeVectorResponse    res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "7";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "3";
    unsigned int totalTypes                = 0;  // enables count details
    ms = mongoEntityTypes(&res, "", servicePathVector, uriParams, V1, &totalTypes, false);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.statusCode.code);
    EXPECT_EQ("No context element found", res.statusCode.reasonPhrase);
    EXPECT_EQ("Number of types: 3. Offset is 7", res.statusCode.details);

    ASSERT_EQ(0, res.entityTypeVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryAllDbException -
*
*/
TEST(mongoQueryTypes, queryAllDbException)
{
  HttpStatusCode         ms;
  EntityTypeVectorResponse    res;

  /* Prepare mock */
  const DBException e = DBException("boom!!", 33);
  DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
  ON_CALL(*connectionMock, runCommand(_, _, _, _))
      .WillByDefault(Throw(e));

  utInit();

  /* Set MongoDB connection */
  DBClientBase* connectionDb = getMongoConnection();
  setMongoConnectionForUnitTest(connectionMock);

  /* Invoke the function in mongoBackend library */
  ms = mongoEntityTypes(&res, "", servicePathVector, uriParams, V1, NULL, false);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  EXPECT_EQ(SccReceiverInternalError, res.statusCode.code);
  EXPECT_EQ("Internal Server Error", res.statusCode.reasonPhrase);
  EXPECT_EQ("Database Error (collection: utest "
            "- runCommand(): { aggregate: \"entities\", "
            "pipeline: [ { $match: { _id.servicePath: { $in: [ /^/.*/, null ] } } }, "
            "{ $project: { _id: 1, attrNames: 1 } }, "
            "{ $project: { attrNames: { $cond: [ { $eq: [ \"$attrNames\", [] ] }, [ null ], \"$attrNames\" ] } } }, "
            "{ $unwind: \"$attrNames\" }, "
            "{ $group: { _id: \"$_id.type\", attrs: { $addToSet: \"$attrNames\" } } }, { $sort: { _id: 1 } } ] } "
            "- exception: boom!!)", res.statusCode.details);
  EXPECT_EQ(0, res.entityTypeVector.size());

  /* Restore real DB connection */
  setMongoConnectionForUnitTest(connectionDb);

  /* Release mock */
  delete connectionMock;

  utExit();
}



/* ****************************************************************************
*
* queryAllGenericException -
*
*/
TEST(mongoQueryTypes, queryAllGenericException)
{
  HttpStatusCode            ms;
  EntityTypeVectorResponse  res;

  /* Prepare mock */
  const std::exception e;
  DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
  ON_CALL(*connectionMock, runCommand(_, _, _, _))
      .WillByDefault(Throw(e));

  utInit();

  /* Set MongoDB connection */
  DBClientBase* connectionDb = getMongoConnection();
  setMongoConnectionForUnitTest(connectionMock);

  /* Invoke the function in mongoBackend library */
  ms = mongoEntityTypes(&res, "", servicePathVector, uriParams, V1, NULL, false);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccReceiverInternalError, res.statusCode.code);
  EXPECT_EQ("Internal Server Error", res.statusCode.reasonPhrase);
  EXPECT_EQ("Database Error (collection: utest "
            "- runCommand(): { aggregate: \"entities\", "
            "pipeline: [ { $match: { _id.servicePath: { $in: [ /^/.*/, null ] } } }, "
            "{ $project: { _id: 1, attrNames: 1 } }, "
            "{ $project: { attrNames: { $cond: [ { $eq: [ \"$attrNames\", [] ] }, "
            "[ null ], \"$attrNames\" ] } } }, "
            "{ $unwind: \"$attrNames\" }, "
            "{ $group: { _id: \"$_id.type\", "
            "attrs: { $addToSet: \"$attrNames\" } } }, { $sort: { _id: 1 } } ] } "
            "- exception: std::exception)", res.statusCode.details);
  EXPECT_EQ(0, res.entityTypeVector.size());

  /* Restore real DB connection */
  setMongoConnectionForUnitTest(connectionDb);

  /* Release mock */
  delete connectionMock;

  utExit();
}



/* ****************************************************************************
*
* queryGivenTypeBasic -
*
*/
TEST(mongoQueryTypes, queryGivenTypeBasic)
{
    HttpStatusCode                ms;
    EntityTypeResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    ms = mongoAttributesForEntityType("Car", &res, "", servicePathVector, uriParams, false, V1);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    EXPECT_EQ(3, res.entityType.count);
    ASSERT_EQ(5, res.entityType.contextAttributeVector.size());

    /* Attr # 1 */
    EXPECT_EQ("colour", res.entityType.contextAttributeVector[0]->name);
    EXPECT_EQ("colour_T", res.entityType.contextAttributeVector[0]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[0]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[0]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[0]->metadataVector.size());

    /* Attr 2 */
    EXPECT_EQ("fuel", res.entityType.contextAttributeVector[1]->name);
    EXPECT_EQ("fuel_T", res.entityType.contextAttributeVector[1]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[1]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[1]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[1]->metadataVector.size());

    /* Attr 3 */
    EXPECT_EQ("plate", res.entityType.contextAttributeVector[2]->name);
    EXPECT_EQ("plate_T", res.entityType.contextAttributeVector[2]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[2]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[2]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[2]->metadataVector.size());

    /* Attr 4 */
    EXPECT_EQ("pos", res.entityType.contextAttributeVector[3]->name);
    EXPECT_EQ("pos_T", res.entityType.contextAttributeVector[3]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[3]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[3]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[3]->metadataVector.size());

    /* Attr 5 */
    EXPECT_EQ("temp", res.entityType.contextAttributeVector[4]->name);
    EXPECT_EQ("temp_T", res.entityType.contextAttributeVector[4]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[4]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[4]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[4]->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryGivenTypePaginationDetails -
*
*/
TEST(mongoQueryTypes, queryGivenTypePaginationDetails)
{
    HttpStatusCode               ms;
    EntityTypeResponse res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "on";
    /* Using default offset/limit */
    ms = mongoAttributesForEntityType("Car", &res, "", servicePathVector, uriParams, false, V1);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("Count: 5", res.statusCode.details);

    EXPECT_EQ(3, res.entityType.count);
    ASSERT_EQ(5, res.entityType.contextAttributeVector.size());

    /* Attr # 1 */
    EXPECT_EQ("colour", res.entityType.contextAttributeVector[0]->name);
    EXPECT_EQ("colour_T", res.entityType.contextAttributeVector[0]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[0]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[0]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[0]->metadataVector.size());

    /* Attr 2 */
    EXPECT_EQ("fuel", res.entityType.contextAttributeVector[1]->name);
    EXPECT_EQ("fuel_T", res.entityType.contextAttributeVector[1]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[1]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[1]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[1]->metadataVector.size());

    /* Attr 3 */
    EXPECT_EQ("plate", res.entityType.contextAttributeVector[2]->name);
    EXPECT_EQ("plate_T", res.entityType.contextAttributeVector[2]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[2]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[2]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[2]->metadataVector.size());

    /* Attr 4 */
    EXPECT_EQ("pos", res.entityType.contextAttributeVector[3]->name);
    EXPECT_EQ("pos_T", res.entityType.contextAttributeVector[3]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[3]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[3]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[3]->metadataVector.size());

    /* Attr 5 */
    EXPECT_EQ("temp", res.entityType.contextAttributeVector[4]->name);
    EXPECT_EQ("temp_T", res.entityType.contextAttributeVector[4]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[4]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[4]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[4]->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryGivenTypePaginationAll -
*
*/
TEST(mongoQueryTypes, queryGivenTypePaginationAll)
{
    HttpStatusCode               ms;
    EntityTypeResponse res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    /* Using default offset/limit */
    ms = mongoAttributesForEntityType("Car", &res, "", servicePathVector, uriParams, false, V1);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    EXPECT_EQ(3, res.entityType.count);
    ASSERT_EQ(5, res.entityType.contextAttributeVector.size());

    /* Attr # 1 */
    EXPECT_EQ("colour", res.entityType.contextAttributeVector[0]->name);
    EXPECT_EQ("colour_T", res.entityType.contextAttributeVector[0]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[0]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[0]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[0]->metadataVector.size());

    /* Attr 2 */
    EXPECT_EQ("fuel", res.entityType.contextAttributeVector[1]->name);
    EXPECT_EQ("fuel_T", res.entityType.contextAttributeVector[1]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[1]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[1]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[1]->metadataVector.size());

    /* Attr 3 */
    EXPECT_EQ("plate", res.entityType.contextAttributeVector[2]->name);
    EXPECT_EQ("plate_T", res.entityType.contextAttributeVector[2]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[2]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[2]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[2]->metadataVector.size());

    /* Attr 4 */
    EXPECT_EQ("pos", res.entityType.contextAttributeVector[3]->name);
    EXPECT_EQ("pos_T", res.entityType.contextAttributeVector[3]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[3]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[3]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[3]->metadataVector.size());

    /* Attr 5 */
    EXPECT_EQ("temp", res.entityType.contextAttributeVector[4]->name);
    EXPECT_EQ("temp_T", res.entityType.contextAttributeVector[4]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[4]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[4]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[4]->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryGivenTypePaginationOnlyFirst -
*
*/
TEST(mongoQueryTypes, queryGivenTypePaginationOnlyFirst)
{
    HttpStatusCode               ms;
    EntityTypeResponse res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_LIMIT] = "1";
    ms = mongoAttributesForEntityType("Car", &res, "", servicePathVector, uriParams, false, V1);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(1, res.entityType.contextAttributeVector.size());

    /* Attr # 1 */
    EXPECT_EQ("colour", res.entityType.contextAttributeVector[0]->name);
    EXPECT_EQ("colour_T", res.entityType.contextAttributeVector[0]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[0]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[0]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[0]->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryGivenTypePaginationOnlySecond -
*
*/
TEST(mongoQueryTypes, queryGivenTypePaginationOnlySecond)
{
    HttpStatusCode               ms;
    EntityTypeResponse res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "1";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "1";
    ms = mongoAttributesForEntityType("Car", &res, "", servicePathVector, uriParams, false, V1);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    EXPECT_EQ(3, res.entityType.count);
    ASSERT_EQ(1, res.entityType.contextAttributeVector.size());

    /* Attr 1 */
    EXPECT_EQ("fuel", res.entityType.contextAttributeVector[0]->name);
    EXPECT_EQ("fuel_T", res.entityType.contextAttributeVector[0]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[0]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[0]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[0]->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryGivenTypePaginationRange -
*
*/
TEST(mongoQueryTypes, queryGivenTypePaginationRange)
{
    HttpStatusCode               ms;
    EntityTypeResponse res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "2";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "3";
    ms = mongoAttributesForEntityType("Car", &res, "", servicePathVector, uriParams, false, V1);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    EXPECT_EQ(3, res.entityType.count);
    ASSERT_EQ(3, res.entityType.contextAttributeVector.size());

    /* Attr 3 */
    EXPECT_EQ("plate", res.entityType.contextAttributeVector[0]->name);
    EXPECT_EQ("plate_T", res.entityType.contextAttributeVector[0]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[0]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[0]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[0]->metadataVector.size());

    /* Attr 4 */
    EXPECT_EQ("pos", res.entityType.contextAttributeVector[1]->name);
    EXPECT_EQ("pos_T", res.entityType.contextAttributeVector[1]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[1]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[1]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[1]->metadataVector.size());

    /* Attr 5 */
    EXPECT_EQ("temp", res.entityType.contextAttributeVector[2]->name);
    EXPECT_EQ("temp_T", res.entityType.contextAttributeVector[2]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[2]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[2]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[2]->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryGivenTypePaginationNonExisting -
*
*/
TEST(mongoQueryTypes, queryGivenTypePaginationNonExisting)
{
    HttpStatusCode               ms;
    EntityTypeResponse res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "7";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "3";
    ms = mongoAttributesForEntityType("Car", &res, "", servicePathVector, uriParams, false, V1);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.statusCode.code);
    EXPECT_EQ("No context element found", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    ASSERT_EQ(0, res.entityType.contextAttributeVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryGivenTypePaginationNonExistingOverlap -
*
*/
TEST(mongoQueryTypes, queryGivenTypePaginationNonExistingOverlap)
{
    HttpStatusCode               ms;
    EntityTypeResponse res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "off";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "3";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "3";
    ms = mongoAttributesForEntityType("Car", &res, "", servicePathVector, uriParams, false, V1);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.statusCode.code);
    EXPECT_EQ("OK", res.statusCode.reasonPhrase);
    EXPECT_EQ("", res.statusCode.details);

    EXPECT_EQ(3, res.entityType.count);
    ASSERT_EQ(2, res.entityType.contextAttributeVector.size());

    /* Attr 1 */
    EXPECT_EQ("pos", res.entityType.contextAttributeVector[0]->name);
    EXPECT_EQ("pos_T", res.entityType.contextAttributeVector[0]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[0]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[0]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[0]->metadataVector.size());

    /* Attr 2 */
    EXPECT_EQ("temp", res.entityType.contextAttributeVector[1]->name);
    EXPECT_EQ("temp_T", res.entityType.contextAttributeVector[1]->type);
    EXPECT_EQ("", res.entityType.contextAttributeVector[1]->stringValue);
    EXPECT_EQ(NULL, res.entityType.contextAttributeVector[1]->compoundValueP);
    EXPECT_EQ(0, res.entityType.contextAttributeVector[1]->metadataVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryGivenTypePaginationNonExistingDetails -
*
*/
TEST(mongoQueryTypes, queryGivenTypePaginationNonExistingDetails)
{
    HttpStatusCode               ms;
    EntityTypeResponse res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Invoke the function in mongoBackend library */
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "on";
    uriParams[URI_PARAM_PAGINATION_OFFSET] = "7";
    uriParams[URI_PARAM_PAGINATION_LIMIT]  = "3";
    ms = mongoAttributesForEntityType("Car", &res, "", servicePathVector, uriParams, false, V1);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.statusCode.code);
    EXPECT_EQ("No context element found", res.statusCode.reasonPhrase);
    EXPECT_EQ("Number of attributes: 5. Offset is 7", res.statusCode.details);

    ASSERT_EQ(0, res.entityType.contextAttributeVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryGivenTypeDbException -
*
*/
TEST(mongoQueryTypes, queryGivenTypeDbException)
{
  HttpStatusCode               ms;
  EntityTypeResponse res;

  /* Prepare mock */
  const DBException e = DBException("boom!!", 33);
  DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
  ON_CALL(*connectionMock, runCommand(_, _, _, _))
      .WillByDefault(Throw(e));

  utInit();

  /* Set MongoDB connection */
  DBClientBase* connectionDb = getMongoConnection();
  setMongoConnectionForUnitTest(connectionMock);

  /* Invoke the function in mongoBackend library */
  ms = mongoAttributesForEntityType("Car", &res, "", servicePathVector, uriParams, false, V1);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccReceiverInternalError, res.statusCode.code);
  EXPECT_EQ("Internal Server Error", res.statusCode.reasonPhrase);
  EXPECT_EQ("Database Error (collection: utest "
            "- runCommand(): { aggregate: \"entities\", "
            "pipeline: [ { $match: { _id.type: \"Car\", _id.servicePath: { $in: [ /^/.*/, null ] } } }, "
            "{ $project: { _id: 1, attrNames: 1 } }, { $unwind: \"$attrNames\" }, "
            "{ $group: { _id: \"$_id.type\", attrs: { $addToSet: \"$attrNames\" } } }, "
            "{ $unwind: \"$attrs\" }, { $group: { _id: \"$attrs\" } }, { $sort: { _id: 1 } } ] } "
            "- exception: boom!!)", res.statusCode.details);
  EXPECT_EQ(0, res.entityType.contextAttributeVector.size());

  /* Restore real DB connection */
  setMongoConnectionForUnitTest(connectionDb);

  /* Release mock */
  delete connectionMock;

  utExit();
}

/* ****************************************************************************
*
* queryGivenTypeGenericException -
*
*/
TEST(mongoQueryTypes, queryGivenTypeGenericException)
{
  HttpStatusCode               ms;
  EntityTypeResponse res;

  /* Prepare mock */
  const std::exception e;
  DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
  ON_CALL(*connectionMock, runCommand(_, _, _, _))
      .WillByDefault(Throw(e));

  utInit();

  /* Set MongoDB connection */
  DBClientBase* connectionDb = getMongoConnection();
  setMongoConnectionForUnitTest(connectionMock);

  /* Invoke the function in mongoBackend library */
  ms = mongoAttributesForEntityType("Car", &res, "", servicePathVector, uriParams, false, V1);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccReceiverInternalError, res.statusCode.code);
  EXPECT_EQ("Internal Server Error", res.statusCode.reasonPhrase);
  EXPECT_EQ("Database Error (collection: utest "
            "- runCommand(): { aggregate: \"entities\", "
            "pipeline: [ { $match: { _id.type: \"Car\", "
            "_id.servicePath: { $in: [ /^/.*/, null ] } } }, "
            "{ $project: { _id: 1, attrNames: 1 } }, "
            "{ $unwind: \"$attrNames\" }, "
            "{ $group: { _id: \"$_id.type\", "
            "attrs: { $addToSet: \"$attrNames\" } } }, "
            "{ $unwind: \"$attrs\" }, { $group: { _id: \"$attrs\" } }, { $sort: { _id: 1 } } ] } "
            "- exception: std::exception)", res.statusCode.details);
  EXPECT_EQ(0, res.entityType.contextAttributeVector.size());

  /* Restore real DB connection */
  setMongoConnectionForUnitTest(connectionDb);

  /* Release mock */
  delete connectionMock;

  utExit();
}
