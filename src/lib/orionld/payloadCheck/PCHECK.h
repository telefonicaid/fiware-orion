#ifndef SRC_LIB_ORIONLD_PAYLOADCHECK_PCHECK_H_
#define SRC_LIB_ORIONLD_PAYLOADCHECK_PCHECK_H_

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
#include "logMsg/logMsg.h"                                    // LM_*

#include "orionld/common/orionldError.h"                      // orionldError
#include "orionld/common/orionldState.h"                      // orionldState
#include "orionld/payloadCheck/pCheckUri.h"                   // pCheckUri



// -----------------------------------------------------------------------------
//
// PCHECK_STRING -
//
#define PCHECK_STRING(kNodeP, _type, _title, detail, status)                                 \
do                                                                                           \
{                                                                                            \
  if (kNodeP->type != KjString)                                                              \
  {                                                                                          \
    int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                     \
    const char* title = (_title == NULL)? "Not a JSON String"   : _title;                    \
                                                                                             \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    return false;                                                                            \
  }                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// PCHECK_OBJECT -
//
#define PCHECK_OBJECT(kNodeP, _type, _title, detail, status)                                 \
do                                                                                           \
{                                                                                            \
  if (kNodeP->type != KjObject)                                                              \
  {                                                                                          \
    int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                     \
    const char* title = (_title == NULL)? "Not a JSON Object"   : _title;                    \
                                                                                             \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    return false;                                                                            \
  }                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// PCHECK_ARRAY -
//
#define PCHECK_ARRAY(kNodeP, _type, _title, detail, status)                                  \
do                                                                                           \
{                                                                                            \
  if (kNodeP->type != KjArray)                                                               \
  {                                                                                          \
    int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                     \
    const char* title = (_title == NULL)? "Not a JSON Array"   : _title;                     \
                                                                                             \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    return false;                                                                            \
  }                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// PCHECK_URI -
//
#define PCHECK_URI(uri, mustBeUri, _type, _title, detail, status)                            \
do                                                                                           \
{                                                                                            \
  int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                       \
  const char* title = (_title == NULL)? "Invalid URI"         : _title;                      \
                                                                                             \
  if (pCheckUri(uri, mustBeUri) == false)                                                    \
  {                                                                                          \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    return false;                                                                            \
  }                                                                                          \
} while (0)

#endif  // SRC_LIB_ORIONLD_PAYLOADCHECK_PCHECK_H_
