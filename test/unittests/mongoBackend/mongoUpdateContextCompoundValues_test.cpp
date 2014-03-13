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
#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "parse/CompoundValueNode.h"

#include "mongo/client/dbclient.h"


/* ****************************************************************************
*
* Tests
*
* - createEntityCompoundValue1
* - createEntityCompoundValue2
* - createEntityCompoundValue1PlusSimpleValue
* - createEntityCompoundValue2PlusSimpleValue
* - appendCompoundValue1
* - appendCompoundValue2
* - appendCompoundValue1PlusSimpleValue
* - appendCompoundValue2PlusSimpleValue
* - updateSimpleToCompund
* - updateCompountToSimple
* - appendSimpleToCompund
* - appendCompountToSimple
*
* Compound 1 is based in: [ 22, {x: [x1, x2], y: 3}, [z1, z2] ]
*
* Compound 2 is based in: { x: {x1: a, x2: b}, y: [ y1, y2 ] }
*
*/

#define CREATE_COMPOUND1(cv) \
    orion::CompoundValueNode cv21, cv22, cv23, cv31, cv32, cv41, cv42, cv43, cv44; \
    \
    /* 4th level nodes */ \
    cv41.type = orion::CompoundValueNode::Leaf; \
    cv41.value = "x1"; \
    \
    cv42.type = orion::CompoundValueNode::Leaf; \
    cv42.value = "x2"; \
    \
    cv43.type = orion::CompoundValueNode::Leaf; \
    cv43.value = "z1"; \
    \
    cv44.type = orion::CompoundValueNode::Leaf; \
    cv44.value = "z2"; \
    \
    /* 3rd level nodes */ \
    cv31.type = orion::CompoundValueNode::Vector; \
    cv31.name = "x"; \
    cv31.childV.push_back(&cv41);  /* x1 */ \
    cv31.childV.push_back(&cv42);  /* x2 */ \
    \
    cv32.type = orion::CompoundValueNode::Leaf; \
    cv32.name = "y"; \
    cv32.value = "3"; \
    \
    /* 2nd level nodes */ \
    cv21.type = orion::CompoundValueNode::Leaf; \
    cv21.value = "22"; \
    \
    cv22.type = orion::CompoundValueNode::Struct; \
    cv22.childV.push_back(&cv31);  /* x: [x1, x2] */ \
    cv22.childV.push_back(&cv32);  /* y: 3 */ \
    \
    cv23.type = orion::CompoundValueNode::Vector; \
    cv23.childV.push_back(&cv43);  /* z1 */ \
    cv23.childV.push_back(&cv44);  /* z2 */ \
    \
    /* 1st level node */ \
    cv.type = orion::CompoundValueNode::Vector; \
    cv.childV.push_back(&cv21);  /* 22 */ \
    cv.childV.push_back(&cv22);  /* {x: [x1, x2], y: 3} */ \
    cv.childV.push_back(&cv23);  /* [z1, z2] */ \
    \
    cv.shortShow("shortShow: "); \
    cv.show("show: "); \

#define CREATE_COMPOUND2(cv) \
    orion::CompoundValueNode cv21, cv22, cv31, cv32, cv33, cv34; \
    \
    /* 3rd level nodes */ \
    cv31.type = orion::CompoundValueNode::Leaf; \
    cv31.name = "x1"; \
    cv31.value = "a"; \
    \
    cv32.type = orion::CompoundValueNode::Leaf; \
    cv32.name = "x2"; \
    cv32.value = "b"; \
    \
    cv33.type = orion::CompoundValueNode::Leaf; \
    cv33.value = "y1"; \
    \
    cv34.type = orion::CompoundValueNode::Leaf; \
    cv34.value = "y2"; \
    \
    /* 2nd level nodes */ \
    cv21.type = orion::CompoundValueNode::Struct; \
    cv21.name = "x"; \
    cv21.childV.push_back(&cv31);  /* x1: a */ \
    cv21.childV.push_back(&cv32);  /* x2: b */ \
    \
    cv22.type = orion::CompoundValueNode::Vector; \
    cv22.name = "y"; \
    cv22.childV.push_back(&cv33);  /* y1 */ \
    cv22.childV.push_back(&cv34);  /* y2 */ \
    \
    /* 1st level node */ \
    cv.type = orion::CompoundValueNode::Struct; \
    cv.childV.push_back(&cv21);  /* x: {x1: a, x2: b} */ \
    cv.childV.push_back(&cv22);  /* y: [y1, y2] */ \
    \
    cv.shortShow("shortShow: "); \
    cv.show("show: "); \

/* ****************************************************************************
*
* getAttr -
*
* We need this function because we can not trust on array index, at mongo will
* not sort the elements within the array. This function assumes that always will
* find a result, that is ok for testing code.
*/
static BSONObj getAttr(std::vector<BSONElement> attrs, std::string name, std::string type, std::string id = "") {

    BSONElement be;
    for (unsigned int ix = 0; ix < attrs.size(); ++ix) {
        BSONObj attr = attrs[ix].embeddedObject();
        std::string attrName = STR_FIELD(attr, "name");
        std::string attrType = STR_FIELD(attr, "type");
        std::string attrId = STR_FIELD(attr, "id");
        if (attrName == name && attrType == type && ( id == "" || attrId == id )) {
            be = attrs[ix];
            break;
        }
    }
    return be.embeddedObject();

}

/* ****************************************************************************
*
* createCompoundValue1 -
*/
TEST(mongoUpdateContextCompoundValuesRequest, createCompoundValue1)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    orion::CompoundValueNode cv;
    CREATE_COMPOUND1(cv)
    ContextAttribute ca("A1", "TA1", &cv);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

     /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_EQ("22", a1.getField("value").Array()[0].str());
    EXPECT_EQ("x1", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[0].str());
    EXPECT_EQ("x2", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[1].str());
    EXPECT_EQ("3", a1.getField("value").Array()[1].embeddedObject().getField("y").str());
    EXPECT_EQ("z1", a1.getField("value").Array()[2].Array()[0].str());
    EXPECT_EQ("z2", a1.getField("value").Array()[2].Array()[1].str());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* createCompoundValue2 -
*/
TEST(mongoUpdateContextCompoundValuesRequest, createCompoundValue2)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    orion::CompoundValueNode cv;
    CREATE_COMPOUND2(cv)
    ContextAttribute ca("A1", "TA1", &cv);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
   /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
    * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

   DBClientConnection* connection = getMongoConnection();

   /* entities collection */
   BSONObj ent;
   std::vector<BSONElement> attrs;
   ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_EQ("a", a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x1").str());
    EXPECT_EQ("b", a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x2").str());
    EXPECT_EQ("y1", a1.getField("value").embeddedObject().getField("y").Array()[0].str());
    EXPECT_EQ("y2", a1.getField("value").embeddedObject().getField("y").Array()[1].str());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* createCompoundValue1PlusSimpleValue -
*/
TEST(mongoUpdateContextCompoundValuesRequest, createCompoundValue1PlusSimpleValue)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    orion::CompoundValueNode cv;
    CREATE_COMPOUND1(cv)
    ContextAttribute ca1("A1", "TA1", &cv);
    ContextAttribute ca2("A2", "TA2", "simple2");
    ce.contextAttributeVector.push_back(&ca1);
    ce.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

     /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientConnection* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_EQ("22", a1.getField("value").Array()[0].str());
    EXPECT_EQ("x1", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[0].str());
    EXPECT_EQ("x2", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[1].str());
    EXPECT_EQ("3", a1.getField("value").Array()[1].embeddedObject().getField("y").str());
    EXPECT_EQ("z1", a1.getField("value").Array()[2].Array()[0].str());
    EXPECT_EQ("z2", a1.getField("value").Array()[2].Array()[1].str());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2",C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("simple2",C_STR_FIELD(a2, "value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));
    EXPECT_EQ(1360232700, a2.getIntField("creDate"));


    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* createCompoundValue2PlusSimpleValue -
*/
TEST(mongoUpdateContextCompoundValuesRequest, createCompoundValue2PlusSimpleValue)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    orion::CompoundValueNode cv;
    CREATE_COMPOUND2(cv)
    ContextAttribute ca1("A1", "TA1", &cv);
    ContextAttribute ca2("A2", "TA2", "simple2");
    ce.contextAttributeVector.push_back(&ca1);
    ce.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
   /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
    * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

   DBClientConnection* connection = getMongoConnection();

   /* entities collection */
   BSONObj ent;
   std::vector<BSONElement> attrs;
   ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_EQ("a", a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x1").str());
    EXPECT_EQ("b", a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x2").str());
    EXPECT_EQ("y1", a1.getField("value").embeddedObject().getField("y").Array()[0].str());
    EXPECT_EQ("y2", a1.getField("value").embeddedObject().getField("y").Array()[1].str());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2",C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("simple2",C_STR_FIELD(a2, "value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));
    EXPECT_EQ(1360232700, a2.getIntField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* appendCompoundValue1 -
*/
TEST(mongoUpdateContextCompoundValuesRequest, appendCompoundValue1)
{
    EXPECT_EQ(1, 0) << "to be implemented";
}

/* ****************************************************************************
*
* appendCompoundValue2 -
*/
TEST(mongoUpdateContextCompoundValuesRequest, appendCompoundValue2)
{
    EXPECT_EQ(1, 0) << "to be implemented";
}

/* ****************************************************************************
*
* appendCompoundValue1PlusSimpleValue -
*/
TEST(mongoUpdateContextCompoundValuesRequest, appendCompoundValue1PlusSimpleValue)
{
    EXPECT_EQ(1, 0) << "to be implemented";
}

/* ****************************************************************************
*
* appendCompoundValue2PlusSimpleValue -
*/
TEST(mongoUpdateContextCompoundValuesRequest, appendCompoundValue2PlusSimpleValue)
{
    EXPECT_EQ(1, 0) << "to be implemented";
}

/* ****************************************************************************
*
* updateSimpleToCompund -
*/
TEST(mongoUpdateContextCompoundValuesRequest, updateSimpleToCompund)
{
    EXPECT_EQ(1, 0) << "to be implemented";
}

/* ****************************************************************************
*
* updateCompountToSimple -
*/
TEST(mongoUpdateContextCompoundValuesRequest, updateCompountToSimple)
{
    EXPECT_EQ(1, 0) << "to be implemented";
}

/* ****************************************************************************
*
*  appendSimpleToCompund -
*/
TEST(mongoUpdateContextCompoundValuesRequest, appendSimpleToCompund)
{
    EXPECT_EQ(1, 0) << "to be implemented";
}

/* ****************************************************************************
*
* appendCompountToSimple -
*/
TEST(mongoUpdateContextCompoundValuesRequest, appendCompountToSimple)
{
    EXPECT_EQ(1, 0) << "to be implemented";
}
