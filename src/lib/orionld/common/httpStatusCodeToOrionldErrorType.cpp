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
#include "rest/HttpStatusCode.h"                               // HttpStatusCode
#include "orionld/common/orionldErrorResponse.h"               // OrionldResponseErrorType
#include "orionld/common/httpStatusCodeToOrionldErrorType.h"   // Own interface



// -----------------------------------------------------------------------------
//
// httpStatusCodeToOrionldErrorType
//
OrionldResponseErrorType httpStatusCodeToOrionldErrorType(HttpStatusCode sc)
{
  switch (sc)
  {
  case SccNone:                           return OrionldInternalError;
  case SccOk:                             return OrionldInternalError;  // Should not be here if 200 ...
  case SccCreated:                        return OrionldInternalError;
  case SccNoContent:                      return OrionldInternalError;
  case SccBadRequest:                     return OrionldBadRequestData;
  case SccForbidden:                      return OrionldOperationNotSupported;
  case SccContextElementNotFound:         return OrionldResourceNotFound;
  case SccBadVerb:                        return OrionldInvalidRequest;
  case SccNotAcceptable:                  return OrionldInvalidRequest;
  case SccConflict:                       return OrionldAlreadyExists;
  case SccContentLengthRequired:          return OrionldInvalidRequest;
  case SccRequestEntityTooLarge:          return OrionldInvalidRequest;
  case SccUnsupportedMediaType:           return OrionldInvalidRequest;
  case SccInvalidModification:            return OrionldOperationNotSupported;
  case SccSubscriptionIdNotFound:         return OrionldResourceNotFound;
  case SccMissingParameter:               return OrionldBadRequestData;
  case SccInvalidParameter:               return OrionldBadRequestData;
  case SccErrorInMetadata:                return OrionldBadRequestData;
  case SccEntityIdReNotAllowed:           return OrionldBadRequestData;
  case SccEntityTypeRequired:             return OrionldBadRequestData;
  case SccAttributeListRequired:          return OrionldInvalidRequest;
  case SccReceiverInternalError:          return OrionldInternalError;
  case SccNotImplemented:                 return OrionldOperationNotSupported;
  }

  return OrionldInternalError;
}
