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
#include "parseArgs/baStd.h"      /* BA standard header file                 */
#include "logMsg/logMsg.h"        /* LM_ENTRY, LM_EXIT, ...                  */

#include "parseArgs/parseArgs.h"  /* PaArgument                              */
#include "parseArgs/paBuiltin.h"  /* paBuiltin, paBuiltinNoOf                */
#include "parseArgs/paLog.h"      /* PA_M, ...                               */
#include "parseArgs/paOptions.h"  /* Own interface                           */



/* ****************************************************************************
*
* paOptionsNoOf - 
*/
int paOptionsNoOf(PaiArgument* paList)
{
  int ix   = 0;
  int opts = 0;

  if (paList == NULL)
  {
    return paBuiltinNoOf();
  }

  PA_M(("Counting args"));

  while (paList[ix].type != PaLastArg)
  {
    PA_M(("arg %d: '%s'", ix, paList[ix].option));
    if (paList[ix].removed != true)
    {
      ++opts;
    }
    ++ix;
    PA_M(("%d args so far", opts));
  }

  return opts + paBuiltinNoOf();
}
