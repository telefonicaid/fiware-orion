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
#include <stdlib.h>                   /* free, ...                           */
#include <string.h>                   /* strdup, ...                         */
#include <string>                     /* std::string                         */

#include "parseArgs/baStd.h"          /* BA_VEC_SIZE, ...                    */
#include "logMsg/logMsg.h"            /* lmTraceSet                          */

#include "parseArgs/paPrivate.h"      /* PaTypeUnion, config variables, ...  */
#include "parseArgs/paTraceLevels.h"  /* LmtPaDefaultValues, ...             */
#include "parseArgs/parseArgs.h"      /* paWarnings, paWarning               */
#include "parseArgs/paWarning.h"      /* Own interface                       */



/* ****************************************************************************
*
* paWarning -
*/
PaWarning  paWarning[100];
int        paWarnings = 0;



/* ****************************************************************************
*
* paWarningInit -
*/
void paWarningInit(void)
{
  int ix;

  for (ix = 0; ix < (int) BA_VEC_SIZE(paWarning); ix++)
  {
    paWarning[ix].string = NULL;
    paWarning[ix].severity = PasNone;
  }

  paWarnings = 0;
}



/* ****************************************************************************
*
* paWarningAdd -
*/
void paWarningAdd(PaSeverity severity, char* txt)
{
  static int ix = 0;

  if (ix >= (int) BA_VEC_SIZE(paWarning))
  {
    ix = 0;
  }

  if (paWarning[ix].string != NULL)
  {
    free(paWarning[ix].string);
  }

  paWarning[ix].string   = strdup(txt);
  paWarning[ix].severity = severity;

  // LM_W((paWarning[ix].string));
  ++ix;

  ++paWarnings;
}
