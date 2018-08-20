#ifndef SRC_LIB_LOGMSG_LOGMSG_H_
#define SRC_LIB_LOGMSG_LOGMSG_H_

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
#include <stdio.h>              /* !sprintf                                  */

#if !defined(__APPLE__)
#include <malloc.h>             /* free                                      */
#endif

#include <errno.h>              /* errno                                     */
#include <string.h>             /* strerror                                  */
#include <stdarg.h>             /* ellipses                                  */
#include <stdlib.h>             /* free()                                    */
#include <time.h>
#include <stdint.h>             /* int64, ...                                */

#include <string>               /* std::string                               */

#include "common/globals.h"     /* transactionIdSet,correlatorIdSet          */
#include "common/limits.h"      // FIXME: this should be removed if this library wants to be generic again



/******************************************************************************
*
* globals
*/
extern int             inSigHandler;
extern char*           progName;
extern __thread char   transactionId[64];
extern __thread char   correlatorId[64];



/* ****************************************************************************
*
* LmStatus -
*/
typedef enum LmStatus
{
  LmsOk = 0,

  LmsInitNotDone,
  LmsInitAlreadyDone,
  LmsNull,
  LmsFdNotFound,
  LmsFdInvalid,
  LmsFdOccupied,

  LmsMalloc,
  LmsOpen,
  LmsFopen,
  LmsLseek,
  LmsFseek,
  LmsWrite,
  LmsFgets,

  LmsNoFiles,
  LmsProgNameLevels,
  LmsLineTooLong,
  LmsStringTooLong,
  LmsBadFormat,
  LmsBadSize,
  LmsBadParams,
  LmsPrognameNotSet,
  LmsPrognameError,
  LmsClearNotAllowed
} LmStatus;



/* ****************************************************************************
*
* LmFormat - format for buffer presenting function
*/
typedef enum LmFormat
{
  LmfByte = 1,
  LmfWord = 2,
  LmfLong = 4
} LmFormat;



/* ****************************************************************************
*
* LmExitFp - function type for the exit function
*/
typedef void (*LmExitFp)(int code, void* input, char* logLine, char* stre);
typedef void (*LmWarningFp)(void* input, char* logLine, char* stre);
typedef void (*LmErrorFp)(void* input, char* logLine, char* stre);



/* ****************************************************************************
*
* LmWriteFp - alternative write function type
*/
typedef void (*LmWriteFp)(char* callback);



/* ****************************************************************************
*
* LmOutHook - type for function pointer for lmOut hook
*/
typedef void (*LmOutHook)
(
  void*        vP,
  char*        text,
  char         type,
  time_t       secondsNow,
  int          timezone,
  int          dst,
  const char*  file,
  int          lineNo,
  const char*  fName,
  int          tLev,
  const char*  stre
);



/* ****************************************************************************
*
* LmxFp - extra log function to call (for SGS use)
*/
typedef int (*LmxFp)(int, char*);



/* ****************************************************************************
*
* Predefined trace levels
*/
typedef enum LmTraceLevel
{
  LmtIn = 0,
  LmtFrom,
  LmtEntry,
  LmtExit,
  LmtLine,

  /* For listLib */
  LmtListCreate,
  LmtListInit,
  LmtListInsert,
  LmtListItemCreate,

  LmtUserStart
} LmTraceLevel;



/* ****************************************************************************
*
* LmTracelevelName -
*/
typedef char* (*LmTracelevelName)(int level);



#define _LM_MAGIC (('z' << 24) | ('u' << 16) | ('k' << 8) | 'a')
/* ****************************************************************************
*
* LogHeader -
*/
typedef struct LogHeader
{
  int magic;     // LOG_MAGIC: "zuka" (akuz in little-endian ...)
  int dataLen;   // Length of data part of message

  bool checkMagicNumber(void)
  {
    return magic == _LM_MAGIC;
  }

  void setMagicNumber(void)
  {
    magic = _LM_MAGIC;
  }
} LogHeader;



/* ****************************************************************************
*
* LogData - 
*/
typedef struct LogData
{
  int             lineNo;      // Line number in file
  char            traceLevel;  // Tracelevel, in case of 'LM_T'
  char            type;        // type of message 'M', 'E', 'W', ...
  struct timeval  tv;          // time since 1970
  int             timezone;    // The timezone
  int             dst;         // Type of Daylight Saving Time
  pid_t           pid;         // pid of the process
  pid_t           tid;         // Identifier of the thread
} LogData;



/* ****************************************************************************
*
* LogMsg - 
*/
typedef struct LogMsg
{
  LogHeader  header;  // header ...
  LogData    data;    // Integer part of log message
} LogMsg;



/* ****************************************************************************
*
* LogLevelMask - 
*/
typedef enum LogLevelMask
{
  LogLevelExit       = 0x40000,
  LogLevelError      = 0x20000,
  LogLevelWarning    = 0x10000,
  LogLevelForce      = 0x08000,
  LogLevelMsg        = 0x04000,
  LogLevelInfo       = 0x02000,
  LogLevelVerbose    = 0x01000,
  LogLevelDebug      = 0x00800,
  LogLevelTrace      = 0x00400,
  LogLevelHidden     = 0x00200,
  LogLevelTimestamp  = 0x00100,
  LogLevelReads      = 0x00080,
  LogLevelWrites     = 0x00040,
  LogLevelBuf        = 0x00020,
  LogLevelFix        = 0x00010,
  LogLevelRaw        = 0x00008,
  LogLevelToDo       = 0x00004,
  LogLevelDoubt      = 0x00002,
  LogLevelBug        = 0x00001
} LogLevelMask;

extern int         lmLevelMask;
extern void        lmLevelMaskSet(int levelMask);
extern void        lmLevelMaskSetString(char* level);
extern int         lmLevelMaskGet(void);
extern std::string lmLevelMaskStringGet(void);



/* ****************************************************************************
*
*
*/
#ifdef LM_OFF
  #define LM_NO_V
  #define LM_NO_M
  #define LM_NO_H
  #define LM_NO_W
  #define LM_NO_E
  #define LM_NO_P
  #define LM_NO_X
  #define LM_NO_XP
  #ifdef LM_ON
    #undef LM_ON
  #endif
#endif

#ifdef LM_NO_V
  #define LM_V(s)
  #define LM_VV(s)
  #define LM_VVV(s)
  #define LM_VVVV(s)
  #define LM_VVVVV(s)
  #define LM_V2(s)
  #define LM_V3(s)
  #define LM_V4(s)
  #define LM_V5(s)
  #define LM_LV(s)

#else

#define LM_MASK(level) ((lmLevelMask & level) == level)
/* ****************************************************************************
*
* LM_V - log verbose message
*/
#define LM_V(s)                                                            \
do                                                                         \
{                                                                          \
  if (LM_MASK(LogLevelVerbose) && lmOk('V', 0) == LmsOk)                   \
  {                                                                        \
    char* text;                                                            \
                                                                           \
    if ((text = lmTextGet s) != NULL)                                      \
    {                                                                      \
      lmOut(text, 'V', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
      ::free(text);                                                        \
    }                                                                      \
  }                                                                        \
} while (0)

#define LM_V2(s)                                                           \
do                                                                         \
{                                                                          \
  if (LM_MASK(LogLevelVerbose) && lmOk('2', 0) == LmsOk)                   \
  {                                                                        \
    char* text;                                                            \
                                                                           \
    if ((text = lmTextGet s) != NULL)                                      \
    {                                                                      \
      lmOut(text, '2', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
      ::free(text);                                                        \
    }                                                                      \
  }                                                                        \
} while (0)


#define LM_V3(s)                                                           \
do                                                                         \
{                                                                          \
  if (LM_MASK(LogLevelVerbose) && lmOk('3', 0) == LmsOk)                   \
  {                                                                        \
    char* text;                                                            \
                                                                           \
    if ((text = lmTextGet s) != NULL)                                      \
    {                                                                      \
      lmOut(text, '3', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
      ::free(text);                                                        \
    }                                                                      \
  }                                                                        \
} while (0)


#define LM_V4(s)                                                           \
do                                                                         \
{                                                                          \
  if (LM_MASK(LogLevelVerbose) && lmOk('4', 0) == LmsOk)                   \
  {                                                                        \
    char* text;                                                            \
                                                                           \
    if ((text = lmTextGet s) != NULL)                                      \
    {                                                                      \
      lmOut(text, '4', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
      ::free(text);                                                        \
    }                                                                      \
  }                                                                        \
} while (0)


#define LM_V5(s)                                                           \
do                                                                         \
{                                                                          \
  if (LM_MASK(LogLevelVerbose) && lmOk('5', 0) == LmsOk)                   \
  {                                                                        \
    char* text;                                                            \
                                                                           \
    if ((text = lmTextGet s) != NULL)                                      \
    {                                                                      \
      lmOut(text, '5', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
      ::free(text);                                                        \
    }                                                                      \
  }                                                                        \
} while (0)

#define LM_LV(s)                                                                 \
do                                                                               \
{                                                                                \
  if (LM_MASK(LogLevelVerbose) && lmOk('V', 0) == LmsOk)                         \
  {                                                                              \
    char* text;                                                                  \
                                                                                 \
    if ((text = lmTextGet s) != NULL)                                            \
    {                                                                            \
      lmOut(text, 'V', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL, false);\
      ::free(text);                                                              \
    }                                                                            \
  }                                                                              \
} while (0)


#define LM_VV     LM_V2
#define LM_VVV    LM_V3
#define LM_VVVV   LM_V4
#define LM_VVVVV  LM_V5
#endif

/* ****************************************************************************
*
* LM_LM - log message ( only local ) // No hook function
*/
#define LM_LM(s)                                                                 \
do                                                                               \
{                                                                                \
  if ((char* text = lmTextGet s) != NULL)                                        \
  {                                                                              \
    lmOut(text, 'M', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL, false);  \
    ::free(text);                                                                \
  }                                                                              \
} while (0)

/* ****************************************************************************
*
* LM_LW - log warning message ( only local ) // No hook function
*/
#define LM_LW(s)                                                               \
do                                                                             \
{                                                                              \
  if ((char* text = lmTextGet s) != NULL)                                      \
  {                                                                            \
    lmOut(text, 'W', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL, false);\
    ::free(text);                                                              \
  }                                                                            \
} while (0)



#ifdef LM_NO_M
#define LM_M(s)
#else
/* ****************************************************************************
*
* LM_M - log message
*/
#define LM_M(s)                                                           \
do                                                                        \
{                                                                         \
  char* text;                                                             \
                                                                          \
  if (LM_MASK(LogLevelMsg) && (text = lmTextGet s) != NULL)               \
  {                                                                       \
    lmOut(text, 'M', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL);  \
    ::free(text);                                                         \
  }                                                                       \
} while (0)
#endif


#ifdef LM_NO_I
#define LM_I(s)
#else
/* ****************************************************************************
*
* LM_I - log info
*/
#define LM_I(s)                                                           \
do                                                                        \
{                                                                         \
  char* text;                                                             \
                                                                          \
  if (LM_MASK(LogLevelInfo) && (text = lmTextGet s) != NULL)              \
  {                                                                       \
    lmOut(text, 'I', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL);  \
    ::free(text);                                                         \
  }                                                                       \
} while (0)
#endif



#ifdef LM_NO_S
#define LM_S(s)
#else
/* ****************************************************************************
*
* LM_S - log summary
*/
#define LM_S(s)                                                           \
do                                                                        \
{                                                                         \
  char* text;                                                             \
                                                                          \
  if ((text = lmTextGet s) != NULL)                                       \
  {                                                                       \
    lmOut(text, 'S', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL);  \
    ::free(text);                                                         \
  }                                                                       \
} while (0)
#endif



#ifdef LM_NO_H
#define LM_H(s)
#else
/* ****************************************************************************
*
* LM_H - "hidden" log message
*/
#define LM_H(s)                                                           \
do                                                                        \
{                                                                         \
  char* text;                                                             \
                                                                          \
  if (LM_MASK(LogLevelHidden) && (text = lmTextGet s) != NULL)            \
  {                                                                       \
    lmOut(text, 'H', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL);  \
    ::free(text);                                                         \
  }                                                                       \
} while (0)
#endif


#ifdef LM_NO_F
#define LM_F(s)
#else
/* ****************************************************************************
*
* LM_F - log message
*/
#define LM_F(s)                                                        \
do                                                                     \
{                                                                      \
  char* text;                                                          \
                                                                       \
  if (LM_MASK(LogLevelForce) && (text = lmTextGet s) != NULL)          \
  {                                                                    \
    lmOut(text, 'F', "ForcedLog", 0, "***", 0, NULL);                  \
    ::free(text);                                                      \
  }                                                                    \
} while (0)
#endif


#ifdef LM_NO_TMP
#define LM_TMP(s)
#else
/* ****************************************************************************
*
* LM_TMP - temporal log message
*
* LM_TMP is meant *only* for temporal logging and all occurrencies of LM_TMP
* should be removed before creating pull requests for review.
*/
#define LM_TMP(s)                                                        \
do                                                                       \
{                                                                        \
  char* text;                                                            \
                                                                         \
  if (LM_MASK(LogLevelWarning) && (text = lmTextGet s) != NULL)          \
  {                                                                      \
    lmOut(text, 'W', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
    ::free(text);                                                        \
  }                                                                      \
} while (0)
#endif


#ifdef LM_NO_W
#define LM_W(s)
#else
/* ****************************************************************************
*
* LM_W - log warning message
*/
#define LM_W(s)                                                          \
do                                                                       \
{                                                                        \
  char* text;                                                            \
                                                                         \
  if (LM_MASK(LogLevelWarning) && (text = lmTextGet s) != NULL)          \
  {                                                                      \
    lmOut(text, 'W', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
    ::free(text);                                                        \
  }                                                                      \
} while (0)
#endif



#ifdef LM_NO_E
#define LM_E(s)
#else
/* ****************************************************************************
*
* LM_E - log error message
*/
#define LM_E(s)                                                           \
do                                                                        \
{                                                                         \
  char* text;                                                             \
                                                                          \
  if (LM_MASK(LogLevelError) && (text = lmTextGet s) != NULL)             \
  {                                                                       \
    lmOut(text, 'E', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL);  \
    ::free(text);                                                         \
  }                                                                       \
} while (0)

#define LM_LE(s)                                                               \
do                                                                             \
{                                                                              \
  char* text;                                                                  \
                                                                               \
  if ((text = lmTextGet s) != NULL)                                            \
  {                                                                            \
    lmOut(text, 'E', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL, false);\
    ::free(text);                                                              \
  }                                                                            \
} while (0)

#endif


#ifdef LM_NO_P
#define LM_P(s)
#else
/* ****************************************************************************
*
* LM_P - log perror message
*/
#define LM_P(s)                                                          \
do                                                                       \
{                                                                        \
  char* text;                                                            \
  char  stre[128];                                                       \
                                                                         \
  snprintf(stre, sizeof(stre), "%s", strerror(errno));                   \
  if ((text = lmTextGet s) != NULL)                                      \
  {                                                                      \
    lmOut(text, 'P', __FILE__, __LINE__, (char*) __FUNCTION__, 0, stre); \
    ::free(text);                                                        \
  }                                                                      \
} while (0)
#endif


/* LMS - signal handler-safe log message functions */

#ifdef LM_NO_V
#define LMS_V(s)
#else
/* ****************************************************************************
*
* LMS_V - log verbose message
*/
#define LMS_V(s)                                                                  \
do                                                                                \
{                                                                                 \
  if (lmOk('V', 0) == LmsOk)                                                      \
  {                                                                               \
    char* text;                                                                   \
                                                                                  \
    if ((text = lmTextGet s) != NULL)                                             \
    {                                                                             \
      lmAddMsgBuf(text, 'V', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL);  \
      ::free(text);                                                               \
    }                                                                             \
  }                                                                               \
} while (0)
#endif


#ifdef LM_NO_M
#define LMS_M(s)
#else
/* ****************************************************************************
*
* LMS_M - log message
*/
#define LMS_M(s)                                                               \
do                                                                             \
{                                                                              \
  char* text;                                                                  \
                                                                               \
  if ((text = lmTextGet s) != NULL)                                            \
  {                                                                            \
    lmAddMsgBuf(text, 'M', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
    ::free(text);                                                              \
  }                                                                            \
} while (0)
#endif


#ifdef LM_NO_F
#define LMS_F(s)
#else
/* ****************************************************************************
*
* LMS_F - log message
*/
#define LMS_F(s)                                                      \
do                                                                    \
{                                                                     \
  char* text;                                                         \
                                                                      \
  if ((text = lmTextGet s) != NULL)                                   \
  {                                                                   \
    lmAddMsgBuf(text, 'F', __FILE__, __LINE__, "FFF", 0, NULL);       \
    ::free(text);                                                     \
  }                                                                   \
} while (0)
#endif


#ifdef LM_NO_W
#define LMS_W(s)
#else
/* ****************************************************************************
*
* LMS_W - log warning message
*/
#define LMS_W(s)                                                               \
do                                                                             \
{                                                                              \
  char* text;                                                                  \
                                                                               \
  if ((text = lmTextGet s) != NULL)                                            \
  {                                                                            \
    lmAddMsgBuf(text, 'W', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
    ::free(text);                                                              \
  }                                                                            \
} while (0)
#endif



#ifdef LM_NO_E
#define LMS_E(s)
#else
/* ****************************************************************************
*
* LMS_E - log error message
*/
#define LMS_E(s)                                                               \
do                                                                             \
{                                                                              \
  char* text;                                                                  \
                                                                               \
  if ((text = lmTextGet s) != NULL)                                            \
  {                                                                            \
    lmAddMsgBuf(text, 'E', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
    ::free(text);                                                              \
  }                                                                            \
} while (0)
#endif

#ifdef LM_NO_P
#define LMS_P(s)
#else
/* ****************************************************************************
*
* LMS_P - log perror message
*/
#define LMS_P(s)                                                               \
do                                                                             \
{                                                                              \
  char* text;                                                                  \
  char  stre[128];                                                             \
                                                                               \
  snprintf(stre, sizeof(stre), "%s", strerror(errno));                         \
  if ((text = lmTextGet s) != NULL)                                            \
  {                                                                            \
    lmAddMsgBuf(text, 'P', __FILE__, __LINE__, (char*) __FUNCTION__, 0, stre); \
    ::free(text);                                                              \
  }                                                                            \
} while (0)
#endif


/* ****************************************************************************
*
* LMS_T - log trace message
*/
#define LMS_T(tLev, s)                                                              \
do                                                                                  \
{                                                                                   \
  if (lmOk('T', tLev) == LmsOk)                                                     \
  {                                                                                 \
    char* text;                                                                     \
                                                                                    \
    if ((text = lmTextGet s) != NULL)                                               \
    {                                                                               \
      lmAddMsgBuf(text, 'T', __FILE__, __LINE__, (char*) __FUNCTION__, tLev, NULL); \
      ::free(text);                                                                 \
    }                                                                               \
  }                                                                                 \
} while (0)


/* ****************************************************************************
*
* LM_RE - log error message and return with code
*/
#define LM_RE(code, s)           \
do                               \
{                                \
  LM_E(s);                       \
  if (1 == 1) return code;       \
} while (0)



/* ****************************************************************************
*
* LM_RP - log perror message and return with code
*/
#define LM_RP(code, s)           \
do                               \
{                                \
  LM_P(s);                       \
  if (1 == 1) return code;       \
} while (0)



/* ****************************************************************************
*
* LM_RVE - log error message and return with void
*/
#define LM_RVE(s)           \
do                          \
{                           \
  LM_E(s);                  \
  if (1 == 1) return;       \
} while (0)



/* ****************************************************************************
*
* LM_RVP - log perror message and return with void
*/
#define LM_RVP(s)           \
do                          \
{                           \
  LM_P(s);                  \
  if (1 == 1) return;       \
} while (0)



#ifdef LM_NO_X
#define LM_X(c, s)  exit(c)
#else
/* ****************************************************************************
*
* LM_X - log error message and exit with code
*/
#define LM_X(c, s)                                                       \
do                                                                       \
{                                                                        \
  char* text;                                                            \
                                                                         \
  if ((text = lmTextGet s) != NULL)                                      \
  {                                                                      \
    lmOut(text, 'X', __FILE__, __LINE__, (char*) __FUNCTION__, c, NULL); \
    ::free(text);                                                        \
  }                                                                      \
} while (0)

#define LM_LX(c, s)                                                             \
do                                                                              \
{                                                                               \
  char* text;                                                                   \
                                                                                \
  if ((text = lmTextGet s) != NULL)                                             \
  {                                                                             \
    lmOut(text, 'X', __FILE__, __LINE__, (char*) __FUNCTION__, c, NULL, false); \
    ::free(text);                                                               \
  }                                                                             \
} while (0)

#endif



#ifdef LM_NO_XP
#define LM_XP(c, s)  exit(c)
#else
/* ****************************************************************************
*
* LM_XP - log perror message and exit with code
*/
#define LM_XP(c, s)                                                      \
do                                                                       \
{                                                                        \
  char* text;                                                            \
  char  stre[128];                                                       \
                                                                         \
  snprintf(stre, sizeof(stre), "%s", strerror(errno));                   \
  if ((text = lmTextGet s) != NULL)                                      \
  {                                                                      \
    lmOut(text, 'x', __FILE__, __LINE__, __FUNCTION__, c, stre);         \
    ::free(text);                                                        \
  }                                                                      \
} while (0)
#endif


#ifndef LM_ON
#  define LM_TODO(s)
#  define LM_DOUBT(s)
#  define LM_FIX(s)
#  define LM_T(tLev, s)
#  define LM_D(s)
#  define LM_RAW(s)
#  define LM_IN(s)
#  define LM_FROM(s)
#  define LM_LINE()
#  define LM_MLINE()
#  define LM_ENTRY()
#  define LM_EXIT()
#  define LM_BUG(s)
#  define LM_READS(_to, _desc, _buf, _sz, _form)
#  define LM_WRITES(_to, _desc, _buf, _sz, _form)
#  define LM_BUF(_desc, _buf, _sz, _form)
#else



/* ****************************************************************************
*
* LM_TODO - log todo message
*/
#define LM_TODO(s)                                                         \
do                                                                         \
{                                                                          \
  if (LM_MASK(LogLevelToDo) && (lmOk('t', 0) == LmsOk))                    \
  {                                                                        \
    char* text;                                                            \
                                                                           \
    if ((text = lmTextGet s) != NULL)                                      \
    {                                                                      \
      lmOut(text, 't', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
      ::free(text);                                                        \
    }                                                                      \
  }                                                                        \
} while (0)



/* ****************************************************************************
*
* LM_DOUBT - log doubt message
*/
#define LM_DOUBT(s)                                                      \
do                                                                       \
{                                                                        \
  char* text;                                                            \
                                                                         \
  if (LM_MASK(LogLevelDoubt) && (text = lmTextGet s) != NULL)            \
  {                                                                      \
    lmOut(text, 'd', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
    ::free(text);                                                        \
  }                                                                      \
} while (0)



/* ****************************************************************************
*
* LM_FIX - log fix message
*/
#define LM_FIX(s)                                                        \
do                                                                       \
{                                                                        \
  char* text;                                                            \
                                                                         \
  if (LM_MASK(LogLevelFix) && (text = lmTextGet s) != NULL)              \
  {                                                                      \
    lmOut(text, 'F', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
    ::free(text);                                                        \
  }                                                                      \
} while (0)



/* ****************************************************************************
*
* LM_BUG - log bug message
*/
#define LM_BUG(s)                                                        \
do                                                                       \
{                                                                        \
  char* text;                                                            \
                                                                         \
  if (LM_MASK(LogLevelBug) && (text = lmTextGet s) != NULL)              \
  {                                                                      \
    lmOut(text, 'B', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
    ::free(text);                                                        \
  }                                                                      \
} while (0)



/* ****************************************************************************
*
* LM_T - log trace message
*
* FIXME: temporal change, just for Orion contextBroker, use LogLevelDebug for LM_T
*        instead of its correct level LogLevelTrace.
*/
#define LM_T(tLev, s)                                                         \
do                                                                            \
{                                                                             \
  if (LM_MASK(LogLevelDebug) && lmOk('T', tLev) == LmsOk)                     \
  {                                                                           \
    char* text;                                                               \
                                                                              \
    if ((text = lmTextGet s) != NULL)                                         \
    {                                                                         \
      lmOut(text, 'T', __FILE__, __LINE__, (char*) __FUNCTION__, tLev, NULL); \
      ::free(text);                                                           \
    }                                                                         \
  }                                                                           \
} while (0)

#define LM_LT(tLev, s)                                                               \
do                                                                                   \
{                                                                                    \
  if (LM_MASK(LogLevelTrace) && lmOk('T', tLev) == LmsOk)                            \
  {                                                                                  \
    char* text;                                                                      \
                                                                                     \
    if ((text = lmTextGet s) != NULL)                                                \
    {                                                                                \
      lmOut(text, 'T', __FILE__, __LINE__, (char*) __FUNCTION__, tLev, NULL, false); \
      ::free(text);                                                                  \
    }                                                                                \
  }                                                                                  \
} while (0)



/* ****************************************************************************
*
* LM_D - log debug message
*/
#define LM_D(s)                                                            \
do                                                                         \
{                                                                          \
  char* text;                                                              \
                                                                           \
  if (LM_MASK(LogLevelDebug) && lmOk('D', 0) == LmsOk)                     \
  {                                                                        \
    if ((text = lmTextGet s) != NULL)                                      \
    {                                                                      \
      lmOut(text, 'D', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
      ::free(text);                                                        \
    }                                                                      \
  }                                                                        \
} while (0)



/* ****************************************************************************
*
* LM_RAW - log raw message
*/
#define LM_RAW(s)                                                     \
do                                                                    \
{                                                                     \
  char* text;                                                         \
                                                                      \
  if (LM_MASK(LogLevelRaw) && (text = lmTextGet s) != NULL)           \
  {                                                                   \
    lmOut(text, 'R', __FILE__, __LINE__, NULL, 0, NULL);              \
    ::free(text);                                                     \
  }                                                                   \
} while (0)



/* ****************************************************************************
*
* LM_LINE - line number trace macro
*/
#define LM_LINE()  LM_T(LmtLine, ("line %d", __LINE__))


/* ****************************************************************************
*
* LM_MLINE - line number trace macro
*/
#define LM_MLINE()   LM_M(("line %d", __LINE__))


/* ****************************************************************************
*
* LM_IN - in function trace macro
*/
#define LM_IN(s)   LM_T(LmtIn, s)



/* ****************************************************************************
*
* LM_FROM - from function trace macro
*/
#define LM_FROM(s)   LM_T(LmtFrom, s)



/* ****************************************************************************
*
* LM_ENTRY - in function trace macro
*/
#define LM_ENTRY()   LM_T(LmtEntry, ("Entering function %s", (char*) __FUNCTION__))



/* ****************************************************************************
*
* LM_EXIT - exit function trace macro
*/
#define LM_EXIT()   LM_T(LmtExit, ("Leaving function %s", (char*) __FUNCTION__))



/* ****************************************************************************
*
* LM_READS - read buffer presentation
*/
#define LM_READS(_from, _desc, _buf, _sz, _form)  \
  lmBufferPresent((char*) _from, (char*) _desc, _buf, _sz, _form, 'r')




/* ****************************************************************************
*
* LM_BUF - buffer presentation
*/
#define LM_BUF(_desc, _buf, _sz, _form)  \
  lmBufferPresent(NULL, _desc, _buf, _sz, _form, 'b')




/* ****************************************************************************
*
* LM_WRITES - written buffer presentation
*/
#define LM_WRITES(_to, _desc, _buf, _sz, _form)  \
  lmBufferPresent((char*) _to, (char*) _desc, (char*) _buf, _sz, _form, 'w')

#endif



/* ****************************************************************************
*
* LMX_E - 
*/
#define LMX_E(xCode, s)                                                  \
do                                                                       \
{                                                                        \
  char* text;                                                            \
                                                                         \
  if ((text = lmTextGet s) != NULL)                                      \
  {                                                                      \
    lmOut(text, 'E', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
    if (lmxFp != NULL)                                                   \
      lmxFp(xCode, text);                                                \
    ::free(text);                                                        \
  }                                                                      \
} while (0)



/* ****************************************************************************
*
* LMX_RE - 
*/
#define LMX_RE(xCode, rCode, s)                                          \
do                                                                       \
{                                                                        \
  char* text;                                                            \
                                                                         \
  if ((text = lmTextGet s) != NULL)                                      \
  {                                                                      \
    lmOut(text, 'E', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
    if (lmxFp != NULL)                                                   \
      lmxFp(xCode, text);                                                \
    ::free(text);                                                        \
  }                                                                      \
  if (1 == 1) return rCode;                                              \
} while (0)



/* ****************************************************************************
*
* LMX_X - log error message and exit with code
*/
#define LMX_X(xCode, eCode, s)                                               \
do                                                                           \
{                                                                            \
  char* text;                                                                \
                                                                             \
  if ((text = lmTextGet s) != NULL)                                          \
  {                                                                          \
    lmOut(text, 'X', __FILE__, __LINE__, (char*) __FUNCTION__, eCode, NULL); \
    if (lmxFp != NULL) lmxFp(xCode, text);                                   \
    ::free(text);                                                            \
  }                                                                          \
} while (0)



/* ****************************************************************************
*
* LMX_W - log warning message
*/
#define LMX_W(xCode, s)                                                  \
do                                                                       \
{                                                                        \
  char* text;                                                            \
                                                                         \
  if (LM_MASK(LogLevelWarning) && (text = lmTextGet s) != NULL)          \
  {                                                                      \
    lmOut(text, 'W', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
    if (lmxFp != NULL) lmxFp(xCode, text);                               \
    ::free(text);                                                        \
  }                                                                      \
} while (0)



/* ****************************************************************************
*
* LMX_M - log message
*/
#define LMX_M(xCode, s)                                                  \
do                                                                       \
{                                                                        \
  char* text;                                                            \
                                                                         \
  if (LM_MASK(LogLevelMsg) && (text = lmTextGet s) != NULL)              \
  {                                                                      \
    lmOut(text, 'M', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
    if (lmxFp != NULL) lmxFp(xCode, text);                               \
    ::free(text);                                                        \
  }                                                                      \
} while (0)




/* ****************************************************************************
*
* LMX_V - log verbose message
*/
#define LMX_V(xCode, s)                                                    \
do                                                                         \
{                                                                          \
  char* text;                                                              \
                                                                           \
  if (LM_MASK(LogLevelVerbose) && lmOk('V', 0) == LmsOk)                   \
  {                                                                        \
    if ((text = lmTextGet s) != NULL)                                      \
    {                                                                      \
      lmOut(text, 'V', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
      if (lmxFp != NULL) lmxFp(xCode, text);                               \
      ::free(text);                                                        \
    }                                                                      \
  }                                                                        \
} while (0)



/* ****************************************************************************
*
* LMX_D - log message
*/
#define LMX_D(xCode, s)                                                    \
do                                                                         \
{                                                                          \
  char* text;                                                              \
                                                                           \
  if (LM_MASK(LogLevelDebug) && lmOk('D', 0) == LmsOk)                     \
  {                                                                        \
    if ((text = lmTextGet s) != NULL)                                      \
    {                                                                      \
      lmOut(text, 'D', __FILE__, __LINE__, (char*) __FUNCTION__, 0, NULL); \
      if (lmxFp != NULL) lmxFp(xCode, text);                               \
      ::free(text);                                                        \
    }                                                                      \
  }                                                                        \
} while (0)



/* ****************************************************************************
*
* LMX_T - log trace message
*/
#define LMX_T(xCode, tLev, s)                                                 \
do                                                                            \
{                                                                             \
  char* text;                                                                 \
                                                                              \
  if (LM_MASK(LogLevelTrace) && lmOk('T', tLev) == LmsOk)                     \
  {                                                                           \
    if ((text = lmTextGet s) != NULL)                                         \
    {                                                                         \
      lmOut(text, 'T', __FILE__, __LINE__, (char*) __FUNCTION__, tLev, NULL); \
      if (lmxFp != NULL) lmxFp(xCode, text);                                  \
      ::free(text);                                                           \
    }                                                                         \
  }                                                                           \
} while (0)



/* ****************************************************************************
*
* Output decision variables
*/
extern int  lmBug2;
extern bool lmBug;
extern bool lmVerbose;
extern bool lmVerbose2;
extern bool lmVerbose3;
extern bool lmVerbose4;
extern bool lmVerbose5;
extern bool lmDebug;
extern bool lmHidden;
extern bool lmToDo;
extern bool lmFix;
extern bool lmDoubt;
extern bool lmBuf;
extern bool lmReads;
extern bool lmWrites;
extern bool lmAssertAtExit;
extern bool lmNoTracesToFileIfHookActive;
extern bool lmSilent;

extern __thread char   service[SERVICE_NAME_MAX_LEN + 1];
extern __thread char   subService[101];                 // Using SERVICE_PATH_MAX_TOTAL will be too much
extern __thread char   fromIp[IP_LENGTH_MAX + 1];


/* ****************************************************************************
*
* lmxFp - 
*/
extern LmxFp lmxFp;



/* ****************************************************************************
*
* lmInit - 
*/
extern LmStatus lmInit(void);



/* ****************************************************************************
*
* lmInitX - 
*/
extern LmStatus lmInitX(char* progName, char* tLevel, int* i1P, int* i2P);



/* ****************************************************************************
*
* lmStrerror - 
*/
extern const char* lmStrerror(LmStatus s);



/* ****************************************************************************
*
* lmProgName - 
*/
extern char* lmProgName(char* pn, int levels, bool pid, const char* extra = NULL);



/* ****************************************************************************
*
* lmTraceSet - 
*/
extern LmStatus lmTraceSet(const char* levelFormat);



/* ****************************************************************************
*
* lmTraceAdd - 
*/
extern LmStatus lmTraceAdd(const char* levelFormat);



/* ****************************************************************************
*
* lmTraceSub - 
*/
extern LmStatus lmTraceSub(const char* levelFormat);



/* ****************************************************************************
*
* lmTraceGet - 
*/
extern char* lmTraceGet(char* levelString, int levelStringSize);
extern char* lmTraceGet(char* levelString, int levelStringSize, char* traceV);



/* ****************************************************************************
*
* lmFormat - 
*/
extern LmStatus lmFormat(int index, char* f);



/* ****************************************************************************
*
* lmTimeFormat - 
*/
extern LmStatus lmTimeFormat(int index, char* f);



/* ****************************************************************************
*
* lmGetInfo - 
*/
LmStatus lmGetInfo(int index, char* info);



/* ****************************************************************************
*
* lmFdGet - 
*/
extern LmStatus lmFdGet(int index, int* iP);



/* ****************************************************************************
*
* lmTraceAtEnd - 
*/
extern LmStatus lmTraceAtEnd(int index, char* start, char* end);



/* ****************************************************************************
*
* lmAux - 
*/
extern LmStatus lmAux(char* a);



/* ****************************************************************************
*
* lmTextGet - 
*/
extern char* lmTextGet(const char* format, ...);



/* ****************************************************************************
*
* lmOk - check whether or not to present the line 
*/
extern LmStatus lmOk(char type, int tLev);



/* ****************************************************************************
*
* lmFdRegister - 
*/
extern LmStatus lmFdRegister
(
  int          fd,
  const char*  format,
  const char*  timeFormat,
  const char*  info,
  int*         indexP
);



/* ****************************************************************************
*
* lmFdUnregister
*/
extern void lmFdUnregister(int fd);



/* ****************************************************************************
*
* lmPathRegister - 
*/
extern LmStatus lmPathRegister
(
  const char*  path,
  const char*  format,
  const char*  timeFormat,
  int*         indexP,
  bool         appendToLogFile = false
);



/* ****************************************************************************
*
* lmOut - 
*/
extern LmStatus lmOut
(
  char*        text,
  char         type,
  const char*  file,
  int          lineNo,
  const char*  fName,
  int          tLev,
  const char*  stre,
  bool         use_hook = true
);



/* ****************************************************************************
*
* lmOutHookSet - 
*/
extern void lmOutHookSet(LmOutHook hook, void* vP);
extern bool lmOutHookInhibit();
extern void lmOutHookRestore(bool onoff);



/* ****************************************************************************
*
* lmExitFunction - 
*/
LmStatus lmExitFunction(LmExitFp fp, void* input);



/* ****************************************************************************
*
* lmErrorFunction - 
*/
extern LmStatus lmErrorFunction(LmErrorFp fp, void* input);



/* ****************************************************************************
*
* lmWarningFunction - 
*/
extern LmStatus lmWarningFunction(LmWarningFp fp, void* input);



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
);



/* ****************************************************************************
*
* lmWriteFunction - use alternative write function
*/
LmStatus lmWriteFunction(int i, LmWriteFp fp);



/* ****************************************************************************
*
* lmClear -
*/
extern LmStatus lmClear(int index, int keepLines, int lastLines);



/* ****************************************************************************
*
* lmDoClear -
*/
extern LmStatus lmDoClear(void);



/* ****************************************************************************
*
* lmDontClear -
*/
extern LmStatus lmDontClear(void);



/* ****************************************************************************
*
* lmClearAt -
*/
LmStatus lmClearAt(int atLines, int keepLines, int lastLines);



/* ****************************************************************************
*
* lmClearGet -
*/
extern void lmClearGet
(
  bool* clearOn,
  int*       atP,
  int*       keepP,
  int*       lastP,
  int*       logFileBytesP
);



/* ****************************************************************************
*
* lmExitForced -
*/
extern void lmExitForced(int code);



/* ****************************************************************************
*
* lmxFunction - extra function to call for LMX_ macros
*/
extern LmStatus lmxFunction(LmxFp fp);



/* ****************************************************************************
*
* lmReopen -
*/
extern LmStatus lmReopen(int index);



/* ****************************************************************************
*
* lmOnlyErrors
*/
extern LmStatus lmOnlyErrors(int index);



/* ****************************************************************************
*
* lmTraceLevel -
*/
extern const char* lmTraceLevel(int level);



/* ****************************************************************************
*
* lmTraceNameCbSet -
*/
extern void lmTraceNameCbSet(LmTracelevelName cb);



/* ****************************************************************************
*
* lmTraceIsSet -
*/
extern bool lmTraceIsSet(int level);



/* ****************************************************************************
*
* lmTraceLevelSet - set a particular level to true/false
*/
extern void lmTraceLevelSet(unsigned int level, bool onOff);



/* ****************************************************************************
*
* lmSdGet -
*/
int lmSdGet(void);



/* ****************************************************************************
*
* lmAddMsgBuf -
*/
void lmAddMsgBuf(char* text, char type, const char* file, int line, const char* func, int tLev, const char *stre);



/* ****************************************************************************
*
* lmPrintMsgBuf -
*/
void lmPrintMsgBuf();


/* ****************************************************************************
*
* lmWarningFunctionDebug
*/
extern void lmWarningFunctionDebug(char* info, char* file, int line);
#define LMWARNINGFUNCTIONDEBUG(info) lmWarningFunctionDebug(info, __FILE__, __LINE__)



/* ****************************************************************************
*
* lmFirstDiskFileDescriptor -
*/
extern int lmFirstDiskFileDescriptor(void);



/* ****************************************************************************
*
* lmLogLineGet -
*/
extern int64_t lmLogLineGet
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
);



/* ****************************************************************************
*
* lmCleanProgName -
*/
extern void lmCleanProgName(void);



/* ****************************************************************************
*
* lmLogLinesGet -
*/
extern int lmLogLinesGet(void);



/* ****************************************************************************
*
* lmTransactionReset -
*/
inline void lmTransactionReset()
{
  strncpy(transactionId, "N/A", sizeof(transactionId));
  strncpy(correlatorId,  "N/A", sizeof(correlatorId));
  strncpy(service,       "N/A", sizeof(service));
  strncpy(subService,    "N/A", sizeof(subService));
  strncpy(fromIp,        "N/A", sizeof(fromIp));
}



/* ****************************************************************************
*
* lmTransactionStart -
*/
inline void lmTransactionStart(const char* keyword, const char* schema, const char* ip, int port, const char* path)
{
  transactionIdSet();

  snprintf(service,    sizeof(service),    "pending");
  snprintf(subService, sizeof(subService), "pending");
  snprintf(fromIp,     sizeof(fromIp),     "pending");
  LM_I(("Starting transaction %s %s%s:%d%s", keyword, schema, ip, port, path));
}



/* ****************************************************************************
*
* lmTransactionSetService -
*/
inline void lmTransactionSetService(const char* _service)
{
  if (strlen(_service) != 0)
  {
    snprintf(service, sizeof(service), "%s", _service);
  }
  else
  {
    snprintf(service, sizeof(service), "%s", "<default>");
  }
}



/* ****************************************************************************
*
* lmTransactionSetSubservice -
*/
inline void lmTransactionSetSubservice(const char* _subService)
{
  if (strlen(_subService) != 0)
  {
    snprintf(subService, sizeof(service), "%s", _subService);
  }
  else
  {
    snprintf(subService, sizeof(service), "%s", "<default>");
  }
}



/* ****************************************************************************
*
* lmTransactionSetFrom -
*/
inline void lmTransactionSetFrom(const char* _fromIp)
{
  if (strlen(_fromIp) != 0)
  {
    snprintf(fromIp, sizeof(fromIp), "%s", _fromIp);
  }
  else
  {
    snprintf(fromIp, sizeof(fromIp), "%s", "<no ip>");
  }
}



#if 0

// This piece of code seems not be in use. Maybe it is related with ONTIMENOTIFICATION
// notifications... let's hold in until OTI (deprecated in 0.26.0) gets definitivealy
// removed from code

/* ****************************************************************************
*
* lmTransactionStart_URL -
*/
inline void lmTransactionStart_URL(const char* url)
{
  transactionIdSet();
  LM_I(("Starting transaction from %s", url));
}
#endif



/* ****************************************************************************
*
* lmTransactionEnd -
*/
inline void lmTransactionEnd()
{
  LM_I(("Transaction ended"));
  lmTransactionReset();
}



/* ****************************************************************************
*
* lmSemGet - 
*/
extern const char* lmSemGet(void);

#endif  // SRC_LIB_LOGMSG_LOGMSG_H_
