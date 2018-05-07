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
#include <stdlib.h>                  /* free, ...                            */
#include <string.h>                  /* strdup, ...                          */

#include "logMsg/logMsg.h"           /* lmTraceSet                           */

#include "parseArgs/paPrivate.h"     /* PaTypeUnion, config variables, ...   */
#include "parseArgs/paTraceLevels.h" /* LmtPaDefaultValues, ...              */
#include "parseArgs/paLog.h"         /* log macros to debug paConfig         */
#include "parseArgs/paWarning.h"     /* paWaringInit, paWarningAdd           */
#include "parseArgs/paBuiltin.h"     /* paBuiltinRemove                      */
#include "parseArgs/paParse.h"       /* paTypeName                           */
#include "parseArgs/paConfig.h"      /* Own interface                        */



/* ****************************************************************************
*
* lmlib variables
* These variables are non-static in logMsg.cpp but not declared 'extern' in logMsg.cpp
* Thus, global variables in lmlib, but only meant for libpa. 
*/
extern bool lmPreamble;



/* ****************************************************************************
*
* Configurable variables
*
* PROBLEM
* What do I do with default values for strings?
* if (p != NULL) free(p) ... 
*
* I might create a defaultVariable for each of the strings
*/
bool       paUseBuiltins         = true;
bool       paExitOnError         = true;
bool       paExitOnUsage         = true;
bool       paPrintErrorsOnStderr = true;
char*      paPrefix              = NULL;
char*      paBuiltinPrefix       = NULL;
char*      paVersionString       = NULL;
char*      paExecVersion         = NULL;
char*      paTraceInfoAtEol      = NULL;
char*      paProgName            = NULL;
char*      paRcFileName          = NULL;
char*      paRcFileDir           = NULL;
char*      paGenericRcDir        = NULL;

/* lmLib configuration variables */
bool       paLogToFile           = false;
bool       paLogToScreen         = false;
bool       paLogScreenToStderr   = false;
bool       paLogScreenOnlyErrors = false;
bool       paBoolWithValueIsUnrecognized = false;

char*      paLogFilePath         = NULL;
char*      paLogFileLineFormat   = NULL;
char*      paLogFileTimeFormat   = NULL;
char*      paLogScreenLineFormat = NULL;
char*      paLogScreenTimeFormat = NULL;

bool       paLogClearing         = false;
bool       paUsageOnAnyWarning   = false;
char*      paHelpFile            = NULL;
char*      paHelpText            = NULL;

char*      paManSynopsis         = NULL;
char*      paManShortDescription = NULL;
char*      paManDescription      = NULL;
char*      paManExitStatus       = NULL;
char*      paManReportingBugs    = NULL;

char*      paManCopyright        = NULL;
char*      paManVersion          = NULL;
char*      paManAuthor           = NULL;


/* Debug setting variables */
char       paTraceV[1024];
char*      paTracelevels         = NULL;

bool       paVerbose             = false;
bool       paVerbose2            = false;
bool       paVerbose3            = false;
bool       paVerbose4            = false;
bool       paVerbose5            = false;
bool       paDebug               = false;
bool       paToDo                = false;
bool       paReads               = false;
bool       paWrites              = false;
bool       paFix                 = false;
bool       paBug                 = false;
bool       paBuf                 = false;
bool       paDoubt               = false;

bool       paSilent              = false;
bool       paMsgsToStdout        = true;
bool       paMsgsToStderr        = false;
char       paPid[16];
bool       paNoTracesToFileIfHookActive = false;
char**     paValidLogLevels             = NULL;



/* ****************************************************************************
*
* paConfigCleanup - 
*/
void paConfigCleanup(void)
{
  PA_M(("Cleaning up"));
  // printf("Cleaning up parseArgs library\n");

  PA_M(("freeing paPrefix"));

  if (paUsageProgName != NULL)
  {
    free(paUsageProgName);
    paUsageProgName = NULL;
  }

  if (paPrefix)
  {
    free(paPrefix);
    paPrefix = NULL;
  }

  if (paBuiltinPrefix)
  {
    free(paBuiltinPrefix);
    paBuiltinPrefix = NULL;
  }

  if (paRcFileName != NULL)
  {
    free(paRcFileName);
    paRcFileName = NULL;
  }

  if (paRcFileDir)
  {
    free(paRcFileDir);
    paRcFileDir = NULL;
  }

  if (paGenericRcDir)
  {
    free(paGenericRcDir);
    paGenericRcDir = NULL;
  }

  if (paVersionString)
  {
    free(paVersionString);
    paVersionString = NULL;
  }

  if (paExecVersion)
  {
    free(paExecVersion);
    paExecVersion = NULL;
  }

  if (paHelpFile)
  {
    free(paHelpFile);
    paHelpFile = NULL;
  }

  if (paHelpText)
  {
    free(paHelpText);
    paHelpText = NULL;
  }

  if (paTracelevels)
  {
    free(paTracelevels);
    paTracelevels = NULL;
  }

  if (paTraceInfoAtEol != NULL)
  {
    free(paTraceInfoAtEol);
    paTraceInfoAtEol = NULL;
  }

  if (paLogFilePath != NULL)
  {
    free(paLogFilePath);
    paLogFilePath = NULL;
  }

  if (paLogFileLineFormat != NULL)
  {
    free(paLogFileLineFormat);
    paLogFileLineFormat = NULL;
  }

  if (paLogFileTimeFormat != NULL)
  {
    free(paLogFileTimeFormat);
    paLogFileTimeFormat = NULL;
  }

  if (paLogScreenLineFormat != NULL)
  {
    free(paLogScreenLineFormat);
    paLogScreenLineFormat = NULL;
  }

  if (paLogScreenTimeFormat != NULL)
  {
    free(paLogScreenTimeFormat);
    paLogScreenTimeFormat = NULL;
  }

  if (paManSynopsis)
  {
    free(paManSynopsis);
    paManSynopsis = NULL;
  }

  if (paManShortDescription)
  {
    free(paManShortDescription);
    paManShortDescription = NULL;
  }

  if (paManDescription)
  {
    free(paManDescription);
    paManDescription = NULL;
  }

  if (paManExitStatus)
  {
    // LM_T(LmtParse,("Freeing paManExitStatus"));
    PA_M(("Freeing paManExitStatus"));
    free(paManExitStatus);
    paManExitStatus = NULL;
  }

  if (paManAuthor)
  {
    // LM_T(LmtParse,("Freeing paManAuthor"));
    PA_M(("Freeing paManAuthor"));
    free(paManAuthor);
    paManAuthor = NULL;
  }

  if (paManReportingBugs)
  {
    free(paManReportingBugs);
    paManReportingBugs = NULL;
  }

  PA_M(("freeing paManCopyright"));
  if (paManCopyright)
  {
    free(paManCopyright);
    paManCopyright = NULL;
  }

  PA_M(("freeing paManVersion"));
  if (paManVersion)
  {
    free(paManVersion);
    paManVersion = NULL;
  }

  PA_M(("freeing paiList"));
  if (paiList != NULL)
  {
    PA_M(("freeing paiList"));
    free(paiList);
    PA_M(("freeing paiList"));
    paiList = NULL;
    PA_M(("freed paiList II"));
  }

  PA_M(("freeing progName"));
  if (progName != NULL)
  {
    free(progName);
    progName = NULL;
  }

  PA_M(("freeing paProgName"));
  if (paProgName != NULL)
  {
    free(paProgName);
    paProgName = NULL;
  }
}



/* ****************************************************************************
*
* paConfigInit - 
*/
static void paConfigInit(void)
{
  paWarningInit();

  if (paPrefix)
  {
    paPrefix = strdup(paPrefix);
  }

  if (paBuiltinPrefix)
  {
    paBuiltinPrefix = strdup(paBuiltinPrefix);
  }

  if (paProgName)
  {
    paProgName = strdup(paProgName);
  }

  if (paRcFileName)
  {
    paRcFileName = strdup(paRcFileName);
  }

  if (paRcFileDir)
  {
    paRcFileDir = strdup(paRcFileDir);
  }

  if (paGenericRcDir)
  {
    paGenericRcDir = strdup(paGenericRcDir);
  }

  if (paVersionString)
  {
    paVersionString = strdup(paVersionString);
  }

  if (paExecVersion)
  {
    paExecVersion = strdup(paExecVersion);
  }

  if (paHelpFile)
  {
    paHelpFile = strdup(paHelpFile);
  }

  if (paHelpText)
  {
    paHelpText = strdup(paHelpText);
  }

  if (paTracelevels)
  {
    paTracelevels = strdup(paTracelevels);
  }


  if (paManSynopsis)
  {
    paManSynopsis = strdup(paManSynopsis);
  }

  if (paManShortDescription)
  {
    paManShortDescription = strdup(paManShortDescription);
  }

  if (paManDescription)
  {
    paManDescription = strdup(paManDescription);
  }

  if (paManExitStatus)
  {
    paManExitStatus = strdup(paManExitStatus);
  }

  if (paManAuthor)
  {
    paManAuthor = strdup(paManAuthor);
  }

  if (paManReportingBugs)
  {
    paManReportingBugs = strdup(paManReportingBugs);
  }

  paManCopyright        = (paManCopyright)?        strdup(paManCopyright)        : strdup(DEFAULT_COPYRIGHT);
  paManVersion          = (paManVersion)?          strdup(paManVersion)          : strdup(DEFAULT_VERSION);
  paTraceInfoAtEol      = (paTraceInfoAtEol)?      strdup(paTraceInfoAtEol)      : strdup("#");
  paLogFilePath         = (paLogFilePath)?         strdup(paLogFilePath)         : strdup("/tmp/");
  paLogFileLineFormat   = (paLogFileLineFormat)?   strdup(paLogFileLineFormat)   : strdup("DEF");
  paLogFileTimeFormat   = (paLogFileTimeFormat)?   strdup(paLogFileTimeFormat)   : strdup("DEF");
  paLogScreenLineFormat = (paLogScreenLineFormat)? strdup(paLogScreenLineFormat) : strdup("DEF");
  paLogScreenTimeFormat = (paLogScreenTimeFormat)? strdup(paLogScreenTimeFormat) : strdup("DEF");

  if (paLogDir[0] == 0)
  {
    snprintf(paLogDir, sizeof(paLogDir), "%s", "/tmp/");
  }


  /* Should all these be freed after paParse finishes? */
  /* YES ! */
}



/* ****************************************************************************
*
* validLogLevelCheck - 
*/
static bool validLogLevelCheck(char** paValidLogLevels, char* value)
{
  int ix = 0;

  while (paValidLogLevels[ix] != NULL)
  {
    if (strcasecmp(value, paValidLogLevels[ix]) == 0)
    {
      return true;
    }

    ++ix;
  }

  return false;
}



/* ****************************************************************************
*
* paConfig - 
*/
int paConfig(const char* item, const void* value, const void* value2)
{
  static int  firstTime = 0;
  int64_t     val       = (int64_t) value;

  PA_M(("setting value for item '%s'", item));

  if (firstTime == 0)
  {
    paConfigInit();
  }

  firstTime = 1;

  if (strcmp(item, "help file") == 0)
  {
    paHelpFile = strdup((char*) val);
  }
  else if (strcmp(item, "help text") == 0)
  {
    paHelpText = strdup((char*) val);
  }
  else if (strcmp(item, "man synopsis") == 0)
  {
    paManSynopsis = strdup((char*) val);
  }
  else if (strcmp(item, "man shortdescription") == 0)
  {
    paManShortDescription = strdup((char*) val);
  }
  else if (strcmp(item, "man description") == 0)
  {
     paManDescription = strdup((char*) val);
  }
  else if (strcmp(item, "man exitstatus") == 0)
  {
     paManExitStatus = strdup((char*) val);
  }
  else if (strcmp(item, "man author") == 0)
  {
     paManAuthor = strdup((char*) val);
  }
  else if (strcmp(item, "man reportingbugs") == 0)
  {
     paManReportingBugs = strdup((char*) val);
  }
  else if (strcmp(item, "man copyright") == 0)
  {
     paManCopyright = strdup((char*) val);
  }
  else if (strcmp(item, "man version") == 0)
  {
    free(paManVersion);
    paManVersion = strdup((char*) val);
  }
  else if (strcmp(item, "msgs to stdout") == 0)
  {
    paMsgsToStdout = (bool) val;
  }
  else if (strcmp(item, "msgs to stderr") == 0)
  {
    paMsgsToStderr = (bool) val;
  }
  else if (strcmp(item, "usage and exit on any warning") == 0)
  {
    paUsageOnAnyWarning = (bool) val;
  }
  else if (strcmp(item, "remove builtin") == 0)
  {
    paBuiltinRemove((char*) value);
  }
  else if (strcmp(item, "builtins") == 0)
  {
    paUseBuiltins = (bool) val;
  }
  else if (strcmp(item, "exit on error") == 0)
  {
    paExitOnError = (bool) val;
  }
  else if (strcmp(item, "exit on usage") == 0)
  {
    paExitOnUsage = (bool) val;
  }
  else if (strcmp(item, "print errors") == 0)
  {
    paPrintErrorsOnStderr = (bool) val;
  }
  else if (strcmp(item, "no preamble") == 0)
  {
    lmPreamble = false;
  }
  else if (strcmp(item, "prefix") == 0)
  {
    if (paPrefix != NULL)
    {
      free(paPrefix);
    }

    paPrefix = strdup((char*) val);
  }
  else if (strcmp(item, "builtin prefix") == 0)
  {
    if (paBuiltinPrefix != NULL)
    {
      free(paBuiltinPrefix);
    }

    paBuiltinPrefix = strdup((char*) val);
  }
  else if (strcmp(item, "prog name") == 0)
  {
    if (paProgName != NULL)
    {
      free(paProgName);
    }

    paProgName = strdup((char*) val);
  }
  else if (strcmp(item, "rc file") == 0)
  {
    if (paRcFileName != NULL)
    {
      free(paRcFileName);
    }

    paRcFileName = strdup((char*) val);
  }
  else if (strcmp(item, "rc dir") == 0)
  {
    if (paRcFileDir != NULL)
    {
      free(paRcFileDir);
    }

    paRcFileDir = strdup((char*) val);
  }
  else if (strcmp(item, "rc generic dir") == 0)
  {
    if (paGenericRcDir != NULL)
    {
      free(paGenericRcDir);
    }

    paGenericRcDir = strdup((char*) val);
  }
  else if (strcmp(item, "trace levels") == 0)
  {
    if (paTracelevels != NULL)
    {
      free(paTracelevels);
    }

    paTracelevels = strdup((char*) value);
  }
  else if (strcmp(item, "verbose mode") == 0)
  {
    paVerbose = (bool) val;
  }
  else if (strcmp(item, "verbose2 mode") == 0)
  {
    paVerbose2 = (bool) val;
  }
  else if (strcmp(item, "verbose3 mode") == 0)
  {
    paVerbose3 = (bool) val;
  }
  else if (strcmp(item, "verbose4 mode") == 0)
  {
    paVerbose4 = (bool) val;
  }
  else if (strcmp(item, "verbose5 mode") == 0)
  {
    paVerbose5 = (bool) val;
  }
  else if (strcmp(item, "debug mode") == 0)
  {
    paDebug = (bool) val;
  }
  else if (strcmp(item, "toDo mode") == 0)
  {
    paToDo = (bool) val;
  }
  else if (strcmp(item, "reads mode") == 0)
  {
    paReads = (bool) val;
  }
  else if (strcmp(item, "writes mode") == 0)
  {
    paWrites = (bool) val;
  }
  else if (strcmp(item, "fix mode") == 0)
  {
    paFix = (bool) val;
  }
  else if (strcmp(item, "bug mode") == 0)
  {
    paBug = (bool) val;
  }
  else if (strcmp(item, "buf mode") == 0)
  {
    paBuf = (bool) val;
  }
  else if (strcmp(item, "doubt mode") == 0)
  {
    paDoubt = (bool) val;
  }
  else if (strcmp(item, "silent mode") == 0)
  {
    strncpy(paLogLevel, "ERROR", sizeof(paLogLevel));
  }
  else if (strcmp(item, "valid log level strings") == 0)
  {
    paValidLogLevels = (char**) val;
  }
  else if (strcmp(item, "log level string") == 0)
  {
    bool ok = true;

    if (paValidLogLevels != NULL)
    {
      ok = validLogLevelCheck(paValidLogLevels, (char*) val);
    }

    if (ok == true)
    {
      lmLevelMaskSetString((char*) val);
    }
    else
    {
      char w[256];

      snprintf(w, sizeof(w), "invalid log level string: %s", (char*) val);
      PA_WARNING(PasBadValue, w);
      return -1;
    }
  }
  else if (strcmp(item, "log level mask") == 0)
  {
    lmLevelMaskSet((int) val);
  }
  else if (strcmp(item, "version") == 0)
  {
    if (paExecVersion != NULL)
    {
      free(paExecVersion);
    }
    paExecVersion = strdup((char*) value);
  }
  else if (strcmp(item, "log to file") == 0)
  {
    paLogToFile = (bool) val;
  }
  else if (strcmp(item, "log to screen") == 0)
  {
    if (value == (void*)true)
    {
      paLogToScreen  = true;
      paMsgsToStdout = true;
    }
    else if ( value == (void*) false)
    {
      paLogToScreen = false;
    }
    else if (strcmp((char*) value, "only errors") == 0)
    {
      paLogToScreen         = true;
      paLogScreenOnlyErrors = true;
    }
  }
  else if (strcmp(item, "bool option with value as non-recognized option") == 0)
  {
    paBoolWithValueIsUnrecognized = true;
  }
  else if (strcmp(item, "log to stderr") == 0)
  {
    paLogScreenToStderr = (bool) val;
  }
  else if (strcmp(item, "log file") == 0)
  {
    if (paLogFilePath != NULL)
    {
      free(paLogFilePath);
    }

    paLogFilePath = strdup((char*) value);
  }
  else if (strcmp(item, "log dir") == 0)
  {
    snprintf(paLogDir, sizeof(paLogDir), "%s", (char*) value);
  }
  else if (strcmp(item, "default value") == 0)
  {
    PaiArgument*  argP;
    char*         option = (char*) value;
    char*         val    = (char*) value2;

    argP = paBuiltinLookup(option);
    if (argP == NULL)
    {
      printf("Sorry, builtin '%s' not found - cannot change default value ...\n", option);
      exit(1);
    }

    if (argP->type == PaString)
    {
      strcpy((char*) argP->varP, val);
      argP->def = (int64_t) argP->varP;
    }
    else if (argP->type == PaInt)
    {
      argP->def = (int64_t) val;
    }
    else if (argP->type == PaUShort)
    {
      argP->def = (int64_t) val;
    }
    else if (argP->type == PaBool)
    {
      if (strcmp((char*) val, "true") == 0)
      {
        argP->def = 1;
      }
      else if (strcmp((char*) val, "TRUE") == 0)
      {
        argP->def = 1;
      }
      else if (strcmp((char*) val, "false") == 0)
      {
        argP->def = 0;
      }
      else if (strcmp((char*) val, "FALSE") == 0)
      {
        argP->def = 0;
      }
      else if ((int64_t) val == 0)
      {
        argP->def = 0;
      }
      else if ((int64_t) val == 1)
      {
        argP->def = 1;
      }
      else
      {
        printf("Sorry, bad default value for boolean option '%s'\n", argP->option);
        exit(1);
      }
    }
    else
    {
      printf("Sorry, not allowed to set default value for builtin of type '%s'", paTypeName(argP->type));
      exit(1);
    }
  }
  else if (strcmp(item, "log file line format") == 0)
  {
    if (paLogFileLineFormat != NULL)
    {
      free(paLogFileLineFormat);
    }

    paLogFileLineFormat = strdup((char*) value);
  }
  else if (strcmp(item, "log file time format") == 0)
  {
    if (paLogFileTimeFormat != NULL)
    {
      free(paLogFileTimeFormat);
    }

    paLogFileTimeFormat = strdup((char*) value);
  }
  else if (strcmp(item, "screen line format") == 0)
  {
    if (paLogScreenLineFormat != NULL)
    {
      free(paLogScreenLineFormat);
    }

    paLogScreenLineFormat = strdup((char*) value);
  }
  else if (strcmp(item, "screen time format") == 0)
  {
    if (paLogScreenTimeFormat != NULL)
    {
      free(paLogScreenTimeFormat);
    }

    paLogScreenTimeFormat = strdup((char*) value);
  }
  else if (strcmp(item, "make sure paConfigInit is called") == 0)
  {
  }
  else if (strcmp(item, "if hook active, no traces to file") == 0)
  {
    lmNoTracesToFileIfHookActive = true;
  }
  else if (strcmp(item, "even if hook active, no traces to file") == 0)
  {
    lmNoTracesToFileIfHookActive = false;
  }
  else
  {
    char w[256];

    snprintf(w, sizeof(w), "paConfig command '%s' not recognized", item);
    PA_WARNING(PasNoSuchCommand, w);
  }

  return 0;
}



/* ****************************************************************************
*
* paConfigActions - 
*/
int paConfigActions(bool preTreat)
{
  if (paVerbose5 == true)
  {
    paVerbose4 = true;
  }

  if (paVerbose4 == true)
  {
    paVerbose3 = true;
  }

  if (paVerbose3 == true)
  {
    paVerbose2 = true;
  }

  if (paVerbose2 == true)
  {
    paVerbose = true;
  }

  lmVerbose       = paVerbose;
  lmVerbose2      = paVerbose2;
  lmVerbose3      = paVerbose3;
  lmVerbose4      = paVerbose4;
  lmVerbose5      = paVerbose5;
  lmDebug         = paDebug;
  lmToDo          = paToDo;
  lmReads         = paReads;
  lmWrites        = paWrites;
  lmFix           = paFix;
  lmBug           = paBug;
  lmDoubt         = paDoubt;
  lmBuf           = paBuf;
  lmAssertAtExit  = paAssertAtExit;
  lmSilent        = paSilent;

  if (preTreat)
  {
    lmTraceSet(paTracelevels);
  }
  else
  {
    LM_ENTRY();
    lmTraceSet(paTraceV);

    if (paNoClear == true)
    {
      lmDontClear();
    }

    if ((paClearAt != -1) || (paKeepLines != -1) || (paLastLines != -1))
    {
      /* logMsg must be changed to not change -1 values */
      lmClearAt(paClearAt, paKeepLines, paLastLines);
    }

    if (paSilent)
    {
      strncpy(paLogLevel, "ERROR", sizeof(paLogLevel));
    }

    if (paLogLevel[0] != 0)
    {
      bool ok = true;

      if (paValidLogLevels != NULL)
      {
        ok = validLogLevelCheck(paValidLogLevels, paLogLevel);
      }

      if (ok == true)
      {
        lmLevelMaskSetString(paLogLevel);
      }
      else
      {
        char w[256];

        snprintf(w, sizeof(w), "invalid log level string: %s", (char*) paLogLevel);
        PA_WARNING(PasBadValue, w);
      }
    }

    LM_T(LmtPaConfigAction, ("setting trace levels to '%s'", paTraceV));
    LM_EXIT();
  }

  return 0;
}
