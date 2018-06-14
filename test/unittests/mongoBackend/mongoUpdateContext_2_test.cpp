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
#include <vector>

#include "gtest/gtest.h"
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/globals.h"
#include "common/errorMessages.h"
#include "orionTypes/OrionValueType.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"

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
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Throw;
using ::testing::_;



extern void setMongoConnectionForUnitTest(DBClientBase* _connection);



/* ****************************************************************************
*
* Tests
*
* (Continues from mongoUpdateContext_test.cpp)
*
* With isPattern=false, related with attribute IDs
*
* - updateAttrWithId
* - updateAttrWithAndWithoutId
* - appendAttrWithId
* - appendAttrWithAndWithoutId
* - appendAttrWithIdFails
* - appendAttrWithoutIdFails
* - deleteAttrWithId
* - deleteAttrWithAndWithoutId
*
* With isPattern=true and custom metadata
*
* - appendCreateEntWithMd
* - updateMdAllExisting
* - appendMdAllExisting
* - updateMdAllNew
* - appendMdAllNew
* - updateMdSomeNew
* - updateMdSomeNew
* - appendValueAndMd
* - updateValueAndMd
* - appendMdNoActualChanges
* - updateMdNoActualChanges
*
* (N=2 without loss of generality)
*
* With isPattern=true:
*
* - patternUnsupported
*
* ?!exist=entity::type filter
*
* - notExistFilter
*
* Native types
*
* - createNativeTypes
* - updateNativeTypes
* - preservingNativeTypes
* - createMdNativeTypes
* - updateMdNativeTypes
* - preservingMdNativeTypes
*
* REPLACE operation
*
* - replace
*
* Some other NGSIv2 tests
*
* - tooManyEntitiesNGSIv2
* - onlyOneEntityNGSIv2
*
* Simulating fails in MongoDB connection:
*
* - mongoDbUpdateFail
* - mongoDbQueryFail
*
* Lastly a few tests with Service Path
*
* - servicePathEntityUpdate_3levels
* - servicePathEntityUpdate_2levels
* - servicePathEntityAppend_3levels
* - servicePathEntityAppend_2levels
* - servicePathEntityCreation_2levels
* - servicePathEntityCreation_3levels
* - servicePathEntityDeletion_2levels
* - servicePathEntityDeletion_3levels
* - servicePathEntityVectorNotAllowed
*1
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
   *     A2: (no value)
   * - E2
   *     A3: val3
   *     A4: (no value)
   * - E3
   *     A5: val5
   *     A6: (no value)
   * - E1*:
   *     A1: val1bis2
   * - E1**:
   *     A1: val1
   *     A2: (no value)
   *
   * (*) Means that entity/type is using same name but different type. This is included to check that type is
   *     taken into account.
   *
   * (**)same name but without type
   *
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrNames" << BSON_ARRAY("A1" << "A2") <<
                     "attrs" << BSON(
                        "A1" << BSON("type" << "TA1" << "value" << "val1") <<
                        "A2" << BSON("type" << "TA2")));

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T2") <<
                     "attrNames" << BSON_ARRAY("A3" << "A4") <<
                     "attrs" << BSON(
                        "A3" << BSON("type" << "TA3" << "value" << "val3") <<
                        "A4" << BSON("type" << "TA4")));

  BSONObj en3 = BSON("_id" << BSON("id" << "E3" << "type" << "T3") <<
                     "attrNames" << BSON_ARRAY("A5" << "A6") <<
                     "attrs" << BSON(
                        "A5" << BSON("type" << "TA5" << "value" << "val5") <<
                        "A6" << BSON("type" << "TA6")));

  BSONObj en4 = BSON("_id" << BSON("id" << "E1" << "type" << "T1bis") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "TA1" << "value" << "val1bis2")));

  BSONObj en1nt = BSON("_id" << BSON("id" << "E1") <<
                     "attrNames" << BSON_ARRAY("A1" << "A2") <<
                     "attrs" << BSON(
                        "A1" << BSON("type" << "TA1" << "value" << "val1-nt") <<
                        "A2" << BSON("type" << "TA2")));

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en3);
  connection->insert(ENTITIES_COLL, en4);
  connection->insert(ENTITIES_COLL, en1nt);
}



/* ****************************************************************************
*
* prepareDatabaseMd -
*
*/
static void prepareDatabaseMd(void)
{
  /* Set database */
  setupDatabase();

  /* Add an entity with custom metadata */
  DBClientBase* connection = getMongoConnection();
  BSONObj en = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                    "attrNames" << BSON_ARRAY("A1") <<
                    "attrs" << BSON(
                      "A1" << BSON("type" << "TA1" << "value" << "val1" <<
                                   "md" << BSON("MD1" << BSON("type" << "TMD1" << "value" << "MD1val") <<
                                                "MD2" << BSON("type" << "TMD2" << "value" << "MD2val")) <<
                                   "mdNames" << BSON_ARRAY("MD1" << "MD2"))));

  connection->insert(ENTITIES_COLL, en);
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
  BSONObj en = BSON("_id" << BSON("id" << "E10" << "type" << "T10") <<
                    "attrNames" << BSON_ARRAY("A1" << "A2") <<
                    "attrs" << BSON(
                      "A1()ID1" << BSON("type" << "TA1" << "value" << "val11") <<
                      "A1()ID2" << BSON("type" << "TA1" << "value" << "val12") <<
                      "A2" << BSON("type" << "TA2" << "value" << "val2")));

  connection->insert(ENTITIES_COLL, en);
}



/* ****************************************************************************
*
* prepareDatabaseWithServicePaths -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabaseWithServicePaths(void)
{
  /* Set database */
  setupDatabase();

  /* Create entities with service path */
  DBClientBase* connection = getMongoConnection();
  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1" << "servicePath" << "/home/kz/01") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "TA1" << "value" << "kz01")));

  BSONObj en2 = BSON("_id" << BSON("id" << "E1" << "type" << "T1" << "servicePath" << "/home/kz/02") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "TA1" << "value" << "kz02")));

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
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
   *     A3: bool
   *     A4: vector
   *     A5: object
   *     A6: null
   *
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrNames" << BSON_ARRAY("A1" << "A2" << "A3" << "A4" << "A5" << "A6") <<
                     "attrs" << BSON(
                        "A1" << BSON("type" << "T" << "value" << "s") <<
                        "A2" << BSON("type" << "T" << "value" << 42) <<
                        "A3" << BSON("type" << "T" << "value" << false) <<
                        "A4" << BSON("type" << "T" << "value" << BSON("x" << "a" << "y" << "b")) <<
                        "A5" << BSON("type" << "T" << "value" << BSON_ARRAY("x1" << "x2")) <<
                        "A6" << BSON("type" << "T" << "value" << BSONNULL)));

  connection->insert(ENTITIES_COLL, en1);
}



/* ****************************************************************************
*
* prepareDatabaseDifferentMdNativeTypes -
*
*/
static void prepareDatabaseDifferentMdNativeTypes(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following entities:
   *
   * - E1:
   *     MD1: string
   *     MD2: number
   *     MD3: bool
   *     MD4: null
   *
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON(
                       "A1" << BSON("type" << "TA1" << "value" << "val1" <<
                                    "md" << BSON("MD1" << BSON("type" << "T" << "value" << "s") <<
                                                 "MD2" << BSON("type" << "T" << "value" << 55.5) <<
                                                 "MD3" << BSON("type" << "T" << "value" << false) <<
                                                 "MD4" << BSON("type" << "T" << "value" << BSONNULL)) <<
                                    "mdNames" << BSON_ARRAY("MD1" << "MD2" << "MD3" << "MD4"))));

  connection->insert(ENTITIES_COLL, en1);
}



/* ****************************************************************************
*
* findAttr -
*
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
* updateAttrWithId -
*/
TEST(mongoUpdateContextRequest, updateAttrWithId)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E10", "T10", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
    Metadata md("ID", "string", "ID1");
    caP->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(3, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1id1 = attrs.getField("A1()ID1").embeddedObject();
    BSONObj a1id2 = attrs.getField("A1()ID2").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1id1, "value"));
    EXPECT_EQ(1360232700, a1id1.getIntField("modDate"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("val2", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* updateAttrWithAndWithoutId -
*/
TEST(mongoUpdateContextRequest, updateAttrWithAndWithoutId)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E10", "T10", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "new_val");
    Metadata md("ID", "string", "ID1");
    ca1P->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(ca1P);
    ContextAttribute* ca2P = new ContextAttribute("A2", "TA2", "new_val2");
    ceP->contextAttributeVector.push_back(ca2P);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
    ASSERT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(3, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1id1 = attrs.getField("A1()ID1").embeddedObject();
    BSONObj a1id2 = attrs.getField("A1()ID2").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1id1, "value"));
    EXPECT_EQ(1360232700, a1id1.getIntField("modDate"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("new_val2", C_STR_FIELD(a2, "value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* appendAttrWithId -
*/
TEST(mongoUpdateContextRequest, appendAttrWithId)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E10", "T10", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
    Metadata md("ID", "string", "ID3");
    caP->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("ID3", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(4, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1id1 = attrs.getField("A1()ID1").embeddedObject();
    BSONObj a1id2 = attrs.getField("A1()ID2").embeddedObject();
    BSONObj a1id3 = attrs.getField("A1()ID3").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id1, "type"));
    EXPECT_STREQ("val11", C_STR_FIELD(a1id1, "value"));
    EXPECT_FALSE(a1id1.hasField("modDate"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id3, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1id3, "value"));
    EXPECT_EQ(1360232700, a1id3.getIntField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("val2", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* appendAttrWithAndWithoutId -
*/
TEST(mongoUpdateContextRequest, appendAttrWithAndWithoutId)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E10", "T10", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "new_val");
    Metadata md("ID", "string", "ID3");
    ca1P->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(ca1P);
    ContextAttribute* ca2P = new ContextAttribute("A3", "TA3", "new_val3");
    ceP->contextAttributeVector.push_back(ca2P);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("ID3", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ("A3", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
    ASSERT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(5, attrs.nFields());
    ASSERT_EQ(3, attrNames.size());
    BSONObj a1id1 = attrs.getField("A1()ID1").embeddedObject();
    BSONObj a1id2 = attrs.getField("A1()ID2").embeddedObject();
    BSONObj a1id3 = attrs.getField("A1()ID3").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    a3 = attrs.getField("A3").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id1, "type"));
    EXPECT_STREQ("val11", C_STR_FIELD(a1id1, "value"));
    EXPECT_FALSE(a1id1.hasField("modDate"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id3, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1id3, "value"));
    EXPECT_EQ(1360232700, a1id3.getIntField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("val2", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("new_val3", C_STR_FIELD(a3, "value"));
    EXPECT_EQ(1360232700, a3.getIntField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* appendAttrWithIdFails -
*/
TEST(mongoUpdateContextRequest, appendAttrWithIdFails)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E10", "T10", "false");
    ContextAttribute* caP = new ContextAttribute("A2", "TA2", "new_val");
    Metadata md("ID", "string", "IDX");
    caP->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("IDX", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: APPEND - entity: [E10, T10] - offending attribute: A2 - attribute cannot be appended",
              RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(3, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1id1 = attrs.getField("A1()ID1").embeddedObject();
    BSONObj a1id2 = attrs.getField("A1()ID2").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id1, "type"));
    EXPECT_STREQ("val11", C_STR_FIELD(a1id1, "value"));
    EXPECT_FALSE(a1id1.hasField("modDate"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("val2", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* appendAttrWithoutIdFails -
*/
TEST(mongoUpdateContextRequest, appendAttrWithoutIdFails)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E10", "T10", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: APPEND - entity: [E10, T10] - offending attribute: A1 - attribute cannot be appended",
              RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(3, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1id1 = attrs.getField("A1()ID1").embeddedObject();
    BSONObj a1id2 = attrs.getField("A1()ID2").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id1, "type"));
    EXPECT_STREQ("val11", C_STR_FIELD(a1id1, "value"));
    EXPECT_FALSE(a1id1.hasField("modDate"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("val2", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* deleteAttrWithId -
*/
TEST(mongoUpdateContextRequest, deleteAttrWithId)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E10", "T10", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "");
    Metadata md("ID", "string", "ID1");
    caP->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeDelete;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1id2 = attrs.getField("A1()ID2").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("val2", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* deleteAttrWithAndWithoutId -
*/
TEST(mongoUpdateContextRequest, deleteAttrWithAndWithoutId)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E10", "T10", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "");
    Metadata md("ID", "string", "ID1");
    ca1P->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(ca1P);
    ContextAttribute* ca2P = new ContextAttribute("A2", "TA2", "");
    ceP->contextAttributeVector.push_back(ca2P);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeDelete;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
    ASSERT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1id2 = attrs.getField("A1()ID2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_FALSE(a1id2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* appendCreateEntWithMd -
*/
TEST(mongoUpdateContextRequest, appendCreateEntWithMd)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare (a clean) database */
    setupDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "val1");
    Metadata md1("MD1", "TMD1", "MD1val");
    Metadata md2("MD2", "TMD2", "MD2val");
    caP->metadataVector.push_back(&md1);
    caP->metadataVector.push_back(&md2);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(2, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("MD1val", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector[1]->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 0)->metadataVector[1]->type);
    EXPECT_EQ("MD2val", RES_CER_ATTR(0, 0)->metadataVector[1]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> mdNames;
    std::vector<BSONElement> attrNames;

    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(2, mds.nFields());
    ASSERT_EQ(2, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("TMD1", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("MD1val", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("TMD2", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ("MD2val", getStringField(mds.getField("MD2").embeddedObject(), "value"));

    utExit();
}

/* ****************************************************************************
*
* appendMdAllExisting -
*/
TEST(mongoUpdateContextRequest, appendMdAllExisting)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "val1");
    Metadata md("MD1", "TMD1", "new_val");
    caP->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("new_val", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(2, mds.nFields());
    ASSERT_EQ(2, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("TMD1", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("new_val", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("TMD2", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ("MD2val", getStringField(mds.getField("MD2").embeddedObject(), "value"));

    utExit();
}

/* ****************************************************************************
*
* updateMdAllExisting -
*/
TEST(mongoUpdateContextRequest, updateMdAllExisting)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "val1");
    Metadata md("MD1", "TMD1", "new_val");
    caP->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("new_val", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(2, mds.nFields());
    ASSERT_EQ(2, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("TMD1", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("new_val", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("TMD2", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ("MD2val", getStringField(mds.getField("MD2").embeddedObject(), "value"));

    utExit();
}

/* ****************************************************************************
*
* appendMdAllNew -
*/
TEST(mongoUpdateContextRequest, appendMdAllNew)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "val1");
    Metadata md("MD3", "TMD3", "new_val3");
    caP->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD3", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD3", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("new_val3", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(3, mds.nFields());
    ASSERT_EQ(3, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(findAttr(mdNames, "MD3"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("TMD1", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("MD1val", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("TMD2", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ("MD2val", getStringField(mds.getField("MD2").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD3"));
    EXPECT_EQ("TMD3", getStringField(mds.getField("MD3").embeddedObject(), "type"));
    EXPECT_EQ("new_val3", getStringField(mds.getField("MD3").embeddedObject(), "value"));

    utExit();
}

/* ****************************************************************************
*
*  updateMdAllNew -
*/
TEST(mongoUpdateContextRequest, updateMdAllNew)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "val1");
    Metadata md("MD3", "TMD3", "new_val3");
    caP->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD3", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD3", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("new_val3", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(3, mds.nFields());
    ASSERT_EQ(3, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(findAttr(mdNames, "MD3"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("TMD1", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("MD1val", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("TMD2", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ("MD2val", getStringField(mds.getField("MD2").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD3"));
    EXPECT_EQ("TMD3", getStringField(mds.getField("MD3").embeddedObject(), "type"));
    EXPECT_EQ("new_val3", getStringField(mds.getField("MD3").embeddedObject(), "value"));

    utExit();
}

/* ****************************************************************************
*
* appendMdSomeNew -
*/
TEST(mongoUpdateContextRequest, appendMdSomeNew)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "val1");
    Metadata md1("MD2", "TMD2", "new_val2");
    Metadata md2("MD3", "TMD3", "new_val3");
    caP->metadataVector.push_back(&md1);
    caP->metadataVector.push_back(&md2);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(2, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("new_val2", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ("MD3", RES_CER_ATTR(0, 0)->metadataVector[1]->name);
    EXPECT_EQ("TMD3", RES_CER_ATTR(0, 0)->metadataVector[1]->type);
    EXPECT_EQ("new_val3", RES_CER_ATTR(0, 0)->metadataVector[1]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(3, mds.nFields());
    ASSERT_EQ(3, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(findAttr(mdNames, "MD3"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("TMD1", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("MD1val", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("TMD2", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ("new_val2", getStringField(mds.getField("MD2").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD3"));
    EXPECT_EQ("TMD3", getStringField(mds.getField("MD3").embeddedObject(), "type"));
    EXPECT_EQ("new_val3", getStringField(mds.getField("MD3").embeddedObject(), "value"));

    utExit();
}

/* ****************************************************************************
*
* updateMdSomeNew -
*/
TEST(mongoUpdateContextRequest, updateMdSomeNew)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "val1");
    Metadata md1("MD2", "TMD2", "new_val2");
    Metadata md2("MD3", "TMD3", "new_val3");
    caP->metadataVector.push_back(&md1);
    caP->metadataVector.push_back(&md2);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(2, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("new_val2", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ("MD3", RES_CER_ATTR(0, 0)->metadataVector[1]->name);
    EXPECT_EQ("TMD3", RES_CER_ATTR(0, 0)->metadataVector[1]->type);
    EXPECT_EQ("new_val3", RES_CER_ATTR(0, 0)->metadataVector[1]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(3, mds.nFields());
    ASSERT_EQ(3, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(findAttr(mdNames, "MD3"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("TMD1", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("MD1val", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("TMD2", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ("new_val2", getStringField(mds.getField("MD2").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD3"));
    EXPECT_EQ("TMD3", getStringField(mds.getField("MD3").embeddedObject(), "type"));
    EXPECT_EQ("new_val3", getStringField(mds.getField("MD3").embeddedObject(), "value"));

    utExit();
}

/* ****************************************************************************
*
* appendValueAndMd -
*/
TEST(mongoUpdateContextRequest, appendValueAndMd)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "attr_new_val");
    Metadata md("MD1", "TMD1", "new_val");
    caP->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("new_val", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("attr_new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(2, mds.nFields());
    ASSERT_EQ(2, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("TMD1", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("new_val", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("TMD2", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ("MD2val", getStringField(mds.getField("MD2").embeddedObject(), "value"));

    utExit();
}

/* ****************************************************************************
*
* updateValueAndMd -
*/
TEST(mongoUpdateContextRequest, updateValueAndMd)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "attr_new_val");
    Metadata md("MD1", "TMD1", "new_val");
    caP->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("new_val", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("attr_new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(2, mds.nFields());
    ASSERT_EQ(2, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("TMD1", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("new_val", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("TMD2", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ("MD2val", getStringField(mds.getField("MD2").embeddedObject(), "value"));

    utExit();
}


/* ****************************************************************************
*
* appendMdNoActualChanges -
*/
TEST(mongoUpdateContextRequest, appendMdNoActualChanges)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "val1");
    Metadata md("MD1", "TMD1", "MD1val");
    caP->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("MD1val", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(2, mds.nFields());
    ASSERT_EQ(2, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("TMD1", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("MD1val", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("TMD2", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ("MD2val", getStringField(mds.getField("MD2").embeddedObject(), "value"));

    utExit();
}

/* ****************************************************************************
*
* updateMdNoActualChanges -
*/
TEST(mongoUpdateContextRequest, updateMdNoActualChanges)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "val1");
    Metadata md("MD1", "TMD1", "MD1val");
    caP->metadataVector.push_back(&md);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("MD1val", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(2, mds.nFields());
    ASSERT_EQ(2, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("TMD1", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("MD1val", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("TMD2", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ("MD2val", getStringField(mds.getField("MD2").embeddedObject(), "value"));

    utExit();
}

/* ****************************************************************************
*
* patternUnsupported -
*/
TEST(mongoUpdateContextRequest, patternUnsupported)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ce1P = new ContextElement();
    ContextElement* ce2P = new ContextElement();
    ce1P->entityId.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "new_val1");
    ce2P->entityId.fill("E[2-3]", "T", "true");
    ContextAttribute* ca2P = new ContextAttribute("AX", "TAX", "X");

    ce1P->contextAttributeVector.push_back(ca1P);
    ce2P->contextAttributeVector.push_back(ca2P);
    req.contextElementVector.push_back(ce1P);
    req.contextElementVector.push_back(ce2P);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E[2-3]", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("true", RES_CER(1).entityId.isPattern);
    EXPECT_EQ(0, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ(SccNotImplemented, RES_CER_STATUS(1).code);
    EXPECT_EQ("Not Implemented", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* notExistFilter -
*/
TEST(mongoUpdateContextRequest, notExistFilter)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");

    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Set filter */
    uriParams[URI_PARAM_NOT_EXIST] = "entity::type";

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* createNativeTypes -
*/
TEST(mongoUpdateContextRequest, createNativeTypes)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E4", "T4", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "T", "myVal");
    ContextAttribute* ca2P = new ContextAttribute("A2", "T", 42.5);
    ContextAttribute* ca3P = new ContextAttribute("A3", "T", false);
    ContextAttribute* ca4P = new ContextAttribute("A4", "T", "");
    ca4P->valueType = orion::ValueTypeNull;
    ceP->contextAttributeVector.push_back(ca1P);
    ceP->contextAttributeVector.push_back(ca2P);
    ceP->contextAttributeVector.push_back(ca3P);
    ceP->contextAttributeVector.push_back(ca4P);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E4", RES_CER(0).entityId.id);
    EXPECT_EQ("T4", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(4, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ("A3", RES_CER_ATTR(0, 2)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 2)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 2)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 2)->metadataVector.size());
    EXPECT_EQ("A4", RES_CER_ATTR(0, 3)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 3)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 3)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 3)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E4" << "_id.type" << "T4"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T4", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(4, attrs.nFields());
    ASSERT_EQ(4, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    a3 = attrs.getField("A3").embeddedObject();
    a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_STREQ("T", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("myVal", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("T", C_STR_FIELD(a2, "type"));
    EXPECT_EQ(42.5, a2.getField("value").Number());
    EXPECT_EQ(1360232700, a2.getIntField("creDate"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));
    EXPECT_STREQ("T", C_STR_FIELD(a3, "type"));
    EXPECT_FALSE(a3.getBoolField("value"));
    EXPECT_EQ(1360232700, a3.getIntField("creDate"));
    EXPECT_EQ(1360232700, a3.getIntField("modDate"));
    EXPECT_STREQ("T", C_STR_FIELD(a4, "type"));
    EXPECT_TRUE(a4.getField("value").isNull());
    EXPECT_EQ(1360232700, a4.getIntField("creDate"));
    EXPECT_EQ(1360232700, a4.getIntField("modDate"));


    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* updateNativeTypes -
*/
TEST(mongoUpdateContextRequest, updateNativeTypes)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ce1P = new ContextElement();
    ce1P->entityId.fill("E1", "T1", "false");
    ContextAttribute* ca1P = new ContextAttribute("A1", "T", 42.5);
    ContextAttribute* ca2P = new ContextAttribute("A2", "T", false);
    ce1P->contextAttributeVector.push_back(ca1P);
    ce1P->contextAttributeVector.push_back(ca2P);
    req.contextElementVector.push_back(ce1P);

    ContextElement* ce2P = new ContextElement();
    ce2P->entityId.fill("E2", "T2", "false");
    ContextAttribute* ca3P = new ContextAttribute("A3", "T", "");
    ca3P->valueType = orion::ValueTypeNull;
    ce2P->contextAttributeVector.push_back(ca3P);
    req.contextElementVector.push_back(ce2P);

    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "", V2);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A3", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("T", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(1).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("T", C_STR_FIELD(a1, "type"));
    EXPECT_EQ(42.5, a1.getField("value").Number());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("T", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.getBoolField("value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_STREQ("T", C_STR_FIELD(a3, "type"));
    EXPECT_TRUE(a3.getField("value").isNull());
    LM_W(("a3 value == '%s'", a3.getField("value").toString().c_str()));
    EXPECT_EQ(1360232700, a3.getIntField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* preservingNativeTypes -
*
* Changing only one attribute (the string one), other keep the same
*/
TEST(mongoUpdateContextRequest, preservingNativeTypes)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseDifferentNativeTypes();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "T", "new_s");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(6, attrs.nFields());
    ASSERT_EQ(6, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_STREQ("T", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_s", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("T", C_STR_FIELD(a2, "type"));
    EXPECT_EQ(42, a2.getField("value").Number());
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("T", C_STR_FIELD(a3, "type"));
    EXPECT_FALSE(a3.getBoolField("value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("T", C_STR_FIELD(a4, "type"));
    EXPECT_EQ("a", a4.getField("value").embeddedObject().getField("x").str());
    EXPECT_EQ("b", a4.getField("value").embeddedObject().getField("y").str());
    EXPECT_FALSE(a4.hasField("modDate"));
    EXPECT_STREQ("T", C_STR_FIELD(a5, "type"));
    EXPECT_EQ("x1", a5.getField("value").Array()[0].str());
    EXPECT_EQ("x2", a5.getField("value").Array()[1].str());
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("T", C_STR_FIELD(a6, "type"));
    EXPECT_TRUE(a6.getField("value").isNull());
    EXPECT_FALSE(a6.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* createMdNativeTypes -
*/
TEST(mongoUpdateContextRequest, createMdNativeTypes)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E4", "T4", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "T", "new_val");
    Metadata md1("MD1", "T", "s");
    Metadata md2("MD2", "T", 55.5);
    Metadata md3("MD3", "T", false);
    Metadata md4("MD4", "T", "");
    md4.valueType = orion::ValueTypeNull;
    caP->metadataVector.push_back(&md1);
    caP->metadataVector.push_back(&md2);
    caP->metadataVector.push_back(&md3);
    caP->metadataVector.push_back(&md4);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E4", RES_CER(0).entityId.id);
    EXPECT_EQ("T4", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(4, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->metadataVector[0]->valueType);
    EXPECT_EQ("s", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector[1]->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->metadataVector[1]->type);
    EXPECT_EQ(orion::ValueTypeNumber, RES_CER_ATTR(0, 0)->metadataVector[1]->valueType);
    EXPECT_EQ(55.5, RES_CER_ATTR(0, 0)->metadataVector[1]->numberValue);
    EXPECT_EQ("MD3", RES_CER_ATTR(0, 0)->metadataVector[2]->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->metadataVector[2]->type);
    EXPECT_EQ(orion::ValueTypeBoolean, RES_CER_ATTR(0, 0)->metadataVector[2]->valueType);
    EXPECT_FALSE(RES_CER_ATTR(0, 0)->metadataVector[2]->boolValue);
    EXPECT_EQ("MD4", RES_CER_ATTR(0, 0)->metadataVector[3]->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->metadataVector[3]->type);
    EXPECT_EQ(orion::ValueTypeNull, RES_CER_ATTR(0, 0)->metadataVector[3]->valueType);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    BSONObj a4 = attrs.getField("A4").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_TRUE(findAttr(attrNames, "A4"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E4" << "_id.type" << "T4"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T4", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_TRUE(ent.hasField("creDate"));
    EXPECT_TRUE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("T", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_TRUE(a1.hasField("creDate"));
    EXPECT_TRUE(a1.hasField("modDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(4, mds.nFields());
    ASSERT_EQ(4, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(findAttr(mdNames, "MD3"));
    EXPECT_TRUE(findAttr(mdNames, "MD4"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("T", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("s", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("T", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ(55.5, mds.getField("MD2").embeddedObject().getField("value").Number());
    EXPECT_TRUE(mds.hasField("MD3"));
    EXPECT_EQ("T", getStringField(mds.getField("MD3").embeddedObject(), "type"));
    EXPECT_FALSE(mds.getField("MD3").embeddedObject().getBoolField("value"));
    EXPECT_TRUE(mds.hasField("MD4"));
    EXPECT_EQ("T", getStringField(mds.getField("MD4").embeddedObject(), "type"));
    EXPECT_TRUE(mds.getField("MD4").embeddedObject().getField("value").isNull());

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* updateMdNativeTypes -
*/
TEST(mongoUpdateContextRequest, updateMdNativeTypes)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseDifferentMdNativeTypes();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "T", "new_val");
    Metadata md1("MD1", "T", "ss");
    Metadata md2("MD2", "T", 44.4);
    Metadata md3("MD3", "T", true);
    caP->metadataVector.push_back(&md1);
    caP->metadataVector.push_back(&md2);
    caP->metadataVector.push_back(&md3);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(3, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ(orion::ValueTypeString, RES_CER_ATTR(0, 0)->metadataVector[0]->valueType);
    EXPECT_EQ("ss", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector[1]->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->metadataVector[1]->type);
    EXPECT_EQ(orion::ValueTypeNumber, RES_CER_ATTR(0, 0)->metadataVector[1]->valueType);
    EXPECT_EQ(44.4, RES_CER_ATTR(0, 0)->metadataVector[1]->numberValue);
    EXPECT_EQ("MD3", RES_CER_ATTR(0, 0)->metadataVector[2]->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->metadataVector[2]->type);
    EXPECT_EQ(orion::ValueTypeBoolean, RES_CER_ATTR(0, 0)->metadataVector[2]->valueType);
    EXPECT_TRUE(RES_CER_ATTR(0, 0)->metadataVector[2]->boolValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("T", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(4, mds.nFields());
    ASSERT_EQ(4, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(findAttr(mdNames, "MD3"));
    EXPECT_TRUE(findAttr(mdNames, "MD4"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("T", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("ss", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("T", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ(44.4, mds.getField("MD2").embeddedObject().getField("value").Number());
    EXPECT_TRUE(mds.hasField("MD3"));
    EXPECT_EQ("T", getStringField(mds.getField("MD3").embeddedObject(), "type"));
    EXPECT_TRUE(mds.getField("MD3").embeddedObject().getBoolField("value"));
    EXPECT_TRUE(mds.hasField("MD4"));
    EXPECT_EQ("T", getStringField(mds.getField("MD4").embeddedObject(), "type"));
    EXPECT_TRUE(mds.getField("MD4").embeddedObject().getField("value").isNull());

    utExit();
}

/* ****************************************************************************
*
* preservingMdNativeTypes -
*
* Changing only one attribute (the string one), other weeks the same
*/
TEST(mongoUpdateContextRequest, preservingMdNativeTypes)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseDifferentMdNativeTypes();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "T", "new_s");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("T", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_s", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    mds = a1.getField("md").embeddedObject();
    mdNames = a1.getField("mdNames").Array();
    ASSERT_EQ(4, mds.nFields());
    ASSERT_EQ(4, mdNames.size());
    EXPECT_TRUE(findAttr(mdNames, "MD1"));
    EXPECT_TRUE(findAttr(mdNames, "MD2"));
    EXPECT_TRUE(findAttr(mdNames, "MD3"));
    EXPECT_TRUE(findAttr(mdNames, "MD4"));
    EXPECT_TRUE(mds.hasField("MD1"));
    EXPECT_EQ("T", getStringField(mds.getField("MD1").embeddedObject(), "type"));
    EXPECT_EQ("s", getStringField(mds.getField("MD1").embeddedObject(), "value"));
    EXPECT_TRUE(mds.hasField("MD2"));
    EXPECT_EQ("T", getStringField(mds.getField("MD2").embeddedObject(), "type"));
    EXPECT_EQ(55.5, mds.getField("MD2").embeddedObject().getField("value").Number());
    EXPECT_TRUE(mds.hasField("MD3"));
    EXPECT_EQ("T", getStringField(mds.getField("MD3").embeddedObject(), "type"));
    EXPECT_FALSE(mds.getField("MD3").embeddedObject().getBoolField("value"));
    EXPECT_TRUE(mds.hasField("MD4"));
    EXPECT_EQ("T", getStringField(mds.getField("MD4").embeddedObject(), "type"));
    EXPECT_TRUE(mds.getField("MD4").embeddedObject().getField("value").isNull());

    utExit();
}

/* ****************************************************************************
*
* replace -
*/
TEST(mongoUpdateContextRequest, replace)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ce1P = new ContextElement();
    ContextElement* ce2P = new ContextElement();
    ce1P->entityId.fill("E1", "T1", "false");
    ce2P->entityId.fill("E2", "T2", "false");
    ContextAttribute* ca1P = new ContextAttribute("B1", "T", "1");
    ContextAttribute* ca2P = new ContextAttribute("B2", "T", "2");
    ContextAttribute* ca3P = new ContextAttribute("B3", "T", "3");
    ce1P->contextAttributeVector.push_back(ca1P);
    ce1P->contextAttributeVector.push_back(ca2P);
    ce2P->contextAttributeVector.push_back(ca3P);
    req.contextElementVector.push_back(ce1P);
    req.contextElementVector.push_back(ce2P);
    req.updateActionType = ActionTypeReplace;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("B1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("B2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("T", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("B3", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("T", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(1).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs, mds;
    std::vector<BSONElement> attrNames;
    std::vector<BSONElement> mdNames;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj b1 = attrs.getField("B1").embeddedObject();
    BSONObj b2 = attrs.getField("B2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "B1"));
    EXPECT_TRUE(findAttr(attrNames, "B2"));
    EXPECT_STREQ("T", C_STR_FIELD(b1, "type"));
    EXPECT_STREQ("1", C_STR_FIELD(b1, "value"));
    EXPECT_EQ(1360232700, b1.getIntField("modDate"));
    EXPECT_STREQ("T", C_STR_FIELD(b2, "type"));
    EXPECT_STREQ("2", C_STR_FIELD(b2, "value"));
    EXPECT_EQ(1360232700, b2.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj b3 = attrs.getField("B3").embeddedObject();
    EXPECT_STREQ("T", C_STR_FIELD(b3, "type"));
    EXPECT_STREQ("3", C_STR_FIELD(b3, "value"));
    EXPECT_EQ(1360232700, b3.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    BSONObj a6 = attrs.getField("A6").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_TRUE(findAttr(attrNames, "A6"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* tooManyEntitiesNGSIv2 -
*/
TEST(mongoUpdateContextRequest, tooManyEntitiesNGSIv2)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  ContextElement* ceP = new ContextElement();
  ceP->entityId.fill("E1", "", "false");
  ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val1");
  ceP->contextAttributeVector.push_back(caP);
  req.contextElementVector.push_back(ceP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library (note the 2 to activate NGSIv2 special behaviours) */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "", V2);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(0, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ(SccConflict, RES_CER_STATUS(0).code);
  EXPECT_EQ("Too Many Results", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(ERROR_DESC_TOO_MANY_ENTITIES, RES_CER_STATUS(0).details);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* entities collection */
  BSONObj ent, attrs;
  std::vector<BSONElement> attrNames;
  ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(2, attrs.nFields());
  ASSERT_EQ(2, attrNames.size());
  BSONObj a1 = attrs.getField("A1").embeddedObject();
  BSONObj a2 = attrs.getField("A2").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_TRUE(findAttr(attrNames, "A2"));
  EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));
  EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
  EXPECT_FALSE(a2.hasField("value"));
  EXPECT_FALSE(a2.hasField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
  EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(2, attrs.nFields());
  ASSERT_EQ(2, attrNames.size());
  BSONObj a3 = attrs.getField("A3").embeddedObject();
  BSONObj a4 = attrs.getField("A4").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A3"));
  EXPECT_TRUE(findAttr(attrNames, "A4"));
  EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
  EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
  EXPECT_FALSE(a3.hasField("modDate"));
  EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
  EXPECT_FALSE(a4.hasField("value"));
  EXPECT_FALSE(a4.hasField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
  EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(2, attrs.nFields());
  ASSERT_EQ(2, attrNames.size());
  BSONObj a5 = attrs.getField("A5").embeddedObject();
  BSONObj a6 = attrs.getField("A6").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A5"));
  EXPECT_TRUE(findAttr(attrNames, "A6"));
  EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
  EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
  EXPECT_FALSE(a5.hasField("modDate"));
  EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
  EXPECT_FALSE(a6.hasField("value"));
  EXPECT_FALSE(a6.hasField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(1, attrs.nFields());
  ASSERT_EQ(1, attrNames.size());
  a1 = attrs.getField("A1").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));

  /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(2, attrs.nFields());
  ASSERT_EQ(2, attrNames.size());
  a1 = attrs.getField("A1").embeddedObject();
  a2 = attrs.getField("A2").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_TRUE(findAttr(attrNames, "A2"));
  EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));
  EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
  EXPECT_FALSE(a2.hasField("value"));
  EXPECT_FALSE(a2.hasField("modDate"));

  utExit();
}

/* ****************************************************************************
*
* onlyOneEntityNGSIv2 -
*
* This is a "dual" test (testing the happy path) for the tooManyEntitiesNGSIv2 test
*/
TEST(mongoUpdateContextRequest, onlyOneEntityNGSIv2)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  ContextElement* ceP = new ContextElement();
  ceP->entityId.fill("E1", "T1", "false");
  ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val1");
  ceP->contextAttributeVector.push_back(caP);
  req.contextElementVector.push_back(ceP);
  req.updateActionType = ActionTypeUpdate;

  /* Invoke the function in mongoBackend library (note the 2 to activate NGSIv2 special behaviours) */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "", V2);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* entities collection */
  BSONObj ent, attrs;
  std::vector<BSONElement> attrNames;
  ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(1360232700, ent.getIntField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(2, attrs.nFields());
  ASSERT_EQ(2, attrNames.size());
  BSONObj a1 = attrs.getField("A1").embeddedObject();
  BSONObj a2 = attrs.getField("A2").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_TRUE(findAttr(attrNames, "A2"));
  EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("new_val1", C_STR_FIELD(a1, "value"));
  EXPECT_EQ(1360232700, a1.getIntField("modDate"));
  EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
  EXPECT_FALSE(a2.hasField("value"));
  EXPECT_FALSE(a2.hasField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
  EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(2, attrs.nFields());
  ASSERT_EQ(2, attrNames.size());
  BSONObj a3 = attrs.getField("A3").embeddedObject();
  BSONObj a4 = attrs.getField("A4").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A3"));
  EXPECT_TRUE(findAttr(attrNames, "A4"));
  EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
  EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
  EXPECT_FALSE(a3.hasField("modDate"));
  EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
  EXPECT_FALSE(a4.hasField("value"));
  EXPECT_FALSE(a4.hasField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
  EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(2, attrs.nFields());
  ASSERT_EQ(2, attrNames.size());
  BSONObj a5 = attrs.getField("A5").embeddedObject();
  BSONObj a6 = attrs.getField("A6").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A5"));
  EXPECT_TRUE(findAttr(attrNames, "A6"));
  EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
  EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
  EXPECT_FALSE(a5.hasField("modDate"));
  EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
  EXPECT_FALSE(a6.hasField("value"));
  EXPECT_FALSE(a6.hasField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(1, attrs.nFields());
  ASSERT_EQ(1, attrNames.size());
  a1 = attrs.getField("A1").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));

  /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(2, attrs.nFields());
  ASSERT_EQ(2, attrNames.size());
  a1 = attrs.getField("A1").embeddedObject();
  a2 = attrs.getField("A2").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_TRUE(findAttr(attrNames, "A2"));
  EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));
  EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
  EXPECT_FALSE(a2.hasField("value"));
  EXPECT_FALSE(a2.hasField("modDate"));

  utExit();
}

/* ****************************************************************************
*
* firstTimeTrue -
*
* This function is used in some mocks that need to emulate more() function in the
* following way: first call to the function is true, second and further calls are false
*/
bool firstTimeTrue(void)
{
  static bool firstTime = true;

  if (firstTime)
  {
    firstTime = false;
    return true;
  }
  else
  {
    return false;
  }
}



/* ****************************************************************************
*
* mongoDbUpdateFail -
*/
TEST(mongoUpdateContextRequest, mongoDbUpdateFail)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit(false);

    /* Set database */
    setupDatabase();

    /* Prepare mock */
    /* FIXME: cursorMockCsub is probably unnecessary if we solve the problem of Invoke() the real
     * functionality in the DBClientConnectionMockUpdateContext declaration */
    const DBException e = DBException("boom!!", 33);
    BSONObj fakeEn = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                          "attrs" << BSON(
                            "A1" << BSON("type" << "TA1" << "value" << "val1") <<
                            "A2" << BSON("type" << "TA2")));

    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    DBClientCursorMock* cursorMockEnt = new DBClientCursorMock(connectionMock, "", 0, 0, 0);
    DBClientCursorMock* cursorMockCsub = new DBClientCursorMock(connectionMock, "", 0, 0, 0);

    ON_CALL(*cursorMockEnt, more())
            .WillByDefault(Invoke(firstTimeTrue));
    ON_CALL(*cursorMockEnt, next())
            .WillByDefault(Return(fakeEn));
    ON_CALL(*cursorMockCsub, more())
            .WillByDefault(Return(false));
    ON_CALL(*connectionMock, _query("utest.entities", _, _, _, _, _, _))
            .WillByDefault(Return(cursorMockEnt));
    ON_CALL(*connectionMock, _query("utest.csubs", _, _, _, _, _, _))
            .WillByDefault(Return(cursorMockCsub));
    ON_CALL(*connectionMock, update("utest.entities", _, _, _, _, _))
            .WillByDefault(Throw(e));

    /* Set MongoDB connection */
    DBClientBase* connectionDb = getMongoConnection();
    setMongoConnectionForUnitTest(connectionMock);

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
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
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccReceiverInternalError, RES_CER_STATUS(0).code);
    EXPECT_EQ("Internal Server Error", RES_CER_STATUS(0).reasonPhrase);

    EXPECT_EQ("Database Error (collection: utest.entities "
              "- update(): <{ _id.id: \"E1\", _id.type: \"T1\", "
              "_id.servicePath: { $in: [ /^/.*/, null ] } },"
              "{ $set: { attrs.A1: { value: \"new_val\", type: \"TA1\", mdNames: [], "
              "modDate: 1360232700 }, modDate: 1360232700, lastCorrelator: \"\" }, $unset: { location: 1 } }> "
              "- exception: boom!!)", RES_CER_STATUS(0).details);

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb);

    /* Release mocks */
    delete connectionMock;
    delete cursorMockCsub;

    utExit();
}

/* ****************************************************************************
*
* mongoDbQueryFail -
*/
TEST(mongoUpdateContextRequest, mongoDbQueryFail)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare mock */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, _query(_, _, _, _, _, _, _))
            .WillByDefault(Throw(e));

    /* Set MongoDB connection */
    DBClientBase* connectionDb = getMongoConnection();
    setMongoConnectionForUnitTest(connectionMock);

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "new_val");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    EXPECT_EQ(0, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ(SccReceiverInternalError, RES_CER_STATUS(0).code);
    EXPECT_EQ("Internal Server Error", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("Database Error (collection: utest.entities "
              "- query(): { _id.id: \"E1\", _id.type: \"T1\", _id.servicePath: { $in: [ /^/.*/, null ] } } "
              "- exception: boom!!)", RES_CER_STATUS(0).details);

    /* Restore real DB connection */
    setMongoConnectionForUnitTest(connectionDb);

    /* Release mock */
    delete connectionMock;

    utExit();
}


/* ****************************************************************************
*
* servicePathEntityUpdate_3levels -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityUpdate_3levels)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePaths();

  /* Forge the request (from "inside" to "outside") */
  ContextElement* ceP = new ContextElement();
  ceP->entityId.fill("E1", "T1", "false");
  ContextAttribute* caP = new ContextAttribute("A1", "TA1", "kz01-modified");
  ceP->contextAttributeVector.push_back(caP);
  req.contextElementVector.push_back(ceP);
  req.updateActionType = ActionTypeUpdate;
  servicePathVector.clear();
  servicePathVector.push_back("/home/kz/01");

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* entities collection */
  BSONObj ent, attrs;
  std::vector<BSONElement> attrNames;
  ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id"          << "E1" <<
                                                "_id.type"        << "T1" <<
                                                "_id.servicePath" << "/home/kz/01"));

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(1360232700, ent.getIntField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(1, attrs.nFields());
  ASSERT_EQ(1, attrNames.size());
  BSONObj a1 = attrs.getField("A1").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("kz01-modified", C_STR_FIELD(a1, "value"));
  EXPECT_EQ(1360232700, a1.getIntField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id"          << "E1" <<
                                                "_id.type"        << "T1" <<
                                                "_id.servicePath" << "/home/kz/02"));

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(1, attrs.nFields());
  ASSERT_EQ(1, attrNames.size());
  a1 = attrs.getField("A1").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("kz02", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));

  utExit();
}

/* ****************************************************************************
*
* servicePathEntityAppend_3levels -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityAppend_3levels)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePaths();

  /* Forge the request (from "inside" to "outside") */
  ContextElement* ceP = new ContextElement();
  ceP->entityId.fill("E1", "T1", "false");
  ContextAttribute* caP = new ContextAttribute("A2", "TA2", "new");
  ceP->contextAttributeVector.push_back(caP);
  req.contextElementVector.push_back(ceP);
  req.updateActionType = ActionTypeAppend;
  servicePathVector.clear();
  servicePathVector.push_back("/home/kz/01");

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(SccOk, res.errorCode.code);
  EXPECT_EQ("OK", res.errorCode.reasonPhrase);
  EXPECT_EQ(0, res.errorCode.details.size());

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ("", RES_CER_STATUS(0).details);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* entities collection */
  BSONObj ent, attrs;
  std::vector<BSONElement> attrNames;
  ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id"          << "E1" <<
                                                "_id.type"        << "T1" <<
                                                "_id.servicePath" << "/home/kz/01"));

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(1360232700, ent.getIntField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(2, attrs.nFields());
  ASSERT_EQ(2, attrNames.size());
  BSONObj a1 = attrs.getField("A1").embeddedObject();
  BSONObj a2 = attrs.getField("A2").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_TRUE(findAttr(attrNames, "A2"));
  EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("kz01", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));
  EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
  EXPECT_STREQ("new", C_STR_FIELD(a2, "value"));
  EXPECT_EQ(1360232700, a2.getIntField("modDate"));
  EXPECT_EQ(1360232700, a2.getIntField("creDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id"          << "E1" <<
                                                "_id.type"        << "T1" <<
                                                "_id.servicePath" << "/home/kz/02"));

  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(1, attrs.nFields());
  ASSERT_EQ(1, attrNames.size());
  a1 = attrs.getField("A1").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("kz02", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));

  utExit();
}

/* ****************************************************************************
*
* servicePathEntityCreation_2levels -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityCreation_2levels)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithServicePaths();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "fg");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;
    servicePathVector.clear();
    servicePathVector.push_back("/home/fg");

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(3, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id"          << "E1" <<
                                                  "_id.type"        << "T1" <<
                                                  "_id.servicePath" << "/home/kz/01"));

    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("kz01", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id"          << "E1" <<
                                                  "_id.type"        << "T1" <<
                                                  "_id.servicePath" << "/home/kz/02"));

    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("kz02", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id"          << "E1" <<
                                                  "_id.type"        << "T1" <<
                                                  "_id.servicePath" << "/home/fg"));

    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("fg", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));

    utExit();
}

/* ****************************************************************************
*
* servicePathEntityCreation_3levels -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityCreation_3levels)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithServicePaths();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "fg");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;
    servicePathVector.clear();
    servicePathVector.push_back("/home/fg/01");

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(3, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id"          << "E1" <<
                                                  "_id.type"        << "T1" <<
                                                  "_id.servicePath" << "/home/kz/01"));

    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("kz01", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id"          << "E1" <<
                                                  "_id.type"        << "T1" <<
                                                  "_id.servicePath" << "/home/kz/02"));

    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("kz02", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id"          << "E1" <<
                                                  "_id.type"        << "T1" <<
                                                  "_id.servicePath" << "/home/fg/01"));

    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("fg", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));

    utExit();
}

/* ****************************************************************************
*
* servicePathEntityDeletion_3levels -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityDeletion_3levels)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithServicePaths();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeDelete;
    servicePathVector.clear();
    servicePathVector.push_back("/home/kz/01");

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(0, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id"          << "E1" <<
                                                  "_id.type"        << "T1" <<
                                                  "_id.servicePath" << "/home/kz/02"));

    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("kz02", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* servicePathEntityVectorNotAllowed -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityVectorNotAllowed)
{
  HttpStatusCode         ms;
  UpdateContextRequest   ucReq;
  UpdateContextResponse  ucRes;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  ContextElement* ceP = new ContextElement();
  ceP->entityId.fill("E1", "T1", "false");
  ContextAttribute* caP = new ContextAttribute("A1", "TA1", "kz01");
  ceP->contextAttributeVector.push_back(caP);
  ucReq.contextElementVector.push_back(ceP);
  ucReq.updateActionType = ActionTypeAppend;
  servicePathVector.clear();
  servicePathVector.push_back("/home/kz");
  servicePathVector.push_back("/home/fg");

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&ucReq, &ucRes, "", servicePathVector, uriParams, "", "", "");


  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  ASSERT_EQ(0, ucRes.contextElementResponseVector.size());
  EXPECT_EQ(SccBadRequest, ucRes.errorCode.code);
  EXPECT_EQ("Bad Request", ucRes.errorCode.reasonPhrase);
  EXPECT_EQ("service path length greater than the one in update", ucRes.errorCode.details);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* entities collection */
  BSONObj ent, attrs;
  std::vector<BSONElement> attrNames;
  ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(2, attrs.nFields());
  ASSERT_EQ(2, attrNames.size());
  BSONObj a1 = attrs.getField("A1").embeddedObject();
  BSONObj a2 = attrs.getField("A2").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_TRUE(findAttr(attrNames, "A2"));
  EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));
  EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
  EXPECT_FALSE(a2.hasField("value"));
  EXPECT_FALSE(a2.hasField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
  EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(2, attrs.nFields());
  ASSERT_EQ(2, attrNames.size());
  BSONObj a3 = attrs.getField("A3").embeddedObject();
  BSONObj a4 = attrs.getField("A4").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A3"));
  EXPECT_TRUE(findAttr(attrNames, "A4"));
  EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
  EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
  EXPECT_FALSE(a3.hasField("modDate"));
  EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
  EXPECT_FALSE(a4.hasField("value"));
  EXPECT_FALSE(a4.hasField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
  EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(2, attrs.nFields());
  ASSERT_EQ(2, attrNames.size());
  BSONObj a5 = attrs.getField("A5").embeddedObject();
  BSONObj a6 = attrs.getField("A6").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A5"));
  EXPECT_TRUE(findAttr(attrNames, "A6"));
  EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
  EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
  EXPECT_FALSE(a5.hasField("modDate"));
  EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
  EXPECT_FALSE(a6.hasField("value"));
  EXPECT_FALSE(a6.hasField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(1, attrs.nFields());
  ASSERT_EQ(1, attrNames.size());
  a1 = attrs.getField("A1").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));

  /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").embeddedObject();
  attrNames = ent.getField("attrNames").Array();
  ASSERT_EQ(2, attrs.nFields());
  ASSERT_EQ(2, attrNames.size());
  a1 = attrs.getField("A1").embeddedObject();
  a2 = attrs.getField("A2").embeddedObject();
  EXPECT_TRUE(findAttr(attrNames, "A1"));
  EXPECT_TRUE(findAttr(attrNames, "A2"));
  EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));
  EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
  EXPECT_FALSE(a2.hasField("value"));
  EXPECT_FALSE(a2.hasField("modDate"));

  utExit();
}

