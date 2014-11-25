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
#include <string.h>                   /* strcmp                              */

#include "parseArgs/parseArgs.h"      /* PaFrom, PaArgument                  */
#include "parseArgs/paPrivate.h"      /* paiList                             */
#include "parseArgs/paIterate.h"      /* paIterateInit, paIterateNext        */
#include "parseArgs/paEnvVals.h"      /* paEnvName                           */
#include "parseArgs/paValueFrom.h"    /* Own interface                       */



/* ****************************************************************************
*
* paValueFrom -
*/
PaFrom paValueFrom(char* oName)
{
  PaiArgument* aP;

  paIterateInit();
  while ((aP = paIterateNext(paiList)) != NULL)
  {
    char envVarName[64];

    if (aP->option == NULL)
    {
      continue;
    }

    paEnvName(aP, envVarName, sizeof(envVarName));

    if (aP->option && (strcmp(oName, aP->option) == 0))
    {
      return aP->from;
    }
    else if (aP->envName && (strcmp(oName, envVarName) == 0))
    {
      return aP->from;
    }
  }

  return PafError;
}
