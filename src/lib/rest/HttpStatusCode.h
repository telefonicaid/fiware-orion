#ifndef HTTP_STATUS_CODE_H
#define HTTP_STATUS_CODE_H

#include <string>

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
* Author: developer
*/

/* ****************************************************************************
*
* HttpStatusCode - 
*/
typedef enum HttpStatusCode
{
  SccNone                   = 0,     // Undefined
  SccOk                     = 200,   // Success
  SccCreated                = 201,   // Created
  SccNoContent              = 204,   // No content
  SccBadRequest             = 400,   // The request is not well formed
  SccForbidden              = 403,   // The request is not allowed
  SccContextElementNotFound = 404,   // No context element found
  SccBadVerb                = 405,   // Request ok but verb/method NOT OK
  SccNotAcceptable          = 406,   // The Accept header in the request is not supported
  SccConflict               = 409,
  SccContentLengthRequired  = 411,   // Content-Length header missing
  SccRequestEntityTooLarge  = 413,   // Request Entity Too Large - over 1Mb of payload
  SccUnsupportedMediaType   = 415,   // Unsupported Media Type (only support and application/json and -in some cases- text/plain)
  SccInvalidModification    = 422,   // InvalidModification (unprocessable entity)
  SccReceiverInternalError  = 500,   // An unknown error at the receiver has occurred
  SccNotImplemented         = 501    // The given operation is not implemented
} HttpStatusCode;



/* ****************************************************************************
*
* httpStatusCodeString - 
*/
extern std::string httpStatusCodeString(HttpStatusCode code);

#endif

