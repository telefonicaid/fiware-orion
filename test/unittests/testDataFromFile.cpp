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
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>      // needed for read function in Debian 7
#include <string.h>

#include <string>

#include "unittests/testDataFromFile.h"



/* ****************************************************************************
*
* testBuf - writable char vector to hold the input data (from TEXT segment)
*/
char testBuf[TEST_BUFFER_SIZE];



/* ****************************************************************************
*
* expectedBuf - writable char vector to hold the output data
*/
char expectedBuf[TEST_BUFFER_SIZE];



/* ****************************************************************************
*
* toString - 
*/
std::string toString(int i)
{
  char a[32];

  snprintf(a, sizeof(a), "%d", i);

  return std::string(a);
}



/* ****************************************************************************
*
* testDataFromFile - 
*/
std::string testDataFromFile(char* buffer, int bufSize, const char* fileName)
{
  char         path[512];
  int          fd;
  int          nb;
  struct stat  sBuf;

  snprintf(path, sizeof(path), "test/unittests/testData/%s", fileName);

  if (stat(path, &sBuf) == -1)
    return std::string("stat(") + path + "): " + strerror(errno);

  if (sBuf.st_size > bufSize)
  {
    std::string error;

    error  = std::string("buffer too small (");
    error += toString(bufSize) + " bytes): ";
    error += toString(sBuf.st_size);
    error += " bytes needed to hold the content of ";
    error += path;

    return error;
  }

  if ((fd = open(path, O_RDONLY)) == -1)
    return std::string("open(") + path + "): " + strerror(errno);

  nb = read(fd, buffer, sBuf.st_size);
  close(fd);
  if (nb == -1)
    return std::string("read(") + path + "): " + strerror(errno);

  if (nb != sBuf.st_size)
    return std::string("bad size read from ") + path;

  buffer[nb] = 0;

  return "OK";
}
