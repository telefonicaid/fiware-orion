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
#include "HttpStatusCode.h"



/* ****************************************************************************
*
* httpStatusCodeString -
*/
std::string httpStatusCodeString(HttpStatusCode code)
{
  switch (code)
  {
  case SccOk:                                return "OK";
  case SccCreated:                           return "Created";
  case SccBadRequest:                        return "BadRequest";
  case SccForbidden:                         return "Forbidden";
  case SccContextElementNotFound:            return "NotFound";
  case SccBadVerb:                           return "MethodNotAllowed";
  case SccNotAcceptable:                     return "NotAcceptable";
  case SccConflict:                          return "TooManyResults";
  case SccContentLengthRequired:             return "ContentLengthRequired";
  case SccRequestEntityTooLarge:             return "RequestEntityTooLarge";
  case SccUnsupportedMediaType:              return "UnsupportedMediaType";
  case SccInvalidModification:               return "InvalidModification";
  case SccReceiverInternalError:             return "InternalServerError";
  case SccNotImplemented:                    return "NotImplemented";
  default:                                   return "Undefined";
  }
}
