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
* fermin at tid dot es
*
* Author: Fermín Galán
*/

#include "gtest/gtest.h"
#include "testInit.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoNotifyContext.h"

#include "mongo/client/dbclient.h"

#include "commonMocks.h"

using ::testing::_;
using ::testing::Throw;
using ::testing::Return;

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
   *     A1*: val1bis
   *     A1**: val1bis1
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
   *     A1*: val1bis
   *     A1**: val1bis1
   *
   * (*) Means that entity/type is using same name but different type. This is included to check that type is
   *     taken into account.
   *
   * (**)same name but without type
   *
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A1" << "type" << "TA1" << "value" << "val1") <<
                        BSON("name" << "A2" << "type" << "TA2") <<
                        BSON("name" << "A1" << "type" << "TA1bis" << "value" << "val1bis") <<
                        BSON("name" << "A1" << "value" << "val1bis1")
                        )
                    );

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T2") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A3" << "type" << "TA3" << "value" << "val3") <<
                        BSON("name" << "A4" << "type" << "TA4")
                        )
                    );

  BSONObj en3 = BSON("_id" << BSON("id" << "E3" << "type" << "T3") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A5" << "type" << "TA5" << "value" << "val5") <<
                        BSON("name" << "A6" << "type" << "TA6")
                        )
                    );

  BSONObj en4 = BSON("_id" << BSON("id" << "E1" << "type" << "T1bis") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A1" << "type" << "TA1" << "value" << "val1bis2")
                        )
                    );

  BSONObj en1nt = BSON("_id" << BSON("id" << "E1") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A1" << "type" << "TA1" << "value" << "val1-nt") <<
                        BSON("name" << "A2" << "type" << "TA2") <<
                        BSON("name" << "A1" << "type" << "TA1bis" << "value" << "val1bis-nt") <<
                        BSON("name" << "A1" << "value" << "val1bis1-nt")
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
* getAttr -
*
* We need this function because we can not trust on array index, at mongo will
* not sort the elements within the array. This function assumes that always will
* find a result, that is ok for testing code.
*
*/
static BSONObj getAttr(std::vector<BSONElement> attrs, std::string name, std::string type) {

    BSONElement be;
    for (unsigned int ix = 0; ix < attrs.size(); ++ix) {
        BSONObj attr = attrs[ix].embeddedObject();
        if (STR_FIELD(attr, "name") == name && STR_FIELD(attr, "type") == type) {
            be = attrs[ix];
            break;
        }
    }
    return be.embeddedObject();

}

/* ****************************************************************************
*
* Ent1Attr1 -
*/
TEST(mongoNotifyContextRequest, Ent1Attr1)
{
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

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContext(&req, &res);

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
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    BSONObj a1bis = getAttr(attrs, "A1", "TA1bis");
    BSONObj a1nt = getAttr(attrs, "A1", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1bis, "name"));
    EXPECT_STREQ("TA1bis",C_STR_FIELD(a1bis, "type"));
    EXPECT_STREQ("val1bis", C_STR_FIELD(a1bis, "value"));
    EXPECT_FALSE(a1bis.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1nt, "name"));
    EXPECT_STREQ("",C_STR_FIELD(a1nt, "type"));
    EXPECT_STREQ("val1bis1", C_STR_FIELD(a1nt, "value"));
    EXPECT_FALSE(a1nt.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    a1bis = getAttr(attrs, "A1", "TA1bis");    
    a1nt = getAttr(attrs, "A1", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1bis, "name"));
    EXPECT_STREQ("TA1bis",C_STR_FIELD(a1bis, "type"));
    EXPECT_STREQ("val1bis-nt", C_STR_FIELD(a1bis, "value"));
    EXPECT_FALSE(a1bis.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1nt, "name"));
    EXPECT_STREQ("",C_STR_FIELD(a1nt, "type"));
    EXPECT_STREQ("val1bis1-nt", C_STR_FIELD(a1nt, "value"));
    EXPECT_FALSE(a1nt.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    /* Release mock */
    delete timerMock;

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

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContext(&req, &res);

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
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    BSONObj a1bis = getAttr(attrs, "A1", "TA1bis");
    BSONObj a1nt = getAttr(attrs, "A1", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("new_val2", C_STR_FIELD(a2, "value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1bis, "name"));
    EXPECT_STREQ("TA1bis",C_STR_FIELD(a1bis, "type"));
    EXPECT_STREQ("val1bis", C_STR_FIELD(a1bis, "value"));
    EXPECT_FALSE(a1bis.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1nt, "name"));
    EXPECT_STREQ("",C_STR_FIELD(a1nt, "type"));
    EXPECT_STREQ("val1bis1", C_STR_FIELD(a1nt, "value"));
    EXPECT_FALSE(a1nt.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    a1bis = getAttr(attrs, "A1", "TA1bis");
    a1nt = getAttr(attrs, "A1", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1bis, "name"));
    EXPECT_STREQ("TA1bis",C_STR_FIELD(a1bis, "type"));
    EXPECT_STREQ("val1bis-nt", C_STR_FIELD(a1bis, "value"));
    EXPECT_FALSE(a1bis.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1nt, "name"));
    EXPECT_STREQ("",C_STR_FIELD(a1nt, "type"));
    EXPECT_STREQ("val1bis1-nt", C_STR_FIELD(a1nt, "value"));
    EXPECT_FALSE(a1nt.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    /* Release mock */
    delete timerMock;

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

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContext(&req, &res);

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
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    BSONObj a1bis = getAttr(attrs, "A1", "TA1bis");
    BSONObj a1nt = getAttr(attrs, "A1", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1bis, "name"));
    EXPECT_STREQ("TA1bis",C_STR_FIELD(a1bis, "type"));
    EXPECT_STREQ("val1bis", C_STR_FIELD(a1bis, "value"));
    EXPECT_FALSE(a1bis.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1nt, "name"));
    EXPECT_STREQ("",C_STR_FIELD(a1nt, "type"));
    EXPECT_STREQ("val1bis1", C_STR_FIELD(a1nt, "value"));
    EXPECT_FALSE(a1nt.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("new_val2", C_STR_FIELD(a3, "value"));
    EXPECT_EQ(1360232700, a3.getIntField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    a1bis = getAttr(attrs, "A1", "TA1bis");
    a1nt = getAttr(attrs, "A1", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1bis, "name"));
    EXPECT_STREQ("TA1bis",C_STR_FIELD(a1bis, "type"));
    EXPECT_STREQ("val1bis-nt", C_STR_FIELD(a1bis, "value"));
    EXPECT_FALSE(a1bis.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1nt, "name"));
    EXPECT_STREQ("",C_STR_FIELD(a1nt, "type"));
    EXPECT_STREQ("val1bis1-nt", C_STR_FIELD(a1nt, "value"));
    EXPECT_FALSE(a1nt.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    /* Release mock */
    delete timerMock;

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

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContext(&req, &res);

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
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    BSONObj a1bis = getAttr(attrs, "A1", "TA1bis");
    BSONObj a1nt = getAttr(attrs, "A1", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("new_val2", C_STR_FIELD(a2, "value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1bis, "name"));
    EXPECT_STREQ("TA1bis",C_STR_FIELD(a1bis, "type"));
    EXPECT_STREQ("val1bis", C_STR_FIELD(a1bis, "value"));
    EXPECT_FALSE(a1bis.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1nt, "name"));
    EXPECT_STREQ("",C_STR_FIELD(a1nt, "type"));
    EXPECT_STREQ("val1bis1", C_STR_FIELD(a1nt, "value"));
    EXPECT_FALSE(a1nt.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("new_val3", C_STR_FIELD(a3, "value"));
    EXPECT_EQ(1360232700, a3.getIntField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_STREQ("new_val4", C_STR_FIELD(a4, "value"));
    EXPECT_EQ(1360232700, a4.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    a1bis = getAttr(attrs, "A1", "TA1bis");
    a1nt = getAttr(attrs, "A1", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1bis, "name"));
    EXPECT_STREQ("TA1bis",C_STR_FIELD(a1bis, "type"));
    EXPECT_STREQ("val1bis-nt", C_STR_FIELD(a1bis, "value"));
    EXPECT_FALSE(a1bis.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1nt, "name"));
    EXPECT_STREQ("",C_STR_FIELD(a1nt, "type"));
    EXPECT_STREQ("val1bis1-nt", C_STR_FIELD(a1nt, "value"));
    EXPECT_FALSE(a1nt.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    /* Release mock */
    delete timerMock;

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

    /* Prepare mock */
    TimerMock* timerMock = new TimerMock();
    ON_CALL(*timerMock, getCurrentTime())
            .WillByDefault(Return(1360232700));
    setTimer(timerMock);

    /* Invoke the function in mongoBackend library */
    ms = mongoNotifyContext(&req, &res);

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
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    BSONObj a1bis = getAttr(attrs, "A1", "TA1bis");
    BSONObj a1nt = getAttr(attrs, "A1", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1bis, "name"));
    EXPECT_STREQ("TA1bis",C_STR_FIELD(a1bis, "type"));
    EXPECT_STREQ("val1bis", C_STR_FIELD(a1bis, "value"));
    EXPECT_FALSE(a1bis.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1nt, "name"));
    EXPECT_STREQ("",C_STR_FIELD(a1nt, "type"));
    EXPECT_STREQ("val1bis1", C_STR_FIELD(a1nt, "value"));
    EXPECT_FALSE(a1nt.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_TRUE(ent.hasField("creDate"));
    EXPECT_TRUE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_TRUE(a1.hasField("creDate"));
    EXPECT_TRUE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    a1bis = getAttr(attrs, "A1", "TA1bis");
    a1nt = getAttr(attrs, "A1", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1bis, "name"));
    EXPECT_STREQ("TA1bis",C_STR_FIELD(a1bis, "type"));
    EXPECT_STREQ("val1bis-nt", C_STR_FIELD(a1bis, "value"));
    EXPECT_FALSE(a1bis.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1nt, "name"));
    EXPECT_STREQ("",C_STR_FIELD(a1nt, "type"));
    EXPECT_STREQ("val1bis1-nt", C_STR_FIELD(a1nt, "value"));
    EXPECT_FALSE(a1nt.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    /* Release mock */
    delete timerMock;

}

