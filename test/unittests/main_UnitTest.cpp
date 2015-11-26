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
#include <stdio.h>
#include <sys/types.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "parseArgs/parseArgs.h"
#include "parseArgs/paBuiltin.h"        // paLsHost, paLsPort
#include "parseArgs/paIsSet.h"

#include "logMsg/logMsg.h"

#undef _i
#include "common/globals.h"
#include "common/sem.h"
#include "mongoBackend/MongoGlobal.h"
#include "ngsiNotify/Notifier.h"

#include "unittest.h"



/* ****************************************************************************
*
* parse arguments
*/
PaArgument paArgs[] =
{
  PA_END_OF_ARGS
};



/* ****************************************************************************
*
* global variables
*/
bool          harakiri              = true;
int           logFd                 = -1;
int           fwdPort               = -1;
int           subCacheInterval      = 10;
unsigned int  cprForwardLimit       = 1000;
bool          noCache               = false;
char          fwdHost[64];
char          notificationMode[64];
bool          simulatedNotification;


/* ****************************************************************************
*
* exitFunction - 
*/
void exitFunction(int code, const std::string& reason)
{
  LM_E(("Orion library asks to exit %d: '%s', but no exit is allowed inside unit tests", code, reason.c_str()));
}


const char* orionUnitTestVersion = "0.0.1-unittest";



/* ****************************************************************************
*
* main - 
*/
int main(int argC, char** argV)
{
  paConfig("usage and exit on any warning", (void*) true);
  paConfig("log to screen",                 (void*) "only errors");
  paConfig("log file line format",          (void*) "TYPE:DATE:EXEC-AUX/FILE[LINE](p.PID)(t.TID) FUNC: TEXT");
  paConfig("screen line format",            (void*) "TYPE@TIME  EXEC: TEXT");
  paConfig("log to file",                   (void*) true);
  paConfig("default value", "-logDir",      (void*) "/tmp");
  paConfig("man author",                    "Fermín Galán and Ken Zangelin");

  if (argC > 1)
  {
     if (strcmp(argV[1], "-t") == 0)
       paParse(paArgs, 3, argV, 3, false);
     else
       paParse(paArgs, 1, argV, 1, false);
  }
  else
    paParse(paArgs, 1, argV, 1, false);

  LM_M(("Init tests"));
  orionInit(exitFunction, orionUnitTestVersion, SemReadWriteOp, false, false, false, false);
  setupDatabase();

  LM_M(("Run all tests"));
  ::testing::InitGoogleMock(&argC, argV);
  return RUN_ALL_TESTS();
}
