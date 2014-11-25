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
* Author: Ken Zangelin
*/

/* ****************************************************************************
*
* Compilation:
*  g++  -g    lmTest.cpp   -o lmTest -I ../ -L ../../../BUILD_DEBUG/src/lib/parseArgs/ -lpa -L ../../../BUILD_DEBUG/src/lib/logMsg/ -llm -lpthread

*/
#include <pthread.h>

#include "parseArgs/parseArgs.h"
#include "parseArgs/paConfig.h"
#include "logMsg/logMsg.h"



/* ****************************************************************************
*
* paArgs - 
*/
PaArgument paArgs[] =
{
    PA_END_OF_ARGS
};



/* ****************************************************************************
*
* child - 
*/
void* child(void* vP)
{
  int childNo = *((int*) vP);

  for (int ix = 0; ix < 1000; ix++)
    LM_M(("Child %d: M%d", childNo, ix));

  return (void*) 0;
}



/* ****************************************************************************
*
* main - 
*/
int main(int argC, char* argV[])
{
  paConfig("prefix", "LT_");

  paConfig("log to file", "/tmp/");
  paConfig("log file line format", "DEF");
  paConfig("log file time format", "DEF");
  paConfig("usage and exit on any warning", (void*) 1);

  paConfig("log to screen", "only errors");
  paConfig("screen line format", "TYPE:FUNC: TEXT");

  paParse(paArgs, argC, argV, 1, 0);

  pthread_t pid[10];
  for (int cIx = 0; cIx < 10; ++cIx)
    pthread_create(&pid[cIx], NULL, child, &cIx);

  for (int cIx = 0; cIx < 10; ++cIx)
    pthread_join(pid[cIx], NULL);

  //
  // FIXME P4: This test program should be a unit test
  //
  // Now, lets see if we have 10000 lines in the logfile
  // Actually, 4 lines is the header, so the total should be 1004 lines
  //
  int lines = lmLogLinesGet();

  if (lines == 10004)
    printf("OK\n");
  else
    printf("Error - log file has %d lines. Should have 1004\n", lines);

  return 0;
}
