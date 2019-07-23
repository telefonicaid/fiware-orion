#ifndef SRC_LIB_ORIONLD_COMMON_CHECK_H_
#define SRC_LIB_ORIONLD_COMMON_CHECK_H_

/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate, OrionldDetailsString, ...


// -----------------------------------------------------------------------------
//
// DUPLICATE_CHECK -
//
#define DUPLICATE_CHECK(pointer, fieldName, value)                                                                                    \
do                                                                                                                                    \
{                                                                                                                                     \
  if (pointer != NULL)                                                                                                                \
  {                                                                                                                                   \
    LM_E(("Duplicated attribute: '%s'", fieldName));                                                                                  \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Duplicated field", fieldName, OrionldDetailsString);                      \
    ciP->httpStatusCode = SccBadRequest;                                                                                              \
    return false;                                                                                                                     \
  }                                                                                                                                   \
  pointer = value;                                                                                                                    \
} while (0)



// -----------------------------------------------------------------------------
//
// DUPLICATE_CHECK_WITH_PRESENCE -
//
#define DUPLICATE_CHECK_WITH_PRESENCE(alreadyPresent, valueHolder, fieldName, value)                                                  \
do                                                                                                                                    \
{                                                                                                                                     \
  if (alreadyPresent == true)                                                                                                         \
  {                                                                                                                                   \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Duplicated field", fieldName, OrionldDetailsString);                      \
    ciP->httpStatusCode = SccBadRequest;                                                                                              \
    return false;                                                                                                                     \
  }                                                                                                                                   \
  valueHolder    = value;                                                                                                             \
  alreadyPresent = true;                                                                                                              \
} while (0)



// -----------------------------------------------------------------------------
//
// OBJECT_CHECK -
//
#define OBJECT_CHECK(nodeP, what)                                                                                                 \
do                                                                                                                                \
{                                                                                                                                 \
  if (nodeP->type != KjObject)                                                                                                    \
  {                                                                                                                               \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON Object", what, OrionldDetailsString);                      \
    ciP->httpStatusCode = SccBadRequest;                                                                                          \
    return false;                                                                                                                 \
  }                                                                                                                               \
} while (0)



// -----------------------------------------------------------------------------
//
// ARRAY_CHECK -
//
#define ARRAY_CHECK(kNodeP, fieldName)                                                                                                       \
do                                                                                                                                           \
{                                                                                                                                            \
  if (kNodeP->type != KjArray)                                                                                                               \
  {                                                                                                                                          \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "not a JSON Array", fieldName, OrionldDetailsString);                             \
    ciP->httpStatusCode = SccBadRequest;                                                                                                     \
    return false;                                                                                                                            \
  }                                                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// EMPTY_ARRAY_CHECK -
//
#define EMPTY_ARRAY_CHECK(kNodeP, what)                                                                                  \
do                                                                                                                       \
{                                                                                                                        \
  if (kNodeP->value.firstChildP == NULL)                                                                                 \
  {                                                                                                                      \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Empty Array", what, OrionldDetailsString);                   \
    ciP->httpStatusCode = SccBadRequest;                                                                                 \
    return false;                                                                                                        \
  }                                                                                                                      \
} while (0)



// -----------------------------------------------------------------------------
//
// ATTRIBUTE_IS_OBJECT_CHECK -
//
#define ATTRIBUTE_IS_OBJECT_CHECK(nodeP)                                                                                          \
do                                                                                                                                \
{                                                                                                                                 \
  if (nodeP->type != KjObject)                                                                                                    \
  {                                                                                                                               \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Attribute must be a JSON object", nodeP->name, OrionldDetailsString); \
    ciP->httpStatusCode = SccBadRequest;                                                                                          \
    return false;                                                                                                                 \
  }                                                                                                                               \
} while (0)



// -----------------------------------------------------------------------------
//
// STRING_CHECK -
//
#define STRING_CHECK(kNodeP, fieldName)                                                                                                      \
do                                                                                                                                           \
{                                                                                                                                            \
  if (kNodeP->type != KjString)                                                                                                              \
  {                                                                                                                                          \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON String", fieldName, OrionldDetailsString);                            \
    ciP->httpStatusCode = SccBadRequest;                                                                                                     \
    return false;                                                                                                                            \
  }                                                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// INTEGER_CHECK -
//
#define INTEGER_CHECK(nodeP, fieldName)                                                                                                      \
do                                                                                                                                           \
{                                                                                                                                            \
  if (nodeP->type != KjInt)                                                                                                                  \
  {                                                                                                                                          \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON Integer", fieldName, OrionldDetailsString);                           \
    ciP->httpStatusCode = SccBadRequest;                                                                                                     \
    return false;                                                                                                                            \
  }                                                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// BOOL_CHECK -
//
#define BOOL_CHECK(kNodeP, fieldName)                                                                                                 \
do                                                                                                                                    \
{                                                                                                                                     \
  if (kNodeP->type != KjBoolean)                                                                                                      \
  {                                                                                                                                   \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON Boolean", fieldName, OrionldDetailsString);                    \
    ciP->httpStatusCode = SccBadRequest;                                                                                              \
    return false;                                                                                                                     \
  }                                                                                                                                   \
} while (0)



// -----------------------------------------------------------------------------
//
// DATETIME_CHECK -
//
#define DATETIME_CHECK(stringValue, fieldName)                                                                                        \
do                                                                                                                                    \
{                                                                                                                                     \
  if (parse8601Time(stringValue) == -1)                                                                                               \
  {                                                                                                                                   \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid DateTime value", fieldName, OrionldDetailsString);                \
    ciP->httpStatusCode = SccBadRequest;                                                                                              \
    return false;                                                                                                                     \
  }                                                                                                                                   \
} while (0)



// -----------------------------------------------------------------------------
//
// ARRAY_OR_STRING_CHECK -
//
#define ARRAY_OR_STRING_CHECK(nodeP, what)                                                                             \
do                                                                                                                     \
{                                                                                                                      \
  if ((nodeP->type != KjArray) && (nodeP->type != KjString))                                                           \
  {                                                                                                                    \
    LM_T(LmtPayloadCheck, ("the node is a '%s'", kjValueType(nodeP->type)));                                           \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON Array nor String", what, OrionldDetailsString); \
    ciP->httpStatusCode = SccBadRequest;                                                                               \
    return false;                                                                                                      \
  }                                                                                                                    \
} while (0)



// -----------------------------------------------------------------------------
//
// ARRAY_OR_STRING_OR_OBJECT_CHECK -
//
#define ARRAY_OR_STRING_OR_OBJECT_CHECK(nodeP, what)                                                                                \
do                                                                                                                                  \
{                                                                                                                                   \
  if ((nodeP->type != KjArray) && (nodeP->type != KjString) && (nodeP->type != KjObject))                                           \
  {                                                                                                                                 \
    LM_T(LmtPayloadCheck, ("the node is a '%s'", kjValueType(nodeP->type)));                                                        \
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Not a JSON Array nor Object nor a String", what, OrionldDetailsString); \
    ciP->httpStatusCode = SccBadRequest;                                                                                            \
    return false;                                                                                                                   \
  }                                                                                                                                 \
} while (0)

#endif  // SRC_LIB_ORIONLD_COMMON_CHECK_H_
