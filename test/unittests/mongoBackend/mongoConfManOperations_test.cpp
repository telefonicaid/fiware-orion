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
#include "gtest/gtest.h"
#include "testInit.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoConfManOperations.h"

#include "mongo/client/dbclient.h"

#include "commonMocks.h"

/* ****************************************************************************
*
* Tests
*
* - getFwdRegId
* - setFwdRegId
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

  BSONObj cr1 = BSON("providingApplication" << "http://cr1.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E1" << "type" << "T1")
                         ) <<
                     "attrs" << BSON_ARRAY(
                         BSON("name" << "A1" << "type" << "TA1" << "isDomain" << "true")                         )
                     );

  BSONObj reg1 = BSON(
              "_id" << OID("51307b66f481db11bf860001") <<
              "expiration" << 10000000 <<
              "contextRegistration" << BSON_ARRAY(cr1)
              );

  BSONObj cr2 = BSON("providingApplication" << "http://cr2.com" <<
                     "entities" << BSON_ARRAY(
                         BSON("id" << "E2" << "type" << "T2")
                         ) <<
                     "attrs" << BSON_ARRAY(
                         BSON("name" << "A2" << "type" << "TA2" << "isDomain" << "true")                         )
                     );

  BSONObj reg2 = BSON(
              "_id" << OID("51307b66f481db11bf860002") <<
              "fwdRegId" << "51307b66f481db11bf860003" <<
              "expiration" << 10000000 <<
              "contextRegistration" << BSON_ARRAY(cr1)
              );

  connection->insert(REGISTRATIONS_COLL, reg1);
  connection->insert(REGISTRATIONS_COLL, reg2);

}

/* ****************************************************************************
*
* getFwdRegId -
*
*/
TEST(mongoConfManOperations, getFwdRegId)
{
    /* Setup database */
    prepareDatabase();

    /* Do operation and test */
    EXPECT_EQ("51307b66f481db11bf860003", mongoGetFwdRegId("51307b66f481db11bf860002"));
}

/* ****************************************************************************
*
* setFwdRegId -
*
*/
TEST(mongoConfManOperations, setFwdRegId)
{
    /* Setup database */
    prepareDatabase();

    /* Do operation */
    mongoSetFwdRegId("51307b66f481db11bf860001", "51307b66f481db11bf860004");

    /* Check database is as expected*/
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */
    DBClientBase* connection = getMongoConnection();
    BSONObj reg1 = connection->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860001")));
    BSONObj reg2 = connection->findOne(REGISTRATIONS_COLL, BSON("_id" << OID("51307b66f481db11bf860002")));
    EXPECT_STREQ("51307b66f481db11bf860004", C_STR_FIELD(reg1, "fwdRegId"));
    EXPECT_STREQ("51307b66f481db11bf860003", C_STR_FIELD(reg2, "fwdRegId"));

}

