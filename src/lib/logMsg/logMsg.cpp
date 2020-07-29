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
#define F_OK 0

#include <semaphore.h>          /* sem_init, sem_wait, sem_post              */
#include <errno.h>
#include <sys/types.h>          /* types needed for other includes           */
#include <stdio.h>              /*                                           */
#include <unistd.h>             /* getpid, write, ...                        */
#include <sys/syscall.h>        /* gettid() not working, trying with syscall */
#include <string.h>             /* strncat, strdup, memset                   */
#include <stdarg.h>             /* va_start, va_arg, va_end                  */
#include <stdlib.h>             /* atoi                                      */
#include <stdint.h>             /* int64, ...                                */

#if !defined(__APPLE__)
#include <malloc.h>             /* free                                      */
#endif

#include <fcntl.h>              /* O_RDWR, O_TRUNC, O_CREAT                  */
#include <ctype.h>              /* isprint                                   */
#include <sys/stat.h>           /* fstat, S_IFDIR                            */
#include <sys/time.h>           /* gettimeofday                              */
#include <time.h>               /* time, gmtime_r, ...                       */
#include <sys/timeb.h>          /* timeb, ftime, ...                         */

#undef NDEBUG
#include <assert.h>
#include <string>

#include "logMsg/time.h"
#include "logMsg/logMsg.h"      /* Own interface                             */

#include "common/limits.h"      // FIXME: this should be removed if this library wants to be generic again

extern "C" pid_t gettid(void);



/* ****************************************************************************
*
* INIT_CHECK -
*/
#define INIT_CHECK()                       \
do                                         \
{                                          \
  if (initDone == false)                   \
  {                                        \
    return LmsInitNotDone;                 \
  }                                        \
} while (0)



/* ****************************************************************************
*
* PROGNAME_CHECK -
*/
#define PROGNAME_CHECK()                   \
do                                         \
{                                          \
  if (progName == NULL)                    \
  {                                        \
    return LmsPrognameNotSet;              \
  }                                        \
} while (0)



/* ****************************************************************************
*
* INDEX_CHECK -
*/
#define INDEX_CHECK(index)                 \
do                                         \
{                                          \
  if ((index < 0) || (index >= FDS_MAX))   \
  {                                        \
    return LmsFdNotFound;                  \
  }                                        \
} while (0)



/* ****************************************************************************
*
* STRING_CHECK -
*/
#define STRING_CHECK(s, l)                 \
do                                         \
{                                          \
  if (s == NULL)                           \
  {                                        \
    return LmsNull;                        \
  }                                        \
                                           \
  if (strlen(s) >= l)                      \
  {                                        \
    return LmsStringTooLong;               \
  }                                        \
} while (0)



/* ****************************************************************************
*
* POINTER_CHECK -
*/
#define POINTER_CHECK(p)                   \
do                                         \
{                                          \
  if (p == NULL)                           \
  {                                        \
    return LmsNull;                        \
  }                                        \
} while (0)



/* ****************************************************************************
*
* NOT_OCC_CHECK -
*/
#define NOT_OCC_CHECK(i)                   \
do                                         \
{                                          \
  if (fds[i].state != Occupied)            \
  {                                        \
    return LmsFdNotFound;                  \
  }                                        \
} while (0)



/* ****************************************************************************
*
* LOG_OUT -
*/
#define LOG_OUT(s)



/* ****************************************************************************
*
* CHAR_ADD -
*/
#define CHAR_ADD(c, l)                     \
do                                         \
{                                          \
  char xin[3];                             \
                                           \
  xin[0] = c;                              \
  xin[1] = 0;                              \
  strncat(line, xin,                       \
          lineLen - strlen(line) - 1);     \
                                           \
  fi += l;                                 \
} while (0)



/* ****************************************************************************
*
* STRING_ADD -
*/
#define STRING_ADD(s, l)                          \
do                                                \
{                                                 \
  if (s != NULL)                                  \
  {                                               \
    strncat(line, s, lineLen - strlen(line) - 1); \
  }                                               \
  else                                            \
  {                                               \
    strncat(line, "noprogname", lineLen - strlen(line) - 1); \
  }                                               \
                                                  \
  fi += l;                                        \
} while (0)



/* ****************************************************************************
*
* INT_ADD -
*/
#define INT_ADD(i, l)                      \
do                                         \
{                                          \
  char xin[20];                            \
                                           \
  snprintf(xin, sizeof(xin), "%d", i);     \
  strncat(line, xin,                       \
          lineLen - strlen(line) - 1);     \
                                           \
  fi += l;                                 \
} while (0)



/* ****************************************************************************
*
* TLEV_ADD -
*/
#define TLEV_ADD(type, tLev)                      \
do                                                \
{                                                 \
  char xin[4];                                    \
                                                  \
  if ((type != 'T') && (type != 'X'))             \
  {                                               \
    strncpy(xin, "   ", sizeof(xin));             \
  }                                               \
  else                                            \
  {                                               \
    snprintf(xin, sizeof(xin), "%03d", tLev);     \
  }                                               \
                                                  \
  strncat(line, xin, lineLen - strlen(line) - 1); \
  fi += 4;                                        \
} while (0)



/* ****************************************************************************
*
* DELIMITER -    a comma is used as delimiter in the trace level format string
* ADD -          value for adding levels
* SUB -          value for subtracting levels
* TRACE_LEVELS - number of trace levels
*/
#define DELIMITER        ','
#define ADD              0
#define SUB              1
#define TRACE_LEVELS     256
#define FDS_MAX          2
#define LM_LINE_MAX      (32 * 1024)
#define TEXT_MAX         512
#define FORMAT_LEN       1024
#define FORMAT_DEF       "TYPE:DATE:TID:EXEC/FILE[LINE] FUNC: TEXT"
#define DEF1             "TYPE:EXEC/FUNC: TEXT"
#define TIME_FORMAT_DEF  "%A %d %h %H:%M:%S %Y"
#define F_LEN            200
#define TF_LEN           64
#define INFO_LEN         256
#define TMS_LEN          20
#define TME_LEN          20
#define TMS_DEF          "<trace "
#define TME_DEF          ">"
#define AUX_LEN          16
#define LOG_PERM         0666
#define LOG_MASK         0



/* ****************************************************************************
*
* FdState -
*/
typedef enum FdState
{
  Free = 0,
  Occupied
} FdState;



/* ****************************************************************************
*
* Type -
*/
typedef enum Type
{
  Stdout,
  Fichero
} Type;




/* ****************************************************************************
*
* Fds - file descriptors of log files
*/
typedef struct Fds
{
  int        fd;                   /* file descriptor                      */
  int        type;                 /* stdout or file                       */
  int        state;                /* Free or Occupied                     */
  char       format[F_LEN];        /* Output format                        */
  char       timeFormat[TF_LEN];   /* Output time format                   */
  char       info[INFO_LEN];       /* fd info (path, stdout, ...)          */
  char       tMarkStart[TMS_LEN];  /* Start string for trace at EOL        */
  char       tMarkEnd[TME_LEN];    /* End string for trace at EOL          */
  LmWriteFp  write;                /* Other write function                 */
  bool       traceShow;            /* show trace level at EOL (if LM_T)    */
  bool       onlyErrorAndVerbose;  /* No tracing to stdout                 */
} Fds;



/* ****************************************************************************
*
* Line -
*/
typedef struct Line
{
  char type;
  char remove;
} Line;



/******************************************************************************
*
* globals
*/
int             inSigHandler                      = 0;
char*           progName;                         /* needed for messages (and by lmLib) */
char            progNameV[512];                   /* where to store progName            */
__thread char   transactionId[64]                 = "N/A";
__thread char   correlatorId[64]                  = "N/A";
__thread char   service[SERVICE_NAME_MAX_LEN + 1] = "N/A";
__thread char   subService[101]                   = "N/A";   // Using SERVICE_PATH_MAX_TOTAL will be too much
__thread char   fromIp[IP_LENGTH_MAX + 1]         = "N/A";



/* ****************************************************************************
*
* Static Variables
*/
static sem_t             sem;
static Fds               fds[FDS_MAX];
static bool              initDone;
static time_t            secondsAtStart;
static bool              tLevel[TRACE_LEVELS];
static char              aux[AUX_LEN];
static bool              lmOutHookActive        = false;
static void*             lmOutHookParam         = NULL;
static LmExitFp          exitFunction           = NULL;
static void*             exitInput              = NULL;
static LmWarningFp       warningFunction        = NULL;
static void*             warningInput           = NULL;
static LmErrorFp         errorFunction          = NULL;
static void*             errorInput             = NULL;
static LmOutHook         lmOutHook              = NULL;
static bool              auxOn                  = false;
static int               atLines                = 5000;
static int               keepLines              = 1000;
static int               lastLines              = 1000;
static int               logLines               = 0;
static bool              doClear                = false;
static int               fdNoOf                 = 0;
static LmTracelevelName  userTracelevelName     = NULL;
static int               lmSd                   = -1;



/* ****************************************************************************
*
* Global variables
*/
bool  lmDebug                      = false;
bool  lmHidden                     = false;
bool  lmVerbose                    = false;
bool  lmVerbose2                   = false;
bool  lmVerbose3                   = false;
bool  lmVerbose4                   = false;
bool  lmVerbose5                   = false;
bool  lmToDo                       = false;
bool  lmDoubt                      = false;
bool  lmReads                      = false;
bool  lmWrites                     = false;
bool  lmBug                        = false;
bool  lmBuf                        = false;
bool  lmFix                        = false;
int   lmLevelMask                  = 0xFFFFFFFF;  /* All "masked in" by default */
bool  lmAssertAtExit               = false;
LmxFp lmxFp                        = NULL;
bool  lmNoTracesToFileIfHookActive = false;
bool  lmSilent                     = false;
bool  lmPreamble                   = true;



/* ****************************************************************************
*
* lmSemInit -
*/
static void lmSemInit(void)
{
  sem_init(&sem, 0, 1);
}



/* ****************************************************************************
*
* lmSemGet - 
*/
const char* lmSemGet(void)
{
  int value;

  if (sem_getvalue(&sem, &value) == -1)
  {
    return "error";
  }

  if (value == 0)
  {
    return "taken";
  }

  return "free";
}



/* ****************************************************************************
*
* semTake -
*/
static void semTake(void)
{
  static int firstTime = 1;

  if (firstTime == 1)
  {
    lmSemInit();
    firstTime = 0;
  }

  sem_wait(&sem);
}



/* ****************************************************************************
*
* semGive -
*/
static void semGive(void)
{
  sem_post(&sem);
}



/* ****************************************************************************
*
* lmProgName -
*/
char* lmProgName(char* pn, int levels, bool pid, const char* extra)
{
  char*        start;
  static char  pName[512];

  if (pn == NULL)
  {
    return NULL;
  }

  if (levels < 1)
  {
    levels = 1;
  }

  start = &pn[strlen(pn) - 1];
  while (start > pn)
  {
    if (*start == '/')
    {
      levels--;
    }

    if (levels == 0)
    {
      break;
    }

    --start;
  }

  if (*start == '/')
  {
    ++start;
  }

  strncpy(pName, start, sizeof(pName));

  if (pid == true)
  {
    char  pid[8];
    strncat(pName, "_", sizeof(pName) - strlen(pName) - 1);
    snprintf(pid, sizeof(pid), "%d", (int) getpid());
    strncat(pName, pid, sizeof(pName) - strlen(pName) - 1);
  }

  if (extra != NULL)
  {
    strncat(pName, extra, sizeof(pName) - strlen(pName) - 1);
  }

  printf("pName: %s\n", pName);

  return pName;
}



/* ****************************************************************************
*
* lmLevelMaskSet - 
*/
void lmLevelMaskSet(int levelMask)
{
  lmLevelMask = levelMask;
}



/* ****************************************************************************
*
* lmLevelMaskSetString - 
*/
void lmLevelMaskSetString(char* level)
{
  if (strcasecmp(level, "NONE") == 0)
  {
    lmLevelMask = 0;
  }
  else if (strcasecmp(level, "FATAL") == 0)
  {
    lmLevelMask  = LogLevelExit;
  }
  else if (strcasecmp(level, "ERROR") == 0)
  {
    lmLevelMask  = LogLevelExit;
    lmLevelMask |= LogLevelError;
  }
  else if (strcasecmp(level, "WARN") == 0)
  {
    lmLevelMask  = LogLevelExit;
    lmLevelMask |= LogLevelError;
    lmLevelMask |= LogLevelWarning;
  }
  else if (strcasecmp(level, "INFO") == 0)
  {
    lmLevelMask  = LogLevelExit;
    lmLevelMask |= LogLevelError;
    lmLevelMask |= LogLevelWarning;
    lmLevelMask |= LogLevelInfo;
  }
  else if (strcasecmp(level, "VERBOSE") == 0)
  {
    lmLevelMask  = LogLevelExit;
    lmLevelMask |= LogLevelError;
    lmLevelMask |= LogLevelWarning;
    lmLevelMask |= LogLevelInfo;
    lmLevelMask |= LogLevelVerbose;
  }
  else if (strcasecmp(level, "DEBUG") == 0)
  {
    lmLevelMask  = LogLevelExit;
    lmLevelMask |= LogLevelError;
    lmLevelMask |= LogLevelWarning;
    lmLevelMask |= LogLevelInfo;
    lmLevelMask |= LogLevelVerbose;
    lmLevelMask |= LogLevelDebug;
  }
  else if (strcasecmp(level, "TRACE") == 0)
  {
    lmLevelMask  = LogLevelExit;
    lmLevelMask |= LogLevelError;
    lmLevelMask |= LogLevelWarning;
    lmLevelMask |= LogLevelInfo;
    lmLevelMask |= LogLevelVerbose;
    lmLevelMask |= LogLevelDebug;
    lmLevelMask |= LogLevelTrace;
  }
  else if (strcasecmp(level, "ALL") == 0)
  {
    lmLevelMask  = 0xFFFFFFFF;
  }
}



/* ****************************************************************************
*
* lmLevelMaskGet - 
*/
int lmLevelMaskGet(void)
{
  return lmLevelMask;
}



/* ****************************************************************************
*
* lmLevelMaskStringGet -
*/
std::string lmLevelMaskStringGet(void)
{
  /* Check masks, in incresing severity order */

  if (lmLevelMask == 0)
  {
    return "NONE";
  }
  else if (lmLevelMask & LogLevelDebug)
  {
    return "DEBUG";
  }
  else if (lmLevelMask & LogLevelInfo)
  {
    return "INFO";
  }
  else if (lmLevelMask & LogLevelWarning)
  {
    return "WARN";
  }
  else if (lmLevelMask & LogLevelError)
  {
    return "ERROR";
  }
  else if (lmLevelMask & LogLevelExit)
  {
    return "FATAL";
  }
  else
  {
    // Reaching would be an error in the log level management logic...
    return "UNKNOWN";
  }
}



/* ****************************************************************************
*
* lmTraceIsSet -
*/
bool lmTraceIsSet(int level)
{
  return tLevel[level];
}



/* ****************************************************************************
*
* lmTraceNameCbSet -
*/
void lmTraceNameCbSet(LmTracelevelName cb)
{
  userTracelevelName = cb;
}



/* ****************************************************************************
*
* addLevels -
*/
static void addLevels(bool* tLevelP, unsigned char from, unsigned char to)
{
  int   i;

  for (i = from; i <= to; i++)
  {
    LOG_OUT(("ADD: trace level %d is set", i));
    tLevelP[i] = true;
  }
}



/* ****************************************************************************
*
* subLevels -
*/
static void subLevels(bool* tLevelP, unsigned char from, unsigned char to)
{
  int i;

  for (i = from; i <= to; i++)
  {
    LOG_OUT(("trace level %d is removed", i));
    tLevelP[i] = false;
  }
}



/* ****************************************************************************
*
* zeroDelimiter -
*/
static char* zeroDelimiter(char* string, char delimiter)
{
  if (string == NULL)
  {
    return NULL;
  }

  while (*string != 0)
  {
    if (*string == delimiter)
    {
      *string = 0;
      return ++string;
    }
    else
    {
      ++string;
    }
  }

  return NULL;
}



/* ****************************************************************************
*
* ws - is the character 'c' a whitespace (space, tab or '\n')
*/
static bool ws(char c)
{
  switch (c)
  {
  case ' ':
  case '\t':
  case '\n':
    return true;
    break;

  default:
    return false;
    break;
  }

  return false;
}



/* ****************************************************************************
*
* wsNoOf - number of whitespaces in the string 'string'
*/
static int wsNoOf(char* string)
{
  int no = 0;

  while (*string != 0)
  {
    if (ws(*string) == true)
    {
      ++no;
    }

    ++string;
  }
  return no;
}



/* ****************************************************************************
*
* traceFix - fix trace levels
*
* DESCRIPTION
* traceFix sets the trace levels according to the level format string
* 'levelFormat' and the way 'way' (ADD or SUB).
* 'levelFormat' contains a number of levels, delimited by comma (,) and with
* the format (x and y are integers .LT. 256):
*
* 1. x     set level x
* 2. <x    set all levels from 0 to x   (including 0)
* 3. <=x   set all levels from 0 to x   (including 0 and x)
* 4. >x    set all levels from x to 256 (including 256)
* 5. >x    set all levels from x to 256 (including x and 256)
* 6. x-y   set all levels from x to y   (including x and y)
*
*
* NOTE
* Whitespace is not allowed in the level format string.
*
*/
static void traceFix(char* levelFormat, unsigned int way)
{
  char*          currP;
  char*          nextP;
  unsigned char  min;
  unsigned char  max;
  char*          minusP;
  char*          levelFormatP;


  /* No whitespace allowed + ADD or SUB */
  if ((wsNoOf(levelFormat) != 0) || (way > 1))
  {
    return;
  }

  levelFormatP = strdup(levelFormat);

  currP = &levelFormatP[0];

  while (currP != NULL)
  {
    nextP = zeroDelimiter(currP, DELIMITER);

    if (*currP == '<')
    {
      ++currP;
      min  = 0;
      if (*currP == '=')
      {
        max  = atoi(&currP[1]);
      }
      else
      {
        max  = atoi(currP);
        if (max == 0)
        {
          min = 1;
        }
        else
        {
          --max;
        }
      }
    }
    else if (*currP == '>')
    {
      ++currP;
      max  = TRACE_LEVELS - 1;
      if (*currP == '=')
      {
        min  = atoi(&currP[1]);
      }
      else
      {
        min  = atoi(currP);
        if (min == 255)
        {
          max = 254;
        }
        else
        {
          ++min;
        }
      }
    }
    else if ((*currP >= '0') && (*currP <= '9'))
    {
      minusP       = zeroDelimiter(currP, '-');
      min          = atoi(currP);
      max          = min;

      if (minusP != NULL)
      {
        max = atoi(minusP);
      }
    }
    else
    {
      break;
    }

    if (way == ADD)
    {
      addLevels(tLevel, min, max);
    }
    else if (way == SUB)
    {
      subLevels(tLevel, min, max);
    }

    currP = nextP;
  }

  ::free(levelFormatP);
  return;
}



/* ****************************************************************************
*
* dateGet -
*/
static char* dateGet(int index, char* line, int lineSize)
{
  time_t  secondsNow = time(NULL);

  if (strcmp(fds[index].timeFormat, "UNIX") == 0)
  {
    char secs[32];

    snprintf(secs, sizeof(secs), "%ds", (int) secondsNow);
    strncpy(line, secs, lineSize);
  }
  else if (strcmp(fds[index].timeFormat, "DIFF") == 0)
  {
    int tm = (int) secondsNow - (int) secondsAtStart;
    int days;
    int hours;
    int mins;
    int secs;

    secs  = tm % 60;
    tm    = tm / 60;
    mins  = tm % 60;
    tm    = tm / 60;
    hours = tm % 24;
    tm    = tm / 24;
    days  = tm;

    if (days != 0)
    {
      snprintf(line, lineSize, "%d days %02d:%02d:%02d", days, hours, mins, secs);
    }
    else
    {
      snprintf(line, lineSize, "%02d:%02d:%02d", hours, mins, secs);
    }
  }
  else
  {
    struct timeb timebuffer;
    struct tm    tm;
    char         line_buf[80];

    ftime(&timebuffer);
    gmtime_r(&secondsNow, &tm);
    strftime(line_buf, 80, fds[index].timeFormat, &tm);
    snprintf(line, lineSize, "%s.%.3dZ", line_buf, timebuffer.millitm);
  }

  return line;
}



/* ****************************************************************************
*
* timeGet -
*/
static char* timeGet(int index, char* line, int lineSize)
{
  time_t  secondsNow = time(NULL);

  if (strcmp(fds[index].timeFormat, "UNIX") == 0)
  {
    char secs[32];

    snprintf(secs, sizeof(secs), "%ds", (int) secondsNow);
    strncpy(line, secs, lineSize);
  }
  else
  {
    int tm = (int) secondsNow - (int) secondsAtStart;
    int days;
    int hours;
    int mins;
    int secs;

    if (strcmp(fds[index].timeFormat, "DIFF") == 0)
    {
      tm = (int) secondsNow - (int) secondsAtStart;
    }
    else
    {
      tm = (int) secondsNow;
    }

    secs  = tm % 60;
    tm    = tm / 60;
    mins  = tm % 60;
    tm    = tm / 60;
    hours = tm % 24;
    tm    = tm / 24;
    days  = tm;

    if (strcmp(fds[index].timeFormat, "DIFF") == 0)
    {
      if (days != 0)
      {
        snprintf(line, lineSize, "%d days %02d:%02d:%02d", days, hours, mins, secs);
      }
      else
      {
        snprintf(line, lineSize, "%02d:%02d:%02d", hours, mins, secs);
      }
    }
    else
    {
      snprintf(line, lineSize, "%02d:%02d:%02d", hours, mins, secs);
    }
  }

  return line;
}


#if 0
/* ****************************************************************************
*
* timeStampGet -
*
* This function has been removed as the LM_S macro, formerly a 'timestamp macro'
* has been removed for the Orion Context Broker implementation, to make room for
* a new LM_S macro, the 'S' standing for 'Summary'.
*
* The function is kept in case the LM_S is taken back as a 'timestamp macro' for
* some other project.
*/
static char* timeStampGet(char* line, int len)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  snprintf(line, len, "timestamp: %d.%.6d secs\n", (int)tv.tv_sec, (int)tv.tv_usec);

  return line;
}
#endif


/* ****************************************************************************
*
* longTypeName -
*/
const char* longTypeName(char type)
{
  switch (type)
  {
  case 'W':  return "WARN";
  case 'E':  return "ERROR";
  case 'P':  return "ERROR";
  case 'X':  return "FATAL";
  case 'T':  return "DEBUG";
  case 'D':  return "DEBUG";
  case 'V':  return "DEBUG";
  case '2':  return "DEBUG";
  case '3':  return "DEBUG";
  case '4':  return "DEBUG";
  case '5':  return "DEBUG";
  case 'M':  return "DEBUG";
  case 'F':  return "TMP";
  case 'I':  return "INFO";
  case 'S':  return "SUMMARY";
  case 'K':  return "TMP";
  }

  return "N/A";
}



/* ****************************************************************************
*
* lmLineFix -
*/
static char* lmLineFix
(
  int          index,
  char*        line,
  int          lineLen,
  char         type,
  const char*  file,
  int          lineNo,
  const char*  fName,
  int          tLev
)
{
  char         xin[256];
  int          fLen;
  int          fi     = 0;
  Fds*         fdP    = &fds[index];
  char*        format = fdP->format;

  memset(line, 0, lineLen);

  fLen = strlen(format);
  while (fi < fLen)
  {
    if (strncmp(&format[fi], "TYPE", 4) == 0)
    {
      STRING_ADD(longTypeName(type), 4);
    }
    else if (strncmp(&format[fi], "PID", 3) == 0)
    {
      INT_ADD((int) getpid(), 3);
    }
    else if (strncmp(&format[fi], "DATE", 4) == 0)
    {
      STRING_ADD(dateGet(index, xin, sizeof(xin)), 4);
    }
    else if (strncmp(&format[fi], "TIME", 4) == 0)
    {
      STRING_ADD(timeGet(index, xin, sizeof(xin)), 4);
    }
    else if (strncmp(&format[fi], "TID", 3) == 0)
    {
      pid_t tid;
      tid = syscall(SYS_gettid);
      INT_ADD((int) tid, 3);
    }
    else if (strncmp(&format[fi], "TRANS_ID", 8) == 0)
    {
      STRING_ADD(transactionId, 8);
    }
    else if (strncmp(&format[fi], "CORR_ID", 7) == 0)
    {
      STRING_ADD(correlatorId, 7);
    }
    else if (strncmp(&format[fi], "SERVICE", 7) == 0)
    {
      STRING_ADD(service, 7);
    }
    else if (strncmp(&format[fi], "SUB_SERVICE", 11) == 0)
    {
      STRING_ADD(subService, 11);
    }
    else if (strncmp(&format[fi], "FROM_IP", 7) == 0)
    {
      STRING_ADD(fromIp, 7);
    }
    else if (strncmp(&format[fi], "EXEC", 4) == 0)
    {
      STRING_ADD(progName, 4);
    }
    else if (strncmp(&format[fi], "AUX", 3) == 0)
    {
      STRING_ADD(aux, 3);
    }
    else if (strncmp(&format[fi], "FILE", 4) == 0)
    {
      STRING_ADD(file, 4);
    }
    else if (strncmp(&format[fi], "LINE", 4) == 0)
    {
      INT_ADD(lineNo, 4);
    }
    else if (strncmp(&format[fi], "TLEV", 4) == 0)
    {
      TLEV_ADD(type, tLev);
    }
    else if (strncmp(&format[fi], "TEXT", 4) == 0)
    {
      STRING_ADD("%s", 4);
    }
    else if (strncmp(&format[fi], "FUNC", 4) == 0)
    {
      STRING_ADD(fName, 4);
    }
    else  /* just a normal character */
    {
      CHAR_ADD(format[fi], 1);
    }
  }

  if ((type == 'T') && (fdP->traceShow == true))
  {
    snprintf(xin, sizeof(xin), "%s%d%s\n", fdP->tMarkStart, tLev, fdP->tMarkEnd);
  }
  else if (type == 'x') /* type 'x' => */
  {
    snprintf(xin, sizeof(xin), ": %s\n", strerror(errno));
  }
  else
  {
    strncpy(xin, "\n", sizeof(xin));
  }

  strncat(line, xin, lineLen - strlen(line) - 1);

  return line;
}



/* ****************************************************************************
*
* fdsFreeGet -
*/
static int fdsFreeGet(void)
{
  int i = 0;

  while (i < FDS_MAX)
  {
    if (fds[i].state != Occupied)
    {
      return i;
    }

    i++;
  }

  return -1;
}



/* ****************************************************************************
*
* isdir - is path a directory?
*/
static bool isdir(char* path)
{
  struct stat xStat;

  if (stat(path, &xStat) == -1)
  {
    return false;
  }
  else if ((xStat.st_mode & S_IFDIR) == S_IFDIR)
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* asciiToLeft -
*/
static void asciiToLeft
(
  char*     line,
  int       lineLen,
  char*     buffer,
  int       size,
  LmFormat  form,
  int       last
)
{
  int   i;
  int   offset;
  char  tmp[80];

  switch (form)
  {
  case LmfByte: offset = 16 * 3 + 2 - size * 3;                 break;
  case LmfWord: offset = 8 * 5 + 2 - (size / 2) * 5 - last;     break;
  case LmfLong: offset = 4 * 9 + 2 - (size / 4) * 9 - last * 2; break;
  default:      return;
  }

  while (offset-- >= 0)
  {
    strncat(line, " ", lineLen - strlen(line) - 1);
  }

  for (i = 0; i < size; i++)
  {
    if (buffer[i] == 0x25)
    {
      strncpy(tmp, ".", sizeof(tmp));
    }
    else if (isprint((int) buffer[i]))
    {
      snprintf(tmp, sizeof(tmp), "%c", buffer[i]);
    }
    else
    {
      strncpy(tmp, ".", sizeof(tmp));
    }

    strncat(line, tmp, lineLen - strlen(line) - 1);
  }
}


/* ****************************************************************************
*
* lmCleanProgName -
*/
void lmCleanProgName(void)
{
  if (progName)
  {
    free(progName);
  }

  progName = NULL;
}



/* ************************************************************************* */
/* ********************** EXTERNAL FUNCTIONS ******************************* */
/* ************************************************************************* */



/* ****************************************************************************
*
* lmInit -
*/
LmStatus lmInit(void)
{
  if (fdNoOf == 0)
  {
    return LmsNoFiles;
  }

  if (initDone == true)
  {
    return LmsInitAlreadyDone;
  }

  PROGNAME_CHECK();

  secondsAtStart = time(NULL);
  auxOn          = false;
  aux[0]         = 0;
  logLines       = 4;
  initDone       = true;

  memset(tLevel, 0, sizeof(tLevel));

  return LmsOk;
}



/* ****************************************************************************
*
* lmInitX -
*/
LmStatus lmInitX(char* pName, char* tLevel, int* i1P, int* i2P)
{
  LmStatus s;

  if ((progName = lmProgName(pName, 1, false)) == NULL)
  {
    return LmsPrognameError;
  }

  if ((s = lmFdRegister(1, DEF1, "DEF", "stdout", i1P)) != LmsOk)
  {
    return s;
  }

  if ((s = lmPathRegister("/tmp", "DEF", "DEF", i2P)) != LmsOk)
  {
    return s;
  }

  if ((s = lmInit()) != LmsOk)
  {
    return s;
  }

  if ((s = lmTraceSet(tLevel)) != LmsOk)
  {
    return s;
  }

  return LmsOk;
}



/* ****************************************************************************
*
* lmStrerror
*/
const char* lmStrerror(LmStatus s)
{
  switch (s)
  {
  case LmsOk:               return "no problem";

  case LmsInitNotDone:      return "lmInit must be called first";
  case LmsInitAlreadyDone:  return "init already done";
  case LmsNull:             return "NULL pointer";
  case LmsFdNotFound:       return "fd not found";
  case LmsFdInvalid:        return "invalid file descriptor";
  case LmsFdOccupied:       return "file descriptor occupied";

  case LmsMalloc:           return "malloc error";
  case LmsOpen:             return "cannot open file";
  case LmsFopen:            return "error fopening for clearing out file";
  case LmsLseek:            return "lseek error";
  case LmsFseek:            return "fseek error";
  case LmsWrite:            return "write error";
  case LmsFgets:            return "error reading (fgets)";

  case LmsNoFiles:          return "no log files registrated";
  case LmsProgNameLevels:   return "bad program name level";
  case LmsLineTooLong:      return "log line too long";
  case LmsStringTooLong:    return "string too long";
  case LmsBadFormat:        return "bad format";
  case LmsBadSize:          return "bad size";
  case LmsBadParams:        return "bad parameters";
  case LmsPrognameNotSet:   return "progName not set";
  case LmsPrognameError:    return "error setting progName";
  case LmsClearNotAllowed:  return "clear option not set";
  }

  return "status code not recognized";
}



/* ***************************************************************************
*
* lmTraceSet - set trace levels
*
* DESCRIPTION
* See lmTraceAdd
*/
LmStatus lmTraceSet(const char* levelFormat)
{
  INIT_CHECK();

  subLevels(tLevel, 0, TRACE_LEVELS - 1);

  if (levelFormat != NULL)
  {
    traceFix((char*) levelFormat, ADD);
  }

  return LmsOk;
}



/* ***************************************************************************
*
* lmTraceAdd - add trace levels
*
* DESCRIPTION
* See traceFix
*/
LmStatus lmTraceAdd(const char* levelFormat)
{
  INIT_CHECK();

  traceFix((char*) levelFormat, ADD);

  return LmsOk;
}



/* ****************************************************************************
*
* lmTraceLevelSet -
*/
void lmTraceLevelSet(unsigned int level, bool onOff)
{
  if (level >= TRACE_LEVELS)
  {
    return;
  }

  tLevel[level] = onOff;
}



/* ***************************************************************************
*
* lmTraceSub - remove trace levels
*
* DESCRIPTION
* See traceFix
*/
LmStatus lmTraceSub(const char* levelFormat)
{
  traceFix((char*) levelFormat, SUB);

  return LmsOk;
}



/* ****************************************************************************
*
* lmTraceGet -
*
*/
char* lmTraceGet(char* levelString, int levelStringSize)
{
  int       i;
  int       j = 0;
  int       levels[256];

  bzero(levels, sizeof(levels));

  if (levelString == NULL)
  {
    LOG_OUT(("returning NULL"));
    return NULL;
  }

  levelString[0] = 0;

  for (i = 0; i < 256; i++)
  {
    if (tLevel[i] == true)
    {
      LOG_OUT(("GET: trace level %d is set", i));
      levels[j++] = i;
    }
  }

  if (j == 0)
  {
    LOG_OUT(("returning '%s'", levelString));
    snprintf(levelString, levelStringSize, "empty");
    return levelString;
  }

  snprintf(levelString, levelStringSize, "%d", levels[0]);

  for (i = 1; i < j; i++)
  {
    int   prev   = levels[i - 1];
    int   diss   = levels[i];
    int   next   = levels[i + 1];
    bool  before = (diss == (prev + 1));
    bool  after  = (diss == (next - 1));
    char  str[12];

    if (i == 255)
      after = false;

    if (before && after)
    {
    }
    else if (before && !after)
    {
      snprintf(str, sizeof(str), "-%d", diss);
      strncat(levelString, str, levelStringSize - strlen(levelString) - 1);
    }
    else if (!before && after)
    {
      snprintf(str, sizeof(str), ", %d", diss);
      strncat(levelString, str, levelStringSize - strlen(levelString) - 1);
    }
    else if (!before && !after)
    {
      snprintf(str, sizeof(str), ", %d", diss);
      strncat(levelString, str, levelStringSize - strlen(levelString) - 1);
    }
  }

  return levelString;
}



/* ****************************************************************************
*
* lmTraceGet -
*
*/
char* lmTraceGet(char* levelString, int levelStringSize, char* traceV)
{
  int       i;
  int       j = 0;
  int       levels[256];

  if (levelString == NULL)
  {
    LOG_OUT(("returning NULL"));
    return NULL;
  }

  levelString[0] = 0;

  for (i = 0; i < 256; i++)
  {
    if (traceV[i] == true)
    {
      LOG_OUT(("GET: trace level %d is set", i));
      levels[j++] = i;
    }
  }

  if (j == 0)
  {
    LOG_OUT(("returning '%s'", levelString));
    snprintf(levelString, levelStringSize, "empty");
    return levelString;
  }

  snprintf(levelString, levelStringSize, "%d", levels[0]);

  for (i = 1; i < j; i++)
  {
    int   prev   = levels[i - 1];
    int   diss   = levels[i];
    int   next   = levels[i + 1];
    bool  before = (diss == prev + 1);
    bool  after  = (diss == next - 1);
    char  str[12];

    if (i == 255)
      after = false;

    if (before && after)
    {
    }
    else if (before && !after)
    {
      snprintf(str, sizeof(str), "-%d", diss);
      strncat(levelString, str, levelStringSize - strlen(levelString) - 1);
    }
    else if (!before && after)
    {
      snprintf(str, sizeof(str), ", %d", diss);
      strncat(levelString, str, levelStringSize - strlen(levelString) - 1);
    }
    else if (!before && !after)
    {
      snprintf(str, sizeof(str), ", %d", diss);
      strncat(levelString, str, levelStringSize - strlen(levelString) - 1);
    }
  }

  return levelString;
}



/* ****************************************************************************
*
* lmWriteFunction - use alternative write function
*/
LmStatus lmWriteFunction(int i, LmWriteFp fp)
{
  INIT_CHECK();
  INDEX_CHECK(i);
  NOT_OCC_CHECK(i);

  fds[i].write = fp;

  return LmsOk;
}



/* ****************************************************************************
*
* lmDoClear -
*/
LmStatus lmDoClear(void)
{
  INIT_CHECK();
  doClear = true;
  return LmsOk;
}



/* ****************************************************************************
*
* lmDontClear -
*/
LmStatus lmDontClear(void)
{
  INIT_CHECK();
  doClear = false;
  return LmsOk;
}



/* ****************************************************************************
*
* lmClearAt -
*/
LmStatus lmClearAt(int atL, int keepL, int lastL)
{
  int a = atLines;
  int k = keepLines;
  int l = lastLines;

  INIT_CHECK();

  if (atL   != -1)      a = atL;
  if (keepL != -1)      k = keepL;
  if (lastL != -1)      l = lastL;

  if (a < (k + 4 + l))
  {
    return LmsBadParams;
  }

  atLines   = a;
  keepLines = k;
  lastLines = l;

  return LmsOk;
}



/* ****************************************************************************
*
* lmClearGet -
*/
void lmClearGet
(
  bool*      clearOn,
  int*       atP,
  int*       keepP,
  int*       lastP,
  int*       logFileBytesP
)
{
  if (clearOn != NULL)
  {
    *clearOn = doClear;
  }

  if (atP != NULL)
  {
    *atP = atLines;
  }

  if (keepP != NULL)
  {
    *keepP = keepLines;
  }

  if (lastP != NULL)
  {
    *lastP = lastLines;
  }

  if (logFileBytesP != NULL)
  {
    *logFileBytesP = logLines;
  }
}



/* ****************************************************************************
*
* lmFormat -
*/
LmStatus lmFormat(int index, char* f)
{
  INIT_CHECK();
  INDEX_CHECK(index);
  STRING_CHECK(f, F_LEN);

  if (strcmp(f, "DEF") == 0)
    strncpy(fds[index].format, FORMAT_DEF, sizeof(fds[index].format));
  else
    strncpy(fds[index].format, f, sizeof(fds[index].format));

  return LmsOk;
}



/* ****************************************************************************
*
* lmTimeFormat -
*/
LmStatus lmTimeFormat(int index, char* f)
{
  INIT_CHECK();
  INDEX_CHECK(index);
  STRING_CHECK(f, TF_LEN);

  if (strcmp(f, "DEF") == 0)
  {
    strncpy(fds[index].timeFormat, TIME_FORMAT_DEF, sizeof(fds[index].timeFormat));
  }
  else
  {
    strncpy(fds[index].timeFormat, f, sizeof(fds[index].timeFormat));
  }

  return LmsOk;
}



/* ****************************************************************************
*
* lmGetInfo -
*
* NOTE
* info must be a pointer to a buffer of at least the size 80 bytes
*/
LmStatus lmGetInfo(int index, char* info)
{
  INIT_CHECK();
  INDEX_CHECK(index);
  NOT_OCC_CHECK(index);

  strncpy(info, fds[index].info, 80);

  return LmsOk;
}



/* ****************************************************************************
*
* lmFdGet -
*/
LmStatus lmFdGet(int index, int* iP)
{
  INIT_CHECK();
  INDEX_CHECK(index);
  NOT_OCC_CHECK(index);

  *iP = fds[index].fd;

  return LmsOk;
}



/* ****************************************************************************
*
* lmTraceAtEnd - I don't like the name of this function ...
*/
LmStatus lmTraceAtEnd(int index, char* start, char* end)
{
  INIT_CHECK();
  INDEX_CHECK(index);
  STRING_CHECK(start, TMS_LEN);
  STRING_CHECK(end, TME_LEN);

  if ((start == NULL) || (end == NULL))
  {
    fds[index].traceShow = false;
  }
  else
  {
    if (strcmp(start, "DEF") == 0)
    {
      strncpy(fds[index].tMarkStart, TMS_DEF, sizeof(fds[index].tMarkStart));
    }
    else
    {
      strncpy(fds[index].tMarkStart, start, sizeof(fds[index].tMarkStart));
    }

    if (strcmp(end, "DEF") == 0)
    {
      strncpy(fds[index].tMarkEnd, TME_DEF, sizeof(fds[index].tMarkEnd));
    }
    else
    {
      strncpy(fds[index].tMarkEnd, end, sizeof(fds[index].tMarkEnd));
    }

    fds[index].traceShow  = true;
  }

  return LmsOk;
}



/* ****************************************************************************
*
* lmAux -
*/
LmStatus lmAux(char* a)
{
  INIT_CHECK();
  STRING_CHECK(a, AUX_LEN);

  strncpy(aux, a, sizeof(aux));
  auxOn = true;

  return LmsOk;
}



/* ****************************************************************************
*
* lmTextGet - format message into string
*/
char* lmTextGet(const char* format, ...)
{
  va_list  args;
  char*    vmsg = (char*) calloc(1, LM_LINE_MAX);

  /* "Parse" the varible arguments */
  va_start(args, format);

  /* Print message to variable */
  vsnprintf(vmsg, LM_LINE_MAX, format, args);
  va_end(args);

  return vmsg;
}



/* ****************************************************************************
*
* lmOk -
*/
LmStatus lmOk(char type, int tLev)
{
  if (lmSilent == true)
  {
    return LmsNull;
  }

  if ((type == 'T') && (tLevel[tLev] == false))
  {
    return LmsNull;
  }

  if ((type == 'D') && (lmDebug == false))
  {
    return LmsNull;
  }

  if ((type == 'H') && (lmHidden == false))
  {
    return LmsNull;
  }

  if ((type == 'V') && (lmVerbose == false))
  {
    return LmsNull;
  }

  if ((type == '2') && (lmVerbose2 == false))
  {
    return LmsNull;
  }

  if ((type == '3') && (lmVerbose3 == false))
  {
    return LmsNull;
  }

  if ((type == '4') && (lmVerbose4 == false))
  {
    return LmsNull;
  }

  if ((type == '5') && (lmVerbose5 == false))
  {
    return LmsNull;
  }

  if ((type == 't') && (lmToDo == false))
  {
    return LmsNull;
  }

  if ((type == 'w') && (lmWrites == false))
  {
    return LmsNull;
  }

  if ((type == 'r') && (lmReads == false))
  {
    return LmsNull;
  }

  if ((type == 'b') && (lmBuf == false))
  {
    return LmsNull;
  }

  if ((type == 'B') && (lmBug == false))
  {
    return LmsNull;
  }

  if ((type == 'F') && (lmFix == false))
  {
    return LmsNull;
  }

  if ((type == 'd') && (lmDoubt == false))
  {
    return LmsNull;
  }

  return LmsOk;
}



/* ****************************************************************************
*
* lmFdRegister -
*/
LmStatus lmFdRegister
(
  int          fd,
  const char*  format,
  const char*  timeFormat,
  const char*  info,
  int*         indexP
)
{
  int        index;
  time_t     secsNow;
  struct tm tmP;

  PROGNAME_CHECK();

  if (fdNoOf == 0)
  {
    int i;

    for (i = 0; i < FDS_MAX; i++)
    {
      fds[i].state = Free;
    }
  }

  STRING_CHECK(info, INFO_LEN);
  STRING_CHECK(format, F_LEN);
  STRING_CHECK(timeFormat, TF_LEN);

  if (fd < 0)
  {
    return LmsFdInvalid;
  }

  if ((index = fdsFreeGet()) == -1)
  {
    return LmsFdOccupied;
  }

  secsNow = time(NULL);
  lm::gmtime_r(&secsNow, &tmP);

  if ((fd >= 0) && (strcmp(info, "stdout") != 0))
  {
    if (lmPreamble == true)
    {
      char  startMsg[256];
      char  dt[128];
      int   sz;

      strftime(dt, 256, "%A %d %h %H:%M:%S %Y", &tmP);
      snprintf(startMsg, sizeof(startMsg),
               "%s log\n-----------------\nStarted %s\nCleared at ...\n",
               progName, dt);

      sz = strlen(startMsg);

      lseek(fd, 0, SEEK_END);

      if (write(fd, startMsg, sz) != sz)
      {
        return LmsWrite;
      }
    }
  }

  fds[index].fd    = fd;
  fds[index].state = Occupied;
  if (strcmp(format, "DEF") == 0)
  {
    strncpy(fds[index].format, FORMAT_DEF, sizeof(fds[index].format));
  }
  else
  {
    strncpy(fds[index].format, format, sizeof(fds[index].format));
  }

  if (strcmp(timeFormat, "DEF") == 0)
  {
    strncpy(fds[index].timeFormat, TIME_FORMAT_DEF, sizeof(fds[index].timeFormat));
  }
  else
  {
    strncpy(fds[index].timeFormat, timeFormat, sizeof(fds[index].timeFormat));
  }

  strncpy(fds[index].info, info, sizeof(fds[index].info));

  if (indexP)
  {
    *indexP = index;
  }

  fdNoOf++;
  fds[index].type = Stdout;

  if ((strcmp(info, "stdout") == 0) ||  (strcmp(info, "stderr") == 0))
  {
    if (indexP)
    {
      lmSd = *indexP;
    }
  }

  return LmsOk;
}



/* ****************************************************************************
*
* lmFdUnregister
*/
void lmFdUnregister(int fd)
{
  int index;

  for (index = 0; index < FDS_MAX; index++)
  {
    if (fds[index].fd != fd)
    {
      continue;
    }

    fds[index].fd    = -1;
    fds[index].state = Free;
  }
}



/* ****************************************************************************
*
* lmPathRegister -
*/
LmStatus lmPathRegister
(
  const char*  path,
  const char*  format,
  const char*  timeFormat,
  int*         indexP,
  bool         appendToLogFile
)
{
  int       fd;
  LmStatus  s;
  char      fileName[256];
  int       index;

  PROGNAME_CHECK();

  // FIXME P4: check whether this might be dangerous ...
  // if (initDone == true)
  //   return LmsInitAlreadyDone;

  STRING_CHECK(format, F_LEN);

  if (isdir((char*) path) == true)
  {
    char *leaf_path = strrchr(progName, '/');
    if (leaf_path == NULL)
    {
      leaf_path = progName;
    }
    snprintf(fileName, sizeof(fileName), "%s/%s.log", path, leaf_path);
  }
  else
  {
    if (access(fileName, X_OK) == -1)
    {
      printf("Sorry, directory '%s' doesn't exist (errno == %d)\n", path, errno);
      exit(1);
    }

    strncpy(fileName, path, sizeof(fileName));
  }

  if (appendToLogFile == false)
  {
    if (access(fileName, F_OK) == 0)
    {
      char newName[260];

      snprintf(newName, sizeof(newName), "%s.old", fileName);
      rename(fileName, newName);
    }

    fd = open(fileName, O_RDWR | O_TRUNC | O_CREAT, 0664);
  }
  else
  {
    fd = open(fileName, O_RDWR | O_CREAT | O_APPEND, 0664);
  }

  if (fd == -1)
  {
    char str[300];

    printf("Error opening '%s': %s\n", fileName, strerror(errno));

    snprintf(str, sizeof(str), "open(%s)", fileName);
    perror(str);
    return LmsOpen;
  }

  chmod(fileName, 0664);

  s = lmFdRegister(fd, format, timeFormat, fileName/* info */, &index);
  if (s != LmsOk)
  {
    close(fd);
    return s;
  }

  fds[index].type = Fichero;

  if (indexP)
  {
    *indexP = index;
  }

  return LmsOk;
}



/* ****************************************************************************
*
* lmOut -
*/
LmStatus lmOut
(
  char*        text,
  char         type,
  const char*  file,
  int          lineNo,
  const char*  fName,
  int          tLev,
  const char*  stre,
  bool         use_hook
)
{
  INIT_CHECK();
  POINTER_CHECK(text);

  int   i;
  char* line = (char*) calloc(1, LM_LINE_MAX);
  int   sz;
  char* format = (char*) calloc(1, FORMAT_LEN + 1);
  char* tmP;

  if ((line == NULL) || (format == NULL))
  {
    if (line   != NULL)   free(line);
    if (format != NULL)   free(format);

    return LmsNull;
  }

  tmP = strrchr((char*) file, '/');
  if (tmP != NULL)
  {
    file = &tmP[1];
  }

  if (inSigHandler && (type != 'X' || type != 'x'))
  {
    lmAddMsgBuf(text, type, file, lineNo, fName, tLev, (char*) stre);
    free(line);
    free(format);

    return LmsOk;
  }

  memset(format, 0, FORMAT_LEN + 1);

  semTake();

  if ((type != 'H') && lmOutHook && lmOutHookActive == true)
  {
    time_t secondsNow = time(NULL);
    if (use_hook)
    {
      lmOutHook(lmOutHookParam, text, type, secondsNow, 0, 0, file, lineNo, fName, tLev, stre);
    }
  }

  for (i = 0; i < FDS_MAX; i++)
  {
    if (fds[i].state != Occupied)
    {
      continue;
    }

    if ((fds[i].type == Stdout) && (fds[i].onlyErrorAndVerbose == true))
    {
      if ((type == 'T') ||
          (type == 'D') ||
          (type == 'H') ||
          (type == 'M') ||
          (type == 't')
        )
      {
        continue;
      }
    }

    if (type == 'R')
    {
      if (text[1] != ':')
      {
        snprintf(line, LM_LINE_MAX, "R: %s\n%c", text, 0);
      }
      else
      {
        snprintf(line, LM_LINE_MAX, "%s\n%c", text, 0);
      }
    }
    else
    {
      /* Danger: 'format' might be too short ... */
      if (lmLineFix(i, format, FORMAT_LEN, type, file, lineNo, fName, tLev) == NULL)
      {
        continue;
      }

      if ((strlen(format) + strlen(text) + strlen(line)) > LM_LINE_MAX)
      {
        snprintf(line, LM_LINE_MAX, "%s[%d]: %s\n%c", file, lineNo, "LM ERROR: LINE TOO LONG", 0);
      }
      else
      {
        snprintf(line, LM_LINE_MAX, format, text);
      }
    }

    if (stre != NULL)
    {
      strncat(line, stre, LM_LINE_MAX - strlen(stre) - 1);
    }

    sz = strlen(line);

    if (fds[i].write != NULL)
    {
      fds[i].write(line);
    }
    else
    {
      int nb;

      lseek(fds[i].fd, 0, SEEK_END);
      nb = write(fds[i].fd, line, sz);

      if (nb == -1)
      {
        printf("LOG error: write(%d): %s\n", fds[i].fd, strerror(errno));
      }
      else if (nb != sz)
      {
        printf("LOG error: written %d bytes only (wanted %d)\n", nb, sz);
      }
    }
  }

  ++logLines;
  LOG_OUT(("logLines: %d", logLines));

  if (type == 'W')
  {
    if (warningFunction != NULL)
    {
      fprintf(stderr, "Calling warningFunction (at %p)\n", &warningFunction);

      warningFunction(warningInput, text, (char*) stre);
      fprintf(stderr, "warningFunction done\n");
    }
  }
  else if ((type == 'E') || (type == 'P'))
  {
    if (errorFunction != NULL)
    {
      errorFunction(errorInput, text, (char*) stre);
    }
  }
  else if ((type == 'X') || (type == 'x'))
  {
    semGive();

    if (exitFunction != NULL)
    {
      exitFunction(tLev, exitInput, text, (char*) stre);
    }

    if (lmAssertAtExit == true)
    {
      assert(false);
    }

    /* exit here, just in case */
    free(line);
    free(format);
    exit(tLev);
  }

  if ((doClear == true) && (logLines >= atLines))
  {
    int i;

    for (i = 0; i < FDS_MAX; i++)
    {
      if (fds[i].state == Occupied)
      {
        LmStatus s;

        if (fds[i].type != Fichero)
        {
          continue;
        }

        if ((s = lmClear(i, keepLines, lastLines)) != LmsOk)
        {
          free(line);
          free(format);
          semGive();
          return s;
        }
      }
    }
  }

  free(line);
  free(format);
  semGive();
  return LmsOk;
}



/* ****************************************************************************
*
* lmOutHookSet -
*/
void lmOutHookSet(LmOutHook hook, void* param)
{
  lmOutHook       = hook;
  lmOutHookParam  = param;
  lmOutHookActive = true;
}



/* ****************************************************************************
*
* lmOutHookInhibit -
*/
bool lmOutHookInhibit(void)
{
  bool oldValue = lmOutHookActive;

  lmOutHookActive = false;

  return oldValue;
}



/* ****************************************************************************
*
* lmOutHookRestore -
*/
void lmOutHookRestore(bool onoff)
{
  lmOutHookActive = onoff;
}



/* ****************************************************************************
*
* lmExitFunction -
*/
LmStatus lmExitFunction(LmExitFp fp, void* input)
{
  INIT_CHECK();
  POINTER_CHECK(fp);

  exitFunction = fp;
  exitInput    = input;

  return LmsOk;
}



/* ****************************************************************************
*
* lmWarningFunction -
*/
LmStatus lmWarningFunction(LmWarningFp fp, void* input)
{
  INIT_CHECK();
  POINTER_CHECK(fp);

  warningFunction = fp;
  warningInput    = input;

  fprintf(stderr, "Set warningFunction to %p\n", &warningFunction);

  return LmsOk;
}



/* ****************************************************************************
*
* lmWarningFunctionDebug
*/
void lmWarningFunctionDebug(char* info, char* file, int line)
{
  fprintf(stderr, "%s[%d]: %s: warningFunction: %p\n",
          file, line, info, &warningFunction);
}



/* ****************************************************************************
*
* lmErrorFunction -
*/
LmStatus lmErrorFunction(LmErrorFp fp, void* input)
{
  INIT_CHECK();
  POINTER_CHECK(fp);

  errorFunction = fp;
  errorInput    = input;

  return LmsOk;
}



/******************************************************************************
*
* lmBufferPresent -
*
* lmBufferPresent presents a buffer in a nice hexa decimal format.
*/
int lmBufferPresent
(
  char*       to,
  char*       description,
  void*       bufP,
  int         size,
  LmFormat    format,
  int         type
)
{
  int   bIndex     = 0;
  int   bIndexLast = 0;
  int   xx         = 0;
  char* buffer     = (char*) bufP;
  int   start      = 0;
  char  line[160];
  char  tmp[80];

  if (size > 0x800)
  {
    size = 0x800;
  }

  if (lmOk(type, type) != LmsOk)
  {
    return LmsOk;
  }

  if (size <= 0)
  {
    return LmsBadSize;
  }

  if ((format != LmfByte) && (format != LmfWord) && (format != LmfLong))
  {
    return LmsBadFormat;
  }

  memset(line, 0, sizeof(line));

  if (to != NULL)
  {
    char  msg[160];

    snprintf(msg, sizeof(msg), "%s %s %s (%d bytes) %s %s", progName,
             (type == 'r')? "reading" : "writing", description, size,
             (type == 'r')? "from"    : "to", to);

    LM_RAW(("%c: ----  %s ----",  type, msg));
  }

  while (bIndex < size)
  {
    if ((bIndex % 0x10) == 0)
    {
      snprintf(line, sizeof(line), "%c: %.8x:   ", type, start);
    }

    switch (format)
    {
    case LmfLong:
      if (bIndex + 4 <= size)
      {
        snprintf(tmp, sizeof(tmp), "%.8x ", *((int*) &buffer[bIndex]));
        strncat(line, tmp, sizeof(line) - strlen(line) - 1);
        bIndex += 4;
      }
      else if (bIndex + 1 == size)
      {
        snprintf(tmp, sizeof(tmp), "%.2xxxxxxx",
                 (*((int*) &buffer[bIndex]) & 0xFF000000) >> 24);
        strncat(line, tmp, sizeof(line) - strlen(line) - 1);
        bIndex += 1;
        xx      = 1;
      }
      else if (bIndex + 2 == size)
      {
        snprintf(tmp, sizeof(tmp), "%.4xxxxx",
                 (*((int*) &buffer[bIndex]) & 0xFFFF0000) >> 16);
        strncat(line, tmp, sizeof(line) - strlen(line) - 1);
        bIndex += 2;
        xx      = 2;
      }
      else if (bIndex + 3 == size)
      {
        snprintf(tmp, sizeof(tmp), "%.6xxx", (*((int*) &buffer[bIndex]) & 0xFFFFFF00) >> 8);
        strncat(line, tmp, sizeof(line) - strlen(line) - 1);
        bIndex += 3;
        xx      = 3;
      }
      break;

    case LmfWord:
      if (bIndex + 2 <= size)
      {
        snprintf(tmp, sizeof(tmp), "%.4x ", *((int16_t*) &buffer[bIndex]) & 0xFFFF);
        strncat(line, tmp, sizeof(line) - strlen(line) - 1);
        bIndex += 2;
      }
      else
      {
        snprintf(tmp, sizeof(tmp), "%.2xxx", (*((int16_t*) &buffer[bIndex]) & 0xFF00) >> 8);
        strncat(line, tmp, sizeof(line) - strlen(line) - 1);
        bIndex += 1;
        xx      = 1;
      }
      break;

    case LmfByte:
      snprintf(tmp, sizeof(tmp), "%.2x ", buffer[bIndex] & 0xFF);
      strncat(line, tmp, sizeof(line) - strlen(line) - 1);
      bIndex += 1;
      break;
    }

    if (((bIndex % 0x10) == 0) || (bIndex >= size))
    {
      int len = bIndex - bIndexLast;

      if (bIndex > size)
      {
        len = size - bIndexLast;
      }

      asciiToLeft(line, sizeof(line), &buffer[bIndexLast], len,
                  format, xx? 4 : 0);

      start     += 0x10;
      bIndexLast = bIndex;

      LM_RAW((line));
      memset(line, 0, sizeof(line));
    }
  }

  LM_RAW(("%c: --------------------------------------------------------------", type));

  return LmsOk;
}



/* ****************************************************************************
*
* lmReopen -
*/
LmStatus lmReopen(int index)
{
  int      fdPos;
  LmStatus s;
  int      newLogLines = 0;
  FILE*    fP;
  char     tmpName[512];
  int      fd;

  fdPos = lseek(fds[index].fd, 0, SEEK_CUR);

  if ((fP = fopen(fds[index].info, "r")) == NULL)
  {
    return LmsFopen;
  }

  rewind(fP);

  s = LmsOk;

  snprintf(tmpName, sizeof(tmpName), "%s_%d", fds[index].info, (int) getpid());

  if ((fd = open(tmpName, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
  {
    lseek(fds[index].fd, fdPos, SEEK_SET);
    fclose(fP);
    return LmsOpen;
  }

  while (true)
  {
    char* line = (char*) calloc(1, LM_LINE_MAX);
    int   len;

    if (fgets(line, LM_LINE_MAX, fP) == NULL)
    {
      s = LmsFgets;
      free(line);
      break;
    }

    len = strlen(line);

    if (strncmp(line,  "Cleared ", 8) == 0)
    {
      time_t     now = time(NULL);
      struct tm tmP;
      char       tm[80];
      char       buf[128];

      lm::gmtime_r(&now, &tmP);
      strftime(tm, 80, TIME_FORMAT_DEF, &tmP);

      snprintf(buf, sizeof(buf), "Cleared at %s  (a reopen ...)\n", tm);
      if (write(fd, buf, strlen(buf)) != (int) strlen(buf))
      {
        s = LmsWrite;
        free(line);
        break;
      }

      ++newLogLines;
      free(line);
      break;
    }

    if (write(fd, line, len) != len)
    {
      s = LmsWrite;
      free(line);
      break;
    }
    else
      ++newLogLines;
  }

  if (fd != -1)
  {
    if (close(fd) == -1)
    {
      perror("close");
    }
  }

  if (fclose(fP) == -1)
  {
    perror("fclose");
  }

  if (s == LmsOk)
  {
    close(fds[index].fd);
    rename(tmpName, fds[index].info);
    fds[index].fd = open(fds[index].info, O_RDWR, 0666);

    if (fds[index].fd == -1)
    {
      fds[index].state = Free;
      return LmsOpen;
    }

    lseek(fds[index].fd, 0, SEEK_END);
    logLines = newLogLines;
  }

  return s;
}



/* ****************************************************************************
*
* lmLogLineGet -
*/
int64_t lmLogLineGet
(
  char*     typeP,
  char*     dateP,
  int       dateLen,
  int*      msP,
  char*     progNameP,
  int       progNameLen,
  char*     fileNameP,
  int       fileNameLen,
  int*      lineNoP,
  int*      pidP,
  int*      tidP,
  char*     funcNameP,
  int       funcNameLen,
  char*     messageP,
  int       messageLen,
  int64_t   offset,
  char**    allP
)
{
  static FILE*  fP     = NULL;
  char*         line   = (char*) calloc(1, LM_LINE_MAX);
  char*         lineP  = line;
  char*         delimiter;
  int64_t       ret;

  if (allP != NULL)
  {
    *allP = NULL;
  }

  if (typeP == NULL)
  {
    if (fP != NULL)
    {
      fclose(fP);
    }

    fP = NULL;
    free(line);
    return 0;
  }

  if (fP == NULL)
  {
    // printf("opening log file\n");
    if ((fP = fopen(fds[0].info, "r")) == NULL)
    {
      // printf("error opening log file '%s'\n", fds[0].info);
      free(line);
      return -1;
    }

    // Set file pointer to the beginning of the fifth line
    char* fgP;

    fgP = fgets(line, 1024, fP);
    if (fgP != NULL) fgP = fgets(line, 1024, fP);
    if (fgP != NULL) fgP = fgets(line, 1024, fP);
    if (fgP != NULL) fgP = fgets(line, 1024, fP);
    if (fgP == NULL)
    {
      goto lmerror;
    }
  }
  else
  {
    // printf("seek to %lu\n", offset);
    if (fseek(fP, offset, SEEK_SET) == -1)
    {
      goto lmerror;
    }
  }

  if (fgets(line, LM_LINE_MAX, fP) == NULL)
  {
    fclose(fP);
    fP = NULL;
    free(line);
    return -2;  // EOF
  }

  if (line[strlen(line) - 1] == '\n')
  {
    line[strlen(line) - 1] = 0;
  }

  if (allP != NULL)
  {
    *allP = strdup(line);
  }

  *typeP = *lineP;

  lineP += 2;
  delimiter = strchr(lineP, '(');
  if (delimiter == NULL)
    goto lmerror;
  *delimiter = 0;
  snprintf(dateP, dateLen, "%s", lineP);
  lineP = delimiter + 1;

  delimiter = strchr(lineP, ')');
  if (delimiter == NULL)
    goto lmerror;
  *delimiter = 0;
  *msP = atoi(lineP);
  lineP = delimiter + 2;

  delimiter = strchr(lineP, '/');
  if (delimiter == NULL)
    goto lmerror;
  *delimiter = 0;
  snprintf(progNameP, progNameLen, "%s", lineP);
  lineP = delimiter + 1;

  delimiter = strchr(lineP, '[');
  if (delimiter == NULL)
    goto lmerror;
  *delimiter = 0;
  snprintf(fileNameP, fileNameLen, "%s", lineP);
  lineP = delimiter + 1;

  delimiter = strchr(lineP, ']');
  if (delimiter == NULL)
    goto lmerror;
  *delimiter = 0;
  *lineNoP = atoi(lineP);
  lineP = delimiter + 4;

  delimiter = strchr(lineP, ')');
  if (delimiter == NULL)
    goto lmerror;
  *delimiter = 0;
  *pidP = atoi(lineP);
  lineP = delimiter + 4;

  delimiter = strchr(lineP, ')');
  if (delimiter == NULL)
    goto lmerror;
  *delimiter = 0;
  *tidP = atoi(lineP);
  lineP = delimiter + 2;

  delimiter = strchr(lineP, ':');
  if (delimiter == NULL)
    goto lmerror;
  *delimiter = 0;
  snprintf(funcNameP, funcNameLen, "%s", lineP);
  lineP = delimiter + 2;

  snprintf(messageP, messageLen, "%s", lineP);

  ret = ftell(fP);
  // printf("Line parsed - returning %lu\n", ret);
  free(line);
  return ret;

 lmerror:
  free(line);
  fclose(fP);
  fP = NULL;

  // printf("Error in line: %s", lineP);
  return -1;
}



typedef struct LineRemove
{
  char      type;
  int       offset;
  bool      remove;
} LineRemove;



/* ****************************************************************************
*
* lmClear -
*/
LmStatus lmClear(int index, int keepLines, int lastLines)
{
  INIT_CHECK();
  INDEX_CHECK(index);

  LineRemove* lrV;
  void*       initialLrv;
  char*       line = (char*) calloc(1, LM_LINE_MAX);
  int         i;
  int         j = 0;
  FILE*       fP;
  int         oldOffset;
  int         linesToRemove;
  char*       order       = (char*) "rwbRTdtDVFMWEPBXh";
  int         newLogLines = 0;
  int         fdPos;
  char        tmpName[512];
  int         len;
  int         fd;
  static int  headerLines = 4;

  POINTER_CHECK(line);

  if (logLines < (keepLines + lastLines))
  {
    free(line);
    return LmsOk;
  }

  if ((fP = fopen(fds[index].info, "r")) == NULL)
  {
    atLines += 1000;
    if (atLines > 20000)
    {
      free(line);
      doClear = false;
      return LmsFopen;
    }

    free(line);
    return LmsFopen;
  }

  semTake();
  rewind(fP);

  lrV = (LineRemove*) malloc(sizeof(LineRemove) * (logLines + 4));
  if (lrV == NULL)
  {
    semGive();
    free(line);
    return LmsMalloc;
  }

  initialLrv = lrV;
  memset(lrV, 0, sizeof(LineRemove) * (logLines + 4));

  i = 0;
  oldOffset = 0;
  while (fgets(line, LM_LINE_MAX, fP) != NULL)
  {
    lrV[i].type   = (line[1] == ':')? line[0] : 'h';
    lrV[i].offset = oldOffset;
    lrV[i].remove = false;

    LOG_OUT(("got line %d: '%s'", i, line));
    oldOffset = ftell(fP);

    ++i;
    if (i > logLines + 4)
    {
      break;
    }
  }

  linesToRemove = logLines - headerLines - keepLines - lastLines;

  LOG_OUT(("linesToRemove == %d", linesToRemove));
  LOG_OUT(("logLines      == %d", logLines));
  LOG_OUT(("headerLines   == %d", headerLines));
  LOG_OUT(("keepLines     == %d", keepLines));
  LOG_OUT(("lastLines     == %d", lastLines));
  LOG_OUT(("keeping lastLines (%d - EOF)", logLines - lastLines));
  LOG_OUT(("Removing from %d to %d", headerLines, logLines - lastLines));

  while (order[j] != 0)
  {
    for (i = headerLines; i < logLines - lastLines; i++)
    {
      if ((lrV[i].remove == false) && (lrV[i].type == order[j]))
      {
        lrV[i].remove = true;
        LOG_OUT(("Removing line %d", i));
        if (--linesToRemove <= 0)
        {
          goto Removing;
        }
      }
    }
    ++j;
  }

 Removing:
  fdPos = ftell(fP);
  rewind(fP);
  snprintf(tmpName, sizeof(tmpName), "%s_%d", fds[index].info, (int) getpid());
  if ((fd = open(tmpName, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
  {
    perror("open");
    fclose(fP);
    lseek(fds[index].fd, fdPos, SEEK_SET);
    ::free((char*) lrV);
    semGive();
    return LmsOpen;
  }

#define CLEANUP(str, s)                    \
{                                          \
  if (str != NULL)                         \
  {                                        \
    perror(str);                           \
  }                                        \
                                           \
  close(fd);                               \
  fclose(fP);                              \
  lseek(fds[index].fd, fdPos, SEEK_SET);   \
  if (lrV != NULL)                         \
  {                                        \
    ::free((void*) lrV);                   \
  }                                        \
                                           \
  lrV = NULL;                              \
  unlink(tmpName);                         \
  free(line);                              \
  semGive();                               \
                                           \
  return s;                                \
}

  for (i = 0; i < logLines; i++)
  {
    if (lrV[i].remove == false)
    {
      char*  line  = (char*) calloc(1, LM_LINE_MAX);

      if (fseek(fP, lrV[i].offset, SEEK_SET) != 0)
      {
        CLEANUP("fseek", LmsFseek);
      }

      if (fgets(line, LM_LINE_MAX, fP) == NULL)
      {
        break;
      }

      if (strncmp(line, "Cleared at", 10) != 0)
      {
        len = strlen(line);
        if (write(fd, line, len) != len)
        {
          CLEANUP("write", LmsWrite);
        }
      }

      if (newLogLines == 2)
      {
        time_t     now = time(NULL);
        struct tm  tmP;
        char       tm[80];
        char       buf[128];

        lm::gmtime_r(&now, &tmP);
        strftime(tm, 80, TIME_FORMAT_DEF, &tmP);

        snprintf(buf, sizeof(buf), "Cleared at %s\n", tm);
        if (write(fd, buf, strlen(buf)) != (int) strlen(buf))
        {
          CLEANUP("write", LmsWrite);
        }
      }

      ++newLogLines;
      free(line);
      line = NULL;
    }
  }

  if (lrV != NULL)
  {
    if (lrV == initialLrv)
    {
      ::free(lrV);
    }
    else
    {
      LOG_OUT(("ERROR: lrV has changed ... (from 0x%lx to 0x%lx)", initialLrv, lrV));
    }
  }

  fclose(fP);
  close(fd);
  close(fds[index].fd);
  rename(tmpName, fds[index].info);
  fds[index].fd = open(fds[index].info, O_RDWR, 0666);

  if (fds[index].fd == -1)
  {
    fds[index].state = Free;
    semGive();
    if (line) free(line);
    return LmsOpen;
  }

  lseek(fds[index].fd, 0, SEEK_END);

  logLines = newLogLines;
  LOG_OUT(("Set logLines to %d", logLines));
  if (line) free(line);

  semGive();
  return LmsOk;
}



/* ****************************************************************************
*
* lmShowLog -
*/
void lmShowLog(int logFd)
{
  char com[512];

  snprintf(com, sizeof(com), "echo xterm -e tail -f %s & > /tmp/xxx", fds[logFd].info);

  if (system(com) == 0)
  {
    chmod("/tmp/xxx", 0700);

    if (system("/tmp/xxx") != 0)
    {
      printf("error in call to 'system'\n");
    }
  }
}



/* ****************************************************************************
*
* lmExitForced -
*/
void lmExitForced(int c)
{
#ifdef ASSERT_FOR_EXIT
  assert(false);
#endif

  if (exitFunction != NULL)
  {
    exitFunction(c, exitInput, (char*) "exit forced", NULL);
  }

  exit(c);
}



/* ****************************************************************************
*
* lmxFunction - extra function to call for LMX_ macros
*/
LmStatus lmxFunction(LmxFp fp)
{
  INIT_CHECK();
  POINTER_CHECK(fp);

  lmxFp = fp;

  return LmsOk;
}



/* ****************************************************************************
*
* lmOnlyErrors
*/
LmStatus lmOnlyErrors(int index)
{
  INIT_CHECK();
  INDEX_CHECK(index);

  if (fds[index].type == Stdout)
  {
    fds[index].onlyErrorAndVerbose = true;
  }

  return LmsOk;
}



/* ****************************************************************************
*
* lmTraceLevel -
*/
const char* lmTraceLevel(int level)
{
  char* userName = NULL;

  switch (level)
  {
  case LmtIn:              return "In function";
  case LmtFrom:            return "From function";
  case LmtLine:            return "Line number";
  case LmtEntry:           return "Function Entry";
  case LmtExit:            return "Function Exit";
  case LmtListCreate:      return "List Create";
  case LmtListInit:        return "List Init";
  case LmtListInsert:      return "List Insert";
  case LmtListItemCreate:  return "List Item Create";
  }

  if (userTracelevelName != NULL)
  {
    userName = userTracelevelName(level);
  }

  if (userName == NULL)
  {
    static char name[32];
    snprintf(name, sizeof(name), "trace level %d", level);
    return name;
  }

  return (const char*) userName;
}



/* ****************************************************************************
*
* lmSdGet -
*/
int lmSdGet(void)
{
  return lmSd;
}


struct logMsg
{
  char msg[256];
  char type;
  char file[256];
  int  line;
  char func[256];
  int  tLev;
  char stre[256];
  char transactionId[64];
  char correlatorId[64];
  struct logMsg* next;
};


static struct logMsg* logMsgs = NULL;



/* ****************************************************************************
*
* lmAddMsgBuf - This funtion was added because of the lack of reentrancy of
* certain date functions (called from within signal handlers).
*/
void lmAddMsgBuf
(
  char*        text,
  char         type,
  const char*  file,
  int          line,
  const char*  func,
  int          tLev,
  const char*  stre
)
{
  struct logMsg* newMsg;
  struct logMsg* logP;

  newMsg = (logMsg*) malloc(sizeof(struct logMsg));
  if (newMsg == NULL)
  {
    return;
  }

  memset(newMsg, 0, sizeof(struct logMsg));

  if (text)
  {
    strncpy(newMsg->msg, text, sizeof(newMsg->msg));
  }

  if (file)
  {
    strncpy(newMsg->file, file, sizeof(newMsg->file));
  }

  newMsg->line = line;
  newMsg->type = type;

  if (func)
  {
    strncpy(newMsg->func, func, sizeof(newMsg->func));
  }

  if (stre)
  {
    strncpy(newMsg->stre, stre, sizeof(newMsg->stre));
  }

  if (tLev)
  {
    newMsg->tLev = tLev;
  }

  if (logMsgs)
  {
    struct logMsg* lastlogP;

    logP = logMsgs;
    while (logP)
    {
      lastlogP = logP;
      logP     = logP->next;
    }
    lastlogP->next = newMsg;
    newMsg->next   = NULL;
  }
  else
  {
    logMsgs        = newMsg;
    newMsg->next   = NULL;
  }
}



/* ****************************************************************************
*
* lmPrintMsgBuf -
*/
void lmPrintMsgBuf()
{
  struct logMsg *logP;

  while (logMsgs)
  {
    logP = logMsgs;
    strncpy(transactionId, logP->transactionId, sizeof(transactionId));
    lmOut(logP->msg, logP->type, logP->file, logP->line, (char*) logP->func, logP->tLev, logP->stre, false);
    logMsgs = logP->next;
    ::free(logP);
  }
}



/* ****************************************************************************
*
* lmFirstDiskFileDescriptor -
*/
int lmFirstDiskFileDescriptor(void)
{
  int ix = 0;

  for (ix = 0; ix < FDS_MAX; ix++)
  {
    if (fds[ix].type == Fichero)
    {
      return fds[ix].fd;
    }
  }

  return -1;
}



/* ****************************************************************************
*
* lmLogLinesGet -
*/
int lmLogLinesGet(void)
{
  return logLines;
}
