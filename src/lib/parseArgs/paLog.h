#ifndef SRC_LIB_PARSEARGS_PALOG_H_
#define SRC_LIB_PARSEARGS_PALOG_H_

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
#include <stdio.h>              /* printf                                    */
#include <errno.h>              /* errno, strerror                           */

extern bool paLogOn;

#define PA_DEBUG

#ifdef PA_DEBUG

#define LOG printf   /* used by PA_M, PA_E & PA_P if PA_DEBUG defined */

#define PA_M(s)                                                     \
do {                                                                \
      if (paLogOn == true)                                          \
      {                                                             \
        LOG("M: %s/%s[%d]: ", __FILE__, __FUNCTION__, __LINE__);    \
        LOG s;                                                      \
        LOG("\n");                                                  \
      }                                                             \
} while (0)

#define PA_E(s)                                                     \
do {                                                                \
      if (paLogOn == true)                                          \
      {                                                             \
        LOG("E: %s/%s[%d]: ", __FILE__, __FUNCTION__, __LINE__);    \
        LOG s;                                                      \
        LOG("\n");                                                  \
      }                                                             \
} while (0)

#define PA_W(s)                                                     \
do {                                                                \
      if (paLogOn == true)                                          \
      {                                                             \
        LOG("W: %s/%s[%d]: ", __FILE__, __FUNCTION__, __LINE__);    \
        LOG s;                                                      \
        LOG("\n");                                                  \
      }                                                             \
} while (0)

#define PA_P(s)                                                     \
do {                                                                \
      if (paLogOn == true)                                          \
      {                                                             \
        LOG("E: %s/%s[%d]: ", __FILE__, __FUNCTION__, __LINE__);    \
        LOG s;                                                      \
        LOG(": %s\n", strerror(errno));                             \
      }                                                             \
} while (0)

#else

#  define PA_M(s)
#  define PA_E(s)
#  define PA_P(s)
#endif

#endif  // SRC_LIB_PARSEARGS_PALOG_H_
