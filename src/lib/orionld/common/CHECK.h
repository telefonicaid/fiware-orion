#ifndef SRC_LIB_ORIONLD_COMMON_CHECK_H_
#define SRC_LIB_ORIONLD_COMMON_CHECK_H_

/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "orionld/common/urlCheck.h"                           // urlCheck



// -----------------------------------------------------------------------------
//
// DUPLICATE_CHECK -
//
#define DUPLICATE_CHECK(pointer, fieldName, value)                                                               \
do                                                                                                               \
{                                                                                                                \
  if (pointer != NULL)                                                                                           \
  {                                                                                                              \
    LM_E(("Duplicated attribute: '%s'", fieldName));                                                             \
    orionldErrorResponseCreate(OrionldBadRequestData, "Duplicated field", fieldName);                            \
    ciP->httpStatusCode = SccBadRequest;                                                                         \
    return false;                                                                                                \
  }                                                                                                              \
  pointer = value;                                                                                               \
} while (0)



// -----------------------------------------------------------------------------
//
// DUPLICATE_CHECK_WITH_PRESENCE -
//
#define DUPLICATE_CHECK_WITH_PRESENCE(alreadyPresent, valueHolder, fieldName, value)                             \
do                                                                                                               \
{                                                                                                                \
  if (alreadyPresent == true)                                                                                    \
  {                                                                                                              \
    orionldErrorResponseCreate(OrionldBadRequestData, "Duplicated field", fieldName);                            \
    ciP->httpStatusCode = SccBadRequest;                                                                         \
    return false;                                                                                                \
  }                                                                                                              \
  valueHolder    = value;                                                                                        \
  alreadyPresent = true;                                                                                         \
} while (0)



// -----------------------------------------------------------------------------
//
// OBJECT_CHECK -
//
#define OBJECT_CHECK(nodeP, what)                                                                            \
do                                                                                                           \
{                                                                                                            \
  if (nodeP->type != KjObject)                                                                               \
  {                                                                                                          \
    orionldErrorResponseCreate(OrionldBadRequestData, "Not a JSON Object", what);                            \
    ciP->httpStatusCode = SccBadRequest;                                                                     \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// EMPTY_OBJECT_CHECK -
//
#define EMPTY_OBJECT_CHECK(kNodeP, what)                                                                     \
do                                                                                                           \
{                                                                                                            \
  if (kNodeP->value.firstChildP == NULL)                                                                     \
  {                                                                                                          \
    orionldErrorResponseCreate(OrionldBadRequestData, "Empty Object", what);                                 \
    ciP->httpStatusCode = SccBadRequest;                                                                     \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// ARRAY_CHECK -
//
#define ARRAY_CHECK(kNodeP, fieldName)                                                                       \
do                                                                                                           \
{                                                                                                            \
  if (kNodeP->type != KjArray)                                                                               \
  {                                                                                                          \
    orionldErrorResponseCreate(OrionldBadRequestData, "Not a JSON Array", fieldName);                        \
    ciP->httpStatusCode = SccBadRequest;                                                                     \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// EMPTY_ARRAY_CHECK -
//
#define EMPTY_ARRAY_CHECK(kNodeP, what)                                                                      \
do                                                                                                           \
{                                                                                                            \
  if (kNodeP->value.firstChildP == NULL)                                                                     \
  {                                                                                                          \
    orionldErrorResponseCreate(OrionldBadRequestData, "Empty Array", what);                                  \
    ciP->httpStatusCode = SccBadRequest;                                                                     \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// ATTRIBUTE_IS_OBJECT_CHECK -
//
#define ATTRIBUTE_IS_OBJECT_CHECK(nodeP)                                                                     \
do                                                                                                           \
{                                                                                                            \
  if (nodeP->type != KjObject)                                                                               \
  {                                                                                                          \
    orionldErrorResponseCreate(OrionldBadRequestData, "Attribute must be a JSON object", nodeP->name);       \
    ciP->httpStatusCode = SccBadRequest;                                                                     \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// STRING_CHECK -
//
#define STRING_CHECK(kNodeP, fieldName)                                                                      \
do                                                                                                           \
{                                                                                                            \
  if (kNodeP->type != KjString)                                                                              \
  {                                                                                                          \
    orionldErrorResponseCreate(OrionldBadRequestData, "Not a JSON String", fieldName);                       \
    ciP->httpStatusCode = SccBadRequest;                                                                     \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// EMPTY_STRING_CHECK -
//
#define EMPTY_STRING_CHECK(kNodeP, what)                                                                     \
do                                                                                                           \
{                                                                                                            \
  if (kNodeP->value.s[0] == 0)                                                                               \
  {                                                                                                          \
    orionldErrorResponseCreate(OrionldBadRequestData, "Empty String", what);                                 \
    ciP->httpStatusCode = SccBadRequest;                                                                     \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// INTEGER_CHECK -
//
#define INTEGER_CHECK(nodeP, fieldName)                                                                      \
do                                                                                                           \
{                                                                                                            \
  if (nodeP->type != KjInt)                                                                                  \
  {                                                                                                          \
    orionldErrorResponseCreate(OrionldBadRequestData, "Not a JSON Integer", fieldName);                      \
    ciP->httpStatusCode = SccBadRequest;                                                                     \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// BOOL_CHECK -
//
#define BOOL_CHECK(kNodeP, fieldName)                                                                        \
do                                                                                                           \
{                                                                                                            \
  if (kNodeP->type != KjBoolean)                                                                             \
  {                                                                                                          \
    orionldErrorResponseCreate(OrionldBadRequestData, "Not a JSON Boolean", fieldName);                      \
    ciP->httpStatusCode = SccBadRequest;                                                                     \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// DATETIME_CHECK -
//
#define DATETIME_CHECK(stringValue, dateTimeValue, fieldName)                                                \
do                                                                                                           \
{                                                                                                            \
  if ((dateTimeValue = parse8601Time(stringValue)) == -1)                                                    \
  {                                                                                                          \
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid DateTime value", fieldName);                  \
    ciP->httpStatusCode = SccBadRequest;                                                                     \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// ARRAY_OR_STRING_CHECK -
//
#define ARRAY_OR_STRING_CHECK(nodeP, what)                                                                   \
do                                                                                                           \
{                                                                                                            \
  if ((nodeP->type != KjArray) && (nodeP->type != KjString))                                                 \
  {                                                                                                          \
    LM_T(LmtPayloadCheck, ("the node is a '%s'", kjValueType(nodeP->type)));                                 \
    orionldErrorResponseCreate(OrionldBadRequestData, "Not a JSON Array nor String", what);                  \
    ciP->httpStatusCode = SccBadRequest;                                                                     \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// ARRAY_OR_STRING_OR_OBJECT_CHECK -
//
#define ARRAY_OR_STRING_OR_OBJECT_CHECK(nodeP, what)                                                         \
do                                                                                                           \
{                                                                                                            \
  if ((nodeP->type != KjArray) && (nodeP->type != KjString) && (nodeP->type != KjObject))                    \
  {                                                                                                          \
    LM_T(LmtPayloadCheck, ("the node is a '%s'", kjValueType(nodeP->type)));                                 \
    orionldErrorResponseCreate(OrionldBadRequestData, "Not a JSON Array nor Object nor a String", what);     \
    ciP->httpStatusCode = SccBadRequest;                                                                     \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// URI_CHECK -
//
#define URI_CHECK(kNodeP, fieldName)                                                                         \
do                                                                                                           \
{                                                                                                            \
  char* detail;                                                                                              \
  if (!urlCheck(kNodeP->value.s, &detail) && !urnCheck(kNodeP->value.s, &detail))                            \
  {                                                                                                          \
    orionldErrorResponseCreate(OrionldBadRequestData, "Not a URI", fieldName);                               \
    ciP->httpStatusCode = SccBadRequest;                                                                     \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// PAYLOAD_EMPTY_CHECK -
//
#define PAYLOAD_EMPTY_CHECK()                                                                   \
do                                                                                              \
{                                                                                               \
  if (orionldState.requestTree == NULL)                                                         \
  {                                                                                             \
    ciP->httpStatusCode = SccBadRequest;                                                        \
    orionldErrorResponseCreate(OrionldBadRequestData, "Payload is missing", NULL);              \
    return false;                                                                               \
  }                                                                                             \
} while (0)



// -----------------------------------------------------------------------------
//
// PAYLOAD_IS_OBJECT_CHECK -
//
#define PAYLOAD_IS_OBJECT_CHECK()                                                               \
do                                                                                              \
{                                                                                               \
  if (orionldState.requestTree->type != KjObject)                                               \
  {                                                                                             \
    ciP->httpStatusCode = SccBadRequest;                                                        \
    orionldErrorResponseCreate(OrionldBadRequestData, "Payload not a JSON object",              \
                               kjValueType(orionldState.requestTree->type));                    \
    return false;                                                                               \
  }                                                                                             \
} while (0)



// -----------------------------------------------------------------------------
//
// PAYLOAD_EMPTY_OBJECT_CHECK -
//
#define PAYLOAD_EMPTY_OBJECT_CHECK()                                                            \
do                                                                                              \
{                                                                                               \
  if (orionldState.requestTree->value.firstChildP == NULL)                                      \
  {                                                                                             \
    ciP->httpStatusCode = SccBadRequest;                                                        \
    orionldErrorResponseCreate(OrionldBadRequestData, "Payload is an empty JSON object", NULL); \
    return false;                                                                               \
  }                                                                                             \
} while (0)

#endif  // SRC_LIB_ORIONLD_COMMON_CHECK_H_
