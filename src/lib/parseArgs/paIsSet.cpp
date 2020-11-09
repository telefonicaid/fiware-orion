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

#include "parseArgs/parseArgs.h"  /* PaArgument, ...                           */
#include "parseArgs/paConfig.h"   /* paPrefix                                  */
#include "parseArgs/paIsSet.h"    /* Own interface                             */



/* *****************************************************************************
*
* getEnvVarValue - get value of the env var with prefix taken into account
*/
static const char* getEnvVarValue(const char* envVarName)
{
  char prefixedEnvVarName[256];

  snprintf(prefixedEnvVarName, sizeof(prefixedEnvVarName), "%s%s", paPrefix, envVarName);

  return getenv(prefixedEnvVarName);
}



/* *****************************************************************************
*
* getCorrespondingEnvVar - get the name of the env var that corresponds with
* the given option or null if there is none
*/
static const char* getCorrespondingEnvVar(PaArgument* pArguments, const char* option)
{
  int count = 0;

  while (pArguments[count].type != PaLastArg)
  {
    if (strncmp(pArguments[count].option, option, strlen(option)) == 0)
    {
      return pArguments[count].envName;
    }
    ++count;
  }

  return NULL;
}



/* ****************************************************************************
*
* paIsSet - is an argument existing in the parse list or is it set to true as
* an env var
*/
bool paIsSet(int argC, char* argV[], PaArgument* pArguments, const char* option)
{
  int i;

  for (i = 1; i < argC; i++)
  {
    if (strcmp(argV[i], option) == 0)
    {
      return true;
    }
  }

  const char* envVar = getCorrespondingEnvVar(pArguments, option);

  if (envVar == NULL)
  {
    return false;
  }
  else
  {
    const char* envVarValue = getEnvVarValue(envVar);

    if (envVarValue == NULL)
      return false;

    //
    // Equivalent to a command-line option being set is, that the var is set to
    // TRUE therefore we only return true in that cases.
    //
    return strcmp(envVarValue, "TRUE") == 0;
  }
}



/* ****************************************************************************
*
* paIsSetSoGet - return value of option 'option'
*/
const char* paIsSetSoGet(int argC, char* argV[], PaArgument* pArguments, const char* option)
{
  int i;

  for (i = 1; i < argC; i++)
  {
    if (strcmp(argV[i], option) == 0)
    {
      return argV[i + 1];
    }
  }

  const char* envVar = getCorrespondingEnvVar(pArguments, option);

  if (envVar == NULL)
  {
    return NULL;
  }

  return getEnvVarValue(envVar);
}
