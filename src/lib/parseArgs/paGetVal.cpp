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
#include <stdio.h>                /* stderr, stdout, ...                     */
#include <stdlib.h>               /* strtol, atoi                            */
#include <string>                 /* std::string                             */

#include "parseArgs/baStd.h"      /* BA standard header file                 */
#include "logMsg/logMsg.h"        /* lmVerbose, lmDebug, ...                 */

#include "parseArgs/parseArgs.h"  /* progName                                */
#include "parseArgs/paWarning.h"  /* paWarningAdd                            */
#include "parseArgs/paGetVal.h"   /* Own interface                           */



/* ****************************************************************************
*
* paGetVal - calculate the integer value of a string
*/
void* paGetVal(char* string, int* error)
{
  int64_t   value;
  int       type;
  char      errorText[256];

  errorText[0] = 0;

  *error = PaOk;

  value = baStoi(string, &type, errorText);
  if (errorText[0] != 0)
  {
    PA_WARNING(PasBadValue, errorText);
    *error = type;
    return NULL;
  }

  return (void*) value;
}
