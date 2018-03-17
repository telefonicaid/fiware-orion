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
#include <vector>

#include "gtest/gtest.h"
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "orionTypes/OrionValueType.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "parse/CompoundValueNode.h"

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



/* ****************************************************************************
*
* Tests
*
* - createEntityCompoundValue1
* - createEntityCompoundValue2
* - createEntityCompoundValue1PlusSimpleValue
* - createEntityCompoundValue2PlusSimpleValue
*
* - createEntityCompoundValue1Native
* - createEntityCompoundValue2Native
* - createEntityCompoundValue1PlusSimpleValueNative
* - createEntityCompoundValue2PlusSimpleValueNative
*
* - appendCompoundValue1
* - appendCompoundValue2
* - appendCompoundValue1PlusSimpleValue
* - appendCompoundValue2PlusSimpleValue
*
* - updateSimpleToCompoundObject
* - updateCompoundObjectToSimple
* - appendAsUpdateSimpleToCompoundObject
* - appendAsUpdateCompoundObjectToSimple
*
* - updateSimpleToCompundVector
* - updateCompountVectorToSimple
* - appendAsUpdateSimpleToCompoundVector
* - appendAsUpdateCompoundVectorToSimple
*
* Compound 1: [ 22, { x: [x1, x2], y: 3 }, [ z1, z2 ] ]
*
* Compound 2: { x: { x1: a, x2: b }, y: [ y1, y2 ] }
*
*/


// Compound1: [ 22, { x: [x1, x2], y: 3 }, [ z1, z2 ] ]
#define CREATE_COMPOUND1(cv)                                             \
    orion::CompoundValueNode*  str;                                      \
    orion::CompoundValueNode*  vec;                                      \
    orion::CompoundValueNode*  x;                                        \
                                                                         \
    cv = new orion::CompoundValueNode(orion::ValueTypeVector);           \
                                                                         \
    cv->add(orion::ValueTypeString,         "",  "22");                  \
    str = cv->add(orion::ValueTypeObject,   "",  "");                    \
    vec = cv->add(orion::ValueTypeVector,   "",  "");                    \
                                                                         \
    x = str->add(orion::ValueTypeVector,    "x", "");                    \
    str->add(orion::ValueTypeString,        "y", "3");                   \
                                                                         \
    x->add(orion::ValueTypeString,          "",  "x1");                  \
    x->add(orion::ValueTypeString,          "",  "x2");                  \
                                                                         \
    vec->add(orion::ValueTypeString,        "",  "z1");                  \
    vec->add(orion::ValueTypeString,        "",  "z2");                  \
                                                                         \
    cv->shortShow("shortShow1: ");                                       \
    cv->show("show1: ");


// Compound2: { x: { x1: a, x2: b }, y: [ y1, y2 ] }
#define CREATE_COMPOUND2(cv)                                             \
    orion::CompoundValueNode*  x;                                        \
    orion::CompoundValueNode*  y;                                        \
    orion::CompoundValueNode*  leaf;                                     \
                                                                         \
    cv = new orion::CompoundValueNode(orion::ValueTypeObject);           \
                                                                         \
    x = cv->add(orion::ValueTypeObject, "x",  "");                       \
    y = cv->add(orion::ValueTypeVector, "y",  "");                       \
                                                                         \
    x->add(orion::ValueTypeString,      "x1", "a");                      \
    x->add(orion::ValueTypeString,      "x2", "b");                      \
                                                                         \
    y->add(orion::ValueTypeString,      "",   "y1");                     \
    y->add(orion::ValueTypeString,      "",   "y2");                     \
                                                                         \
    cv->shortShow("shortShow2: ");                                       \
    cv->show("show2: ");


// Compound1 native: [ 22.0, { x: [x1, x2], y: 3.0, z: null}, [ z1, false, null ] ]
#define CREATE_COMPOUND1_NATIVE(cv)                                      \
    orion::CompoundValueNode*  str;                                      \
    orion::CompoundValueNode*  vec;                                      \
    orion::CompoundValueNode*  x;                                        \
                                                                         \
    cv = new orion::CompoundValueNode(orion::ValueTypeVector);           \
                                                                         \
    cv->add(orion::ValueTypeNumber,         "",  22.0);                  \
    str  = cv->add(orion::ValueTypeObject,  "",  "");                    \
    vec  = cv->add(orion::ValueTypeVector,  "",  "");                    \
                                                                         \
    x    = str->add(orion::ValueTypeVector, "x", "");                    \
    str->add(orion::ValueTypeNumber,        "y", 3.0);                   \
    str->add(orion::ValueTypeNull,          "z", "");                    \
                                                                         \
    x->add(orion::ValueTypeString,          "",  "x1");                  \
    x->add(orion::ValueTypeString,          "",  "x2");                  \
                                                                         \
    vec->add(orion::ValueTypeString,        "",  "z1");                  \
    vec->add(orion::ValueTypeBoolean,       "",  false);                 \
    vec->add(orion::ValueTypeNull,          "",  "");                    \
                                                                         \
    cv->shortShow("shortShow1: ");                                       \
    cv->show("show1: ");


// Compound2 native: { x: { x1: a, x2: true }, y: [ y1, y2 ], z: null }
#define CREATE_COMPOUND2_NATIVE(cv)                                      \
    orion::CompoundValueNode*  x;                                        \
    orion::CompoundValueNode*  y;                                        \
                                                                         \
    cv = new orion::CompoundValueNode(orion::ValueTypeObject);           \
                                                                         \
    x    = cv->add(orion::ValueTypeObject, "x",  "");                    \
    y    = cv->add(orion::ValueTypeVector, "y",  "");                    \
    cv->add(orion::ValueTypeNull,          "z",  "");                    \
                                                                         \
    x->add(orion::ValueTypeString,         "x1", "a");                   \
    x->add(orion::ValueTypeBoolean,        "x2", true);                  \
                                                                         \
    y->add(orion::ValueTypeString,         "",   "y1");                  \
    y->add(orion::ValueTypeString,         "",   "y2");                  \
                                                                         \
    cv->shortShow("shortShow2: ");                                       \
    cv->show("show2: ");



/* ****************************************************************************
*
* prepareDatabaseSimple -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabaseSimple(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  BSONObj en = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrNames" << BSON_ARRAY("AX") <<
                     "attrs" << BSON(
                       "AX" << BSON("type" << "TAX" << "value" << "valX")));

  connection->insert(ENTITIES_COLL, en);
}



/* ****************************************************************************
*
* prepareDatabaseCompoundVector -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabaseCompoundVector(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  BSONObj en = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                    "attrNames" << BSON_ARRAY("AX") <<
                    "attrs" << BSON(
                      "AX" << BSON("type" << "TAX" << "value" << BSON_ARRAY("A" << "B"))));

  connection->insert(ENTITIES_COLL, en);
}



/* ****************************************************************************
*
* prepareDatabaseCompoundObject -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabaseCompoundObject(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  BSONObj en = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                    "attrNames" << BSON_ARRAY("AX") <<
                    "attrs" << BSON(
                      "AX" << BSON("type" << "TAX" << "value" << BSON("x" << "A" << "y" << "B"))));


  connection->insert(ENTITIES_COLL, en);
}



/* ****************************************************************************
*
* findAttr -
*
* FIXME P4: this functions is repeated in several places (e.g. mongoUpdateContext_test.cpp). Factorice in a common place.
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
* createCompoundValue1 -
*/
TEST(mongoUpdateContextCompoundValuesRequest, createCompoundValue1)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E3", "T3", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND1(cv)
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", cv);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).entityId.id);
    EXPECT_EQ("T3", RES_CER(0).entityId.type);
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
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
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
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E3", "T3", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND2(cv)
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", cv);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).entityId.id);
    EXPECT_EQ("T3", RES_CER(0).entityId.type);
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
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
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
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E3", "T3", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND1(cv)
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", cv);
    ContextAttribute* ca2P = new ContextAttribute("A2", "TA2", "simple2");
    ceP->contextAttributeVector.push_back(ca1P);
    ceP->contextAttributeVector.push_back(ca2P);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).entityId.id);
    EXPECT_EQ("T3", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
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

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_EQ("22", a1.getField("value").Array()[0].str());
    EXPECT_EQ("x1", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[0].str());
    EXPECT_EQ("x2", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[1].str());
    EXPECT_EQ("3", a1.getField("value").Array()[1].embeddedObject().getField("y").str());
    EXPECT_EQ("z1", a1.getField("value").Array()[2].Array()[0].str());
    EXPECT_EQ("z2", a1.getField("value").Array()[2].Array()[1].str());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("simple2", C_STR_FIELD(a2, "value"));
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
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E3", "T3", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND2(cv)
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", cv);
    ContextAttribute* ca2P = new ContextAttribute("A2", "TA2", "simple2");
    ceP->contextAttributeVector.push_back(ca1P);
    ceP->contextAttributeVector.push_back(ca2P);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).entityId.id);
    EXPECT_EQ("T3", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
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

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_EQ("a", a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x1").str());
    EXPECT_EQ("b", a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x2").str());
    EXPECT_EQ("y1", a1.getField("value").embeddedObject().getField("y").Array()[0].str());
    EXPECT_EQ("y2", a1.getField("value").embeddedObject().getField("y").Array()[1].str());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("simple2", C_STR_FIELD(a2, "value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));
    EXPECT_EQ(1360232700, a2.getIntField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* createCompoundValue1Native -
*/
TEST(mongoUpdateContextCompoundValuesRequest, createCompoundValue1Native)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E3", "T3", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND1_NATIVE(cv)
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", cv);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).entityId.id);
    EXPECT_EQ("T3", RES_CER(0).entityId.type);
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
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_EQ(22.0, a1.getField("value").Array()[0].Number());
    EXPECT_EQ("x1", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[0].str());
    EXPECT_EQ("x2", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[1].str());
    EXPECT_EQ(3.0, a1.getField("value").Array()[1].embeddedObject().getField("y").Number());
    EXPECT_TRUE(a1.getField("value").Array()[1].embeddedObject().getField("z").isNull());
    EXPECT_EQ("z1", a1.getField("value").Array()[2].Array()[0].str());
    EXPECT_FALSE(a1.getField("value").Array()[2].Array()[1].Bool());
    EXPECT_TRUE(a1.getField("value").Array()[2].Array()[2].isNull());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* createCompoundValue2Native -
*/
TEST(mongoUpdateContextCompoundValuesRequest, createCompoundValue2Native)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E3", "T3", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND2_NATIVE(cv)
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", cv);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).entityId.id);
    EXPECT_EQ("T3", RES_CER(0).entityId.type);
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
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_EQ("a", a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x1").str());
    EXPECT_TRUE(a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x2").Bool());
    EXPECT_EQ("y1", a1.getField("value").embeddedObject().getField("y").Array()[0].str());
    EXPECT_EQ("y2", a1.getField("value").embeddedObject().getField("y").Array()[1].str());
    EXPECT_TRUE(a1.getField("value").embeddedObject().getField("z").isNull());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* createCompoundValue1PlusSimpleValueNative -
*/
TEST(mongoUpdateContextCompoundValuesRequest, createCompoundValue1PlusSimpleValueNative)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E3", "T3", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND1_NATIVE(cv)
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", cv);
    ContextAttribute* ca2P = new ContextAttribute("A2", "TA2", "simple2");
    ceP->contextAttributeVector.push_back(ca1P);
    ceP->contextAttributeVector.push_back(ca2P);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).entityId.id);
    EXPECT_EQ("T3", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
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

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_EQ(22.0, a1.getField("value").Array()[0].Number());
    EXPECT_EQ("x1", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[0].str());
    EXPECT_EQ("x2", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[1].str());
    EXPECT_EQ(3.0, a1.getField("value").Array()[1].embeddedObject().getField("y").Number());
    EXPECT_TRUE(a1.getField("value").Array()[1].embeddedObject().getField("z").isNull());
    EXPECT_EQ("z1", a1.getField("value").Array()[2].Array()[0].str());
    EXPECT_FALSE(a1.getField("value").Array()[2].Array()[1].Bool());
    EXPECT_TRUE(a1.getField("value").Array()[2].Array()[2].isNull());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("simple2", C_STR_FIELD(a2, "value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));
    EXPECT_EQ(1360232700, a2.getIntField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* createCompoundValue2PlusSimpleValueNative -
*/
TEST(mongoUpdateContextCompoundValuesRequest, createCompoundValue2PlusSimpleValueNative)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E3", "T3", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND2_NATIVE(cv)
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", cv);
    ContextAttribute* ca2P = new ContextAttribute("A2", "TA2", "simple2");
    ceP->contextAttributeVector.push_back(ca1P);
    ceP->contextAttributeVector.push_back(ca2P);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).entityId.id);
    EXPECT_EQ("T3", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
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

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_EQ("a", a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x1").str());
    EXPECT_TRUE(a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x2").Bool());
    EXPECT_EQ("y1", a1.getField("value").embeddedObject().getField("y").Array()[0].str());
    EXPECT_EQ("y2", a1.getField("value").embeddedObject().getField("y").Array()[1].str());
    EXPECT_TRUE(a1.getField("value").embeddedObject().getField("z").isNull());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("simple2", C_STR_FIELD(a2, "value"));
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
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();
    prepareDatabaseSimple();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND1(cv)
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", cv);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
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
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj aX = attrs.getField("AX").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "AX"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_EQ("22", a1.getField("value").Array()[0].str());
    EXPECT_EQ("x1", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[0].str());
    EXPECT_EQ("x2", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[1].str());
    EXPECT_EQ("3", a1.getField("value").Array()[1].embeddedObject().getField("y").str());
    EXPECT_EQ("z1", a1.getField("value").Array()[2].Array()[0].str());
    EXPECT_EQ("z2", a1.getField("value").Array()[2].Array()[1].str());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));
    EXPECT_STREQ("TAX", C_STR_FIELD(aX, "type"));
    EXPECT_STREQ("valX", C_STR_FIELD(aX, "value"));
    EXPECT_FALSE(aX.hasField("modDate"));
    EXPECT_FALSE(aX.hasField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* appendCompoundValue2 -
*/
TEST(mongoUpdateContextCompoundValuesRequest, appendCompoundValue2)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();
    prepareDatabaseSimple();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND2(cv)
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", cv);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
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
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj aX = attrs.getField("AX").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "AX"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_EQ("a", a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x1").str());
    EXPECT_EQ("b", a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x2").str());
    EXPECT_EQ("y1", a1.getField("value").embeddedObject().getField("y").Array()[0].str());
    EXPECT_EQ("y2", a1.getField("value").embeddedObject().getField("y").Array()[1].str());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));
    EXPECT_STREQ("TAX", C_STR_FIELD(aX, "type"));
    EXPECT_STREQ("valX", C_STR_FIELD(aX, "value"));
    EXPECT_FALSE(aX.hasField("modDate"));
    EXPECT_FALSE(aX.hasField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* appendCompoundValue1PlusSimpleValue -
*/
TEST(mongoUpdateContextCompoundValuesRequest, appendCompoundValue1PlusSimpleValue)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();
    prepareDatabaseSimple();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND1(cv)
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", cv);
    ContextAttribute* ca2P = new ContextAttribute("A2", "TA2", "simple2");
    ceP->contextAttributeVector.push_back(ca1P);
    ceP->contextAttributeVector.push_back(ca2P);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
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
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
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
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(3, attrs.nFields());
    ASSERT_EQ(3, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    BSONObj aX = attrs.getField("AX").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_TRUE(findAttr(attrNames, "AX"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_EQ("22", a1.getField("value").Array()[0].str());
    EXPECT_EQ("x1", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[0].str());
    EXPECT_EQ("x2", a1.getField("value").Array()[1].embeddedObject().getField("x").Array()[1].str());
    EXPECT_EQ("3", a1.getField("value").Array()[1].embeddedObject().getField("y").str());
    EXPECT_EQ("z1", a1.getField("value").Array()[2].Array()[0].str());
    EXPECT_EQ("z2", a1.getField("value").Array()[2].Array()[1].str());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("simple2", C_STR_FIELD(a2, "value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));
    EXPECT_EQ(1360232700, a2.getIntField("creDate"));
    EXPECT_STREQ("TAX", C_STR_FIELD(aX, "type"));
    EXPECT_STREQ("valX", C_STR_FIELD(aX, "value"));
    EXPECT_FALSE(aX.hasField("modDate"));
    EXPECT_FALSE(aX.hasField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* appendCompoundValue2PlusSimpleValue -
*/
TEST(mongoUpdateContextCompoundValuesRequest, appendCompoundValue2PlusSimpleValue)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();
    prepareDatabaseSimple();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND2(cv)
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", cv);
    ContextAttribute* ca2P = new ContextAttribute("A2", "TA2", "simple2");
    ceP->contextAttributeVector.push_back(ca1P);
    ceP->contextAttributeVector.push_back(ca2P);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
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
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
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
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(3, attrs.nFields());
    ASSERT_EQ(3, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    BSONObj aX = attrs.getField("AX").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_TRUE(findAttr(attrNames, "AX"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_EQ("a", a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x1").str());
    EXPECT_EQ("b", a1.getField("value").embeddedObject().getField("x").embeddedObject().getField("x2").str());
    EXPECT_EQ("y1", a1.getField("value").embeddedObject().getField("y").Array()[0].str());
    EXPECT_EQ("y2", a1.getField("value").embeddedObject().getField("y").Array()[1].str());
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("simple2", C_STR_FIELD(a2, "value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));
    EXPECT_EQ(1360232700, a2.getIntField("creDate"));
    EXPECT_STREQ("TAX", C_STR_FIELD(aX, "type"));
    EXPECT_STREQ("valX", C_STR_FIELD(aX, "value"));
    EXPECT_FALSE(aX.hasField("modDate"));
    EXPECT_FALSE(aX.hasField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* updateSimpleToCompoundObject -
*/
TEST(mongoUpdateContextCompoundValuesRequest, updateSimpleToCompoundObject)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();
    prepareDatabaseSimple();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND2(cv)
    ContextAttribute* caP = new ContextAttribute("AX", "TAX", cv);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
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
    EXPECT_EQ("AX", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TAX", RES_CER_ATTR(0, 0)->type);
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
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj aX = attrs.getField("AX").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "AX"));
    EXPECT_STREQ("TAX", C_STR_FIELD(aX, "type"));
    EXPECT_EQ("a", aX.getField("value").embeddedObject().getField("x").embeddedObject().getField("x1").str());
    EXPECT_EQ("b", aX.getField("value").embeddedObject().getField("x").embeddedObject().getField("x2").str());
    EXPECT_EQ("y1", aX.getField("value").embeddedObject().getField("y").Array()[0].str());
    EXPECT_EQ("y2", aX.getField("value").embeddedObject().getField("y").Array()[1].str());
    EXPECT_EQ(1360232700, aX.getIntField("modDate"));
    EXPECT_FALSE(aX.hasField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* updateCompoundObjectToSimple -
*/
TEST(mongoUpdateContextCompoundValuesRequest, updateCompoundObjectToSimple)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();
    prepareDatabaseCompoundObject();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("AX", "TAX", "new_value");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
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
    EXPECT_EQ("AX", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TAX", RES_CER_ATTR(0, 0)->type);
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
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj aX = attrs.getField("AX").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "AX"));
    EXPECT_STREQ("TAX", C_STR_FIELD(aX, "type"));
    EXPECT_STREQ("new_value", C_STR_FIELD(aX, "value"));
    EXPECT_EQ(1360232700, aX.getIntField("modDate"));
    EXPECT_FALSE(aX.hasField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
*  appendAsUpdateSimpleToCompoundObject -
*/
TEST(mongoUpdateContextCompoundValuesRequest, appendAsUpdateSimpleToCompoundObject)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();
    prepareDatabaseSimple();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND2(cv)
    ContextAttribute* caP = new ContextAttribute("AX", "TAX", cv);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
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
    EXPECT_EQ("AX", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TAX", RES_CER_ATTR(0, 0)->type);
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
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj aX = attrs.getField("AX").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "AX"));
    EXPECT_STREQ("TAX", C_STR_FIELD(aX, "type"));
    EXPECT_EQ("a", aX.getField("value").embeddedObject().getField("x").embeddedObject().getField("x1").str());
    EXPECT_EQ("b", aX.getField("value").embeddedObject().getField("x").embeddedObject().getField("x2").str());
    EXPECT_EQ("y1", aX.getField("value").embeddedObject().getField("y").Array()[0].str());
    EXPECT_EQ("y2", aX.getField("value").embeddedObject().getField("y").Array()[1].str());
    EXPECT_EQ(1360232700, aX.getIntField("modDate"));
    EXPECT_FALSE(aX.hasField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* appendAsUpdateCompoundObjectToSimple -
*/
TEST(mongoUpdateContextCompoundValuesRequest, appendAsUpdateCompoundObjectToSimple)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();
    prepareDatabaseCompoundObject();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("AX", "TAX", "new_value");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
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
    EXPECT_EQ("AX", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TAX", RES_CER_ATTR(0, 0)->type);
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
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj aX = attrs.getField("AX").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "AX"));
    EXPECT_STREQ("TAX", C_STR_FIELD(aX, "type"));
    EXPECT_STREQ("new_value", C_STR_FIELD(aX, "value"));
    EXPECT_EQ(1360232700, aX.getIntField("modDate"));
    EXPECT_FALSE(aX.hasField("creDate"));

    /* Release mock */
    utExit();
}


/* ****************************************************************************
*
* updateSimpleToCompoundVector -
*/
TEST(mongoUpdateContextCompoundValuesRequest, updateSimpleToCompoundVector)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();
    prepareDatabaseSimple();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND1(cv)
    ContextAttribute* caP = new ContextAttribute("AX", "TAX", cv);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
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
    EXPECT_EQ("AX", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TAX", RES_CER_ATTR(0, 0)->type);
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
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj aX = attrs.getField("AX").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "AX"));
    EXPECT_STREQ("TAX", C_STR_FIELD(aX, "type"));
    EXPECT_EQ("22", aX.getField("value").Array()[0].str());
    EXPECT_EQ("x1", aX.getField("value").Array()[1].embeddedObject().getField("x").Array()[0].str());
    EXPECT_EQ("x2", aX.getField("value").Array()[1].embeddedObject().getField("x").Array()[1].str());
    EXPECT_EQ("3", aX.getField("value").Array()[1].embeddedObject().getField("y").str());
    EXPECT_EQ("z1", aX.getField("value").Array()[2].Array()[0].str());
    EXPECT_EQ("z2", aX.getField("value").Array()[2].Array()[1].str());
    EXPECT_EQ(1360232700, aX.getIntField("modDate"));
    EXPECT_FALSE(aX.hasField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* updateCompoundVectorToSimple -
*/
TEST(mongoUpdateContextCompoundValuesRequest, updateCompoundVectorToSimple)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();
    prepareDatabaseCompoundVector();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("AX", "TAX", "new_value");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
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
    EXPECT_EQ("AX", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TAX", RES_CER_ATTR(0, 0)->type);
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
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj aX = attrs.getField("AX").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "AX"));
    EXPECT_STREQ("TAX", C_STR_FIELD(aX, "type"));
    EXPECT_STREQ("new_value", C_STR_FIELD(aX, "value"));
    EXPECT_EQ(1360232700, aX.getIntField("modDate"));
    EXPECT_FALSE(aX.hasField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
*  appendAsUpdateSimpleToCompoundVector -
*/
TEST(mongoUpdateContextCompoundValuesRequest, appendAsUpdateSimpleToCompoundVector)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();
    prepareDatabaseSimple();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    orion::CompoundValueNode* cv;
    CREATE_COMPOUND1(cv)
    ContextAttribute* caP = new ContextAttribute("AX", "TAX", cv);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
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
    EXPECT_EQ("AX", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TAX", RES_CER_ATTR(0, 0)->type);
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
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj aX = attrs.getField("AX").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "AX"));
    EXPECT_STREQ("TAX", C_STR_FIELD(aX, "type"));
    EXPECT_EQ("22", aX.getField("value").Array()[0].str());
    EXPECT_EQ("x1", aX.getField("value").Array()[1].embeddedObject().getField("x").Array()[0].str());
    EXPECT_EQ("x2", aX.getField("value").Array()[1].embeddedObject().getField("x").Array()[1].str());
    EXPECT_EQ("3", aX.getField("value").Array()[1].embeddedObject().getField("y").str());
    EXPECT_EQ("z1", aX.getField("value").Array()[2].Array()[0].str());
    EXPECT_EQ("z2", aX.getField("value").Array()[2].Array()[1].str());
    EXPECT_EQ(1360232700, aX.getIntField("modDate"));
    EXPECT_FALSE(aX.hasField("creDate"));

    /* Release mock */
    utExit();
}

/* ****************************************************************************
*
* appendAsUpdateCompoundVectorToSimple -
*/
TEST(mongoUpdateContextCompoundValuesRequest, appendAsUpdateCompoundVectorToSimple)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();
    prepareDatabaseCompoundObject();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("AX", "TAX", "new_value");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
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
    EXPECT_EQ("AX", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TAX", RES_CER_ATTR(0, 0)->type);
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
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj aX = attrs.getField("AX").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "AX"));
    EXPECT_STREQ("TAX", C_STR_FIELD(aX, "type"));
    EXPECT_STREQ("new_value", C_STR_FIELD(aX, "value"));
    EXPECT_EQ(1360232700, aX.getIntField("modDate"));
    EXPECT_FALSE(aX.hasField("creDate"));

    /* Release mock */
    utExit();
}
