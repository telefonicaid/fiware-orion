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
extern "C"
{
#include "kjson/KjNode.h"                                     // KjNode
}

#include "logMsg/logMsg.h"                                    // LM_*

#include "orionld/types/OrionldAttributeType.h"               // NoAttributeType, Property
#include "orionld/common/orionldState.h"                      // orionldState
#include "orionld/common/orionldError.h"                      // orionldError
#include "orionld/common/dateTime.h"                          // dateTimeFromString
#include "orionld/payloadCheck/pCheckUri.h"                   // pCheckUri



// -----------------------------------------------------------------------------
//
// PCHECK_DUPLICATE -
//
#define PCHECK_DUPLICATE(pointer, pointerValue, _type, _title, _detail, status)              \
do                                                                                           \
{                                                                                            \
  if (pointer != NULL)                                                                       \
  {                                                                                          \
    int         type   = (_type  ==    0)? OrionldBadRequestData : _type;                    \
    const char* title  = (_title == NULL)? "Duplicated field" : _title;                      \
    orionldError((OrionldResponseErrorType) type, title, _detail, status);                   \
    return false;                                                                            \
  }                                                                                          \
  pointer = pointerValue;                                                                    \
} while (0)



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
// PCHECK_STRING_EMPTY -
//
#define PCHECK_STRING_EMPTY(kNodeP, _type, _title, detail, status)                           \
do                                                                                           \
{                                                                                            \
  if (kNodeP->value.s[0] == 0)                                                               \
  {                                                                                          \
    int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                     \
    const char* title = (_title == NULL)? "Empty String"   : _title;                         \
                                                                                             \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    return false;                                                                            \
  }                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// PCHECK_INTEGER -
//
#define PCHECK_INTEGER(kNodeP, _type, _title, detail, status)                                \
do                                                                                           \
{                                                                                            \
  if (kNodeP->type != KjInt)                                                                 \
  {                                                                                          \
    int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                     \
    const char* title = (_title == NULL)? "Not a JSON Integer"   : _title;                   \
                                                                                             \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    return false;                                                                            \
  }                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// PCHECK_STRING_OR_ARRAY -
//
#define PCHECK_STRING_OR_ARRAY(kNodeP, _type, _title, detail, status)                        \
do                                                                                           \
{                                                                                            \
  if ((kNodeP->type != KjArray) && (kNodeP->type != KjString))                               \
  {                                                                                          \
    int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                     \
    const char* title = (_title == NULL)? "Not a JSON String nor Array"   : _title;          \
                                                                                             \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    return false;                                                                            \
  }                                                                                          \
} while (0)



#define PCHECK_STRING_OR_ARRAY_R(kNodeP, _type, _title, detail, status, retVal)              \
do                                                                                           \
{                                                                                            \
  if ((kNodeP->type != KjArray) && (kNodeP->type != KjString))                               \
  {                                                                                          \
    int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                     \
    const char* title = (_title == NULL)? "Not a JSON String nor Array"   : _title;          \
                                                                                             \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    return retVal;                                                                           \
  }                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// PCHECK_BOOL -
//
#define PCHECK_BOOL(kNodeP, _type, _title, detail, status)                                   \
do                                                                                           \
{                                                                                            \
  if (kNodeP->type != KjBoolean)                                                             \
  {                                                                                          \
    int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                     \
    const char* title = (_title == NULL)? "Not a JSON Boolean"  : _title;                    \
                                                                                             \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    return false;                                                                            \
  }                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// PCHECK_NUMBER -
//
#define PCHECK_NUMBER(kNodeP, _type, _title, detail, status)                                 \
do                                                                                           \
{                                                                                            \
  if ((kNodeP->type != KjInt) && (kNodeP->type != KjFloat))                                  \
  {                                                                                          \
    int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                     \
    const char* title = (_title == NULL)? "Not a JSON Number"   : _title;                    \
                                                                                             \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    return false;                                                                            \
  }                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// PCHECK_NUMBER_GT -
//
#define PCHECK_NUMBER_GT(kNodeP, _type, _title, detail, status, minValue)                    \
do                                                                                           \
{                                                                                            \
  if ((kNodeP->type == KjInt) && (kNodeP->value.i <= minValue))                              \
  {                                                                                          \
    int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                     \
    const char* title = (_title == NULL)? "Too small a Number"  : _title;                    \
                                                                                             \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    return false;                                                                            \
  }                                                                                          \
                                                                                             \
  if ((kNodeP->type == KjFloat) && (kNodeP->value.f <= minValue))                            \
  {                                                                                          \
    int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                     \
    const char* title = (_title == NULL)? "Too small a Number"  : _title;                    \
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
// PCHECK_OBJECT_EMPTY -
//
#define PCHECK_OBJECT_EMPTY(kNodeP, _type, _title, detail, status)                           \
do                                                                                           \
{                                                                                            \
  if (kNodeP->value.firstChildP == NULL)                                                     \
  {                                                                                          \
    int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                     \
    const char* title = (_title == NULL)? "Empty Object"        : _title;                    \
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
// PCHECK_ARRAY_EMPTY -
//
#define PCHECK_ARRAY_EMPTY(kNodeP, _type, _title, detail, status)                            \
do                                                                                           \
{                                                                                            \
  if (kNodeP->value.firstChildP == NULL)                                                     \
  {                                                                                          \
    int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                     \
    const char* title = (_title == NULL)? "Empty Array"         : _title;                    \
                                                                                             \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    return false;                                                                            \
  }                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// PCHECK_ARRAY_OF_STRING -
//
#define PCHECK_ARRAY_OF_STRING(kNodeP, _type, _title, detail, status)                        \
do                                                                                           \
{                                                                                            \
  for (KjNode* itemP = kNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)        \
  {                                                                                          \
    if (itemP->type != KjString)                                                             \
    {                                                                                        \
      int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                   \
      const char* title = (_title == NULL)? "Non-String item in String-Array"  : _title;     \
                                                                                             \
      orionldError((OrionldResponseErrorType) type, title, detail, status);                  \
      return false;                                                                          \
    }                                                                                        \
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
  if (pCheckUri(uri, uri, mustBeUri) == false)                                               \
  {                                                                                          \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    return false;                                                                            \
  }                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// PCHECK_ISO8601 -
//
#define PCHECK_ISO8601(iso8601Var, iso8601String, _type, _title, fieldName, status)            \
do                                                                                             \
{                                                                                              \
  int         type  = (_type  ==    0)? OrionldBadRequestData : _type;                         \
  const char* title = (_title == NULL)? "Invalid ISO8601"     : _title;                        \
  char        errorString[512];                                                                \
                                                                                               \
  if ((iso8601Var = dateTimeFromString(iso8601String, errorString, sizeof(errorString))) < 0)  \
  {                                                                                            \
    orionldError((OrionldResponseErrorType) type, title, errorString, status);                 \
    pdField(fieldName);                                                                        \
    return false;                                                                              \
  }                                                                                            \
} while (0)



// -----------------------------------------------------------------------------
//
// PCHECK_EXPIRESAT_IN_FUTURE
//
#define PCHECK_EXPIRESAT_IN_FUTURE(_type, _title, detail, status, expiresAt, now)            \
do                                                                                           \
{                                                                                            \
  int         type  = (_type  ==    0)? OrionldBadRequestData   : _type;                     \
  const char* title = (_title == NULL)? "expiresAt in the past" : _title;                    \
                                                                                             \
  if (expiresAt < now)                                                                       \
  {                                                                                          \
    orionldError((OrionldResponseErrorType) type, title, detail, status);                    \
    pdField("expiresAt");                                                                    \
    return false;                                                                            \
  }                                                                                          \
} while (0)



// -----------------------------------------------------------------------------
//
// PCHECK_SPECIAL_ATTRIBUTE
//
#define PCHECK_SPECIAL_ATTRIBUTE(attrP, isAttribute, attrTypeFromDb)            \
do                                                                              \
{                                                                               \
  if (attrTypeFromDb == NoAttributeType)                                        \
  {                                                                             \
    if (isAttribute == true)                                                    \
    {                                                                           \
      if ((strcmp(attrP->name, "location")         == 0) ||                     \
          (strcmp(attrP->name, "observationSpace") == 0) ||                     \
          (strcmp(attrP->name, "operationSpace")   == 0))                       \
      {                                                                         \
        orionldError(OrionldBadRequestData,                                     \
                     "Invalid type for special GeoProperty attribute",          \
                     attrP->name,                                               \
                     400);                                                      \
        return false;                                                           \
      }                                                                         \
    }                                                                           \
  }                                                                             \
} while (0)



// -----------------------------------------------------------------------------
//
// PCHECK_NOT_A_PROPERTY -
//
#define PCHECK_NOT_A_PROPERTY(attrP, attrTypeFromDb)                         \
do                                                                           \
{                                                                            \
  if ((attrTypeFromDb != NoAttributeType) && (attrTypeFromDb != Property))   \
  {                                                                          \
    const char* title = attrTypeChangeTitle(attrTypeFromDb, Property);       \
    orionldError(OrionldBadRequestData, title, attrP->name, 400);            \
    return false;                                                            \
  }                                                                          \
} while (0)

#endif  // SRC_LIB_ORIONLD_PAYLOADCHECK_PCHECK_H_
