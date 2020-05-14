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
#include "orionld/common/orionldErrorResponse.h"               // OrionldResponseErrorType
#include "orionld/common/httpStatusCodeToOrionldErrorType.h"   // Own interface



// -----------------------------------------------------------------------------
//
// httpStatusCodeToOrionldErrorType
//
OrionldResponseErrorType httpStatusCodeToOrionldErrorType(int httpStatusCode)
{
  switch (httpStatusCode)
  {
  case 0:   return OrionldInternalError;
  case 200: return OrionldInternalError;  // Should not be here if 200 ...
  case 201: return OrionldInternalError;
  case 204: return OrionldInternalError;
  case 207: return OrionldInternalError;
  case 400: return OrionldBadRequestData;
  case 403: return OrionldOperationNotSupported;
  case 404: return OrionldResourceNotFound;
  case 405: return OrionldInvalidRequest;
  case 406: return OrionldInvalidRequest;
  case 409: return OrionldAlreadyExists;
  case 411: return OrionldInvalidRequest;
  case 413: return OrionldInvalidRequest;
  case 415: return OrionldInvalidRequest;
  case 422: return OrionldOperationNotSupported;
  case 470: return OrionldResourceNotFound;
  case 471: return OrionldBadRequestData;
  case 472: return OrionldBadRequestData;
  case 473: return OrionldBadRequestData;
  case 480: return OrionldBadRequestData;
  case 481: return OrionldBadRequestData;
  case 482: return OrionldInvalidRequest;
  case 500: return OrionldInternalError;
  case 501: return OrionldOperationNotSupported;
  case 503: return OrionldInternalError;
  }

  return OrionldInternalError;
}
