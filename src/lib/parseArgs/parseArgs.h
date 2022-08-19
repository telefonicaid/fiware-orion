#ifndef SRC_LIB_PARSEARGS_PARSEARGS_H_
#define SRC_LIB_PARSEARGS_PARSEARGS_H_

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
#include <string>

#include "parseArgs/baStd.h"           /* bool                               */
#include "parseArgs/paConfig.h"        /* paConfig                           */



/* ****************************************************************************
*
* PaNoLimit - limit not to be checked
*/
#define PaNoLim  -20
#define PaNL     PaNoLim



/* ****************************************************************************
*
* PaNoDefault - no default value set
*/
#define PaNoDef  (int) -19
#define PaND     PaNoDef



/* ****************************************************************************
*
* PA_END_OF_ARGS - the last of the arguments in the option vector
*/
#define PA_REST         PafUnchanged, " ", 0, 0, false, false, false, false, 0
#define PA_END_OF_ARGS  { "", NULL, "NADA", PaLastArg, PaReq, 0, 0, 0, "" }



/* ****************************************************************************
*
* PaFrom - from where did the argument come?
*/
typedef enum PaFrom
{
  PafError     = -1,
  PafUnchanged = 1,
  PafDefault,
  PafRcFile,
  PafEnvVar,
  PafArgument
} PaFrom;



/* ****************************************************************************
*
* PaSeverity -
*/
typedef enum PaSeverity
{
  PasNone = 0,
  PasLogFile,

  PasBuiltinRemove,
  PasNoSuchBooleanValue,
  PasNoSuchVariable,
  PasNoSuchCommand,
  PasNoSuchType,
  PasNoSuchOption,
  PasNoSuchOptType,

  PasBadValue,
  PasValueToBooleanOption,
  PasLimitError,
  PasMultipleOptionUse,
  PasRequiredOption,

  PasUnidentified,
  PasVarNameWs,
  PasMissingValue,
  PasParseError,
  PasProgrammingError,
  PasSystemError, /* malloc, ... */
  PasSetup,
  PasOptionNameDuplicated,
  PasEnvNameDuplicated
} PaSeverity;



/* ****************************************************************************
*
* PaWarning
*/
typedef struct PaWarning
{
  PaSeverity severity;
  char*      string;
} PaWarning;



/* ****************************************************************************
*
* PavCb - callback function for config variables
*/
typedef void (*PavCb)(char* varName, int value);



/* ****************************************************************************
*
* PaType - type of the option
*/
typedef enum PaType
{
  PaBoolean = 0,  PaBool    = PaBoolean,
  PaString,       PaStr     = PaString,
  PaShort,        PaSh      = PaShort,
  PaShortU,       PaShU     = PaShortU,    PaUShort  = PaShortU,
  PaChar,         PaCh      = PaChar,
  PaCharU,        PaChU     = PaCharU,     PaUChar   = PaCharU,
  PaInt,          PaInt32   = PaInt,
  PaIntU,         PaUInt    = PaIntU,
  PaInt64,        PaLong    = PaInt64,
  PaIntU64,       PaULong   = PaIntU64,    PaUInt64  = PaIntU64,
  PaFloat,
  PaDouble,
  PaIList,
  PaSList,
  PaLastArg
} PaType;



/* ****************************************************************************
*
* PaSort - sort of option
*/
typedef enum PaSort
{
  PaOpt,
  PaReq,
  PaHid,    /* Always Optional */
} PaSort;



/* ****************************************************************************
*
* PaValue -
*/
typedef union PaValue
{
  int               i;
  unsigned int      ui;
  int16_t           s;
  uint16_t          us;
  char              c;
  uint8_t           uc;
  char*             cP;
  float             f;
  double            d;
  char**            cPP;
} PaValue;



/* ****************************************************************************
*
* PaArgument - arguments for the parse argument utility
*/
typedef struct PaArgument
{
  const char*  option;
  void*        varP;
  const char*  envName;
  PaType       type;
  PaSort       sort;
  int64_t      def;
  int64_t      min;
  int64_t      max;
  const char*  description;
} PaArgument;


typedef struct PaiArgument
{
  const char*  option;
  void*        varP;
  const char*  envName;
  PaType       type;
  PaSort       sort;
  int64_t      def;
  int64_t      min;
  int64_t      max;
  const char*  description;

  /* The following fields are set initially by paParse */
  PaFrom       from;
  char         name[64];
  int          used;
  int          what;   /* option, variable, parameter, ... */
  bool         hasDefault;
  bool         hasMinLimit;
  bool         hasMaxLimit;
  bool         isBuiltin;
  bool         removed;
  bool         includeInUsage;
  int          aux;
} PaiArgument;



/* ****************************************************************************
*
* _i - to typecast values in the PaArgument vector
*
*/
#define _i (int64_t)



/* ****************************************************************************
*
* variables
*/
extern char*      progName;
extern char*      paUsageProgName;
extern char       paCommandLine[1024];
extern PaWarning  paWarning[];
extern int        paWarnings;
extern char       paResultString[60000];



/* ****************************************************************************
*
* paParse -
*/
extern int paParse
(
  PaArgument*  paArgV,
  int          argC,
  char*        argV[],
  int          levels,
  bool         pid,
  const char*  extra = (const char*) 0
);



/* ****************************************************************************
*
* paCleanup - free allocated variables
*/
extern void paCleanup(void);



/* ****************************************************************************
*
* paConfig -
*/
// extern int paConfig(const char* item, const void* value, const void* value2 = NULL);



/* ****************************************************************************
*
* paLmSdGet
*/
extern int paLmSdGet(void);



/* ****************************************************************************
*
* paLmFdGet
*/
extern int paLmFdGet(void);

#endif  // SRC_LIB_PARSEARGS_PARSEARGS_H_
