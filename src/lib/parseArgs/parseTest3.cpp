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
* Author: developer
*/
#include "logMsg/logMsg.h"
#include "parseArgs/parseArgs.h"



/* ****************************************************************************
*
* main -
*/
int main(int argC, char* argV[])
{
  paConfig("trace levels", "0-255");
  paConfig("help file",     "/users/kzangeli/systems/parseArgs/parseTest3.help");
  paConfig("log to file",   (void*) TRUE);

  paParse(NULL, argC, argV, 1, FALSE);

  lmVerbose = true;
  LM_V(("line with newlines ...\n 1\n2\n3\n4\n"));

  return 0;
}
