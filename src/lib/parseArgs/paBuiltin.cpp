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
* fermin at tid dot es
*
* Author: developer
*/

#include <stdio.h>              /* sprintf                                  */
#include <cstdlib>				/* C++ free(.)								 */

#include "baStd.h"              /* BA standard header file                  */

#include "parseArgs.h"          /* PaArgument                                */
#include "paLog.h"              /* PA_XXX                                    */
#include "paConfig.h"           /* config variables                          */
#include "paWarning.h"          /* paWaringInit, paWarningAdd                */
#include "paIterate.h"          /* paIterateInit, paIterateNext              */
#include "paEnvVals.h"          /* paEnvName                                 */
#include "paBuiltin.h"          /* Own interface                             */



/* ****************************************************************************
*
* option variables
*/
bool            paUsageVar;     /* the -u options (-u, -h, -?, -help) */
bool            paEUsageVar;    /* the -U option                      */
bool            paHelpVar;      /* the -help option                   */
bool            paVersion;
char            paHome[512];
bool            paNoClear;
bool            paClearAt;
int             paKeepLines;
int             paLastLines;
bool            paLogAppend;
bool            paAssertAtExit;
static int      dummy;
char            paUserName[64];
char            paPwd[512];
char            paColumns[128];
char            paRows[128];
char            paDisplay[128];
char            paEditor[128];
char            paLang[128];
char            paPager[128];
char            paPpid[128];
char            paPrinter[128];
char            paShell[128];
char            paTerm[128];
char            paSystem[128];
char            paVisual[128];
char            paLogDir[256];


#define T (int) true
#define F (int) false

#define PAI_REST            PafUnchanged, { 'N', 'a', 'm', 'e', 0 }, 0, 0, false, false, false, true, false, false, 0
#define PAI_REST_IN_USAGE   PafUnchanged, { 'N', 'a', 'm', 'e', 0 }, 0, 0, false, false, false, true, false, true, 0
#define PAI_END_OF_ARGS     { "^D", NULL, "NADA", PaLastArg, PaReq, 0, 0, 0, "", PAI_REST }
/* ****************************************************************************
*
* paBuiltin - 
*/
PaiArgument paBuiltin[] =
{
 { 
    "--", 
    &dummy,
    NULL,
    PaBool,
    PaHid,
    false,
    true,
    false,
    "X delimiter", PAI_REST
 },

 { "-U",         &paEUsageVar,    NULL,               PaBool,   PaOpt,      F,     T,     F,  "extended usage",              PAI_REST_IN_USAGE },
 { "-u",         &paUsageVar,     NULL,               PaBool,   PaOpt,      F,     T,     F,  "usage",                       PAI_REST_IN_USAGE },
 { "-h",         &paUsageVar,     NULL,               PaBool,   PaOpt,      F,     T,     F,  "usage",                       PAI_REST },
 { "-help",      &paHelpVar,      NULL,               PaBool,   PaOpt,      F,     T,     F,  "show help",                   PAI_REST },
 { "--help",     &paHelpVar,      NULL,               PaBool,   PaOpt,      F,     T,     F,  "show help",                   PAI_REST },
 { "--version",  &paVersion,      NULL,               PaBool,   PaOpt,      F,     T,     F,  "show version",                PAI_REST_IN_USAGE },
 { "-version",   &paVersion,      NULL,               PaBool,   PaOpt,      F,     T,     F,  "show version",                PAI_REST },
 { "-logDir",    &paLogDir,       "LOG_DIR",          PaStr,    PaOpt,      -19,  PaNL,  PaNL,  "log file directory",          PAI_REST_IN_USAGE }, // No default value, to avoid removing the one set in paConfig
 { "",           paUserName,      "!USER",            PaStr,    PaHid,      0,  PaNL,  PaNL,  "user name",                   PAI_REST },
 { "",           paPwd,           "!PWD",             PaStr,    PaHid,      0,  PaNL,  PaNL,  "current dir",                 PAI_REST },
 { "",           paColumns,       "!COLUMNS",         PaStr,    PaHid,      0,  PaNL,  PaNL,  "columns",                     PAI_REST },
 { "",           paRows,          "!ROWS",            PaStr,    PaHid,      0,  PaNL,  PaNL,  "rows",                        PAI_REST },
 { "",           paDisplay,       "!DISPLAY",         PaStr,    PaHid,      0,  PaNL,  PaNL,  "display",                     PAI_REST },
 { "",           paEditor,        "!EDITOR",          PaStr,    PaHid,      0,  PaNL,  PaNL,  "editor",                      PAI_REST },
 { "",           paLang,          "!LANG",            PaStr,    PaHid,      0,  PaNL,  PaNL,  "language",                    PAI_REST },
 { "",           paPager,         "!PAGER",           PaStr,    PaHid,      0,  PaNL,  PaNL,  "pager",                       PAI_REST },
 { "",           paPpid,          "!PPID",            PaStr,    PaHid,      0,  PaNL,  PaNL,  "parent process id",           PAI_REST },
 { "",           paPrinter,       "!PRINTER",         PaStr,    PaHid,      0,  PaNL,  PaNL,  "printer",                     PAI_REST },
 { "",           paShell,         "!SHELL",           PaStr,    PaHid,      0,  PaNL,  PaNL,  "shell",                       PAI_REST },
 { "",           paTerm,          "!TERM",            PaStr,    PaHid,      0,  PaNL,  PaNL,  "terminal",                    PAI_REST },
 { "",           paSystem,        "!SYSTEM",          PaStr,    PaHid,      0,  PaNL,  PaNL,  "system",                      PAI_REST },
 { "",           paVisual,        "!VISUAL",          PaStr,    PaHid,      0,  PaNL,  PaNL,  "visual",                      PAI_REST },
 { "-t",         paTraceV,        "TRACE",            PaStr,    PaOpt,      0,  PaNL,  PaNL,  "trace level",                 PAI_REST_IN_USAGE },
 { "--silent",   &paSilent,       "SILENT",           PaBool,   PaOpt,      F,     T,     F,  "silent mode",                 PAI_REST_IN_USAGE },
 { "-v",         &paVerbose,      "VERBOSE",          PaBool,   PaOpt,      F,     T,     F,  "verbose mode",                PAI_REST_IN_USAGE },
 { "-vv",        &paVerbose2,     "VERBOSE2",         PaBool,   PaOpt,      F,     T,     F,  "verbose2 mode",               PAI_REST_IN_USAGE },
 { "-vvv",       &paVerbose3,     "VERBOSE3",         PaBool,   PaOpt,      F,     T,     F,  "verbose3 mode",               PAI_REST_IN_USAGE },
 { "-vvvv",      &paVerbose4,     "VERBOSE4",         PaBool,   PaOpt,      F,     T,     F,  "verbose4 mode",               PAI_REST_IN_USAGE },
 { "-vvvvv",     &paVerbose5,     "VERBOSE5",         PaBool,   PaOpt,      F,     T,     F,  "verbose5 mode",               PAI_REST_IN_USAGE },
 { "-d",         &paDebug,        "DEBUG",            PaBool,   PaOpt,      F,     T,     F,  "debug mode",                  PAI_REST_IN_USAGE },
 { "-r",         &paReads,        "READS",            PaBool,   PaOpt,      F,     T,     F,  "reads mode",                  PAI_REST_IN_USAGE },
 { "-w",         &paWrites,       "WRITES",           PaBool,   PaOpt,      F,     T,     F,  "writes mode",                 PAI_REST_IN_USAGE },
 { "-toDo",      &paToDo,         "TODO",             PaBool,   PaOpt,      F,     T,     F,  "toDo mode",                   PAI_REST },
 { "-F",         &paFix,          "FIX",              PaBool,   PaOpt,      F,     T,     F,  "fixes mode",                  PAI_REST },
 { "-B",         &paBug,          "BUGS",             PaBool,   PaOpt,      F,     T,     F,  "bugs mode",                   PAI_REST },
 { "-b",         &paBuf,          "BUFS",             PaBool,   PaOpt,      F,     T,     F,  "buf mode",                    PAI_REST },
 { "-?",         &paDoubt,        "DOUBT",            PaBool,   PaOpt,      F,     T,     F,  "doubts mode",                 PAI_REST },
 { "-lmnc",      &paNoClear,      "NO_CLEAR",         PaBool,   PaOpt,      F,     T,     F,  "don't clear log file",        PAI_REST },
 { "-lmca",      &paClearAt,      "CLEAR_AT",         PaInt,    PaOpt,     -1,  PaNL,  PaNL,  "clear at lines",              PAI_REST },
 { "-lmkl",      &paKeepLines,    "KEEP_LINES",       PaInt,    PaOpt,     -1,  PaNL,  PaNL,  "clear 'keep lines'",          PAI_REST },
 { "-lmll",      &paLastLines,    "LAST_LINES",       PaInt,    PaOpt,     -1,  PaNL,  PaNL,  "clear 'last lines'",          PAI_REST },
 { "-logAppend", &paLogAppend,   "LOG_APPEND",       PaBool,   PaOpt,      F,     T,     F,  "append to log-file",          PAI_REST_IN_USAGE },
 { "-assert",    &paAssertAtExit, "ASSERT_AT_EXIT",   PaBool,   PaOpt,      F,     T,     F,  "assert instead of exiting",   PAI_REST },
 PAI_END_OF_ARGS
};

#undef T
#undef F


extern int paBuiltins;
/* ****************************************************************************
*
* paBuiltinNoOf - 
*/
int paBuiltinNoOf(void)
{
	int ix = 0;

	if (paBuiltins != -1)
		return paBuiltins;

	PA_M(("Counting builtins"));

	if (paUseBuiltins == false)
	{
		PA_M(("Not using builtins!"));
		return 0;
	}

	while (paBuiltin[ix].type != PaLastArg)
		++ix;

	paBuiltins = ix;
	PA_M(("%d builtins found", paBuiltins));
	return paBuiltins;
}



/* ****************************************************************************
*
* paBuiltinRemove
*/
int paBuiltinRemove(char* name)
{
	PaiArgument* aP;

	/* 1. lookup aP->option or aP->variable          */
	/* 2. if found - mark the aP as PaRemoved        */
	/* 3. Then paConfig needs the actions to do it   */

	paIterateInit();
	while ((aP = paIterateNext(NULL)) != NULL)
	{
		char envVarName[128];

		paEnvName(aP, envVarName);

		if (((aP->option) && (strcmp(name, aP->option) == 0))
		|| ((aP->envName) && (strcmp(name, envVarName) == 0)))
		{
			aP->removed = true;
			break;
		}
	}

	if (aP == NULL)
	{
		char w[512];

		sprintf(w, "cannot remove builtin '%s' - not found", name);
		PA_WARNING(PasBuiltinRemove, w);
		return 1;
	}

	return 0;
}



/* ****************************************************************************
*
* paBuiltinLookup - 
*/
PaiArgument* paBuiltinLookup(char* option)
{
	PaiArgument* aP;

	paIterateInit();
	while ((aP = paIterateNext(NULL)) != NULL)
    {
        if (strcmp(aP->option, option) == 0)
            return aP;
    }

    return NULL;
}
