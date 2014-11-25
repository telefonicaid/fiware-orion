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
#include <string.h>                   /* strncmp                             */
#include <string>                     /* std::string                         */
#include <cstdlib>                    /* C++ free                            */

#include "parseArgs/baStd.h"          /* BA standard header file             */
#include "logMsg/logMsg.h"            /* LM_T                                */

#include "parseArgs/parseArgs.h"      /* PaArgument, ...                     */
#include "parseArgs/paTraceLevels.h"  /* LmtXXX                              */
#include "parseArgs/paBuiltin.h"      /* paBuiltin, paBuiltinNoOf            */
#include "parseArgs/paIterate.h"      /* paIterateInit, paIterateNext        */
#include "parseArgs/paIsOption.h"     /* Own interface                       */



/* ****************************************************************************
*
* paIsOption - is the string 'string' an option?
*/
bool paIsOption(PaiArgument* paList, char* string)
{
  int           len;
  PaiArgument*  aP;

  LM_ENTRY();
  paIterateInit();
  while ((aP = paIterateNext(paList)) != NULL)
  {
    if ((aP->option == NULL) || (aP->option[0] == 0))
    {
      continue;
    }

    len = MAX(strlen(aP->option), strlen(string));

    if (strncmp(aP->option, string, len) == 0)
    {
      LM_EXIT();
      return true;
    }
  }

  LM_EXIT();
  return false;
}
