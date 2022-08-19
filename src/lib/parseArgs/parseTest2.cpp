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
#include <stdio.h>
#include <string>

#include "logMsg/logMsg.h"

#include "parseArgs/parseArgs.h"



/* ****************************************************************************
*
* main -
*/
int main(int argC, char* argV[])
{
  paConfig("prefix", "P2_");
  paConfig("log to file", "/tmp/");
  paConfig("msgs to stdout", (void*) FALSE);
  paParse(NULL, argC, argV, 1, FALSE);

  LM_V(("verbose message"));
  if (paResultString[0] != 0)
  {
    printf("Got a paResultString:\n%s", paResultString);
  }
  else if (paWarnings != 0)
  {
    int ix;

    printf("Got warnings:\n");
    for (ix = 0; ix < paWarnings; ix++)
    {
      printf("Severity %02d: %s\n", paWarning[ix].severity, paWarning[ix].string);
    }
    printf("\n");
  }

  return 0;
}
