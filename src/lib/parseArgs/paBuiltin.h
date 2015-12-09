#ifndef SRC_LIB_PARSEARGS_PABUILTIN_H_
#define SRC_LIB_PARSEARGS_PABUILTIN_H_

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
#include "parseArgs/parseArgs.h"         /* PaArgument                                 */



/* ****************************************************************************
*
* builtin option variables
*/
extern char            paHome[512];
extern bool            paNoClear;
extern bool            paClearAt;
extern bool            paAssertAtExit;
extern int             paKeepLines;
extern int             paLastLines;
extern bool            paLogAppend;
extern bool            paUsageVar;
extern bool            paEUsageVar;
extern bool            paHelpVar;
extern bool            paVersion;
extern char            paUserName[64];
extern char            paPwd[512];
extern char            paColumns[128];
extern char            paRows[128];
extern char            paDisplay[128];
extern char            paEditor[128];
extern char            paLang[128];
extern char            paPager[128];
extern char            paPpid[128];
extern char            paPrinter[128];
extern char            paShell[128];
extern char            paTerm[128];
extern char            paSystem[128];
extern char            paVisual[128];
extern char            paLogDir[256];
extern char            paLogLevel[256];



/* ****************************************************************************
*
* paBuiltin - vector of builtin
*/
extern PaiArgument  paBuiltin[];
extern int          paBuiltins;



/* ****************************************************************************
*
* paBuiltinNoOf - 
*/
extern int paBuiltinNoOf(void);



/* ****************************************************************************
*
* paBuiltinLookup - 
*/
extern PaiArgument* paBuiltinLookup(char* option);



/* ****************************************************************************
*
* paBuiltinRemove
*/
extern int paBuiltinRemove(char* name);

#endif  // SRC_LIB_PARSEARGS_PABUILTIN_H_
