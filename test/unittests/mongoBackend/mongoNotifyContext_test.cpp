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
* Author: Fermín Galán
*/

#include "gtest/gtest.h"
#include "testInit.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "apiTypesV2/HttpInfo.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoNotifyContext.h"
#include "ngsi10/NotifyContextRequest.h"

#include "mongo/client/dbclient.h"

#include "commonMocks.h"
#include "unittest.h"

using ::testing::_;
using ::testing::Throw;
using ::testing::Return;

extern void setMongoConnectionForUnitTest(DBClientBase*);



/* ****************************************************************************
*
* Tests
*
* - Ent1Attr1
* - Ent1AttrN
* - EntNAttr1
* - EntNAttrN
* - createEntity
*
*- FIXME P6: we can not provide a complete set of unit test right now, due to the rush
*  for Campus Party. This fixme mesage is a mark to get these tests completed in the
*  future. Look to updateContext test to get ideas of what is missing here
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
                        "A2" << BSON("type" << "TA2")
                        )
                    );

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T2") <<
                     "attrNames" << BSON_ARRAY("A3" << "A4") <<
                     "attrs" << BSON(
                        "A3" << BSON("type" << "TA3" << "value" << "val3") <<
                        "A4" << BSON("type" << "TA4")
                        )
                    );

  BSONObj en3 = BSON("_id" << BSON("id" << "E3" << "type" << "T3") <<
                     "attrNames" << BSON_ARRAY("A5" << "A6") <<
                     "attrs" << BSON(
                        "A5" << BSON("type" << "TA5" << "value" << "val5") <<
                        "A6" << BSON("type" << "TA6")
                        )
                    );

  BSONObj en4 = BSON("_id" << BSON("id" << "E1" << "type" << "T1bis") <<
                     "attrNames" << BSON_ARRAY("A1") <<
                     "attrs" << BSON(
                        "A1" << BSON("type" << "TA1" << "value" << "val1bis2")
                        )
                    );

  BSONObj en1nt = BSON("_id" << BSON("id" << "E1") <<
                     "attrNames" << BSON_ARRAY("A1" << "A2") <<
                     "attrs" << BSON(
                        "A1" << BSON("type" << "TA1" << "value" << "val1-nt") <<
                        "A2" << BSON("type" << "TA2")
                        )
                    );

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en3);
  connection->insert(ENTITIES_COLL, en4);
  connection->insert(ENTITIES_COLL, en1nt);

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
* Ent1Attr1 -
*/
TEST(mongoNotifyContextRequest, Ent1Attr1)
{
    utInit();

    HttpStatusCode         ms;
    NotifyContextRequest   req;
    NotifyContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request */
    ContextElementResponse cer;
    req.subscriptionId.set("51307b66f481db11bf860001");
    req.originator.set("localhost");
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "new_val");
    cer.contextElement.contextAttributeVector.push_back(&ca);
    cer.statusCode.fill(SccOk, "");
    req.contextElementResponseVector.push_back(&cer);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContext(&req, &res, "", "", servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

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
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
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
    ASSERT_EQ(2, attrs.nFields());
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
    ASSERT_EQ(1, attrs.nFields());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
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
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* Ent1AttrN -
*/
TEST(mongoNotifyContextRequest, Ent1AttrN)
{
    HttpStatusCode         ms;
    NotifyContextRequest   req;
    NotifyContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request */
    ContextElementResponse cer;
    req.subscriptionId.set("51307b66f481db11bf860001");
    req.originator.set("localhost");
    cer.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "new_val");
    ContextAttribute ca2("A2", "TA2", "new_val2");
    cer.contextElement.contextAttributeVector.push_back(&ca1);
    cer.contextElement.contextAttributeVector.push_back(&ca2);
    cer.statusCode.fill(SccOk);
    req.contextElementResponseVector.push_back(&cer);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContext(&req, &res, "", "", servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

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
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("new_val2", C_STR_FIELD(a2, "value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));

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
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
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
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* EntNAttr1 -
*/
TEST(mongoNotifyContextRequest, EntNAttr1)
{
    HttpStatusCode         ms;
    NotifyContextRequest   req;
    NotifyContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request */
    ContextElementResponse cer1, cer2;
    req.subscriptionId.set("51307b66f481db11bf860001");
    req.originator.set("localhost");
    cer1.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "new_val");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.statusCode.fill(SccOk);
    req.contextElementResponseVector.push_back(&cer1);
    cer2.contextElement.entityId.fill("E2", "T2", "false");
    ContextAttribute ca2("A3", "TA3", "new_val2");
    cer2.contextElement.contextAttributeVector.push_back(&ca2);
    cer2.statusCode.fill(SccOk);
    req.contextElementResponseVector.push_back(&cer2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContext(&req, &res, "", "", servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

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
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

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
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("new_val2", C_STR_FIELD(a3, "value"));
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
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
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
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* EntNAttrN -
*/
TEST(mongoNotifyContextRequest, EntNAttrN)
{
    HttpStatusCode         ms;
    NotifyContextRequest   req;
    NotifyContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request */
    ContextElementResponse cer1, cer2;
    req.subscriptionId.set("51307b66f481db11bf860001");
    req.originator.set("localhost");
    cer1.contextElement.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "new_val");
    ContextAttribute ca2("A2", "TA2", "new_val2");
    cer1.contextElement.contextAttributeVector.push_back(&ca1);
    cer1.contextElement.contextAttributeVector.push_back(&ca2);
    cer1.statusCode.fill(SccOk);
    req.contextElementResponseVector.push_back(&cer1);
    cer2.contextElement.entityId.fill("E2", "T2", "false");
    ContextAttribute ca3("A3", "TA3", "new_val3");
    ContextAttribute ca4("A4", "TA4", "new_val4");
    cer2.contextElement.contextAttributeVector.push_back(&ca3);
    cer2.contextElement.contextAttributeVector.push_back(&ca4);
    cer2.statusCode.fill(SccOk);
    req.contextElementResponseVector.push_back(&cer2);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContext(&req, &res, "", "", servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

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
    ASSERT_EQ(2, attrs.nFields());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("new_val2", C_STR_FIELD(a2, "value"));
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
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("new_val3", C_STR_FIELD(a3, "value"));
    EXPECT_EQ(1360232700, a3.getIntField("modDate"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_STREQ("new_val4", C_STR_FIELD(a4, "value"));
    EXPECT_EQ(1360232700, a4.getIntField("modDate"));

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
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
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
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* createEntity -
*/
TEST(mongoNotifyContextRequest, createEntity)
{
    HttpStatusCode         ms;
    NotifyContextRequest   req;
    NotifyContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request */
    ContextElementResponse cer;
    req.subscriptionId.set("51307b66f481db11bf860001");
    req.originator.set("localhost");
    cer.contextElement.entityId.fill("E10", "T10", "false");
    ContextAttribute ca("A1", "TA1", "new_val");
    cer.contextElement.contextAttributeVector.push_back(&ca);
    cer.statusCode.fill(SccOk);
    req.contextElementResponseVector.push_back(&cer);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContext(&req, &res, "", "", servicePathVector);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.responseCode.code);
    EXPECT_EQ("OK", res.responseCode.reasonPhrase);
    EXPECT_EQ(0, res.responseCode.details.size());

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
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
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
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_TRUE(ent.hasField("creDate"));
    EXPECT_TRUE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_TRUE(a1.hasField("creDate"));
    EXPECT_TRUE(a1.hasField("modDate"));

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
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    utExit();
}



//
// FIXME: All following tests (templateNotification*) must be removed before
//        merging into develop.
//
//

/* ****************************************************************************
*
* templateNotificationVerb -
*/
TEST(mongoNotifyContextRequest, templateNotificationVerb)
{
  utInit();

  ngsiv2::HttpInfo          httpInfo("http://localhost:1028/notify");
  Notifier                  notifier;
  std::vector<std::string>  attrsOrder;
  NotifyContextRequest      ncr;
  ContextElementResponse    cer1;
  ContextElementResponse    cer2;
  ContextElementResponse    cer3;
  
  ncr.contextElementResponseVector.push_back(&cer1);
  ncr.contextElementResponseVector.push_back(&cer2);
  ncr.contextElementResponseVector.push_back(&cer3);

  httpInfo.extended      = true; 
  httpInfo.verb          = PUT;
  httpInfo.payload       = "{ \"Template\": \"payload\" }";
  httpInfo.qs["qs1"]     = "q1";
  httpInfo.qs["qs2"]     = "q2";
  httpInfo.headers["h1"] = "h1";
  httpInfo.headers["h2"] = "h2";
  httpInfo.headers["h3"] = "h3";

  notifier.sendNotifyContextRequest(&ncr, httpInfo, "", "", "", NGSI_V2_NORMALIZED, attrsOrder);

  utExit();
}


/* ****************************************************************************
*
* templateNotificationStdPayload -
*/
TEST(mongoNotifyContextRequest, templateNotificationStdPayload)
{
  utInit();

  ngsiv2::HttpInfo          httpInfo("http://localhost:1028/notify");
  Notifier                  notifier;
  std::vector<std::string>  attrsOrder;
  NotifyContextRequest      ncr;
  ContextElementResponse    cer0;
  ContextElementResponse    cer1;
  ContextElementResponse    cer2;

  prepareDatabase();

  ncr.subscriptionId.set("51307b66f481db11bf860001");
  ncr.originator.set("localhost");

  cer0.contextElement.entityId.fill("E.*", "", "true");
  cer1.contextElement.entityId.fill("E1",  "", "false");
  cer2.contextElement.entityId.fill("E2",  "", "false");

  ncr.contextElementResponseVector.push_back(&cer0);
  ncr.contextElementResponseVector.push_back(&cer1);
  ncr.contextElementResponseVector.push_back(&cer2);

  httpInfo.extended      = true; 
  httpInfo.verb          = NOVERB;
  httpInfo.payload       = "";

  notifier.sendNotifyContextRequest(&ncr, httpInfo, "", "", "", NGSI_V2_NORMALIZED, attrsOrder);

  utExit();
}



/* ****************************************************************************
*
* templateNotificationOverrideStdHeader -
*/
TEST(mongoNotifyContextRequest, templateNotificationOverrideStdHeader)
{
  utInit();

  ngsiv2::HttpInfo          httpInfo("http://localhost:1028/notify");
  Notifier                  notifier;
  std::vector<std::string>  attrsOrder;
  NotifyContextRequest      ncr;
  ContextElementResponse    cer1;
  
  ncr.contextElementResponseVector.push_back(&cer1);

  httpInfo.extended      = true; 
  httpInfo.payload       = "{ \"Template\": \"payload\" }";
  httpInfo.qs["qs1"]     = "q1";

  httpInfo.headers["Content-Length"] = "1234";  // Override the 'real' Content-Length and must appear only once
  httpInfo.headers["dup"]            = "DUP1";
  httpInfo.headers["dup"]            = "DUP2";  // This header should appear TWICE - the user asked for it ...

  notifier.sendNotifyContextRequest(&ncr, httpInfo, "", "", "", NGSI_V2_NORMALIZED, attrsOrder);

  utExit();
}
