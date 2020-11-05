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
#include <stdio.h>                        /* stderr, stdout, ...             */
#include <string.h>                       /* strdup, strncmp                 */
#include <memory.h>                       /* memset                          */
#include <unistd.h>                       /* getpid                          */
#include <stdlib.h>                       /* exit                            */
#include <string>                         /* std::string                     */

#include "parseArgs/baStd.h"              /* BA standard header file         */
#include "logMsg/logMsg.h"                /* lmVerbose, lmDebug, ...         */

#include "parseArgs/paPrivate.h"          /* PaTypeUnion, config variables,  */
#include "parseArgs/paTraceLevels.h"      /* LmtPaEnvVal, ...                */
#include "parseArgs/paUsage.h"            /* paUsage, paExtendedUsage, paHelp*/
#include "parseArgs/paLog.h"              /* PA_XXX                          */
#include "parseArgs/paConfig.h"           /* paConfigActions                 */
#include "parseArgs/paFullName.h"         /* paFullName                      */
#include "parseArgs/paBuiltin.h"          /* paBuiltin                       */
#include "parseArgs/paEnvVals.h"          /* paEnvVals                       */
#include "parseArgs/paRcFile.h"           /* paRcFileParse                   */
#include "parseArgs/paWarning.h"          /* paWaringInit, paWarningAdd      */
#include "parseArgs/paIterate.h"          /* paIterateInit, paIterateNext    */
#include "parseArgs/paDefaultValues.h"    /* paDefaultValues                 */
#include "parseArgs/paOptionsParse.h"     /* paOptionsParse                  */
#include "parseArgs/paLimitCheck.h"       /* paLimitCheck                    */
#include "parseArgs/paLogSetup.h"         /* paLogSetup                      */
#include "parseArgs/paIxLookup.h"         /* paIxLookup                      */
#include "parseArgs/paOptions.h"          /* paOptionsNoOf                   */
#include "parseArgs/parseArgs.h"          /* Own interface                   */



/* ****************************************************************************
*
* variables for user of this library
*/
char*        rcFileName;               /* needed for messages (and by lmLib) */
char         paCommandLine[1024];      /* entire command line                */
char         paResultString[60000];
char*        paExtraLogSuffix = NULL;  /* To append to log file path         */



/* ****************************************************************************
*
* Global vars
*/
PaiArgument* paiList;
bool         paLogOn    = false;
int          paBuiltins = -1;



/* ****************************************************************************
*
* optionNameDuplicated -
*/
static bool optionNameDuplicated(char* name, int start)
{
  int ix;
  int opts    = paOptionsNoOf(paiList);
  int matches = 0;

  if (name == NULL)
  {
    return false;
  }

  if (name[0] == 0)
  {
    return false;
  }

  for (ix = start; ix < opts; ix++)
  {
    PaiArgument* aP;

    if ((aP = paIxLookup(paiList, ix)) == NULL)
    {
      break;
    }

    if (aP->option == NULL)
    {
      continue;
    }

    if (aP->removed == true)
    {
      continue;
    }

    if (aP->option[0] == 0)
    {
      continue;
    }

    if (strcmp(aP->option, " ") == 0)
    {
      continue;
    }

    if (strcmp(name, aP->option) == 0)
    {
      ++matches;
    }
  }

  if (matches <= 1)
  {
    return false;
  }

  return true;
}



/* ****************************************************************************
*
* envNameDuplicated -
*/
static bool envNameDuplicated(char* name, int start)
{
  int   ix;
  int   opts    = paOptionsNoOf(paiList);
  int   matches = 0;

  if (name == NULL)
  {
    return false;
  }

  if (name[0] == 0)
  {
    return false;
  }

  for (ix = start; ix < opts; ix++)
  {
    PaiArgument*  aP;
    char          envVarName[128];

    if ((aP = paIxLookup(paiList, ix)) == NULL)
    {
      break;
    }

    if (aP->removed == true)
    {
      continue;
    }

    if (aP->envName == NULL)
    {
      continue;
    }

    paEnvName(aP, envVarName, sizeof(envVarName));

    if (strcmp(name, envVarName) == 0)
    {
      ++matches;
    }
  }

  if (matches <= 1)
  {
    return false;
  }

  return true;
}



/* ****************************************************************************
*
* paArgInit -
*/
static int paArgInit(PaArgument* paList)
{
  PaiArgument* aP;
  int          parNo = 0;
  int          ix    = 0;
  int          args  = 0;

  while (paList[ix].type != PaLastArg)
  {
    ++args;
    ++ix;
  }

  paiList = (PaiArgument*) calloc(args + 1, sizeof(PaiArgument));
  if (paiList == NULL)
  {
    PA_E(("Error allocating room for %d options", args));
    exit(1);
  }

  PA_M(("Allocated room for %d args (plus one)", args));
  for (ix = 0; ix < args + 1; ix++)
  {
    PA_M(("Copying arg %d", ix));
    memcpy(&paiList[ix], &paList[ix], sizeof(paList[ix]));
    paiList[ix].isBuiltin = false;
  }

  paIterateInit();
  while ((aP = paIterateNext(paiList)) != NULL)
  {
    char envVarName[128];

    /* Set argument to be unused */
    aP->used        = 0;
    aP->hasDefault  = true;
    aP->hasMinLimit = true;
    aP->hasMaxLimit = true;
    aP->aux         = 0;

    /* Set the value to come from nowhere */
    aP->from = PafUnchanged;

    if ((aP->option != NULL) && (aP->option[0] == 0))
    {
      aP->option = NULL;
    }

    if ((aP->envName != NULL) && (aP->envName[0] == 0))
    {
      aP->envName = NULL;
    }


    if ((aP->envName != NULL) && ((aP->envName[0] == ' ') || (aP->envName[0] == '\t')))
    {
      char w[512];

      snprintf(w, sizeof(w), "%s", "found an item with var name starting with whitespace - forbidden");
      PA_WARNING(PasVarNameWs, w);
      continue;
    }

    /* Set the name of the argument */
    if (aP->option == NULL)
    {
      if (aP->envName == NULL)
      {
        snprintf(aP->name, sizeof(aP->name), "parameter %d", ++parNo);
        aP->what = PawParameter;
      }
      else
      {
        char envVarName[128];

        snprintf(aP->name, sizeof(aP->name), "variable/parameter %s", paEnvName(aP, envVarName, sizeof(envVarName)));
        aP->what = PawVariable;
      }
    }
    else if ((aP->option[0] == ' ') && (aP->option[1] == 0))
    {
      ++parNo;

      if (aP->envName == NULL)
      {
        snprintf(aP->name, sizeof(aP->name), "parameter %d", parNo);
        aP->what = PawParameter;
      }
      else
      {
        char envVarName[128];

        snprintf(aP->name, sizeof(aP->name), "parameter %s", paEnvName(aP, envVarName, sizeof(envVarName)));
        aP->what = PawVariable | PawParameter;
      }
    }
    else
    {
      if (aP->envName == NULL)
      {
        snprintf(aP->name, sizeof(aP->name), "option '%s'", aP->option);
        aP->what = PawOption;
      }
      else
      {
        snprintf(aP->name, sizeof(aP->name), "option '%s'", aP->option);
        aP->what = PawOption | PawVariable;
      }
    }

    if (optionNameDuplicated((char*) aP->option, ix) == true)
    {
      char w[512];

      snprintf(w, sizeof(w), "%s (%s) duplicated in source code", aP->name, aP->description);
      PA_WARNING(PasOptionNameDuplicated, w);
      aP->removed = true;
    }

    paEnvName(aP, envVarName, sizeof(envVarName));

    if (envNameDuplicated(envVarName, ix) == true)
    {
      char w[512];

      snprintf(w, sizeof(w), "Env var '%s' (%s) duplicated in source code", envVarName, aP->description);
      PA_WARNING(PasEnvNameDuplicated, w);
      aP->removed = true;
    }

    ++ix;
  }

  return 0;
}



/* ****************************************************************************
*
* paProgNameSet -
*/
static char* paProgNameSet(char* pn, int levels, bool pid, const char* extra = NULL)
{
  char*        start;
  static char  pName[128];

  if (pn == NULL)
  {
    return NULL;
  }

  if (levels < 1)
  {
    levels = 1;
  }

  start = &pn[strlen(pn) - 1];
  while (start > pn)
  {
    if (*start == '/')
    {
      levels--;
    }

    if (levels == 0)
    {
      break;
    }

    --start;
  }

  if (*start == '/')
  {
    ++start;
  }

  strncpy(pName, start, sizeof(pName));

  if (paUsageProgName == NULL)
    paUsageProgName = strdup(pName);

  if (pid == true)
  {
    char  pid[8];

    strncat(pName, "_", sizeof(pName) - 1);
    snprintf(pid, sizeof(pid), "%d", (int) getpid());
    strncat(pName, pid, sizeof(pName) - 1);
  }

  if (extra != NULL)
  {
    strncat(pName, extra, sizeof(pName) - 1);
  }

  return pName;
}


/* ****************************************************************************
*
* RETURN_ERROR -
*/
#define RETURN_ERROR(s)              \
do                                   \
{                                    \
  PA_WARNING(PasSetup, (char*) s);   \
  return -1;                         \
} while (0)



/* ****************************************************************************
*
* paParse - parse the argument list
*
*/
int paParse
(
  PaArgument*  paList,
  int          argC,
  char*        argV[],
  int          level,
  bool         pid,
  const char*  extra
)
{
  char*  progNameCopy;
  int    s;
  FILE*  fP;

  if (extra != NULL)
  {
    paExtraLogSuffix = strdup(extra);
  }

  memset(paResultString, 0, sizeof(paResultString));

  /* **************************************************** */
  /* Creating flat command line string paCommandLine      */
  /*                                                      */
  if (argC > 1)
  {
    int ix;

    strncpy(paCommandLine, argV[1], sizeof(paCommandLine));
    for (ix = 2; ix < argC; ix++)
    {
      if (sizeof(paCommandLine) - strlen(paCommandLine) > 1)
      {
        strncat(paCommandLine, " ", sizeof(paCommandLine) - strlen(paCommandLine) - 1);
      }
      else
      {
        break;
      }

      if (sizeof(paCommandLine) - strlen(paCommandLine) > 1)
      {
        strncat(paCommandLine, argV[ix], sizeof(paCommandLine) - strlen(paCommandLine) - 1);
      }
      else
      {
        break;
      }
    }
  }


  /* ********************************************************* */
  /* Calling paConfig just to make sure paConfigInit is called */
  /*                                                           */
  paConfig("make sure paConfigInit is called", 0);



  /* *************************** */
  /* Setting up the program name */
  /*                             */
  if (paProgName == NULL) /* hasn't been set with paConfig */
  {
    progNameCopy = strdup(argV[0]);

    progName     = strdup(paProgNameSet(argV[0], level, pid, extra));
    if (progName == NULL)
    {
      // strdup could return NULL if we run of of memory. Very unlikely, but
      // theoretically possible (and static code analysis tools complaint about it ;)
      printf("FATAL ERROR: strdup returns NULL");
      exit(1);
    }

    paProgName   = strdup(progName);
    if (paProgName == NULL)
    {
      // strdup could return NULL if we run of of memory. Very unlikely, but
      // theoretically possible (and static code analysis tools complaint about it ;)
      printf("FATAL ERROR: strdup returns NULL");
      exit(1);
    }
  }
  else
  {
    progNameCopy = strdup(paProgName);
    progName     = strdup(paProgName);
    if ((progNameCopy == NULL) || (progName == NULL))
    {
      // strdup could return NULL if we run of of memory. Very unlikely, but
      // theoretically possible (and static code analysis tools complaint about it ;)
      printf("FATAL ERROR: strdup returns NULL");
      exit(1);
    }
  }


  /* ************************************************* */
  /* The rcFileName must not have any '/' in its name. */
  /* it should be called (by default) .<progname>rc    */
  /* So, first we must get a progName without '/' ...  */
  /*                                                   */
  if (paRcFileName == NULL)
  {
    char rName[128];

    snprintf(rName, sizeof(rName), ".%src", paProgNameSet(progNameCopy, 1, false));
    paRcFileName = strdup(rName);
  }
  free(progNameCopy);


  /* ****************************************************** */
  /* Initializing all subsystems                            */
  /*                                                        */
  if ((s = paArgInit(paList)) == -1)
  {
    RETURN_ERROR("paArgInit error");
  }

  if ((s != -2) && ((s = paConfigActions(true)) == -1))
  {
    RETURN_ERROR("paConfigActions");
  }

  if ((s != -2) && ((s = paDefaultValues(paiList)) == -1))
    RETURN_ERROR("paDefaultValues");

#if 0
  // Orion doesn't use paRcFileParse ...
  if ((s != -2) && ((s = paRcFileParse()) == -1))
    RETURN_ERROR("paRcFileParse");
#endif

  if ((s != -2) && ((s = paEnvVals(paiList)) == -1))
  {
    RETURN_ERROR("paEnvVals");
  }

  if ((s != -2) && ((s = paOptionsParse(paiList, argV, argC)) == -1))
  {
    RETURN_ERROR("paOptionsParse");
  }

  if ((s != -2) && ((s = paLimitCheck(paiList)) == -1))
  {
    RETURN_ERROR("paLimitCheck");
  }

  fP = NULL;
  if (paMsgsToStdout)
  {
    fP = stdout;
  }

  if (paMsgsToStderr)
  {
    fP = stderr;
  }

  if (paResultString[0] != 0)
  {
    if (fP != NULL)
    {
      fprintf(fP, "%s\n", paResultString);
    }
  }
  else if (paWarnings > 0)
  {
    int ix;

    if (fP != NULL)
    {
      // fprintf(fP, "\nEntire command line: '%s'\n\n", paCommandLine);

      for (ix = 0; ix < paWarnings; ix++)
      {
        fprintf(fP, "%s\n\n", paWarning[ix].string);
      }

      if (paUsageOnAnyWarning)
      {
        paUsage();
      }

      if (paResultString[0] != 0)
      {
        fprintf(fP, "%s\n", paResultString);
      }

      exit(1);
    }
    else
    {
      char s[64000];

      snprintf(paResultString, sizeof(paResultString), "\nEntire command line options: '%s'\n\n", paCommandLine);

      snprintf(s, sizeof(s), "--- %s warnings ---\n", progName);
      strncat(paResultString, s, sizeof(paResultString) - 1);

      for (ix = 0; ix < paWarnings; ix++)
      {
        snprintf(s, sizeof(s), "Severity % 2d: %s\n\n", paWarning[ix].severity, paWarning[ix].string);
        strncat(paResultString, s, sizeof(paResultString) - 1);
      }

      strncat(paResultString, "\n\n", sizeof(paResultString) - 1);

      if (paUsageOnAnyWarning)
      {
        printf("paUsageOnAnyWarning == true (2)\n");
        paUsage();
      }
    }
  }

  return 0;
}



/* ****************************************************************************
*
* paCleanup - free allocated variables
*/
void paCleanup(void)
{
  free(paUsageProgName);
  free(paRcFileName);
  free(paiList);

  paUsageProgName = NULL;
  paRcFileName    = NULL;
  paiList         = NULL;
}



/* ****************************************************************************
*
* paTypeName -
*/
const char* paTypeName(PaType type)
{
  switch (type)
  {
  case PaBoolean:       return "Boolean";
  case PaString:        return "String";
  case PaShort:         return "Short";
  case PaShortU:        return "UShort";
  case PaChar:          return "Char";
  case PaUChar:         return "UChar";
  case PaInt:           return "Int";
  case PaUInt:          return "UInt";
  case PaLong:          return "Long";
  case PaULong:         return "ULong";
  case PaFloat:         return "Float";
  case PaDouble:        return "Double";
  case PaIList:         return "IList";
  case PaSList:         return "SList";
  case PaLastArg:       return "Not A Type";
  }

  return "Unknown Type";
}
