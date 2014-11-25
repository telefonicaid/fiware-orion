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
#include <stdio.h>                    /* stderr, stdout, ...                 */
#include <cstdlib>                    /* C++ free(.)                         */

#include "parseArgs/baStd.h"          /* BA standard header file             */
#include "logMsg/logMsg.h"            /* lmVerbose, lmDebug, ...             */

#include "parseArgs/paPrivate.h"      /* PaTypeUnion, config variables, ...  */
#include "parseArgs/paBuiltin.h"      /* paLogDir                            */
#include "parseArgs/paTraceLevels.h"  /* LmtPaEnvVal, ...                    */
#include "parseArgs/paConfig.h"       /* paConfigActions                     */
#include "parseArgs/paWarning.h"      /* paWaringInit, paWarningAdd          */
#include "parseArgs/paLogSetup.h"     /* Own interface                       */



/* ****************************************************************************
*
*
*/
int  lmFd   = -1;
int  lmSd   = -1;



/* ****************************************************************************
*
* paLmFdGet -
*/
int paLmFdGet(void)
{
  return lmFd;
}



/* ****************************************************************************
*
* paLmSdGet
*/
int paLmSdGet(void)
{
  return lmSd;
}



/* ****************************************************************************
*
* paLogSetup - 
*/
extern char* paExtraLogSuffix;
int paLogSetup(void)
{
  LmStatus    s = LmsOk;
  char        w[512];

  if (paLogToFile == true)
  {
    // printf("paLogDir == '%s'\n", paLogDir);
    if (paLogDir[0] != 0)
    {
      // printf("Using paLogDir '%s'", paLogDir);
      s = lmPathRegister(paLogDir, paLogFileLineFormat, paLogFileTimeFormat, &lmFd, paLogAppend);
    }
    else
    {
      // printf("Using paLogFilePath: '%s'\n", paLogFilePath);
      s = lmPathRegister(paLogFilePath, paLogFileLineFormat, paLogFileTimeFormat, &lmFd, paLogAppend);
    }

    if (s != LmsOk)
    {
      snprintf(w, sizeof(w), "lmPathRegister: %s", lmStrerror(s));
      PA_WARNING(PasLogFile, w);
      return -2;
    }
  }

  if (paLogToScreen)
  {
    int fd = 1;

    if (paLogScreenToStderr)
    {
      fd = 2;
    }

    s = lmFdRegister(fd, paLogScreenLineFormat, paLogScreenTimeFormat, "stdout", &lmSd);
    if (s != LmsOk)
    {
      snprintf(w, sizeof(w), "lmFdRegister: %s", lmStrerror(s));
      PA_WARNING(PasLogFile, w);
      return -3;
    }
  }

  if (paLogToFile || paLogToScreen || lmNoTracesToFileIfHookActive)
  {
    if ((s = lmInit()) != LmsOk)
    {
      snprintf(w, sizeof(w), "lmInit: %s", lmStrerror(s));
      PA_WARNING(PasLogFile, w);
      return -4;
    }

    lmToDo     = false;
    lmVerbose  = false;
    lmVerbose2 = false;
    lmVerbose3 = false;
    lmVerbose4 = false;
    lmVerbose5 = false;
    lmDebug    = false;
    lmReads    = false;
    lmWrites   = false;
    lmSilent   = false;
    /* lmBug     = false; */

    lmTraceSet((char*) "");

    if (paLogToScreen && paLogScreenOnlyErrors)
    {
      lmOnlyErrors(lmSd);
    }
  }

  return 0;
}
