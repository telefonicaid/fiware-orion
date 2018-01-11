#ifndef SRC_LIB_PARSEARGS_PACONFIG_H_
#define SRC_LIB_PARSEARGS_PACONFIG_H_

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
#include "parseArgs/baStd.h"      /* bool, ...                              */
#include "parseArgs/parseArgs.h"  /* Where we have the ext decl of paConfig */



#ifndef DEFAULT_VERSION
#  define DEFAULT_VERSION "alpha"
#endif

#ifndef DEFAULT_AUTHOR
#  define DEFAULT_AUTHOR "Telefonica I+D"
#endif

#ifndef DEFAULT_COPYRIGHT
    #define DEFAULT_COPYRIGHT "Copyright 2013-2018 Telefonica Investigacion y Desarrollo, S.A.U\n" \
                              "Orion Context Broker is free software: you can redistribute it and/or\n" \
                              "modify it under the terms of the GNU Affero General Public License as\n" \
                              "published by the Free Software Foundation, either version 3 of the\n" \
                              "License, or (at your option) any later version.\n" \
                              "\n"\
                              "Orion Context Broker is distributed in the hope that it will be useful,\n" \
                              "but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
                              "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero\n" \
                              "General Public License for more details."
#endif



/* ****************************************************************************
*
* Configurable variables
*/
extern bool       paUseBuiltins;
extern bool       paExitOnError;
extern bool       paExitOnUsage;
extern bool       paPrintErrorsOnStderr;
extern char*      paBuiltinPrefix;
extern char*      paPrefix;
extern char*      paRcFileName;
extern char*      paRcFileDir;
extern char*      paGenericRcDir;
extern char*      paProgName;
extern char       paTraceV[1024];

extern char*      paHelpFile;
extern char*      paHelpText;

extern char*      paManSynopsis;
extern char*      paManShortDescription;
extern char*      paManDescription;
extern char*      paManExitStatus;
extern char*      paManReportingBugs;

extern char*      paManCopyright;
extern char*      paManVersion;
extern char*      paManAuthor;

extern bool       paUsageOnAnyWarning;

extern bool       paLogToFile;
extern bool       paLogToScreen;
extern bool       paLogScreenToStderr;
extern bool       paLogScreenOnlyErrors;
extern bool       paBoolWithValueIsUnrecognized;

extern char*      paLogFilePath;
extern char*      paLogFileLineFormat;
extern char*      paLogFileTimeFormat;
extern char*      paLogScreenLineFormat;
extern char*      paLogScreenTimeFormat;

extern char*      paTracelevels;

extern bool       paVerbose;
extern bool       paVerbose2;
extern bool       paVerbose3;
extern bool       paVerbose4;
extern bool       paVerbose5;
extern bool       paDebug;
extern bool       paToDo;
extern bool       paReads;
extern bool       paWrites;
extern bool       paFix;
extern bool       paBug;
extern bool       paBuf;
extern bool       paDoubt;

extern bool       paSilent;

extern bool       paMsgsToStdout;
extern bool       paMsgsToStderr;

extern char       paPid[16];




/* ****************************************************************************
*
* paConfig - 
*/
extern int paConfig(const char* item, const void* value, const void* value2 = (const void*) 0);



/* ****************************************************************************
*
* paConfigActions - 
*/
extern int paConfigActions(bool preTreat);



/* ****************************************************************************
*
* paConfigCleanup - 
*/
extern void paConfigCleanup(void);

#endif  // SRC_LIB_PARSEARGS_PACONFIG_H_
