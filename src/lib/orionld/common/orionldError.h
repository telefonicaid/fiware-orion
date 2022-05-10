#ifndef SRC_LIB_ORIONLD_COMMON_ORIONLDERROR_H_
#define SRC_LIB_ORIONLD_COMMON_ORIONLDERROR_H_

/*
*
* Copyright 2022 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include "orionld/types/OrionldResponseErrorType.h"           // OrionldResponseErrorType
#include "orionld/types/OrionldProblemDetails.h"              // OrionldProblemDetails



// ----------------------------------------------------------------------------
//
// orionldError -
//
extern void orionldError
(
  OrionldResponseErrorType  errorType,
  const char*               title,
  const char*               detail,
  int                       status
);



// -----------------------------------------------------------------------------
//
// OERROR
//
#define OERROR(retVal, errorType, title, detail, status)                              \
do {                                                                                  \
  orionldError2(errorType, title, detail, status, __FILE__, __LINE__, __FUNCTION__);  \
  return retVal;                                                                      \
} while (0)



// -----------------------------------------------------------------------------
//
// OERRORV
//
#define OERRORV(errorType, title, detail, status)                                     \
do {                                                                                  \
  orionldError2(errorType, title, detail, status, __FILE__, __LINE__, __FUNCTION__);  \
  return;                                                                             \
} while (0)

#endif  // SRC_LIB_ORIONLD_COMMON_ORIONLDERROR_H_
