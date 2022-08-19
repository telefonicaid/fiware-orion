#ifndef TEST_UNITTESTS_UNITTEST_H_
#define TEST_UNITTESTS_UNITTEST_H_

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
* Author: Ken Zangelin
*/
#include <string>
#include <vector>
#include <map>

#include "gtest/gtest.h"

#include "unittests/testDataFromFile.h"
#include "unittests/commonMocks.h"
#include "unittests/testInit.h"



/* ****************************************************************************
*
* uriParams -
*/
extern std::map<std::string, std::string> uriParams;



/* ****************************************************************************
*
* options -
*/
extern std::map<std::string, bool> options;



/* ****************************************************************************
*
* servicePathVector -
*/
extern std::vector<std::string> servicePathVector;



/* ****************************************************************************
*
* utInit - unit test init
*/
extern void utInit(bool notifierMocked = true, bool timerMocked = true);



/* ****************************************************************************
*
* utExit - unit test exit
*/
extern void utExit(void);

#endif  // TEST_UNITTESTS_UNITTEST_H_
