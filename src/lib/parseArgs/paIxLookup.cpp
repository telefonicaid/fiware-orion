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
#include "parseArgs/baStd.h"       /* BA standard header file                */
#include "logMsg/logMsg.h"         /* LM_ENTRY, LM_EXIT, ...                 */

#include "parseArgs/parseArgs.h"   /* PaArgument                             */
#include "parseArgs/paBuiltin.h"   /* paBuiltin, paBuiltinNoOf               */
#include "parseArgs/paOptions.h"   /* paOptionsNoOf                          */
#include "parseArgs/paIxLookup.h"  /* Own interface                          */



/* ****************************************************************************
*
* paIxLookup - 
*/
PaiArgument* paIxLookup(PaiArgument* paList, int ix)
{
  int builtins = paBuiltinNoOf();

  if (ix < builtins)
  {
    return &paBuiltin[ix];
  }
  else if (ix < paOptionsNoOf(paList))
  {
    return &paList[ix - builtins];
  }

  return NULL;
}
