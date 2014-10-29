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
* Author: Ken Zangelin
*/
#include "gtest/gtest.h"
#include "testInit.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "convenienceMap/mapDeleteIndividualContextEntityAttribute.h"
#include "rest/ConnectionInfo.h"

#include "commonMocks.h"

using ::testing::_;
using ::testing::Throw;
using ::testing::Return;



/* ****************************************************************************
*
* prepareDatabase -
*/
static void prepareDatabase(std::string id, std::string type)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create one entity:
   *
   * - 'id', 'type' with four attributes
   *     A1: X
   *     A1: Y
   *     A2: Z
   *     A3: W
   */

  BSONObj en1 = BSON("_id" << BSON("id" << id << "type" << type) <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A1" << "type" << "TA1" << "value" << "X") <<
                        BSON("name" << "A1" << "type" << "TA1bis" << "value" << "Y") <<
                        BSON("name" << "A2" << "type" << "TA2" << "value" << "Z") <<
                        BSON("name" << "A3" << "type" << "TA3" << "value" << "W")
                        )
     );

  connection->insert(ENTITIES_COLL, en1);
}



/* ****************************************************************************
*
* notFound - 
*/
TEST(mapDeleteIndividualContextEntityAttribute, notFound)
{
  StatusCode      sc;
  ConnectionInfo  ci;

  prepareDatabase("ID", "NAME");

  mapDeleteIndividualContextEntityAttribute("ID2", "", "NAME2", &sc, &ci);

  EXPECT_EQ(SccContextElementNotFound, sc.code);
  EXPECT_STREQ("No context element found", sc.reasonPhrase.c_str());
  EXPECT_STREQ("ID2", sc.details.c_str());

  mongoDisconnect();
}



/* ****************************************************************************
*
* ok - 
*/
TEST(mapDeleteIndividualContextEntityAttribute, ok)
{
  std::string     id        = "ID";
  std::string     name      = "A1";
  ConnectionInfo  ci;
  StatusCode      sc;

  prepareDatabase("ID", "TYPE");

  TimerMock* timerMock = new TimerMock();
  ON_CALL(*timerMock, getCurrentTime())
    .WillByDefault(Return(1360232700));
  setTimer(timerMock);

  mapDeleteIndividualContextEntityAttribute(id, "", name, &sc, &ci);

  EXPECT_EQ(SccOk, sc.code);
  EXPECT_STREQ("OK", sc.reasonPhrase.c_str());
  EXPECT_STREQ("", sc.details.c_str());

  mongoDisconnect();
  delete timerMock;
}
