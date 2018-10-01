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
#include <string>

#include "unittests/unittest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

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
using mongo::BSONArray;
using mongo::BSONNULL;
using mongo::DBException;
using orion::ValueTypeString;
using orion::ValueTypeNumber;
using orion::ValueTypeBoolean;
using orion::ValueTypeNull;
using ::testing::Throw;
using ::testing::_;



extern void setMongoConnectionForUnitTest(DBClientBase* _connection);



/* ****************************************************************************
*
* Tests
*
* Note that these tests are similar in structure to the ones in DiscoverContextAvailability,
* due to both operations behaves quite similar regarding entities and attributes matching
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
* With servicePath:
*
* - queryWithServicePathEntPatternType_2levels
* - queryWithServicePathEntPatternType_1level
* - queryWithServicePathEntPatternType_0levels
* - queryWithServicePathEntPatternType_1levelbis
* - queryWithIdenticalEntitiesButDifferentServicePath_case1
* - queryWithIdenticalEntitiesButDifferentServicePath_case2
* - queryWithIdenticalEntitiesButDifferentServicePath_case3
* - queryWithIdenticalEntitiesButDifferentServicePath_case4
* - queryWithServicePathEntPatternNoType_2levels
* - queryWithServicePathEntPatternNoType_1level
* - queryWithServicePathEntPatternNoType_0levels
* - queryWithServicePathEntPatternNoType_1levelbis
* - queryWithServicePathEntNoPatternTypeFail
* - queryWithServicePathEntNoPatternTypeOk
* - queryWithServicePathEntNoPatternNoType
* - queryWithSeveralServicePaths
*
* With isPattern=false:
*
* - query1Ent0Attr
* - query1Ent1Attr
* - queryNEnt0Attr
* - queryNEnt1AttrSingle
* - queryNEnt1AttrMulti
* - queryNEntNAttr
* - query1Ent0AttrFail
* - query1Ent1AttrFail
* - query1EntWA0AttrFail
* - query1EntWA1Attr
* - queryNEntWA0Attr
* - queryNEntWA1Attr
* - queryNoType
* - queryIdMetadata
* - queryCustomMetadata
* - queryCustomMetadataNative
*
* (N=2 without loss of generality)
* (WA = Without Attributes)
*
* With isPattern=true:
*
* - queryPattern0Attr
* - queryPattern1AttrSingle
* - queryPattern1AttrMulti
* - queryPatternNAttr
* - queryPatternFail
* - queryMixPatternAndNotPattern
* - queryNoTypePattern
* - queryIdMetadataPattern
* - queryCustomMetadataPattern
*
* Some bacic test with typePattern true
*
*  - queryTypePattern
*
* Native types
*
* - queryNativeTypes
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
* entities collection.
*/
static void prepareDatabase(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following entities:
   *
   * - E1:
   *     A1: val1
   *     A2: val2
   * - E2
   *     A2: val2bis
   *     A3: val3
   * - E4
   *     (no attrs)
   * - E1*
   *     A1: val1bis
   * - E1**
   *     A1: val1bis1
   *
   * (*) Means that entity/type is using same name but different type. This is included to check that type is
   *     taken into account.
   * (**)same name but without type
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrNames" << BSON_ARRAY("A1" << "A2") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "TA1" << "value" << "val1") <<
                       "A2" << BSON("type" << "TA2" << "value" << "val2")));

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T2") <<
                     "attrNames" << BSON_ARRAY("A2" << "A3") <<
                     "attrs" << BSON(
                       "A2" << BSON("type" << "TA2" << "value" << "val2bis") <<
                       "A3" << BSON("type" << "TA3" << "value" << "val3")));

  BSONObj en4 = BSON("_id" << BSON("id" << "E4" << "type" << "T4") <<
                     "attrNames" << BSONArray() <<
                     "attrs" << BSONObj());

  BSONObj en5 = BSON("_id" << BSON("id" << "E1" << "type" << "T1bis") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "TA1" << "value" << "val1bis")));

  BSONObj en6 = BSON("_id" << BSON("id" << "E1") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "TA1" << "value" << "val1bis1")));

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en4);
  connection->insert(ENTITIES_COLL, en5);
  connection->insert(ENTITIES_COLL, en6);
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

  /* We create the following entities:
   *
   * - E1:
   *     A1: val1
   *     A2: val2
   * - E2
   *     A2: val2bis
   *     A3: val3
   * - E1*:
   *     A4: val4
   *     A5: val5
   * - E4
   *     (no attrs)
   * - E2**
   *     A2: val2bis1
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A1" << "A2") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "TA1" << "value" << "val1") <<
                       "A2" << BSON("type" << "TA2" << "value" << "val2")));

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A2" << "A3") <<
                     "attrs" << BSON(
                       "A2" << BSON("type" << "TA2" << "value" << "val2bis") <<
                       "A3" << BSON("type" << "TA3" << "value" << "val3")));

  BSONObj en4 = BSON("_id" << BSON("id" << "E4" << "type" << "T") <<
                     "attrNames" << BSONArray() <<
                     "attrs" << BSONObj());

  BSONObj en5 = BSON("_id" << BSON("id" << "E1" << "type" << "Tbis") <<
                     "attrNames" << BSON_ARRAY("A4" << "A5") <<
                     "attrs" << BSON(
                       "A4" << BSON("type" << "TA4" << "value" << "val4") <<
                       "A5" << BSON("type" << "TA5" << "value" << "val5")));

  BSONObj en6 = BSON("_id" << BSON("id" << "E2") <<
                     "attrNames" << BSON_ARRAY("A2") <<
                     "attrs" << BSON(
                       "A2" << BSON("type" << "TA2" << "value" << "val2bis1")));

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en4);
  connection->insert(ENTITIES_COLL, en5);
  connection->insert(ENTITIES_COLL, en6);
}



/* ****************************************************************************
*
* prepareDatabaseWithAttributeIds -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabaseWithAttributeIds(void)
{
    /* Start with the base entities */
    prepareDatabase();

    /* Add some entities with metadata ID */

    DBClientBase* connection = getMongoConnection();
    BSONObj en1 = BSON("_id" << BSON("id" << "E10" << "type" << "T") <<
                       "attrNames" << BSON_ARRAY("A1" << "A2") <<
                       "attrs" << BSON(
                         "A1()ID1" << BSON("type" << "TA1" << "value" << "A") <<
                         "A1()ID2" << BSON("type" << "TA1" << "value" << "B") <<
                         "A2"      << BSON("type" << "TA2" << "value" << "D")));

    BSONObj en2 = BSON("_id" << BSON("id" << "E11" << "type" << "T") <<
                       "attrNames" << BSON_ARRAY("A1" << "A2") <<
                       "attrs" << BSON(
                         "A1()ID1" << BSON("type" << "TA1" << "value" << "E") <<
                         "A1()ID2" << BSON("type" << "TA1" << "value" << "F") <<
                         "A2"      << BSON("type" << "TA2" << "value" << "H")));

    connection->insert(ENTITIES_COLL, en1);
    connection->insert(ENTITIES_COLL, en2);
}



/* ****************************************************************************
*
* prepareDatabaseWithAttributeCustomMetadata -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabaseWithCustomMetadata(void)
{
    /* Start with the base entities */
    prepareDatabase();

    /* Add some entities with metadata ID */

    DBClientBase* connection = getMongoConnection();
    BSONObj en1 = BSON("_id" << BSON("id" << "E10" << "type" << "T") <<
                       "attrNames" << BSON_ARRAY("A1" << "A2") <<
                       "attrs" << BSON(
                         "A1" << BSON("type" << "TA1" << "value" << "A" <<
                                      "md" << BSON("MD1" << BSON("type" << "TMD1" << "value" << "1") <<
                                                   "MD2" << BSON("type" << "TMD2" << "value" << "2")) <<
                                      "mdNames" << BSON_ARRAY("MD1" << "MD2")) <<
                         "A2" << BSON("type" << "TA2" << "value" << "C" <<
                                      "md" << BSON("MD1" << BSON("type" << "TMD1" << "value" << "5") <<
                                                   "MD2" << BSON("type" << "TMD2" << "value" << "6")) <<
                                      "mdNames" << BSON_ARRAY("MD1" << "MD2"))));

    BSONObj en2 = BSON("_id" << BSON("id" << "E11" << "type" << "T") <<
                       "attrNames" << BSON_ARRAY("A1" << "A2") <<
                       "attrs" << BSON(
                         "A1" << BSON("type" << "TA1" << "value" << "D" <<
                                      "md" << BSON("MD1" << BSON("type" << "TMD1" << "value" << "7") <<
                                                   "MD2" << BSON("type" << "TMD2" << "value" << "8")) <<
                                      "mdNames" << BSON_ARRAY("MD1" << "MD2")) <<
                         "A2" << BSON("type" << "TA2" << "value" << "F" <<
                                      "md" << BSON("MD1" << BSON("type" << "TMD1" << "value" << "11") <<
                                                   "MD2" << BSON("type" << "TMD2" << "value" << "12")) <<
                                      "mdNames" << BSON_ARRAY("MD1" << "MD2"))));

    connection->insert(ENTITIES_COLL, en1);
    connection->insert(ENTITIES_COLL, en2);
}

/* ****************************************************************************
*
* prepareDatabaseWithAttributeCustomMetadataNative -
*
*/
static void prepareDatabaseWithCustomMetadataNative(void)
{
    /* Start with the base entities */
    prepareDatabase();

    /* Add some entities with metadata ID */

    DBClientBase* connection = getMongoConnection();
    BSONObj en1 = BSON("_id" << BSON("id" << "E10" << "type" << "T") <<
                       "attrNames" << BSON_ARRAY("A1" << "A2") <<
                       "attrs" << BSON(
                         "A1" << BSON("type" << "TA1" << "value" << "A" <<
                                      "md" << BSON("MD1" << BSON("type" << "TMD1" << "value" << "val1") <<
                                                   "MD2" << BSON("type" << "TMD2" << "value" << 2.1) <<
                                                   "MD3" << BSON("type" << "TMD3" << "value" << false) <<
                                                   "MD4" << BSON("type" << "TMD4" << "value" << BSONNULL)) <<
                                      "mdNames" << BSON_ARRAY("MD1" << "MD2" << "MD3" << "MD4")) <<
                         "A2" << BSON("type" << "TA2" << "value" << "C" <<
                                      "md" << BSON("MD1" << BSON("type" << "TMD1" << "value" << false) <<
                                                   "MD2" << BSON("type" << "TMD2" << "value" << 6.5)) <<
                                      "mdNames" << BSON_ARRAY("MD1" << "MD2"))));

    BSONObj en2 = BSON("_id" << BSON("id" << "E11" << "type" << "T") <<
                       "attrNames" << BSON_ARRAY("A1" << "A2") <<
                       "attrs" << BSON(
                         "A1" << BSON("type" << "TA1" << "value" << "D" <<
                                      "md" << BSON("MD1" << BSON("type" << "TMD1" << "value" << "x") <<
                                                   "MD2" << BSON("type" << "TMD2" << "value" << 8.7)) <<
                                      "mdNames" << BSON_ARRAY("MD1" << "MD2")) <<
                         "A2" << BSON("type" << "TA2" << "value" << "F" <<
                                      "md" << BSON("MD1" << BSON("type" << "TMD1" << "value" << true) <<
                                                   "MD2" << BSON("type" << "TMD2" << "value" << "val2")) <<
                                      "mdNames" << BSON_ARRAY("MD1" << "MD2"))));

    connection->insert(ENTITIES_COLL, en1);
    connection->insert(ENTITIES_COLL, en2);
}

/* ****************************************************************************
*
* prepareDatabaseWithServicePath -
*
*/
static void prepareDatabaseWithServicePath(const std::string& modifier)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following entities:
   *
   * - E1:  { Type: T, ServicePath: /home,        Attribute: { A1, a1  } }
   * - E2:  { Type: T, ServicePath: /home/kz,     Attribute: { A1, a2  } }
   * - E3:  { Type: T, ServicePath: /home/fg,     Attribute: { A1, a3  } }
   * - E4:  { Type: T, ServicePath: /home/kz/e4,  Attribute: { A1, a4  } }
   * - E5:  { Type: T, ServicePath: /home/kz/e5,  Attribute: { A1, a5  } }
   * - E6:  { Type: T, ServicePath: /home/fg/e6,  Attribute: { A1, a6  } }
   * - E7:  { Type: T, ServicePath: /home2,       Attribute: { A1, a7  } }
   * - E8:  { Type: T, ServicePath: /home2/kz,    Attribute: { A1, a8  } }
   * - E9:  { Type: T, ServicePath: "",           Attribute: { A1, a9  } }
   * - E10: { Type: T, ServicePath: NO,           Attribute: { A1, a10 } }
   * - E11: { Type: T, ServicePath: /home3/e11,   Attribute: { A1, a11 } }
   * - E12: { Type: T, ServicePath: /home3/e12,   Attribute: { A1, a12 } }
   * - E13: { Type: T, ServicePath: /home3/e13,   Attribute: { A1, a13 } }
   *
   */

  BSONObj e01 = BSON("_id" << BSON("id" << "E1"  << "type" << "T" << "servicePath" << "/home")       <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a1")));

  BSONObj e02 = BSON("_id" << BSON("id" << "E2"  << "type" << "T" << "servicePath" << "/home/kz")    <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a2")));

  BSONObj e03 = BSON("_id" << BSON("id" << "E3"  << "type" << "T" << "servicePath" << "/home/fg")    <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a3")));

  BSONObj e04 = BSON("_id" << BSON("id" << "E4"  << "type" << "T" << "servicePath" << "/home/kz/e4") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a4")));

  BSONObj e05 = BSON("_id" << BSON("id" << "E5"  << "type" << "T" << "servicePath" << "/home/kz/e5") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a5")));

  BSONObj e06 = BSON("_id" << BSON("id" << "E6"  << "type" << "T" << "servicePath" << "/home/fg/e6") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a6")));

  BSONObj e07 = BSON("_id" << BSON("id" << "E7"  << "type" << "T" << "servicePath" << "/home2")      <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a7")));

  BSONObj e08 = BSON("_id" << BSON("id" << "E8"  << "type" << "T" << "servicePath" << "/home2/kz")   <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a8")));

  BSONObj e09 = BSON("_id" << BSON("id" << "E9"  << "type" << "T" << "servicePath" << "")            <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a9")));

  BSONObj e10 = BSON("_id" << BSON("id" << "E10" << "type" << "T")                                   <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a10")));


  BSONObj e11 = BSON("_id" << BSON("id" << "E11" << "type" << "T" << "servicePath" << "/home3/e11")   <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a11")));

  BSONObj e12 = BSON("_id" << BSON("id" << "E12" << "type" << "T" << "servicePath" << "/home3/e12")   <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a12")));

  BSONObj e13 = BSON("_id" << BSON("id" << "E13" << "type" << "T" << "servicePath" << "/home3/e13")   <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a13")));


  connection->insert(ENTITIES_COLL, e01);
  connection->insert(ENTITIES_COLL, e02);
  connection->insert(ENTITIES_COLL, e03);
  connection->insert(ENTITIES_COLL, e04);
  connection->insert(ENTITIES_COLL, e05);
  connection->insert(ENTITIES_COLL, e06);
  connection->insert(ENTITIES_COLL, e07);
  connection->insert(ENTITIES_COLL, e08);
  connection->insert(ENTITIES_COLL, e09);
  connection->insert(ENTITIES_COLL, e10);
  connection->insert(ENTITIES_COLL, e11);
  connection->insert(ENTITIES_COLL, e12);
  connection->insert(ENTITIES_COLL, e13);

  if (modifier == "")
    return;

  if (modifier == "patternNoType")
  {
    BSONObj e = BSON("_id" << BSON("id" << "E" << "type" << "OOO" << "servicePath" << "/home/kz/123") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "ae_1")));

    connection->insert(ENTITIES_COLL, e);
  }
  else if (modifier == "noPatternNoType")
  {
    BSONObj e = BSON("_id" << BSON("id" << "E3" << "type" << "OOO" << "servicePath" << "/home/fg/124") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "ae_2")));

    connection->insert(ENTITIES_COLL, e);
  }
  else if (modifier == "IdenticalEntitiesButDifferentServicePaths")
  {
    BSONObj ie1 = BSON("_id" << BSON("id" << "IE" << "type" << "T" << "servicePath" << "/home/fg/01") <<
                       "attrNames" << BSON_ARRAY("A1") <<
                       "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "ie_01")));

    BSONObj ie2 = BSON("_id" << BSON("id" << "IE" << "type" << "T" << "servicePath" << "/home/fg/02") <<
                       "attrNames" << BSON_ARRAY("A1") <<
                       "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "ie_02")));

    BSONObj ie3 = BSON("_id" << BSON("id" << "IE" << "type" << "T" << "servicePath" << "/home/fg/03") <<
                       "attrNames" << BSON_ARRAY("A1") <<
                       "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "ie_03")));

    connection->insert(ENTITIES_COLL, ie1);
    connection->insert(ENTITIES_COLL, ie2);
    connection->insert(ENTITIES_COLL, ie3);
  }
}

/* ****************************************************************************
*
* prepareDatabaseWithServicePath -
*
*/
static void prepareDatabaseForPagination(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following entities:
   *
   * - E1:  { Type: T, Attribute: { A1, a1  } }
   * - E2:  { Type: T, Attribute: { A1, a2  } }
   * - E3:  { Type: T, Attribute: { A1, a3  } }
   * - E4:  { Type: T, Attribute: { A1, a4  } }
   * - E5:  { Type: T, Attribute: { A1, a5  } }
   * - E6:  { Type: T, Attribute: { A1, a6  } }
   *
   */

  BSONObj e01 = BSON("_id" << BSON("id" << "E1"  << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a1")));

  BSONObj e02 = BSON("_id" << BSON("id" << "E2"  << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a2")));

  BSONObj e03 = BSON("_id" << BSON("id" << "E3"  << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a3")));

  BSONObj e04 = BSON("_id" << BSON("id" << "E4"  << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a4")));

  BSONObj e05 = BSON("_id" << BSON("id" << "E5"  << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a5")));

  BSONObj e06 = BSON("_id" << BSON("id" << "E6"  << "type" << "T") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON("A1" << BSON("type" << "TA1" << "value" << "a6")));

  connection->insert(ENTITIES_COLL, e01);
  connection->insert(ENTITIES_COLL, e02);
  connection->insert(ENTITIES_COLL, e03);
  connection->insert(ENTITIES_COLL, e04);
  connection->insert(ENTITIES_COLL, e05);
  connection->insert(ENTITIES_COLL, e06);
}

/* ****************************************************************************
*
* prepareDatabaseDifferentNativeTypes -
*
*/
static void prepareDatabaseDifferentNativeTypes(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following entities:
   *
   * - E1:
   *     A1: string
   *     A2: number
   *     A2: bool
   *     A3: vector
   *     A5: object
   *     A6: null
   *
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrNames" << BSON_ARRAY("A1" << "A2" << "A3" << "A4" << "A5" << "A6") <<
                     "attrs" << BSON(
                        "A1" << BSON("type" << "T" << "value" << "s") <<
                        "A2" << BSON("type" << "T" << "value" << 42.0) <<
                        "A3" << BSON("type" << "T" << "value" << false) <<
                        "A4" << BSON("type" << "T" << "value" << BSON("x" << "a" << "y" << "b")) <<
                        "A5" << BSON("type" << "T" << "value" << BSON_ARRAY("x1" << "x2")) <<
                        "A6" << BSON("type" << "T" << "value" << BSONNULL)));

  connection->insert(ENTITIES_COLL, en1);
}



/* ****************************************************************************
*
* paginationDetails -
*/
TEST(mongoQueryContextRequest, paginationDetails)
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
    uriParams[URI_PARAM_PAGINATION_DETAILS]  = "on";

    /* Invoke the function in mongoBackend library */
    long long count;
    ms = mongoQueryContext(&req, &res, "", servicePathVector , uriParams, options, &count);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("Count: 6", res.errorCode.details);

    ASSERT_EQ(6, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("a1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).id);
    EXPECT_EQ("T", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(1, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("a2", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E3", RES_CER(2).id);
    EXPECT_EQ("T", RES_CER(2).type);
    EXPECT_EQ("false", RES_CER(2).isPattern);
    ASSERT_EQ(1, RES_CER(2).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ("a3", RES_CER_ATTR(2, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Context Element response # 4 */
    EXPECT_EQ("E4", RES_CER(3).id);
    EXPECT_EQ("T", RES_CER(3).type);
    EXPECT_EQ("false", RES_CER(3).isPattern);
    ASSERT_EQ(1, RES_CER(3).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(3, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(3, 0)->type);
    EXPECT_EQ("a4", RES_CER_ATTR(3, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
    EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

    /* Context Element response # 5 */
    EXPECT_EQ("E5", RES_CER(4).id);
    EXPECT_EQ("T", RES_CER(4).type);
    EXPECT_EQ("false", RES_CER(4).isPattern);
    ASSERT_EQ(1, RES_CER(4).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(4, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(4, 0)->type);
    EXPECT_EQ("a5", RES_CER_ATTR(4, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(4).code);
    EXPECT_EQ("OK", RES_CER_STATUS(4).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(4).details.size());

    /* Context Element response # 6 */
    EXPECT_EQ("E6", RES_CER(5).id);
    EXPECT_EQ("T", RES_CER(5).type);
    EXPECT_EQ("false", RES_CER(5).isPattern);
    ASSERT_EQ(1, RES_CER(5).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(5, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(5, 0)->type);
    EXPECT_EQ("a6", RES_CER_ATTR(5, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(5).code);
    EXPECT_EQ("OK", RES_CER_STATUS(5).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(5).details.size());

    utExit();
}

/* ****************************************************************************
*
* paginationAll -
*
*/
TEST(mongoQueryContextRequest, paginationAll)
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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(6, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("a1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).id);
    EXPECT_EQ("T", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(1, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("a2", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E3", RES_CER(2).id);
    EXPECT_EQ("T", RES_CER(2).type);
    EXPECT_EQ("false", RES_CER(2).isPattern);
    ASSERT_EQ(1, RES_CER(2).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ("a3", RES_CER_ATTR(2, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Context Element response # 4 */
    EXPECT_EQ("E4", RES_CER(3).id);
    EXPECT_EQ("T", RES_CER(3).type);
    EXPECT_EQ("false", RES_CER(3).isPattern);
    ASSERT_EQ(1, RES_CER(3).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(3, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(3, 0)->type);
    EXPECT_EQ("a4", RES_CER_ATTR(3, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
    EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

    /* Context Element response # 5 */
    EXPECT_EQ("E5", RES_CER(4).id);
    EXPECT_EQ("T", RES_CER(4).type);
    EXPECT_EQ("false", RES_CER(4).isPattern);
    ASSERT_EQ(1, RES_CER(4).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(4, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(4, 0)->type);
    EXPECT_EQ("a5", RES_CER_ATTR(4, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(4).code);
    EXPECT_EQ("OK", RES_CER_STATUS(4).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(4).details.size());

    /* Context Element response # 6 */
    EXPECT_EQ("E6", RES_CER(5).id);
    EXPECT_EQ("T", RES_CER(5).type);
    EXPECT_EQ("false", RES_CER(5).isPattern);
    ASSERT_EQ(1, RES_CER(5).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(5, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(5, 0)->type);
    EXPECT_EQ("a6", RES_CER_ATTR(5, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(5).code);
    EXPECT_EQ("OK", RES_CER_STATUS(5).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(5).details.size());

    utExit();
}

/* ****************************************************************************
*
* paginationOnlyFirst -
*
*/
TEST(mongoQueryContextRequest, paginationOnlyFirst)
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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("a1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    utExit();
}

/* ****************************************************************************
*
* paginationOnlySecond -
*
*/
TEST(mongoQueryContextRequest, paginationOnlySecond)
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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E2", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("a2", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    utExit();
}

/* ****************************************************************************
*
* paginationRange -
*
*/
TEST(mongoQueryContextRequest, paginationRange)
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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("a3", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E4", RES_CER(1).id);
    EXPECT_EQ("T", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(1, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("a4", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E5", RES_CER(2).id);
    EXPECT_EQ("T", RES_CER(2).type);
    EXPECT_EQ("false", RES_CER(2).isPattern);
    ASSERT_EQ(1, RES_CER(2).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ("a5", RES_CER_ATTR(2, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    utExit();
}

/* ****************************************************************************
*
* paginationNonExisting -
*
*/
TEST(mongoQueryContextRequest, paginationNonExisting)
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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    utExit();
}

/* ****************************************************************************
*
* paginationNonExistingOverlap -
*
*/
TEST(mongoQueryContextRequest, paginationNonExistingOverlap)
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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E6", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("a6", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    utExit();
}

/* ****************************************************************************
*
* paginationNonExistingDetails -
*
*/
TEST(mongoQueryContextRequest, paginationNonExistingDetails)
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
    long long count;
    ms = mongoQueryContext(&req, &res, "", servicePathVector , uriParams, options, &count);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ("Number of matching entities: 6. Offset is 7", res.errorCode.details);

    ASSERT_EQ(0, res.contextElementResponseVector.size());

    utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntPatternType_2levels -
*
*/
TEST(mongoQueryContextRequest, queryWithServicePathEntPatternType_2levels)
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
  servicePathVector.push_back("/home/kz/#");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(3, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E2", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a2", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E4", RES_CER(1).id);
  EXPECT_EQ("T", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  ASSERT_EQ(1, RES_CER(1).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a4", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Context Element response # 3 */
  EXPECT_EQ("E5", RES_CER(2).id);
  EXPECT_EQ("T", RES_CER(2).type);
  EXPECT_EQ("false", RES_CER(2).isPattern);
  ASSERT_EQ(1, RES_CER(2).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("a5", RES_CER_ATTR(2, 0)->stringValue);
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
TEST(mongoQueryContextRequest, queryWithServicePathEntPatternType_1level)
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
  servicePathVector.push_back("/home/#");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(6, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a1", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E2", RES_CER(1).id);
  EXPECT_EQ("T", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  ASSERT_EQ(1, RES_CER(1).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a2", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Context Element response # 3 */
  EXPECT_EQ("E3", RES_CER(2).id);
  EXPECT_EQ("T", RES_CER(2).type);
  EXPECT_EQ("false", RES_CER(2).isPattern);
  ASSERT_EQ(1, RES_CER(2).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("a3", RES_CER_ATTR(2, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

  /* Context Element response # 4 */
  EXPECT_EQ("E4", RES_CER(3).id);
  EXPECT_EQ("T", RES_CER(3).type);
  EXPECT_EQ("false", RES_CER(3).isPattern);
  ASSERT_EQ(1, RES_CER(3).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(3, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(3, 0)->type);
  EXPECT_EQ("a4", RES_CER_ATTR(3, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
  EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

  /* Context Element response # 5 */
  EXPECT_EQ("E5", RES_CER(4).id);
  EXPECT_EQ("T", RES_CER(4).type);
  EXPECT_EQ("false", RES_CER(4).isPattern);
  ASSERT_EQ(1, RES_CER(4).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(4, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(4, 0)->type);
  EXPECT_EQ("a5", RES_CER_ATTR(4, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(4).code);
  EXPECT_EQ("OK", RES_CER_STATUS(4).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(4).details.size());

  /* Context Element response # 6 */
  EXPECT_EQ("E6", RES_CER(5).id);
  EXPECT_EQ("T", RES_CER(5).type);
  EXPECT_EQ("false", RES_CER(5).isPattern);
  ASSERT_EQ(1, RES_CER(5).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(5, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(5, 0)->type);
  EXPECT_EQ("a6", RES_CER_ATTR(5, 0)->stringValue);
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
TEST(mongoQueryContextRequest, queryWithServicePathEntPatternType_0levels)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("patternType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1\\d", "T", "true");
  req.entityIdVector.push_back(&en);
  servicePathVector.clear();

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);


  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(4, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E10", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a10", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E11", RES_CER(1).id);
  EXPECT_EQ("T", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  ASSERT_EQ(1, RES_CER(1).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a11", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Context Element response # 3 */
  EXPECT_EQ("E12", RES_CER(2).id);
  EXPECT_EQ("T", RES_CER(2).type);
  EXPECT_EQ("false", RES_CER(2).isPattern);
  ASSERT_EQ(1, RES_CER(2).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("a12", RES_CER_ATTR(2, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

  /* Context Element response # 4 */
  EXPECT_EQ("E13", RES_CER(3).id);
  EXPECT_EQ("T", RES_CER(3).type);
  EXPECT_EQ("false", RES_CER(3).isPattern);
  ASSERT_EQ(1, RES_CER(3).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(3, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(3, 0)->type);
  EXPECT_EQ("a13", RES_CER_ATTR(3, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
  EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntPatternType_1levelbis -
*
*/
TEST(mongoQueryContextRequest, queryWithServicePathEntPatternType_1levelbis)
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
  servicePathVector.push_back("/home2/#");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E7", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a7", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E8", RES_CER(1).id);
  EXPECT_EQ("T", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  ASSERT_EQ(1, RES_CER(1).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a8", RES_CER_ATTR(1, 0)->stringValue);
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
TEST(mongoQueryContextRequest, queryWithIdenticalEntitiesButDifferentServicePaths_case1)
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
  servicePathVector.push_back("/home/fg/#");

  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(3, res.contextElementResponseVector.size());

  EXPECT_EQ("IE", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("ie_01", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  EXPECT_EQ("IE", RES_CER(1).id);
  EXPECT_EQ("T", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  ASSERT_EQ(1, RES_CER(1).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("ie_02", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  EXPECT_EQ("IE", RES_CER(2).id);
  EXPECT_EQ("T", RES_CER(2).type);
  EXPECT_EQ("false", RES_CER(2).isPattern);
  ASSERT_EQ(1, RES_CER(2).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("ie_03", RES_CER_ATTR(2, 0)->stringValue);
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
TEST(mongoQueryContextRequest, queryWithIdenticalEntitiesButDifferentServicePaths_case2)
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

  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  EXPECT_EQ("IE", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("ie_01", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}

/* ****************************************************************************
*
* queryWithIdenticalEntitiesButDifferentServicePaths_case3 -
*
*/
TEST(mongoQueryContextRequest, queryWithIdenticalEntitiesButDifferentServicePaths_case3)
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

  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  EXPECT_EQ("IE", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("ie_02", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}



/* ****************************************************************************
*
* queryWithIdenticalEntitiesButDifferentServicePaths_case4 -
*/
TEST(mongoQueryContextRequest, queryWithIdenticalEntitiesButDifferentServicePaths_case4)
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

  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  EXPECT_EQ("IE", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("ie_03", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntPatternNoType_2levels -
*
*/
TEST(mongoQueryContextRequest, queryWithServicePathEntPatternNoType_2levels)
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
  servicePathVector.push_back("/home/kz/#");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(4, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E2", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a2", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E4", RES_CER(1).id);
  EXPECT_EQ("T", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  ASSERT_EQ(1, RES_CER(1).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a4", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Context Element response # 3 */
  EXPECT_EQ("E5", RES_CER(2).id);
  EXPECT_EQ("T", RES_CER(2).type);
  EXPECT_EQ("false", RES_CER(2).isPattern);
  ASSERT_EQ(1, RES_CER(2).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("a5", RES_CER_ATTR(2, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

  /* Context Element response # 4 */
  EXPECT_EQ("E", RES_CER(3).id);
  EXPECT_EQ("OOO", RES_CER(3).type);
  EXPECT_EQ("false", RES_CER(3).isPattern);
  ASSERT_EQ(1, RES_CER(3).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(3, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(3, 0)->type);
  EXPECT_EQ("ae_1", RES_CER_ATTR(3, 0)->stringValue);
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
TEST(mongoQueryContextRequest, queryWithServicePathEntPatternNoType_1level)
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
  servicePathVector.push_back("/home/#");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(7, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a1", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E2", RES_CER(1).id);
  EXPECT_EQ("T", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  ASSERT_EQ(1, RES_CER(1).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a2", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Context Element response # 3 */
  EXPECT_EQ("E3", RES_CER(2).id);
  EXPECT_EQ("T", RES_CER(2).type);
  EXPECT_EQ("false", RES_CER(2).isPattern);
  ASSERT_EQ(1, RES_CER(2).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("a3", RES_CER_ATTR(2, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

  /* Context Element response # 4 */
  EXPECT_EQ("E4", RES_CER(3).id);
  EXPECT_EQ("T", RES_CER(3).type);
  EXPECT_EQ("false", RES_CER(3).isPattern);
  ASSERT_EQ(1, RES_CER(3).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(3, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(3, 0)->type);
  EXPECT_EQ("a4", RES_CER_ATTR(3, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
  EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

  /* Context Element response # 5 */
  EXPECT_EQ("E5", RES_CER(4).id);
  EXPECT_EQ("T", RES_CER(4).type);
  EXPECT_EQ("false", RES_CER(4).isPattern);
  ASSERT_EQ(1, RES_CER(4).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(4, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(4, 0)->type);
  EXPECT_EQ("a5", RES_CER_ATTR(4, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(4).code);
  EXPECT_EQ("OK", RES_CER_STATUS(4).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(4).details.size());

  /* Context Element response # 6 */
  EXPECT_EQ("E6", RES_CER(5).id);
  EXPECT_EQ("T", RES_CER(5).type);
  EXPECT_EQ("false", RES_CER(5).isPattern);
  ASSERT_EQ(1, RES_CER(5).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(5, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(5, 0)->type);
  EXPECT_EQ("a6", RES_CER_ATTR(5, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(5).code);
  EXPECT_EQ("OK", RES_CER_STATUS(5).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(5).details.size());

  /* Context Element response # 7 */
  EXPECT_EQ("E", RES_CER(6).id);
  EXPECT_EQ("OOO", RES_CER(6).type);
  EXPECT_EQ("false", RES_CER(6).isPattern);
  ASSERT_EQ(1, RES_CER(6).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(6, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(6, 0)->type);
  EXPECT_EQ("ae_1", RES_CER_ATTR(6, 0)->stringValue);
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
TEST(mongoQueryContextRequest, queryWithServicePathEntPatternNoType_0levels)
{
  HttpStatusCode         ms;
  QueryContextRequest    req;
  QueryContextResponse   res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePath("patternNoType");

  /* Forge the request (from "inside" to "outside") */
  EntityId en("E1\\d", "", "true");
  req.entityIdVector.push_back(&en);
  servicePathVector.clear();

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(4, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E10", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a10", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E11", RES_CER(1).id);
  EXPECT_EQ("T", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  ASSERT_EQ(1, RES_CER(1).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a11", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Context Element response # 3 */
  EXPECT_EQ("E12", RES_CER(2).id);
  EXPECT_EQ("T", RES_CER(2).type);
  EXPECT_EQ("false", RES_CER(2).isPattern);
  ASSERT_EQ(1, RES_CER(2).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
  EXPECT_EQ("a12", RES_CER_ATTR(2, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
  EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

  /* Context Element response # 4 */
  EXPECT_EQ("E13", RES_CER(3).id);
  EXPECT_EQ("T", RES_CER(3).type);
  EXPECT_EQ("false", RES_CER(3).isPattern);
  ASSERT_EQ(1, RES_CER(3).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(3, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(3, 0)->type);
  EXPECT_EQ("a13", RES_CER_ATTR(3, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
  EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntPatternNoType_1levelbis -
*
*/
TEST(mongoQueryContextRequest, queryWithServicePathEntPatternNoType_1levelbis)
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
  servicePathVector.push_back("/home2/#");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E7", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a7", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E8", RES_CER(1).id);
  EXPECT_EQ("T", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  ASSERT_EQ(1, RES_CER(1).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a8", RES_CER_ATTR(1, 0)->stringValue);
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
TEST(mongoQueryContextRequest, queryWithServicePathEntNoPatternTypeFail)
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
  servicePathVector.clear();
  servicePathVector.push_back("/home/kz");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

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
TEST(mongoQueryContextRequest, queryWithServicePathEntNoPatternTypeOk)
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
  servicePathVector.clear();
  servicePathVector.push_back("/home/fg");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(1, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E3", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a3", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  utExit();
}

/* ****************************************************************************
*
* queryWithServicePathEntNoPatternNoType -
*
*/
TEST(mongoQueryContextRequest, queryWithServicePathEntNoPatternNoType)
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
  servicePathVector.clear();
  servicePathVector.push_back("/home/fg/#");

  /* Invoke the function in mongoBackend library */
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccNone, res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E3", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a3", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E3", RES_CER(1).id);
  EXPECT_EQ("OOO", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  ASSERT_EQ(1, RES_CER(1).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("ae_2", RES_CER_ATTR(1, 0)->stringValue);
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
TEST(mongoQueryContextRequest, queryWithSeveralServicePaths)
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
  ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(0 , res.errorCode.code);
  EXPECT_EQ("", res.errorCode.reasonPhrase);
  EXPECT_EQ("", res.errorCode.details);

  ASSERT_EQ(2, res.contextElementResponseVector.size());

  /* Context Element response # 1 */
  EXPECT_EQ("E4", RES_CER(0).id);
  EXPECT_EQ("T", RES_CER(0).type);
  EXPECT_EQ("false", RES_CER(0).isPattern);
  ASSERT_EQ(1, RES_CER(0).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ("a4", RES_CER_ATTR(0, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Context Element response # 2 */
  EXPECT_EQ("E12", RES_CER(1).id);
  EXPECT_EQ("T", RES_CER(1).type);
  EXPECT_EQ("false", RES_CER(1).isPattern);
  ASSERT_EQ(1, RES_CER(1).attributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ("a12", RES_CER_ATTR(1, 0)->stringValue);
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  utExit();
}

/* ****************************************************************************
*
* query1Ent0Attr -
*
* Query:     E1 - no attrs
* Result:    E1 - (A1, A2)
*
* This test also checks that querying for type (E1) doesn't match with no-typed
* entities (E1** is not returned)
*
*/
TEST(mongoQueryContextRequest, query1Ent0Attr)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* query1Ent1Attr -
*
* Query:     E1 - A1
* Result:    E1 - A1
*/
TEST(mongoQueryContextRequest, query1Ent1Attr)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    utExit();
}

/* ****************************************************************************
*
* queryNEnt0Attr -
*
* Query:     (E1, E2) -no attrs
* Result:    E1 - (A1, A2)
*            E2 - (A3, A4)
*/
TEST(mongoQueryContextRequest, queryNEnt0Attr)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).id);
    EXPECT_EQ("T2", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(2, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryNEnt1AttrSingle -
*
* Query:     (E1, E2) - A1
* Result:    E1 - A1
*/
TEST(mongoQueryContextRequest, queryNEnt1AttrSingle)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryNEnt1AttrMulti -
*
* Query:     (E1, E2) - A2
* Result:    E1 - A2
*            E2 - A2
*/
TEST(mongoQueryContextRequest, queryNEnt1AttrMulti)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).id);
    EXPECT_EQ("T2", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(1, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryNEntNAttr -
*
* Query:     (E1, E2) - (A1, A3)
* Result:    E1 - A1
*            E2 - A3
*/
TEST(mongoQueryContextRequest, queryNEntNAttr)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).id);
    EXPECT_EQ("T2", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(1, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A3", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* query1Ent0AttrFail -
*
* Query:     E3 - no attrs
* Result:    none
*/
TEST(mongoQueryContextRequest, query1Ent0AttrFail)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E3", "T3", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);
    EXPECT_EQ(0, res.contextElementResponseVector.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* query1Ent1AttrFail -
*
* Query:     E1 - A3
* Result:    none
*/
TEST(mongoQueryContextRequest, query1Ent1AttrFail)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);
    EXPECT_EQ(0, res.contextElementResponseVector.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}


/* ****************************************************************************
*
* query1EntWA0AttrFail -
*
* Query:     E4 - A1
* Result:    none
*/
TEST(mongoQueryContextRequest, query1EntWA0AttrFail)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccContextElementNotFound, res.errorCode.code);
    EXPECT_EQ("No context element found", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);
    EXPECT_EQ(0, res.contextElementResponseVector.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* query1EntWA1Attr -
*
* Query:     E4 - none
* Result:    E4 - none
*/
TEST(mongoQueryContextRequest, query1EntWA1Attr)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E4", "T4", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E4", RES_CER(0).id);
    EXPECT_EQ("T4", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    EXPECT_EQ(0, RES_CER(0).attributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryNEntWA0Attr -
*
* Query:     (E1, E4) - none
* Result:    E1 - A1, A2
*            E4 - none
*/
TEST(mongoQueryContextRequest, queryNEntWA0Attr)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E4", RES_CER(1).id);
    EXPECT_EQ("T4", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    EXPECT_EQ(0, RES_CER(1).attributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryNEntWA1Attr -
*
* Query:     (E1, E4) - A1
* Result:    E1 - A1
*/
TEST(mongoQueryContextRequest, queryNEntWA1Attr)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    utExit();
}

/* ****************************************************************************
*
* queryNoType -
*
* Query:     E1
* Result:    E1   - A1
*            E1*  - A1
*            E1** - A1
*
* Note that this case checks matching of no-type in the query for both the case in
* which the returned entity has type (E1 and E1*) and the case in which it has no type (E1**)
*
*/
TEST(mongoQueryContextRequest, queryNoType)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response #2 */
    EXPECT_EQ("E1", RES_CER(1).id);
    EXPECT_EQ("T1bis", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(1, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val1bis", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response #3 */
    EXPECT_EQ("E1", RES_CER(2).id);
    EXPECT_EQ(0, RES_CER(2).type.size());
    EXPECT_EQ("false", RES_CER(2).isPattern);
    ASSERT_EQ(1, RES_CER(2).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ("val1bis1", RES_CER_ATTR(2, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    utExit();
}

/* ****************************************************************************
*
* queryIdMetadata -
*
*/
TEST(mongoQueryContextRequest, queryIdMetadata)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("A", RES_CER_ATTR(0, 0)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("B", RES_CER_ATTR(0, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->metadataVector[0]->type);
    EXPECT_EQ("ID2", RES_CER_ATTR(0, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    utExit();
}

/* ****************************************************************************
*
* queryCustomMetadata -
*
*/
TEST(mongoQueryContextRequest, queryCustomMetadata)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("A", RES_CER_ATTR(0, 0)->stringValue);
    ASSERT_EQ(2, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("1", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector[1]->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 0)->metadataVector[1]->type);
    EXPECT_EQ("2", RES_CER_ATTR(0, 0)->metadataVector[1]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    utExit();
}


/* ****************************************************************************
*
* queryCustomMetadataNatibve -
*
*/
TEST(mongoQueryContextRequest, queryCustomMetadataNative)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabaseWithCustomMetadataNative();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E10", "T", "false");
    req.entityIdVector.push_back(&en);
    req.attributeList.push_back("A1");

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("A", RES_CER_ATTR(0, 0)->stringValue);
    ASSERT_EQ(4, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->metadataVector[0]->valueType);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector[1]->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 0)->metadataVector[1]->type);
    EXPECT_EQ(2.1, RES_CER_ATTR(0, 0)->metadataVector[1]->numberValue);
    EXPECT_EQ(orion::ValueTypeNumber, RES_CER_ATTR(0, 0)->metadataVector[1]->valueType);
    EXPECT_EQ("MD3", RES_CER_ATTR(0, 0)->metadataVector[2]->name);
    EXPECT_EQ("TMD3", RES_CER_ATTR(0, 0)->metadataVector[2]->type);
    EXPECT_FALSE(RES_CER_ATTR(0, 0)->metadataVector[2]->boolValue);
    EXPECT_EQ(orion::ValueTypeBoolean, RES_CER_ATTR(0, 0)->metadataVector[2]->valueType);
    EXPECT_EQ("MD4", RES_CER_ATTR(0, 0)->metadataVector[3]->name);
    EXPECT_EQ("TMD4", RES_CER_ATTR(0, 0)->metadataVector[3]->type);
    EXPECT_EQ(orion::ValueTypeNull, RES_CER_ATTR(0, 0)->metadataVector[3]->valueType);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    utExit();
}


/* ****************************************************************************
*
* queryPattern0Attr -
*
* Query:     E[1-2] - none
* Result:    E1 - (A1, A2)
*            E2 - (A2, A3)
*
* This test also checks that querying for type (E[1-2]) doesn't match with no-typed
* entities (E1** is not returned)
*
*/
TEST(mongoQueryContextRequest, queryPattern0Attr)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "T", "true");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).id);
    EXPECT_EQ("T", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(2, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    utExit();
}

/* ****************************************************************************
*
* queryPattern1AttrSingle -
*
* Query:     E[1-2] - A1
* Result:    E1 - A1
*/
TEST(mongoQueryContextRequest, queryPattern1AttrSingle)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    utExit();
}

/* ****************************************************************************
*
* queryPattern1AttrMulti -
*
* Query:     E[1-2] - A2
* Result:    E1 - A2
*            E2 - A2
*/
TEST(mongoQueryContextRequest, queryPattern1AttrMulti)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).id);
    EXPECT_EQ("T", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(1, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    utExit();
}

/* ****************************************************************************
*
* queryPatternNAttr -
*
* Query:     E[1-2] - (A1, A2)
* Result:    E1 - (A1, A2)
*            E2 - A2
*/
TEST(mongoQueryContextRequest, queryPatternNAttr)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).id);
    EXPECT_EQ("T", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(1, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    utExit();
}

/* ****************************************************************************
*
* queryPatternFail -
*
* Query:     R.* - none
* Result:    none
*/
TEST(mongoQueryContextRequest, queryPatternFail)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("R.*", "T", "true");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

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
* queryMixPatternAndNotPattern -
*
* Query:     (E[1-2]. E4) - none
* Result:    E1 - (A1, A2)
*            E2 - (A2 ,A3)
*            E4 - none
*/
TEST(mongoQueryContextRequest, queryMixPatternAndNotPattern)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).id);
    EXPECT_EQ("T", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(2, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E4", RES_CER(2).id);
    EXPECT_EQ("T", RES_CER(2).type);
    EXPECT_EQ("false", RES_CER(2).isPattern);
    EXPECT_EQ(0, RES_CER(2).attributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryNoTypePattern -
*
* Query:     E[1-2]
* Result:    E1   - A1, A2
*            E2   - A2 ,A3
*            E1*  - A4, A5
*            E2** - A2
*
* Note that this case checks matching of no-type in the query for both the case in
* which the returned entity has type (E1, E2 and E1*) and the case in which it has no type (E2**)
*
*/
TEST(mongoQueryContextRequest, queryNoTypePattern)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabasePatternTrue();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E[1-2]", "", "true");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(4, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).id);
    EXPECT_EQ("T", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(2, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E1", RES_CER(2).id);
    EXPECT_EQ("Tbis", RES_CER(2).type);
    EXPECT_EQ("false", RES_CER(2).isPattern);
    ASSERT_EQ(2, RES_CER(2).attributeVector.size());
    EXPECT_EQ("A4", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA4", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ("val4", RES_CER_ATTR(2, 0)->stringValue);
    EXPECT_EQ("A5", RES_CER_ATTR(2, 1)->name);
    EXPECT_EQ("TA5", RES_CER_ATTR(2, 1)->type);
    EXPECT_EQ("val5", RES_CER_ATTR(2, 1)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Context Element response # 4 */
    EXPECT_EQ("E2", RES_CER(3).id);
    EXPECT_EQ(0, RES_CER(3).type.size());
    EXPECT_EQ("false", RES_CER(3).isPattern);
    ASSERT_EQ(1, RES_CER(3).attributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(3, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(3, 0)->type);
    EXPECT_EQ("val2bis1", RES_CER_ATTR(3, 0)->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(3).code);
    EXPECT_EQ("OK", RES_CER_STATUS(3).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(3).details.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* queryIdMetadataPattern -
*
*/
TEST(mongoQueryContextRequest, queryIdMetadataPattern)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("A", RES_CER_ATTR(0, 0)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);

    EXPECT_EQ("A1", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("B", RES_CER_ATTR(0, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->metadataVector[0]->type);
    EXPECT_EQ("ID2", RES_CER_ATTR(0, 1)->metadataVector[0]->stringValue);

    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E11", RES_CER(1).id);
    EXPECT_EQ("T", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(2, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("E", RES_CER_ATTR(1, 0)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(1, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(1, 0)->metadataVector[0]->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(1, 0)->metadataVector[0]->stringValue);

    EXPECT_EQ("A1", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("F", RES_CER_ATTR(1, 1)->stringValue);
    ASSERT_EQ(1, RES_CER_ATTR(1, 1)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(1, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(1, 1)->metadataVector[0]->type);
    EXPECT_EQ("ID2", RES_CER_ATTR(1, 1)->metadataVector[0]->stringValue);

    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    utExit();
}

/* ****************************************************************************
*
* queryCustomMetadataPattern -
*
*/
TEST(mongoQueryContextRequest, queryCustomMetadataPattern)
{
    utInit();

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
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).id);
    EXPECT_EQ("T", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(1, RES_CER(0).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("A", RES_CER_ATTR(0, 0)->stringValue);
    ASSERT_EQ(2, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("1", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector[1]->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 0)->metadataVector[1]->type);
    EXPECT_EQ("2", RES_CER_ATTR(0, 0)->metadataVector[1]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E11", RES_CER(1).id);
    EXPECT_EQ("T", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(1, RES_CER(1).attributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("D", RES_CER_ATTR(1, 0)->stringValue);
    ASSERT_EQ(2, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(1, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(1, 0)->metadataVector[0]->type);
    EXPECT_EQ("7", RES_CER_ATTR(1, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ("MD2", RES_CER_ATTR(1, 0)->metadataVector[1]->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(1, 0)->metadataVector[1]->type);
    EXPECT_EQ("8", RES_CER_ATTR(1, 0)->metadataVector[1]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    utExit();
}



/* ****************************************************************************
*
* queryTypePattern -
*
* Query:     id pattern = E.* - type Pattern = T[1-2]$ - no attrs
* Result:    E1 (all attributes) and E2 (all attributes)
*
*/
TEST(mongoQueryContextRequest, queryTypePattern)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E.*", "T[1-2]$", "true", true);
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(2, RES_CER(0).attributeVector.size());

    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("val1", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(ValueTypeString, RES_CER_ATTR(0, 0)->valueType);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());

    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ("val2", RES_CER_ATTR(0, 1)->stringValue);
    EXPECT_EQ(ValueTypeString, RES_CER_ATTR(0, 1)->valueType);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).id);
    EXPECT_EQ("T2", RES_CER(1).type);
    EXPECT_EQ("false", RES_CER(1).isPattern);
    ASSERT_EQ(2, RES_CER(1).attributeVector.size());

    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ("val2bis", RES_CER_ATTR(1, 0)->stringValue);
    EXPECT_EQ(ValueTypeString, RES_CER_ATTR(1, 0)->valueType);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());

    EXPECT_EQ("A3", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ("val3", RES_CER_ATTR(1, 1)->stringValue);
    EXPECT_EQ(ValueTypeString, RES_CER_ATTR(1, 1)->valueType);
    EXPECT_EQ(0, RES_CER_ATTR(1, 1)->metadataVector.size());

    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(1).details);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}



/* ****************************************************************************
*
* queryNativeTypes -
*
* Query:     E1 - no attrs
* Result:    E1 - (A1 string, A2 number, A3 bool, A4 vector, A5 object)
*
*/
TEST(mongoQueryContextRequest, queryNativeTypes)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare database */
    prepareDatabaseDifferentNativeTypes();

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccNone, res.errorCode.code);
    EXPECT_EQ("", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).id);
    EXPECT_EQ("T1", RES_CER(0).type);
    EXPECT_EQ("false", RES_CER(0).isPattern);
    ASSERT_EQ(6, RES_CER(0).attributeVector.size());

    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ("s", RES_CER_ATTR(0, 0)->stringValue);
    EXPECT_EQ(ValueTypeString, RES_CER_ATTR(0, 0)->valueType);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());

    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(42, RES_CER_ATTR(0, 1)->numberValue);
    EXPECT_EQ(ValueTypeNumber, RES_CER_ATTR(0, 1)->valueType);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());

    EXPECT_EQ("A3", RES_CER_ATTR(0, 2)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 2)->type);
    EXPECT_FALSE(RES_CER_ATTR(0, 2)->boolValue);
    EXPECT_EQ(ValueTypeBoolean, RES_CER_ATTR(0, 2)->valueType);
    EXPECT_EQ(0, RES_CER_ATTR(0, 2)->metadataVector.size());

    EXPECT_EQ("A4", RES_CER_ATTR(0, 3)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 3)->type);
    EXPECT_EQ(orion::ValueTypeObject, RES_CER_ATTR(0, 3)->compoundValueP->valueType);
    EXPECT_EQ("x", RES_CER_ATTR(0, 3)->compoundValueP->childV[0]->name);
    EXPECT_EQ("a", RES_CER_ATTR(0, 3)->compoundValueP->childV[0]->stringValue);
    EXPECT_EQ("y", RES_CER_ATTR(0, 3)->compoundValueP->childV[1]->name);
    EXPECT_EQ("b", RES_CER_ATTR(0, 3)->compoundValueP->childV[1]->stringValue);
    EXPECT_EQ(0, RES_CER_ATTR(0, 3)->metadataVector.size());

    EXPECT_EQ("A5", RES_CER_ATTR(0, 4)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 4)->type);
    EXPECT_EQ(orion::ValueTypeVector, RES_CER_ATTR(0, 4)->compoundValueP->valueType);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 4)->compoundValueP->childV[0]->valueType);
    EXPECT_EQ("x1", RES_CER_ATTR(0, 4)->compoundValueP->childV[0]->stringValue);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 4)->compoundValueP->childV[1]->valueType);
    EXPECT_EQ("x2", RES_CER_ATTR(0, 4)->compoundValueP->childV[1]->stringValue);
    EXPECT_EQ(0, RES_CER_ATTR(0, 4)->metadataVector.size());

    EXPECT_EQ("A6", RES_CER_ATTR(0, 5)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 5)->type);
    EXPECT_EQ(ValueTypeNull, RES_CER_ATTR(0, 5)->valueType);
    EXPECT_EQ(0, RES_CER_ATTR(0, 5)->metadataVector.size());

    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    utExit();
}

/* ****************************************************************************
*
* mongoDbQueryFail -
*
*/
TEST(mongoQueryContextRequest, mongoDbQueryFail)
{
    utInit();

    HttpStatusCode         ms;
    QueryContextRequest   req;
    QueryContextResponse  res;

    /* Prepare mock */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, _query(_, _, _, _, _, _, _))
            .WillByDefault(Throw(e));

    /* Set MongoDB connection */
    DBClientBase* connectionDb = getMongoConnection();
    setMongoConnectionForUnitTest(connectionMock);

    /* Forge the request (from "inside" to "outside") */
    EntityId en("E1", "T1", "false");
    req.entityIdVector.push_back(&en);

    /* Invoke the function in mongoBackend library */
    ms = mongoQueryContext(&req, &res, "", servicePathVector, uriParams, options);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccReceiverInternalError, res.errorCode.code);
    EXPECT_EQ("Internal Server Error", res.errorCode.reasonPhrase);
    EXPECT_EQ("Database Error (collection: utest.entities - "
              "query(): { query: { $or: [ { _id.id: \"E1\", _id.type: \"T1\" } ], "
              "_id.servicePath: { $in: [ /^/.*/, null ] } }, orderby: { creDate: 1 } } - "
              "exception: boom!!)", res.errorCode.details);
    EXPECT_EQ(0, res.contextElementResponseVector.size());

    /* Release dynamic memory used by response (mongoBackend allocates it) */
    res.contextElementResponseVector.release();

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb);

    delete connectionMock;
    utExit();
}
