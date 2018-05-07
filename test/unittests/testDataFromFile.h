#ifndef TEST_UNITTESTS_TESTDATAFROMFILE_H_
#define TEST_UNITTESTS_TESTDATAFROMFILE_H_

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



/* ****************************************************************************
*
* TEST_BUFFER_SIZE - 
*/
#define TEST_BUFFER_SIZE 8192



/* ****************************************************************************
*
* testBuf - writable char vector to hold the input data (from TEXT segment)
*/
extern char testBuf[TEST_BUFFER_SIZE];



/* ****************************************************************************
*
* expectedBuf - writable char vector to hold the output data
*/
extern char expectedBuf[TEST_BUFFER_SIZE];



/* ****************************************************************************
*
* toString - 
*/
extern std::string toString(int i);



/* ****************************************************************************
*
* testDataFromFile - 
*/
extern std::string testDataFromFile(char* buffer, int bufSize, const char* fileName);

#endif  // TEST_UNITTESTS_TESTDATAFROMFILE_H_
