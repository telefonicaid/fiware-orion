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
#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dateTime.h"                             // dateTimeFromString
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri



// -----------------------------------------------------------------------------
//
// DUPLICATE_CHECK -
//
#define DUPLICATE_CHECK(pointer, fieldName, value)                                                               \
do                                                                                                               \
{                                                                                                                \
  if (pointer != NULL)                                                                                           \
  {                                                                                                              \
    orionldError(OrionldBadRequestData, "Duplicated field", fieldName, 400);                                     \
    return false;                                                                                                \
  }                                                                                                              \
  pointer = value;                                                                                               \
} while (0)



#define DUPLICATE_CHECK_R(pointer, fieldName, value, retVal)                                                     \
do                                                                                                               \
{                                                                                                                \
  if (pointer != NULL)                                                                                           \
  {                                                                                                              \
    orionldError(OrionldBadRequestData, "Duplicated field", fieldName, 400);                                     \
    return retVal;                                                                                               \
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
    orionldError(OrionldBadRequestData, "Duplicated field", fieldName, 400);                                     \
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
    orionldError(OrionldBadRequestData, "Not a JSON Object", what, 400);                                     \
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
    orionldError(OrionldBadRequestData, "Empty Object", what, 400);                                          \
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
    orionldError(OrionldBadRequestData, "Not a JSON Array", fieldName, 400);                                 \
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
    orionldError(OrionldBadRequestData, "Empty Array", what, 400);                                           \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// ARRAY_OR_OBJECT_CHECK -
//
#define ARRAY_OR_OBJECT_CHECK(nodeP, what)                                                                   \
do                                                                                                           \
{                                                                                                            \
  if ((nodeP->type != KjArray) && (nodeP->type != KjObject))                                                 \
  {                                                                                                          \
    orionldError(OrionldBadRequestData, "Not a JSON Array nor an Object", what, 400);                        \
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
    orionldError(OrionldBadRequestData, "Attribute must be a JSON object", nodeP->name, 400);                \
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
    orionldError(OrionldBadRequestData, "Not a JSON String", fieldName, 400);                                \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



#define STRING_CHECK_R(kNodeP, fieldName, retVal)                                                            \
do                                                                                                           \
{                                                                                                            \
  if (kNodeP->type != KjString)                                                                              \
  {                                                                                                          \
    orionldError(OrionldBadRequestData, "Not a JSON String", fieldName, 400);                                \
    return retVal;                                                                                           \
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
    orionldError(OrionldBadRequestData, "Empty String", what, 400);                                          \
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
    orionldError(OrionldBadRequestData, "Not a JSON Integer", fieldName, 400);                               \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// NUMBER_CHECK -
//
#define NUMBER_CHECK(nodeP, fieldName)                                                                       \
do                                                                                                           \
{                                                                                                            \
  if ((nodeP->type != KjInt) && (nodeP->type != KjFloat))                                                    \
  {                                                                                                          \
    orionldError(OrionldBadRequestData, "Not a JSON Number", fieldName, 400);                                \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// POSITIVE_NUMBER_CHECK -
//
#define POSITIVE_NUMBER_CHECK(nodeP, fieldName)                                                                    \
do                                                                                                                 \
{                                                                                                                  \
  if (((nodeP->type == KjInt) && (nodeP->value.i < 0)) || ((nodeP->type == KjFloat) && (nodeP->value.f < 0)))      \
  {                                                                                                                \
    orionldError(OrionldBadRequestData, "Negative Number not allowed in this position", fieldName, 400);           \
    return false;                                                                                                  \
  }                                                                                                                \
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
    orionldError(OrionldBadRequestData, "Not a JSON Boolean", fieldName, 400);                               \
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
  char errorString[512];                                                                                     \
                                                                                                             \
  if ((dateTimeValue = dateTimeFromString(stringValue, errorString, sizeof(errorString))) < 0)               \
  {                                                                                                          \
    orionldError(OrionldBadRequestData, "Invalid DateTime value", errorString, 400);                         \
    pdField(fieldName);                                                                                      \
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
    orionldError(OrionldBadRequestData, "Not a JSON Array nor String", what, 400);                           \
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
    orionldError(OrionldBadRequestData, "Not a JSON Array nor Object nor a String", what, 400);              \
    return false;                                                                                            \
  }                                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// URI_CHECK -
//
#define URI_CHECK(uri, fieldName, strict)                                                                    \
do                                                                                                           \
{                                                                                                            \
  if (pCheckUri(uri, fieldName, strict) == false)                                                            \
    return false;                                                                                            \
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
    orionldError(OrionldBadRequestData, "Payload is missing", NULL, 400);                       \
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
    orionldError(OrionldBadRequestData, "Payload not a JSON object",                            \
                 kjValueType(orionldState.requestTree->type), 400);                             \
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
    orionldError(OrionldBadRequestData, "Payload is an empty JSON object", NULL, 400);          \
    return false;                                                                               \
  }                                                                                             \
} while (0)

#endif  // SRC_LIB_ORIONLD_COMMON_CHECK_H_
