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
* Author: Ken Zangelin
*/
#include <stdio.h>              /* stderr, stdout, ...                       */
#include <string.h>             /* strdup, strncmp                           */
#include <stdlib.h>             /* atoi                                      */

#include "baStd.h"              /* BA standard header file                   */
#include "logMsg/logMsg.h"      /* lmVerbose, lmDebug, ...                   */

#include "parseArgs.h"          /* PaArgument                                */
#include "paPrivate.h"          /* PaTypeUnion, config variables, ...        */
#include "paTraceLevels.h"      /* LmtPaEnvVal, ...                          */
#include "paLog.h"              /* PA_XXX                                    */
#include "paIterate.h"          /* paIterateInit, paIterateNext              */
#include "paGetVal.h"           /* paGetVal                                  */
#include "paIsOption.h"         /* paIsOption                                */
#include "paWarning.h"          /* paWaringInit, paWarningAdd                */
#include "paFullName.h"         /* paFullName                                */
#include "paUsage.h"            /* paUsage, paExtendedUsage                  */
#include "paConfig.h"           /* paMsgsToStdout, paMsgsToStderr            */
#include "paBuiltin.h"          /* paUsageVar, paEUsageVar, paHelpVar        */
#include "paOptionsParse.h"     /* Own interface                             */



/* ****************************************************************************
*
* stricts - argument for function argFind
*
* STRICT   - find argument strictly
* UNSTRICT - find argument unstrictly
*/
#define STRICT   1
#define UNSTRICT 2



/* ****************************************************************************
*
* REQUIRE - error handling for option requiring value
*/
#define REQUIRE(aP)                                                       \
do                                                                        \
{                                                                         \
	char e[80];                                                           \
	char w[512];                                                          \
                                                                          \
	sprintf(w, "%s requires %s", paFullName(e, aP), require(aP->type));   \
	PA_WARNING(PasMissingValue, w);                                       \
} while (0)



/* ****************************************************************************
*
* require - translate option type to string format
*
*/
static char* require(PaType type)
{
	switch(type)
	{
	case PaString:  return (char*) "a string";                    break;
	case PaInt:     return (char*) "an int";                      break;
	case PaIntU:    return (char*) "an unsigned int";             break;
	case PaInt64:   return (char*) "a 64-bit integer";            break;
	case PaIntU64:  return (char*) "a 64-bit unsigned integer";   break;
	case PaShort:   return (char*) "a short";                     break;
	case PaFloat:   return (char*) "a float";                     break;
	case PaDouble:  return (char*) "a double";                    break;
	case PaShortU:  return (char*) "an unsigned short";           break;
	case PaChar:    return (char*) "a char";                      break;
	case PaCharU:   return (char*) "an unsigned char";            break;
	default:        return (char*) "<yes, what?>";                break;
	}
}



/* ****************************************************************************
*
* argFind - search the slot in the argument list corresponding to 'string'
*/
static PaiArgument* argFind
(
	PaiArgument*  paList,
	char*         string,
	int           strict,
	int*          parNoP
)
{
	PaiArgument*  aP;
	PaiArgument*  foundP       = NULL;
	int           foundNameLen = 0;

	LM_ENTRY();

	paIterateInit();
	PA_M(("----- Looking up '%s' -----", string));
	while ((aP = paIterateNext(paList)) != NULL)
	{	
		if (parNoP == NULL)
		{
			int len;

			PA_M(("Got option '%s' from itrration", aP->option));
			if ((aP->option == NULL) || (aP->option[0] == 0))
				continue;
			
			PA_M(("comparing '%s' to '%s'", string, aP->option));

			len = strlen(aP->option);
			if (strict == STRICT)
				len = MAX(strlen(string), (unsigned int) len);

			if (len == 0)
			{
				LM_EXIT();
				return NULL;
			}

			if (strncmp(aP->option, string, len) == 0)
			{
				if (foundP == NULL)
				{
					foundP = aP;
					foundNameLen = strlen(aP->option);
				}
				else if (strlen(aP->option) > (unsigned int) foundNameLen)
				{
					foundP = aP;
					foundNameLen = strlen(aP->option);
				}
			}
		}
		else if ((aP->what & PawParameter) == PawParameter)
		{
			if (aP->aux != 0)
				PA_W(("cant use this parameter"));
			else
			{
				aP->aux = 1;
				LM_EXIT();				
				return aP;
			}
		}
		else
			PA_M(("skipping option '%s'", aP->option));
	}
	
	PA_M(("----- returning foundP ... -----"));

	LM_EXIT();
	return foundP;
}



/* ****************************************************************************
*
* iListFix - 
*
* NOTE
* This function destroys its input string 's'.
*/
static int iListFix(int* iV, char* s, int* error)
{
	char* tmP   = s;
	char* endP  = &s[strlen(s)];
	int   ix    = 0;

	PA_M(("incoming list: '%s'", s));
	baWsStrip(s);

	while ((unsigned long) tmP < (unsigned long) endP)
	{
		tmP = strchr(s, ' ');
		if (tmP == NULL)
			tmP = strchr(s, '\t');
		if (tmP >= endP)
			break;

		if (tmP == NULL) /* last argument */
		{
			/* Check 's' for valid integer - reflect in *error parameter */
			iV[ix + 1] = baStoi(s);
			PA_M(("item %d in int-list: %d", ix + 1, iV[ix + 1]));
			++ix;
			iV[0] = ix;
			break;
		}

		*tmP = 0;

		/* Check 's' for valid integer - reflect in *error parameter */
		iV[ix + 1] = baStoi(s);
		s = &tmP[1];

		LM_T(LmtPaIList, ("item %d in int-list: %d", ix + 1, iV[ix + 1]));
		LM_T(LmtPaIList, ("rest: '%s'", s));
		++ix;

		baWsStrip(s);
	}

	LM_T(LmtPaIList, ("got %d items in int-list", ix));

	*error = PaOk;
	return ix;
}



/* ****************************************************************************
*
* sListFix - 
*/
static int sListFix(char** sV, char* s, int* error)
{
	char* tmP   = s;
	char* endP  = &s[strlen(s)];
	long   ix   = 0;

	LM_T(LmtPaSList, ("incoming list: '%s'", s));
	LM_T(LmtPaSList, ("list vector at %p", sV));
	baWsStrip(s);

	while ((unsigned long) tmP < (unsigned long) endP)
	{
		// 1. eat beginning whitespace
		while ((*tmP == ' ') || (*tmP == '\t'))
			++tmP;
		if (*tmP == 0)
			break;

		s = tmP;


		// 2. Now we're at the start of the string - find WS after it and NULL it (if needed)
		tmP = strchr(s, ' ');
		if (tmP)
		{
			*tmP = 0;
			++tmP;
		}


		// 3. We have an option - record it!
		++ix;
		sV[ix] = s;
		LM_T(LmtPaSList, ("Found item %d in string-list: '%s'", ix + 1, s));
		LM_T(LmtPaSList, ("rest: '%s'", s));

		// 4. Point 's' to end-of the recently found option (if needed)
		if (tmP)
		{
			s = tmP;
		}
		else
		{
			LM_T(LmtPaSList, ("A total of %d items in string-list", ix));
			sV[0] = (char*) ix;
			break;
		}
	}

#if 0
	while ((unsigned long) tmP < (unsigned long) endP)
	{
		tmP = strchr(s, ' ');
		if (tmP == NULL)
			tmP = strchr(s, '\t');
		if (tmP >= endP)
			break;

		if (tmP == NULL) /* last argument */
		{
			sV[ix + 1] = s;
			LM_T(LmtPaSList, ("item %d in string-list: '%s' (pointer at %p)", ix + 1, sV[ix + 1], sV[ix + 1]));
			++ix;
			sV[0] = (char*) ix;
			break;
		}

		//while ((*tmP == ' ') || (*tmP == '\t'))
		//   ++tmP;

		//if (*tmP == 0)
		//   break;

		*tmP = 0;
		if (s[0] == 0)
			continue;

		sV[ix + 1] = s;
		s = &tmP[1];

		LM_T(LmtPaSList, ("item %d in string-list: '%s'", ix + 1, sV[ix + 1]));
		++ix;
		LM_T(LmtPaSList, ("rest: '%s'", s));
	}
#endif

	LM_T(LmtPaSList, ("got %d items in string-list", ix));

	*error = PaOk;
	return ix;
}



/* ****************************************************************************
*
* optOrPar - 
*/
char* optOrPar(char* opt)
{
	if ((*opt == '-') || (*opt == '+'))
		return (char*) "option";
	else
		return (char*) "parameter";
}



/* ****************************************************************************
*
* paOptionsParse - 
*/
int paOptionsParse(PaiArgument* paList, char* argV[], int argC)
{
	PaiArgument*  aP;
	char*         valueP;
	int           argNo         = 0;
	int           param         = 1;
	bool          extendedUsage = false;
	char          w[512];

	LM_ENTRY();
	w[0] = 0;

	PA_M(("incoming arg list of %d args", argC));
	while (++argNo < argC)
	{
		char  e[80];
		char  o[80];
		int   eee;
		int*  eP;

		eP  = &eee;
		*eP = PaNotOk;

		PA_M(("Looking up '%s'", argV[argNo]));
		if ((aP = argFind(paList, argV[argNo], STRICT, NULL)) != NULL)
		{
			valueP = argV[++argNo];
			if (aP->type == PaBoolean)
				--argNo;
		}			
		else if ((aP = argFind(paList, argV[argNo], UNSTRICT, NULL)) != NULL)
			valueP = &argV[argNo][strlen(aP->option)];
		else if ((aP = argFind(paList, (char*) "", UNSTRICT, &param)) != NULL)
			valueP = argV[argNo];
		else
		{
			sprintf(w, "%s '%s' not recognized", optOrPar(argV[argNo]), argV[argNo]);
			PA_W(("Warning: '%s'", w));
			PA_WARNING(PasNoSuchOption, w);
			continue;
		}

		LM_T(LmtPaApVals, ("found option '%s'", aP->name));

		if (aP->varP == (void*) &paUsageVar)
		{
			memset(paResultString, 0, sizeof(paResultString));
			if (extendedUsage == false)
			{
				paUsage();
				return -2;
			}
		}
		else if (aP->varP == (void*) &paVersion)
		{
			paVersionPrint();
			exit(0);
		}
		else if (aP->varP == (void*) &paLogDir)
		{
            if (valueP != NULL)
            {

               strcpy(paLogDir, (char*) valueP);
               printf("log directory: '%s'\n", paLogDir);
            }
		}
		else if (aP->varP == (void*) &paEUsageVar)
			extendedUsage = true;
		else if (aP->varP == (void*) &paHelpVar)
		{
			memset(paResultString, 0, sizeof(paResultString));
			paHelp();
			return -2;
		}

		if (aP->used > 0)
		{
			if ((aP->type != PaSList) && (aP->type != PaIList) && (aP->what != PawParameter))
			{
				sprintf(w, "multiple use of %s", aP->name);
				PA_WARNING(PasMultipleOptionUse, w);
				continue;
			}
		}

		aP->from = PafArgument;


		if (aP->type == PaBoolean)
		{
			if (strlen(argV[argNo]) != strlen(aP->option))
			{
				char tmp[128];
				sprintf(w, "boolean option '%s' doesn't take parameters",
						paFullName(e, aP));
				/* PA_WARNING(PasValueToBooleanOption, w); */
				sprintf(tmp, "%c%s", argV[argNo][0], &argV[argNo][2]);
				LM_W(("Changing arg %d from '%s' to '%s'", argNo, argV[argNo], tmp));
				strcpy(argV[argNo], tmp);
				--argNo;
			}
			
			*((bool*) aP->varP) = 
			  (((bool) ((int) aP->def)) == true)? false : true;

			aP->used++;
			continue;
		}

		if ((argNo >= argC)	|| (paIsOption(paList, valueP) == true))
		{
			REQUIRE(aP);
			break;
		}

		paFullName(o, aP);

		switch (aP->type)
		{
		case PaInt:
		case PaIntU:
            *((int*) aP->varP) = (int) (long long) paGetVal(valueP, eP);
            if (*eP != PaOk)
                return -1;

			LM_T(LmtPaApVals, ("got value %d for %s", *((int*) aP->varP), aP->name)); 
			break;

		case PaInt64:
		case PaIntU64:
			*((long long*) aP->varP) = (long long) paGetVal(valueP, eP);
            if (*eP != PaOk)
                return -1;

			LM_T(LmtPaApVals, ("got value %ul for %s", *((long long*) aP->varP), aP->name)); 
			break;

		case PaChar:
		case PaCharU:
            *((char*)  aP->varP) = (char) (long long) paGetVal(valueP, eP);
            if (*eP != PaOk)
                return -1;

			LM_T(LmtPaApVals, ("got value %d for %s", *((char*) aP->varP), aP->name)); 
			break;

		case PaShort:
		case PaShortU:
            *((short*) aP->varP) = (short) (long long) paGetVal(valueP, eP);
            if (*eP != PaOk)
                return -1;

            LM_T(LmtPaApVals, ("got value %d for %s", *((short*) aP->varP), aP->name));
            break;

		case PaFloat:
			*((float*) aP->varP) = baStof(valueP);
			*eP = PaOk;
			LM_T(LmtPaApVals, ("got value %f for %s", *((float*) aP->varP), aP->name));
			break;

		case PaDouble:
			*((double*) aP->varP) = baStof(valueP);
			*eP = PaOk;
			LM_T(LmtPaApVals, ("got value %f for %s", *((double*) aP->varP), aP->name));
			break;

		case PaString:
			strcpy((char*) aP->varP, valueP);
			LM_T(LmtPaApVals, ("got value '%s' for %s", (char*) aP->varP, aP->name));
			*eP = PaOk;
			break;

		case PaIList:
			LM_T(LmtPaIList, ("setting list '%s' to var %s", valueP, aP->name));
			iListFix((int*) aP->varP, valueP, eP);
			break;

		case PaSList:
			LM_T(LmtPaSList, ("setting list '%s' to var %s", valueP, aP->name));
			sListFix((char**) aP->varP, valueP, eP);
			break;

		case PaBoolean:
		case PaLastArg:
			LM_T(LmtPaApVals, ("PaList, PaBoolean, PaLastArg ..."));
			*eP = PaOk;			
			break;

		default:
			LM_W(("bad type for option '%s'", aP->name));
		}

		if (*eP != PaOk)
			REQUIRE(aP);

		aP->used++;
		LM_V(("%s used %d times", aP->name, aP->used));
	}


	/* checking that all required arguments are set */
	paIterateInit();
	while ((aP = paIterateNext(paList)) != NULL)
	{
		if ((aP->sort == PaReq) && (aP->used == 0))
		{
			sprintf(w, "%s required", aP->name);
			PA_WARNING(PasRequiredOption, w);
		}
	}		

	if (extendedUsage == true)
	{
		paExtendedUsage();
		return -2;
	}

	LM_EXIT();
	return 0;
}
