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
#include "logMsg/logMsg.h"             /* lmVerbose, lmDebug, ...                   */

#include "parseArgs/parseArgs.h"          /* Own interface                             */
#include "paPrivate.h"          /* PaTypeUnion, config variables, ...        */
#include "paTraceLevels.h"      /* LmtPaEnvVal, ...                          */
#include "paWarning.h"          /* paWaringInit, paWarningAdd                */
#include "paFrom.h"             /* paFrom                                    */
#include "paIterate.h"          /* paIterateInit, paIterateNext              */
#include <cstdlib>				/* C++ free(.)								 */



/* ****************************************************************************
*
* getSpaces - 
*/
static char* getSpaces(int spaces, char* s)
{
	int ix = 0;

	while (--spaces >= 0)
		s[ix++] = ' ';
	s[ix] = 0;

	return s;
}



/* ****************************************************************************
*
* limits - check limits for an option
*/
static int limits(PaiArgument* paList, PaiArgument* aP)
{
	char                valS[80];
	bool                upper = true;
	bool                lower = true;
	char                w[512];
	long long           i64Val;
	int                 iVal;
	short               sVal;
	char                cVal;
	float               fVal;
	double              dVal;
	unsigned long long  ui64Val;
	unsigned int        uiVal;
	unsigned short      usVal;
	unsigned char       ucVal;

	if ((aP->type == PaSList) || (aP->type == PaIList) || (aP->type == PaBoolean))
		return 0;

	if (aP->min == aP->max)
	{
		LM_T(LmtPaLimits, ("'%s': limits equal - no limit check", aP->name));
		lower = false;
		upper = false;
	}
	else if (aP->min == PaNoLim)
	{
		LM_T(LmtPaLimits, ("'%s': no lower limit check", aP->name));
		lower = false;
	}
	else if (aP->max == PaNoLim)
	{
		LM_T(LmtPaLimits, ("'%s': no upper limit check", aP->name));
		upper = false;
	}

	if ((upper == false) && (lower == false))
		return 0;

	LM_T(LmtPaLimits, ("limit check for %s", aP->name));

	w[0]    = 0;
	i64Val  = *((long long*)            aP->varP);
	iVal    = *((int*)                  aP->varP);
	sVal    = *((short*)                aP->varP);
	cVal    = *((char*)                 aP->varP);
	fVal    = *((float*)                aP->varP);
	dVal    = *((double*)               aP->varP);
	uiVal   = *((unsigned int*)         aP->varP);
	ui64Val = *((unsigned long long*)   aP->varP);
	usVal   = *((unsigned short*)       aP->varP);
	ucVal   = *((unsigned char*)        aP->varP);
	
	switch (aP->type)
	{
	case PaString:
		if (lower && (strcmp((char*) aP->varP, (char*) aP->min) < 0))
		{
			LM_E(("low limit error for %s (strcmp(\"%s\", \"%s\")')", aP->name,
				  aP->varP, aP->min));
			sprintf(valS, "%s", (char*) aP->varP);
			sprintf(w, "value(\"%s\") < minimum(\"%s\") (%s)",
					(char*) aP->varP, 
					(char*) aP->min,
					aP->option? paFrom(paList, (char*) aP->option): "parameter");
		}
		else if (upper && (strcmp((char*) aP->varP, (char*) aP->max) > 0))
		{
			LM_E(("high limit error for %s", aP->name));
			sprintf(valS, "%s", (char*) aP->varP);
			sprintf(w, "value(\"%s\") > maximum(\"%s\") (%s)",
					(char*) aP->varP, 
					(char*) aP->max,
					aP->option? paFrom(paList, (char*) aP->option) : "parameter");
		}
		break;

	case PaInt:
		LM_T(LmtPaLimits, ("checking '%s' (value %d): limits '%d' - '%d'",
						   aP->name, iVal, aP->min, aP->max));
		if ((lower && (iVal < (int) aP->min))
		||  (upper && (iVal > (int) aP->max)))
		{
			sprintf(valS, "%d", iVal);
			sprintf(w, "%d <= %d <= %d (%s)",
                    (int) aP->min,
					(int) iVal,
					(int) aP->max,
					aP->option? paFrom(paList, (char*) aP->option) : "parameter");
		}
		break;
		
	case PaIntU:
		LM_T(LmtPaLimits, ("checking '%s' (value %d): limits '%d' - '%d'",
						   aP->name, uiVal, aP->min, aP->max));
		if ((lower && (uiVal < (unsigned int) aP->min))
		||  (upper && (uiVal > (unsigned int) aP->max)))
		{
			sprintf(valS, "%u", uiVal);
			sprintf(w, "%u <= %u <= %u (%s)",
					(int) aP->min,
					(int) uiVal,
					(int) aP->max,
					aP->option? paFrom(paList, (char*) aP->option) : "parameter");
		}
		break;
		
	case PaInt64:
		LM_T(LmtPaLimits, ("checking '%s' (value %lld): limits '%d' - '%d'",
						   aP->name, i64Val, aP->min, aP->max));
		if ((lower && (i64Val < aP->min))
		||  (upper && (i64Val > aP->max)))
		{
			sprintf(valS, "%lld", i64Val);
			sprintf(w, "%lld <= %lld <= %lld (%s)",
					aP->min,
					i64Val,
					aP->max,
					aP->option? paFrom(paList, (char*) aP->option) : "parameter");
		}
		break;
		
	case PaIntU64:
		LM_T(LmtPaLimits, ("checking '%s' (value %llu): limits '%d' - '%d'",
						   aP->name, ui64Val, aP->min, aP->max));
		if ((lower && (uiVal < (unsigned long long) aP->min))
		||  (upper && (uiVal > (unsigned long long) aP->max)))
		{
			sprintf(valS, "%llu", ui64Val);
			sprintf(w, "%llu <= %llu <= %llu (%s)",
					aP->min,
					ui64Val,
					aP->max,
					aP->option? paFrom(paList, (char*) aP->option) : "parameter");
		}
		break;
		
	case PaShort:
		LM_T(LmtPaLimits, ("checking '%s' (value %d): limits '%d' - '%d'",
						   aP->name, sVal, aP->min, aP->max));
		if ((lower && (sVal < (short) aP->min))
		||  (upper && (sVal > (short) aP->max)))
		{
			sprintf(valS, "%d", sVal);
			sprintf(w, "%d <= %d <= %d (%s)",
					(short) aP->min,
					sVal,
					(short) aP->max,
					aP->option? paFrom(paList, (char*) aP->option) : "parameter");
		}
		break;
		
	case PaShortU:
		LM_T(LmtPaLimits, ("checking '%s' (value %d): limits '%d' - '%d'",
						   aP->name, usVal, aP->min, aP->max));
		if ((lower && (usVal < (unsigned short) aP->min))
		||  (upper && (usVal > (unsigned short) aP->max)))
		{
			sprintf(valS, "%u", usVal);
			sprintf(w, "%u <= %u <= %u (%s)",
					(short) aP->min,
					usVal,
					(short) aP->max,
					aP->option? paFrom(paList, (char*) aP->option) : "parameter");
		}
		break;
		
	case PaChar:
		LM_T(LmtPaLimits, ("checking '%s' (value %d): limits '%d' - '%d'",
						   aP->name, cVal, aP->min, aP->max));
		if ((lower && (cVal < (char) aP->min))
		||  (upper && (cVal > (char) aP->max)))
		{
			sprintf(valS, "%d", cVal);
			sprintf(w, "%d <= %d <= %d (%s)",
					(char) aP->min,
					cVal,
					(char) aP->max,
					aP->option? paFrom(paList, (char*) aP->option) : "parameter");
		}
		break;
		
	case PaCharU:
		LM_T(LmtPaLimits, ("checking '%s' (value %d): limits '%d' - '%d'",
						   aP->name, ucVal, aP->min, aP->max));
		if ((lower && (ucVal < (unsigned char) aP->min))
		||  (upper && (ucVal > (unsigned char) aP->max)))
		{
			sprintf(valS, "%u", ucVal);
			sprintf(w, "%u <= %u <= %u (%s)",
					(char) aP->min,
					ucVal,
					(char) aP->max,
					aP->option? paFrom(paList, (char*) aP->option) : "parameter");
		}
		break;
		
	case PaFloat:
		LM_T(LmtPaLimits, ("checking '%s' (value %d): limits '%f' - '%f'",
						   aP->name, fVal, aP->min, aP->max));
		if ((lower && (fVal < (float) aP->min))
		||  (upper && (fVal > (float) aP->max)))
		{
			sprintf(valS, "%f", fVal);
			sprintf(w, "%f <= %f <= %f (%s)",
                    (float) aP->min,
                    fVal,
                    (float) aP->max,
					aP->option? paFrom(paList, (char*) aP->option) : "parameter");
		}
		break;
		
	case PaDouble:
		LM_T(LmtPaLimits, ("checking '%s' (value %d): limits '%f' - '%f'",
						   aP->name, dVal, aP->min, aP->max));
		if ((lower && (dVal < (double) aP->min))
		||  (upper && (dVal > (double) aP->max)))
		{
			sprintf(valS, "%f", dVal);
			sprintf(w, "%f <= %f <= %f (%s)",
					(double) aP->min,
					dVal,
					(double) aP->max,
					aP->option? paFrom(paList, (char*) aP->option) : "parameter");
		}
		break;
		
	default:
		sprintf(w, "type %d unknown for %s", aP->type, aP->name);
		PA_WARNING(PasNoSuchOptType, w);
		return 0;
	}

	if (w[0] != 0)
	{
		char  spaces[80];
		int   len = strlen(progName) + 2;
		char  eString[1024];

		sprintf(eString, "%s: value %s for %s not within limits\n%s%s",
				progName,
				valS,
				aP->name,
				getSpaces(len, spaces),
				w);
		PA_WARNING(PasLimitError, eString);
	}
	
	return 0;
}



/* ****************************************************************************
*
* paLimitCheck - check limits for all options
*/
int paLimitCheck(PaiArgument* paList)
{
	PaiArgument* aP;

	LM_ENTRY();

	paIterateInit();
	while ((aP = paIterateNext(paList)) != NULL)
	{
		if (limits(paList, aP) == -1)
		{
			LM_EXIT();
			return -1;
		}
	}

	LM_EXIT();
	return 0;
}



