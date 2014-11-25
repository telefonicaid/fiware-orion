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
#include "parseArgs/paLog.h"      /* PA_XXX                                  */
#include "parseArgs/paBuiltin.h"  /* paBuiltin, paBuiltinNoOf                */
#include "parseArgs/paPrivate.h"  /* PawBuiltin                              */
#include "parseArgs/paIterate.h"  /* Own interface                           */



/* ****************************************************************************
*
* static variables
*/
static int  ix       = 0;
int         builtins = -1;



/* ****************************************************************************
*
* paIterateInit - 
*/
void paIterateInit(void)
{
  PA_M(("Preparing to iterate"));
  ix       = 0;
  builtins = paBuiltinNoOf();
}



/* ****************************************************************************
*
* paIterateNext - 
*/
PaiArgument* paIterateNext(PaiArgument* paList)
{
  PaiArgument* aP;

  if (builtins == -1)
  {
    paIterateInit();
  }

  PA_M(("builtins == %d (ix == %d)", builtins, ix));
  do
  {
    if (ix < builtins)
    {
      aP = &paBuiltin[ix];
      aP->what |= PawBuiltin;
      PA_M(("Found builtin '%s'", aP->option));
    }
    else if (paList != NULL)
    {
      aP = &paList[ix - builtins];
    }
    else
    {
      return NULL;
    }

    ++ix;
  } while (aP->removed == true);

  if (aP->type == PaLastArg)
  {
    return NULL;
  }

  return aP;
}
