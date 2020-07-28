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
#include <stdlib.h>                   /* atoi                                 */
#include <sys/types.h>                /* uid_t                                */
#include <unistd.h>                   /* geteuid                              */
#include <pwd.h>                      /* getpw                                */

#include "parseArgs/baStd.h"          /* BA standard header file              */
#include "logMsg/logMsg.h"            /* lmVerbose, lmDebug, ...              */

#include "parseArgs/paPrivate.h"      /* PaTypeUnion, config variables, ...   */
#include "parseArgs/paTraceLevels.h"  /* LmtPaEnvVal, ...                     */
#include "parseArgs/parseArgs.h"      /* PaArgument                           */
#include "parseArgs/paBuiltin.h"      /* paBuiltinNoOf, ...                   */
#include "parseArgs/paWarning.h"      /* paWarningAdd                         */
#include "parseArgs/paIterate.h"      /* paIterateInit, paIterateNext         */
#include "parseArgs/paEnvVals.h"      /* paEnvName                            */
#include "parseArgs/paConfig.h"       /* paConfigActions                      */



/* ****************************************************************************
*
* newlineStrip - 
*/
static char* newlineStrip(char* s)
{
  if (s[strlen(s) - 1] == '\n')
  {
    s[strlen(s) - 1] = 0;
  }

  return s;
}


/* ****************************************************************************
*
* commentStrip - 
*/
static char* commentStrip(char* s, char c)
{
  char* tmP;

  if ((tmP = strchr(s, c)) != NULL)
  {
    *tmP = 0;
  }

  return s;
}



/* ****************************************************************************
*
* dirFind - find "most important" directory with rc-file
*/
static int dirFind(char* dir, int dirLen)
{
  char            path[512];
  uid_t           euid;

  /* Path to lookup RC file:
     1. paConfig
     2. this directory
     3. home directory
     4. generic directory
   */

  LM_T(LmtPaRcFile, ("paRcFileName: '%s'", paRcFileName));

  /* X. prescan argV to see if the home directory is set */
  /* X. if env var XX_HOME set: $XX_HOME/.progname       */

  /* 1. paConfig */
  if (paRcFileDir != NULL)
  {
    LM_T(LmtPaRcFile, ("checking dir '%s'", paRcFileDir));
    snprintf(dir,  dirLen, "%s", paRcFileDir);
    snprintf(path, sizeof(path), "%s/%s", dir, paRcFileName);

    if (access(path, R_OK) == 0)
    {
      return 0;
    }
  }

  /* 2. directory of execution */
  if (getcwd(dir, dirLen) != NULL)
  {
    LM_T(LmtPaRcFile, ("checking dir '%s'", dir));
    snprintf(path, sizeof(path), "%s/%s", dir, paRcFileName);

    if (access(path, R_OK) == 0)
    {
      return 0;
    }
  }
  else
  {
    LM_W(("getcwd failed"));
  }

  /* 3. users home directory */
  struct passwd   pw;
  struct passwd*  pwP;
  char            buf[64];

  euid = geteuid();
  if (getpwuid_r(euid, &pw, buf, sizeof(buf), &pwP) == 0)
  {
    LM_T(LmtPaRcFile, ("checking home dir '%s'", pw.pw_dir));

    snprintf(dir, dirLen, "%s", pw.pw_dir);
    snprintf(path, sizeof(path), "%s/%s", dir, paRcFileName);

    if (access(path, R_OK) == 0)
    {
      return 0;
    }
  }
  else
  {
    LM_W(("geteuid or getpwuid_r failed"));
  }

  /* 4. Generic RC file directory, if any ... */
  if (paGenericRcDir != NULL)
  {
    LM_T(LmtPaRcFile, ("checking dir '%s'", paGenericRcDir));
    snprintf(dir, dirLen, "%s", paGenericRcDir);
    snprintf(path, sizeof(path), "%s/%s", dir, paRcFileName);

    if (access(path, R_OK) == 0)
    {
      return 0;
    }
  }

  return -1;
}



/* ****************************************************************************
*
* paRcFileParse - parse startup file
*/
int paRcFileParse(void)
{
  char   dir[256];
  char   path[512];
  char   line[512];
  int    lineNo = 0;
  FILE*  fP;
  char   w[512];

  LM_ENTRY();

  if ((paRcFileName == NULL) || (paRcFileName[0] == 0))
  {
    LM_EXIT();
    return 0;
  }

  if (dirFind(dir, sizeof(dir)) == 0)
  {
    LM_T(LmtPaRcFile, ("RC file '%s' found in directory '%s'", paRcFileName, dir));
  }
  else
  {
    return 0;
  }

  snprintf(path, sizeof(path), "%s/%s", dir, paRcFileName);
  if ((fP = fopen(path, "r")) == NULL)
  {
    LM_RE(-1, ("error opening RC file '%s': %s", path, strerror(errno)));
  }

  LM_T(LmtPaRcFile, ("parsing RC file %s", path));

  while (fgets(line, sizeof(line), fP) != NULL)
  {
    char*         delim;
    char*         var;
    char*         val;
    PaiArgument*  aP;
    bool          varFound;
    char          envVarName[128];

    ++lineNo;
    LM_T(LmtPaRcFile, ("got line %d", lineNo));
    newlineStrip(line);
    LM_T(LmtPaRcFile, ("got line %d", lineNo));
    commentStrip(line, '#');
    LM_T(LmtPaRcFile, ("got line %d", lineNo));
    baWsStrip(line);
    LM_T(LmtPaRcFile, ("got line %d", lineNo));
    if (line[0] == 0)
    {
      continue;
    }

    LM_T(LmtPaRcFile, ("got line %d", lineNo));
    delim = strchr(line, '=');
    if (delim == NULL)
    {
      char w[600];

      snprintf(w, sizeof(w), "%s[%d]: no delimiter found", path, lineNo);
      PA_WARNING(PasParseError, w);
      continue;
    }

    *delim = 0;
    var    = line;
    val    = &delim[1];
    baWsStrip(var);
    baWsStrip(val);

    if (var[0] == 0)
    {
      fclose(fP);
      LM_RE(-1, ("%s[%d]: no variable ...", path, lineNo));
    }

    if (val[0] == 0)
    {
      fclose(fP);
      LM_RE(-1, ("%s[%d]: no value for variable %s", path, lineNo, var));
    }

    varFound = false;

    paIterateInit();
    while ((aP = paIterateNext(paiList)) != NULL)
    {
      paEnvName(aP, envVarName, sizeof(envVarName));

      if (strcmp(var, envVarName) == 0)
      {
        aP->from = PafRcFile;
        LM_T(LmtPaRcFileVal, ("got value '%s' for %s", val, envVarName));
        varFound = true;
        break;
      }
    }

    if (varFound == false)
    {
      char w[600];

      snprintf(w, sizeof(w), "%s[%d]: variable '%s' not recognized", path, lineNo, var);
      PA_WARNING(PasNoSuchVariable, w);
      continue;
    }

    switch (aP->type)
    {
    case PaString:
      strcpy((char*) aP->varP, val);
      break;

    case PaBoolean:
      if ((strcmp(val, "TRUE") == 0)
      ||  (strcmp(val, "ON")   == 0)
      ||  (strcmp(val, "yes")   == 0)
      ||  (strcmp(val, "1")    == 0))
        *((bool*) (int64_t) aP->varP) = true;
      else if ((strcmp(val, "FALSE") == 0)
      ||       (strcmp(val, "OFF")   == 0)
      ||       (strcmp(val, "no")   == 0)
      ||       (strcmp(val, "0")     == 0))
        *((bool*) (int64_t) aP->varP) = false;
      else
      {
        snprintf(w, sizeof(w), "bad value '%s' for boolean variable %s", val, envVarName);
        PA_WARNING(PasNoSuchBooleanValue, w);
      }
      break;

    case PaSList:
    case PaIList:
      LM_TODO(("lists ..."));
      break;

    case PaInt:
    case PaIntU:
      *((int64_t*) aP->varP) = baStoi(val);
      break;

    case PaShort:
    case PaShortU:
      *((int16_t*) (int64_t) aP->varP) = baStoi(val);
      break;

    case PaFloat:
      *((float*) (int64_t) aP->varP) = baStof(val);
      break;

    case PaDouble:
      *((double*) (int64_t) aP->varP) = baStod(val);
      break;

    case PaChar:
    case PaCharU:
      *((char*) (int64_t) aP->varP) = baStoi(val);
      break;

    default:
      snprintf(w, sizeof(w), "bad type %d for variable %s", aP->type, envVarName);
      PA_WARNING(PasNoSuchType, w);
    }
  }

  fclose(fP);
  return 0;
}
