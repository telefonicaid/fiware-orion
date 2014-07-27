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

#include <stdlib.h>             /* system                                    */
#include <unistd.h>             /* getpid                                    */

#include "baStd.h"              /* BA standard header file                   */
#include "logMsg/logMsg.h"      /* lmVerbose, lmDebug, ...                   */

#include "parseArgs.h"          /* PaArgument                                */
#include "paPrivate.h"          /* PaTypeUnion, paBuiltins, ...              */
#include "paTraceLevels.h"      /* LmtPaApVals, LmtPaUsage, ...              */
#include "paLog.h"              /* PA_XXX                                    */
#include "paFrom.h"             /* paFrom, paFromName                        */
#include "paConfig.h"           /* paHelpFile, paHelpText                    */
#include "paFullName.h"         /* paFullName                                */
#include "paBuiltin.h"          /* paBuiltin, paBuiltinNoOf                  */
#include "paIterate.h"          /* paIterateInit, paIterateNext              */
#include "paEnvVals.h"          /* paEnvName                                 */
#include "paParse.h"            /* progNameV                                 */
#include "paBuiltin.h"          /* paBuiltinNoOf                             */
#include "paUsage.h"            /* Own interface                             */



/* ****************************************************************************
*
* paUsageProgName -
*/
char* paUsageProgName = NULL;



/* ****************************************************************************
*
* escape - replace '/' for '//' ...
*/
char* escape(char out[], char* value)
{
	int ix    = 0;
	int outIx = 0;
	
	// printf("In escape\n");
	if (value == NULL)
		return (char*) "(null)";

	while (value[ix] != 0)
	{
		// printf("value[%d]: %d\n", ix, value[ix]);
		if (value[ix] == '\n')
		{
			out[outIx++] = '\\';
			out[outIx++] = 'n';
		}
		else if (value[ix] == '\t')
		{
			out[outIx++] = '\\';
			out[outIx++] = 't';
		}
		else
			out[outIx++] = value[ix];

		++ix;
	}
	out[outIx] = 0;

	// printf("Done!\n");
	return out;
}



/* ****************************************************************************
*
* getApVals - 
*/
static void getApVals
(
	PaiArgument*  aP,
	char*         defVal,
	char*         minVal,
	char*         maxVal,
	char*         realVal
)
{
	PaTypeUnion*  defP;
	PaTypeUnion*  minP;
	PaTypeUnion*  maxP;
	char          out[256];

	LM_T(LmtPaApVals, ("Fixing def, min, max, real values for %s", aP->name));

	if (aP->def == PaNoDef)
		aP->hasDefault = false;
	if (aP->min == PaNoLim)
		aP->hasMinLimit = false;
	if (aP->max == PaNoLim)
		aP->hasMaxLimit = false;

	defP = (PaTypeUnion*) &aP->def;
	minP = (PaTypeUnion*) &aP->min;
	maxP = (PaTypeUnion*) &aP->max;

	switch (aP->type)
	{
	case PaIList:
	case PaSList:
	case PaLastArg:
		return;

	case PaString:
		// printf("Calling 'escape' four times\n");
		sprintf(defVal,  "'%s'", (defP->i != PaNoDef)? escape(out, (char*) aP->def) : "no default");
		sprintf(minVal,  "'%s'", (minP->i != PaNoLim)? escape(out, (char*) aP->min)  : "no min limit");
		sprintf(maxVal,  "'%s'", (maxP->i != PaNoLim)? escape(out, (char*) aP->max)  : "no max limit");
		sprintf(realVal, "'%s'", (aP->varP != NULL)?   escape(out, (char*) aP->varP) : "no value");
		break;

	case PaBoolean:
		sprintf(defVal,  "%s", BA_FT(aP->def));
		sprintf(realVal, "%s", BA_FT(*((bool*) aP->varP)));
		aP->hasMinLimit = false;
		aP->hasMaxLimit = false;
		break;

	case PaInt:
        sprintf(defVal,  "%d", (int) (aP->def & 0xFFFFFFFF));
		sprintf(minVal,  "%d", (int) (aP->min & 0xFFFFFFFF));
		sprintf(maxVal,  "%d", (int) (aP->max & 0xFFFFFFFF));
		sprintf(realVal, "%d", *((int*) aP->varP));
		break;

    case PaInt64:
		sprintf(defVal,  "%lld", aP->def);
		sprintf(minVal,  "%lld", aP->min);
		sprintf(maxVal,  "%lld", aP->max);
		sprintf(realVal, "%lld", *((long long*) aP->varP));
		break;

	case PaIntU:
		sprintf(defVal,  "%u", (int) (aP->def & 0xFFFFFFFF));
		sprintf(minVal,  "%u", (int) (aP->min & 0xFFFFFFFF));
		sprintf(maxVal,  "%u", (int) (aP->max & 0xFFFFFFFF));
		sprintf(realVal, "%u", *((unsigned int*) aP->varP));
		break;

    case PaIntU64:
		sprintf(defVal,  "%llu",  aP->def);
		sprintf(minVal,  "%llu",  aP->min);
		sprintf(maxVal,  "%llu",  aP->max);
		sprintf(realVal, "%llu", *((unsigned long long*) aP->varP));
		break;

	case PaShort:
		sprintf(defVal,  "%d", (short) (aP->def & 0xFFFF));
		sprintf(minVal,  "%d", (short) (aP->min & 0xFFFF));
		sprintf(maxVal,  "%d", (short) (aP->max & 0xFFFF));
		sprintf(realVal, "%d", (*((short*) aP->varP) & 0xFFFF));
		break;

	case PaShortU:
		sprintf(defVal,  "%u", (int) (aP->def & 0xFFFF));
		sprintf(minVal,  "%u", (int) (aP->min & 0xFFFF));
		sprintf(maxVal,  "%u", (int) (aP->max & 0xFFFF));
		sprintf(realVal, "%u", (*((unsigned short*) aP->varP) & 0xFFFF));
		break;

	case PaChar:
		sprintf(defVal,  "%d", (int) (aP->def & 0xFF));
		sprintf(minVal,  "%d", (int) (aP->min & 0xFF));
		sprintf(maxVal,  "%d", (int) (aP->max & 0xFF));
		sprintf(realVal, "%d", (*((char*) aP->varP) & 0xFF));
		break;

	case PaCharU:
		sprintf(defVal,  "%u", (int) (aP->def & 0xFF));
		sprintf(minVal,  "%u", (int) (aP->min & 0xFF));
		sprintf(maxVal,  "%u", (int) (aP->max & 0xFF));
		sprintf(realVal, "%u", (*((unsigned char*) aP->varP) & 0xFF));
		break;

	case PaFloat:
		sprintf(defVal,  "%f", (float) aP->def);
		sprintf(minVal,  "%f", (float) aP->min);
		sprintf(maxVal,  "%f", (float) aP->max);
		sprintf(realVal, "%f", (*((float*) aP->varP)));
		break;

	case PaDouble:
		sprintf(defVal,  "%f", (double) aP->def);
		sprintf(minVal,  "%f", (double) aP->min);
		sprintf(maxVal,  "%f", (double) aP->max);
		sprintf(realVal, "%f", (*((double*) aP->varP)));
		break;
	}

	LM_T(LmtPaApVals, ("Get def(%s), min(%s), max(%s), real(%s) values for %s",
					   defVal, minVal, maxVal, realVal, aP->name));
}



/* ****************************************************************************
*
* paUsage - print synopsis (to file pointer or to paResultString)
*/
void paUsage(void)
{
	char*         spacePad;
	char          string[512];
	char          s[1024];
	PaiArgument*  aP;
	int           ix       = -1;
	bool          firstUserOptionFound = false;

	LM_T(LmtPaUsage, ("presenting usage"));

	spacePad = (char*) strdup(progName);
	memset(spacePad, 0x20202020, strlen(spacePad));  /* replace progName */

  if (paUsageProgName != NULL)
    sprintf(s, "Usage: %s ", paUsageProgName);
  else
    sprintf(s, "Usage: %s ", progName);

	strncat(paResultString, s, sizeof(paResultString) - 1);

	// paLogOn = true;
	paIterateInit();
	PA_M(("------------- presenting usage -------------"));

  bool firstTime = true;
	while ((aP = paIterateNext(paiList)) != NULL)
	{
		char  xName[512];

		PA_M(("Got option '%s'", aP->option));

		++ix;

		if (aP->sort == PaHid)
			continue;
		if ((aP->isBuiltin == true) && (aP->includeInUsage == false))
			continue;

    if (!firstTime)
    {
		  sprintf(s, "%s        ", spacePad); /* 8 spaces for "Usage:  " */
		  strncat(paResultString, s, sizeof(paResultString) - 1);
    }
    firstTime = false;

		if ((aP->isBuiltin == false) && (firstUserOptionFound == false))
		{
			firstUserOptionFound = true;
			strncat(paResultString, "\n",       sizeof(paResultString) - 1 - strlen(paResultString));
			strncat(paResultString, spacePad,   sizeof(paResultString) - 1 - strlen(paResultString));
			strncat(paResultString, "        ", sizeof(paResultString) - 1 - strlen(paResultString));
		}

		if (PA_IS_OPTION(aP) && (aP->sort == PaOpt))
			sprintf(xName, "[%s]", paFullName(aP, string, sizeof(string)));
		else if (PA_IS_OPTION(aP) && (aP->sort == PaReq))
			sprintf(xName, "%s", paFullName(aP, string, sizeof(string)));
		else if (PA_IS_PARAMETER(aP) && (aP->sort == PaOpt))
			sprintf(xName, "[parameter: %s]", aP->description);
		else if (PA_IS_PARAMETER(aP) && (aP->sort == PaReq))
			sprintf(xName, "parameter: %s", aP->description);
		else
			strcpy(xName, "                    ");

		sprintf(s, " %s\n", xName);
		strncat(paResultString, s, sizeof(paResultString) - 1);

		PA_M(("parsed arg %d", ix));
    }
    // paLogOn = false;
    free(spacePad);
    PA_M(("presenting usage"));

	printf("%s\n", paResultString);
    exit(0);
}



/* ****************************************************************************
*
* paExtendedUsage - print extended synopsis
*/
void paExtendedUsage(void)
{
	char*         spacePad;
	char          string[80];
	PaiArgument*  aP;
	int           optNameMaxLen = 0;
	int           varNameMaxLen = 0;
	int           valsMaxLen    = 0;
	char          format[64];
	char          progNAME[128];
	bool          firstLine = true;

	sprintf(progNAME, "Extended Usage: %s ", progName);
	spacePad = (char*) strdup(progNAME);
	memset(spacePad, 0x20202020, strlen(spacePad));  /* replace progNAME */

	PA_M(("-------------- Preparing list for Extended usage -----------------"));
	paIterateInit();
	while ((aP = paIterateNext(paiList)) != NULL)
	{
		char  name[128];
		char  vals[128];
		char  defVal[20];
		char  minVal[20];
		char  maxVal[20];
		char  realVal[80];
		char  out[256];
		char  out2[256];

        PA_M(("processing '%s' for extended usage\n", aP->name));

		/* 1. Option Name */
		memset(name, 0, sizeof(name));
		if (PA_IS_OPTION(aP) && (aP->sort == PaOpt))
			sprintf(name, "[%s]", paFullName(aP, string, sizeof(string)));
		else if (PA_IS_OPTION(aP) && (aP->sort == PaReq))
			sprintf(name, "%s", paFullName(aP, string, sizeof(string)));
		else if (PA_IS_PARAMETER(aP) && (aP->sort == PaOpt))
			sprintf(name, "(%s)", aP->description);
		else if (PA_IS_PARAMETER(aP) && (aP->sort == PaReq))
			sprintf(name, "[%s]", aP->description);
		optNameMaxLen = MAX(strlen(name), (unsigned int) optNameMaxLen);


		/* 2. Variable Name */
		memset(name, 0, sizeof(name));
		if (PA_IS_VARIABLE(aP))
		{
			paEnvName(aP, name, sizeof(name));
			varNameMaxLen = MAX(strlen(name), (unsigned int) varNameMaxLen);
		}
		

		/* 3. Values */
		getApVals(aP, defVal, minVal, maxVal, realVal);
	
		if (aP->hasDefault == false)
			sprintf(name, "%s", realVal);
		else
			sprintf(name, "%s /%s/", realVal, defVal);

		if ((aP->hasMinLimit == false) && (aP->hasMaxLimit == false))
			sprintf(vals, "%s", name);
		else if (aP->hasMinLimit == false)
			sprintf(vals, "%s <= %s", name, escape(out, maxVal));
		else if (aP->hasMaxLimit == false)
			sprintf(vals, "%s >= %s", name, escape(out, minVal));
		else
			sprintf(vals, "%s <= %s <= %s", escape(out, minVal), name, escape(out2, maxVal));

		valsMaxLen = MAX(strlen(vals), (unsigned int) valsMaxLen);
	}
	
	sprintf(format, "%%-%ds %%-%ds %%-%ds %%-%ds %%s\n",
			(int) strlen(progName) + 1,
			optNameMaxLen + 2,
			varNameMaxLen + 2,
			valsMaxLen + 2);

	// paLogOn = true;
	paIterateInit();
	while ((aP = paIterateNext(paiList)) != NULL)
	{
		char  optName[128];
		char  varName[128];
		char  vals[128];
		char  from[128];
		char  defVal[20];
		char  minVal[20];
		char  maxVal[20];
		char  realVal[80];
		char  s[512];

		if (aP->sort == PaHid)
		{
			PA_M(("skipping hidden option '%s'", aP->option));
			continue;
		}

		/* 1. Option Name */
		if (PA_IS_OPTION(aP) && (aP->sort == PaOpt))
			sprintf(optName, "[%s]", paFullName(aP, string, sizeof(string)));
		else if (PA_IS_OPTION(aP) && (aP->sort == PaReq))
			sprintf(optName, "%s", paFullName(aP, string, sizeof(string)));
		else if (PA_IS_PARAMETER(aP) && (aP->sort == PaOpt))
			sprintf(optName, "(%s)", aP->description);
		else if (PA_IS_PARAMETER(aP) && (aP->sort == PaReq))
			sprintf(optName, "[%s]", aP->description);
		else
			strcpy(optName, " ");

	
		/* 2. variable name */
		if (PA_IS_VARIABLE(aP))
			paEnvName(aP, varName, sizeof(varName));
		else
			strcpy(varName, " ");


		/* 3. Limits */
		if ((aP->type != PaSList) && (aP->type != PaIList))
		{
			char valWithDef[128];
			char fromN[64];
			
			getApVals(aP, defVal, minVal, maxVal, realVal);

			if (aP->hasDefault == false)
				sprintf(valWithDef, "%s", realVal);
			else
				sprintf(valWithDef, "%s /%s/", realVal, defVal);

			if ((aP->hasMinLimit == false) && (aP->hasMaxLimit == false))
				sprintf(vals, "%s", valWithDef);
			else if (aP->hasMinLimit == false)
				sprintf(vals, "%s <= %s", valWithDef, maxVal);
			else if (aP->hasMaxLimit == false)
				sprintf(vals, "%s >= %s", valWithDef, minVal);
			else
				sprintf(vals, "%s <= %s <= %s",
						minVal, valWithDef, maxVal);

			sprintf(from, "  (%s)", paFromName(aP, fromN, sizeof(fromN)));
		}
		else
		{
			sprintf(vals, " ");
			sprintf(from, " ");
		}
	
		sprintf(s, format, (firstLine)? progNAME : spacePad, optName, varName, vals, from);
		strncat(paResultString, s, sizeof(paResultString) - 1);
	
		firstLine = false;
	}
	// paLogOn = false;

	strncat(paResultString, "\r", sizeof(paResultString) - 1);
    
	free(spacePad);

	printf("%s\n", paResultString);
	exit(1);
}



typedef struct HelpVar
{
	const char*  varName;
	char*        varP;
} HelpVar;


static char usageString[800 * 200];
/* ****************************************************************************
*
* helpVar - 
*/
HelpVar helpVar[] = 
{
    { "$PROGNAME",  progNameV      },
    { "$USER",      paUserName     },
    { "$PWD",       paPwd          },
    { "$USAGE",     usageString    },
    { "$COLUMNS",   paColumns      },
    { "$ROWS",      paRows         },
    { "$DISPLAY",   paDisplay      },
    { "$EDITOR",    paEditor       },
    { "$LANG",      paLang         },
    { "$PAGER",     paPager        },
    { "$PPID",      paPpid         },
    { "$PID",       paPid          },
    { "$PRINTER",   paPrinter      },
    { "$SHELL",     paShell        },
    { "$TERM",      paTerm         },
    { "$SYSTEM",    paSystem       },
    { "$VISUAL",    paVisual       }
};



/* ****************************************************************************
*
* paVersionPrint - 
*/
void paVersionPrint(void)
{
	if (paManVersion)
		printf("%s\n", paManVersion);
	else
		printf("No MAN version\n");

	if (paManCopyright)
		printf("%s\n\n", paManCopyright);
	else
		printf("No MAN Copyright\n");

	if (paManAuthor)
		printf("%s\n", paManAuthor);
	else
		printf("No MAN Author\n");
}



/* ****************************************************************************
*
* paManUsage - 
*/
static void paManUsage(void)
{
	PaiArgument*  aP;

	paIterateInit();

	while ((aP = paIterateNext(paiList)) != NULL)
	{
		if (aP->sort == PaHid)
			continue;
		if ((aP->what & PawOption) == 0)
		{
			printf("Skipping '%s' (what == %d)\n", aP->option, aP->what);
			continue;
		}

		printf("  %-20s %s\n", aP->option, aP->description);
	}
}



/* ****************************************************************************
*
* paManHelp - 
*/
static void paManHelp(void)
{
	paExitOnUsage = false;

	if (paManSynopsis)
		printf("Usage: %s %s\n", paProgName, paManSynopsis);
	else
		printf("Usage: %s <no synopsis available>\n", paProgName);

	if (paManShortDescription)
		printf("%s\n", paManShortDescription);
	else
		printf("%s (short description) ...\n", paProgName);

	paManUsage();

	if (paManDescription)
		printf("%s\n", paManDescription);
	else
		printf("<No man description available>\n");

	if (paManExitStatus)
		printf("Exit status:\n %s\n", paManExitStatus);
	else
		printf("Exit status:\n <no exit status available>\n");

	if (paManReportingBugs)
		printf("Report %s %s\n", paProgName, paManReportingBugs);
		
	fflush(stdout);
}



/* ****************************************************************************
*
* paHelp - print help text
*/
void paHelp(void)
{
	LM_ENTRY();

	paManHelp();
	exit(1);

#if 0
	pid_t  pid   = getpid();   

	sprintf(paPid, "%d", pid);

	memset(paResultString, 0, sizeof(paResultString));
	paExitOnUsage = false;
	paUsage();
	strncpy(usageString, paResultString, sizeof(usageString));
	memset(paResultString, 0, sizeof(paResultString));
	
	if (paHelpFile != NULL)
	{
		char  s[512];
		FILE* fP;
		char  line[1024];
		char  start[512];
		char  end[512];

		LM_T(LmtHelp, ("Got help file '%s'", paHelpFile));

		sprintf(s, "----- %s Help -----\n", progName);
		strncat(paResultString, s, sizeof(paResultString) - 1);

		if ((fP = fopen(paHelpFile, "r")) == NULL)
		{
			sprintf(s, "error opening help file '%s': %s", paHelpFile, strerror(errno));
			strncat(paResultString, s, sizeof(paResultString) - 1);
			return;
		}
	
		while (fgets(line, sizeof(line), fP) != NULL)
		{
			int ix;
			int changes;

			do
			{
				changes = 0;

				for (ix = 0; ix < (int) BA_VEC_SIZE(helpVar); ix++)
				{
					char* tmp;

					if ((tmp = strstr(line, helpVar[ix].varName)) != NULL)
					{
						if (helpVar[ix].varP == NULL)
							continue;

						LM_T(LmtHelp, ("found variable '%s'", helpVar[ix].varName));
						++changes;

						strncpy(end, &tmp[strlen(helpVar[ix].varName)], sizeof(end));
						*tmp = 0;
						strncpy(start, line, sizeof(start));
						sprintf(line, "%s%s%s", start, helpVar[ix].varP, end);
					}
				}
			} while (changes != 0);

			strncat(paResultString, line, sizeof(paResultString) - 1);
		}

		fclose(fP);
		strncat(paResultString, 
				"\n---------------------------------------------\n",
				sizeof(paResultString) - 1);
	}
	else
		paUsage();
#endif
}
