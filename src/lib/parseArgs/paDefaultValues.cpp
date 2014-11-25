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
#include <stdio.h>                      /* stderr, stdout, ...               */
#include <string.h>                     /* strncmp, strspn, ...              */
#include <cstdlib>                      /* C++ free()                        */

#include "parseArgs/baStd.h"            /* BA standard header file           */
#include "logMsg/logMsg.h"              /* LM_ENTRY, LM_EXIT, ...            */

#include "parseArgs/parseArgs.h"        /* PaArgument                        */
#include "parseArgs/paPrivate.h"        /* paBuiltin                         */
#include "parseArgs/paTraceLevels.h"    /* LmtPaDefVal, LmtPaLimits, ...     */
#include "parseArgs/paWarning.h"        /* paWaringInit, paWarningAdd        */
#include "parseArgs/paIterate.h"        /* paIterateInit, paIterateNext      */
#include "parseArgs/paDefaultValues.h"  /* Own interface                     */



/* ****************************************************************************
*
* paDefaultValues - set default values, initially
*/
int paDefaultValues(PaiArgument* paList)
{
  PaiArgument*  aP;
  char          w[512];

  LM_ENTRY();

  paIterateInit();
  while ((aP = paIterateNext(paList)) != NULL)
  {
    int64_t*  defP;

    if (aP->def == PaNoDef)
    {
      continue;
    }

    aP->from = PafDefault;
    if (aP->type != PaString)
    {
      LM_T(LmtPaDefVal, ("setting default value for '%s' (0x%x)", aP->name, (int) aP->def));
    }
    else
    {
      LM_T(LmtPaDefVal, ("setting default value for '%s' (%s)", aP->name, (char*) aP->def));
    }

    defP = (int64_t*) &aP->def;

    switch (aP->type)
    {
    case PaInt:     *((int*)       aP->varP) = *defP;                    break;
    case PaIntU:    *((int*)       aP->varP) = *defP;                    break;
    case PaInt64:   *((int64_t*)   aP->varP) = *defP;                    break;
    case PaIntU64:  *((uint64_t*)  aP->varP) = *defP;                    break;
    case PaChar:    *((char*)      aP->varP) = (char)      *defP;        break;
    case PaCharU:   *((char*)      aP->varP) = (char)      *defP;        break;
    case PaShort:   *((int16_t*)   aP->varP) = (int16_t)   *defP;        break;
    case PaShortU:  *((uint16_t*)  aP->varP) = (uint16_t)  *defP;        break;
    case PaBoolean: *((bool*)      aP->varP) = (bool)      *defP;        break;
    case PaFloat:   *((float*)     aP->varP) = (float)     *defP;        break;
    case PaDouble:  *((double*)    aP->varP) = (double)    *defP;        break;

    case PaString:
      if (aP->def)
      {
        if (((char*) aP->def)[0] != 0)
        {
          if ((char*) aP->varP != (char*) aP->def)
            strcpy((char*) aP->varP, (char*) aP->def);
        }
      }
      else
      {
        ((char*) aP->varP)[0] = 0;
      }
      break;

    default:
      snprintf(w, sizeof(w), "type %d unknown for %s", aP->type, aP->name);
      PA_WARNING(PasProgrammingError, w);
      continue;
    }

    if (aP->type != PaString)
    {
      LM_T(LmtPaDefVal, ("default value for '%s' is set", aP->name));
    }
    else
    {
      LM_T(LmtPaDefVal, ("default value for '%s' is set to '%s'", aP->name, (char*) aP->varP));
    }
  }

  LM_EXIT();
  return 0;
}
