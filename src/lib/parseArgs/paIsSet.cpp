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
#include <string.h>               /* strcmp                                    */
#include <stdio.h>                /* snprintf                                  */
#include <stdlib.h>               /* getenv                                    */

#include "parseArgs/parseArgs.h"  /* PaArgument                                */
#include "parseArgs/paConfig.h"   /* paPrefix                                  */
#include "parseArgs/paIsSet.h"    /* Own interface                             */



/* ****************************************************************************
*
* paIsSet - is an argument existing in the parse list OR defined as env-var
*/
bool paIsSet(int argC, char* argV[], PaArgument* paArgs, const char* option)
{
  int ix;

  // 1. CLI argument list
  for (ix = 1; ix < argC; ix++)
  {
    if (strcmp(argV[ix], option) == 0)
      return true;
  }

  // 2. env-var
  while (paArgs[ix].type != PaLastArg)
  {
    if (strcmp(paArgs[ix].option, option) == 0)
    {
      if (paArgs[ix].envName == NULL)
        return false;

      char  expandedEnvVar[256];
      char* envVarP = (char*) paArgs[ix].envName;

      if (paPrefix != NULL)
      {
        snprintf(expandedEnvVar, sizeof(expandedEnvVar), "%s%s", paPrefix, paArgs[ix].envName);
        envVarP = expandedEnvVar;
      }

      char* envVarValue = getenv(envVarP);

      if ((envVarValue != NULL) && (strcmp(envVarValue, "TRUE") == 0))
        return true;

      return false;
    }

    ++ix;
  }

  return false;
}



/* ****************************************************************************
*
* paIsSetSoGet - return value of option 'option'
*/
const char* paIsSetSoGet(int argC, char* argV[], const char* option)
{
  int ix;

  for (ix = 1; ix < argC; ix++)
  {
    if (strcmp(argV[ix], option) == 0)
    {
      return argV[ix + 1];
    }
  }

  return NULL;
}
