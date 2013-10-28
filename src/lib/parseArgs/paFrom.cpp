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
#include <stdio.h>              /* stderr, stderr, ...                       */
#include <string.h>             /* strncmp, strspn, ...                      */
#include <stdlib.h>             /* strtol, ...                               */

#include "baStd.h"              /* BA standard header file                   */
#include "logMsg/logMsg.h"             /* LM_ENTRY, LM_EXIT, ...                    */

#include "parseArgs/parseArgs.h"          /* PaArgument                                */
#include "paPrivate.h"          /* PafDefault, ...                           */
#include "paIterate.h"          /* paIterateInit, paIterateNext              */
#include "paEnvVals.h"          /* paEnvName                                 */
#include "paFrom.h"             /* Own interface                             */



/* ****************************************************************************
*
* paFromName - 
*/
char* paFromName(PaiArgument* aP, char* out)
{
	switch (aP->from)
	{
	case PafError:        strcpy(out, "error");                 return out;
	case PafUnchanged:    strcpy(out, "not altered");           return out;
	case PafDefault:      strcpy(out, "default value");         return out;
	case PafEnvVar:       strcpy(out, "environment variable");  return out;
	case PafRcFile:       strcpy(out, "RC file");               return out;
	case PafArgument:     strcpy(out, "command line argument"); return out;
	}

	strcpy(out, "Yes, from where?");
	return out;
}



/* ****************************************************************************
*
* paFrom - from where did the value come?
*/
char* paFrom(PaiArgument* paList, const char* name)
{
	PaiArgument* aP;

	paIterateInit();

	while ((aP = paIterateNext(paList)) != NULL)
	{
		char envVarName[128];

		paEnvName(aP, envVarName);

		if ((aP->option) && strcmp(aP->option, name) == 0)
			break;
		if ((aP->envName) && strcmp(envVarName, name) == 0)
			break;
	}

	if (aP->type == PaLastArg)
		return (char*) "unrecognized option";

	switch (aP->from)
	{
	case PafError:        return (char*) "error";
	case PafUnchanged:    return (char*) "not altered";
	case PafDefault:      return (char*) "default value";
	case PafEnvVar:       return (char*) "environment variable";
	case PafRcFile:       return (char*) "RC file";
	case PafArgument:     return (char*) "command line argument";
	}

	return (char*) "origin unknown";
}



